param([Parameter(Mandatory)][string[]]$Binaries,[string]$Dumpbin='dumpbin.exe')
$ErrorActionPreference='Stop'
foreach($binary in $Binaries){
  $dependencies=& $Dumpbin /dependents $binary
  if($LASTEXITCODE -ne 0){throw "Cannot inspect $binary"}
  if($dependencies -match '(?i)\b(?:MSVCP\d+|VCRUNTIME\d+|MSVCR\d+)[\w_]*\.dll\b'){throw "External Visual C++ runtime dependency in $binary"}
  Write-Output "PASS: $binary has no external Visual C++ runtime prerequisite."
}
