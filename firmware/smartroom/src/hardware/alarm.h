#pragma once
#include <stdint.h>
#include "alarm_logic.h"

// LED buzzer, dan button
void alarmBegin(float threshold);
bool alarmTick(uint32_t now);  

void alarmOnTemperature(float tempC, bool valid);
bool alarmAcknowledge();  
void alarmSetThreshold(float threshold);
void alarmSetSensorFault(bool fault);

AlarmState alarmState();
float alarmThreshold();
