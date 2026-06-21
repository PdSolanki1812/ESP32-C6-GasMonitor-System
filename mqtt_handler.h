#pragma once

#include <Arduino.h>

void mqtt_init();
bool mqtt_publish(float mq6_ppm, float mq9_ppm, float temperature, float humidity);
void mqtt_loop();
bool mqtt_publish_alarm(const char *source, float ppm);
