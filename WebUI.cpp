#include "WebUI.h"
#include "config.h"
#include "index_html.h"

void WebUI::begin() {
  if (_begun) return;
  _server.begin();
  _begun = true;
}

void WebUI::handle() {
  if (!_begun) return;
  WiFiClient client = _server.available();
  if (!client) return;
  serve(client);
  client.stop();
}

// ---- request parsing --------------------------------------------------------
static bool readLine(WiFiClient &client, char *buf, size_t len, uint32_t deadline) {
  size_t n = 0;
  while ((int32_t)(millis() - deadline) < 0) {
    if (!client.connected()) return false;
    if (!client.available()) continue;
    char c = (char)client.read();
    if (c == '\n') { buf[n] = '\0'; return true; }
    if (c != '\r' && n < len - 1) buf[n++] = c;
  }
  return false;
}

bool WebUI::readRequest(WiFiClient &client, char *method, size_t mlen,
                        char *path, size_t plen, char *body, size_t blen) {
  uint32_t deadline = millis() + HTTP_CLIENT_TIMEOUT_MS;
  char line[160];
  if (!readLine(client, line, sizeof(line), deadline)) return false;

  // "GET /api/state HTTP/1.1"
  char *sp1 = strchr(line, ' ');
  if (!sp1) return false;
  *sp1 = '\0';
  char *sp2 = strchr(sp1 + 1, ' ');
  if (sp2) *sp2 = '\0';
  strncpy(method, line, mlen - 1); method[mlen - 1] = '\0';
  strncpy(path, sp1 + 1, plen - 1); path[plen - 1] = '\0';
  char *q = strchr(path, '?'); if (q) *q = '\0';   // ignore query strings

  size_t contentLength = 0;
  while (readLine(client, line, sizeof(line), deadline)) {
    if (line[0] == '\0') break;                      // end of headers
    if (!strncasecmp(line, "Content-Length:", 15)) contentLength = (size_t)atol(line + 15);
  }

  size_t n = 0;
  if (contentLength > blen - 1) contentLength = blen - 1;
  while (n < contentLength && (int32_t)(millis() - deadline) < 0) {
    if (client.available()) body[n++] = (char)client.read();
  }
  body[n] = '\0';
  return true;
}

// ---- minimal JSON picking: good enough for a flat object of known keys -----
static const char *findValue(const char *json, const char *key) {
  char pat[24];
  snprintf(pat, sizeof(pat), "\"%s\"", key);
  const char *p = strstr(json, pat);
  if (!p) return nullptr;
  p = strchr(p + strlen(pat), ':');
  if (!p) return nullptr;
  p++;
  while (*p == ' ') p++;
  return p;
}

static bool getInt(const char *json, const char *key, long &out) {
  const char *v = findValue(json, key);
  if (!v || !(isdigit((unsigned char)*v) || *v == '-')) return false;
  out = atol(v);
  return true;
}

static bool getBool(const char *json, const char *key, bool &out) {
  const char *v = findValue(json, key);
  if (!v) return false;
  if (!strncmp(v, "true", 4)) { out = true; return true; }
  if (!strncmp(v, "false", 5)) { out = false; return true; }
  return false;
}

static bool getStr(const char *json, const char *key, char *out, size_t len) {
  const char *v = findValue(json, key);
  if (!v || *v != '"') return false;
  v++;
  size_t n = 0;
  while (*v && *v != '"' && n < len - 1) out[n++] = *v++;
  out[n] = '\0';
  return true;
}

static uint8_t clamp8(long v) { return v < 0 ? 0 : v > 255 ? 255 : (uint8_t)v; }

void WebUI::applyJson(const char *body) {
  char preset[16];
  if (getStr(body, "preset", preset, sizeof(preset))) {
    _state.applyPreset(preset);
    return;                                   // a preset sets everything
  }
  long v; bool b;
  RGBW c = _state.color();
  bool colourChanged = false;
  if (getInt(body, "r", v)) { c.r = clamp8(v); colourChanged = true; }
  if (getInt(body, "g", v)) { c.g = clamp8(v); colourChanged = true; }
  if (getInt(body, "b", v)) { c.b = clamp8(v); colourChanged = true; }
  if (getInt(body, "w", v)) { c.w = clamp8(v); colourChanged = true; }
  if (getInt(body, "bri", v)) _state.setBrightness(clamp8(v));
  if (colourChanged) _state.setColor(c);
  if (getBool(body, "on", b)) _state.setOn(b);  // last, so {"on":false,...} wins
}

// ---- responses ----------------------------------------------------------------
void WebUI::sendHeader(WiFiClient &client, int code, const char *type, size_t len) {
  client.print(F("HTTP/1.1 ")); client.print(code);
  client.println(code == 200 ? F(" OK") : code == 404 ? F(" Not Found") : F(" Bad Request"));
  client.print(F("Content-Type: ")); client.println(type);
  client.print(F("Content-Length: ")); client.println(len);
  client.println(F("Cache-Control: no-store"));
  client.println(F("Connection: close"));
  client.println();
}

void WebUI::sendState(WiFiClient &client) {
  char buf[192];
  size_t n = _state.toJson(buf, sizeof(buf));
  sendHeader(client, 200, "application/json", n);
  client.write((const uint8_t *)buf, n);
}

void WebUI::sendPresets(WiFiClient &client) {
  char buf[160];
  size_t n = 0;
  uint8_t count;
  const Preset *p = LightState::presets(count);
  buf[n++] = '[';
  for (uint8_t i = 0; i < count && n < sizeof(buf) - 4; i++) {
    n += snprintf(buf + n, sizeof(buf) - n, "%s\"%s\"", i ? "," : "", p[i].name);
  }
  buf[n++] = ']'; buf[n] = '\0';
  sendHeader(client, 200, "application/json", n);
  client.write((const uint8_t *)buf, n);
}

void WebUI::sendPage(WiFiClient &client) {
  size_t len = strlen(INDEX_HTML);
  sendHeader(client, 200, "text/html; charset=utf-8", len);
  client.write((const uint8_t *)INDEX_HTML, len);
}

void WebUI::serve(WiFiClient &client) {
  char method[8], path[40], body[192];
  if (!readRequest(client, method, sizeof(method), path, sizeof(path), body, sizeof(body))) {
    sendHeader(client, 400, "text/plain", 0);
    return;
  }
  bool isGet = !strcmp(method, "GET"), isPost = !strcmp(method, "POST");

  if (isGet && !strcmp(path, "/")) {
    sendPage(client);
  } else if (!strcmp(path, "/api/state") && (isGet || isPost)) {
    if (isPost) applyJson(body);
    sendState(client);
  } else if (isGet && !strcmp(path, "/api/presets")) {
    sendPresets(client);
  } else {
    sendHeader(client, 404, "text/plain", 0);
  }
  client.flush();
}
