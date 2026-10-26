// Network.h — WiFi connection as a non-blocking state machine (WiFiNINA).
//
// WiFi.begin() blocks for up to 10 s, which froze effects and hid the status
// LED. Instead the connection request is sent and the status polled from
// loop(). After WIFI_MODULE_RESET_AFTER failed attempts the NINA module is
// reset through its reset pin, which clears the wedged states that otherwise
// produce hundreds of failed retries.
#pragma once

#include <Arduino.h>
#include <WiFiNINA.h>
#include "config.h"
#include "StatusLed.h"
#if USE_MDNS
#include <WiFiUdp.h>
#include <ArduinoMDNS.h>
#endif

class Network {
 public:
  enum State : uint8_t { IDLE, CONNECTING, CONNECTED, FAILED, NO_MODULE };

  void begin(const char *ssid, const char *pass, const char *hostname);
  void update();                 // call every loop
  bool connected() const { return _state == CONNECTED; }
  State state() const { return _state; }
  IPAddress ip() const { return WiFi.localIP(); }
  void resetModule();            // hard reset of the NINA; reconnects after
  void printStatus(Stream &out) const;
  StatusLed &led() { return _led; }

 private:
  void startAttempt();
  void onConnected();
  void onFailed(uint8_t status);
  void startMdns();

  StatusLed _led;
#if USE_MDNS
  WiFiUDP _udp;
  MDNS _mdns{_udp};
#endif
  const char *_ssid = nullptr;
  const char *_pass = nullptr;
  const char *_hostname = nullptr;
  State    _state = IDLE;
  uint32_t _attemptStart = 0;
  uint32_t _nextAttempt = 0;
  uint32_t _lastPoll = 0;
  uint32_t _attempts = 0;
  uint32_t _failStreak = 0;
  uint32_t _connects = 0;
  uint32_t _drops = 0;
  uint32_t _moduleResets = 0;
  uint8_t  _lastStatus = 0;
};
