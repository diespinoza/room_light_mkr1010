#include "Network.h"
#include "config.h"

void Network::begin(const char *ssid, const char *pass, const char *hostname) {
  _ssid = ssid; _pass = pass; _hostname = hostname;
  if (WiFi.status() == WL_NO_MODULE) {
    _moduleOk = false;
    Serial.println(F("WiFi: NINA module not responding"));
    return;
  }
  String fv = WiFi.firmwareVersion();
  Serial.print(F("WiFi: NINA firmware ")); Serial.print(fv);
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print(F(" (updater has ")); Serial.print(WIFI_FIRMWARE_LATEST_VERSION); Serial.print(F(")"));
  }
  Serial.println();
  WiFi.setHostname(_hostname);
  _lastAttempt = millis() - WIFI_RETRY_MS;   // connect on the first update()
}

void Network::tryConnect() {
  _attempts++;
  Serial.print(F("WiFi: connecting to ")); Serial.print(_ssid);
  Serial.print(F(" (attempt ")); Serial.print(_attempts); Serial.println(F(")"));
  // WiFi.begin blocks for a few seconds; the strip simply holds its colour.
  int st = WiFi.begin(_ssid, _pass);
  _lastAttempt = millis();
  if (st == WL_CONNECTED) {
    _connected = true;
    Serial.print(F("WiFi: connected, IP ")); Serial.println(WiFi.localIP());
    Serial.print(F("WiFi: open http://")); Serial.print(WiFi.localIP()); Serial.println(F("/"));
    startMdns();
  } else {
    Serial.print(F("WiFi: failed, status ")); Serial.println(st);
  }
}

void Network::startMdns() {
#if USE_MDNS
  _mdns.removeAllServiceRecords();
  if (_mdns.begin(WiFi.localIP(), _hostname)) {
    _mdns.addServiceRecord(MDNS_SERVICE_NAME, HTTP_PORT, MDNSServiceTCP);
    Serial.print(F("mDNS: http://")); Serial.print(_hostname); Serial.println(F(".local/"));
  } else {
    Serial.println(F("mDNS: failed to start"));
  }
#endif
}

void Network::update() {
  if (!_moduleOk || !_ssid) return;
#if USE_MDNS
  if (_connected) _mdns.run();
#endif
  bool now = (WiFi.status() == WL_CONNECTED);
  if (_connected && !now) {
    _connected = false;
    Serial.println(F("WiFi: connection lost"));
  }
  if (!now && (millis() - _lastAttempt) >= WIFI_RETRY_MS) tryConnect();
}

void Network::printStatus(Stream &out) const {
  if (!_moduleOk) { out.println(F("wifi: no module")); return; }
  out.print(F("wifi: ")); out.print(_connected ? F("connected") : F("disconnected"));
  if (_connected) {
    out.print(F("  ip=")); out.print(WiFi.localIP());
    out.print(F("  rssi=")); out.print(WiFi.RSSI()); out.print(F(" dBm"));
  }
  out.print(F("  attempts=")); out.println(_attempts);
}
