param(
  [Parameter(Mandatory=$true)] [string]$Target,
  [Parameter(Mandatory=$true)] [string]$Out,
  [int]$RecurseDepth = 1,
  [switch]$Runtime
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function ThrowIfMissing([string]$Path){ if(-not (Test-Path -LiteralPath $Path)){ throw "Path not found: $Path" } }

ThrowIfMissing $Target

function Ensure-Dumpbin(){
  if(Get-Command dumpbin.exe -ErrorAction SilentlyContinue){ return }
  $helper = Join-Path (Get-Location) 'scripts\use_dumpbin.ps1'
  if(Test-Path -LiteralPath $helper){ & $helper }
  if(-not (Get-Command dumpbin.exe -ErrorAction SilentlyContinue)){
    throw 'dumpbin.exe not available. Run scripts/use_dumpbin.ps1 or a VS Developer Command Prompt.'
  }
}

Ensure-Dumpbin

function Get-Dependents([string]$path){
  $p = (Resolve-Path -LiteralPath $path).Path
  $out = & dumpbin.exe /nologo /DEPENDENTS "$p" 2>$null
  $list = @()
  $capture = $false
  foreach($line in $out){
    if($line -match 'Image has the following dependencies') { $capture = $true; continue }
    if($capture){
      $t = $line.Trim()
      if([string]::IsNullOrWhiteSpace($t)) { continue }
      if($t -like '*Summary*') { break }
      $list += $t
    }
  }
  return $list | Sort-Object -Unique
}

$seen = @{}
$queue = New-Object System.Collections.Generic.Queue[object]
$queue.Enqueue(@{ Path = (Resolve-Path -LiteralPath $Target).Path; Depth = 0 })

$sb = New-Object System.Text.StringBuilder
$sb.AppendLine("# Module Map for `"$Target`"") | Out-Null
$sb.AppendLine() | Out-Null

while($queue.Count -gt 0){
  $item = $queue.Dequeue()
  $p = $item.Path
  $d = [int]$item.Depth
  if($seen.ContainsKey($p)){ continue }
  $seen[$p] = $true

  $deps = Get-Dependents $p
  $sb.AppendLine("## $p") | Out-Null
  if($deps.Count -eq 0){
    $sb.AppendLine("(no static dependencies found by dumpbin)") | Out-Null
  } else {
    foreach($dep in $deps){ $sb.AppendLine("- $dep") | Out-Null }
  }
  $sb.AppendLine() | Out-Null

  if($d -lt $RecurseDepth){
    foreach($dep in $deps){
      # Attempt to resolve DLL in common locations if available
      $candidate = $dep
      $resolved = $null
      foreach($dir in @(
        (Split-Path -Parent $p),
        "$Env:SystemRoot\System32",
        "$Env:SystemRoot\SysWOW64",
        "$Env:ProgramFiles",
        "$Env:ProgramFiles(x86)"
      )){
        if([string]::IsNullOrWhiteSpace($dir)){ continue }
        $try = Join-Path $dir $candidate
        if(Test-Path -LiteralPath $try){ $resolved = $try; break }
      }
      if($resolved){ $queue.Enqueue(@{ Path = (Resolve-Path -LiteralPath $resolved).Path; Depth = $d + 1 }) }
    }
  }
}

if($Runtime){
  $sb.AppendLine("## Runtime Modules (best-effort)") | Out-Null
  try {
    $pinfo = New-Object System.Diagnostics.ProcessStartInfo
    $pinfo.FileName = (Resolve-Path -LiteralPath $Target).Path
    $pinfo.UseShellExecute = $false
    $pinfo.RedirectStandardOutput = $false
    $pinfo.RedirectStandardError = $false
    $pinfo.WorkingDirectory = (Split-Path -Parent $pinfo.FileName)
    $proc = [System.Diagnostics.Process]::Start($pinfo)
    Start-Sleep -Seconds 5
    $mods = $proc.Modules | ForEach-Object { $_.FileName } | Sort-Object -Unique
    foreach($m in $mods){ $sb.AppendLine("- $m") | Out-Null }
    $proc.Kill() | Out-Null
  } catch {
    $sb.AppendLine("(runtime scan failed: $_)") | Out-Null
  }
}

$outPath = (Resolve-Path -LiteralPath (Split-Path -Parent $Out) -ErrorAction SilentlyContinue)
if(-not $outPath){ $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) }
[System.IO.File]::WriteAllText($Out, $sb.ToString())
Write-Host "Module map written: $Out"

