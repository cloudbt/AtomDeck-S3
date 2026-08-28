#include <Arduino.h>

#include "api_server.h"
#include "app_state.h"
#include "config.h"
#include "hid_executor.h"
#include "macro_store.h"
#include "wifi_manager.h"

AppState appState;
WifiManager wifiManager(appState);
MacroStore macroStore;
HidExecutor hidExecutor;
ApiServer apiServer(appState, wifiManager, macroStore, hidExecutor);

namespace {

bool buttonWasPressed = false;
bool ignoreFirstRelease = false;
uint32_t buttonPressedAt = 0;

void handleButton() {
  const bool pressed = digitalRead(atomdeck::BUTTON_PIN) == LOW;
  if (pressed && !buttonWasPressed) {
    buttonWasPressed = true;
    buttonPressedAt = millis();
    return;
  }
  if (pressed || !buttonWasPressed) return;

  buttonWasPressed = false;
  const uint32_t duration = millis() - buttonPressedAt;
  if (ignoreFirstRelease) {
    ignoreFirstRelease = false;
    return;
  }
  if (duration >= atomdeck::LONG_PRESS_RESET_MS) {
    appState.disarm();
    wifiManager.clearCredentials();
    delay(100);
    ESP.restart();
    return;
  }
  if (duration >= 35) {
    if (appState.isArmed()) appState.disarm();
    else appState.arm(atomdeck::ARM_WINDOW_MS);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(atomdeck::BUTTON_PIN, INPUT_PULLUP);
  delay(80);
  const bool forceSetup = digitalRead(atomdeck::BUTTON_PIN) == LOW;
  ignoreFirstRelease = forceSetup;

  String error;
  if (!macroStore.begin(error)) {
    Serial.println("Macro storage initialization failed");
  }
  if (!wifiManager.begin(forceSetup, error)) {
    Serial.println("Wi-Fi initialization failed");
  }

  hidExecutor.begin();
  apiServer.begin();
  Serial.printf("AtomDeck-S3 ready at http://%s/\n", wifiManager.ip().c_str());
}

void loop() {
  handleButton();
  wifiManager.loop();
  apiServer.loop();
  delay(2);
}
