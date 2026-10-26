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

// Index 0 must stay "static". Order is saved to flash, so append only.
static const char *const EFFECT_NAMES[] = {
  "static", "breathe", "cycle", "rainbow", "candle", "twinkle",
};
static const uint8_t EFFECT_COUNT = sizeof(EFFECT_NAMES) / sizeof(EFFECT_NAMES[0]);

LightState::LightState(LedStrip &strip)
    : _strip(strip), _on(true), _color{0, 0, 0, 160}, _bri(DEFAULT_BRIGHTNESS),
      _fadeMs(DEFAULT_FADE_MS), _effect(0), _speed(DEFAULT_SPEED), _version(0) {}

const char *LightState::effectName() const { return EFFECT_NAMES[_effect]; }

void LightState::setOn(bool on) { _on = on; _version++; apply(); }

void LightState::setColor(RGBW c) {
  _color = c; _version++;
  if (!_on) _on = true;         // picking a colour turns the light on
  apply();
}

void LightState::setBrightness(uint8_t b) {
  _bri = b; _version++;
  _strip.setBrightness(_bri);   // brightness is applied by the strip layer
}

void LightState::setEffect(uint8_t idx) {
  if (idx >= EFFECT_COUNT) idx = 0;
  _effect = idx; _version++;
  if (!_on) _on = true;
  apply();
}

bool LightState::setEffectByName(const char *name) {
  for (uint8_t i = 0; i < EFFECT_COUNT; i++)
    if (!strcmp(EFFECT_NAMES[i], name)) { setEffect(i); return true; }
  return false;
}

void LightState::setSpeed(uint8_t s) {
  _speed = (s < 1) ? 1 : (s > 100) ? 100 : s;
  _version++;
}

bool LightState::applyPreset(const char *name) {
  for (const Preset &p : PRESETS) {
    if (!strcmp(p.name, name)) {
      _color = p.color; _bri = p.bri; _on = true; _effect = 0; _version++;
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

const char *const *LightState::effectNames(uint8_t &count) {
  count = EFFECT_COUNT;
  return EFFECT_NAMES;
}

void LightState::apply() {
  if (!_on)            _strip.fadeTo({0, 0, 0, 0}, _fadeMs);
  else if (_effect == 0) _strip.fadeTo(_color, _fadeMs);
  // else: the Effects module renders from this model every frame
}

void LightState::toSaved(SavedState &s) const {
  s.on = _on; s.r = _color.r; s.g = _color.g; s.b = _color.b; s.w = _color.w;
  s.bri = _bri; s.effect = _effect; s.speed = _speed;
}

void LightState::restore(const SavedState &s) {
  _on = s.on; _color = {s.r, s.g, s.b, s.w}; _bri = s.bri;
  _effect = (s.effect < EFFECT_COUNT) ? s.effect : 0;
  _speed = (s.speed < 1) ? 1 : (s.speed > 100) ? 100 : s.speed;
  _version++;
  _strip.setBrightness(_bri);
  if (!_on)              _strip.fill({0, 0, 0, 0});
  else if (_effect == 0) _strip.fill(_color);
}

size_t LightState::toJson(char *buf, size_t len) const {
  int n = snprintf(buf, len,
      "{\"on\":%s,\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"bri\":%u,"
      "\"effect\":\"%s\",\"speed\":%u,"
      "\"est_ma\":%u,\"cap_ma\":%u,\"limited\":%s,\"version\":%lu}",
      _on ? "true" : "false", _color.r, _color.g, _color.b, _color.w, _bri,
      EFFECT_NAMES[_effect], _speed,
      _strip.estimatedMa(), _strip.currentCapMa(),
      _strip.wasLimited() ? "true" : "false", (unsigned long)_version);
  return (n < 0) ? 0 : (size_t)n;
}
