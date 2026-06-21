#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "config.h"
#include "sensor.h"
#include "mqtt_handler.h"

static TaskHandle_t h_sensor_task = nullptr;
static TaskHandle_t h_mqtt_task   = nullptr;
static TaskHandle_t h_alarm_task  = nullptr;

static SemaphoreHandle_t s_alarm_sem    = nullptr;
static volatile uint8_t  s_alarm_source = 0;   // bit0 = MQ6, bit1 = MQ9

static void IRAM_ATTR isr_mq6_do() {
    s_alarm_source |= (1 << 0);
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_alarm_sem, &woken);
    portYIELD_FROM_ISR(woken);
}

static void IRAM_ATTR isr_mq9_do() {
    s_alarm_source |= (1 << 1);
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_alarm_sem, &woken);
    portYIELD_FROM_ISR(woken);
}

static void sensor_task(void *param) {
    (void)param;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        SensorData data;
        bool ok = sensors_read(data);

        Serial.printf("[SENSOR] MQ6: %.1f ppm | MQ9: %.1f ppm | Temp: %.1f C | Hum: %.1f %%\n",
                      data.mq6_ppm, data.mq9_ppm, data.temperature, data.humidity);

        if (!ok) {
            Serial.println("[SENSOR] Warning: DHT11 returned bad data this cycle");
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

static void alarm_task(void *param) {
    (void)param;

    for (;;) {
        if (xSemaphoreTake(s_alarm_sem, portMAX_DELAY) == pdTRUE) {

            uint8_t triggered = s_alarm_source;
            s_alarm_source    = 0;

            Serial.printf("[ALARM] DO interrupt fired! Source mask: 0x%02X\n", triggered);

            SensorData data;
            bool has_data = sensors_get_latest(data);

            if (triggered & (1 << 0)) {
                Serial.println("[ALARM] MQ6 — LPG/Butane threshold exceeded! EMERGENCY");
                mqtt_publish_alarm("MQ6", has_data ? data.mq6_ppm : -1.0f);
            }

            if (triggered & (1 << 1)) {
                Serial.println("[ALARM] MQ9 — CO threshold exceeded! EMERGENCY");
                mqtt_publish_alarm("MQ9", has_data ? data.mq9_ppm : -1.0f);
            }

            // Debounce — MQ DO can bounce for a few ms after edge
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

static void mqtt_task(void *param) {
    (void)param;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        wifi_ensure_connected();
        mqtt_loop();

        SensorData data;
        if (sensors_get_latest(data)) {
            mqtt_publish(data.mq6_ppm, data.mq9_ppm, data.temperature, data.humidity);
        } else {
            Serial.println("[MQTT] No sensor data ready yet, skipping publish");
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(MQTT_PUBLISH_INTERVAL_MS));
    }
}

static void wifi_connect() {
    Serial.printf("[WIFI] Connecting to \"%s\" ", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }

    Serial.printf("\n[WIFI] Connected — IP: %s\n", WiFi.localIP().toString().c_str());
}

static void wifi_ensure_connected() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.println("[WIFI] Connection lost, reconnecting...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print('.');
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WIFI] Reconnected — IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[WIFI] Reconnect failed, will retry next cycle");
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ESP32-C6 Gas & Environment Monitor ===");

    sensors_init();

    s_alarm_sem = xSemaphoreCreateBinary();
    configASSERT(s_alarm_sem);

    pinMode(PIN_MQ6_DO, INPUT_PULLUP);
    pinMode(PIN_MQ9_DO, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_MQ6_DO), isr_mq6_do, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_MQ9_DO), isr_mq9_do, FALLING);
    Serial.println("[MAIN] DO pin interrupts attached");

    wifi_connect();
    mqtt_init();

    xTaskCreate(sensor_task, "sensor_task", STACK_SENSOR, nullptr, 2, &h_sensor_task);
    xTaskCreate(alarm_task,  "alarm_task",  STACK_SENSOR, nullptr, 3, &h_alarm_task);
    xTaskCreate(mqtt_task,   "mqtt_task",   STACK_MQTT,   nullptr, 1, &h_mqtt_task);

    Serial.println("[MAIN] Tasks started.");
}

void loop() {
    // All work is done inside FreeRTOS tasks.
    vTaskDelay(pdMS_TO_TICKS(10000));
}
