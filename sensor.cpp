#include "sensor.h"
#include "config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

//DHT11
static DHT11 dht11(PIN_DHT11);

//Shared state
static SensorData    s_latest   = {};
static bool          s_has_data = false;
static SemaphoreHandle_t s_mutex = nullptr;

//convert MQ raw ADC to approximate ppm

static float adc_to_ppm_mq6(int raw) {
    if (raw <= 0) return 0.0f;

    float vout = raw * (ADC_VREF / ADC_RESOLUTION);
    if (vout <= 0.0f) return 0.0f;

    float rs    = MQ_RL_VALUE * (ADC_VREF - vout) / vout;
    const float R0 = 10.0f;
    float ratio = rs / R0;

    if (ratio <= 0.0f) return 0.0f;
    float ppm = 1000.0f * powf(ratio, -2.3f);
    return ppm;
}

static float adc_to_ppm_mq9(int raw) {
    if (raw <= 0) return 0.0f;

    float vout = raw * (ADC_VREF / ADC_RESOLUTION);
    if (vout <= 0.0f) return 0.0f;

    float rs    = MQ_RL_VALUE * (ADC_VREF - vout) / vout;
    // R0 in clean air ≈ 9.8 kΩ for MQ9
    const float R0 = 9.8f;
    float ratio = rs / R0;

    if (ratio <= 0.0f) return 0.0f;
    float ppm = 100.0f * powf(ratio, -1.5f);
    return ppm;
}

void sensors_init() {
    s_mutex = xSemaphoreCreateMutex();
    configASSERT(s_mutex);

    analogSetPinAttenuation(PIN_MQ6, ADC_ATTENDB_MAX);
    analogSetPinAttenuation(PIN_MQ9, ADC_ATTENDB_MAX);

    Serial.println("[SENSOR] Initialized MQ6, MQ9, DHT11");
}

bool sensors_read(SensorData &out) {
    // Gas sensors — straightforward analogRead
    int raw_mq6 = analogRead(PIN_MQ6);
    int raw_mq9 = analogRead(PIN_MQ9);

    out.mq6_ppm = adc_to_ppm_mq6(raw_mq6);
    out.mq9_ppm = adc_to_ppm_mq9(raw_mq9);

    int temperature = 0;
    int humidity = 0;

    int result = dht11.readTemperatureHumidity(temperature, humidity);

    if (result != 0) {
        out.temperature = 0.0f;
        out.humidity    = 0.0f;
        out.dht_ok      = false;
        Serial.printf("[SENSOR] DHT11 read failed (err %d)\n", result);
    } else {
        out.temperature = static_cast<float>(temperature);
        out.humidity    = static_cast<float>(humidity);
        out.dht_ok      = true;
    }

    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        s_latest   = out;
        s_has_data = true;
        xSemaphoreGive(s_mutex);
    }

    return out.dht_ok;
}

bool sensors_get_latest(SensorData &out) {
    if (!s_mutex) return false;

    bool ok = false;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (s_has_data) {
            out = s_latest;
            ok  = true;
        }
        xSemaphoreGive(s_mutex);
    }
    return ok;
}
