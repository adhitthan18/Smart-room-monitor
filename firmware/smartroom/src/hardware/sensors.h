#pragma once
#include <stdint.h>

struct SensorReadings {
  bool dhtOk = false;          
  float temperature = 0;     
  float humidity = 0;          
  uint8_t dhtFailStreak = 0;   
  uint8_t lightPct = 0;        
  bool pirReady = false;       
  bool motion = false;
};

void sensorsBegin();
bool sensorsTick(uint32_t now); 
const SensorReadings &sensors();
