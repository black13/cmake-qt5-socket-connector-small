<#
.SYNOPSIS
  Safely remove Ghidra (Windows): user configs, caches, and optional install folders.

.DESCRIPTION
  - Stops Ghidra-related processes (best effort)
  - Removes GHIDRA_* environment variables (User/Machine, if permitted)
  - Deletes user config folders .ghidra* under %USERPROFILE%
  - Optionally deletes ghidra_projects and install folders you provide

.EXAMPLE
  # Dry-run (what if only)
  pwsh -File .\scripts\cleanup_ghidra.ps1 -WhatIf

.EXAMPLE
  # Aggressive: delete user configs + default project folder, keep installs
  pwsh -File .\scripts\cleanup_ghidra.ps1 -PurgeProjects

.EXAMPLE
  # Delete specific install folders too
  pwsh -File .\scripts\cleanup_ghidra.ps1 -InstallDirs 'C:\ghidra_10.4.2','D:\tools\ghidra' -PurgeProjects

.PARAMETER InstallDirs
  One or more Ghidra install directories to remove. If omitted, install folders are not touched.

.PARAMETER PurgeProjects
  Remove %USERPROFILE%\ghidra_projects (if present). WARNING: deletes your local projects.

.NOTES
  Run in an elevated PowerShell to ensure Machine-level env var removal and Program Files deletes.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
  [string[]]$InstallDirs = @(),
  [switch]$PurgeProjects
)

function Test-IsAdmin {
  try {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $p  = New-Object Security.Principal.WindowsPrincipal($id)
    return $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
  } catch { return $false }
}

function Stop-GhidraProcesses {
  Write-Host "Stopping Ghidra-related processes (best effort)..." -ForegroundColor Cyan
  try {
    $procs = Get-CimInstance Win32_Process | Where-Object {
      $_.CommandLine -match 'ghidra' -or $_.Name -match 'ghidra'
    }
    foreach ($p in $procs) {
      try {
        if ($PSCmdlet.ShouldProcess("$($p.Name) (PID $($p.ProcessId))","Stop-Process")) {
          Stop-Process -Id $p.ProcessId -Force -ErrorAction SilentlyContinue
        }
      } catch {}
    }
  } catch {}
}

function Remove-EnvVar([string]$name) {
  foreach ($scope in @('User','Machine')) {
    try {
      $current = [Environment]::GetEnvironmentVariable($name, $scope)
      if ($current) {
        if ($PSCmdlet.ShouldProcess("$name ($scope)","Clear env var")) {
          [Environment]::SetEnvironmentVariable($name, $null, $scope)
          Write-Host "Cleared $name ($scope)" -ForegroundColor Yellow
        }
      }
    } catch {}
  }
}

function Remove-Folder([string]$path) {
  if (-not $path) { return }
  if (-not (Test-Path -LiteralPath $path)) { return }
  if ($PSCmdlet.ShouldProcess($path, "Remove-Item -Recurse -Force")) {
    try { Remove-Item -LiteralPath $path -Recurse -Force -ErrorAction Stop } catch {
      Write-Warning "Failed to remove: $path ($_ )"
    }
  }
}

# 1) Stop processes
Stop-GhidraProcesses

# 2) Clear GHIDRA_* environment variables
Write-Host "Removing GHIDRA_* environment variables..." -ForegroundColor Cyan
Remove-EnvVar GHIDRA_HOME
Remove-EnvVar GHIDRA_INSTALL_DIR
Remove-EnvVar GHIDRA_JDK

# 3) Delete user configs and caches under %USERPROFILE%
$home = $Env:USERPROFILE
$userConfigs = Get-ChildItem -Force -LiteralPath $home -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -like '.ghidra*' }
foreach ($d in $userConfigs) { Remove-Folder $d.FullName }

# 4) Optionally delete default project folder
if ($PurgeProjects) {
  $proj = Join-Path $home 'ghidra_projects'
  Remove-Folder $proj
}

# 5) Delete specified install folders
foreach ($dir in $InstallDirs) { Remove-Folder $dir }

Write-Host "Ghidra cleanup complete." -ForegroundColor Green
if (-not (Test-IsAdmin)) { Write-Host "Note: Run elevated for full Machine-scope cleanup." -ForegroundColor Yellow }

