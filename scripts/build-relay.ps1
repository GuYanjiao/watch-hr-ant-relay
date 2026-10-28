$ErrorActionPreference = 'Stop'
$source = Join-Path $PSScriptRoot '..\firmware'
# Keep the toolchain source/build paths ASCII; repository remains in the requested folder.
$staged = 'C:\ncs\apps\watch-hr-ant-relay'
New-Item -ItemType Directory -Force $staged | Out-Null
& robocopy $source $staged /E /NJH /NJS /NFL /NDL /NP
if ($LASTEXITCODE -ge 8) { throw 'Cannot stage firmware sources' }
& 'C:\ncs\bin\nrfutil.exe' sdk-manager toolchain launch --ncs-version v3.2.4 --install-dir C:\ncs --chdir C:\ncs\v3.2.4 -- west build -b nrf52840dk/nrf52840 C:\ncs\apps\watch-hr-ant-relay -d C:\ncs\build\watch-hr-relay -- '-DZEPHYR_EXTRA_MODULES=C:\ncs\addons\sdk-ant-v2.1.1'
if ($LASTEXITCODE -ne 0) { throw 'Relay firmware build failed' }
