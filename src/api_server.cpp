#include "api_server.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "config.h"
#include "web_ui.h"

namespace {

String jsonMessage(const char* key, const String& value) {
  DynamicJsonDocument doc(512);
  doc[key] = value;
  String json;
  serializeJson(doc, json);
  return json;
}

}  // namespace

void ApiServer::begin() {
  const char* headers[] = {"Origin", "Host"};
  server_.collectHeaders(headers, 2);

  server_.on("/", HTTP_GET, [this]() { server_.send_P(200, "text/html; charset=utf-8", INDEX_HTML); });
  server_.on("/generate_204", HTTP_GET,
             [this]() { server_.sendHeader("Location", "/", true); server_.send(302, "text/plain", ""); });
  server_.on("/hotspot-detect.html", HTTP_GET,
             [this]() { server_.send_P(200, "text/html; charset=utf-8", INDEX_HTML); });
  server_.on("/api/v1/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/v1/macros", [this]() { handleMacroCollection(); });
  server_.on("/api/v1/type", HTTP_POST, [this]() { handleType(); });
  server_.on("/api/v1/mouse", HTTP_POST, [this]() { handleMouse(); });
  server_.on("/api/v1/wifi", HTTP_POST, [this]() { handleWifi(); });
  server_.onNotFound([this]() { handleDynamicRoute(); });
  server_.begin();
}

void ApiServer::loop() {
  server_.handleClient();
  if (restartAt_ != 0 && static_cast<int32_t>(millis() - restartAt_) >= 0) {
    delay(50);
    ESP.restart();
  }
}

void ApiServer::sendJson(int status, const String& json) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(status, "application/json", json);
}

void ApiServer::sendError(int status, const String& error) {
  sendJson(status, jsonMessage("error", error));
}

bool ApiServer::requireArmed() {
  if (state_.isArmed()) return true;
  sendError(423, "press the AtomS3U button to arm operations for 60 seconds");
  return false;
}

bool ApiServer::requireSameOrigin() {
  if (!server_.hasHeader("Origin") || server_.header("Origin").isEmpty()) return true;
  const String expected = String("http://") + server_.header("Host");
  if (server_.header("Origin") == expected) return true;
  sendError(403, "cross-origin write request rejected");
  return false;
}

void ApiServer::handleStatus() {
  DynamicJsonDocument doc(1024);
  doc["device"] = "M5Stack AtomS3U";
  doc["version"] = atomdeck::VERSION;
  doc["device_id"] = state_.deviceId;
  doc["hostname"] = state_.hostName + ".local";
  doc["setup_mode"] = state_.setupMode;
  doc["setup_expired"] = wifi_.setupExpired();
  doc["armed"] = state_.isArmed();
  doc["armed_seconds"] = state_.armedSeconds();
  JsonObject wifi = doc.createNestedObject("wifi");
  wifi["mode"] = state_.setupMode ? "setup-ap" : "station";
  wifi["connected"] = wifi_.connected();
  wifi["ssid"] = wifi_.ssid();
  wifi["ip"] = wifi_.ip();
  JsonObject storage = doc.createNestedObject("storage");
  storage["used"] = LittleFS.usedBytes();
  storage["total"] = LittleFS.totalBytes();
  String json;
  serializeJson(doc, json);
  sendJson(200, json);
}

void ApiServer::handleMacroCollection() {
  if (server_.method() == HTTP_GET) {
    String json, error;
    if (!store_.list(json, error)) return sendError(500, error);
    return sendJson(200, json);
  }
  if (server_.method() == HTTP_POST) {
    if (!requireSameOrigin() || !requireArmed()) return;
    String json, error;
    if (!store_.create(server_.arg("plain"), json, error)) return sendError(422, error);
    return sendJson(201, json);
  }
  sendError(405, "method not allowed");
}

void ApiServer::handleDynamicRoute() {
  const String uri = server_.uri();
  const String prefix = "/api/v1/macros/";
  if (uri.startsWith(prefix)) {
    String tail = uri.substring(prefix.length());
    bool run = false;
    if (tail.endsWith("/run")) {
      run = true;
      tail.remove(tail.length() - 4);
    }
    if (tail.isEmpty() || tail.length() > 32) return sendError(404, "not found");
    if (!requireSameOrigin()) return;

    if (run && server_.method() == HTTP_POST) {
      if (!requireArmed()) return;
      DynamicJsonDocument macro(atomdeck::MACRO_DOC_CAPACITY);
      String error;
      if (!store_.get(tail, macro, error)) return sendError(404, error);
      if (!hid_.run(macro["actions"].as<JsonArrayConst>(), error)) return sendError(422, error);
      return sendJson(200, "{\"ok\":true}");
    }
    if (server_.method() == HTTP_PUT) {
      if (!requireArmed()) return;
      String json, error;
      if (!store_.replace(tail, server_.arg("plain"), json, error)) return sendError(422, error);
      return sendJson(200, json);
    }
    if (server_.method() == HTTP_DELETE) {
      if (!requireArmed()) return;
      String error;
      if (!store_.remove(tail, error)) return sendError(404, error);
      return sendJson(200, "{\"ok\":true}");
    }
    return sendError(405, "method not allowed");
  }

  if (state_.setupMode && server_.method() == HTTP_GET) {
    return server_.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  }
  sendError(404, "not found");
}

void ApiServer::handleType() {
  if (!requireSameOrigin() || !requireArmed()) return;
  DynamicJsonDocument body(1024);
  if (deserializeJson(body, server_.arg("plain")) || !body.is<JsonObject>()) {
    return sendError(400, "request body must be JSON");
  }
  String error;
  if (!hid_.typeText(String(body["text"] | ""), error)) return sendError(422, error);
  sendJson(200, "{\"ok\":true}");
}

void ApiServer::handleMouse() {
  if (!requireSameOrigin() || !requireArmed()) return;
  DynamicJsonDocument body(1024);
  if (deserializeJson(body, server_.arg("plain")) || !body.is<JsonObject>()) {
    return sendError(400, "request body must be JSON");
  }
  const String op = body["op"] | "";
  String error;
  bool ok = false;
  if (op == "move") ok = hid_.moveMouse(body["x"] | 0, body["y"] | 0, error);
  else if (op == "click") ok = hid_.clickMouse(String(body["button"] | ""), error);
  else if (op == "scroll") ok = hid_.scrollMouse(body["amount"] | 0, error);
  else error = "op must be move, click, or scroll";
  if (!ok) return sendError(422, error);
  sendJson(200, "{\"ok\":true}");
}

void ApiServer::handleWifi() {
  if (!requireSameOrigin()) return;
  if (!state_.setupMode || wifi_.setupExpired()) return sendError(409, "setup mode is not active");
  DynamicJsonDocument body(1024);
  if (deserializeJson(body, server_.arg("plain")) || !body.is<JsonObject>()) {
    return sendError(400, "request body must be JSON");
  }
  String password = body["password"] | "";
  String error;
  const bool saved = wifi_.saveCredentials(String(body["ssid"] | ""), password, error);
  password.clear();
  body.clear();
  if (!saved) return sendError(422, error);
  sendJson(202, "{\"ok\":true,\"restarting\":true}");
  restartAt_ = millis() + 1200;
}
