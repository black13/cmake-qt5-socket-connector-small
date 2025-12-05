<#
.SYNOPSIS
  Uninstall NI-VISA and related VISA packages via NI Package Manager (nipkg), then cleanup caches.

.DESCRIPTION
  - Detects nipkg.exe in common locations
  - Lists installed packages and removes those with 'visa' (ni-visa*, visa-runtime) and optional GPIB (ni-488.2)
  - Runs nipkg cleanup to purge caches

.EXAMPLE (run as Admin)
  pwsh -File .\scripts\ni_visa_reset.ps1 -IncludeGPIB -WhatIf

.PARAMETER IncludeGPIB
  Also remove NI-488.2 packages.

.PARAMETER Force
  Pass --force to nipkg remove.

.NOTES
  Requires elevation. You may also remove via GUI (NI Package Manager). Reboot after uninstall.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
  [switch]$IncludeGPIB,
  [switch]$Force
)

function Test-IsAdmin {
  try {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $p  = New-Object Security.Principal.WindowsPrincipal($id)
    return $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
  } catch { return $false }
}

function Find-Nipkg {
  $candidates = @(
    "$Env:ProgramFiles/National Instruments/NI Package Manager/nipkg.exe",
    "$Env:ProgramFiles/NI/NI Package Manager/nipkg.exe"
  )
  foreach ($c in $candidates) { if (Test-Path -LiteralPath $c) { return (Resolve-Path $c).Path } }
  # try PATH
  $try = (Get-Command nipkg -ErrorAction SilentlyContinue)?.Path
  if ($try) { return $try }
  return $null
}

if (-not (Test-IsAdmin)) {
  Write-Error "Please run this script in an elevated PowerShell (Run as administrator)."; exit 1
}

$nipkg = Find-Nipkg
if (-not $nipkg) { Write-Error "Could not find nipkg.exe. Install NI Package Manager first."; exit 1 }

Write-Host "Using: $nipkg" -ForegroundColor Cyan

# Get installed packages
$installed = & $nipkg list --installed 2>$null | Where-Object { $_ -match '^ni-' -or $_ -match 'visa' }

# Build removal set: ni-visa*, visa-runtime*, optional ni-488.2*
$remove = @()
foreach ($line in $installed) {
  $name = ($line -split '\s+')[0]
  if ($name -match '^(ni-visa|ni-visa-runtime|ni-visa-full|visa-runtime)') { $remove += $name; continue }
  if ($IncludeGPIB -and $name -match '^(ni-488\.2)') { $remove += $name; continue }
}

$remove = $remove | Sort-Object -Unique
if ($remove.Count -eq 0) { Write-Host "No VISA-related packages found to remove." -ForegroundColor Yellow; exit 0 }

Write-Host "Packages to remove:" -ForegroundColor Cyan
$remove | ForEach-Object { Write-Host "  $_" }

foreach ($pkg in $remove) {
  $args = @('remove', $pkg, '--yes')
  if ($Force) { $args += '--force' }
  if ($PSCmdlet.ShouldProcess($pkg, 'nipkg remove')) {
    & $nipkg @args
  }
}

if ($PSCmdlet.ShouldProcess('nipkg cache', 'nipkg cleanup --all')) {
  & $nipkg cleanup --all
}

Write-Host "NI-VISA removal complete. Reboot recommended." -ForegroundColor Green

