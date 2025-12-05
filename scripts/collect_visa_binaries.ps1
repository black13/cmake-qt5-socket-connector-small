<#
.SYNOPSIS
  Collect locally installed VISA runtime DLLs into a private research folder for Ghidra analysis.

.DESCRIPTION
  Searches common NI‑VISA install locations and copies selected DLLs into a target folder
  (default: .\research_bins\visa). The target is created if it does not exist. Files are not
  committed to git; this is for local analysis only.

.EXAMPLE
  pwsh ./scripts/collect_visa_binaries.ps1 -TargetDir .\research_bins\visa -WhatIf

.PARAMETER TargetDir
  Destination directory for copied files. Created if missing.

.PARAMETER Overwrite
  Overwrite existing files in the target directory.

.PARAMETER WhatIf
  Show what would be done without copying files.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
  [string]$TargetDir = "./research_bins/visa",
  [switch]$Overwrite
)

function Ensure-Dir($path) {
  if (-not (Test-Path -Path $path -PathType Container)) {
    New-Item -ItemType Directory -Path $path | Out-Null
  }
}

$candidateRoots = @(
  "$Env:ProgramFiles/IVI Foundation/VISA/Win64/Bin",
  "$Env:ProgramFiles/IVI Foundation/VISA/WinNT/Bin",
  "$Env:ProgramFiles(x86)/IVI Foundation/VISA/WinNT/Bin"
)

$dllNames = @(
  'visa64.dll','visa32.dll','nivisa64.dll','nivisa32.dll'
)

Write-Host "Scanning for VISA binaries..." -ForegroundColor Cyan
Ensure-Dir -path $TargetDir

foreach ($root in $candidateRoots) {
  if (-not $root) { continue }
  if (-not (Test-Path -Path $root -PathType Container)) { continue }
  foreach ($name in $dllNames) {
    $source = Join-Path $root $name
    if (Test-Path -Path $source -PathType Leaf) {
      $dest = Join-Path $TargetDir $name
      $action = if ($Overwrite) { 'Copy (overwrite)' } else { 'Copy (skip existing)' }
      if ($PSCmdlet.ShouldProcess($source, $action)) {
        if ($Overwrite -or -not (Test-Path -Path $dest -PathType Leaf)) {
          Copy-Item -Path $source -Destination $dest -Force
          Write-Host "Copied: $name" -ForegroundColor Green
        } else {
          Write-Host "Exists: $name (skipped)" -ForegroundColor Yellow
        }
      }
    }
  }
}

Write-Host "Done. Files in: $((Resolve-Path $TargetDir).Path)" -ForegroundColor Cyan

