#pragma once

#include <Arduino.h>
#include <Preferences.h>

enum class AuthKind : uint8_t { None = 0, Session = 1, Bearer = 2 };

struct AuthAudit {
  uint32_t loginSuccess = 0;
  uint32_t loginFailure = 0;
  uint32_t authDenied = 0;
  uint32_t macroRuns = 0;
};

class AuthManager {
 public:
  bool begin(String& error);
  bool configured();
  bool setInitialPassword(const String& password, String& error);
  bool changePassword(const String& currentPassword, const String& newPassword, String& error);
  void clearAll();

  bool login(const String& password, const String& clientId, String& sessionId,
             uint32_t& retryAfterSeconds, String& error);
  void logout(const String& sessionId);
  AuthKind authorize(const String& sessionId, const String& bearerToken);

  bool listTokens(String& json, String& error);
  bool createToken(const String& name, String& json, String& error);
  bool deleteToken(const String& id, String& error);

  void noteDenied() { ++audit_.authDenied; }
  void noteMacroRun() { ++audit_.macroRuns; }
  const AuthAudit& audit() const { return audit_; }

 private:
  struct SessionSlot {
    bool used = false;
    uint32_t expiresAt = 0;
    String hash;
  };

  struct LoginRateSlot {
    String clientId;
    uint8_t failures = 0;
    uint32_t blockedUntil = 0;
    uint32_t lastSeen = 0;
  };

  Preferences prefs_;
  SessionSlot sessions_[4];
  LoginRateSlot loginRates_[4];
  AuthAudit audit_;

  bool validatePasswordFormat(const String& password, String& error) const;
  bool storePassword(const String& password, String& error);
  bool verifyPassword(const String& password);
  bool derivePassword(const String& password, const uint8_t* salt, size_t saltLength,
                      uint32_t iterations, uint8_t output[32]) const;
  String randomHex(size_t byteCount) const;
  String sha256Hex(const String& value) const;
  String toHex(const uint8_t* data, size_t length) const;
  bool fromHex(const String& input, uint8_t* output, size_t outputLength) const;
  bool constantEquals(const String& left, const String& right) const;
  void invalidateSessions();
  LoginRateSlot& rateSlot(const String& clientId);
  bool tokenExists(const String& rawToken);
};
