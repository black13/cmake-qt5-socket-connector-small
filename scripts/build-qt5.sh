#!/usr/bin/env bash
set -euo pipefail

# Build Qt 5 (qtbase + qtdeclarative) Debug and Release in this repo.
# - Installs under third_party/qt-<TAG>/install-{debug,release}
# - TAG defaults to 5.15.16; override with QT5_TAG env var.
# - Requires build deps (apt list provided in project build notes).

QT5_TAG=${QT5_TAG:-5.15.16}
REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)
SRC_ROOT="$REPO_ROOT/third_party/qt5-src"
INSTALL_ROOT="$REPO_ROOT/third_party/qt-$QT5_TAG"

echo "Qt5 tag: $QT5_TAG"
echo "Source  : $SRC_ROOT"
echo "Install : $INSTALL_ROOT"

mkdir -p "$SRC_ROOT" "$INSTALL_ROOT"

ensure_repo() {
  local name=$1 url=$2 tag=$3
  if [ -d "$SRC_ROOT/$name/.git" ]; then
    echo "[qt] $name already present"
  else
    echo "[qt] cloning $name ($tag)"
    git clone --depth=1 --branch "$tag" "$url" "$SRC_ROOT/$name"
  fi
}

configure_build_install_qtbase() {
  local build_type=$1  # debug or release
  local prefix="$INSTALL_ROOT/install-$build_type"
  local bdir="$SRC_ROOT/qtbase/build-$build_type"

  mkdir -p "$bdir"
  pushd "$bdir" >/dev/null

  local conf_flags=(
    "-prefix" "$prefix"
    -opensource -confirm-license
    -nomake examples -nomake tests
    -platform linux-g++-64
    -opengl desktop
    -xcb
  )

  if [ "$build_type" = "debug" ]; then
    conf_flags+=( -debug )
  else
    conf_flags+=( -release )
  fi

  echo "[qtbase] configure ($build_type) -> $prefix"
  ../configure "${conf_flags[@]}"
  echo "[qtbase] build ($build_type)"
  make -j"$(nproc)"
  echo "[qtbase] install ($build_type)"
  make install
  popd >/dev/null
}

build_install_qtdeclarative() {
  local build_type=$1  # debug or release
  local prefix="$INSTALL_ROOT/install-$build_type"
  local bdir="$SRC_ROOT/qtdeclarative/build-$build_type"
  local qmake="$prefix/bin/qmake"

  if [ ! -x "$qmake" ]; then
    echo "[qtdeclarative] qmake not found at $qmake (qtbase not installed?)" >&2
    exit 1
  fi

  mkdir -p "$bdir"
  pushd "$bdir" >/dev/null
  local config_arg=
  if [ "$build_type" = "debug" ]; then
    config_arg="CONFIG+=debug"
  else
    config_arg="CONFIG+=release"
  fi

  echo "[qtdeclarative] qmake ($build_type)"
  "$qmake" $config_arg "$SRC_ROOT/qtdeclarative/qtdeclarative.pro"
  echo "[qtdeclarative] build ($build_type)"
  make -j"$(nproc)"
  echo "[qtdeclarative] install ($build_type)"
  make install
  popd >/dev/null
}

main() {
  echo "[qt] fetching sources (qtbase, qtdeclarative)"
  ensure_repo qtbase        https://github.com/qt/qtbase.git        "$QT5_TAG"
  ensure_repo qtdeclarative https://github.com/qt/qtdeclarative.git "$QT5_TAG"

  echo "[qt] building Release first (qtbase + qtdeclarative)"
  configure_build_install_qtbase release
  build_install_qtdeclarative    release

  echo "[qt] building Debug (qtbase + qtdeclarative)"
  configure_build_install_qtbase debug
  build_install_qtdeclarative    debug

  echo "[qt] done. Installed to:"
  echo "  - $INSTALL_ROOT/install-release"
  echo "  - $INSTALL_ROOT/install-debug"

  local envdir="$REPO_ROOT/env"
  mkdir -p "$envdir"
  cat > "$envdir/activate-qt5-release.sh" <<EOF
# Source this to use Qt $QT5_TAG Release from this repo
export QT_ROOT="$INSTALL_ROOT/install-release"
export CMAKE_PREFIX_PATH="$QT_ROOT:
$QT_ROOT/lib/cmake:
$CMAKE_PREFIX_PATH"
export PATH="$QT_ROOT/bin:$PATH"
export PKG_CONFIG_PATH="$QT_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="$QT_ROOT/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$QT_ROOT/plugins:$QT_PLUGIN_PATH"
export QML2_IMPORT_PATH="$QT_ROOT/qml:$QML2_IMPORT_PATH"
echo "Activated Qt $QT5_TAG Release at $QT_ROOT"
EOF

  cat > "$envdir/activate-qt5-debug.sh" <<EOF
# Source this to use Qt $QT5_TAG Debug from this repo
export QT_ROOT="$INSTALL_ROOT/install-debug"
export CMAKE_PREFIX_PATH="$QT_ROOT:
$QT_ROOT/lib/cmake:
$CMAKE_PREFIX_PATH"
export PATH="$QT_ROOT/bin:$PATH"
export PKG_CONFIG_PATH="$QT_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="$QT_ROOT/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$QT_ROOT/plugins:$QT_PLUGIN_PATH"
export QML2_IMPORT_PATH="$QT_ROOT/qml:$QML2_IMPORT_PATH"
echo "Activated Qt $QT5_TAG Debug at $QT_ROOT"
EOF

  echo "[qt] Activation scripts written to: $envdir"
}

main "$@"

