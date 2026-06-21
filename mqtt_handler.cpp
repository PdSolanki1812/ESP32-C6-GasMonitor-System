#include "mqtt_handler.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>

static WiFiClient   s_wifi_client;
static PubSubClient s_mqtt(s_wifi_client);

static bool mqtt_reconnect() {
    int attempts = 0;

    while (!s_mqtt.connected()) {
        Serial.printf("[MQTT] Connecting to %s ... ", TB_HOST);

        if (s_mqtt.connect("espdev", "pd", "pdsolanki")) {
            Serial.println("connected");
            return true;
        }

        Serial.printf("failed (rc=%d), retrying in 5 s\n", s_mqtt.state());
        attempts++;

        if (attempts >= 10) {
            Serial.println("[MQTT] Too many failures, backing off");
            return false;
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    return true;
}

void mqtt_init() {
    s_mqtt.setServer(TB_HOST, TB_PORT);
    s_mqtt.setKeepAlive(60);
    Serial.println("[MQTT] Client configured");
}

bool mqtt_publish(float mq6_ppm, float mq9_ppm, float temperature, float humidity) {
    if (!s_mqtt.connected()) {
        if (!mqtt_reconnect()) return false;
    }

    char payload[200];
    snprintf(payload, sizeof(payload),
             "{\"mq6_ppm\":%.2f,\"mq9_ppm\":%.2f,\"temperature\":%.2f,\"humidity\":%.2f}",
             mq6_ppm, mq9_ppm, temperature, humidity);

    bool ok = s_mqtt.publish(TB_TOPIC, payload);
    if (ok) {
        Serial.printf("[MQTT] Published → %s\n", payload);
    } else {
        Serial.println("[MQTT] Publish failed");
    }
    return ok;
}

bool mqtt_publish_alarm(const char *source, float ppm) {
    if (!s_mqtt.connected()) {
        if (!mqtt_reconnect()) return false;
    }

    char payload[160];
    snprintf(payload, sizeof(payload),
             "{\"alarm\":true,\"source\":\"%s\",\"ppm\":%.2f}",
             source, ppm);

    bool ok = s_mqtt.publish(TB_TOPIC_ALARM, payload);
    if (ok) {
        Serial.printf("[MQTT] ALARM published → %s\n", payload);
    } else {
        Serial.println("[MQTT] ALARM publish failed");
    }
    return ok;
}

void mqtt_loop() {
    if (!s_mqtt.connected()) {
        mqtt_reconnect();
    }
    s_mqtt.loop();
}
