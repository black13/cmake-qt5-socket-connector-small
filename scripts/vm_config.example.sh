#!/usr/bin/env bash
set -euo pipefail

# VirtualBox VM settings for VXI-11 development.
# Create scripts/vm_config.sh with your real values.

VM_NAME="SCPIEmulator"
VM_START_TYPE="headless"   # headless or gui
VM_STOP_MODE="acpi"        # acpi or poweroff

# SSH connection settings.
VM_SSH_HOST="127.0.0.1"    # Use host-only IP if you prefer.
VM_SSH_PORT="2222"         # Forwarded SSH port or 22 for host-only.
VM_SSH_USER="ubuntu"
# VM_SSH_KEY="$HOME/.ssh/id_ed25519"
# VM_SSH_EXTRA_ARGS="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"
# VM_SSH_PASS="gubble"      # Optional; requires sshpass on the host (avoid committing).

# Repo path inside the VM.
VM_REPO_DIR="/mnt/hostshare"

# Remote commands (run inside VM_REPO_DIR).
VM_BUILD_CMD="./scripts/vxi11_build.sh"
VM_SERVER_CTL_CMD="./scripts/vxi11_server_ctl.sh"
