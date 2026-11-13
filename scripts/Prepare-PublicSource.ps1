param([Parameter(Mandatory)][string]$Destination)
$ErrorActionPreference='Stop'
$project=(Resolve-Path -LiteralPath (Split-Path $PSScriptRoot -Parent)).Path
$output=[IO.Path]::GetFullPath($Destination)
if($output.StartsWith($project+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or $output -eq $project){throw 'Export must be outside the source tree.'}
if(Test-Path -LiteralPath $output){throw 'Use a new empty export destination; existing files will not be overwritten.'}
New-Item -ItemType Directory -Path $output | Out-Null
$files=@('CMakeLists.txt','README.md','SOURCE-README.md','RELEASE-README.md','LICENSE.txt','NOTICE.md','THIRD-PARTY-LICENSES.md','CHANGELOG.md','CODE_SIGNING.md','SECURITY.md','.gitignore','docs/UPDATES-2.5.4.md','docs/INPLACE-UI-2.5.3.md')
$folders=@('src','tests','assets','installer','scripts','third_party/JUCE/modules','third_party/JUCE/extras/Build')
foreach($folder in $folders){foreach($file in Get-ChildItem -LiteralPath (Join-Path $project $folder) -Recurse -File -Force){
    $relative=[IO.Path]::GetRelativePath($project,$file.FullName)
    if($relative -match '(?i)(^|[\\/])(\.git|\.env[^\\/]*|\.dev\.vars[^\\/]*|\.codex|\.wrangler|node_modules)([\\/]|$)' -or $file.Name -match '(?i)\.(exe|dll|pdb|obj|zip|pfx|p12|key)$'){continue}
    $files+=$relative
}}
foreach($name in @('CMakeLists.txt','LICENSE.md','README.md','CHANGE_LIST.md','BREAKING_CHANGES.md')){$files+='third_party/JUCE/'+$name}
foreach($relative in $files){$target=Join-Path $output $relative;New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null;Copy-Item -LiteralPath (Join-Path $project $relative) -Destination $target}
Copy-Item -LiteralPath (Join-Path $project 'repository/README.md') -Destination (Join-Path $output 'README.md') -Force
New-Item -ItemType Directory -Path (Join-Path $output '.github/workflows') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $project 'repository/windows-ci.yml') -Destination (Join-Path $output '.github/workflows/windows-ci.yml')
"Prepared source-only repository: $output ($($files.Count) files). No Git history, binaries, account credentials, website database or signing keys are included. Publish under the owner's account only; code signing is not yet active."
