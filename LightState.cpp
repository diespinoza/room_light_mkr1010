#include "LightState.h"
#include "config.h"

// The white die is 5500K, so "warm" presets lean on red/green to pull it down.
static const Preset PRESETS[] = {
  {"warm",    {255, 120,  20, 180}, 200},
  {"cool",    {  0,   0,   0, 255}, 220},
  {"reading", {255, 200, 120, 255}, 255},
  {"night",   {120,  30,   0,  20},  40},
  {"sunset",  {255,  70,   0,  40}, 160},
  {"ocean",   {  0, 120, 255,  30}, 180},
};

LightState::LightState(LedStrip &strip)
    : _strip(strip), _on(true), _color{0, 0, 0, 160}, _bri(DEFAULT_BRIGHTNESS),
      _fadeMs(DEFAULT_FADE_MS), _version(0) {}

void LightState::setOn(bool on) {
  _on = on; _version++; apply();
}

void LightState::setColor(RGBW c) {
  _color = c; _version++;
  if (!_on) { _on = true; }     // picking a colour turns the light on
  apply();
}

void LightState::setBrightness(uint8_t b) {
  _bri = b; _version++;
  _strip.setBrightness(_bri);   // brightness is applied by the strip layer
}

bool LightState::applyPreset(const char *name) {
  for (const Preset &p : PRESETS) {
    if (!strcmp(p.name, name)) {
      _color = p.color; _bri = p.bri; _on = true; _version++;
      _strip.setBrightness(_bri);
      apply();
      return true;
    }
  }
  return false;
}

const Preset *LightState::presets(uint8_t &count) {
  count = sizeof(PRESETS) / sizeof(PRESETS[0]);
  return PRESETS;
}

void LightState::apply() {
  if (_on) _strip.fadeTo(_color, _fadeMs);
  else     _strip.fadeTo({0, 0, 0, 0}, _fadeMs);
}

size_t LightState::toJson(char *buf, size_t len) const {
  int n = snprintf(buf, len,
      "{\"on\":%s,\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"bri\":%u,"
      "\"est_ma\":%u,\"cap_ma\":%u,\"limited\":%s,\"version\":%lu}",
      _on ? "true" : "false", _color.r, _color.g, _color.b, _color.w, _bri,
      _strip.estimatedMa(), _strip.currentCapMa(),
      _strip.wasLimited() ? "true" : "false", (unsigned long)_version);
  return (n < 0) ? 0 : (size_t)n;
}
