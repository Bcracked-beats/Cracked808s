param([Parameter(Mandatory)][string]$Makensis,[Parameter(Mandatory)][string]$BuildDir)
$ErrorActionPreference='Stop'
$projectDir=Split-Path $PSScriptRoot -Parent
$version=[regex]::Match((Get-Content -LiteralPath "$projectDir/CMakeLists.txt" -Raw),'project\(Cracked808s VERSION ([0-9.]+)\)').Groups[1].Value
$qaRoot=Join-Path ([System.IO.Path]::GetTempPath()) ('Cracked808s-Installer-QA-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $qaRoot | Out-Null
$setup=Join-Path $qaRoot 'Setup-QA.exe'
& $Makensis "/DQA_ROOT=$qaRoot" "/DRELEASE_DIR=$projectDir/Release-$version" "/DSETUP_FILE=$setup" "$projectDir/installer/Cracked808s.nsi"
if($LASTEXITCODE -ne 0){throw 'QA installer compile failed.'}
$dll=Join-Path $qaRoot 'VST3/Cracked808s.vst3/Contents/x86_64-win/Cracked808s.vst3'
$app=Join-Path $qaRoot 'Application'
$expected=(Get-FileHash -LiteralPath "$projectDir/Release-$version/VST3/Cracked808s.vst3/Contents/x86_64-win/Cracked808s.vst3").Hash
function Install-QA { $process=Start-Process -FilePath $setup -ArgumentList '/S' -PassThru -Wait -WindowStyle Hidden;if($process.ExitCode -ne 0){throw "QA install exit $($process.ExitCode)"};if((Get-FileHash -LiteralPath $dll).Hash -ne $expected){throw 'Installed binary hash differs from release.'} }
Install-QA
foreach($helper in @("$qaRoot/VST3/Cracked808s.vst3/Contents/x86_64-win/Cracked808sUpdate.exe","$app/Cracked808sUpdate.exe")){if(!(Test-Path -LiteralPath $helper)){throw 'Bundled update helper missing from install.'}}
if(!(Test-Path -LiteralPath "$app/Licenses/SpaceGrotesk-OFL.txt") -or !(Test-Path -LiteralPath "$app/Licenses/JUCE-LICENSE.md")){throw 'Binary installer must include full font/framework notices.'}
if(@(Get-ChildItem "$app/Licenses/JUCE-modules" -File -Recurse).Count -lt 5){throw 'JUCE dependency notices must accompany the binary.'}
if(!(Test-Path -LiteralPath "$app/Cracked808s.exe")){throw 'Standalone component missing.'}
& "$BuildDir/Cracked808sBundleProbe.exe" $dll '{ABCDEF01-9182-FAEB-4263-726B436B3873}'
if($LASTEXITCODE -ne 0){throw 'Installed VST3 failed to load with original identity.'}
$sentinel=Join-Path $qaRoot 'VST3/Cracked808s.vst3/leave-my-file.txt'
[System.IO.File]::WriteAllText($sentinel,'unrelated file must survive')
[System.IO.File]::WriteAllText("$app/Licenses/Leave.txt",'user addition must survive')
New-Item -ItemType Directory -Path "$qaRoot/UserPresets" | Out-Null
[System.IO.File]::WriteAllText("$qaRoot/UserPresets/Keep.cracked808s",'user preset must survive')
# Simulate an older payload, then verify the normal file-replacement path.
[System.IO.File]::WriteAllText($dll,'old disposable QA payload')
$oldHash=(Get-FileHash -LiteralPath $dll).Hash
$locked=[System.IO.File]::Open("$app/Cracked808s.exe",[System.IO.FileMode]::Open,[System.IO.FileAccess]::Read,[System.IO.FileShare]::Read)
try{$blocked=Start-Process -FilePath $setup -ArgumentList '/S' -PassThru -Wait -WindowStyle Hidden;if($blocked.ExitCode -eq 0 -or (Get-FileHash -LiteralPath $dll).Hash -ne $oldHash){throw 'Locked upgrade must abort before replacing any files.'}}finally{$locked.Dispose()}
Install-QA
$locked=[System.IO.File]::Open("$app/Cracked808s.exe",[System.IO.FileMode]::Open,[System.IO.FileAccess]::Read,[System.IO.FileShare]::Read)
try{
  $blocked=Start-Process -FilePath "$app/Uninstall.exe" -ArgumentList '/S',"_?=$app" -PassThru -Wait -WindowStyle Hidden
  if($blocked.ExitCode -eq 0 -or !(Test-Path -LiteralPath $dll) -or !(Test-Path 'HKCU:/Software/Microsoft/Windows/CurrentVersion/Uninstall/Cracked808s-Installer-QA')){throw 'Locked standalone must stop uninstall before deleting plugin files or registration.'}
}finally{$locked.Dispose()}
$uninstaller=Start-Process -FilePath "$app/Uninstall.exe" -ArgumentList '/S',"_?=$app" -PassThru -Wait -WindowStyle Hidden
if($uninstaller.ExitCode -ne 0){throw 'QA uninstall failed.'}
if((Test-Path -LiteralPath $dll) -or (Test-Path -LiteralPath "$app/Cracked808s.exe") -or (Test-Path -LiteralPath "$app/Cracked808sUpdate.exe") -or (Test-Path -LiteralPath "$qaRoot/VST3/Cracked808s.vst3/Contents/x86_64-win/Cracked808sUpdate.exe")){throw 'Uninstall left a managed binary behind.'}
if(!(Test-Path -LiteralPath $sentinel) -or [System.IO.File]::ReadAllText("$qaRoot/UserPresets/Keep.cracked808s") -ne 'user preset must survive'){throw 'Uninstall removed unrelated files or presets.'}
if(!(Test-Path -LiteralPath "$app/Licenses/Leave.txt")){throw 'Uninstall removed an unrelated addition in the license directory.'}
if(Test-Path 'HKCU:/Software/Microsoft/Windows/CurrentVersion/Uninstall/Cracked808s-Installer-QA'){throw 'QA uninstall registration remains.'}
Write-Output "PASS: install, VST3 identity/load, repair/upgrade, uninstall, unrelated-file and preset preservation. QA root: $qaRoot"
