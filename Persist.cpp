#include "Persist.h"
#include "config.h"
#include <FlashStorage.h>

static const uint32_t MAGIC = 0x524C3103;   // 'R' 'L' + layout version 3

FlashStorage(savedStore, SavedState);

bool Persist::begin() {
  SavedState s = savedStore.read();
  _seenVersion = _light.version();
  if (s.magic != MAGIC) return false;
  _light.restore(s);
  _seenVersion = _light.version();           // restoring is not a change to save
  return true;
}

void Persist::update() {
  uint32_t v = _light.version();
  if (v != _seenVersion) { _seenVersion = v; _changedAt = millis(); _dirty = true; }
  if (_dirty && (millis() - _changedAt) >= SAVE_DELAY_MS) saveNow();
}

void Persist::saveNow() {
  SavedState s; s.magic = MAGIC;
  _light.toSaved(s);
  savedStore.write(s);
  _dirty = false; _saves++;
}
