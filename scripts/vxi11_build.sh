#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)
SRC_DIR="$REPO_ROOT/shared/server"

BUILD_TYPE="Debug"
for arg in "$@"; do
  case $arg in
    debug|Debug)
      BUILD_TYPE="Debug"
      ;;
    release|Release)
      BUILD_TYPE="Release"
      ;;
    *)
      echo "Usage: $(basename "$0") [debug|release]" >&2
      exit 1
      ;;
  esac
done

build_suffix=$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')
DEFAULT_BUILD_DIR="$SRC_DIR/build_${build_suffix}"
FALLBACK_BUILD_DIR="${HOME:-/tmp}/.cache/vxi11_server_${build_suffix}"

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
if [ -z "${CFLAGS:-}" ]; then
  if [ "$BUILD_TYPE" = "Debug" ]; then
    CFLAGS="-O0 -g"
  else
    CFLAGS="-O2"
  fi
fi
if [ -z "${CXXFLAGS:-}" ]; then
  if [ "$BUILD_TYPE" = "Debug" ]; then
    CXXFLAGS="-O0 -g -std=c++17"
  else
    CXXFLAGS="-O2 -std=c++17"
  fi
fi
RPC_INCLUDE=${RPC_INCLUDE:-"/usr/include/tirpc"}
RPC_LIBS=${RPC_LIBS:-"-ltirpc"}
DUKTAPE_DIR=${DUKTAPE_DIR:-"$REPO_ROOT/third_party/duktape"}

if [ ! -d "$SRC_DIR" ]; then
  echo "Source directory not found: $SRC_DIR" >&2
  exit 1
fi

mkdir -p "$BUILD_DIR"
echo "Build type: $BUILD_TYPE"
echo "Using build directory: $BUILD_DIR"

DUKTAPE_SRC="$DUKTAPE_DIR/duktape.c"
DUKTAPE_HDR="$DUKTAPE_DIR/duktape.h"
if [ -f "$DUKTAPE_SRC" ] && [ -f "$DUKTAPE_HDR" ]; then
  CFLAGS="$CFLAGS -DVXI11_USE_DUKTAPE -I$DUKTAPE_DIR"
  DUKTAPE_OBJ="$BUILD_DIR/duktape.o"
fi

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

if [ -n "${DUKTAPE_OBJ:-}" ]; then
  "${CC}" $CFLAGS -c "$DUKTAPE_SRC" -o "$DUKTAPE_OBJ"
  if [ -f "$DUKTAPE_OBJ" ]; then
    objs+=("$DUKTAPE_OBJ")
  fi
fi

"$CXX" $CXXFLAGS -o "$BUILD_DIR/vxi11_server" "${objs[@]}" $RPC_LIBS -lpthread -lm

echo "Built: $BUILD_DIR/vxi11_server"
