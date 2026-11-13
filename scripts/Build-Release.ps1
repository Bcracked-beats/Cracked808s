param([Parameter(Mandatory)][string]$BuildDir,[Parameter(Mandatory)][string]$Makensis)
$ErrorActionPreference='Stop'
$projectDir=Split-Path $PSScriptRoot -Parent
$version='2.5.4'
$releaseDir=Join-Path $projectDir "Release-$version"
$sourceDir=Join-Path $projectDir "Source-$version"
$artifacts=Join-Path $BuildDir 'Cracked808s_artefacts/Release'
$moduleInfo=Join-Path $artifacts 'VST3/Cracked808s.vst3/Contents/Resources/moduleinfo.json'
$standalone=Join-Path $artifacts 'Standalone/Cracked808s.exe'
if(!(Test-Path -LiteralPath $moduleInfo) -or !(Test-Path -LiteralPath $standalone)){throw 'Build the VST3 and Standalone targets before packaging.'}
$moduleVersion=(Get-Content -LiteralPath $moduleInfo -Raw | ConvertFrom-Json).Version
$appVersion=(Get-Item -LiteralPath $standalone).VersionInfo.ProductVersion
if($moduleVersion -ne $version -or $appVersion -notin @($version,"$version.0")){throw "Release version mismatch: expected $version, VST3=$moduleVersion, Standalone=$appVersion"}
foreach($helper in @("$artifacts/VST3/Cracked808s.vst3/Contents/x86_64-win/Cracked808sUpdate.exe","$artifacts/Standalone/Cracked808sUpdate.exe")){if(!(Test-Path -LiteralPath $helper)){throw 'The complete bundle must include the update helper.'}}
if((Test-Path -LiteralPath $releaseDir) -or (Test-Path -LiteralPath $sourceDir)){throw 'Use fresh staging directories; existing release/source directories will not be overwritten.'}
New-Item -ItemType Directory -Path $releaseDir,$sourceDir | Out-Null
Copy-Item -LiteralPath "$artifacts/VST3","$artifacts/Standalone" -Destination $releaseDir -Recurse
Copy-Item -LiteralPath "$projectDir/LICENSE.txt","$projectDir/NOTICE.md","$projectDir/THIRD-PARTY-LICENSES.md" -Destination $releaseDir
Copy-Item -LiteralPath "$projectDir/CODE_SIGNING.md","$projectDir/SECURITY.md" -Destination $releaseDir
Copy-Item -LiteralPath "$projectDir/RELEASE-README.md" -Destination "$releaseDir/README.md"
Copy-Item -LiteralPath "$projectDir/CHANGELOG.md" -Destination $releaseDir
New-Item -ItemType Directory -Path "$releaseDir/Licenses/JUCE-modules" -Force | Out-Null
Copy-Item -LiteralPath "$projectDir/assets/SpaceGrotesk-OFL.txt" -Destination "$releaseDir/Licenses"
Copy-Item -LiteralPath "$projectDir/third_party/JUCE/LICENSE.md" -Destination "$releaseDir/Licenses/JUCE-LICENSE.md"
$moduleDir=(Resolve-Path "$projectDir/third_party/JUCE/modules").Path
foreach($notice in Get-ChildItem -LiteralPath $moduleDir -Recurse -File | Where-Object Name -Match '^(?i:licen[cs]e|copying|copyright|notice)'){
  $relative=[System.IO.Path]::GetRelativePath($moduleDir,$notice.FullName)
  $destination=Join-Path "$releaseDir/Licenses/JUCE-modules" $relative
  New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
  Copy-Item -LiteralPath $notice.FullName -Destination $destination
}
# Generate exact uninstall entries; never recursively remove a directory with user additions.
$licenseFiles=Get-ChildItem -LiteralPath "$releaseDir/Licenses" -Recurse -File
$manifest=@('!macro RemoveLicenseFiles')
foreach($file in $licenseFiles){$relative=[System.IO.Path]::GetRelativePath($releaseDir,$file.FullName);if($relative -match '["$\r\n]'){throw 'Unsafe license filename'};$manifest+='  Delete "$INSTDIR\'+$relative+'"'}
$licenseDirs=Get-ChildItem -LiteralPath "$releaseDir/Licenses" -Recurse -Directory | Sort-Object {$_.FullName.Length} -Descending
foreach($directory in $licenseDirs){$manifest+='  RMDir "$INSTDIR\'+[System.IO.Path]::GetRelativePath($releaseDir,$directory.FullName)+'"'}
$manifest+=@('  RMDir "$INSTDIR\Licenses"','!macroend')
[System.IO.File]::WriteAllLines("$releaseDir/LicenseUninstall.nsh",$manifest)
& $Makensis "/DRELEASE_DIR=$releaseDir" "/DSETUP_FILE=$projectDir/Cracked808s-$version-Windows-x64-Setup.exe" "$projectDir/installer/Cracked808s.nsi"
if($LASTEXITCODE -ne 0){throw 'Installer compiler failed.'}
foreach($name in @('src','tests','assets','installer','scripts','repository')){Copy-Item -LiteralPath "$projectDir/$name" -Destination $sourceDir -Recurse}
foreach($name in @('CMakeLists.txt','LICENSE.txt','NOTICE.md','THIRD-PARTY-LICENSES.md','README.md','RELEASE-README.md','SOURCE-README.md','CHANGELOG.md','CODE_SIGNING.md','SECURITY.md','.gitignore')){Copy-Item -LiteralPath "$projectDir/$name" -Destination $sourceDir}
New-Item -ItemType Directory -Path "$sourceDir/docs","$sourceDir/third_party/JUCE/extras" -Force | Out-Null
Copy-Item -LiteralPath "$projectDir/docs/PAN-INSTALLER-2.5.md" -Destination "$sourceDir/docs"
Copy-Item -LiteralPath "$projectDir/docs/SHUTDOWN-FIX-2.5.1.md" -Destination "$sourceDir/docs"
Copy-Item -LiteralPath "$projectDir/docs/PERFORMANCE-POPOUT-2.5.2.md" -Destination "$sourceDir/docs"
Copy-Item -LiteralPath "$projectDir/docs/INPLACE-UI-2.5.3.md" -Destination "$sourceDir/docs"
Copy-Item -LiteralPath "$projectDir/docs/UPDATES-2.5.4.md" -Destination "$sourceDir/docs"
foreach($name in @('modules','CMakeLists.txt','LICENSE.md','README.md','CHANGE_LIST.md','BREAKING_CHANGES.md')){Copy-Item -LiteralPath "$projectDir/third_party/JUCE/$name" -Destination "$sourceDir/third_party/JUCE" -Recurse}
Copy-Item -LiteralPath "$projectDir/third_party/JUCE/extras/Build" -Destination "$sourceDir/third_party/JUCE/extras" -Recurse
Compress-Archive -Path "$releaseDir/*" -DestinationPath "$projectDir/Cracked808s-$version-Windows-x64.zip"
Compress-Archive -Path "$sourceDir/*" -DestinationPath "$projectDir/Cracked808s-$version-source.zip"
Get-FileHash -LiteralPath "$projectDir/Cracked808s-$version-Windows-x64-Setup.exe","$projectDir/Cracked808s-$version-Windows-x64.zip","$projectDir/Cracked808s-$version-source.zip" | Select-Object Hash,Path
Write-Output 'UNSIGNED CANDIDATE ONLY: do not publish this as a signed or warning-free release. Provider approval, signing, full signature verification and final post-signing checksums are still required.'
