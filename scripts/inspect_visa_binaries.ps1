<#
.SYNOPSIS
  Inspect VISA DLLs: find candidate VISA binaries, print file/product versions, and (optionally) check exports.

.DESCRIPTION
  Scans common directories for VISA DLLs (nivisa64.dll, visa64.dll, nivisa32.dll, visa32.dll) and prints metadata
  using FileVersionInfo. With -CheckExports and if dumpbin is available, it confirms presence of key VISA exports.

.EXAMPLE
  pwsh -File .\scripts\inspect_visa_binaries.ps1

.EXAMPLE
  # Also verify exported symbols via dumpbin if available
  pwsh -File .\scripts\inspect_visa_binaries.ps1 -CheckExports

.PARAMETER Roots
  Optional directories to search; if omitted, searches common Program Files and Windows system folders.

.PARAMETER CheckExports
  Use dumpbin /exports to check for key symbol names.
#>
[CmdletBinding()]
param(
  [string[]]$Roots,
  [switch]$CheckExports
)

$defaultRoots = @(
  "$Env:ProgramFiles/IVI Foundation/VISA/Win64/Bin",
  "$Env:ProgramFiles(x86)/IVI Foundation/VISA/WinNT/Bin",
  "$Env:ProgramFiles/IVI Foundation/VISA/WinNT/Bin",
  "$Env:ProgramFiles",
  "$Env:ProgramFiles(x86)",
  "$Env:WINDIR/System32",
  "$Env:WINDIR/SysWOW64"
)

$searchRoots = if ($Roots -and $Roots.Length -gt 0) { $Roots } else { $defaultRoots }
$searchRoots = $searchRoots | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

if ($searchRoots.Count -eq 0) {
  Write-Host "No valid roots to search." -ForegroundColor Yellow
  exit 0
}

$names = 'nivisa64.dll','visa64.dll','nivisa32.dll','visa32.dll'
$found = @()

foreach ($root in $searchRoots) {
  foreach ($name in $names) {
    try {
      $items = Get-ChildItem -LiteralPath $root -Recurse -ErrorAction SilentlyContinue -Filter $name -File
      if ($items) { $found += $items }
    } catch {}
  }
}

if (-not $found -or $found.Count -eq 0) {
  Write-Host "No VISA DLLs found under searched roots." -ForegroundColor Yellow
  exit 0
}

Write-Host "Found VISA DLLs:" -ForegroundColor Cyan
$found = $found | Sort-Object FullName -Unique

$keyExports = @('viOpenDefaultRM','viOpen','viClose','viRead','viWrite','viSetAttribute','viGetAttribute','viFindRsrc')

function Try-Dumpbin {
  $g = Get-Command dumpbin -ErrorAction SilentlyContinue
  if ($g) { return $g.Path } else { return $null }
}

$dumpbin = $null
if ($CheckExports) { $dumpbin = Try-Dumpbin }

foreach ($f in $found) {
  $vi = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($f.FullName)
  Write-Host "- $($f.FullName)" -ForegroundColor Gray
  Write-Host ("  FileVersion   : {0}" -f $vi.FileVersion)
  Write-Host ("  ProductVersion: {0}" -f $vi.ProductVersion)
  if ($vi.CompanyName) { Write-Host ("  Company       : {0}" -f $vi.CompanyName) }
  if ($vi.ProductName) { Write-Host ("  Product       : {0}" -f $vi.ProductName) }

  if ($CheckExports -and $dumpbin) {
    try {
      $exports = & $dumpbin /exports $f.FullName 2>$null
      foreach ($sym in $keyExports) {
        $present = ($exports | Select-String -SimpleMatch $sym) -ne $null
        Write-Host ("    {0,-16} : {1}" -f $sym, ($present ? 'present' : 'missing'))
      }
    } catch {
      Write-Host "  (dumpbin failed)" -ForegroundColor Yellow
    }
  }
}

