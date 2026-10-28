"""Request UF2 mode from the running relay; does not erase or flash anything."""
import time
import serial
from serial.tools import list_ports
matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x2fe3, 0x0004)]
if len(matches) != 1:
    raise SystemExit('Expected exactly one running relay USB device')
with serial.Serial(matches[0].device, 115200, timeout=0.5, write_timeout=1) as stream:
    stream.dtr = True
    time.sleep(0.2)
    stream.write(b'B')
    time.sleep(0.3)
print('Requested UF2 bootloader mode')
