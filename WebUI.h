// WebUI.h — tiny HTTP server: serves the phone page and a JSON API.
//
//   GET  /            the page (index_html.h)
//   GET  /api/state   {"on":true,"r":..,"g":..,"b":..,"w":..,"bri":..,...}
//   POST /api/state   same keys, any subset; "preset":"warm" applies a preset
//   GET  /api/presets ["warm","cool",...]
#pragma once

#include <Arduino.h>
#include <WiFiNINA.h>
#include "LightState.h"

class WebUI {
 public:
  explicit WebUI(LightState &state) : _state(state), _server(HTTP_PORT_DEFAULT) {}
  void begin();
  void handle();                 // call every loop; serves at most one request

 private:
  static const uint16_t HTTP_PORT_DEFAULT = 80;
  void serve(WiFiClient &client);
  bool readRequest(WiFiClient &client, char *method, size_t mlen,
                   char *path, size_t plen, char *body, size_t blen);
  void applyJson(const char *body);
  void sendState(WiFiClient &client);
  void sendPresets(WiFiClient &client);
  void sendPage(WiFiClient &client);
  void sendHeader(WiFiClient &client, int code, const char *type, size_t len);

  LightState &_state;
  WiFiServer _server;
  bool _begun = false;
};
