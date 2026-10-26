// LightState.h — the single source of truth for what the light should show.
//
// Every control surface (serial shell, web page, later MQTT and a button)
// changes this object; it pushes the result to the LedStrip. Keeping the
// model separate from the strip means the API and the UI never touch pixels.
#pragma once

#include <Arduino.h>
#include "LedStrip.h"

struct Preset {
  const char *name;
  RGBW color;
  uint8_t bri;
};

class LightState {
 public:
  explicit LightState(LedStrip &strip);

  // Current values
  bool     on() const { return _on; }
  RGBW     color() const { return _color; }
  uint8_t  brightness() const { return _bri; }
  uint16_t fadeMs() const { return _fadeMs; }

  // Setters: each applies immediately (with a fade) and bumps a change counter
  void setOn(bool on);
  void setColor(RGBW c);
  void setBrightness(uint8_t b);
  void setFadeMs(uint16_t ms) { _fadeMs = ms; }
  void toggle() { setOn(!_on); }
  bool applyPreset(const char *name);   // false if unknown

  static const Preset *presets(uint8_t &count);

  // Increments on every change; lets the UI poll cheaply.
  uint32_t version() const { return _version; }

  // Serialise to JSON into buf (returns bytes written, excluding NUL).
  size_t toJson(char *buf, size_t len) const;

 private:
  void apply();
  LedStrip &_strip;
  bool     _on;
  RGBW     _color;
  uint8_t  _bri;
  uint16_t _fadeMs;
  uint32_t _version;
};
