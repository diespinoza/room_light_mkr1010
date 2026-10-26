#!/usr/bin/env python3
"""Generate Fritzing sketches (breadboard view) for room_light_mkr1010.

    python3 make_fzz.py bench <out_dir> <unpacked ABX00023-fritzing.fzpz dir>
    python3 make_fzz.py final <out_dir> <unpacked ABX00023-fritzing.fzpz dir>

bench: diode + 1000 uF bench setup (the committed room_light_bench.fzz was
       generated this way, then tidied by hand in Fritzing).
final: 74AHCT125 level shifter, no diode (room_light_final.fzz).

Parts come from the Fritzing core library; Arduino's MKR WiFi 1010 part is
bundled into the .fzz so the file opens anywhere. Connector positions were
read from the parts' breadboard SVGs (90 dpi) so wires land on the pins.
"""
import os, sys, zipfile, html

VARIANT, OUT_DIR, FZPZ_DIR = sys.argv[1], sys.argv[2], sys.argv[3]
CORE = '/home/pearl/.local/share/fritzing/parts/core/'
USER_FZP = '/home/pearl/Documents/Fritzing/parts/user/MKR_WIFI_1010_6dcd07c87dc811b49284a0769a3386cb_1.fzp'
RED, BLK, GRN, BLU = '#cc1414', '#404040', '#25cc40', '#418dd9'

# ---- part catalogue: key -> (moduleId, fzp path, connector offsets {id: (dx, dy)})
CAT = {
  'mkr':   ('MKR_WIFI_1010_6dcd07c87dc811b49284a0769a3386cb_1', USER_FZP,
            {'connector67': (212.0, 6.4),   # D6
             'connector57': (116.1, 6.4),   # GND
             'connector90': (96.9, 6.4)}),  # VIN
  'jack':  ('SparkFun-Connectors-POWER_JACK-SLT', CORE+'sparkfun-connectors-power_jack-slt.fzp',
            {'connector2': (29.4, 4.5), 'connector1': (29.4, 85.5)}),            # PWR, GND
  'term':  ('Camdenboss_CTB0158-2_5_08mm_pitch_90deg_terminals', CORE+'Camdenboss_CTB0158-2_5_08mm_pitch_90deg_terminals.fzp',
            {'connector0': (9.0, 31.5), 'connector1': (27.0, 31.5),              # pin1, pin2
             'connector2': (9.0, 22.5), 'connector3': (27.0, 22.5)}),            # terminal 1 (+), terminal 2 (-)
  'diode': ('3254CBFC44diode', CORE+'diode_1N4001_300mil.fzp',
            {'connector1': (36.7, 4.6), 'connector0': (0.6, 4.5)}),              # anode, cathode
  'cap':   ('MediumElectrolyticCapacitorModuleID', CORE+'capacitor_electrolytic_medium.fzp',
            {'connector1': (18.7, 47.2), 'connector0': (9.7, 47.2)}),            # +, -
  'cer':   ('200milCeramicCapacitorModuleID', CORE+'capacitor_ceramic_200mil.fzp',
            {'connector0': (4.0, 13.8), 'connector1': (13.0, 13.8)}),
  'res':   ('ResistorModuleID', CORE+'resistor.fzp',
            {'connector0': (0.9, 3.7), 'connector1': (37.7, 3.7)}),
  'jst':   ('JST-B3P-VH', CORE+'JST-B3P-VH.fzp',
            {'connector0': (6.9, 20.2), 'connector1': (20.9, 20.2), 'connector2': (35.0, 20.2)}),  # 5V, DATA, GND
  'strip': ('WS2815_RGB_LED_strip_17b5c981895fad978faf4675cba95063_37', CORE+'ws2815.fzp',
            {'connector1': (1.4, 14.9), 'connector2': (1.4, 10.6), 'connector4': (1.4, 2.1)}),  # VCC, DIN, GND
  # SparkFun 74ACT125 (SOIC) — same pinout as the 74AHCT125 DIP we use.
  # connectorN = pin N+1: 0=1OE 1=1A 2=1Y 3=2OE 4=2A 5=2Y 6=GND 7=3Y 8=3A 9=3OE 10=4Y 11=4A 12=4OE 13=VCC
  'ic':    ('SparkFun-DigitalIC-74ACT125D-', CORE+'sparkfun-digitalic-74act125d-.fzp',
            {f'connector{i}': ((3.1 + 9.0 * i, 48.1) if i < 7 else (3.1 + 9.0 * (13 - i), 3.1)) for i in range(14)}),
}

