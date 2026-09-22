# scripts/run_coverage.ps1
# Runs the LLVM-instrumented NodeGraph.exe with the coverage test suite,
# collects .profraw, merges, and generates an HTML coverage report.
#
# Usage: .\scripts\run_coverage.ps1 [-BuildDir build_llvm] [-QtPath D:\Qt\5.15.19\debug]
#
# Prerequisites:
#   1. Build with: build-llvm.bat debug
#   2. Run this script (no VS prompt needed - just needs the exe + Qt DLLs on PATH)

param(
    [string]$BuildDir = "build_llvm",
    [string]$QtPath   = "D:\Qt\5.15.19\debug"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $repoRoot

Set-Location $repoRoot

$exe        = "$BuildDir\NodeGraph.exe"
$llvmBin    = "C:\Program Files\LLVM\bin"
$testScript = "tests\coverage_suite.js"
$reportDir  = "$BuildDir\coverage_report"
$profrawDir = "$BuildDir\profraw"

# ── 1. Sanity checks ────────────────────────────────────
if (-not (Test-Path $exe)) {
    Write-Error "NodeGraph.exe not found at $exe. Run: build-llvm.bat debug"
    exit 1
}
if (-not (Test-Path $testScript)) {
    Write-Error "Test script not found at $testScript"
    exit 1
}
if (-not (Test-Path "$llvmBin\llvm-profdata.exe")) {
    Write-Error "LLVM tools not found at $llvmBin. Install: winget install LLVM.LLVM"
    exit 1
}
if (-not (Test-Path "$QtPath\bin\Qt5Cored.dll")) {
    Write-Error "Qt5 not found at $QtPath. Adjust -QtPath parameter."
    exit 1
}

# ── 2. Environment ───────────────────────────────────────
$env:PATH               = "$QtPath\bin;$llvmBin;$env:PATH"
$env:QT_PLUGIN_PATH     = "$QtPath\plugins"

# Clean previous profraw
if (Test-Path $profrawDir) { Remove-Item -Recurse -Force $profrawDir }
New-Item -ItemType Directory -Force -Path $profrawDir | Out-Null

$env:LLVM_PROFILE_FILE = "$profrawDir\nodegraph_%p_%m.profraw"

Write-Host "==============================================" -ForegroundColor Cyan
Write-Host " LLVM Coverage Test Runner"                     -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "Binary:       $exe"
Write-Host "Test:         $testScript"
Write-Host "Profraw dir:  $profrawDir"
Write-Host "Report dir:   $reportDir"
Write-Host ""

# ── 3. Run instrumented binary ──────────────────────────
Write-Host "[1/4] Running instrumented binary with coverage suite..." -ForegroundColor Yellow

$proc = Start-Process -FilePath $exe `
    -ArgumentList "--script", $testScript `
    -NoNewWindow -Wait -PassThru

Write-Host "       Exit code: $($proc.ExitCode)"

# ── 4. Collect profraw files ────────────────────────────
Write-Host "[2/4] Collecting .profraw files..." -ForegroundColor Yellow

$profraws = Get-ChildItem -Path $profrawDir -Filter "*.profraw" -ErrorAction SilentlyContinue
if (-not $profraws -or $profraws.Count -eq 0) {
    Write-Error "No .profraw files generated in $profrawDir"
    Write-Host "Check LLVM_PROFILE_FILE setting and that the binary was built with ENABLE_LLVM_COVERAGE=ON"
    exit 1
}
foreach ($p in $profraws) {
    Write-Host "       $($p.Name)  $($p.Length) bytes"
}
Write-Host "       Total: $($profraws.Count) file(s)"

# ── 5. Merge profraw -> profdata ────────────────────────
Write-Host "[3/4] Merging into merged.profdata..." -ForegroundColor Yellow

$profdata = "$BuildDir\merged.profdata"
& "$llvmBin\llvm-profdata.exe" merge -sparse -o $profdata (Get-ChildItem $profrawDir -Filter "*.profraw" | ForEach-Object { $_.FullName })
if ($LASTEXITCODE -ne 0) {
    Write-Error "llvm-profdata merge failed"
    exit 1
}
$profdataSize = (Get-Item $profdata).Length
Write-Host "       $profdata  $profdataSize bytes"

# ── 6. Generate HTML report ─────────────────────────────
Write-Host "[4/4] Generating HTML coverage report..." -ForegroundColor Yellow

if (Test-Path $reportDir) { Remove-Item -Recurse -Force $reportDir }

$covArgs = @(
    "show", $exe,
    "-instr-profile=$profdata",
    "-format=html",
    "-output-dir=$reportDir",
    "-show-line-counts-or-regions",
    "-show-instantiations",
    "-ignore-filename-regex=.*mocs_compilation.*|.*qrc_icons.*|.*_deps.*"
)
& "$llvmBin\llvm-cov.exe" $covArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error "llvm-cov show failed"
    exit 1
}

# ── 7. Summary ──────────────────────────────────────────
Write-Host ""
Write-Host "=============================================="  -ForegroundColor Green
Write-Host " Coverage report generated!"                      -ForegroundColor Green
Write-Host "=============================================="  -ForegroundColor Green
Write-Host ""
Write-Host "  HTML:  $reportDir\index.html"
Write-Host "  Data:  $profdata"
Write-Host "  Raw:   $profrawDir\"
Write-Host ""
Write-Host "Open in browser:"
Write-Host "  start $reportDir\index.html"
Write-Host ""
