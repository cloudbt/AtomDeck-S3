#pragma once

#include <Arduino.h>

struct AppState {
  String deviceId;
  String hostName;
  bool setupMode = false;
  bool armed = false;

  bool isArmed() const { return armed; }
  void arm() { armed = true; }
  void disarm() { armed = false; }
};
