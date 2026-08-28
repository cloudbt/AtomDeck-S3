#include "auth_manager.h"

#include <ArduinoJson.h>
#include <esp_system.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/sha256.h>

#include "config.h"

namespace {

constexpr size_t SALT_BYTES = 16;
constexpr size_t HASH_BYTES = 32;

String tokenKey(size_t slot, const char* suffix) {
  return String("t") + String(slot) + suffix;
}

}  // namespace

bool AuthManager::begin(String& error) {
  if (!prefs_.begin("atom-auth", false)) {
    error = "cannot open authentication storage";
    return false;
  }
  invalidateSessions();
  return true;
}

bool AuthManager::configured() {
  return prefs_.getString("pw_hash", "").length() == HASH_BYTES * 2 &&
         prefs_.getString("pw_salt", "").length() == SALT_BYTES * 2 &&
         prefs_.getUInt("pw_iter", 0) > 0;
}

bool AuthManager::validatePasswordFormat(const String& password, String& error) const {
  if (password.length() < atomdeck::MIN_ADMIN_PASSWORD_BYTES ||
      password.length() > atomdeck::MAX_ADMIN_PASSWORD_BYTES) {
    error = "admin password must contain 12-72 bytes";
    return false;
  }
  bool hasLetter = false;
  bool hasOther = false;
  for (size_t i = 0; i < password.length(); ++i) {
    const char c = password.charAt(i);
    const bool letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    hasLetter = hasLetter || letter;
    hasOther = hasOther || !letter;
  }
  if (!hasLetter || !hasOther) {
    error = "admin password must combine letters with numbers, spaces, or symbols";
    return false;
  }
  return true;
}

String AuthManager::toHex(const uint8_t* data, size_t length) const {
  static const char* digits = "0123456789abcdef";
  String result;
  result.reserve(length * 2);
  for (size_t i = 0; i < length; ++i) {
    result += digits[(data[i] >> 4) & 0x0f];
    result += digits[data[i] & 0x0f];
  }
  return result;
}

bool AuthManager::fromHex(const String& input, uint8_t* output, size_t outputLength) const {
  if (input.length() != outputLength * 2) return false;
  for (size_t i = 0; i < outputLength; ++i) {
    const char high = input.charAt(i * 2);
    const char low = input.charAt(i * 2 + 1);
    auto nibble = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return -1;
    };
    const int h = nibble(high);
    const int l = nibble(low);
    if (h < 0 || l < 0) return false;
    output[i] = static_cast<uint8_t>((h << 4) | l);
  }
  return true;
}

String AuthManager::randomHex(size_t byteCount) const {
  uint8_t bytes[32] = {};
  if (byteCount > sizeof(bytes)) byteCount = sizeof(bytes);
  esp_fill_random(bytes, byteCount);
  return toHex(bytes, byteCount);
}

String AuthManager::sha256Hex(const String& value) const {
  uint8_t digest[HASH_BYTES] = {};
  mbedtls_sha256_ret(reinterpret_cast<const unsigned char*>(value.c_str()), value.length(), digest, 0);
  return toHex(digest, sizeof(digest));
}

bool AuthManager::constantEquals(const String& left, const String& right) const {
  const size_t maximum = max(left.length(), right.length());
  uint8_t difference = static_cast<uint8_t>(left.length() ^ right.length());
  for (size_t i = 0; i < maximum; ++i) {
    const uint8_t a = i < left.length() ? static_cast<uint8_t>(left.charAt(i)) : 0;
    const uint8_t b = i < right.length() ? static_cast<uint8_t>(right.charAt(i)) : 0;
    difference |= a ^ b;
  }
  return difference == 0;
}

bool AuthManager::derivePassword(const String& password, const uint8_t* salt, size_t saltLength,
                                 uint32_t iterations, uint8_t output[HASH_BYTES]) const {
  mbedtls_md_context_t context;
  mbedtls_md_init(&context);
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (info == nullptr || mbedtls_md_setup(&context, info, 1) != 0) {
    mbedtls_md_free(&context);
    return false;
  }
  const int result = mbedtls_pkcs5_pbkdf2_hmac(
      &context, reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), salt,
      saltLength, iterations, HASH_BYTES, output);
  mbedtls_md_free(&context);
  return result == 0;
}