if VARIANT == 'bench':
    NAME = 'room_light_bench'
    PARTS = {   # key: (title, x, y, properties)
      'mkr':   ('Arduino MKR WiFi 1010', 20, 20, {}),
      'jack':  ('Barrel jack (5 V 2 A supply)', 40, 220, {}),
      'term':  ('Screw terminal', 130, 250, {}),
      'diode': ('1N4007 (band toward strip)', 230, 200, {'part number': '1N4007'}),
      'cap':   ('1000 uF', 300, 160, {'capacitance': '1000µF', 'voltage': '6.3V'}),
      'res':   ('330 ohm data resistor', 330, 100, {'resistance': '330Ω'}),
      'jst':   ('JST SM 3-pin (strip input)', 380, 250, {}),
      'strip': ('Adafruit 4689 NeoPixel RGBW neon strip (stand-in)', 460, 240, {}),
    }
    WIRES = [
      (('jack','connector2'), ('term','connector0'), RED, 'jack + to terminal'),
      (('jack','connector1'), ('term','connector1'), BLK, 'jack - to terminal'),
      (('term','connector2'), ('diode','connector1'), RED, 'terminal + to diode anode'),
      (('diode','connector0'), ('cap','connector1'), RED, 'diode cathode to cap +'),
      (('cap','connector1'), ('jst','connector0'), RED, 'cap + to strip 5V'),
      (('term','connector3'), ('cap','connector0'), BLK, 'terminal - to cap -'),
      (('cap','connector0'), ('jst','connector2'), BLK, 'cap - to strip GND'),
      (('term','connector3'), ('mkr','connector57'), BLK, 'terminal - to MKR GND'),
      (('mkr','connector67'), ('res','connector0'), GRN, 'MKR D6 to resistor'),
      (('res','connector1'), ('jst','connector1'), GRN, 'resistor to strip DATA'),
      (('jst','connector0'), ('strip','connector1'), RED, 'JST 5V to strip VCC'),
      (('jst','connector1'), ('strip','connector2'), GRN, 'JST DATA to strip DIN'),
      (('jst','connector2'), ('strip','connector4'), BLK, 'JST GND to strip GND'),
    ]
elif VARIANT == 'final':
    NAME = 'room_light_final'
    PARTS = {
      'mkr':   ('Arduino MKR WiFi 1010 (VIN from 5 V rail)', 20, 20, {}),
      'jack':  ('Barrel jack (5 V 4 A supply)', 40, 220, {}),
      'term':  ('POWER IN screw terminal', 130, 250, {}),
      'ic':    ('74AHCT125 level shifter (74ACT125 stand-in, same pinout)', 230, 150, {}),
      'cer':   ('0.1 uF decoupling', 310, 140, {'capacitance': '100nF'}),
      'res':   ('470 ohm data resistor', 330, 100, {'resistance': '470Ω'}),
      'cap':   ('1000 uF at the strip', 380, 160, {'capacitance': '1000µF', 'voltage': '10V'}),
      'jst':   ('JST SM 3-pin pigtail (to strip)', 440, 250, {}),
      'strip': ('Adafruit 4689 NeoPixel RGBW neon strip (stand-in)', 520, 240, {}),
    }
    WIRES = [
      (('jack','connector2'), ('term','connector0'), RED, 'jack + to POWER IN'),
      (('jack','connector1'), ('term','connector1'), BLK, 'jack - to POWER IN'),
      (('term','connector2'), ('cap','connector1'), RED, '5 V bus to cap + (20 AWG)'),
      (('cap','connector1'), ('jst','connector0'), RED, 'cap + to strip 5V'),
      (('term','connector3'), ('cap','connector0'), BLK, 'GND bus to cap - (20 AWG)'),
      (('cap','connector0'), ('jst','connector2'), BLK, 'cap - to strip GND'),
      (('term','connector2'), ('mkr','connector90'), RED, '5 V to MKR VIN'),
      (('term','connector3'), ('mkr','connector57'), BLK, 'GND to MKR GND'),
      (('term','connector2'), ('ic','connector13'), RED, '5 V to 74AHCT125 VCC'),
      (('ic','connector6'), ('term','connector3'), BLK, '74AHCT125 GND'),
      (('ic','connector13'), ('cer','connector0'), BLU, 'VCC to 0.1 uF'),
      (('cer','connector1'), ('ic','connector6'), BLU, '0.1 uF to GND'),
      (('mkr','connector67'), ('ic','connector1'), GRN, 'MKR D6 to 1A'),
      (('ic','connector2'), ('res','connector0'), GRN, '1Y to resistor'),
      (('res','connector1'), ('jst','connector1'), GRN, 'resistor to strip DATA (5 V logic)'),
      (('ic','connector0'), ('ic','connector6'), BLK, '1OE to GND (enable buffer 1)'),
      (('ic','connector3'), ('ic','connector6'), BLK, '2OE to GND'),
      (('ic','connector4'), ('ic','connector6'), BLK, '2A to GND'),
      (('ic','connector9'), ('ic','connector6'), BLK, '3OE to GND'),
      (('ic','connector8'), ('ic','connector6'), BLK, '3A to GND'),
      (('ic','connector12'), ('ic','connector6'), BLK, '4OE to GND'),
      (('ic','connector11'), ('ic','connector6'), BLK, '4A to GND'),
      (('jst','connector0'), ('strip','connector1'), RED, 'JST 5V to strip VCC'),
      (('jst','connector1'), ('strip','connector2'), GRN, 'JST DATA to strip DIN'),
      (('jst','connector2'), ('strip','connector4'), BLK, 'JST GND to strip GND'),
    ]
