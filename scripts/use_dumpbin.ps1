Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Add-ToPath([string]$dir){
  if([string]::IsNullOrWhiteSpace($dir)){ return }
  if(-not (Test-Path -LiteralPath $dir)){ return }
  if(($Env:PATH -split ';') -contains $dir){ return }
  $Env:PATH = $dir + ';' + $Env:PATH
}

# If VS dev cmd already initialized, dumpbin should be present
if(Get-Command dumpbin.exe -ErrorAction SilentlyContinue){
  Write-Host "dumpbin already available: $(Get-Command dumpbin.exe).Source"
  return
}

# Try common VS 2022 locations
$roots = @(
  'C:\Program Files\Microsoft Visual Studio\2022\Community',
  'C:\Program Files\Microsoft Visual Studio\2022\Enterprise',
  'C:\Program Files\Microsoft Visual Studio\2022\Professional',
  'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools'
)

$dumpbin = $null
foreach($r in $roots){
  if(-not (Test-Path -LiteralPath $r)){ continue }
  $c = Get-ChildItem -LiteralPath $r -Recurse -Filter dumpbin.exe -ErrorAction SilentlyContinue | Select-Object -First 1
  if($c){ $dumpbin = $c.FullName; break }
}

if($dumpbin){
  Add-ToPath (Split-Path -Parent $dumpbin)
  Write-Host "Configured PATH for dumpbin: $dumpbin"
} else {
  Write-Error "dumpbin.exe not found. Launch the VS Developer Command Prompt (vcvars64.bat) or install MSVC tools."
}

