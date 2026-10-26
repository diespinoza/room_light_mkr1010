# Final build — perfboard with 74AHCT125 level shifter

Layout drawing: `perfboard.svg` (top view, hole-exact). This document is the
parts list, the connection list, the build order and the test procedure.

Design goals, from what the bench taught us:

- The strip gets the **full 5 V**: no diode, no breadboard, no jumpers in the
  power path. The bench lost 1.4 V that way and the white/blue dies shimmered.
- **One soldered ground node.** Supply −, strip GND and MKR GND meet on the
  board, so the data signal has a solid reference.
- The data line is driven at **5 V** by the 74AHCT125, so the level margin
  that caused the fade flicker is gone.
- The strip current never flows through thin perfboard rail traces: a
  **20 AWG bus wire** on the underside runs straight from POWER IN to the
  strip pins.
- The **1000 µF capacitor sits at the strip pins**, where its job is.

## Parts list

| Qty | Part | Example source | Notes |
|---|---|---|---|
| 1 | Adafruit Perma-Proto half-size breadboard PCB | Adafruit 1609 | breadboard-style traces, so the bench layout transfers 1:1 |
| 1 | 74AHCT125 quad level shifter, DIP-14 | Adafruit 1787 | the one part that fixes the 3.3 V problem |
| 1 | 14-pin DIP socket | any | optional, lets you swap the chip |
| 2 | 1×14 female header, 0.1" | cut from Adafruit 598 or similar | the MKR plugs into these |
| 1 | 470 Ω resistor, 1/4 W | kit | data series resistor (330 Ω also fine) |
| 1 | 1000 µF electrolytic, 10 V or 16 V | kit / Adafruit 1589 | across 5 V/GND at the strip pins |
| 1 | 0.1 µF ceramic | kit | chip decoupling |
| 1 | 2-position screw terminal, 5.08 mm | Adafruit 724 | POWER IN; fits every other hole |
| 1 | 2.1 mm barrel jack, panel mount with wires | Adafruit 610 | goes in the enclosure wall, wires to POWER IN |
| 1 | JST SM 3-pin pigtail pair | Adafruit 1663 | one half solders to the board, the other is already on the strip |
| 1 | 5 V 4 A power supply, 2.1 mm plug | Adafruit 1466 | full white on 60 px with margin |
| — | 20 AWG solid wire, red and black | | the underside power bus |
| — | 22–24 AWG solid wire, assorted | | everything else |
| 1 | Enclosure, approx. 120 × 80 × 40 mm, vented | | plus a cable gland or zip-tie strain relief for the strip lead |
| 4 | M3 standoffs + screws | | board mounting |
| opt | Momentary push button, panel mount | | phase 4, between D7 and GND |

Not needed any more: the 1N4007, the breadboard, the screw-terminal barrel
jack adapter (keep it for bench work).

## Where things go (columns 1–30, rows a–e and f–j, rails top and bottom)

| Item | Location |
|---|---|
| MKR WiFi 1010 | columns 1–14, pins in rows **j** (5V…D6) and **a** (AREF…D5), USB end at column 1 |
| 74AHCT125 | pin 1 at column 17 row **e**; pins 1–7 along row e (cols 17–23), pins 8–14 along row f (cols 23–17) |
| POWER IN terminal | columns 27 (+) and 29 (−), top edge |
| Strip pigtail | columns 26 (DATA), 28 (5V), 30 (GND), bottom edge |
| 1000 µF | column 28 (+) to column 30 (−), row d |
| 0.1 µF | column 17 row h to top GND rail |
| 470 Ω | column 19 row b to column 25 row b |

## Connection list

Power

