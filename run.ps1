# run.ps1 - launch NodeGraph with the Qt runtime on PATH.
#
# The build already deploys Qt next to NodeGraph.exe, so this launcher is the
# belt-and-suspenders path: it prepends the Qt <prefix>\bin for the selected
# configuration and points QT_PLUGIN_PATH at the matching plugins directory,
# then forwards every remaining argument to the application unchanged.
#
# Usage (from the repo root):
#   .\run.ps1                                   # Release build, empty canvas
#   .\run.ps1 --script scripts/drop_demo.js     # any app args are forwarded
#   .\run.ps1 -Debug --script tests/coverage_suite.js
#   .\run.ps1 -BuildDir build_review -Debug
#
# Flags consumed by this script: -Debug / -Release, -BuildDir <dir>.
# Everything else goes to NodeGraph.exe.

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

$config = 'Release'
$buildDirOverride = $null
$forward = @()

$i = 0
while ($i -lt $args.Count) {
    $a = [string]$args[$i]
    switch -Regex ($a) {
        '^--?debug$'   { $config = 'Debug' }
        '^--?release$' { $config = 'Release' }
        '^--?builddir$' {
            ++$i
            if ($i -ge $args.Count) { throw '-BuildDir needs a directory value' }
            $buildDirOverride = [string]$args[$i]
        }
        default { $forward += $a }
    }
    ++$i
}

# Locate the executable: explicit -BuildDir first, then the conventional
# directories. Debug builds may live in build_review (test tree) or
# build_Debug (build.bat tree).
$candidates = @()
if ($buildDirOverride) { $candidates += $buildDirOverride }
if ($config -eq 'Release') {
    $candidates += 'build_Release', 'build'
} else {
    $candidates += 'build_review', 'build_Debug', 'build'
}

$exe = $null
$buildDir = $null
foreach ($candidate in $candidates) {
    $dir = Join-Path $root $candidate
    $path = Join-Path $dir "$config\NodeGraph.exe"
    if (Test-Path -LiteralPath $path) {
        $exe = $path
        $buildDir = $dir
        break
    }
}
if (-not $exe) {
    throw "NodeGraph.exe ($config) not found. Build it first, e.g.: cmake --build build_$config --config $config"
}

# Prefer the Qt install the build was configured against (CMakeCache.txt),
# fall back to the known D:\Qt layout.
$qtBin = $null
$cache = Join-Path $buildDir 'CMakeCache.txt'
if (Test-Path -LiteralPath $cache) {
    $line = Select-String -Path $cache -Pattern '^Qt5Core_DIR:PATH=' | Select-Object -First 1
    if ($line) {
        # Qt5Core_DIR = <prefix>/lib/cmake/Qt5Core
        $qtCoreDir = $line.Line.Substring($line.Line.IndexOf('=') + 1)
        $prefix = Split-Path (Split-Path (Split-Path $qtCoreDir -Parent) -Parent) -Parent
        $candidateBin = Join-Path $prefix 'bin'
        $coreDll = if ($config -eq 'Debug') { 'Qt5Cored.dll' } else { 'Qt5Core.dll' }
        if (Test-Path -LiteralPath (Join-Path $candidateBin $coreDll)) {
            $qtBin = $candidateBin
        }
    }
}
if (-not $qtBin) {
    $fallback = if ($config -eq 'Release') { 'D:\Qt\5.15.19\release\bin' } else { 'D:\Qt\5.15.19\debug\bin' }
    $hasReleaseDll = Test-Path -LiteralPath (Join-Path $fallback 'Qt5Core.dll')
    $hasDebugDll = Test-Path -LiteralPath (Join-Path $fallback 'Qt5Cored.dll')
    if ($hasReleaseDll -or $hasDebugDll) {
        $qtBin = $fallback
    }
}

if ($qtBin) {
    $env:PATH = "$qtBin;$env:PATH"
    $plugins = Join-Path (Split-Path $qtBin -Parent) 'plugins'
    if (Test-Path -LiteralPath $plugins) {
        $env:QT_PLUGIN_PATH = $plugins
    }
    Write-Host "Qt runtime : $qtBin"
} else {
    Write-Warning 'Qt bin directory not found; relying on the DLLs deployed next to NodeGraph.exe.'
}

Write-Host "Executable : $exe"
if ($forward.Count -gt 0) {
    Write-Host "Arguments  : $($forward -join ' ')"
}

& $exe @forward
exit $LASTEXITCODE
