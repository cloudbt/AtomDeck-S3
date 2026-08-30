#include "macro_store.h"

#include <LittleFS.h>
#include <esp_system.h>

#include "config.h"
#include "hid_executor.h"

bool MacroStore::begin(String& error) {
  if (!LittleFS.begin(true)) {
    error = "LittleFS mount failed";
    return false;
  }
  if (!LittleFS.exists(PATH) && LittleFS.exists(BAK_PATH)) {
    LittleFS.rename(BAK_PATH, PATH);
  }
  if (!LittleFS.exists(PATH)) {
    DynamicJsonDocument empty(256);
    empty.to<JsonArray>();
    return save(empty, error);
  }
  DynamicJsonDocument check(atomdeck::STORE_DOC_CAPACITY);
  return load(check, error);
}

bool MacroStore::load(DynamicJsonDocument& doc, String& error) {
  File file = LittleFS.open(PATH, "r");
  if (!file) {
    error = "macro store unavailable";
    return false;
  }
  if (file.size() > atomdeck::MAX_STORE_BYTES) {
    file.close();
    error = "macro store exceeds size limit";
    return false;
  }
  const DeserializationError jsonError = deserializeJson(doc, file);
  file.close();
  if (jsonError || !doc.is<JsonArray>()) {
    error = "macro store is invalid JSON";
    return false;
  }
  return true;
}

bool MacroStore::save(const DynamicJsonDocument& doc, String& error) {
  File temp = LittleFS.open(TMP_PATH, "w");
  if (!temp) {
    error = "cannot open temporary macro store";
    return false;
  }
  if (serializeJson(doc, temp) == 0) {
    temp.close();
    LittleFS.remove(TMP_PATH);
    error = "cannot serialize macro store";
    return false;
  }
  temp.flush();
  temp.close();

  LittleFS.remove(BAK_PATH);
  if (LittleFS.exists(PATH) && !LittleFS.rename(PATH, BAK_PATH)) {
    LittleFS.remove(TMP_PATH);
    error = "cannot back up macro store";
    return false;
  }
  if (!LittleFS.rename(TMP_PATH, PATH)) {
    if (LittleFS.exists(BAK_PATH)) LittleFS.rename(BAK_PATH, PATH);
    error = "cannot activate macro store update";
    return false;
  }
  LittleFS.remove(BAK_PATH);
  return true;
}

bool MacroStore::validate(JsonObjectConst macro, String& error) {
  const String name = macro["name"] | "";
  if (name.isEmpty() || name.length() > atomdeck::MAX_NAME_BYTES) {
    error = "name must contain 1-48 bytes";
    return false;
  }
  const String icon = macro["icon"] | "";
  if (icon.length() > atomdeck::MAX_ICON_BYTES) {
    error = "icon must contain no more than 16 bytes";
    return false;
  }
  const String color = macro["color"] | "blue";
  if (color != "blue" && color != "purple" && color != "green" && color != "orange" &&
      color != "pink" && color != "cyan") {
    error = "invalid card color";
    return false;
  }
  const String kind = macro["kind"] | "macro";
  if (kind != "launcher" && kind != "hotkey" && kind != "text" && kind != "macro") {
    error = "invalid card kind";
    return false;
  }
  if (!macro["actions"].is<JsonArrayConst>()) {
    error = "actions must be an array";
    return false;
  }
  const JsonArrayConst actions = macro["actions"].as<JsonArrayConst>();
  if (actions.isNull() || actions.size() == 0 || actions.size() > atomdeck::MAX_ACTIONS) {
    error = "actions must contain 1-32 items";
    return false;
  }

  uint32_t totalDelay = 0;
  for (JsonVariantConst item : actions) {
    if (!item.is<JsonObjectConst>()) {
      error = "each action must be an object";
      return false;
    }
    const JsonObjectConst action = item.as<JsonObjectConst>();
    const String type = action["type"] | "";
    if (type == "text") {
      const String value = action["value"] | "";
      if (value.isEmpty() || value.length() > atomdeck::MAX_TEXT_BYTES) {
        error = "text action must contain 1-256 bytes";
        return false;
      }
    } else if (type == "key") {
      if (!HidExecutor::validKeyName(String(action["value"] | ""))) {
        error = "invalid key action";
        return false;
      }
    } else if (type == "chord") {
      if (!HidExecutor::validChord(String(action["value"] | ""))) {
        error = "invalid chord action";
        return false;
      }
    } else if (type == "delay") {
      const uint32_t ms = action["ms"] | 0;
      if (ms > atomdeck::MAX_DELAY_MS || totalDelay + ms > atomdeck::MAX_TOTAL_DELAY_MS) {
        error = "delay limit exceeded";
        return false;
      }
      totalDelay += ms;
    } else if (type == "mouse_move") {
      const int x = action["x"] | 0;
      const int y = action["y"] | 0;
      if (x < -127 || x > 127 || y < -127 || y > 127) {
        error = "mouse movement must be between -127 and 127";
        return false;
      }
    } else if (type == "mouse_click") {
      String button = action["button"] | "";
      button.toUpperCase();
      if (button != "LEFT" && button != "RIGHT" && button != "MIDDLE") {
        error = "invalid mouse button";
        return false;
      }
    } else if (type == "scroll") {
      const int amount = action["amount"] | 0;
      if (amount < -20 || amount > 20) {
        error = "scroll amount must be between -20 and 20";
        return false;
      }
    } else {
      error = "unsupported action type";
      return false;
    }
  }
  return true;
}

