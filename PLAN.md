# Room Lighting Controller — Arduino MKR WiFi 1010 + Adafruit NeoPixel RGBW Neon-like Strip

Status: phase 2 code written (2026-10-25); phase 1 verified on the bench. See README.md for current state.

## 1. Goal

A wall/shelf light for the room that can be switched, dimmed and recoloured
from a phone on the home WiFi, with no cloud dependency. Later: Home Assistant
integration and a physical button. Code should stay readable for someone with
basic Arduino experience.

## 2. The hardware we have

**Strip: Adafruit 4689 — NeoPixel RGBW Neon-like Flex Strip, Cool White 5500K,
1 m, 5 V, 60 LEDs/m.** Identified by its JST SM 3-pin input connector (the
other neon strips have bare wires or 2-pin JSTs). Facts that matter:

- 60 individually addressable RGBW pixels (WS281x / SK6812-style protocol).
- **5 V** power. Worst case ~80 mA per pixel with all four dies at full
  → ~4.8 A for the whole metre. Real room-light use is far lower, and the
  firmware will cap it.
- Data is a 5 V logic signal on the third JST pin. The MKR outputs 3.3 V.
- The white die is **cool white, 5500K** (daylight-ish). Warm tones come from
  mixing the white die with red/amber in software.
- Silicone tube, sealed at the far end, cuttable with wire cutters. Not for
  long-term outdoor use.
- JST SM 3-pin wire colours are not documented on the page: on Adafruit strips
  it is normally red = 5 V, black = GND, and the remaining (white or green,
  usually the middle pin) = data. **Verify on the strip before powering.**

**Board: Arduino MKR WiFi 1010.**

- SAMD21 Cortex-M0+, 48 MHz, 256 KB flash, 32 KB RAM.
- **3.3 V logic, pins are not 5 V tolerant.** Never feed 5 V into a pin.
- Power input: USB, or **VIN = regulated 5 V**. The same 5 V supply that runs
  the strip can run the board through VIN. The `5V` pin is an output; don't
  inject power there.
- WiFi/BLE via u-blox NINA-W102 → `WiFiNINA` library. Update the NINA firmware
  once (IDE Tools → WiFi101 / WiFiNINA Firmware Updater).
- `FlashStorage` library emulates EEPROM on SAMD (save last state).
- The NeoPixel library bit-bangs with interrupts off during `show()`: 60 RGBW
  pixels ≈ 2.4 ms. WiFiNINA talks over SPI and is unaffected.

## 3. Hardware design

### Parts list

| Qty | Part | Notes |
|---|---|---|
| 1 | Arduino MKR WiFi 1010 | have |
| 1 | Adafruit 4689 NeoPixel RGBW neon strip | have |
| 1 | **5 V DC 4 A** power supply (e.g. Adafruit 1466) | enough for one strip with a firmware cap of ~3.5 A. Two strips → 5 V 10 A |
| 1 | DC barrel jack (female, screw terminal or panel mount) | |
| 1 | JST SM 3-pin pigtail (plug side, e.g. Adafruit 1663) | mates with the strip, avoids cutting its connector |
| 1 | 74AHCT125 level shifter (Adafruit 1787) | lifts the 3.3 V data to 5 V. Try without first; add it if the strip glitches |
| 1 | 470 Ω resistor | in the data line, close to the strip |
| 1 | 1000 µF, 6.3 V+ electrolytic | across 5 V/GND right at the strip input |
| 1 | Breadboard + jumper wire → later half-size perfboard | |
| opt | Momentary button, or rotary encoder with push | physical on/off + brightness (phase 4) |
| opt | Enclosure | |

### Wiring

```
 5V PSU (+) ──┬──────────────────────────► strip JST  5V  (red)
              ├── 1000 µF ──┐
              └─────────────┼─────────────► MKR VIN
 5V PSU (−) ────────────────┴──────┬──────► strip JST  GND (black)
                                   └──────► MKR GND        ◄── common ground

 MKR D6 ──► [74AHCT125 in → out, optional] ──470Ω──► strip JST DATA (middle)

 MKR D7 ──► button to GND (INPUT_PULLUP)   (optional, phase 4)
```

Rules: power the strip from the supply, never from the MKR's 5 V pin. Connect
ground before data. Keep the data wire short (<30 cm) if running without the
level shifter. When bench-testing with the MKR on USB, keep the strip on its own
PSU with the grounds tied together.

### Power budget

| Load | Estimate |
|---|---|
| Strip, all 60 pixels full RGBW | ~4.8 A worst case (never allowed by firmware) |
| Strip, firmware cap | 3.5 A (configurable in `config.h`) |
| Strip, typical "warm white at 50 %" | ~1 A |
| MKR WiFi 1010 with WiFi active | ~0.2 A |
| **Supply** | **5 V 4 A** |

Measure the real draw with a multimeter once wired and note it here.

## 4. Firmware architecture

One `.ino` plus a few `.h/.cpp` tabs so the IDE 1.8.19 sketch stays navigable:

```
room_light_mkr1010/
  room_light_mkr1010.ino   setup/loop glue only
  secrets.h                WiFi SSID/password (git-ignored); secrets.example.h committed
  config.h                 pin numbers, pixel count, current cap, hostname
  LedStrip.h/.cpp          wraps Adafruit_NeoPixel (NEO_GRBW): fill(), per-pixel set,
                           gamma, master brightness, current limiter, non-blocking fades
  LightState.h/.cpp        the "model": on/off, r,g,b,w, brightness, effect, speed;
                           save/load to flash
  Effects.h/.cpp           static, breathe, colour cycle, rainbow, gradient, twinkle,
                           candle, sunrise
  WebUI.h/.cpp             WiFiNINA WiFiServer: serves index page + JSON API
  index_html.h             the phone UI as a PROGMEM string (single page, no framework)
  Network.h/.cpp           connect, reconnect on drop, mDNS name, optional MQTT
  Button.h/.cpp            debounced button / encoder (phase 4)
```

