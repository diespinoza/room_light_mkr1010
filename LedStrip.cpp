#include "LedStrip.h"

LedStrip::LedStrip()
    : _px(NUM_PIXELS, LED_PIN, PIXEL_TYPE),
      _bri(DEFAULT_BRIGHTNESS),
      _capMa(CURRENT_CAP_MA),
      _estMa(0),
      _limited(false),
      _dirty(true),
      _lastFrame(0),
      _refreshMs(IDLE_REFRESH_MS),
      _cur{0, 0, 0, 0},
      _fadeFrom{0, 0, 0, 0},
      _fadeTarget{0, 0, 0, 0},
      _fadeStart(0),
      _fadeMs(0),
      _fading(false) {
  clear();
}

void LedStrip::begin() {
  // Gamma 2.2 into 16 bits so that brightness scaling happens with headroom:
  // an 8-bit table would turn every dim value into 0 before brightness is
  // even applied.
  for (uint16_t i = 0; i < 256; i++)
    _gamma16[i] = (uint16_t)(powf(i / 255.0f, 2.2f) * 65535.0f + 0.5f);
  _px.begin();
  _px.clear();
  _px.show();  // strip dark and in a known state before anything else
}

void LedStrip::fill(RGBW c) {
  for (uint16_t i = 0; i < NUM_PIXELS; i++) _buf[i] = c;
  _cur = c;
  _fading = false;
  _dirty = true;
}

void LedStrip::setPixel(uint16_t i, RGBW c) {
  if (i >= NUM_PIXELS) return;
  _buf[i] = c;
  _dirty = true;
}

void LedStrip::clear() {
  fill({0, 0, 0, 0});
}

void LedStrip::fadeTo(RGBW c, uint16_t ms) {
  if (ms == 0) {
    fill(c);
    return;
  }
  _fadeFrom = _cur;
  _fadeTarget = c;
  _fadeStart = millis();
  _fadeMs = ms;
  _fading = true;
}

void LedStrip::setBrightness(uint8_t b) {
  _bri = b;
  _dirty = true;
}

uint8_t LedStrip::lerp8(uint8_t a, uint8_t b, uint16_t t, uint16_t span) {
  // a + (b - a) * t / span, without overflow or sign trouble.
  int32_t d = (int32_t)b - (int32_t)a;
  return (uint8_t)((int32_t)a + d * (int32_t)t / (int32_t)span);
}

bool LedStrip::update() {
  uint32_t now = millis();
  if (now - _lastFrame < FRAME_INTERVAL_MS) return false;

  if (_fading) {
    uint32_t t = now - _fadeStart;
    if (t >= _fadeMs) {
      fill(_fadeTarget);  // also clears _fading
    } else {
      RGBW c = {lerp8(_fadeFrom.r, _fadeTarget.r, t, _fadeMs),
                lerp8(_fadeFrom.g, _fadeTarget.g, t, _fadeMs),
                lerp8(_fadeFrom.b, _fadeTarget.b, t, _fadeMs),
                lerp8(_fadeFrom.w, _fadeTarget.w, t, _fadeMs)};
      for (uint16_t i = 0; i < NUM_PIXELS; i++) _buf[i] = c;
      _cur = c;
      _dirty = true;
    }
  }

  if (!_dirty && (_refreshMs == 0 || (now - _lastFrame) < _refreshMs)) return false;
  show();
  return true;
}

uint16_t LedStrip::toLinear(uint8_t v) const {
  return (uint16_t)((uint32_t)_gamma16[v] * _bri / 255);  // 0..65535
}

void LedStrip::show() {
  // 1. gamma (colour) then linear brightness → 16-bit linear light. The LED
  //    current is proportional to this value, so the estimate uses it.
  static uint16_t lin[NUM_PIXELS][4];
  uint32_t sum = 0;  // max 60*4*65535 = 15.7M, fits easily
  for (uint16_t i = 0; i < NUM_PIXELS; i++) {
    const RGBW &c = _buf[i];
    lin[i][0] = toLinear(c.r);
    lin[i][1] = toLinear(c.g);
    lin[i][2] = toLinear(c.b);
    lin[i][3] = toLinear(c.w);
    sum += lin[i][0] + lin[i][1] + lin[i][2] + lin[i][3];
  }

  // 2. estimate current and scale down if over the cap. Scaling happens in
  //    16 bits so consecutive clamped frames quantise to the same 8-bit
  //    values instead of jittering by one count.
  const uint32_t idleMa = (uint32_t)NUM_PIXELS * MA_IDLE_PER_PIXEL;
  uint32_t ledMa = (uint32_t)((uint64_t)sum * MA_PER_DIE / 65535);
  uint32_t totalMa = idleMa + ledMa;
  _limited = false;

  if (totalMa > _capMa && ledMa > 0) {
    uint32_t allowedLedMa = (_capMa > idleMa) ? (_capMa - idleMa) : 0;
    uint32_t scale = (uint32_t)(((uint64_t)allowedLedMa << 16) / ledMa);  // 1/65536ths
    for (uint16_t i = 0; i < NUM_PIXELS; i++)
      for (uint8_t ch = 0; ch < 4; ch++)
        lin[i][ch] = (uint16_t)(((uint32_t)lin[i][ch] * scale) >> 16);
    totalMa = idleMa + allowedLedMa;
    _limited = true;
  }
  _estMa = (uint16_t)min(totalMa, (uint32_t)65535);

  // 3. quantise to 8 bits once, with rounding, and push to the strip.
  static uint8_t out[NUM_PIXELS][4];
  for (uint16_t i = 0; i < NUM_PIXELS; i++)
    for (uint8_t ch = 0; ch < 4; ch++) {
      uint32_t o = ((uint32_t)lin[i][ch] + 128) >> 8;
      out[i][ch] = (o > 255) ? 255 : (uint8_t)o;
    }
  for (uint16_t i = 0; i < NUM_PIXELS; i++)
    _px.setPixelColor(i, out[i][0], out[i][1], out[i][2], out[i][3]);
  _px.show();

  _dirty = false;
  _lastFrame = millis();
}
