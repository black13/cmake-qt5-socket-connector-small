# scripts/run_coverage_manual.ps1
# Launches instrumented NodeGraph.exe for manual preflight testing.
# You work through the checklist (docs/COVERAGE_CHECKLIST.md), close the app,
# and this script collects + merges + generates the HTML coverage report.
#
# Usage: .\scripts\run_coverage_manual.ps1

param(
    [string]$BuildDir = "build_llvm",
    [string]$QtPath   = "D:\Qt\5.15.19\debug"
)

$ErrorActionPreference = "Stop"
$repoRoot = $PSScriptRoot | Split-Path -Parent
Set-Location $repoRoot

$exe        = "$BuildDir\NodeGraph.exe"
$llvmBin    = "C:\Program Files\LLVM\bin"
$reportDir  = "$BuildDir\coverage_report"
$profrawDir = "$BuildDir\profraw"

# ── Sanity ────────────────────────────────────────────
if (-not (Test-Path $exe)) {
    Write-Error "NodeGraph.exe not found at $exe. Run: build-llvm.bat debug"
    exit 1
}
if (-not (Test-Path "$llvmBin\llvm-profdata.exe")) {
    Write-Error "LLVM not found at $llvmBin"
    exit 1
}

# ── Environment ───────────────────────────────────────
$env:PATH               = "$QtPath\bin;$llvmBin;$env:PATH"
$env:QT_PLUGIN_PATH     = "$QtPath\plugins"

# Clean previous profraw
if (Test-Path $profrawDir) { Remove-Item -Recurse -Force $profrawDir }
New-Item -ItemType Directory -Force -Path $profrawDir | Out-Null
$env:LLVM_PROFILE_FILE = "$profrawDir\nodegraph_%p_%m.profraw"

# ── Launch ────────────────────────────────────────────
Write-Host ""
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "  LLVM COVERAGE — MANUAL PREFLIGHT RUN" -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Checklist:  docs\COVERAGE_CHECKLIST.md"
Write-Host "  Profraw:    $profrawDir\nodegraph_*_*.profraw"
Write-Host ""
Write-Host "  Work through ALL 46 checklist items."
Write-Host "  Then close the app (X button or Alt+F4)."
Write-Host ""
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Launching NodeGraph..." -ForegroundColor Yellow
$proc = Start-Process -FilePath $exe -PassThru

# ── Wait ──────────────────────────────────────────────
Write-Host "Waiting for you to close the app..." -ForegroundColor Yellow
$proc.WaitForExit()

Write-Host ""
Write-Host "App closed. Exit code: $($proc.ExitCode)"

# ── Collect ───────────────────────────────────────────
Start-Sleep -Seconds 1
$profraws = Get-ChildItem -Path $profrawDir -Filter "*.profraw" -ErrorAction SilentlyContinue
if (-not $profraws -or $profraws.Count -eq 0) {
    Write-Error "No .profraw generated! Was the binary built with ENABLE_LLVM_COVERAGE=ON?"
    exit 1
}
Write-Host "Found $($profraws.Count) .profraw file(s):" -ForegroundColor Green
foreach ($p in $profraws) { Write-Host "  $($p.Name)  $($p.Length) bytes" }

# ── Merge ─────────────────────────────────────────────
Write-Host "Merging..." -ForegroundColor Yellow
$profdata = "$BuildDir\merged.profdata"
& "$llvmBin\llvm-profdata.exe" merge -sparse -o $profdata (Get-ChildItem $profrawDir -Filter "*.profraw" | ForEach-Object FullName)
if ($LASTEXITCODE -ne 0) { Write-Error "llvm-profdata merge failed"; exit 1 }
Write-Host "  merged.profdata: $((Get-Item $profdata).Length) bytes" -ForegroundColor Green

# ── HTML Report ───────────────────────────────────────
Write-Host "Generating HTML report..." -ForegroundColor Yellow
if (Test-Path $reportDir) { Remove-Item -Recurse -Force $reportDir }
& "$llvmBin\llvm-cov.exe" show $exe `
    -instr-profile=$profdata `
    -format=html `
    -output-dir=$reportDir `
    -show-line-counts-or-regions `
    -ignore-filename-regex=".*mocs_compilation.*|.*qrc_icons.*|.*_deps.*"
if ($LASTEXITCODE -ne 0) { Write-Error "llvm-cov show failed"; exit 1 }

# ── Summary ───────────────────────────────────────────
& "$llvmBin\llvm-cov.exe" report $exe -instr-profile=$profdata -ignore-filename-regex=".*mocs_compilation.*|.*qrc_icons.*|.*_deps.*"

Write-Host ""
Write-Host "=============================================="  -ForegroundColor Green
Write-Host "  Coverage report ready!"  -ForegroundColor Green
Write-Host "  HTML: $reportDir\index.html"  -ForegroundColor Green
Write-Host "=============================================="  -ForegroundColor Green
Write-Host ""
Write-Host "Open: start $reportDir\index.html" -ForegroundColor Green
