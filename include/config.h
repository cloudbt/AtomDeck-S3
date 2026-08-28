#pragma once

#include <Arduino.h>

namespace atomdeck {

constexpr const char* VERSION = "0.1.0";
constexpr uint8_t BUTTON_PIN = 41;
constexpr uint32_t ARM_WINDOW_MS = 60000;
constexpr uint32_t LONG_PRESS_RESET_MS = 8000;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t SETUP_AP_TIMEOUT_MS = 10 * 60000;

constexpr size_t MAX_MACROS = 20;
constexpr size_t MAX_ACTIONS = 32;
constexpr size_t MAX_NAME_BYTES = 48;
constexpr size_t MAX_TEXT_BYTES = 256;
constexpr uint32_t MAX_DELAY_MS = 3000;
constexpr uint32_t MAX_TOTAL_DELAY_MS = 15000;
constexpr size_t MAX_STORE_BYTES = 32 * 1024;
constexpr size_t STORE_DOC_CAPACITY = 48 * 1024;
constexpr size_t MACRO_DOC_CAPACITY = 16 * 1024;

}  // namespace atomdeck
