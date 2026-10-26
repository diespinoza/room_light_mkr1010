#!/usr/bin/env python3
"""Draw docs/perfboard.svg: room light controller on an Adafruit Perma-Proto
half-size board (30 columns, rows a-e / f-j, power rails top and bottom)."""
import sys

P = 20                      # hole pitch in px
X0 = 70                     # column 1 x
ROWS_TOP = ['j', 'i', 'h', 'g', 'f']
ROWS_BOT = ['e', 'd', 'c', 'b', 'a']
Y = {}
y = 120
for r in ROWS_TOP: Y[r] = y; y += P
y += 2 * P                  # centre channel
for r in ROWS_BOT: Y[r] = y; y += P
Y['rail+top'] = 60; Y['rail-top'] = 80
Y['rail+bot'] = Y['a'] + 2 * P + 20; Y['rail-bot'] = Y['rail+bot'] + 20
W = X0 + 30 * P + 40
H = Y['rail-bot'] + 190

def cx(c): return X0 + (c - 1) * P
def pt(c, r): return (cx(c), Y[r])

out = []
def add(s): out.append(s)

add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="Helvetica, Arial, sans-serif">')
add('''<style>
.t{font-size:11px;fill:#222}.ts{font-size:9px;fill:#444}.tt{font-size:16px;font-weight:bold;fill:#222}
.lab{font-size:8px;fill:#fff;font-weight:bold}
.w5{stroke:#d62828;stroke-width:3;fill:none;stroke-linecap:round;stroke-linejoin:round}
.wg{stroke:#111;stroke-width:3;fill:none;stroke-linecap:round;stroke-linejoin:round}
.wd{stroke:#2a9d3a;stroke-width:3;fill:none;stroke-linecap:round;stroke-linejoin:round}
.wb{stroke:#1d6fd6;stroke-width:3;fill:none;stroke-linecap:round;stroke-linejoin:round}
.bus{stroke:#d62828;stroke-width:5;fill:none;stroke-dasharray:8 5;opacity:.8}
.busg{stroke:#111;stroke-width:5;fill:none;stroke-dasharray:8 5;opacity:.8}
</style>''')
add(f'<rect width="{W}" height="{H}" fill="#fff"/>')
add('<text x="20" y="28" class="tt">room_light_mkr1010 — perfboard layout (Adafruit Perma-Proto half-size, top view)</text>')

# board
add(f'<rect x="{X0-40}" y="{Y["rail+top"]-25}" width="{30*P+60}" height="{Y["rail-bot"]-Y["rail+top"]+50}" rx="6" fill="#1b5e3a"/>')
# rails
for name, col, lab in (('rail+top', '#d62828', '+ 5 V'), ('rail-top', '#111', 'GND'), ('rail+bot', '#d62828', '+ 5 V'), ('rail-bot', '#111', 'GND')):
    yy = Y[name]
    add(f'<line x1="{cx(1)-8}" y1="{yy}" x2="{cx(30)+8}" y2="{yy}" stroke="{col}" stroke-width="1.5" opacity=".7"/>')
    add(f'<text x="{cx(1)-34}" y="{yy+3}" font-size="8" fill="#fff" font-weight="bold">{lab}</text>')
    for c in range(1, 31):
        add(f'<circle cx="{cx(c)}" cy="{yy}" r="3" fill="#cfd8c9"/>')
# column strips a-e and f-j
for c in range(1, 31):
    for grp in (ROWS_TOP, ROWS_BOT):
        add(f'<line x1="{cx(c)}" y1="{Y[grp[0]]}" x2="{cx(c)}" y2="{Y[grp[-1]]}" stroke="#8aa88f" stroke-width="1"/>')
        for r in grp: add(f'<circle cx="{cx(c)}" cy="{Y[r]}" r="3" fill="#cfd8c9"/>')
    add(f'<text x="{cx(c)}" y="{Y["j"]-10}" text-anchor="middle" font-size="8" fill="#fff">{c}</text>')
for r in ROWS_TOP + ROWS_BOT:
    add(f'<text x="{cx(1)-18}" y="{Y[r]+3}" font-size="8" fill="#fff">{r}</text>')

