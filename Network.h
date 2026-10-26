// Network.h — WiFi connection with non-blocking reconnect (WiFiNINA).
#pragma once

#include <Arduino.h>
#include <WiFiNINA.h>
#include "config.h"
#if USE_MDNS
#include <WiFiUdp.h>
#include <ArduinoMDNS.h>
#endif

class Network {
 public:
  void begin(const char *ssid, const char *pass, const char *hostname);
  void update();                 // call every loop; reconnects when dropped
  bool connected() const { return _connected; }
  IPAddress ip() const { return WiFi.localIP(); }
  void printStatus(Stream &out) const;

 private:
  void tryConnect();
  void startMdns();
#if USE_MDNS
  WiFiUDP _udp;
  MDNS _mdns{_udp};
#endif
  const char *_ssid = nullptr;
  const char *_pass = nullptr;
  const char *_hostname = nullptr;
  bool _connected = false;
  bool _moduleOk = true;
  uint32_t _lastAttempt = 0;
  uint32_t _attempts = 0;
};
