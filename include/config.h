#pragma once

#include <Arduino.h>

namespace atomdeck {

constexpr const char* VERSION = "0.2.1";
constexpr uint8_t BUTTON_PIN = 41;
constexpr uint32_t LONG_PRESS_RESET_MS = 8000;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t SETUP_AP_TIMEOUT_MS = 10 * 60000;

constexpr size_t MAX_MACROS = 20;
constexpr size_t MAX_ACTIONS = 32;
constexpr size_t MAX_NAME_BYTES = 48;
constexpr size_t MAX_ICON_BYTES = 16;
constexpr size_t MAX_TEXT_BYTES = 256;
constexpr uint32_t MAX_DELAY_MS = 3000;
constexpr uint32_t MAX_TOTAL_DELAY_MS = 15000;
constexpr size_t MAX_STORE_BYTES = 32 * 1024;
constexpr size_t STORE_DOC_CAPACITY = 48 * 1024;
constexpr size_t MACRO_DOC_CAPACITY = 16 * 1024;

constexpr size_t MIN_ADMIN_PASSWORD_BYTES = 12;
constexpr size_t MAX_ADMIN_PASSWORD_BYTES = 72;
constexpr uint32_t PBKDF2_ITERATIONS = 60000;
constexpr uint32_t SESSION_TTL_MS = 60 * 60 * 1000;
constexpr uint32_t LOGIN_BLOCK_MS = 60 * 1000;
constexpr uint8_t MAX_LOGIN_FAILURES = 5;
constexpr size_t MAX_SESSIONS = 4;
constexpr size_t MAX_TOKENS = 4;

}  // namespace atomdeck
