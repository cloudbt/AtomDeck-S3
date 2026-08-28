#include "hid_executor.h"

#include "config.h"

namespace {

bool isSingleKey(const String& name) {
  if (name.length() != 1) return false;
  const char c = name.charAt(0);
  return isAlphaNumeric(c);
}

bool isModifier(const String& name) {
  return name == "CTRL" || name == "ALT" || name == "SHIFT" || name == "GUI";
}

uint8_t namedKeyCode(const String& name) {
  if (name == "ENTER") return KEY_RETURN;
  if (name == "TAB") return KEY_TAB;
  if (name == "ESC") return KEY_ESC;
  if (name == "BACKSPACE") return KEY_BACKSPACE;
  if (name == "DELETE") return KEY_DELETE;
  if (name == "UP") return KEY_UP_ARROW;
  if (name == "DOWN") return KEY_DOWN_ARROW;
  if (name == "LEFT") return KEY_LEFT_ARROW;
  if (name == "RIGHT") return KEY_RIGHT_ARROW;
  if (name == "HOME") return KEY_HOME;
  if (name == "END") return KEY_END;
  if (name == "PAGEUP") return KEY_PAGE_UP;
  if (name == "PAGEDOWN") return KEY_PAGE_DOWN;
  if (name == "SPACE") return ' ';
  return 0;
}

uint8_t modifierCode(const String& name) {
  if (name == "CTRL") return KEY_LEFT_CTRL;
  if (name == "ALT") return KEY_LEFT_ALT;
  if (name == "SHIFT") return KEY_LEFT_SHIFT;
  if (name == "GUI") return KEY_LEFT_GUI;
  return 0;
}

}  // namespace

void HidExecutor::begin() {
  keyboard_.begin();
  mouse_.begin();
  USB.begin();
}

bool HidExecutor::validKeyName(const String& input) {
  String name = input;
  name.trim();
  name.toUpperCase();
  return isSingleKey(name) || namedKeyCode(name) != 0;
}

bool HidExecutor::validChord(const String& input) {
  String chord = input;
  chord.trim();
  chord.toUpperCase();
  if (chord.isEmpty() || chord.length() > 32) return false;

  int start = 0;
  int count = 0;
  int regularKeys = 0;
  while (start < chord.length()) {
    const int plus = chord.indexOf('+', start);
    String token = plus < 0 ? chord.substring(start) : chord.substring(start, plus);
    token.trim();
    if (token.isEmpty()) return false;
    ++count;
    if (count > 4) return false;
    if (!isModifier(token)) {
      if (!validKeyName(token)) return false;
      ++regularKeys;
    }
    if (plus < 0) break;
    start = plus + 1;
  }
  return regularKeys == 1;
}

bool HidExecutor::pressNamedKey(const String& input) {
  String name = input;
  name.trim();
  name.toUpperCase();
  if (isSingleKey(name)) {
    keyboard_.write(static_cast<uint8_t>(tolower(name.charAt(0))));
    return true;
  }
  const uint8_t code = namedKeyCode(name);
  if (code == 0) return false;
  keyboard_.write(code);
  return true;
}

bool HidExecutor::runChord(const String& input, String& error) {
  if (!validChord(input)) {
    error = "invalid chord";
    return false;
  }

  String chord = input;
  chord.trim();
  chord.toUpperCase();
  String regularKey;
  int start = 0;
  while (start < chord.length()) {
    const int plus = chord.indexOf('+', start);
    String token = plus < 0 ? chord.substring(start) : chord.substring(start, plus);
    token.trim();
    if (isModifier(token)) {
      keyboard_.press(modifierCode(token));
    } else {
      regularKey = token;
    }
    if (plus < 0) break;
    start = plus + 1;
  }

  if (isSingleKey(regularKey)) {
    keyboard_.press(static_cast<uint8_t>(tolower(regularKey.charAt(0))));
  } else {
    keyboard_.press(namedKeyCode(regularKey));
  }
  delay(70);
  keyboard_.releaseAll();
  return true;
}

bool HidExecutor::typeText(const String& text, String& error) {
  if (text.isEmpty() || text.length() > atomdeck::MAX_TEXT_BYTES) {
    error = "text must contain 1-256 bytes";
    return false;
  }
  keyboard_.print(text);
  return true;
}

bool HidExecutor::moveMouse(int x, int y, String& error) {
  if (x < -127 || x > 127 || y < -127 || y > 127) {
    error = "mouse movement must be between -127 and 127";
    return false;
  }
  mouse_.move(static_cast<int8_t>(x), static_cast<int8_t>(y));
  return true;
}

bool HidExecutor::clickMouse(const String& input, String& error) {
  String button = input;
  button.trim();
  button.toUpperCase();
  if (button == "LEFT") mouse_.click(MOUSE_LEFT);
  else if (button == "RIGHT") mouse_.click(MOUSE_RIGHT);
  else if (button == "MIDDLE") mouse_.click(MOUSE_MIDDLE);
  else {
    error = "mouse button must be LEFT, RIGHT, or MIDDLE";
    return false;
  }
  return true;
}

bool HidExecutor::scrollMouse(int amount, String& error) {
  if (amount < -20 || amount > 20) {
    error = "scroll amount must be between -20 and 20";
    return false;
  }
  mouse_.move(0, 0, static_cast<int8_t>(amount));
  return true;
}

bool HidExecutor::run(JsonArrayConst actions, String& error) {
  uint32_t totalDelay = 0;
  for (JsonObjectConst action : actions) {
    const String type = action["type"] | "";
    if (type == "text") {
      if (!typeText(String(action["value"] | ""), error)) return false;
    } else if (type == "key") {
      if (!pressNamedKey(String(action["value"] | ""))) {
        error = "invalid key";
        return false;
      }
    } else if (type == "chord") {
      if (!runChord(String(action["value"] | ""), error)) return false;
    } else if (type == "delay") {
      const uint32_t ms = action["ms"] | 0;
      if (ms > atomdeck::MAX_DELAY_MS || totalDelay + ms > atomdeck::MAX_TOTAL_DELAY_MS) {
        error = "delay limit exceeded";
        return false;
      }
      totalDelay += ms;
      delay(ms);
    } else if (type == "mouse_move") {
      if (!moveMouse(action["x"] | 0, action["y"] | 0, error)) return false;
    } else if (type == "mouse_click") {
      if (!clickMouse(String(action["button"] | ""), error)) return false;
    } else if (type == "scroll") {
      if (!scrollMouse(action["amount"] | 0, error)) return false;
    } else {
      error = "unsupported action";
      return false;
    }
    delay(25);
  }
  keyboard_.releaseAll();
  return true;
}
