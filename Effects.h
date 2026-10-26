// Effects.h — animated modes rendered from the LightState every frame.
//
//   breathe  the chosen colour swells and fades
//   cycle    whole strip drifts through the spectrum
//   rainbow  spectrum spread along the strip, rotating
//   candle   warm flicker, each pixel wanders on its own
//   twinkle  dim base colour with random white sparks
// Speed 1..100 scales the rate of all of them.
#pragma once

#include <Arduino.h>
#include "LedStrip.h"
#include "LightState.h"

class Effects {
 public:
  Effects(LedStrip &strip, LightState &light) : _strip(strip), _light(light) {}
  void update();                                   // call every loop

 private:
  void breathe(uint32_t dt);
  void cycle(uint32_t dt);
  void rainbow(uint32_t dt);
  void candle(uint32_t dt);
  void twinkle(uint32_t dt);
  static RGBW scale(RGBW c, uint16_t num, uint16_t den);
  static RGBW hsv(uint8_t h, uint8_t v);

  LedStrip   &_strip;
  LightState &_light;
  bool     _active = false;
  uint8_t  _lastEffect = 0;
  uint32_t _lastTick = 0;
  float    _phase = 0;                             // breathe / cycle / rainbow
  uint8_t  _lvl[NUM_PIXELS];                       // candle level, twinkle spark
  uint8_t  _tgt[NUM_PIXELS];                       // candle target
};
