#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)
SRC_DIR="$REPO_ROOT/shared/server"
DEFAULT_BUILD_DIR="$SRC_DIR/build"
FALLBACK_BUILD_DIR="${HOME:-/tmp}/.cache/vxi11_server"

if [ -n "${VXI11_BUILD_DIR:-}" ]; then
  BUILD_DIR="$VXI11_BUILD_DIR"
else
  BUILD_DIR="$DEFAULT_BUILD_DIR"
  if ! mkdir -p "$BUILD_DIR" 2>/dev/null; then
    BUILD_DIR="$FALLBACK_BUILD_DIR"
  fi
fi

CC=${CC:-gcc}
CXX=${CXX:-g++}
CFLAGS=${CFLAGS:-"-O2"}
CXXFLAGS=${CXXFLAGS:-"-O2 -std=c++17"}
RPC_INCLUDE=${RPC_INCLUDE:-"/usr/include/tirpc"}
RPC_LIBS=${RPC_LIBS:-"-ltirpc"}

if [ ! -d "$SRC_DIR" ]; then
  echo "Source directory not found: $SRC_DIR" >&2
  exit 1
fi

mkdir -p "$BUILD_DIR"
echo "Using build directory: $BUILD_DIR"

objs=(
  "$BUILD_DIR/vxi11core_server.o"
  "$BUILD_DIR/vxi11core_svc.o"
  "$BUILD_DIR/vxi11core_xdr.o"
  "$BUILD_DIR/scpi_core.o"
)

"$CC" $CFLAGS -I"$RPC_INCLUDE" -c "$SRC_DIR/vxi11core_server.c" -o "${objs[0]}"
"$CC" $CFLAGS -I"$RPC_INCLUDE" -c "$SRC_DIR/vxi11core_svc.c" -o "${objs[1]}"
"$CC" $CFLAGS -I"$RPC_INCLUDE" -c "$SRC_DIR/vxi11core_xdr.c" -o "${objs[2]}"
"$CXX" $CXXFLAGS -I"$RPC_INCLUDE" -c "$SRC_DIR/scpi_core.cpp" -o "${objs[3]}"

"$CXX" $CXXFLAGS -o "$BUILD_DIR/vxi11_server" "${objs[@]}" $RPC_LIBS -lpthread -lm

echo "Built: $BUILD_DIR/vxi11_server"
