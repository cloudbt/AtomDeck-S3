#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>

#include "app_state.h"

class WifiManager {
 public:
  explicit WifiManager(AppState& state) : state_(state) {}

  bool begin(bool forceSetup, String& error);
  void loop();
  bool saveCredentials(const String& ssid, const String& password, String& error);
  void clearCredentials();
  bool connected() const;
  String ssid() const;
  String ip() const;
  bool setupExpired() const { return setupExpired_; }

 private:
  AppState& state_;
  Preferences prefs_;
  DNSServer dns_;
  uint32_t setupStartedAt_ = 0;
  bool setupExpired_ = false;

  void startSetupAp();
};
