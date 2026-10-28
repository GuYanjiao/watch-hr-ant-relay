"""Execute the actual ARM C parser and test vectors using Unicorn (no board)."""
import pathlib
import subprocess
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_R0

root = pathlib.Path(__file__).resolve().parents[1]
bin_dir = pathlib.Path(r'C:\ncs\toolchains\fd21892d0f\opt\zephyr-sdk\arm-zephyr-eabi\bin')
build = pathlib.Path(r'C:\ncs\build\watch-hr-parser-test')
build.mkdir(parents=True, exist_ok=True)
elf = build / 'parser.elf'
subprocess.run([str(bin_dir / 'arm-zephyr-eabi-gcc.exe'), '-O2', '-mcpu=cortex-m4',
                '-mthumb', '-ffreestanding', '-fno-builtin', '-nostdlib',
                '-Wl,-Ttext=0x10000', '-Wl,-e,run_tests', '-I' + str(root / 'firmware/src'),
                str(root / 'firmware/src/hr_measurement.c'),
                str(root / 'tests/hr_measurement_test.c'),
                str(root / 'firmware/src/activity.c'), str(root / 'tests/activity_test.c'), '-o', str(elf)], check=True)
subprocess.run([str(bin_dir / 'arm-zephyr-eabi-objcopy.exe'), '-O', 'binary',
                str(elf), str(build / 'parser.bin')], check=True)
symbols = subprocess.check_output([str(bin_dir / 'arm-zephyr-eabi-nm.exe'), str(elf)], text=True)
entry = next(int(row.split()[0], 16) for row in symbols.splitlines() if row.endswith(' T run_tests'))
emu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
emu.mem_map(0x10000, 0x10000)
emu.mem_write(0x10000, (build / 'parser.bin').read_bytes())
emu.mem_map(0x20000000, 0x10000)
emu.reg_write(UC_ARM_REG_SP, 0x2000f000)
emu.reg_write(UC_ARM_REG_LR, 0x1f001)
emu.emu_start(entry | 1, 0x1f000, count=100000)
result = emu.reg_read(UC_ARM_REG_R0)
if result:
    source, line = ('activity_test.c', result - 1000) if result >= 1000 else ('hr_measurement_test.c', result)
    raise AssertionError(f'C firmware test failed at {source}:{line}')
from unicorn.arm_const import UC_ARM_REG_PC
assert emu.reg_read(UC_ARM_REG_PC) == 0x1f000, 'Test did not return before instruction limit'
print('PASS: ARM C parser: 8/16-bit BPM, energy, RR, contact, zero, limits, truncated/malformed packets')

print('PASS: ARM C activity policy: search deadline, cooldown, HR loss, coasting, inactivity; cadence layouts and rollover')