| From | To | Wire |
|---|---|---|
| POWER IN + (col 27) | top + rail | red, 22 AWG |
| POWER IN − (col 29) | top GND rail | black, 22 AWG |
| POWER IN + (col 27, underside) | strip 5V (col 28, underside) | **red 20 AWG bus** |
| POWER IN − (col 29, underside) | strip GND (col 30, underside) | **black 20 AWG bus** |
| top + rail | bottom + rail (around the column-1 end) | red |
| top GND rail | bottom GND rail (around the column-1 end) | black |
| MKR VIN (col 2, row i) | top + rail | red |
| MKR GND (col 4, row h) | top GND rail | black |
| 74AHCT125 VCC pin 14 (col 17, row g) | top + rail | red |
| 74AHCT125 GND pin 7 (col 23, row d) | bottom GND rail | black |
| 0.1 µF | col 17 row h ↔ top GND rail | |
| 1000 µF | + at col 28 row d, − (stripe) at col 30 row d | |

Data

| From | To | Wire |
|---|---|---|
| MKR D6 (col 14, row i) | 1A, pin 2 (col 18, row c) | green |
| 1Y, pin 3 (col 19, row b) | 470 Ω | |
| 470 Ω | strip DATA (col 26, row b) | green |

Tie-offs, so no pin floats (all to GND)

| Pin | Location |
|---|---|
| 1OE (pin 1) | col 17 row b → bottom GND rail |
| 2OE (pin 4) | col 20 row b → bottom GND rail |
| 2A (pin 5) | col 21 row b → bottom GND rail |
| 4OE (pin 13) | col 18 row i → top GND rail |
| 4A (pin 12) | col 19 row i → top GND rail |
| 3OE (pin 10) | col 21 row i → top GND rail |
| 3A (pin 9) | col 22 row i → top GND rail |

Pulling 1OE low enables buffer 1. The other three buffers are disabled and
their inputs grounded.

## Build order

1. Solder the two 14-pin female headers in rows j and a, columns 1–14. Plug
   the MKR in while soldering so they stay aligned, then remove it.
2. Solder the DIP socket (or the chip) with pin 1 at column 17 row e.
3. Solder the screw terminal at columns 27/29 top edge and the pigtail wires
   at columns 26/28/30 bottom edge.
4. Underside: solder the two 20 AWG bus wires from the POWER IN pins to the
   strip 5V/GND pins. Keep them short and flat against the board.
5. Solder the 1000 µF (watch the stripe), the 0.1 µF and the 470 Ω.
6. Top-side hookup wires per the connection list. Do the GND tie-offs last.
7. Check with a multimeter, **MKR and chip not fitted**: POWER IN + to strip
   5V pin = 0 Ω; POWER IN − to strip GND = 0 Ω; + to − = open (not a short);
   + rail to MKR VIN hole = 0 Ω; GND rail to MKR GND hole = 0 Ω.
8. Fit the chip, then the MKR. Set `CURRENT_CAP_MA` to 3500 in `config.h`
   and upload over USB.

## Test procedure

1. Power from the 4 A supply with the strip unplugged. Measure 5 V on the
   strip 5V pin and 3.3 V on the MKR VCC pin.
2. `pintest` in the Serial Monitor: the strip DATA pin should read **~5 V**
   now, not 3.3 V. That proves the level shifter is working.
3. Plug the strip in (power off first). Power up: soft white at boot.
4. `test`: all six steps clean, including 5 and 6, with the limiter on.
5. `255 255 255 255` at `bri 255`: measure the supply current. Record it here
   and adjust `MA_PER_DIE` if the estimate from `status` is off by more than
   ~15%.
6. Scope the strip 5 V pin at full white: expect close to 5 V with well under
   100 mV of ripple. Compare to the bench's 3.7 V and 300 mV.

## Enclosure notes

- Barrel jack and (optional) button on the wall; strip lead through a gland
  or wrapped once around a zip-tie anchor inside, so a tug never reaches the
  first pixel's solder joint.
- Leave the MKR's USB port reachable, or plan on OTA updates in phase 4.
- A few vent holes: the MKR's regulator and the chip barely warm up, but a
  closed box in a warm room adds up.
