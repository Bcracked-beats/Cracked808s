param(
    [Parameter(Mandatory)][string]$PayloadRoot,
    [Parameter(Mandatory)][string]$InstallerFile,
    [Parameter(Mandatory)][string]$UninstallerFile,
    [Parameter(Mandatory)][ValidatePattern('^[A-Fa-f0-9]{40}$')][string]$ExpectedSignerThumbprint
)
$ErrorActionPreference='Stop'
$requiredFiles=@($InstallerFile,$UninstallerFile,
    (Join-Path $PayloadRoot 'VST3/Cracked808s.vst3/Contents/x86_64-win/Cracked808s.vst3'),
    (Join-Path $PayloadRoot 'VST3/Cracked808s.vst3/Contents/x86_64-win/Cracked808sUpdate.exe'),
    (Join-Path $PayloadRoot 'Standalone/Cracked808s.exe'),
    (Join-Path $PayloadRoot 'Standalone/Cracked808sUpdate.exe'))
foreach($requiredFile in $requiredFiles){
    if(!(Test-Path -LiteralPath $requiredFile -PathType Leaf)){throw "Required signed payload missing: $requiredFile"}
    $signature=Get-AuthenticodeSignature -LiteralPath $requiredFile
    if($signature.Status -ne 'Valid'){throw "Release is not trusted: $($signature.Status) -- $requiredFile"}
    if($signature.SignatureType -ne 'Authenticode'){throw "Embedded Authenticode signature required: $requiredFile"}
    if($signature.SignerCertificate.Thumbprint -ne $ExpectedSignerThumbprint){throw "Unexpected signer: $requiredFile"}
    if(!$signature.TimeStamperCertificate){throw "Trusted timestamp missing: $requiredFile"}
    $extension=$signature.SignerCertificate.Extensions | Where-Object {$_.Oid.Value -eq '2.5.29.37'}
    $eku=@($extension.EnhancedKeyUsages | ForEach-Object {$_.Value})
    if('1.3.6.1.5.5.7.3.3' -notin $eku){throw "Certificate is not authorized for code signing: $requiredFile"}
}
if([IO.Path]::GetFullPath($InstallerFile) -eq [IO.Path]::GetFullPath($UninstallerFile)){throw 'The real extracted uninstaller must be verified separately from the installer.'}
'PASS: every managed executable, plugin and installer has the expected trusted signature and timestamp. Compute release checksums now; do not modify these files afterward.'
