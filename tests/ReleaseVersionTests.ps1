param([Parameter(Mandatory)][string]$StaleBuildDir,[Parameter(Mandatory)][string]$Makensis)
$ErrorActionPreference='Stop'
$fixture=Join-Path ([System.IO.Path]::GetTempPath()) ('Cracked808s-Version-QA-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path "$fixture/scripts" -Force | Out-Null
$script=Join-Path $fixture 'scripts/Build-Release.ps1'
Copy-Item -LiteralPath (Join-Path (Split-Path $PSScriptRoot -Parent) 'scripts/Build-Release.ps1') -Destination $script
try { & $script -BuildDir (Resolve-Path -LiteralPath $StaleBuildDir).Path -Makensis $Makensis; throw 'Packaging unexpectedly accepted stale binaries.' }
catch { if($_.Exception.Message -notmatch 'version mismatch'){ throw }; Write-Output 'PASS: stale payload rejected before staging.' }
if((Test-Path -LiteralPath "$fixture/Release-2.5.3") -or (Test-Path -LiteralPath "$fixture/Source-2.5.3")){throw 'Version rejection must happen before staging.'}
