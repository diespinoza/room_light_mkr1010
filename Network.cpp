#include "Network.h"
#include <utility/wifi_drv.h>

static const char *statusName(uint8_t s) {
  switch (s) {
    case WL_IDLE_STATUS:     return "idle";
    case WL_NO_SSID_AVAIL:   return "no SSID";
    case WL_SCAN_COMPLETED:  return "scan done";
    case WL_CONNECTED:       return "connected";
    case WL_CONNECT_FAILED:  return "connect failed";
    case WL_CONNECTION_LOST: return "connection lost";
    case WL_DISCONNECTED:    return "disconnected";
    case WL_NO_MODULE:       return "no module";
    default:                 return "?";
  }
}

void Network::begin(const char *ssid, const char *pass, const char *hostname) {
  _ssid = ssid; _pass = pass; _hostname = hostname;
  if (WiFi.status() == WL_NO_MODULE) {
    _state = NO_MODULE;
    Serial.println(F("WiFi: NINA module not responding"));
    return;
  }
  String fv = WiFi.firmwareVersion();
  Serial.print(F("WiFi: NINA firmware ")); Serial.print(fv);
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print(F(" (library expects ")); Serial.print(WIFI_FIRMWARE_LATEST_VERSION); Serial.print(F(")"));
  }
  Serial.println();
  WiFi.setHostname(_hostname);
  _led.begin();
  _state = IDLE;
  _nextAttempt = millis();
}

void Network::startAttempt() {
  if (_failStreak >= WIFI_MODULE_RESET_AFTER) resetModule();
  _attempts++;
  Serial.print(F("WiFi: connecting to ")); Serial.print(_ssid);
  Serial.print(F(" (attempt ")); Serial.print(_attempts); Serial.println(F(")"));
  _led.setMode(StatusLed::CONNECTING);
  // Same call WiFi.begin() makes, minus its blocking poll loop.
  int8_t r = WiFiDrv::wifiSetPassphrase(_ssid, strlen(_ssid), _pass, strlen(_pass));
  _attemptStart = millis();
  _lastPoll = 0;
  if (r == WL_FAILURE) { onFailed(WL_CONNECT_FAILED); return; }
  _state = CONNECTING;
}

void Network::onConnected() {
  _state = CONNECTED;
  _failStreak = 0;
  _connects++;
  Serial.print(F("WiFi: connected, IP ")); Serial.print(WiFi.localIP());
  Serial.print(F("  http://")); Serial.print(WiFi.localIP()); Serial.println(F("/"));
  _led.setMode(StatusLed::CONNECTED);
  startMdns();
}

void Network::onFailed(uint8_t status) {
  _state = FAILED;
  _failStreak++;
  _nextAttempt = millis() + WIFI_RETRY_MS;
  Serial.print(F("WiFi: attempt failed (")); Serial.print(statusName(status));
  Serial.print(F("), retry in ")); Serial.print(WIFI_RETRY_MS / 1000); Serial.println(F(" s"));
  _led.setMode(StatusLed::LOST);
}

void Network::resetModule() {
  _moduleResets++;
  _failStreak = 0;
  Serial.println(F("WiFi: resetting the NINA module"));
  WiFi.end();                  // holds the module in reset
  delay(100);
  // The next library call re-initialises the SPI link and releases reset
  // (about 750 ms, blocking). Re-apply settings that live on the module.
  WiFi.setHostname(_hostname);
  _led.begin();
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
  if (_state == NO_MODULE || !_ssid) return;
  _led.update();
  uint32_t now = millis();

  switch (_state) {
    case IDLE:
    case FAILED:
      if ((int32_t)(now - _nextAttempt) >= 0) startAttempt();
      break;

    case CONNECTING: {
      if (now - _lastPoll < 250) break;          // one SPI query per 250 ms
      _lastPoll = now;
      uint8_t st = WiFi.status();
      _lastStatus = st;
      if (st == WL_CONNECTED) { onConnected(); break; }
      bool stillTrying = (st == WL_IDLE_STATUS || st == WL_NO_SSID_AVAIL || st == WL_SCAN_COMPLETED);
      if (!stillTrying || (now - _attemptStart) > WIFI_CONNECT_TIMEOUT_MS) onFailed(st);
      break;
    }

    case CONNECTED:
#if USE_MDNS
      _mdns.run();
#endif
      if (now - _lastPoll >= WIFI_STATUS_POLL_MS) {
        _lastPoll = now;
        uint8_t st = WiFi.status();
        _lastStatus = st;
        if (st != WL_CONNECTED) {
          _drops++;
          Serial.print(F("WiFi: connection lost (")); Serial.print(statusName(st)); Serial.println(F(")"));
          _state = FAILED;
          _nextAttempt = now;                      // retry straight away
          _led.setMode(StatusLed::LOST);
        }
      }
      break;

    default: break;
  }
}

void Network::printStatus(Stream &out) const {
  static const char *const NAMES[] = {"idle", "connecting", "connected", "failed, waiting to retry", "no module"};
  out.print(F("wifi: ")); out.print(NAMES[_state]);
  if (_state == CONNECTED) {
    out.print(F("  ip=")); out.print(WiFi.localIP());
    out.print(F("  rssi=")); out.print(WiFi.RSSI()); out.print(F(" dBm"));
  } else {
    out.print(F("  last status=")); out.print(statusName(_lastStatus));
  }
  out.println();
  out.print(F("      attempts=")); out.print(_attempts);
  out.print(F("  connects=")); out.print(_connects);
  out.print(F("  drops=")); out.print(_drops);
  out.print(F("  module resets=")); out.print(_moduleResets);
  out.print(F("  uptime=")); out.print(millis() / 1000); out.println(F(" s"));
}
