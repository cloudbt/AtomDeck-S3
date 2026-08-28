#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class MacroStore {
 public:
  bool begin(String& error);
  bool list(String& json, String& error);
  bool create(const String& body, String& json, String& error);
  bool replace(const String& id, const String& body, String& json, String& error);
  bool remove(const String& id, String& error);
  bool get(const String& id, DynamicJsonDocument& output, String& error);

 private:
  static constexpr const char* PATH = "/macros.json";
  static constexpr const char* TMP_PATH = "/macros.tmp";
  static constexpr const char* BAK_PATH = "/macros.bak";

  bool load(DynamicJsonDocument& doc, String& error);
  bool save(const DynamicJsonDocument& doc, String& error);
  bool parseAndValidate(const String& body, DynamicJsonDocument& macro, String& error);
  bool validate(JsonObjectConst macro, String& error);
  int findIndex(JsonArrayConst macros, const String& id);
  String newId();
};
