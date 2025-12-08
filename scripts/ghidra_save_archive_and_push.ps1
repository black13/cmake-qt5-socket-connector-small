<#
.SYNOPSIS
  Add/commit/push the newest Ghidra Project Archive (.gar) from ghidra\archives.

.DESCRIPTION
  Finds the most recently modified .gar under ghidra\archives (or uses -ArchivePath),
  then runs git add/commit/push on the current branch with a timestamped message.

.EXAMPLE
  pwsh -File .\scripts\ghidra_save_archive_and_push.ps1

.EXAMPLE
  pwsh -File .\scripts\ghidra_save_archive_and_push.ps1 -ArchivePath .\ghidra\archives\NI-VISA-Research_20251205-1530.gar

.PARAMETER ArchivePath
  Optional explicit path to a .gar file to commit.
#>
[CmdletBinding()]
param(
  [string]$ArchivePath
)

$ErrorActionPreference = 'Stop'

function Ensure-InRepo {
  if (-not (Test-Path -LiteralPath '.git')) {
    throw 'Run this from the repository root.'
  }
}

function Find-LatestArchive {
  $dir = Join-Path (Get-Location) 'ghidra/archives'
  if (-not (Test-Path -LiteralPath $dir)) {
    throw "Archives folder not found: $dir"
  }
  $f = Get-ChildItem -LiteralPath $dir -Filter *.gar -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
  if (-not $f) { throw 'No .gar files found under ghidra/archives' }
  return $f.FullName
}

Ensure-InRepo
if (-not $ArchivePath) { $ArchivePath = Find-LatestArchive }
if (-not (Test-Path -LiteralPath $ArchivePath)) { throw "Archive not found: $ArchivePath" }

Write-Host "Committing archive: $ArchivePath" -ForegroundColor Cyan

$branch = (git rev-parse --abbrev-ref HEAD).Trim()
$stamp  = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'

git add -- "$ArchivePath"
git commit -m "ghidra(archive): $([System.IO.Path]::GetFileName($ArchivePath)) @ $stamp"
git push -u origin $branch

Write-Host 'Done.' -ForegroundColor Green

