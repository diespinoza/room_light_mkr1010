// StatusLed.h — WiFi status on the RGB LED of the NINA-W102 module.
//
//   blue, blinking   connecting (or retrying after a drop)
//   green, 3 s       just connected, then off so it is not a night light
//   red, solid       connection lost, waiting to retry
#pragma once

#include <Arduino.h>

class StatusLed {
 public:
  enum Mode : uint8_t { OFF, CONNECTING, CONNECTED, LOST };
  void begin();
  void setMode(Mode m);
  Mode mode() const { return _mode; }
  void update();                 // call every loop; handles blink and timeout

 private:
  void write(uint8_t r, uint8_t g, uint8_t b);
  Mode     _mode = OFF;
  uint32_t _since = 0;
  bool     _phase = false;
};
