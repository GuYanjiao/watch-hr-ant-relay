"""Check application-only UF2 blocks before copying to a Pro Micro boot drive."""
import pathlib
import struct
import sys

path = pathlib.Path(sys.argv[1])
image = path.read_bytes()
if not image or len(image) % 512:
    raise SystemExit('Invalid UF2 length')
count = len(image) // 512
addresses = []
for index in range(count):
    block = image[index * 512:(index + 1) * 512]
    magic0, magic1, flags, address, size, number, total, family = struct.unpack_from('<8I', block)
    if (magic0, magic1, struct.unpack_from('<I', block, 508)[0]) != (0x0A324655, 0x9E5D5157, 0x0AB16F30):
        raise SystemExit(f'Invalid UF2 magic in block {index}')
    if flags != 0x2000 or family != 0xADA52840:
        raise SystemExit('Expected normal nRF52840 family-tagged flash blocks')
    if number != index or total != count or not 0 < size <= 476:
        raise SystemExit('Invalid block numbering or payload size')
    if not 0x26000 <= address < address + size <= 0xEC000:
        raise SystemExit(f'Block outside application partition: 0x{address:x}')
    if addresses and address < addresses[-1][1]:
        raise SystemExit('Overlapping or unsorted UF2 blocks')
    addresses.append((address, address + size))
if addresses[0][0] != 0x26000:
    raise SystemExit('Incorrect application start address')
stack, reset = struct.unpack_from('<2I', image, 32)
if not 0x20000000 < stack <= 0x20040000 or stack % 8:
    raise SystemExit(f'Invalid initial stack pointer: 0x{stack:x}')
if not reset & 1 or not addresses[0][0] <= (reset & ~1) < addresses[-1][1]:
    raise SystemExit(f'Reset handler is not linked inside the application: 0x{reset:x}')
print(f'PASS: {count} UF2 blocks, application only: 0x{addresses[0][0]:x}..0x{addresses[-1][1]:x}')
print(f'PASS: stack 0x{stack:x}, reset handler 0x{reset:x}')
