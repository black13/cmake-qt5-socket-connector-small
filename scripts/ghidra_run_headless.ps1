param(
  [Parameter(Mandatory=$true)] [string]$GhidraHome,
  [Parameter(Mandatory=$true)] [string]$BinaryPath,
  [string]$OutDir,
  [string]$OutFile,
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

# Validate inputs
ThrowIfMissing $GhidraHome
ThrowIfMissing $BinaryPath

$ah = Join-Path $GhidraHome 'support\analyzeHeadless.bat'
ThrowIfMissing $ah

if([string]::IsNullOrWhiteSpace($OutDir) -and [string]::IsNullOrWhiteSpace($OutFile)){
  throw 'Provide either -OutDir or -OutFile'
}

# Determine project root
if([string]::IsNullOrWhiteSpace($OutDir)){
  # Derive from OutFile without resolving (file may not exist yet)
  $OutDir = Split-Path -Parent $OutFile
}
# Ensure base output directory exists
if(-not [string]::IsNullOrWhiteSpace($OutDir)){
  New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
}
$projRoot = Join-Path $OutDir '_gh_proj'
if($Clean){ Remove-Item -Recurse -Force $projRoot -ErrorAction SilentlyContinue }
New-Item -ItemType Directory -Force -Path $projRoot | Out-Null

# Build script path/name and args
$scriptDir = (Join-Path (Get-Location) 'scripts')
$scriptName = 'ghidra_export_decomp.java'
$scriptPath = (Join-Path $scriptDir $scriptName)
$useScriptPath = $false
if(Test-Path -LiteralPath $scriptPath){ $useScriptPath = $true }

$argsForScript = @()
if($OutFile){
  $argsForScript += ("outFile=" + $OutFile)
} else {
  $argsForScript += ("outDir=" + (Resolve-Path -LiteralPath $OutDir))
}

# Use short path for the binary to avoid issues with parentheses in .bat
$binShort = Get-ShortPath $BinaryPath

Write-Host "Running analyzeHeadless..."

# Compose headless invocation (do not embed quotes; pass tokens)
$cmd = @(
  $projRoot,
  $ProjectName,
  '-import', $binShort,
  '-overwrite'
)
if($useScriptPath){
  $cmd += @('-scriptPath', $scriptDir)
  $cmd += @('-postScript', $scriptName)
} else {
  # Fallback: hope the script is on Ghidra's default script path
  $cmd += @('-postScript', $scriptName)
}

# Quote script args to preserve spaces/special chars through .bat parsing
$quotedArgs = @()
foreach($a in $argsForScript){ $quotedArgs += '"' + $a + '"' }
$cmd += $quotedArgs

& $ah @cmd

Write-Host "Headless export complete. Outputs in: $OutDir"
