$ErrorActionPreference = 'Stop'
$source = Join-Path $PSScriptRoot '..\firmware'
# Keep the toolchain source/build paths ASCII; repository remains in the requested folder.
$staged = 'C:\ncs\apps\watch-hr-ant-relay'
New-Item -ItemType Directory -Force $staged | Out-Null
Copy-Item (Join-Path $PSScriptRoot '..\config\promicro-pm-static.yml') C:\ncs\build\watch-hr-promicro-pm.yml -Force
& robocopy $source $staged /E /NJH /NJS /NFL /NDL /NP
if ($LASTEXITCODE -ge 8) { throw 'Cannot stage firmware sources' }
& 'C:\ncs\bin\nrfutil.exe' sdk-manager toolchain launch --ncs-version v3.2.4 --install-dir C:\ncs --chdir C:\ncs\v3.2.4 -- west build -b promicro_nrf52840/nrf52840/uf2 C:\ncs\apps\watch-hr-ant-relay -d C:\ncs\build\watch-hr-promicro -- '-DZEPHYR_EXTRA_MODULES=C:\ncs\addons\sdk-ant-v2.1.1' '-DPM_STATIC_YML_FILE=C:\ncs\build\watch-hr-promicro-pm.yml'
if ($LASTEXITCODE -ne 0) { throw 'Relay firmware build failed' }
& 'C:\ncs\toolchains\fd21892d0f\opt\bin\python.exe' (Join-Path $PSScriptRoot 'verify-uf2.py') 'C:\ncs\build\watch-hr-promicro\watch-hr-ant-relay\zephyr\zephyr.uf2'
if ($LASTEXITCODE -ne 0) { throw 'UF2 address validation failed' }
