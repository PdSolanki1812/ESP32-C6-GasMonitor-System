#pragma once

#include <Arduino.h>
#include <DHT11.h>

struct SensorData {
    float mq6_ppm; 
    float mq9_ppm;      
    float temperature;  
    float humidity;    
    bool  dht_ok;  
};

void sensors_init();
bool sensors_read(SensorData &out);
bool sensors_get_latest(SensorData &out);
