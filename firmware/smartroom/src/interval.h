#pragma once
#include <stdint.h>

class Interval {
 public:
  explicit Interval(uint32_t periodMs) : period_(periodMs) {}

  bool due(uint32_t now) {
    if (started_ && now - last_ < period_) return false;
    last_ = now;
    started_ = true;
    return true;
  }

  void trigger() { started_ = false; } 

 private:
  uint32_t period_;
  uint32_t last_ = 0;
  bool started_ = false;
};
