// Persist.h — remember the last state across power cycles (SAMD flash).
//
// Writes are debounced: the state is saved SAVE_DELAY_MS after the last
// change, so dragging a slider costs one flash erase, not hundreds.
#pragma once

#include <Arduino.h>
#include "LightState.h"

class Persist {
 public:
  explicit Persist(LightState &light) : _light(light) {}
  bool begin();                  // load; returns true if a saved state was found
  void update();                 // call every loop
  void saveNow();
  uint32_t saves() const { return _saves; }

 private:
  LightState &_light;
  uint32_t _seenVersion = 0;
  uint32_t _changedAt = 0;
  bool     _dirty = false;
  uint32_t _saves = 0;
};
