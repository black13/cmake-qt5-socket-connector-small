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
4. Shared folder (host↔guest code):
   ```
   VBoxManage sharedfolder add SCPIEmulator --name hostshare --hostpath /Users/jjosburn/temp/cmake-qt5-socket-connector-small/scpi/share --automount
   ```
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
   sudo mount -t vboxsf hostshare /mnt/hostshare
   sudo systemctl enable ssh
   (install SCPI server that listens on 5025)
   ```
7. Test from macOS using NI-MAX/NIvisaic (connect to TCPIP::localhost::5025::SOCKET).
