#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)
CONFIG_PATH="${VM_CONFIG_PATH:-$REPO_ROOT/scripts/vm_config.sh}"
VM_CTL="$REPO_ROOT/scripts/vm_ctl.sh"

if [ ! -f "$CONFIG_PATH" ]; then
  echo "Missing VM config: $CONFIG_PATH" >&2
  echo "Create scripts/vm_config.sh based on scripts/vm_config.example.sh" >&2
  exit 1
fi

# shellcheck disable=SC1090
source "$CONFIG_PATH"

require_var() {
  local name=$1
  if [ -z "${!name:-}" ]; then
    echo "Config error: $name is required in $CONFIG_PATH" >&2
    exit 1
  fi
}

require_var VM_REPO_DIR

status="$("$VM_CTL" status 2>/dev/null || true)"
if [ "$status" != "running" ]; then
  echo "VM not running; starting..."
  "$VM_CTL" start
fi

"$VM_CTL" wait-ssh "${VM_WAIT_SSH:-60}"

BUILD_CMD=${VM_BUILD_CMD_OVERRIDE:-"./scripts/vxi11_build.sh --rebuild --verbose"}
VM_BUILD_CMD="$BUILD_CMD" "$VM_CTL" build

script_path=${VXI11_SCRIPT_PATH:-"$VM_REPO_DIR/scpi/vxi11_script.js"}
script_path_escaped=$(printf '%q' "$script_path")
server_cmd="VXI11_SCRIPT=$script_path_escaped VXI11_DEBUG=${VXI11_DEBUG:-1} ./scripts/vxi11_server_ctl.sh"
VM_SERVER_CTL_CMD="$server_cmd" "$VM_CTL" server-restart

repo_dir_escaped=$(printf '%q' "$VM_REPO_DIR")
"$VM_CTL" run "cd $repo_dir_escaped && lxi scpi -a 127.0.0.1 '*IDN?'"
"$VM_CTL" run "cd $repo_dir_escaped && lxi scpi -a 127.0.0.1 ':SIM:TRACE:TRI'"

echo "Done."
