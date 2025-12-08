param(
  [string]$BinaryPath,
  [Parameter(Mandatory=$true)] [string]$GhidraHome,
  [string]$OutDir = (Join-Path (Get-Location) 'ghidra\exports\NIMax'),
  [switch]$Single,
  [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function ThrowIfMissing([string]$Path){ if(-not (Test-Path -LiteralPath $Path)){ throw "Path not found: $Path" } }

if([string]::IsNullOrWhiteSpace($BinaryPath)){
  $pf    = ${Env:ProgramFiles}
  $pf86  = ${Env:ProgramFiles(x86)}
  $candidates = @()
  if($pf86){ $candidates += (Join-Path $pf86 'National Instruments\MAX\NIMax.exe') }
  if($pf){  $candidates += (Join-Path $pf  'National Instruments\MAX\NIMax.exe') }
  foreach($p in $candidates){ if(Test-Path -LiteralPath $p){ $BinaryPath = $p; break } }
}

Write-Host "Binary     : $BinaryPath"
Write-Host "GhidraHome : $GhidraHome"
Write-Host "OutDir     : $OutDir"

ThrowIfMissing $BinaryPath
ThrowIfMissing $GhidraHome

$runner = Join-Path (Get-Location) 'scripts\ghidra_run_headless.ps1'
ThrowIfMissing $runner

if($Single){
  $outFile = Join-Path (Split-Path -Parent $OutDir) 'NIMax_decompiled.c'
  Write-Host "Single file: $outFile"
  & $runner -GhidraHome $GhidraHome -BinaryPath $BinaryPath -OutFile $outFile -Clean:$Clean
} else {
  & $runner -GhidraHome $GhidraHome -BinaryPath $BinaryPath -OutDir $OutDir -Clean:$Clean
}
