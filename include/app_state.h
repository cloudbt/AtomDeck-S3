#pragma once

#include <Arduino.h>

struct AppState {
  String deviceId;
  String hostName;
  bool setupMode = false;
  uint32_t armedUntil = 0;

  bool isArmed() const {
    return armedUntil != 0 && static_cast<int32_t>(armedUntil - millis()) > 0;
  }

  uint32_t armedSeconds() const {
    if (!isArmed()) return 0;
    return (armedUntil - millis() + 999) / 1000;
  }

  void arm(uint32_t durationMs) { armedUntil = millis() + durationMs; }
  void disarm() { armedUntil = 0; }
};
