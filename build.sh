#!/usr/bin/env bash
set -euo pipefail

echo "🚀 NodeGraph Linux Build Script"
echo "================================"

# ---------------------------
# Defaults
# ---------------------------
BUILD_TYPE="Debug"
CLEAN_BUILD=false
ENABLE_COVERAGE=false
BUILD_DIR="build_linux"

# ---------------------------
# Usage
# ---------------------------
usage() {
  cat <<EOF
Usage: ./build.sh [debug|release] [clean] [coverage]

Examples:
  ./build.sh debug                 # Debug build (incremental)
  ./build.sh release               # Release build (incremental)
  ./build.sh debug clean           # Debug build (clean)
  ./build.sh release clean         # Release build (clean)
  ./build.sh debug coverage        # Debug build with coverage (HTML + JSON)
  ./build.sh release coverage      # Release build with coverage (HTML + JSON)

Environment:
  QT_PATH=/path/to/Qt (optional; passed to CMake as -DCMAKE_PREFIX_PATH)

Notes:
  - 'coverage' enables -DENABLE_COVERAGE=ON and runs the 'coverage_report' target.
  - Requires clang/llvm-profdata/llvm-cov when coverage is ON.
EOF
}

# ---------------------------
# Parse args (order-independent)
# ---------------------------
if [ "$#" -gt 0 ]; then
  for arg in "$@"; do
    case "$arg" in
      debug|Debug)     BUILD_TYPE="Debug" ;;
      release|Release) BUILD_TYPE="Release" ;;
      clean|Clean)     CLEAN_BUILD=true ;;
      coverage|Coverage) ENABLE_COVERAGE=true ;;
      -h|--help) usage; exit 0 ;;
      *) echo "Unknown argument: $arg"; usage; exit 1 ;;
    esac
  done
fi

echo "Build Type     : $BUILD_TYPE"
echo "Clean Build    : $([ "$CLEAN_BUILD" = true ] && echo Yes || echo No)"
echo "Coverage       : $([ "$ENABLE_COVERAGE" = true ] && echo ON || echo OFF)"
echo "Build Directory: $BUILD_DIR"
echo ""

# ---------------------------
# Helpers
# ---------------------------
print_status()  { echo -e "\033[1;34m[STATUS]\033[0m $*"; }
print_success() { echo -e "\033[1;32m[SUCCESS]\033[0m $*"; }
print_warning() { echo -e "\033[1;33m[WARNING]\033[0m $*"; }
print_error()   { echo -e "\033[1;31m[ERROR]\033[0m $*"; }

# Normalize line endings of this file in case it was edited on Windows
sed -i 's/\r$//' "$0" 2>/dev/null || true

# ---------------------------
# Prepare build dir
# ---------------------------
if [ "$CLEAN_BUILD" = true ] && [ -d "$BUILD_DIR" ]; then
  print_status "Cleaning build directory: $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
print_success "Build directory ready: $BUILD_DIR"
cd "$BUILD_DIR"

# ---------------------------
# Configure with CMake
# ---------------------------
print_status "Configuring project with CMake..."
EXTRA_OPTS=(
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

# Qt prefix (optional)
if [ -n "${QT_PATH:-}" ]; then
  EXTRA_OPTS+=(-DCMAKE_PREFIX_PATH="$QT_PATH")
fi

# Coverage toggle
if [ "$ENABLE_COVERAGE" = true ]; then
  EXTRA_OPTS+=(-DENABLE_COVERAGE=ON)
else
  EXTRA_OPTS+=(-DENABLE_COVERAGE=OFF)
fi

cmake "${EXTRA_OPTS[@]}" .. || { print_error "CMake configuration failed!"; exit 1; }
print_success "CMake configuration completed"

# ---------------------------
# Build
# ---------------------------
print_status "Building..."
cmake --build . --config "$BUILD_TYPE"
print_success "Build completed successfully!"

# ---------------------------
# Coverage (optional)
# ---------------------------
if [ "$ENABLE_COVERAGE" = true ]; then
  print_status "Preparing LLVM coverage artifacts..."

  if ! command -v clang++ >/dev/null 2>&1; then
    print_warning "clang++ not found. JSON/HTML coverage requires Clang/LLVM toolchain."
  fi
  if ! command -v llvm-profdata >/dev/null 2>&1 || ! command -v llvm-cov >/dev/null 2>&1; then
    print_warning "llvm-profdata/llvm-cov missing. coverage_report target will fail unless these tools are installed."
  fi

  PROFILE_PREFIX="$(pwd)/default"
  rm -f "${PROFILE_PREFIX}"*.profraw "${PROFILE_PREFIX}.profdata" coverage.json 2>/dev/null || true
  rm -rf coverage_html 2>/dev/null || true

  export LLVM_PROFILE_FILE="${PROFILE_PREFIX}.%p.profraw"
  print_status "Running tests with LLVM_PROFILE_FILE=${LLVM_PROFILE_FILE}"
  if command -v ctest >/dev/null 2>&1; then
    if ! ctest --output-on-failure; then
      print_error "CTest failed while gathering coverage data"
      exit 1
    fi
  else
    print_warning "ctest not available; run your test suite manually to produce .profraw files."
  fi
  unset LLVM_PROFILE_FILE

  shopt -s nullglob
  profraw_files=("${PROFILE_PREFIX}".*.profraw)
  shopt -u nullglob

  if [ "${#profraw_files[@]}" -eq 0 ]; then
    print_warning "No coverage data (*.profraw) generated. Run your instrumented binaries/tests and rerun 'cmake --build . --target coverage_report'."
  else
    print_status "Merging profiles and generating coverage report..."
    if ! cmake --build . --config "$BUILD_TYPE" --target coverage_report; then
      print_error "Coverage report failed!"
      exit 1
    fi
    print_success "Coverage reports generated:"
    echo "  HTML : $(pwd)/coverage_html/index.html"
    echo "  JSON : $(pwd)/coverage.json"
  fi
fi

print_success "🎉 Build script finished"
