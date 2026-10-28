$ErrorActionPreference = 'Stop'
$firmware = 'C:\ncs\build\watch-hr-promicro\watch-hr-ant-relay\zephyr\zephyr.uf2'
& 'C:\ncs\toolchains\fd21892d0f\opt\bin\python.exe' (Join-Path $PSScriptRoot 'verify-uf2.py') $firmware
if ($LASTEXITCODE -ne 0) { throw 'UF2 validation failed' }
$disks = @(Get-CimInstance Win32_LogicalDisk | Where-Object { $_.DriveType -eq 2 -and $_.VolumeName -eq 'NICENANO' })
if ($disks.Count -ne 1) { throw 'Expected exactly one NICENANO boot disk' }
$target = $disks[0].DeviceID + '\\'
if ((Get-Content (Join-Path $target 'INFO_UF2.TXT') -Raw) -notmatch 'nRF52840-nicenano') { throw 'Unexpected board ID' }
Copy-Item -LiteralPath $firmware -Destination (Join-Path $target 'relay.uf2') -ErrorAction Stop
