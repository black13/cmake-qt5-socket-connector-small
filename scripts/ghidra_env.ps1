<#
.SYNOPSIS
  Configure environment variables for a local Ghidra install and (optionally) launch it.

.DESCRIPTION
  Sets user-level environment variables:
    - GHIDRA_HOME (default: D:\ghidra_11.3.1_PUBLIC)
    - GHIDRA_JDK  (optional, if -JdkPath is provided)
  Can also launch Ghidra after setting variables.

.EXAMPLE
  pwsh -File .\scripts\ghidra_env.ps1

.EXAMPLE
  pwsh -File .\scripts\ghidra_env.ps1 -JdkPath "C:\\Program Files\\Eclipse Adoptium\\jdk-17.0.12.7-hotspot" -Launch

.PARAMETER Home
  Path to the Ghidra folder containing ghidraRun.bat. Default: D:\ghidra_11.3.1_PUBLIC

.PARAMETER JdkPath
  Optional path to a Java 17 JDK installation. If provided, sets GHIDRA_JDK.

.PARAMETER Launch
  Launch Ghidra after setting variables.
#>
[CmdletBinding()]
param(
  [string]$Home = 'D:\ghidra_11.3.1_PUBLIC',
  [string]$JdkPath,
  [switch]$Launch
)

Write-Host "Configuring Ghidra environment..." -ForegroundColor Cyan

if (-not (Test-Path -LiteralPath $Home)) {
  Write-Warning "GHIDRA_HOME does not exist yet: $Home"
  Write-Warning "Extract Ghidra into this folder so ghidraRun.bat is present."
}

[Environment]::SetEnvironmentVariable('GHIDRA_HOME', $Home, 'User')
Write-Host "Set GHIDRA_HOME (User) = $Home" -ForegroundColor Green

if ($JdkPath) {
  if (-not (Test-Path -LiteralPath $JdkPath)) {
    Write-Warning "JDK path not found: $JdkPath"
  }
  [Environment]::SetEnvironmentVariable('GHIDRA_JDK', $JdkPath, 'User')
  Write-Host "Set GHIDRA_JDK (User) = $JdkPath" -ForegroundColor Green
}

if ($Launch) {
  $runBat = Join-Path $Home 'ghidraRun.bat'
  if (Test-Path -LiteralPath $runBat) {
    Write-Host "Launching: $runBat" -ForegroundColor Cyan
    Start-Process -FilePath $runBat
  } else {
    Write-Warning "Cannot launch: ghidraRun.bat not found at $runBat"
  }
}

Write-Host "Done." -ForegroundColor Cyan