# ---- MKR WiFi 1010 in columns 1-14, pins in rows j and a (female headers)
mx0, mx1 = cx(1) - 12, cx(14) + 12
add(f'<rect x="{mx0}" y="{Y["j"]-14}" width="{mx1-mx0}" height="{Y["a"]-Y["j"]+28}" rx="6" fill="#1f8a9e" opacity=".55" stroke="#0d5560" stroke-width="2"/>')
add(f'<text x="{(mx0+mx1)/2}" y="{(Y["f"]+Y["e"])/2+4}" text-anchor="middle" font-size="12" fill="#fff" font-weight="bold">MKR WiFi 1010 (USB end ←)</text>')
add(f'<rect x="{mx0-10}" y="{(Y["f"]+Y["e"])/2-10}" width="14" height="20" fill="#aaa"/>')
top_names = ['5V', 'VIN', 'VCC', 'GND', 'RST', 'D14', 'D13', 'D12', 'D11', 'D10', 'D9', 'D8', 'D7', 'D6']
bot_names = ['AREF', 'A0', 'A1', 'A2', 'A3', 'A4', 'A5', 'A6', 'D0', 'D1', 'D2', 'D3', 'D4', 'D5']
for i, n in enumerate(top_names):
    add(f'<text x="{cx(i+1)}" y="{Y["j"]-4}" text-anchor="middle" font-size="7" fill="#fff" transform="rotate(-60 {cx(i+1)} {Y["j"]-4})">{n}</text>')
for i, n in enumerate(bot_names):
    add(f'<text x="{cx(i+1)}" y="{Y["a"]+12}" text-anchor="middle" font-size="7" fill="#fff" transform="rotate(-60 {cx(i+1)} {Y["a"]+12})">{n}</text>')

# ---- 74AHCT125, pin 1 at column 17 row e, pins 1-7 along row e, 8-14 along row f
c0 = 17
add(f'<rect x="{cx(c0)-10}" y="{Y["f"]-8}" width="{6*P+20}" height="{Y["e"]-Y["f"]+16}" rx="3" fill="#2b2b2b"/>')
add(f'<circle cx="{cx(c0)-2}" cy="{Y["e"]-2}" r="3" fill="#ddd"/>')
add(f'<text x="{cx(c0)+3*P}" y="{(Y["f"]+Y["e"])/2+4}" text-anchor="middle" font-size="11" fill="#fff" font-weight="bold">74AHCT125</text>')
pins_bot = ['1OE', '1A', '1Y', '2OE', '2A', '2Y', 'GND']      # pins 1-7, columns 17-23, row e
pins_top = ['VCC', '4OE', '4A', '4Y', '3OE', '3A', '3Y']      # pins 14-8, columns 17-23, row f
for i, n in enumerate(pins_bot):
    add(f'<text x="{cx(c0+i)}" y="{Y["e"]+6}" text-anchor="middle" font-size="7" fill="#fff" transform="rotate(90 {cx(c0+i)} {Y["e"]+6})">{i+1} {n}</text>')
for i, n in enumerate(pins_top):
    add(f'<text x="{cx(c0+i)}" y="{Y["f"]-6}" text-anchor="end" font-size="7" fill="#fff" transform="rotate(90 {cx(c0+i)} {Y["f"]-6})">{14-i} {n}</text>')

def wire(cls, *pts):
    add(f'<polyline class="{cls}" points="{" ".join(f"{x},{y}" for x,y in pts)}"/>')

# ---- Power from the input terminal (columns 27,29 top side) to rails and to the strip (columns 26,28,30 bottom)
# input terminal
add(f'<rect x="{cx(27)-14}" y="{Y["j"]-26}" width="{2*P+28}" height="{P+30}" rx="3" fill="#2f6f3e"/>')
add(f'<circle cx="{cx(27)}" cy="{Y["j"]-10}" r="6" fill="#aaa"/><circle cx="{cx(29)}" cy="{Y["j"]-10}" r="6" fill="#aaa"/>')
add(f'<text x="{cx(28)}" y="{Y["j"]-34}" text-anchor="middle" class="ts">POWER IN 5 V 4 A (+ | −)</text>')
# strip terminal / pigtail
add(f'<rect x="{cx(26)-14}" y="{Y["a"]+6}" width="{4*P+28}" height="{P+30}" rx="3" fill="#2f6f3e"/>')
for c, lab in ((26, 'DATA'), (28, '5V'), (30, 'GND')):
    add(f'<circle cx="{cx(c)}" cy="{Y["a"]+22}" r="6" fill="#aaa"/>')
    add(f'<text x="{cx(c)}" y="{Y["a"]+48}" text-anchor="middle" font-size="8" fill="#fff" font-weight="bold">{lab}</text>')
add(f'<text x="{cx(28)}" y="{Y["a"]+62}" text-anchor="middle" class="ts">TO STRIP — JST SM 3-pin pigtail</text>')

