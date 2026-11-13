param([string]$ProjectDir=(Split-Path $PSScriptRoot -Parent))
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$version=[regex]::Match((Get-Content -LiteralPath "$ProjectDir/CMakeLists.txt" -Raw),'project\(Cracked808s VERSION ([0-9.]+)\)').Groups[1].Value
$source=[IO.Compression.ZipFile]::OpenRead("$ProjectDir/Cracked808s-$version-source.zip")
$binary=[IO.Compression.ZipFile]::OpenRead("$ProjectDir/Cracked808s-$version-Windows-x64.zip")
try{
  foreach($name in @('CMakeLists.txt','src/DSP/AudioEngine.h','src/DSP/QualityProcessor.h','src/PluginEditor.cpp','src/PluginEditor.h','src/UI/SpaceTheme.h','src/Updates/UpdateClient.h','src/Updates/UpdateHelper.cpp','src/State/ParameterLayout.h','tests/BundleProbe.cpp','installer/Cracked808s.nsi','scripts/Build-Release.ps1','CODE_SIGNING.md','SECURITY.md','assets/SpaceGrotesk-OFL.txt')){
    $entry=$source.GetEntry($name);if(!$entry){throw "Missing source: $name"};$stream=$entry.Open();try{$hash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()};if($hash -ne (Get-FileHash -LiteralPath "$ProjectDir/$name").Hash){throw "Source archive differs: $name"}
  }
  foreach($name in @('third_party/JUCE/CMakeLists.txt','third_party/JUCE/LICENSE.md','third_party/JUCE/extras/Build/juceaide/Main.cpp')){if(!$source.GetEntry($name)){throw "Missing JUCE build source: $name"}}
  foreach($name in @('VST3/Cracked808s.vst3/Contents/Resources/moduleinfo.json','VST3/Cracked808s.vst3/Contents/x86_64-win/Cracked808s.vst3','VST3/Cracked808s.vst3/Contents/x86_64-win/Cracked808sUpdate.exe','Standalone/Cracked808s.exe','Standalone/Cracked808sUpdate.exe','CODE_SIGNING.md','SECURITY.md','Licenses/SpaceGrotesk-OFL.txt','Licenses/JUCE-LICENSE.md')){if(!$binary.GetEntry($name)){throw "Missing binary payload/notice: $name"}}
  if(@($binary.Entries | Where-Object FullName -Like 'Licenses/JUCE-modules/*').Count -lt 5){throw 'Binary dependency notices missing.'}
}finally{$source.Dispose();$binary.Dispose()}
Write-Output 'PASS: corresponding source matches current implementation; JUCE build files, complete bundles and notices are packaged.'
