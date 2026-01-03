#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)
DEFAULT_BUILD_DIR="$REPO_ROOT/shared/server/build_debug"
FALLBACK_BUILD_DIR="${HOME:-/tmp}/.cache/vxi11_server_debug"
FALLBACK_BIN="$REPO_ROOT/shared/server/vxi11_server"

if [ -n "${VXI11_BUILD_DIR:-}" ]; then
  DEFAULT_BUILD_DIR="$VXI11_BUILD_DIR"
fi

VXI11_SERVER_BIN=${VXI11_SERVER_BIN:-""}

RUNTIME_DIR=${VXI11_RUNTIME_DIR:-"$REPO_ROOT/logs"}
if ! mkdir -p "$RUNTIME_DIR" 2>/dev/null; then
  RUNTIME_DIR="${HOME:-/tmp}/.cache/vxi11_server"
  mkdir -p "$RUNTIME_DIR"
fi

if [ ! -w "$RUNTIME_DIR" ]; then
  RUNTIME_DIR="${HOME:-/tmp}/.cache/vxi11_server"
  mkdir -p "$RUNTIME_DIR"
fi

VXI11_LOG_FILE=${VXI11_LOG_FILE:-"$RUNTIME_DIR/vxi11_server.log"}
VXI11_PID_FILE=${VXI11_PID_FILE:-"$RUNTIME_DIR/vxi11_server.pid"}

usage() {
  cat <<EOF
Usage: $(basename "$0") <start|stop|status|restart>

Environment:
  VXI11_SERVER_BIN   Path to vxi11_server binary
  VXI11_LOG_FILE     Path to log file
  VXI11_PID_FILE     Path to PID file
EOF
}

ensure_binary() {
  if [ -n "$VXI11_SERVER_BIN" ] && [ -x "$VXI11_SERVER_BIN" ]; then
    return
  fi

  local candidates=(
    "$DEFAULT_BUILD_DIR/vxi11_server"
    "$REPO_ROOT/shared/server/build_release/vxi11_server"
    "${HOME:-/tmp}/.cache/vxi11_server_debug/vxi11_server"
    "${HOME:-/tmp}/.cache/vxi11_server_release/vxi11_server"
    "$FALLBACK_BUILD_DIR/vxi11_server"
    "/tmp/vxi11_server_build/vxi11_server"
    "$FALLBACK_BIN"
  )

  for candidate in "${candidates[@]}"; do
    if [ -x "$candidate" ]; then
      VXI11_SERVER_BIN="$candidate"
      return
    fi
  done

  echo "vxi11_server not found; set VXI11_SERVER_BIN or VXI11_BUILD_DIR." >&2
  exit 1
}

is_running() {
  if [ ! -f "$VXI11_PID_FILE" ]; then
    return 1
  fi
  local pid
  pid=$(cat "$VXI11_PID_FILE")
  if [ -z "$pid" ]; then
    return 1
  fi
  if kill -0 "$pid" 2>/dev/null; then
    return 0
  fi
  return 1
}

start_server() {
  ensure_binary
  if is_running; then
    echo "vxi11_server already running (pid $(cat "$VXI11_PID_FILE"))"
    return 0
  fi
  mkdir -p "$(dirname "$VXI11_LOG_FILE")"
  nohup "$VXI11_SERVER_BIN" >"$VXI11_LOG_FILE" 2>&1 &
  echo $! >"$VXI11_PID_FILE"
  echo "Started vxi11_server (pid $(cat "$VXI11_PID_FILE"))"
}

stop_server() {
  if ! is_running; then
    echo "vxi11_server not running"
    [ -f "$VXI11_PID_FILE" ] && rm -f "$VXI11_PID_FILE"
    return 0
  fi
  local pid
  pid=$(cat "$VXI11_PID_FILE")
  kill "$pid"
  for _ in {1..10}; do
    if ! kill -0 "$pid" 2>/dev/null; then
      rm -f "$VXI11_PID_FILE"
      echo "Stopped vxi11_server"
      return 0
    fi
    sleep 1
  done
  echo "vxi11_server did not exit cleanly (pid $pid)" >&2
  return 1
}

status_server() {
  if is_running; then
    echo "running (pid $(cat "$VXI11_PID_FILE"))"
  else
    echo "stopped"
  fi
}

case "${1:-}" in
  start)
    start_server
    ;;
  stop)
    stop_server
    ;;
  status)
    status_server
    ;;
  restart)
    stop_server
    start_server
    ;;
  *)
    usage
    exit 1
    ;;
esac
