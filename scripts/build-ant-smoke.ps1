$ErrorActionPreference = 'Stop'
$relayRoot = Split-Path $PSScriptRoot -Parent
$relayConf = Join-Path $relayRoot 'config\ant-evaluation.conf'
# Keep build paths ASCII for the SDK; all project sources remain in the user folder.
New-Item -ItemType Directory -Force C:\ncs\build | Out-Null
Copy-Item -LiteralPath $relayConf -Destination C:\ncs\build\watch-hr-evaluation.conf -Force
& 'C:\ncs\bin\nrfutil.exe' sdk-manager toolchain launch --ncs-version v3.2.4 --install-dir C:\ncs --chdir C:\ncs\v3.2.4 -- west build -b nrf52840dk/nrf52840 C:\ncs\addons\sdk-ant-v2.1.1\samples\ant_plus\ant_hrm\hrm_tx -d C:\ncs\build\watch-hr-ant-smoke -- '-DZEPHYR_EXTRA_MODULES=C:\ncs\addons\sdk-ant-v2.1.1' '-DEXTRA_CONF_FILE=C:\ncs\build\watch-hr-evaluation.conf'
if ($LASTEXITCODE -ne 0) { throw 'ANT reference-board smoke build failed' }
