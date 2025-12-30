#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)
CONFIG_PATH="${VM_CONFIG_PATH:-$REPO_ROOT/scripts/vm_config.sh}"

usage() {
  cat <<EOF
Usage: $(basename "$0") <command> [args]

Commands:
  start             Start the VM
  stop              Stop the VM (acpi or poweroff)
  status            Show VM state
  wait-ssh [secs]   Wait until SSH is available (default 60s)
  ssh               Open an interactive SSH session
  build             Run VM build command
  server-start      Start the VXI-11 server in the VM
  server-stop       Stop the VXI-11 server in the VM
  server-status     Check VXI-11 server status in the VM
  server-restart    Restart the VXI-11 server in the VM

Config:
  Edit scripts/vm_config.sh (see scripts/vm_config.example.sh).
EOF
}

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

require_var VM_NAME
require_var VM_SSH_HOST
require_var VM_SSH_PORT
require_var VM_SSH_USER
require_var VM_REPO_DIR

VM_START_TYPE=${VM_START_TYPE:-headless}
VM_STOP_MODE=${VM_STOP_MODE:-acpi}
VM_BUILD_CMD=${VM_BUILD_CMD:-"./scripts/vxi11_build.sh"}
VM_SERVER_CTL_CMD=${VM_SERVER_CTL_CMD:-"./scripts/vxi11_server_ctl.sh"}
VM_SSH_EXTRA_ARGS=${VM_SSH_EXTRA_ARGS:-""}

ssh_base_args=(-p "$VM_SSH_PORT")
if [ -n "${VM_SSH_KEY:-}" ]; then
  ssh_base_args+=(-i "$VM_SSH_KEY")
fi

if [ -n "$VM_SSH_EXTRA_ARGS" ]; then
  # shellcheck disable=SC2206
  extra_args=($VM_SSH_EXTRA_ARGS)
  ssh_base_args+=("${extra_args[@]}")
fi

SSH_WRAPPER=(ssh)
if [ -n "${VM_SSH_PASS:-}" ]; then
  SSH_WRAPPER=(sshpass -p "$VM_SSH_PASS" ssh)
  ssh_args=("${ssh_base_args[@]}" -o BatchMode=no -o ConnectTimeout=5)
else
  ssh_args=("${ssh_base_args[@]}" -o BatchMode=yes -o ConnectTimeout=5)
fi
ssh_args_interactive=("${ssh_base_args[@]}")

ssh_cmd() {
  local cmd=$1
  if [ -n "${VM_SSH_PASS:-}" ] && ! command -v sshpass >/dev/null 2>&1; then
    echo "VM_SSH_PASS is set but sshpass is not installed on the host." >&2
    echo "Install sshpass or use VM_SSH_KEY for key-based auth." >&2
    exit 1
  fi
  "${SSH_WRAPPER[@]}" "${ssh_args[@]}" "$VM_SSH_USER@$VM_SSH_HOST" "bash -lc $(printf '%q' "$cmd")"
}

vm_start() {
  VBoxManage startvm "$VM_NAME" --type "$VM_START_TYPE"
}

vm_stop() {
  case "$VM_STOP_MODE" in
    acpi)
      VBoxManage controlvm "$VM_NAME" acpipowerbutton
      ;;
    poweroff)
      VBoxManage controlvm "$VM_NAME" poweroff
      ;;
    *)
      echo "Unknown VM_STOP_MODE: $VM_STOP_MODE" >&2
      exit 1
      ;;
  esac
}

vm_status() {
  VBoxManage showvminfo "$VM_NAME" --machinereadable | awk -F= '/^VMState=/{gsub(/"/, "", $2); print $2}'
}

wait_for_ssh() {
  local timeout=${1:-60}
  local start_ts
  start_ts=$(date +%s)
  while true; do
    if ssh_cmd "true" >/dev/null 2>&1; then
      echo "SSH ready"
      return 0
    fi
    if [ $(( $(date +%s) - start_ts )) -ge "$timeout" ]; then
      echo "Timed out waiting for SSH" >&2
      return 1
    fi
    sleep 2
  done
}

command=${1:-}
case "$command" in
  start)
    vm_start
    ;;
  stop)
    vm_stop
    ;;
  status)
    vm_status
    ;;
  wait-ssh)
    wait_for_ssh "${2:-60}"
    ;;
  ssh)
    if [ -n "${VM_SSH_PASS:-}" ] && ! command -v sshpass >/dev/null 2>&1; then
      echo "VM_SSH_PASS is set but sshpass is not installed on the host." >&2
      echo "Install sshpass or use VM_SSH_KEY for key-based auth." >&2
      exit 1
    fi
    "${SSH_WRAPPER[@]}" "${ssh_args_interactive[@]}" "$VM_SSH_USER@$VM_SSH_HOST"
    ;;
  build)
    ssh_cmd "cd $(printf '%q' "$VM_REPO_DIR") && $VM_BUILD_CMD"
    ;;
  server-start)
    ssh_cmd "cd $(printf '%q' "$VM_REPO_DIR") && $VM_SERVER_CTL_CMD start"
    ;;
  server-stop)
    ssh_cmd "cd $(printf '%q' "$VM_REPO_DIR") && $VM_SERVER_CTL_CMD stop"
    ;;
  server-status)
    ssh_cmd "cd $(printf '%q' "$VM_REPO_DIR") && $VM_SERVER_CTL_CMD status"
    ;;
  server-restart)
    ssh_cmd "cd $(printf '%q' "$VM_REPO_DIR") && $VM_SERVER_CTL_CMD restart"
    ;;
  *)
    usage
    exit 1
    ;;
esac