bool AuthManager::storePassword(const String& password, String& error) {
  if (!validatePasswordFormat(password, error)) return false;
  uint8_t salt[SALT_BYTES] = {};
  uint8_t hash[HASH_BYTES] = {};
  esp_fill_random(salt, sizeof(salt));
  if (!derivePassword(password, salt, sizeof(salt), atomdeck::PBKDF2_ITERATIONS, hash)) {
    error = "password derivation failed";
    return false;
  }
  const String previousSalt = prefs_.getString("pw_salt", "");
  const String previousHash = prefs_.getString("pw_hash", "");
  const uint32_t previousIterations = prefs_.getUInt("pw_iter", 0);
  const String nextSalt = toHex(salt, sizeof(salt));
  const String nextHash = toHex(hash, sizeof(hash));
  const bool stored = prefs_.putUInt("pw_iter", atomdeck::PBKDF2_ITERATIONS) != 0 &&
                      prefs_.putString("pw_salt", nextSalt) != 0 &&
                      prefs_.putString("pw_hash", nextHash) != 0;
  if (!stored) {
    if (previousIterations == 0) prefs_.remove("pw_iter");
    else prefs_.putUInt("pw_iter", previousIterations);
    if (previousSalt.isEmpty()) prefs_.remove("pw_salt");
    else prefs_.putString("pw_salt", previousSalt);
    if (previousHash.isEmpty()) prefs_.remove("pw_hash");
    else prefs_.putString("pw_hash", previousHash);
    error = "failed to store password verifier";
    return false;
  }
  return true;
}

bool AuthManager::setInitialPassword(const String& password, String& error) {
  if (configured()) {
    error = "administrator password is already configured";
    return false;
  }
  return storePassword(password, error);
}

bool AuthManager::verifyPassword(const String& password) {
  uint8_t salt[SALT_BYTES] = {};
  uint8_t actual[HASH_BYTES] = {};
  const String saltHex = prefs_.getString("pw_salt", "");
  const String expected = prefs_.getString("pw_hash", "");
  const uint32_t iterations = prefs_.getUInt("pw_iter", 0);
  if (!fromHex(saltHex, salt, sizeof(salt)) || expected.length() != HASH_BYTES * 2 || iterations == 0) {
    return false;
  }
  if (!derivePassword(password, salt, sizeof(salt), iterations, actual)) return false;
  return constantEquals(toHex(actual, sizeof(actual)), expected);
}

AuthManager::LoginRateSlot& AuthManager::rateSlot(const String& clientId) {
  size_t selected = 0;
  for (size_t i = 0; i < 4; ++i) {
    if (loginRates_[i].clientId == clientId) return loginRates_[i];
    if (loginRates_[i].clientId.isEmpty() || loginRates_[i].lastSeen < loginRates_[selected].lastSeen) {
      selected = i;
    }
  }
  loginRates_[selected] = LoginRateSlot{};
  loginRates_[selected].clientId = clientId;
  return loginRates_[selected];
}

bool AuthManager::login(const String& password, const String& clientId, String& sessionId,
                        uint32_t& retryAfterSeconds, String& error) {
  retryAfterSeconds = 0;
  LoginRateSlot& rate = rateSlot(clientId);
  rate.lastSeen = millis();
  if (rate.blockedUntil != 0 && static_cast<int32_t>(rate.blockedUntil - millis()) > 0) {
    retryAfterSeconds = (rate.blockedUntil - millis() + 999) / 1000;
    error = "too many failed logins";
    return false;
  }
  if (rate.blockedUntil != 0) {
    rate.blockedUntil = 0;
    rate.failures = 0;
  }
  if (!verifyPassword(password)) {
    ++audit_.loginFailure;
    ++rate.failures;
    if (rate.failures >= atomdeck::MAX_LOGIN_FAILURES) {
      rate.blockedUntil = millis() + atomdeck::LOGIN_BLOCK_MS;
      retryAfterSeconds = atomdeck::LOGIN_BLOCK_MS / 1000;
    }
    error = "invalid credentials";
    return false;
  }

  ++audit_.loginSuccess;
  rate.failures = 0;
  rate.blockedUntil = 0;
  sessionId = randomHex(32);
  size_t selected = 0;
  for (size_t i = 0; i < atomdeck::MAX_SESSIONS; ++i) {
    if (!sessions_[i].used || static_cast<int32_t>(sessions_[i].expiresAt - millis()) <= 0) {
      selected = i;
      break;
    }
    if (sessions_[i].expiresAt < sessions_[selected].expiresAt) selected = i;
  }
  sessions_[selected].used = true;
  sessions_[selected].expiresAt = millis() + atomdeck::SESSION_TTL_MS;
  sessions_[selected].hash = sha256Hex(sessionId);
  return true;
}

