"""Read relay USB logs; waits for re-enumeration after a UF2 upload."""
import argparse
import pathlib
import time
import serial
from serial.tools import list_ports

parser = argparse.ArgumentParser()
parser.add_argument('--seconds', type=int, default=30)
args = parser.parse_args()
port = None
for _ in range(60):
    matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x2fe3, 0x0004)]
    if matches:
        port = matches[0].device
        break
    time.sleep(0.5)
if not port:
    raise SystemExit('Relay USB serial port not found within 30 seconds')
print(f'Relay USB serial: {port}', flush=True)
data = bytearray()
with serial.Serial(port, 115200, timeout=0.2) as stream:
    stream.dtr = True
    end = time.monotonic() + args.seconds
    while time.monotonic() < end:
        chunk = stream.read(4096)
        if chunk:
            data.extend(chunk)
            print(chunk.decode('utf-8', errors='replace'), end='', flush=True)
out = pathlib.Path(__file__).resolve().parents[1] / 'artifacts' / 'relay-serial.log'
out.parent.mkdir(exist_ok=True)
out.write_bytes(data)
print(f'\nCaptured {len(data)} bytes: {out}')