# rails fed from input: col 27 i -> +rail top ; col 29 i -> -rail top
wire('w5', pt(27, 'i'), (cx(27), Y['rail+top']))
wire('wg', pt(29, 'h'), (cx(29) + 6, Y['rail-top']))
# heavy bus wires (underside) from input to strip terminal, bypassing rails
wire('bus', pt(27, 'g'), (cx(27), Y['f'] + 25), (cx(28), Y['e'] - 25), pt(28, 'c'))
wire('busg', pt(29, 'g'), (cx(29), Y['f'] + 25), (cx(30), Y['e'] - 25), pt(30, 'c'))
# 1000 uF across strip 5V/GND at columns 28/30 row d
add(f'<rect x="{cx(28)-6}" y="{Y["d"]-44}" width="{2*P+12}" height="38" rx="5" fill="#1d3f8a"/>')
add(f'<rect x="{cx(30)-2}" y="{Y["d"]-44}" width="8" height="38" fill="#cfd8ea"/>')
add(f'<text x="{cx(29)}" y="{Y["d"]-22}" text-anchor="middle" font-size="8" fill="#fff" font-weight="bold">1000 µF</text>')
add(f'<text x="{cx(29)}" y="{Y["d"]-50}" text-anchor="middle" class="ts">stripe (−) at col 30</text>')
# bottom rails linked to top rails at column 1 end (two short wires around the board edge)
wire('w5', (cx(1) - 8, Y['rail+top']), (cx(1) - 30, Y['rail+top']), (cx(1) - 30, Y['rail+bot']), (cx(1) - 8, Y['rail+bot']))
wire('wg', (cx(1) - 8, Y['rail-top']), (cx(1) - 24, Y['rail-top']), (cx(1) - 24, Y['rail-bot']), (cx(1) - 8, Y['rail-bot']))

# ---- MKR power: VIN col 2 row i -> +rail top ; GND col 4 row h -> -rail top
wire('w5', pt(2, 'i'), (cx(2), Y['rail+top']))
wire('wg', pt(4, 'h'), (cx(4) + 6, Y['rail-top']))

# ---- Chip power: VCC pin14 col 17 row g -> +rail top ; GND pin7 col 23 row d -> -rail bottom
wire('w5', pt(17, 'g'), (cx(17), Y['f'] - 30), (cx(16), Y['f'] - 30), (cx(16), Y['rail+top']))
wire('wg', pt(23, 'd'), (cx(23), Y['rail-bot']))
# 0.1 uF between col 17 h and -rail top
add(f'<rect x="{cx(17)-5}" y="{Y["h"]-6}" width="10" height="14" rx="3" fill="#e3b341"/>')
wire('wb', (cx(17), Y['h'] - 6), (cx(17) + 10, Y['h'] - 22), (cx(17) + 10, Y['rail-top']))
add(f'<text x="{cx(17)+16}" y="{Y["h"]+2}" class="ts">0.1 µF</text>')
# unused enable/input pins to GND
for c, r in ((17, 'b'), (20, 'b'), (21, 'b')):        # 1OE, 2OE, 2A
    wire('wg', pt(c, r), (cx(c), Y['rail-bot']))
for c in (18, 19, 21, 22):                             # 4OE, 4A, 3OE, 3A
    wire('wg', pt(c, 'i'), (cx(c) + 5, Y['rail-top']))

# ---- Data: MKR D6 col 14 row i -> 1A col 18 row c ; 1Y col 19 row b -> 470R -> col 25 row b -> DATA col 26 row b
wire('wd', pt(14, 'i'), (cx(14), Y['f'] + 25), (cx(18), Y['e'] - 25), pt(18, 'c'))
wire('wd', pt(19, 'b'), (cx(19), Y['b'] - 0))
add(f'<rect x="{cx(19)+6}" y="{Y["b"]-6}" width="{6*P-12}" height="12" rx="4" fill="#e9d5a8" stroke="#8a6d3b"/>')
add(f'<text x="{cx(22)}" y="{Y["b"]+3}" text-anchor="middle" font-size="8" fill="#333">470 Ω</text>')
wire('wd', (cx(19) + 6, Y['b']), pt(19, 'b'))
wire('wd', (cx(25) + P - 6, Y['b']), pt(26, 'b'))

# ---- legend
ly = Y['rail-bot'] + 40
lines = [
    ('Wires: ', 'red = 5 V, black = GND, green = data, blue = decoupling.'),
    ('', 'Dashed = heavy 20 AWG bus on the underside, POWER IN straight to the strip pins,'),
    ('', 'so the thin rail traces only feed the MKR and the chip.'),
    ('Board: ', 'holes a–e in a column are joined, f–j are joined; four rails run the length.'),
    ('', 'MKR plugs into two 14-pin female headers in rows j and a, USB end at column 1.'),
    ('Chip: ', '74AHCT125, pin 1 at column 17 row e. Only buffer 1 is used:'),
    ('', 'D6 → 1A, 1Y → 470 Ω → strip DATA. Enables and unused inputs tied to GND.'),
    ('Button: ', '(phase 4) between MKR D7 (column 13, row i) and GND; sketch uses INPUT_PULLUP.'),
]
for i, (b, t) in enumerate(lines):
    add(f'<text x="20" y="{ly + 16*i}" class="t"><tspan font-weight="bold">{b}</tspan>{t}</text>')
add('</svg>')
open(sys.argv[1], 'w').write('\n'.join(out))
print('wrote', sys.argv[1])
