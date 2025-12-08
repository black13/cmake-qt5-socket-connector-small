param(
  [Parameter(Mandatory=$true)] [string]$GhidraHome,
  [string]$BinaryPath,
  [string]$Dll = 'nivisa64.dll',
  [string]$Regex = '(?i)^vi[A-Za-z0-9_]+$',
  [string]$Report,
  [string]$ProjectName = 'NIResearch',
  [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function ThrowIfMissing([string]$Path){ if(-not (Test-Path -LiteralPath $Path)){ throw "Path not found: $Path" } }

function Get-ShortPath([string]$Path){
  $code = @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class ShortPathHelper{
  [DllImport("kernel32.dll", CharSet=CharSet.Auto, SetLastError=true)]
  static extern int GetShortPathName(string lfn, StringBuilder sfn, int len);
  public static string ToShort(string p){
    var sb = new StringBuilder(32768);
    int r = GetShortPathName(p, sb, sb.Capacity);
    return r>0 ? sb.ToString() : p;
  }
}
'@
  Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue | Out-Null
  $rp = (Resolve-Path -LiteralPath $Path).Path
  return [ShortPathHelper]::ToShort($rp)
}

ThrowIfMissing $GhidraHome

$ah = Join-Path $GhidraHome 'support\analyzeHeadless.bat'
ThrowIfMissing $ah

$scriptDir = (Join-Path (Get-Location) 'scripts')
$scriptName = 'ghidra_find_visa_usage.java'
$scriptPath = (Join-Path $scriptDir $scriptName)
if(-not (Test-Path -LiteralPath $scriptPath)){
  throw "Script not found: $scriptPath"
}

$foundCandidates = @()
if([string]::IsNullOrWhiteSpace($BinaryPath) -or -not (Test-Path -LiteralPath $BinaryPath)){
  $pf    = ${Env:ProgramFiles}
  $pf86  = ${Env:ProgramFiles(x86)}
  if($pf86){ $foundCandidates += (Join-Path $pf86 'National Instruments\MAX\NIMax.exe') }
  if($pf){  $foundCandidates += (Join-Path $pf  'National Instruments\MAX\NIMax.exe') }
  foreach($p in $foundCandidates){ if(Test-Path -LiteralPath $p){ $BinaryPath = $p; break } }
}
if(-not (Test-Path -LiteralPath $BinaryPath)){
  $c = ($foundCandidates -join ', ')
  throw "Could not find NIMax.exe. Provide -BinaryPath explicitly. Tried: $c"
}

$projRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('ghidra_proj_' + [System.Guid]::NewGuid().ToString('N'))
if($Clean){ Remove-Item -Recurse -Force $projRoot -ErrorAction SilentlyContinue }
New-Item -ItemType Directory -Force -Path $projRoot | Out-Null

$argsForScript = @(
  ('dll=' + $Dll)
)
# Only forward -Regex if explicitly provided by the user; default is handled in Java
if($PSBoundParameters.ContainsKey('Regex') -and -not [string]::IsNullOrWhiteSpace($Regex)){
  $argsForScript += ('regex=' + $Regex)
}
if($Report){
  # Allow a new file path; ensure parent exists but do not Resolve-Path the file itself
  try {
    $fullReport = [System.IO.Path]::GetFullPath($Report)
  } catch {
    throw "Invalid -Report path: $Report"
  }
  $parent = Split-Path -Parent $fullReport
  if($parent){ New-Item -ItemType Directory -Force -Path $parent | Out-Null }
  $argsForScript += ('report=' + $fullReport)
}

Write-Host "BinaryPath : $BinaryPath" -ForegroundColor Yellow
$binShort = Get-ShortPath $BinaryPath

Write-Host "Running analyzeHeadless for VISA usage..." -ForegroundColor Cyan

$cmd = @(
  $projRoot,
  $ProjectName,
  '-import', $binShort,
  '-overwrite',
  '-scriptPath', $scriptDir,
  '-postScript', $scriptName
)

$quotedArgs = @()
foreach($a in $argsForScript){ $quotedArgs += '"' + $a + '"' }
$cmd += $quotedArgs

& $ah @cmd
if($LASTEXITCODE -ne 0){ throw "analyzeHeadless exited with code $LASTEXITCODE" }

Write-Host "Headless VISA usage analysis complete." -ForegroundColor Green

# If NIMaxImp.dll is present beside the EXE, run analysis on it as well and append to report
try {
  $nimaxImp = Join-Path (Split-Path -Parent $BinaryPath) 'NIMaxImp.dll'
  if(Test-Path -LiteralPath $nimaxImp){
    Write-Host "Running analysis for NIMaxImp.dll..." -ForegroundColor Cyan
    $projRoot2 = Join-Path ([System.IO.Path]::GetTempPath()) ('ghidra_proj_' + [System.Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Force -Path $projRoot2 | Out-Null

    $cmd2 = @(
      $projRoot2,
      $ProjectName,
      '-import', (Get-ShortPath $nimaxImp),
      '-overwrite',
      '-scriptPath', $scriptDir,
      '-postScript', $scriptName
    )

    $args2 = @('dll=' + $Dll)
    if($Report){ $args2 += ('report=' + $fullReport); $args2 += 'append=true' }
    foreach($a in $args2){ $cmd2 += ('"' + $a + '"') }

    & $ah @cmd2
    if($LASTEXITCODE -ne 0){ throw "analyzeHeadless (NIMaxImp.dll) exited with code $LASTEXITCODE" }
    Write-Host "NIMaxImp.dll analysis complete." -ForegroundColor Green
  }
} catch {
  Write-Warning $_
}
