#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "app_state.h"
#include "auth_manager.h"
#include "hid_executor.h"
#include "macro_store.h"
#include "wifi_manager.h"

class ApiServer {
 public:
  ApiServer(AppState& state, WifiManager& wifi, MacroStore& store, HidExecutor& hid,
            AuthManager& auth)
      : state_(state), wifi_(wifi), store_(store), hid_(hid), auth_(auth), server_(80) {}

  void begin();
  void loop();

 private:
  AppState& state_;
  WifiManager& wifi_;
  MacroStore& store_;
  HidExecutor& hid_;
  AuthManager& auth_;
  WebServer server_;
  uint32_t restartAt_ = 0;

  void handleStatus();
  void handleMacroCollection();
  void handleDynamicRoute();
  void handleType();
  void handleMouse();
  void handleWifi();
  void handleLogin();
  void handleLogout();
  void handleAuthMe();
  void handlePasswordChange();
  void handleTokenCollection();
  void handleLock();
  bool requireArmed();
  bool requireAuth(bool sessionOnly = false);
  bool requireSameOrigin();
  String sessionCookie();
  String bearerToken();
  AuthKind currentAuth();
  void sendJson(int status, const String& json);
  void sendError(int status, const String& error);
};