void AuthManager::logout(const String& sessionId) {
  const String hash = sha256Hex(sessionId);
  for (SessionSlot& session : sessions_) {
    if (session.used && constantEquals(session.hash, hash)) session = SessionSlot{};
  }
}

AuthKind AuthManager::authorize(const String& sessionId, const String& bearerToken) {
  if (!sessionId.isEmpty()) {
    const String hash = sha256Hex(sessionId);
    for (SessionSlot& session : sessions_) {
      if (session.used && static_cast<int32_t>(session.expiresAt - millis()) > 0 &&
          constantEquals(session.hash, hash)) {
        return AuthKind::Session;
      }
    }
  }
  if (!bearerToken.isEmpty() && tokenExists(bearerToken)) return AuthKind::Bearer;
  return AuthKind::None;
}

void AuthManager::invalidateSessions() {
  for (SessionSlot& session : sessions_) session = SessionSlot{};
}

bool AuthManager::changePassword(const String& currentPassword, const String& newPassword, String& error) {
  if (!verifyPassword(currentPassword)) {
    error = "current password is incorrect";
    return false;
  }
  if (!storePassword(newPassword, error)) return false;
  invalidateSessions();
  return true;
}

bool AuthManager::tokenExists(const String& rawToken) {
  if (!rawToken.startsWith("adk_") || rawToken.length() != 68) return false;
  const String candidate = sha256Hex(rawToken);
  for (size_t i = 0; i < atomdeck::MAX_TOKENS; ++i) {
    const String stored = prefs_.getString(tokenKey(i, "h").c_str(), "");
    if (!stored.isEmpty() && constantEquals(candidate, stored)) return true;
  }
  return false;
}

bool AuthManager::listTokens(String& json, String& error) {
  DynamicJsonDocument doc(2048);
  JsonArray tokens = doc.to<JsonArray>();
  for (size_t i = 0; i < atomdeck::MAX_TOKENS; ++i) {
    const String id = prefs_.getString(tokenKey(i, "i").c_str(), "");
    if (id.isEmpty()) continue;
    JsonObject token = tokens.createNestedObject();
    token["id"] = id;
    token["name"] = prefs_.getString(tokenKey(i, "n").c_str(), "REST token");
  }
  if (serializeJson(doc, json) == 0) {
    error = "cannot serialize token list";
    return false;
  }
  return true;
}

bool AuthManager::createToken(const String& inputName, String& json, String& error) {
  String name = inputName;
  name.trim();
  if (name.isEmpty() || name.length() > 32) {
    error = "token name must contain 1-32 bytes";
    return false;
  }
  size_t slot = atomdeck::MAX_TOKENS;
  for (size_t i = 0; i < atomdeck::MAX_TOKENS; ++i) {
    if (prefs_.getString(tokenKey(i, "i").c_str(), "").isEmpty()) {
      slot = i;
      break;
    }
  }
  if (slot == atomdeck::MAX_TOKENS) {
    error = "token limit reached";
    return false;
  }
  const String id = randomHex(8);
  const String rawToken = String("adk_") + randomHex(32);
  if (prefs_.putString(tokenKey(slot, "i").c_str(), id) == 0 ||
      prefs_.putString(tokenKey(slot, "n").c_str(), name) == 0 ||
      prefs_.putString(tokenKey(slot, "h").c_str(), sha256Hex(rawToken)) == 0) {
    prefs_.remove(tokenKey(slot, "i").c_str());
    prefs_.remove(tokenKey(slot, "n").c_str());
    prefs_.remove(tokenKey(slot, "h").c_str());
    error = "failed to store token verifier";
    return false;
  }
  DynamicJsonDocument doc(512);
  doc["id"] = id;
  doc["name"] = name;
  doc["token"] = rawToken;
  serializeJson(doc, json);
  return true;
}

bool AuthManager::deleteToken(const String& id, String& error) {
  for (size_t i = 0; i < atomdeck::MAX_TOKENS; ++i) {
    if (prefs_.getString(tokenKey(i, "i").c_str(), "") != id) continue;
    prefs_.remove(tokenKey(i, "i").c_str());
    prefs_.remove(tokenKey(i, "n").c_str());
    prefs_.remove(tokenKey(i, "h").c_str());
    return true;
  }
  error = "token not found";
  return false;
}

void AuthManager::clearAll() {
  prefs_.clear();
  invalidateSessions();
  for (LoginRateSlot& rate : loginRates_) rate = LoginRateSlot{};
}
