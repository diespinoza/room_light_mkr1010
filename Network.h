// Network.h — WiFi connection with non-blocking reconnect (WiFiNINA).
#pragma once

#include <Arduino.h>
#include <WiFiNINA.h>

class Network {
 public:
  void begin(const char *ssid, const char *pass, const char *hostname);
  void update();                 // call every loop; reconnects when dropped
  bool connected() const { return _connected; }
  IPAddress ip() const { return WiFi.localIP(); }
  void printStatus(Stream &out) const;

 private:
  void tryConnect();
  const char *_ssid = nullptr;
  const char *_pass = nullptr;
  const char *_hostname = nullptr;
  bool _connected = false;
  bool _moduleOk = true;
  uint32_t _lastAttempt = 0;
  uint32_t _attempts = 0;
};
