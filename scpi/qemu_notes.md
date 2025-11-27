# SCPI Emulator Host (QEMU) Notes

## QEMU Status (macOS host)
- Installed via MacPorts (`sudo port install qemu`).
- `qemu-system-x86_64 --version` reports **QEMU emulator version 10.1.2**.

## Requirements Checklist
- [x] QEMU installed on macOS host.
- [x] QCOW2 disk `scpi.img` (20 GB) created via `qemu-img` for the guest OS.
- [x] Ubuntu Server ISO downloaded (`ubuntu-24.04.3-live-server-amd64.iso` currently in ~/Downloads; optionally move to `scpi/iso/`).
- [ ] QEMU launch command with ISO + shared folder (`scpi/share`) + port forwarding (host 5025 → guest 5025).
- [ ] After OS install, mount shared folder inside guest (`sudo mount -t 9p -o trans=virtio hostshare /mnt/hostshare`).
- [ ] SCPI emulator service inside guest (Python/Go/C++) that listens on TCP 5025, logs commands, and responds to `*IDN?`, `*RST`, `:MEAS?`, etc.
- [ ] NI-VISA (macOS) configured to talk to `TCPIP::localhost::5025::SOCKET` so NI-MAX/NI IO Trace can see the emulator.
- [ ] Plan to port the same SCPI server to Raspberry Pi hardware along with NI-488.2 or Ethernet gateway to real instruments.

## Next Actions
1. Create QCOW2 disk + install minimal Linux in QEMU (console only is fine). Mount the host share via virtio-9p:
   - Host command additions: `-fsdev local,id=fsdev0,path=/Users/jjosburn/temp/cmake-qt5-socket-connector-small/scpi/share,security_model=passthrough -device virtio-9p-pci,fsdev=fsdev0,mount_tag=hostshare`
   - Guest mount: `sudo mount -t 9p -o trans=virtio hostshare /mnt/hostshare`
2. Configure port forwarding for SCPI (host 5025 → guest 5025).
3. Implement a simple SCPI server inside the guest (Python script that prints/logs all commands, responds to `*IDN?`). Store sources in the shared folder so both guest and host see them.
4. Verify NIvisaic/NI-MAX (macOS) can connect and issue `*IDN?` over TCP/IP.
5. Expand emulator to simulate signal generator, receiver, spectrum analyzer, and power meter behavior (e.g., scripted responses, state machines).
6. Once stable, port the same emulator to Raspberry Pi hardware or integrate with real instruments.

## NI-VISA (macOS) Installation
1. Go to https://www.ni.com/en-us/support/downloads/drivers/download.ni-visa.html
2. Select the latest NI-VISA version that lists macOS 12.x support.
3. Download and run the DMG installer (includes NI Package Manager + NI-VISA).
4. After installation, verify `visaic` launches and can add a TCPIP instrument (e.g., `TCPIP::localhost::5025::SOCKET`).
5. NodeGraph scripts can then link against `/Library/Frameworks/VISA.framework` or use NI’s C API to talk to the SCPI emulator.
