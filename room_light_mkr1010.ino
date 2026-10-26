// room_light_mkr1010.ino — Phase 3: WiFi web page, effects, saved state, mDNS
//
// Drives the Adafruit 4689 NeoPixel RGBW neon strip from an Arduino MKR WiFi
// 1010. Control from a phone at http://<ip>/ on the home WiFi, or from the
// Serial Monitor (115200 baud, "Newline" line ending).
//
// Serial commands (type "help" for the same list):
//   r g b w          set colour, e.g. "255 80 0 0" (fades)
//   white N          white die only, 0..255
//   on / off         switch, keeping the colour
//   preset NAME      warm, cool, reading, night, sunset, ocean
//   effect NAME      static, breathe, cycle, rainbow, candle, twinkle
//   speed N          effect speed 1..100
//   save             write the state to flash now (normally automatic)
//   bri N            master brightness 0..255
//   fade MS          fade time in ms (0 = instant)
//   cap MA           current cap in mA (default from config.h)
//   refresh MS       idle resend interval; 0 = never resend (diagnostic)
//   pix I r g b w    one pixel, immediate (for finding the data direction)
//   test             step through R, G, B, W, all, off — 1 s each
//   pintest          hold the data pin HIGH for 10 s for a multimeter
//   wifi             connection status, IP, signal, attempt/drop counters
//   wifireset        hard-reset the WiFi module and reconnect
//   reboot           software reset of the board
//   status           colour, brightness, estimated mA, limiter state
//   help

#include "config.h"
#include "secrets.h"
#include "LedStrip.h"
#include "LightState.h"
#include "Network.h"
#include "WebUI.h"
#include "Effects.h"
#include "Persist.h"
#include "Watchdog.h"

LedStrip   strip;
LightState light(strip);
Network    net;
WebUI      web(light);
Effects    effects(strip, light);
Persist    persist(light);

// ---- Self-test sequence (non-blocking) -------------------------------------
const RGBW TEST_STEPS[] = {
    {255, 0, 0, 0}, {0, 255, 0, 0}, {0, 0, 255, 0},
    {0, 0, 0, 255}, {255, 255, 255, 255}, {0, 0, 0, 0}};
const uint8_t TEST_COUNT = sizeof(TEST_STEPS) / sizeof(TEST_STEPS[0]);
int8_t testStep = -1;          // -1 = not running
uint32_t testNextAt = 0;

// ---- Pin test: steady HIGH on the data pin for a multimeter ---------------
uint32_t pinTestUntil = 0;     // 0 = not running

// ---- Serial line buffer ----------------------------------------------------
char line[64];
uint8_t lineLen = 0;

void printHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F("  r g b w        colour 0..255 each, e.g. 255 80 0 0"));
  Serial.println(F("  white N        white die only"));
  Serial.println(F("  on | off"));
  Serial.println(F("  preset NAME    warm cool reading night sunset ocean"));
  Serial.println(F("  effect NAME    static breathe cycle rainbow candle twinkle"));
  Serial.println(F("  speed N        effect speed 1..100"));
  Serial.println(F("  save           write state to flash now"));
  Serial.println(F("  bri N          master brightness 0..255"));
  Serial.println(F("  fade MS        fade time (0 = instant)"));
  Serial.println(F("  cap MA         current cap in mA"));
  Serial.println(F("  refresh MS     idle resend interval, 0 = off"));
  Serial.println(F("  pix I r g b w  one pixel, immediate"));
  Serial.println(F("  test           R, G, B, W, all, off"));
  Serial.println(F("  pintest        data pin HIGH for 10 s (meter: ~3.3 V)"));
  Serial.println(F("  wifi           connection status and counters"));
  Serial.println(F("  wifireset      hard-reset the WiFi module"));
  Serial.println(F("  reboot         software reset"));
  Serial.println(F("  status"));
}