Design rules:
- `loop()` never blocks (no `delay()`): fades, effects, WiFi reconnect and HTTP
  are state machines ticked with `millis()`. Target ~50 frames/s for effects.
- Brightness and gamma (≈2.2, `Adafruit_NeoPixel::gamma8`) are applied in
  `LedStrip` on the way to `setPixelColor`, not with `setBrightness()`, which is
  lossy and makes dim colours banded.
- **Current limiter:** before `show()`, estimate mA from the summed channel
  values (~20 mA per die at 255) and scale the frame down if it exceeds the cap.
  This is what lets a 4 A supply be safe no matter what the UI asks for.
- Everything the UI can change goes through `LightState`; web API, button and
  effects are different callers. Saving to flash is debounced (write a few
  seconds after the last change) to protect flash endurance.
- "Warm white" preset = white die + some red/green, since the die itself is 5500K.
- API (phase 2), plain HTTP on the LAN:
  - `GET /`               → UI page
  - `GET /api/state`      → `{"on":true,"r":255,"g":120,"b":0,"w":80,"bri":200,"effect":"static","speed":50}`
  - `POST /api/state`     → same JSON, any subset of fields
  - `GET /api/effects`    → list of effect names
- Hostname `roomlight.local` via mDNS (ArduinoMDNS works with WiFiNINA), with a
  DHCP reservation on the router as the reliable fallback.

## 5. Control options

| Option | Effort | Pros | Cons |
|---|---|---|---|
| **A. Web page served by the MKR** (recommended first) | low | any phone/laptop, no cloud, add to home screen | only on home WiFi |
| B. MQTT → Home Assistant | medium (after A) | voice assistants, automations, schedules | needs HA + MQTT broker running (a Raspberry Pi is ideal) |
| C. Arduino IoT Cloud | low | official, phone app, MKR 1010 first-class | cloud account, free-tier limits, dead if internet is down |
| D. BLE from phone (ArduinoBLE) | medium | no WiFi needed | needs a custom app or generic BLE tool |
| E. Physical button/encoder | low | works without a phone | no colour picking |

Recommendation: A, then E, then B. C only if you prefer a ready-made app.

## 6. Milestones

| Phase | Deliverable | Done when |
|---|---|---|
| 0 Setup | IDE uploads to the MKR; NINA firmware current; libraries installed; JST pinout verified | Blink runs; `ScanNetworks` example lists your SSID |
| 1 Light | Strip wired on the bench; `LedStrip` module with gamma + current cap; serial commands set colour | Typing `255 0 0 0` in Serial Monitor turns the strip red; 2 % brightness is smooth, not flickery; no glitches with the data line as built |
| 2 WiFi UI | Web page: on/off, brightness, colour picker, white slider, 4 presets | Phone on WiFi changes the light; survives router reboot (auto-reconnect) |
| 3 Polish | Effects, saved last state, mDNS name, perfboard build (see docs/perfboard.md), enclosure, mounting | Power-cycle restores last colour; `http://roomlight.local` resolves |
| 4 Extras | Button/encoder; MQTT + Home Assistant discovery; OTA upload over WiFi | "Hey Google, lights 30 %" works (if HA used) |

## 7. Software to install (Arduino IDE 1.8.19)

Already present: SAMD core 1.8.12 (MKR board support), Adafruit_NeoPixel 1.15.5
(the driver we use), FastLED 3.10.3 (not needed).

Library Manager (Sketch → Include Library → Manage Libraries):
- `WiFiNINA` (Arduino) — WiFi
- `FlashStorage` (Arduino / cmaglie) — save state
- `ArduinoMDNS` (Arduino) — `.local` name
- `ArduinoMqttClient` (Arduino) — phase 4
- `ArduinoOTA` (Juraj Andrassy) — phase 4, network uploads appear in Tools → Port

Then: Tools → Board → Arduino SAMD Boards → **Arduino MKR WiFi 1010**, and run
Tools → WiFi101 / WiFiNINA Firmware Updater once.

On Linux the serial port needs your user in the `dialout` group
(`sudo usermod -aG dialout $USER`, then log out/in) if upload fails with a
permission error.

## 8. Fallback — if a second strip is the analog 12 V type (Adafruit 4548/4549)

Those have five bare wires (black +12 V, R/G/B/W to ground) and need four
logic-level N-MOSFETs on PWM pins plus a 12 V supply with a buck to 5 V for the
MKR. Only `LedStrip.cpp` would change; per-pixel effects are not possible. Not
planned unless a mixed install comes up.

## 9. Open questions

1. Exactly one strip (1 m), or will more be joined? (sets PSU size: 4 A vs 10 A)
2. Where will it mount (behind a shelf, under a bed, around a window)? Decides
   enclosure, cable run, and whether the button is worth it.
3. Is there an always-on Raspberry Pi or other box at home for Home Assistant?
4. Preference: self-hosted web page, or the Arduino IoT Cloud app?

## 10. Sources

- Adafruit 4689 (this strip): https://www.adafruit.com/product/4689
- Other neon variants, for reference: https://www.adafruit.com/product/4549 (analog),
  https://www.adafruit.com/product/5861 , https://www.adafruit.com/product/5869 (144/m)
- MKR WiFi 1010 docs: https://docs.arduino.cc/hardware/mkr-wifi-1010/
- NeoPixel Überguide (power, level shifting, best practices):
  https://learn.adafruit.com/adafruit-neopixel-uberguide
