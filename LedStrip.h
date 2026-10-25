// LedStrip.h — thin layer over Adafruit_NeoPixel for the RGBW neon strip.
//
// Responsibilities:
//   * keeps a logical RGBW frame buffer (what the user asked for, 0..255)
//   * applies master brightness and gamma on the way out
//   * estimates current and scales the frame down to stay under a cap
//   * runs non-blocking fades between uniform colours
#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

struct RGBW {
  uint8_t r, g, b, w;
};

class LedStrip {
 public:
  LedStrip();

  void begin();

  // Frame buffer edits. Nothing reaches the LEDs until show() or update().
  void fill(RGBW c);
  void setPixel(uint16_t i, RGBW c);
  void clear();

  // Smoothly move the whole strip from its current colour to c over ms.
  void fadeTo(RGBW c, uint16_t ms);
  bool isFading() const { return _fading; }

  // Call every loop(). Advances fades and pushes frames at FRAME_INTERVAL_MS.
  // Returns true when a frame was sent to the strip.
  bool update();

  // Push the buffer to the LEDs now (brightness → gamma → current cap).
  void show();

  void     setBrightness(uint8_t b);
  uint8_t  brightness() const { return _bri; }

  void     setCurrentCapMa(uint16_t ma) { _capMa = ma; _dirty = true; }
  uint16_t currentCapMa() const { return _capMa; }

  // Idle resend interval in ms; 0 disables it (frames only on change).
  void     setRefreshMs(uint32_t ms) { _refreshMs = ms; }
  uint32_t refreshMs() const { return _refreshMs; }

  // Diagnostics for the last frame sent.
  uint16_t estimatedMa() const { return _estMa; }
  bool     wasLimited() const { return _limited; }

  // Last uniform colour set by fill()/fadeTo() (the fade's current value).
  RGBW     color() const { return _cur; }

  // Force the next update() to resend the buffer (after a pin test, etc.).
  void     redraw() { _dirty = true; }

 private:
  Adafruit_NeoPixel _px;
  RGBW     _buf[NUM_PIXELS];
  uint16_t _gamma16[256];   // colour value → linear light, 16-bit
  uint8_t  _bri;
  uint16_t _capMa;
  uint16_t _estMa;
  bool     _limited;
  bool     _dirty;
  uint32_t _lastFrame;
  uint32_t _refreshMs;

  RGBW     _cur, _fadeFrom, _fadeTarget;
  uint32_t _fadeStart;
  uint16_t _fadeMs;
  bool     _fading;

  static uint8_t lerp8(uint8_t a, uint8_t b, uint16_t t, uint16_t span);
  uint16_t toLinear(uint8_t v) const;
};
