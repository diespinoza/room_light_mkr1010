#include "Effects.h"
#include "config.h"
#include <math.h>

RGBW Effects::scale(RGBW c, uint16_t num, uint16_t den) {
  return {(uint8_t)((uint16_t)c.r * num / den), (uint8_t)((uint16_t)c.g * num / den),
          (uint8_t)((uint16_t)c.b * num / den), (uint8_t)((uint16_t)c.w * num / den)};
}

// Hue 0..255 around the wheel, full saturation, value v. White die stays off.
RGBW Effects::hsv(uint8_t h, uint8_t v) {
  uint8_t region = h / 43, rem = (h - region * 43) * 6;
  uint8_t q = (uint16_t)v * (255 - rem) / 255, t = (uint16_t)v * rem / 255;
  switch (region) {
    case 0:  return {v, t, 0, 0};
    case 1:  return {q, v, 0, 0};
    case 2:  return {0, v, t, 0};
    case 3:  return {0, q, v, 0};
    case 4:  return {t, 0, v, 0};
    default: return {v, 0, q, 0};
  }
}

void Effects::update() {
  uint8_t e = _light.effect();
  if (!_light.on() || e == 0) { _active = false; return; }

  uint32_t now = millis();
  if (!_active || e != _lastEffect) {              // (re)start
    _active = true; _lastEffect = e; _lastTick = now; _phase = 0;
    for (uint16_t i = 0; i < NUM_PIXELS; i++) { _lvl[i] = 200; _tgt[i] = 200; }
    _strip.fill(_light.color());                   // cancels any running fade
  }
  if (now - _lastTick < FRAME_INTERVAL_MS) return;
  uint32_t dt = now - _lastTick;
  _lastTick = now;

  switch (e) {
    case 1: breathe(dt); break;
    case 2: cycle(dt);   break;
    case 3: rainbow(dt); break;
    case 4: candle(dt);  break;
    case 5: twinkle(dt); break;
  }
}

// speed 50 → one breath every 4 s
void Effects::breathe(uint32_t dt) {
  _phase += (float)dt * _light.speed() / 50.0f * (2.0f * PI / 4000.0f);
  if (_phase > 2.0f * PI) _phase -= 2.0f * PI;
  uint16_t f = 90 + (uint16_t)(165.0f * (0.5f + 0.5f * sinf(_phase)));   // 90..255
  _strip.fill(scale(_light.color(), f, 255));
}

// speed 50 → full spectrum every 20 s
void Effects::cycle(uint32_t dt) {
  _phase += (float)dt * _light.speed() / 50.0f * (256.0f / 20000.0f);
  if (_phase >= 256.0f) _phase -= 256.0f;
  _strip.fill(hsv((uint8_t)_phase, 255));
}

// speed 50 → pattern rotates once every 10 s
void Effects::rainbow(uint32_t dt) {
  _phase += (float)dt * _light.speed() / 50.0f * (256.0f / 10000.0f);
  if (_phase >= 256.0f) _phase -= 256.0f;
  for (uint16_t i = 0; i < NUM_PIXELS; i++)
    _strip.setPixel(i, hsv((uint8_t)(_phase + (uint32_t)i * 256 / NUM_PIXELS), 255));
}

// Each pixel drifts toward a random target between 45% and 100% of the colour.
void Effects::candle(uint32_t dt) {
  uint8_t step = 1 + (uint8_t)(dt * _light.speed() / 400);           // per-tick slew
  RGBW base = _light.color();
  for (uint16_t i = 0; i < NUM_PIXELS; i++) {
    if (_lvl[i] == _tgt[i] || random(100) < 2) _tgt[i] = 115 + random(141);
    if (_lvl[i] < _tgt[i]) _lvl[i] = (uint8_t)min<int>(_lvl[i] + step, _tgt[i]);
    else                   _lvl[i] = (uint8_t)max<int>(_lvl[i] - step, _tgt[i]);
    _strip.setPixel(i, scale(base, _lvl[i], 255));
  }
}

// Base colour at 40%; sparks flash to full white and decay.
void Effects::twinkle(uint32_t dt) {
  uint8_t decay = 1 + (uint8_t)(dt * _light.speed() / 250);
  uint16_t spawn = 1 + _light.speed() / 10;                            // sparks per tick (x/100 chance each)
  RGBW base = scale(_light.color(), 100, 255);
  for (uint16_t s = 0; s < spawn; s++)
    if (random(100) < 8) _lvl[random(NUM_PIXELS)] = 255;
  for (uint16_t i = 0; i < NUM_PIXELS; i++) {
    _lvl[i] = (_lvl[i] > decay) ? _lvl[i] - decay : 0;
    uint8_t sp = _lvl[i];
    _strip.setPixel(i, {(uint8_t)max(base.r, sp), (uint8_t)max(base.g, sp),
                        (uint8_t)max(base.b, sp), (uint8_t)max(base.w, sp)});
  }
}
