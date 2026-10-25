# room_light_mkr1010

WiFi room light: Arduino MKR WiFi 1010 driving an Adafruit 4689 NeoPixel RGBW
neon strip (60 px, 5 V, 5500K). See `PLAN.md` for the full design.

## Current state: phase 1 — bench test

The sketch drives the strip and takes colour commands over USB serial. No WiFi
yet.

### Wiring for the bench

```
 5V PSU (+) ──┬── 1000 µF ──┐ ──────────► strip JST 5V   (red)
 5V PSU (−) ──┴─────────────┴──┬────────► strip JST GND  (black)
                               └────────► MKR GND
 MKR D6 ──470Ω──────────────────────────► strip JST DATA (middle wire)
 MKR USB → computer (power + serial)
```

Verify the JST wire colours on your strip first. Connect ground before data.
If colours glitch or flicker, put a 74AHCT125 between D6 and the data wire.
The bench supply is 5 V 2 A, so `CURRENT_CAP_MA` in `config.h` is 1500 for now;
raise it to 3500 once a 4 A supply is in place.

### Board support on this Raspberry Pi (64-bit)

Install **Arduino SAMD Boards version 1.8.12**, not the newest. In Boards
Manager, search "SAMD", pick 1.8.12 from the version dropdown next to Install.
Versions 1.8.13 and 1.8.14 depend on an avrdude build that does not exist for
64-bit ARM Linux, which produces "Tool avrdude is not available for your
operating system".

### Upload

1. Tools → Board → Arduino SAMD Boards → Arduino MKR WiFi 1010.
2. Tools → Port → the MKR (`/dev/ttyACM0` or similar).
3. Upload, then open Serial Monitor at 115200 with line ending "Newline".

### Serial commands

| Command | Effect |
|---|---|
| `255 80 0 0` | colour as `r g b w`, fades in |
| `white 200` | white die only |
| `off` | fade to dark |
| `bri 64` | master brightness 0..255 |
| `fade 1000` | fade time in ms, `fade 0` for instant |
| `cap 2000` | current cap in mA for this session |
| `pix 0 255 0 0 0` | one pixel, no fade — pixel 0 is at the JST end |
| `test` | red, green, blue, white, all, off, one second each |
| `pintest` | holds D6 HIGH for 10 s so a multimeter can read it (~3.3 V) |
| `status` | colour, brightness, estimated mA, limiter state |

### If the strip stays dark

A multimeter cannot see NeoPixel data: the pin is at 0 V except for a 2 ms
burst each time a frame is sent, and a frame is only sent when something
changes. To test the wiring, type `pintest` in the Serial Monitor. D6 then
sits at about 3.3 V for 10 s, measurable at the pin, at the far end of the
resistor, and at the strip's data wire. If the voltage is there and `test`
still shows nothing:

1. Check the alligator clips. A loose data clip is the usual cause.
2. Confirm the JST wire you clipped is data (the middle pin) and that the
   strip is actually powered (5 V between red and black at the strip).
3. The 3.3 V data level may be too low for a 5 V strip. Quick check without a
   level shifter: put a 1N4001 or similar diode in series with the strip's
   red (+5 V) wire, so the strip runs at ~4.3 V and accepts 3.3 V data. If
   that fixes it, fit a 74AHCT125 for the real build and drop the diode.

### If the strip flickers during fades (3.3 V data into a 5 V strip)

Static colours hold but fades show bursts of random colours, and the boot
frame is partly wrong. The MKR's 3.3 V data is right at the strip's threshold,
so some frames are misread. Mitigations, cheapest first:

1. **Wiring.** Lose the alligator clips on the data path: one jumper from D6
   through the resistor straight to the strip's data wire, as short as
   possible. Drop the resistor to 220–330 Ω (550 Ω slows the edges). Run the
   strip's GND to the MKR GND with a short, solid wire and keep it next to the
   data wire. Flicker that improves with this is marginal-level trouble.
2. **Confirm the diagnosis.** Power the strip from 3×AA (4.5 V) or 4×NiMH
   (4.8 V) for a minute. At that voltage 3.3 V data is well above threshold;
   if the flicker vanishes, the level is the only problem.
3. **Diode trick.** Any rectifier diode rated 1 A or more (1N4001–1N4007,
   common in starter kits) in series with the strip's red wire drops it to
   about 4.3 V. Not a 1N4148 signal diode: it cannot carry the current.
4. **Proper fix.** A 74AHCT125 (Adafruit 1787) or 74HCT245 between D6 and the
   data wire, powered from 5 V. About $1.50; this is what the final build gets.
5. **Two-transistor shifter.** Two NPN inverters in series (PN2222/2N3904,
   1 kΩ base resistors, 1 kΩ pull-ups to 5 V) give a non-inverted 5 V signal.
   Works, but a logic chip is cleaner.

The sketch also resends the frame every second when idle so a corrupted static
frame fixes itself.

### If the strip shimmers at higher brightness (static colour, no data sent)

Measured 2026-10-25 with the 1N4007 in the + lead and a 5 V 2 A adapter: the
rail at the strip was 3.9 V at a 500 mA cap and 3.7 V at 1200 mA, with
270–370 mV of ripple at 1.3–1.7 kHz. That ripple is the strip's own LED PWM
pulling current in pulses through the diode and wiring resistance. Below
about 4 V the blue and white dies lose regulation and their brightness
follows the ripple, which looks like shimmer. Stable up to a 350 mA cap.

Cure is voltage at the strip, not filtering: remove the diode and use a
74AHCT125 for the data line so the strip gets the full 5 V; short, thick
power leads; capacitor right at the JST. A Schottky diode (1N5819/1N5822,
~0.4 V drop) is a stop-gap that roughly doubles the usable current.

### What to check in phase 1

- The strip comes up soft white a few seconds after reset.
- `test` shows pure red, green, blue, then white in that order. If the colours
  are swapped, change `PIXEL_TYPE` in `config.h`.
- `bri 5` then `255 255 255 255` is very dim but every pixel is lit.
- `255 255 255 255` at `bri 255` prints `current limiter ON` and `status`
  shows est ≈ cap. Measure the real current with a meter and adjust
  `MA_PER_DIE` in `config.h` if the estimate is far off.

## Diagrams

- `docs/wiring.svg` — bench wiring, hand-drawn.
- `docs/fritzing/room_light_bench.fzz` — the same wiring as a Fritzing sketch
  (breadboard view). The Arduino MKR WiFi 1010 part is bundled inside; the
  neon strip is represented by the core library's WS2815 strip part. Expect to
  nudge wires after opening — the file was generated, not drawn by hand.
