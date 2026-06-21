#pragma once

#define WIFI_SSID       "WIFI_SSID"
#define WIFI_PASSWORD   "WIFI_PASSWORD"

#define TB_HOST         "mqtt.thingsboard.cloud"
#define TB_PORT         1883
#define TB_TOKEN        "TB_TOKEN"
#define TB_TOPIC        "v1/devices/me/telemetry"
#define TB_TOPIC_ALARM  "v1/devices/me/telemetry"

#define PIN_MQ6         1    // A0 
#define PIN_MQ9         3    // A2
#define PIN_MQ6_DO      5    // D4
#define PIN_MQ9_DO      6    // D5
#define PIN_DHT11       7    // D8

#define SENSOR_READ_INTERVAL_MS   1000
#define MQTT_PUBLISH_INTERVAL_MS  2000

#define STACK_SENSOR    3072
#define STACK_MQTT      4096

#define ADC_VREF        3.3f
#define ADC_RESOLUTION  4095.0f

#define MQ_RL_VALUE     10.0f

#define MQ6_ALARM_PPM   1000
#define MQ9_ALARM_PPM   50
