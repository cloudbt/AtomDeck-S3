#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>

class HidExecutor {
 public:
  void begin();
  bool typeText(const String& text, String& error);
  bool moveMouse(int x, int y, String& error);
  bool clickMouse(const String& button, String& error);
  bool scrollMouse(int amount, String& error);
  bool run(JsonArrayConst actions, String& error);

  static bool validKeyName(const String& name);
  static bool validChord(const String& chord);

 private:
  USBHIDKeyboard keyboard_;
  USBHIDMouse mouse_;

  bool pressNamedKey(const String& name);
  bool runChord(const String& chord, String& error);
};
