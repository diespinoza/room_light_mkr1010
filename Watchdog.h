// Watchdog.h — SAMD21 hardware watchdog.
//
// If anything stalls the main loop for about 16 s (the usual culprit is the
// WiFi module wedging the SPI link), the chip resets, the saved state comes
// back from flash, and the light is on again within a second. WiFiNINA's
// own feed hook keeps us alive during its legitimate waits.
#pragma once

#include <Arduino.h>

namespace Watchdog {
  void begin();                 // ~16 s period
  void feed();
  bool causedLastReset();       // true if the last reset was the watchdog
  const char *resetCause();     // "power-on", "watchdog", "external", ...
}
