$ErrorActionPreference = 'Stop'
# This compiles an SDK sample for a reference board; it does not flash hardware.
& 'C:\ncs\bin\nrfutil.exe' sdk-manager toolchain launch --ncs-version v3.2.4 --install-dir C:\ncs -- west build -b nrf52840dk/nrf52840 C:\ncs\v3.2.4\zephyr\samples\hello_world -d C:\ncs\build\watch-hr-smoke
if ($LASTEXITCODE -ne 0) { throw 'Reference-board smoke build failed' }
