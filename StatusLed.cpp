#include "StatusLed.h"
#include "config.h"
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>

// NINA GPIO numbers for the module's RGB LED
static const uint8_t PIN_G = 25, PIN_R = 26, PIN_B = 27;

void StatusLed::begin() {
  WiFiDrv::pinMode(PIN_R, OUTPUT);
  WiFiDrv::pinMode(PIN_G, OUTPUT);
  WiFiDrv::pinMode(PIN_B, OUTPUT);
  write(0, 0, 0);
}

void StatusLed::write(uint8_t r, uint8_t g, uint8_t b) {
  // Each call is an SPI command to the module; keep calls to state changes.
  WiFiDrv::analogWrite(PIN_R, r);
  WiFiDrv::analogWrite(PIN_G, g);
  WiFiDrv::analogWrite(PIN_B, b);
}

void StatusLed::setMode(Mode m) {
  _mode = m; _since = millis(); _phase = true;
  const uint8_t L = STATUS_LED_LEVEL;
  switch (m) {
    case CONNECTING: write(0, 0, L); break;
    case CONNECTED:  write(0, L, 0); break;
    case LOST:       write(L, 0, 0); break;
    default:         write(0, 0, 0); break;
  }
}

void StatusLed::update() {
  uint32_t now = millis();
  if (_mode == CONNECTING) {
    if (now - _since >= 500) {               // slow blink
      _since = now; _phase = !_phase;
      write(0, 0, _phase ? STATUS_LED_LEVEL : 0);
    }
  } else if (_mode == CONNECTED) {
    if (now - _since >= STATUS_LED_OK_MS) { _mode = OFF; write(0, 0, 0); }
  }
}
