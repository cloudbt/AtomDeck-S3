#include "wifi_manager.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <esp_mac.h>

#include "config.h"

bool WifiManager::begin(bool forceSetup, String& error) {
  if (!prefs_.begin("atomdeck", false)) {
    error = "cannot open NVS preferences";
    return false;
  }

  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
  state_.deviceId = suffix;
  state_.hostName = String("atomdeck-") + String(suffix).substring(0);
  state_.hostName.toLowerCase();

  const String savedSsid = prefs_.getString("ssid", "");
  String savedPassword = prefs_.getString("password", "");
  if (!forceSetup && !savedSsid.isEmpty()) {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(state_.hostName.c_str());
    WiFi.begin(savedSsid.c_str(), savedPassword.c_str());
    const uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < atomdeck::WIFI_CONNECT_TIMEOUT_MS) {
      delay(100);
    }
    savedPassword.clear();
    if (WiFi.status() == WL_CONNECTED) {
      state_.setupMode = false;
      MDNS.begin(state_.hostName.c_str());
      MDNS.addService("http", "tcp", 80);
      return true;
    }
    WiFi.disconnect(true, true);
  }

  startSetupAp();
  return true;
}

void WifiManager::startSetupAp() {
  state_.setupMode = true;
  setupExpired_ = false;
  setupStartedAt_ = millis();
  WiFi.mode(WIFI_AP);
  const String apSsid = String("AtomDeck-Setup-") + state_.deviceId;
  WiFi.softAP(apSsid.c_str());
  dns_.start(53, "*", WiFi.softAPIP());
}

void WifiManager::loop() {
  if (!state_.setupMode) return;
  dns_.processNextRequest();
  if (!setupExpired_ && millis() - setupStartedAt_ >= atomdeck::SETUP_AP_TIMEOUT_MS) {
    dns_.stop();
    WiFi.softAPdisconnect(true);
    setupExpired_ = true;
  }
}

bool WifiManager::saveCredentials(const String& inputSsid, const String& password, String& error) {
  if (!state_.setupMode || setupExpired_) {
    error = "Wi-Fi provisioning is available only in active setup mode";
    return false;
  }
  String ssidValue = inputSsid;
  ssidValue.trim();
  if (ssidValue.isEmpty() || ssidValue.length() > 32) {
    error = "SSID must contain 1-32 bytes";
    return false;
  }
  if (password.length() > 63 || (!password.isEmpty() && password.length() < 8)) {
    error = "password must be empty or contain 8-63 bytes";
    return false;
  }
  if (prefs_.putString("ssid", ssidValue) == 0) {
    error = "failed to save SSID";
    return false;
  }
  if (prefs_.putString("password", password) == 0 && !password.isEmpty()) {
    prefs_.remove("ssid");
    error = "failed to save password";
    return false;
  }
  return true;
}

void WifiManager::clearCredentials() {
  prefs_.remove("ssid");
  prefs_.remove("password");
}

bool WifiManager::connected() const { return WiFi.status() == WL_CONNECTED; }

String WifiManager::ssid() const {
  if (state_.setupMode) return String("AtomDeck-Setup-") + state_.deviceId;
  return connected() ? WiFi.SSID() : String();
}

String WifiManager::ip() const {
  if (state_.setupMode && !setupExpired_) return WiFi.softAPIP().toString();
  return connected() ? WiFi.localIP().toString() : String("0.0.0.0");
}