bool MacroStore::parseAndValidate(const String& body, DynamicJsonDocument& macro, String& error) {
  if (body.isEmpty() || body.length() > atomdeck::MACRO_DOC_CAPACITY) {
    error = "request body is empty or too large";
    return false;
  }
  const DeserializationError jsonError = deserializeJson(macro, body);
  if (jsonError || !macro.is<JsonObject>()) {
    error = "request body must be a JSON object";
    return false;
  }
  return validate(macro.as<JsonObjectConst>(), error);
}

int MacroStore::findIndex(JsonArrayConst macros, const String& id) {
  for (size_t i = 0; i < macros.size(); ++i) {
    if (String(macros[i]["id"] | "") == id) return static_cast<int>(i);
  }
  return -1;
}

String MacroStore::newId() {
  char id[17];
  snprintf(id, sizeof(id), "%08lx%08lx", static_cast<unsigned long>(esp_random()),
           static_cast<unsigned long>(esp_random()));
  return String(id);
}

bool MacroStore::list(String& json, String& error) {
  DynamicJsonDocument doc(atomdeck::STORE_DOC_CAPACITY);
  if (!load(doc, error)) return false;
  serializeJson(doc, json);
  return true;
}

bool MacroStore::create(const String& body, String& json, String& error) {
  DynamicJsonDocument input(atomdeck::MACRO_DOC_CAPACITY);
  if (!parseAndValidate(body, input, error)) return false;

  DynamicJsonDocument store(atomdeck::STORE_DOC_CAPACITY);
  if (!load(store, error)) return false;
  JsonArray macros = store.as<JsonArray>();
  if (macros.size() >= atomdeck::MAX_MACROS) {
    error = "macro limit reached";
    return false;
  }
  JsonObject created = macros.createNestedObject();
  created["id"] = newId();
  created["name"] = input["name"].as<String>();
  created["icon"] = input["icon"] | "⚡";
  created["color"] = input["color"] | "blue";
  created["kind"] = input["kind"] | "macro";
  created["actions"] = input["actions"];
  if (!save(store, error)) return false;
  serializeJson(created, json);
  return true;
}

bool MacroStore::replace(const String& id, const String& body, String& json, String& error) {
  DynamicJsonDocument input(atomdeck::MACRO_DOC_CAPACITY);
  if (!parseAndValidate(body, input, error)) return false;
  DynamicJsonDocument store(atomdeck::STORE_DOC_CAPACITY);
  if (!load(store, error)) return false;
  JsonArray macros = store.as<JsonArray>();
  const JsonArrayConst macroView = macros;
  const int index = findIndex(macroView, id);
  if (index < 0) {
    error = "macro not found";
    return false;
  }
  JsonObject target = macros[index].as<JsonObject>();
  target.clear();
  target["id"] = id;
  target["name"] = input["name"].as<String>();
  target["icon"] = input["icon"] | "⚡";
  target["color"] = input["color"] | "blue";
  target["kind"] = input["kind"] | "macro";
  target["actions"] = input["actions"];
  if (!save(store, error)) return false;
  serializeJson(target, json);
  return true;
}

bool MacroStore::remove(const String& id, String& error) {
  DynamicJsonDocument store(atomdeck::STORE_DOC_CAPACITY);
  if (!load(store, error)) return false;
  JsonArray macros = store.as<JsonArray>();
  const JsonArrayConst macroView = macros;
  const int index = findIndex(macroView, id);
  if (index < 0) {
    error = "macro not found";
    return false;
  }
  macros.remove(index);
  return save(store, error);
}

bool MacroStore::get(const String& id, DynamicJsonDocument& output, String& error) {
  DynamicJsonDocument store(atomdeck::STORE_DOC_CAPACITY);
  if (!load(store, error)) return false;
  JsonArrayConst macros = store.as<JsonArrayConst>();
  const int index = findIndex(macros, id);
  if (index < 0) {
    error = "macro not found";
    return false;
  }
  output.set(macros[index]);
  return true;
}
