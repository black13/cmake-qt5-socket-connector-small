#!/usr/bin/env bash
# NodeGraph LLVM/Clang Build Script for WSL/Ubuntu
# - Prefers Clang/LLVM toolchain
# - Optional LLVM source-based coverage
# - Preserves your Qt5 autodetection & build flow

set -euo pipefail

echo "🚀 NodeGraph LLVM Build Script"
echo "================================"

# ----------------------------
# 1) Parse args
# ----------------------------
BUILD_TYPE="Debug"
CLEAN_BUILD=false
COVERAGE=false

usage() {
  cat <<EOF
Usage: $0 [debug|release] [clean] [coverage]

Examples:
  $0 debug
  $0 release
  $0 debug clean
  $0 coverage          # Debug + LLVM coverage instrumentation

Default: debug (incremental, no coverage)
EOF
}

for arg in "$@"; do
  case "$arg" in
    release|Release) BUILD_TYPE="Release" ;;
    debug|Debug)     BUILD_TYPE="Debug" ;;
    clean)           CLEAN_BUILD=true ;;
    coverage|Coverage)
      BUILD_TYPE="Debug"   # coverage is most reliable with debug info
      COVERAGE=true
      ;;
    -h|--help) usage; exit 0 ;;
    *) usage; exit 1 ;;
  esac
done

echo "Build Type: $BUILD_TYPE"
echo "Clean Build: $CLEAN_BUILD"
echo "Coverage: $COVERAGE"

