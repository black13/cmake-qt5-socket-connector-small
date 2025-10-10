#!/usr/bin/env bash
set -euo pipefail

# ========= CONFIG =========
BUILD_DIR="build_cov"
CONFIG="Debug"
BIN_NAME="NodeGraph"
RUNS_DIR="coverage_runs"
REPORT_DIR="coverage_html"
PROFILE_DATA="coverage.profdata"
TEST_SCRIPT="./test_qgraph_api.js"
EXTRA_RUN_ARGS=()   # e.g., ("--load" "./graphs/smoke.xml")

# ========= CLEAN =========
rm -rf "$BUILD_DIR" "$RUNS_DIR" "$REPORT_DIR" "$PROFILE_DATA" coverage_raw.json coverage_cpp_only.json
mkdir -p "$BUILD_DIR" "$RUNS_DIR" "$REPORT_DIR"

# ========= COMPILER / CONFIGURE =========
export CC=clang
export CXX=clang++

# Turn on project-wide coverage if your CMakeLists supports it (it does):
cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="$CONFIG" \
  -DENABLE_COVERAGE=ON \
  -DCMAKE_PREFIX_PATH=/opt/Qt/5.15.16/debug

# ========= BUILD =========
cmake --build "$BUILD_DIR" --target NodeGraphCore "$BIN_NAME" -j

# ========= RESOLVE BINARY =========
BIN="$BUILD_DIR/$BIN_NAME"
if [[ -x "$BUILD_DIR/$CONFIG/$BIN_NAME" ]]; then
  BIN="$BUILD_DIR/$CONFIG/$BIN_NAME"
fi
if [[ ! -x "$BIN" ]]; then
  echo "ERROR: Could not find executable '$BIN_NAME' in $BUILD_DIR or $BUILD_DIR/$CONFIG"
  exit 1
fi

# ========= DISPLAY (X11 GUI from WSL to Windows) =========
# If DISPLAY is not set, derive it from Windows host resolver
if [[ -z "${DISPLAY:-}" ]]; then
  host_ip=$(awk '/nameserver/{print $2; exit}' /etc/resolv.conf || true)
  if [[ -n "$host_ip" ]]; then
    export DISPLAY="${host_ip}:0.0"
  fi
fi
# Helpful flags for X11 + Qt across WSL/Windows
export QT_X11_NO_MITSHM=1
# Uncomment if you hit OpenGL issues with your X server:
# export LIBGL_ALWAYS_INDIRECT=1

echo "DISPLAY=${DISPLAY:-<unset>} (ensure your Windows X server is running)"
echo "Launching GUI runs to collect coverage…"

# ========= RUNS (GENERATE *.profraw) =========
LLVM_PROFILE_FILE="$RUNS_DIR/smoke_%p_%m.profraw" "$BIN"

if [[ -f "$TEST_SCRIPT" ]]; then
  LLVM_PROFILE_FILE="$RUNS_DIR/script_%p_%m.profraw" "$BIN" --test-script="$TEST_SCRIPT" "${EXTRA_RUN_ARGS[@]}"
fi

# Example: drive additional scenarios (uncomment/adjust)
# for f in graphs/*.xml; do
#   LLVM_PROFILE_FILE="$RUNS_DIR/$(basename "$f")_%p.profraw" "$BIN" --load "$f"
# done

# Sanity: ensure we produced profiles
shopt -s nullglob
profiles=( "$RUNS_DIR"/*.profraw )
if (( ${#profiles[@]} == 0 )); then
  echo "ERROR: No .profraw files were generated. Is coverage enabled and the app exiting cleanly?"
  exit 2
fi

# ========= MERGE PROFILES =========
llvm-profdata merge -sparse "$RUNS_DIR"/*.profraw -o "$PROFILE_DATA"

# ========= TARGETS FOR REPORT (exclude third-party like _deps) =========
mapfile -t SHOW_TARGETS < <(
  find "$BUILD_DIR" -type f \
    \( -name "$BIN_NAME" -o -name "*.a" -o -name "*.so" \) \
    -not -path "$BUILD_DIR/_deps/*"
)
if [[ ${#SHOW_TARGETS[@]} -eq 0 ]]; then
  SHOW_TARGETS=("$BIN")
fi

IGNORE_RE='(_deps|/usr/include|/usr/lib|/Qt5|/qt/|/llvm/)'

# ========= JSON EXPORT =========
llvm-cov export \
  -instr-profile="$PROFILE_DATA" \
  -ignore-filename-regex="${IGNORE_RE}|\.[hH]$" \
  "${SHOW_TARGETS[@]}" > coverage_raw.json

if command -v jq >/dev/null 2>&1; then
  jq '.data[].files |= map(select(.filename | endswith(".cpp")))' \
     coverage_raw.json > coverage_cpp_only.json
  echo "JSON (cpp-only): coverage_cpp_only.json"
else
  echo "JSON (raw): coverage_raw.json  (install jq to filter to .cpp only)"
fi

# ========= HTML & SUMMARY (project-only) =========
llvm-cov show \
  -format=html \
  -output-dir="$REPORT_DIR" \
  -show-expansions \
  -Xdemangler=c++filt \
  -instr-profile="$PROFILE_DATA" \
  -ignore-filename-regex="$IGNORE_RE" \
  "${SHOW_TARGETS[@]}"

echo "================ Coverage Summary ================"
llvm-cov report \
  -instr-profile="$PROFILE_DATA" \
  -ignore-filename-regex="$IGNORE_RE" \
  "${SHOW_TARGETS[@]}"

echo
echo "HTML report: $REPORT_DIR/index.html"
echo "Tip (Windows): wslview $REPORT_DIR/index.html (or open it manually)"
