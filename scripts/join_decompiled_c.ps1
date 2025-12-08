<#
.SYNOPSIS
  Concatenate multiple decompiled .c files into a single file, with separators.

.EXAMPLE
  pwsh -File .\scripts\join_decompiled_c.ps1 -InputDir .\ghidra\exports\NIMax -OutputFile .\ghidra\exports\NIMax_decompiled.c

.EXAMPLE
  pwsh -File .\scripts\join_decompiled_c.ps1 -InputDir .\ghidra\exports\NIMax -OutputFile .\ghidra\exports\NIMax_decompiled.c -DeleteParts

.PARAMETER InputDir
  Directory containing .c files to merge.

.PARAMETER OutputFile
  Destination file for the combined output.

.PARAMETER DeleteParts
  If specified, deletes the original .c files after writing OutputFile.
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)] [string]$InputDir,
  [Parameter(Mandatory=$true)] [string]$OutputFile,
  [switch]$DeleteParts
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $InputDir -PathType Container)) { throw "InputDir not found: $InputDir" }
$files = Get-ChildItem -LiteralPath $InputDir -Recurse -File -Filter *.c | Sort-Object FullName
if (-not $files -or $files.Count -eq 0) { throw "No .c files found under: $InputDir" }

$outPath = (Resolve-Path (Split-Path -Parent $OutputFile) -ErrorAction SilentlyContinue)
if (-not $outPath) { $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $OutputFile) }

$stamp = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
"/* Combined decompilation output\n   Source dir: $InputDir\n   Files merged: $($files.Count)\n   Generated: $stamp\n*/`n" | Out-File -FilePath $OutputFile -Encoding UTF8

foreach ($f in $files) {
  if ($f.FullName -ieq (Resolve-Path $OutputFile).Path) { continue }
  "/* ===== File: $($f.FullName) ===== */`n" | Out-File -FilePath $OutputFile -Encoding UTF8 -Append
  Get-Content -LiteralPath $f.FullName -Raw | Out-File -FilePath $OutputFile -Encoding UTF8 -Append
  "`n/* ===== End File: $($f.Name) ===== */`n" | Out-File -FilePath $OutputFile -Encoding UTF8 -Append
}

if ($DeleteParts) {
  foreach ($f in $files) {
    if ($f.FullName -ieq (Resolve-Path $OutputFile).Path) { continue }
    Remove-Item -LiteralPath $f.FullName -Force
  }
}

Write-Host "Wrote: $OutputFile" -ForegroundColor Green
if ($DeleteParts) { Write-Host "Deleted original .c parts under: $InputDir" -ForegroundColor Yellow }

