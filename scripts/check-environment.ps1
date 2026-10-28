$ErrorActionPreference = 'Stop'
git --version
& 'C:\ncs\bin\nrfutil.exe' --version
& 'C:\ncs\bin\nrfutil.exe' list
& 'C:\ncs\bin\nrfutil.exe' sdk-manager list
if ($LASTEXITCODE -ne 0) { throw 'SDK environment check failed' }
