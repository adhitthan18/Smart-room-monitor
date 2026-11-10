#pragma once
#include <stdint.h>

enum class AlarmState : uint8_t { Normal, Active, Acknowledged };

class AlarmLogic {
 public:
  AlarmLogic(float threshold, float hysteresis) : threshold_(threshold), hysteresis_(hysteresis) {}

  bool update(float tempC, bool valid) {
    if (!valid) return false;
    lastTemp_ = tempC;
    hasTemp_ = true;
    return evaluate();
  }

  bool acknowledge() {
    if (state_ != AlarmState::Active) return false;
    state_ = AlarmState::Acknowledged;
    return true;
  }

  bool setThreshold(float threshold) {
    threshold_ = threshold;
    return hasTemp_ && evaluate();
  }

  AlarmState state() const { return state_; }
  float threshold() const { return threshold_; }

 private:
  bool evaluate() {
    const AlarmState before = state_;
    if (state_ == AlarmState::Normal) {
      if (lastTemp_ > threshold_) state_ = AlarmState::Active;
    } else if (lastTemp_ < threshold_ - hysteresis_) {
      state_ = AlarmState::Normal;
    }
    return state_ != before;
  }

  float threshold_;
  float hysteresis_;
  float lastTemp_ = 0.0f;
  bool hasTemp_ = false;
  AlarmState state_ = AlarmState::Normal;
};

inline const char *alarmStateName(AlarmState s) {
  switch (s) {
    case AlarmState::Active:       return "active";
    case AlarmState::Acknowledged: return "acknowledged";
    default:                       return "normal";
  }
}
