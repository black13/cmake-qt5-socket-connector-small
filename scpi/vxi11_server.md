# VXI-11 Spectrum Analyzer Server

This repo includes a minimal VXI-11 server that emulates a PSA-class spectrum analyzer over ONC/RPC.
It is intentionally lightweight so we can iterate on SCPI behavior without pulling in external libraries.

## Build (Linux VM)
```
./scripts/vxi11_build.sh
```
If the shared folder is read-only, the build falls back to `~/.cache/vxi11_server`.

## Run
```
./scripts/vxi11_server_ctl.sh start
./scripts/vxi11_server_ctl.sh status
```

Ensure `rpcbind` is running before the server starts:
```
sudo systemctl enable --now rpcbind
rpcinfo -p localhost | grep 0607A
```

## Quick Test
```
sudo apt install -y lxi-tools
lxi scpi -a 127.0.0.1 "*IDN?"
```

Expected response:
```
AGILENT,PSA-N9030A,SGNL0001,5.00
```

## Supported SCPI (current)
- `*IDN?`
- `*RST`
- `:SYST:ERR?`
- `:SENSe:FREQuency:CENTer` / `:SENSe:FREQuency:CENTer?`
- `:SENSe:FREQuency:SPAN` / `:SENSe:FREQuency:SPAN?`
- `:SENSe:BANDwidth:RESolution` / `:SENSe:BANDwidth:RESolution?`
- `:DISPlay:WINDow:TRACe:Y:SCALe:RLEVel` / `:DISPlay:WINDow:TRACe:Y:SCALe:RLEVel?`
- `:INIT`
- `:TRACe:DATA?` (returns CSV trace data)

Unknown commands return `ERROR,"Unknown command"` and queue `-113,"Undefined header"` for `:SYST:ERR?`.
