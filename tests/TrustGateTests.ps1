param([string]$ProjectDir=(Split-Path $PSScriptRoot -Parent))
$ErrorActionPreference='Stop'
$version=[regex]::Match((Get-Content -LiteralPath "$ProjectDir/CMakeLists.txt" -Raw),'project\(Cracked808s VERSION ([0-9.]+)\)').Groups[1].Value
try {
    & "$ProjectDir/scripts/Verify-ReleaseTrust.ps1" -PayloadRoot "$ProjectDir/Release-$version" -InstallerFile "$ProjectDir/Cracked808s-$version-Windows-x64-Setup.exe" -UninstallerFile "$ProjectDir/Cracked808s-$version-Windows-x64-Setup.exe" -ExpectedSignerThumbprint ('0'*40)
    throw 'Unsigned release was accepted as trusted.'
} catch {
    if($_.Exception.Message -notmatch 'NotSigned|not trusted'){throw}
    'PASS: an unsigned installer cannot pass the trusted-release gate.'
}
