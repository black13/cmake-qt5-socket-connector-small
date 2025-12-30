# SCPI Emulator Host (VirtualBox) Notes

## VM Setup via VBoxManage
1. Create VM shell:
   ```
   VBoxManage createvm --name SCPIEmulator --register
   VBoxManage modifyvm SCPIEmulator --memory 4096 --cpus 2 --ostype Ubuntu_64
   ```
2. Create disk and attach storage:
   ```
   VBoxManage createmedium disk --filename scpi/scpi.vdi --size 20480
   VBoxManage storagectl SCPIEmulator --name "SATA Controller" --add sata --controller IntelAHCI
   VBoxManage storageattach SCPIEmulator --storagectl "SATA Controller" --port 0 --device 0 --type hdd --medium scpi/scpi.vdi
   VBoxManage storageattach SCPIEmulator --storagectl "SATA Controller" --port 1 --device 0 --type dvddrive --medium /Users/jjosburn/Downloads/ubuntu-24.04.3-live-server-amd64.iso
   ```
3. Networking with SCPI port forwarding:
   ```
   VBoxManage modifyvm SCPIEmulator --nic1 nat
   VBoxManage modifyvm SCPIEmulator --natpf1 "SCPI,tcp,,5025,,5025"
   ```
4. Shared folder (host↔guest repo root, used by automation):
   ```
   VBoxManage sharedfolder add ubuntu2404lts --name repo --hostpath /Users/jjosburn/temp/cmake-qt5-socket-connector-small --automount
   ```
   The existing `shared` folder can remain; automation expects the repo share.
5. Start VM (GUI for OS install, then headless later):
   ```
   VBoxManage startvm SCPIEmulator
   ```
   After Ubuntu install completes, remove ISO:
   ```
   VBoxManage storageattach SCPIEmulator --storagectl "SATA Controller" --port 1 --device 0 --type dvddrive --medium none
   ```
   Subsequent runs can be headless:
   ```
   VBoxManage startvm SCPIEmulator --type headless
   ```
6. Inside Ubuntu guest:
   ```
   sudo mkdir -p /media/sf_repo
   sudo mount -t vboxsf repo /media/sf_repo
   sudo systemctl enable ssh
   (install SCPI server that listens on 5025)
   ```
7. Test from macOS using NI-MAX/NIvisaic (connect to TCPIP::localhost::5025::SOCKET).

## Automation (start VM, SSH, build, run VXI-11 server)
1. Create `scripts/vm_config.sh` on the host (copy from `scripts/vm_config.example.sh`).
2. Start the VM and wait for SSH:
   ```
   ./scripts/vm_ctl.sh start
   ./scripts/vm_ctl.sh wait-ssh
   ```
3. Build the VXI-11 server inside the VM:
   ```
   ./scripts/vm_ctl.sh build
   ```
4. Start/stop the VXI-11 server:
   ```
   ./scripts/vm_ctl.sh server-start
   ./scripts/vm_ctl.sh server-status
   ./scripts/vm_ctl.sh server-stop
   ```
5. Stop the VM:
   ```
   ./scripts/vm_ctl.sh stop
   ```
   If you use password auth, set `VM_SSH_PASS` in the environment and install `sshpass` on the host.
   If the shared folder is read-only, set `VXI11_BUILD_DIR=$HOME/.cache/vxi11_server` before building.
   For SCPI command coverage and response details, see `scpi/vxi11_server.md`.

## VM prerequisites (inside Ubuntu)
```
sudo apt install -y build-essential libtirpc-dev rpcbind openssh-server
sudo systemctl enable --now rpcbind
sudo systemctl enable --now ssh

sudo mkdir -p /mnt/hostshare
sudo mount -t vboxsf hostshare /mnt/hostshare
```
