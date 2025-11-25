#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${repo_root}/build_coverage"
mkdir -p "$build_dir"
cd "$repo_root"
export COVERAGE_FLAGS="-fprofile-instr-generate -fcoverage-mapping"
cmake -S . -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  "-DCMAKE_CXX_FLAGS=${COVERAGE_FLAGS}" \
  "-DCMAKE_C_FLAGS=${COVERAGE_FLAGS}"
cmake --build "$build_dir"
rm -f "$build_dir"/nodegraph-*.profraw "$build_dir"/nodegraph.profdata
if [ $# -eq 0 ]; then
  echo "[INFO] Launching NodeGraph without additional arguments (no startup script). Close the app to finish the coverage run."
fi
LLVM_PROFILE_FILE="$build_dir/nodegraph-%p.profraw" "$build_dir/NodeGraph" "$@"
profdata="$build_dir/nodegraph.profdata"
xcrun llvm-profdata merge -sparse "$build_dir"/nodegraph-*.profraw -o "$profdata"
xcrun llvm-cov report "$build_dir/NodeGraph" -instr-profile="$profdata"
