// room_light_mkr1010.ino — Phase 1 bench test
//
// Drives the Adafruit 4689 NeoPixel RGBW neon strip from an Arduino MKR WiFi
// 1010 and lets you set colours from the Serial Monitor (115200 baud,
// "Newline" line ending). WiFi comes in phase 2.
//
// Commands (type "help" for the same list):
//   r g b w          set colour, e.g. "255 80 0 0" (fades)
//   white N          white die only, 0..255
//   off              all channels to 0 (fades)
//   bri N            master brightness 0..255
//   fade MS          fade time in ms (0 = instant)
//   cap MA           current cap in mA (default from config.h)
//   pix I r g b w    one pixel, immediate (for finding the data direction)
//   test             step through R, G, B, W, all, off — 1 s each
//   pintest          hold the data pin HIGH for 10 s so a multimeter can
//                    read it (expect ~3.3 V); normal output resumes after
//   status           print brightness, colour, estimated mA, limiter state
//   help

#include "config.h"
#include "LedStrip.h"

LedStrip strip;
uint16_t fadeMs = DEFAULT_FADE_MS;

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
  Serial.println(F("  off"));
  Serial.println(F("  bri N          master brightness 0..255"));
  Serial.println(F("  fade MS        fade time (0 = instant)"));
  Serial.println(F("  cap MA         current cap in mA"));
  Serial.println(F("  pix I r g b w  one pixel, immediate"));
  Serial.println(F("  test           R, G, B, W, all, off"));
  Serial.println(F("  pintest        data pin HIGH for 10 s (meter: ~3.3 V)"));
  Serial.println(F("  status"));
}

void printStatus() {
  RGBW c = strip.color();
  Serial.print(F("colour r="));  Serial.print(c.r);
  Serial.print(F(" g="));        Serial.print(c.g);
  Serial.print(F(" b="));        Serial.print(c.b);
  Serial.print(F(" w="));        Serial.print(c.w);
  Serial.print(F("  bri="));     Serial.print(strip.brightness());
  Serial.print(F("  fade="));    Serial.print(fadeMs); Serial.print(F("ms"));
  Serial.print(F("  est="));     Serial.print(strip.estimatedMa()); Serial.print(F("mA"));
  Serial.print(F("  cap="));     Serial.print(strip.currentCapMa()); Serial.print(F("mA"));
  Serial.println(strip.wasLimited() ? F("  LIMITED") : F(""));
}

uint8_t clamp255(long v) {
  if (v < 0) return 0;
  if (v > 255) return 255;
  return (uint8_t)v;
}

void handleLine(char *s) {
  // Tokenise into up to 6 words.
  char *tok[6];
  uint8_t n = 0;
  for (char *p = strtok(s, " \t"); p && n < 6; p = strtok(NULL, " \t")) tok[n++] = p;
  if (n == 0) return;

  bool isNumber = (tok[0][0] >= '0' && tok[0][0] <= '9');

  if (isNumber && n == 4) {
    RGBW c = {clamp255(atol(tok[0])), clamp255(atol(tok[1])),
              clamp255(atol(tok[2])), clamp255(atol(tok[3]))};
    testStep = -1;
    strip.fadeTo(c, fadeMs);
  } else if (!strcmp(tok[0], "white") && n == 2) {
    testStep = -1;
    strip.fadeTo({0, 0, 0, clamp255(atol(tok[1]))}, fadeMs);
  } else if (!strcmp(tok[0], "off")) {
    testStep = -1;
    strip.fadeTo({0, 0, 0, 0}, fadeMs);
  } else if (!strcmp(tok[0], "bri") && n == 2) {
    strip.setBrightness(clamp255(atol(tok[1])));
  } else if (!strcmp(tok[0], "fade") && n == 2) {
    fadeMs = (uint16_t)constrain(atol(tok[1]), 0L, 60000L);
  } else if (!strcmp(tok[0], "cap") && n == 2) {
    strip.setCurrentCapMa((uint16_t)constrain(atol(tok[1]), 0L, 20000L));
  } else if (!strcmp(tok[0], "pix") && n == 6) {
    testStep = -1;
    strip.setPixel((uint16_t)atol(tok[1]),
                   {clamp255(atol(tok[2])), clamp255(atol(tok[3])),
                    clamp255(atol(tok[4])), clamp255(atol(tok[5]))});
  } else if (!strcmp(tok[0], "test")) {
    testStep = 0;
    testNextAt = 0;  // fire immediately
  } else if (!strcmp(tok[0], "pintest")) {
    testStep = -1;
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    pinTestUntil = millis() + 10000;
    Serial.print(F("pin ")); Serial.print(LED_PIN);
    Serial.println(F(" held HIGH for 10 s - measure it against GND now"));
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
    return;
  }
  strip.fadeTo(TEST_STEPS[testStep], fadeMs);
  Serial.print(F("test step ")); Serial.println(testStep + 1);
  testStep++;
  testNextAt = now + 1000;
}

void runPinTest() {
  if (pinTestUntil == 0) return;
  if ((int32_t)(millis() - pinTestUntil) < 0) return;
  pinTestUntil = 0;
  strip.begin();    // re-arm the NeoPixel driver on the pin
  strip.redraw();   // and resend the current colour
  Serial.println(F("pin test done, strip output resumed"));
}

void setup() {
  // Take control of the strip first: a freshly powered NeoPixel strip shows
  // random colours until it receives a frame.
  strip.begin();
  strip.fill({0, 0, 0, 160});  // soft white at boot so you can see it is alive
  strip.show();
  delay(5);
  strip.show();                // twice, in case the first frame was garbled

  Serial.begin(SERIAL_BAUD);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}  // wait for the monitor, not forever

  Serial.println(F("room_light_mkr1010 — phase 1 bench test"));
  Serial.print(F("pixels=")); Serial.print(NUM_PIXELS);
  Serial.print(F(" pin=")); Serial.print(LED_PIN);
  Serial.print(F(" cap=")); Serial.print(CURRENT_CAP_MA); Serial.println(F("mA"));
  printHelp();
  printStatus();
}

void loop() {
  pollSerial();
  runPinTest();
  if (pinTestUntil) return;  // leave the pin alone while the meter test runs
  runTest();
  if (strip.update()) {
    // A frame went out. Report once when the limiter engages, so you notice.
    static bool wasLimited = false;
    if (strip.wasLimited() != wasLimited) {
      wasLimited = strip.wasLimited();
      Serial.println(wasLimited ? F("current limiter ON") : F("current limiter off"));
    }
  }
}