else:
    sys.exit('variant must be bench or final')

def pos(key, cid):
    _, _, offs = CAT[key]; _, x, y, _ = PARTS[key]
    dx, dy = offs[cid]; return (x + dx, y + dy)

idx, n = {}, 1000
for k in PARTS: n += 1; idx[k] = n
wire_idx = []
for _ in WIRES: n += 1; wire_idx.append(n)

part_links = {}
for (a, b, _, _), wi in zip(WIRES, wire_idx):
    part_links.setdefault(a, []).append(wi); part_links.setdefault(b, []).append(wi)

def part_xml(k):
    mid, path, _ = CAT[k]; title, x, y, props = PARTS[k]
    conns = ''
    for (pk, cid), wires in part_links.items():
        if pk != k: continue
        cs = ''
        for wi in wires:
            w = WIRES[wire_idx.index(wi)]
            end = 'connector0' if w[0] == (pk, cid) else 'connector1'
            cs += f'<connect connectorId="{end}" modelIndex="{wi}" layer="breadboardWire"/>'
        conns += f'<connector connectorId="{cid}" layer="breadboardbreadboard"><geometry x="0" y="0"/><connects>{cs}</connects></connector>'
    propxml = ''.join(f'<property name="{html.escape(a)}" value="{html.escape(v)}"/>' for a, v in props.items())
    return f'''
    <instance moduleIdRef="{mid}" modelIndex="{idx[k]}" path="{html.escape(path)}">
      <title>{html.escape(title)}</title>{propxml}
      <views>
        <breadboardView layer="breadboardbreadboard">
          <geometry z="1.5" x="{x}" y="{y}"/>
          <connectors>{conns}</connectors>
        </breadboardView>
        <schematicView layer="schematic"><geometry z="1.5" x="{x}" y="{y}"/></schematicView>
        <pcbView layer="copper0"><geometry z="1.5" x="{x}" y="{y}"/></pcbView>
      </views>
    </instance>'''

def wire_xml(i):
    (a, b, colour, title) = WIRES[i]; wi = wire_idx[i]
    (x1, y1), (x2, y2) = pos(*a), pos(*b)
    return f'''
    <instance moduleIdRef="WireModuleID" modelIndex="{wi}" path=":/resources/parts/core/wire.fzp">
      <title>{html.escape(title)}</title>
      <views>
        <breadboardView layer="breadboardWire">
          <geometry z="3.5" x="{x1:.1f}" y="{y1:.1f}" x1="0" y1="0" x2="{x2-x1:.1f}" y2="{y2-y1:.1f}" wireFlags="64"/>
          <wireExtras mils="22.2222" color="{colour}" opacity="1" banded="0"/>
          <connectors>
            <connector connectorId="connector0" layer="breadboardWire"><geometry x="0" y="0"/>
              <connects><connect connectorId="{a[1]}" modelIndex="{idx[a[0]]}" layer="breadboardbreadboard"/></connects></connector>
            <connector connectorId="connector1" layer="breadboardWire"><geometry x="0" y="0"/>
              <connects><connect connectorId="{b[1]}" modelIndex="{idx[b[0]]}" layer="breadboardbreadboard"/></connects></connector>
          </connectors>
        </breadboardView>
      </views>
    </instance>'''

fz = f'''<?xml version="1.0" encoding="UTF-8"?>
<module fritzingVersion="0.9.6" icon=".png">
  <title>{NAME}</title>
  <views>
    <view name="breadboardView" backgroundColor="#ffffff" gridSize="0.1in" showGrid="1" alignToGrid="1" viewFromBelow="0"/>
    <view name="schematicView" backgroundColor="#ffffff" gridSize="0.1in" showGrid="1" alignToGrid="1" viewFromBelow="0"/>
    <view name="pcbView" backgroundColor="#333333" gridSize="0.05in" showGrid="1" alignToGrid="1" viewFromBelow="0"/>
  </views>
  <instances>{''.join(part_xml(k) for k in PARTS)}{''.join(wire_xml(i) for i in range(len(WIRES)))}
  </instances>
</module>
'''
os.makedirs(OUT_DIR, exist_ok=True)
fzz = os.path.join(OUT_DIR, NAME + '.fzz')
with zipfile.ZipFile(fzz, 'w', zipfile.ZIP_DEFLATED) as z:
    z.writestr(NAME + '.fz', fz)
    for f in sorted(os.listdir(FZPZ_DIR)):
        z.write(os.path.join(FZPZ_DIR, f), f)
print('wrote', fzz, os.path.getsize(fzz), 'bytes')