# ----------------------------
# 2) Environment checks
# ----------------------------
BLUE='\033[0;34m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; NC='\033[0m'
info(){ echo -e "${BLUE}[INFO]${NC} $*"; }
ok(){   echo -e "${GREEN}[OK]${NC} $*"; }
warn(){ echo -e "${YELLOW}[WARN]${NC} $*"; }
err(){  echo -e "${RED}[ERR]${NC}  $*"; }

if grep -qi microsoft /proc/version; then
  info "Running in WSL environment"
else
  warn "Not detected as WSL — continuing"
fi

# Build tools
for t in cmake make; do
  command -v "$t" >/dev/null || { err "$t not found. Install build-essential & cmake."; exit 1; }
done
ok "CMake & build tools present"

# ----------------------------
# 3) Choose LLVM/Clang toolchain
# ----------------------------
CXX_CANDIDATES=(clang++-18 clang++-17 clang++-16 clang++-15 clang++-14 clang++)
CC_CANDIDATES=(clang-18 clang-17 clang-16 clang-15 clang-14 clang)

CXX=""
CC=""
for c in "${CXX_CANDIDATES[@]}"; do command -v "$c" >/dev/null && { CXX="$c"; break; }; done
for c in "${CC_CANDIDATES[@]}";  do command -v "$c" >/dev/null && { CC="$c"; break; }; done

if [[ -z "$CXX" || -z "$CC" ]]; then
  warn "Clang not found; falling back to GCC (no llvm-cov features guaranteed)"
  CXX="g++"; CC="gcc"
else
  ok "Using Clang toolchain: CC=$CC CXX=$CXX"
fi

# Match llvm-cov/profdata to clang major if possible
LLVM_COV="llvm-cov"
LLVM_PROFDATA="llvm-profdata"
if [[ "$CXX" =~ clang\+\+-([0-9]+) ]]; then
  MAJ="${BASH_REMATCH[1]}"
  command -v "llvm-cov-$MAJ" >/dev/null && LLVM_COV="llvm-cov-$MAJ"
  command -v "llvm-profdata-$MAJ" >/dev/null && LLVM_PROFDATA="llvm-profdata-$MAJ"
fi

# ----------------------------
# 4) Qt5 detection (preserved from your script)
# ----------------------------
info "Checking Qt5 installation..."
QT_INSTALLS=($(find /usr/local -maxdepth 1 -name "qt*" -type d 2>/dev/null | sort -V -r))
QT_PATH=""; SYSTEM_QT_VERSION=""

if command -v qmake >/dev/null; then
  SYSTEM_QT_VERSION=$(qmake -version | awk '/Qt version/{print $4}')
  if [[ "$SYSTEM_QT_VERSION" == 5.* ]]; then
    ok "Found system Qt5: $SYSTEM_QT_VERSION"
  fi
fi

if [[ ${#QT_INSTALLS[@]} -eq 0 && -z "$SYSTEM_QT_VERSION" ]]; then
  err "No Qt5 found. Install: sudo apt install qtbase5-dev qtdeclarative5-dev"
  exit 1
fi

# Prefer custom installs in /usr/local (debug/release matched)
if [[ "$BUILD_TYPE" == "Debug" ]]; then
  for d in "${QT_INSTALLS[@]}"; do
    [[ -d "$d/lib/cmake/Qt5" && "$d" == *"-debug"* ]] && QT_PATH="$d" && break
  done
else
  for d in "${QT_INSTALLS[@]}"; do
    [[ -d "$d/lib/cmake/Qt5" && "$d" == *"-release"* ]] && QT_PATH="$d" && break
  done
fi
# Fallback: any /usr/local/qt*
if [[ -z "$QT_PATH" && ${#QT_INSTALLS[@]} -gt 0 ]]; then
  for d in "${QT_INSTALLS[@]}"; do
    [[ -d "$d/lib/cmake/Qt5" ]] && QT_PATH="$d" && break
  done
fi
[[ -n "$QT_PATH" ]] && ok "Using custom Qt at $QT_PATH" || ok "Using system Qt5"

# ----------------------------
# 5) Build directory & cache
# ----------------------------
BUILD_DIR="build_llvm"
[[ "$COVERAGE" == true ]] && BUILD_DIR="build_llvm_cov"
CACHE_DIR=".cmake-cache"

if [[ -d "$BUILD_DIR" && "$CLEAN_BUILD" == true ]]; then
  warn "Clean build requested — removing $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

# ----------------------------
# 6) Configure CMake (LLVM & optional coverage)
# ----------------------------
info "Configuring project with CMake…"
pushd "$BUILD_DIR" >/dev/null

# Compiler & flags
CMAKE_OPTS=(
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  -DCMAKE_C_COMPILER="$CC"
  -DCMAKE_CXX_COMPILER="$CXX"
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

# Coverage flags (LLVM source-based)
if [[ "$COVERAGE" == true && "$CXX" == clang* ]]; then
  CMAKE_OPTS+=(
    "-DCMAKE_CXX_FLAGS=-O0 -g -fprofile-instr-generate -fcoverage-mapping"
    "-DCMAKE_C_FLAGS=-O0 -g -fprofile-instr-generate -fcoverage-mapping"
    "-DCMAKE_EXE_LINKER_FLAGS=-fprofile-instr-generate"
    "-DCMAKE_SHARED_LINKER_FLAGS=-fprofile-instr-generate"
  )
fi

# Qt prefix if using custom
if [[ -n "$QT_PATH" ]]; then
  CMAKE_OPTS+=("-DCMAKE_PREFIX_PATH=$QT_PATH")
fi

cmake "${CMAKE_OPTS[@]}" .. || { err "CMake configuration failed"; exit 1; }
ok "CMake configuration completed"

# ----------------------------
# 7) Build
# ----------------------------
CORES=$(nproc)
info "Building with $CORES cores…"
cmake --build . --config "$BUILD_TYPE" --parallel "$CORES"
ok "Build completed"

# ----------------------------
# 8) Post-build info
# ----------------------------
BIN="./NodeGraph"
if [[ -x "$BIN" ]]; then
  ok "Executable: $(pwd)/NodeGraph"
else
  err "NodeGraph executable not found after build"; exit 1
fi

if [[ -z "${DISPLAY:-}" ]]; then
  warn "DISPLAY not set. To run GUI via Windows X server:"
  echo "  1) Start VcXsrv/X410 on Windows"
  echo "  2) export DISPLAY=:0"
  echo "  3) ./NodeGraph"
fi

# ----------------------------
# 9) (Optional) Quick coverage report helper
#     Run only if coverage build requested and llvm tools exist
# ----------------------------
if [[ "$COVERAGE" == true && "$CXX" == clang* ]]; then
  if command -v "$LLVM_COV" >/dev/null && command -v "$LLVM_PROFDATA" >/dev/null; then
    cat <<'TIP'

To collect coverage:
  export QT_QPA_PLATFORM=offscreen
  export LLVM_PROFILE_FILE=default.profraw
  ./NodeGraph   # run your app or test runner to generate profile data

  # Merge
  LLVM_PROFDATA_CMD default.profraw -> default.profdata
  # Show summary
  LLVM_COV_CMD report ./NodeGraph -instr-profile=default.profdata
  # HTML
  LLVM_COV_CMD show ./NodeGraph -instr-profile=default.profdata \
    -format=html -output-dir=coverage-html \
    -ignore-filename-regex='(Qt|moc_|/usr/include/)'

Replace LLVM_COV_CMD and LLVM_PROFDATA_CMD with the detected tools below.
TIP
    echo "Detected tools:"
    echo "  llvm-cov    : $LLVM_COV"
    echo "  llvm-profdata: $LLVM_PROFDATA"
  else
    warn "llvm-cov/llvm-profdata not found; install matching LLVM tools to report coverage."
  fi
fi

popd >/dev/null
ok "🎉 Build script finished successfully"