void printStatus() {
  RGBW c = light.color();
  Serial.print(light.on() ? F("ON  ") : F("OFF "));
  Serial.print(F("colour r="));  Serial.print(c.r);
  Serial.print(F(" g="));        Serial.print(c.g);
  Serial.print(F(" b="));        Serial.print(c.b);
  Serial.print(F(" w="));        Serial.print(c.w);
  Serial.print(F("  effect="));  Serial.print(light.effectName());
  Serial.print(F("/"));          Serial.print(light.speed());
  Serial.print(F("  bri="));     Serial.print(light.brightness());
  Serial.print(F("  fade="));    Serial.print(light.fadeMs()); Serial.print(F("ms"));
  Serial.print(F("  est="));     Serial.print(strip.estimatedMa()); Serial.print(F("mA"));
  Serial.print(F("  cap="));     Serial.print(strip.currentCapMa()); Serial.print(F("mA"));
  Serial.print(F("  refresh=")); Serial.print(strip.refreshMs()); Serial.print(F("ms"));
  Serial.println(strip.wasLimited() ? F("  LIMITED") : F(""));
}

uint8_t clamp255(long v) {
  if (v < 0) return 0;
  if (v > 255) return 255;
  return (uint8_t)v;
}

void stopTests() { testStep = -1; }

void handleLine(char *s) {
  char *tok[6];
  uint8_t n = 0;
  for (char *p = strtok(s, " \t"); p && n < 6; p = strtok(NULL, " \t")) tok[n++] = p;
  if (n == 0) return;

  bool isNumber = (tok[0][0] >= '0' && tok[0][0] <= '9');

  if (isNumber && n == 4) {
    stopTests();
    light.setColor({clamp255(atol(tok[0])), clamp255(atol(tok[1])),
                    clamp255(atol(tok[2])), clamp255(atol(tok[3]))});
  } else if (!strcmp(tok[0], "white") && n == 2) {
    stopTests();
    light.setColor({0, 0, 0, clamp255(atol(tok[1]))});
  } else if (!strcmp(tok[0], "off")) {
    stopTests(); light.setOn(false);
  } else if (!strcmp(tok[0], "on")) {
    stopTests(); light.setOn(true);
  } else if (!strcmp(tok[0], "preset") && n == 2) {
    stopTests();
    if (!light.applyPreset(tok[1])) { Serial.println(F("? unknown preset")); return; }
  } else if (!strcmp(tok[0], "effect") && n == 2) {
    stopTests();
    if (!light.setEffectByName(tok[1])) { Serial.println(F("? unknown effect")); return; }
  } else if (!strcmp(tok[0], "speed") && n == 2) {
    light.setSpeed((uint8_t)constrain(atol(tok[1]), 1L, 100L));
  } else if (!strcmp(tok[0], "save")) {
    persist.saveNow();
    Serial.print(F("saved (")); Serial.print(persist.saves()); Serial.println(F(" writes this boot)"));
  } else if (!strcmp(tok[0], "bri") && n == 2) {
    light.setBrightness(clamp255(atol(tok[1])));
  } else if (!strcmp(tok[0], "fade") && n == 2) {
    light.setFadeMs((uint16_t)constrain(atol(tok[1]), 0L, 60000L));
  } else if (!strcmp(tok[0], "cap") && n == 2) {
    strip.setCurrentCapMa((uint16_t)constrain(atol(tok[1]), 0L, 20000L));
  } else if (!strcmp(tok[0], "refresh") && n == 2) {
    strip.setRefreshMs((uint32_t)constrain(atol(tok[1]), 0L, 3600000L));
  } else if (!strcmp(tok[0], "pix") && n == 6) {
    stopTests();
    strip.setPixel((uint16_t)atol(tok[1]),
                   {clamp255(atol(tok[2])), clamp255(atol(tok[3])),
                    clamp255(atol(tok[4])), clamp255(atol(tok[5]))});
  } else if (!strcmp(tok[0], "test")) {
    testStep = 0;
    testNextAt = 0;
  } else if (!strcmp(tok[0], "pintest")) {
    stopTests();
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    pinTestUntil = millis() + 10000;
    Serial.print(F("pin ")); Serial.print(LED_PIN);
    Serial.println(F(" held HIGH for 10 s - measure it against GND now"));
  } else if (!strcmp(tok[0], "wifi")) {
    net.printStatus(Serial);
    return;
  } else if (!strcmp(tok[0], "wifireset")) {
    net.resetModule();
  } else if (!strcmp(tok[0], "reboot")) {
    Serial.println(F("rebooting"));
    Serial.flush();
    delay(50);
    NVIC_SystemReset();
  } else if (!strcmp(tok[0], "status")) {
    printStatus();
    return;
  } else if (!strcmp(tok[0], "help") || !strcmp(tok[0], "?")) {
    printHelp();
    return;
  } else {
    Serial.println(F("? unknown command, try: help"));
    return;
  }
  Serial.println(F("ok"));
}

void pollSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (lineLen > 0) {
        line[lineLen] = '\0';
        handleLine(line);
        lineLen = 0;
      }
    } else if (lineLen < sizeof(line) - 1) {
      line[lineLen++] = c;
    }
  }
}

void runTest() {
  if (testStep < 0) return;
  uint32_t now = millis();
  if ((int32_t)(now - testNextAt) < 0) return;
  if (testStep >= TEST_COUNT) {
    testStep = -1;
    Serial.println(F("test done"));
    light.setOn(light.on());   // restore whatever the model says
    return;
  }
  strip.fadeTo(TEST_STEPS[testStep], light.fadeMs());
  Serial.print(F("test step ")); Serial.println(testStep + 1);
  testStep++;
  testNextAt = now + 1000;
}

void runPinTest() {
  if (pinTestUntil == 0) return;
  if ((int32_t)(millis() - pinTestUntil) < 0) return;
  pinTestUntil = 0;
  strip.begin();
  strip.redraw();
  Serial.println(F("pin test done, strip output resumed"));
}

void setup() {
  // Take control of the strip first: a freshly powered NeoPixel strip shows
  // random colours until it receives a frame.
  strip.begin();
  strip.setBrightness(light.brightness());
  strip.fill(light.color());   // soft white default
  bool restored = persist.begin();   // overrides with the last saved state
  effects.update();            // a saved effect draws its first frame now,
  strip.show();                // not after the (blocking) WiFi connect
  delay(5);
  strip.show();                // twice, in case the first frame was garbled

  Serial.begin(SERIAL_BAUD);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}  // wait for the monitor, not forever

  Serial.println(F("room_light_mkr1010 — phase 2 (WiFi web page)"));
  Serial.print(F("pixels=")); Serial.print(NUM_PIXELS);
  Serial.print(F(" pin=")); Serial.print(LED_PIN);
  Serial.print(F(" cap=")); Serial.print(CURRENT_CAP_MA); Serial.println(F("mA"));
  Serial.println(restored ? F("state: restored from flash") : F("state: defaults (nothing saved yet)"));
  Serial.print(F("reset cause: ")); Serial.println(Watchdog::resetCause());

#if WATCHDOG_ENABLED
  Watchdog::begin();
  WiFi.setFeedWatchdogFunc(Watchdog::feed);   // the library feeds us during its own waits
#endif
  net.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_HOSTNAME);
  printHelp();
  printStatus();
}

void loop() {
#if WATCHDOG_ENABLED
  Watchdog::feed();
#endif
  pollSerial();
  runPinTest();
  if (pinTestUntil) return;    // leave the pin alone while the meter test runs
  runTest();

  // Render first: WiFi.begin() blocks for seconds during a (re)connect, and
  // the strip should already show the right thing when that happens.
  effects.update();
  persist.update();
  if (strip.update()) {
    static bool wasLimited = false;
    if (strip.wasLimited() != wasLimited) {
      wasLimited = strip.wasLimited();
      Serial.println(wasLimited ? F("current limiter ON") : F("current limiter off"));
    }
  }

  net.update();
  if (net.connected()) {
    web.begin();               // idempotent; starts listening once connected
    web.handle();
  }
}
