#include "alarm.h"
#include <Arduino.h>
#include "../../config.h"

static AlarmLogic gLogic(DEFAULT_THRESHOLD_C, ALARM_HYSTERESIS_C);
static uint32_t gStateSince = 0; 
static bool gSensorFault = false;

static bool gButtonRaw = false;
static bool gButtonStable = false;
static uint32_t gButtonChangedAt = 0;

static void writeOutputs(bool green, bool red, bool buzzer) {
  digitalWrite(PIN_LED_GREEN, green ? HIGH : LOW);
  digitalWrite(PIN_LED_RED, red ? HIGH : LOW);
  digitalWrite(PIN_BUZZER, buzzer == BUZZER_ACTIVE_HIGH ? HIGH : LOW);
}

static void logChange(bool changed) {
  if (!changed) return;
  gStateSince = millis();
  Serial.printf("[ALARM] %s (ambang %.1f C)\n", alarmStateName(gLogic.state()), gLogic.threshold());
}

void alarmBegin(float threshold) {
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  writeOutputs(false, false, false);
  gLogic.setThreshold(threshold);
}

void alarmOnTemperature(float tempC, bool valid) { logChange(gLogic.update(tempC, valid)); }
void alarmSetThreshold(float threshold) { logChange(gLogic.setThreshold(threshold)); }
void alarmSetSensorFault(bool fault) { gSensorFault = fault; }
AlarmState alarmState() { return gLogic.state(); }
float alarmThreshold() { return gLogic.threshold(); }

bool alarmAcknowledge() {
  const bool changed = gLogic.acknowledge();
  logChange(changed);
  return changed;
}

static bool buttonPressed(uint32_t now) {
  const bool raw = digitalRead(PIN_BUTTON) == LOW;
  if (raw != gButtonRaw) {
    gButtonRaw = raw;
    gButtonChangedAt = now;
  }
  if (gButtonRaw == gButtonStable || now - gButtonChangedAt < BUTTON_DEBOUNCE_MS) return false;
  gButtonStable = gButtonRaw;
  return gButtonStable;
}

bool alarmTick(uint32_t now) {
  const bool pressed = buttonPressed(now);

  const uint32_t t = now - gStateSince;
  switch (gLogic.state()) {
    case AlarmState::Active: 
      writeOutputs(false, true, t % 800 < 400);
      break;
    case AlarmState::Acknowledged:  
      writeOutputs(false, (t / 500) % 2 == 0, false);
      break;
    default: 
      writeOutputs(gSensorFault ? (now / 150) % 2 == 0 : true, false, false);
      break;
  }
  return pressed;
}
