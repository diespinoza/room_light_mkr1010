// LightState.h — the single source of truth for what the light should show.
//
// Every control surface (serial shell, web page, later MQTT and a button)
// changes this object; it pushes the result to the LedStrip. Keeping the
// model separate from the strip means the API and the UI never touch pixels.
// Effects (effect index > 0) are rendered by the Effects module, which reads
// this model every frame.
#pragma once

#include <Arduino.h>
#include "LedStrip.h"

struct Preset {
  const char *name;
  RGBW color;
  uint8_t bri;
};

// What gets saved to flash. Plain bytes, no pointers.
struct SavedState {
  uint32_t magic;
  uint8_t on, r, g, b, w, bri, effect, speed;
};

class LightState {
 public:
  explicit LightState(LedStrip &strip);

  bool     on() const { return _on; }
  RGBW     color() const { return _color; }
  uint8_t  brightness() const { return _bri; }
  uint16_t fadeMs() const { return _fadeMs; }
  uint8_t  effect() const { return _effect; }       // 0 = static
  uint8_t  speed() const { return _speed; }         // 1..100
  const char *effectName() const;

  void setOn(bool on);
  void setColor(RGBW c);
  void setBrightness(uint8_t b);
  void setFadeMs(uint16_t ms) { _fadeMs = ms; }
  void setEffect(uint8_t idx);
  bool setEffectByName(const char *name);           // false if unknown
  void setSpeed(uint8_t s);
  void toggle() { setOn(!_on); }
  bool applyPreset(const char *name);               // false if unknown

  static const Preset *presets(uint8_t &count);
  static const char *const *effectNames(uint8_t &count);

  // Flash persistence helpers
  void toSaved(SavedState &s) const;
  void restore(const SavedState &s);                // applies without a fade

  // Increments on every change; lets the UI and the saver poll cheaply.
  uint32_t version() const { return _version; }

  size_t toJson(char *buf, size_t len) const;

 private:
  void apply();
  LedStrip &_strip;
  bool     _on;
  RGBW     _color;
  uint8_t  _bri;
  uint16_t _fadeMs;
  uint8_t  _effect;
  uint8_t  _speed;
  uint32_t _version;
};
