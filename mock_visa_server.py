#!/usr/bin/env python3
"""
Mock VISA SCPI Server for protocol investigation.
Logs all traffic in hex + ASCII to understand what NI VISA expects.

Usage:
    python mock_visa_server.py

Then in NI MAX, add: TCPIP::127.0.0.1::5025::SOCKET
"""
import socket
import datetime

PORT = 5025
TERMINATOR = b"\n"  # Try: b"\n", b"\r\n", b""

def hexdump(data, prefix=""):
    """Print hex + ASCII dump of data."""
    if not data:
        return
    hex_part = data.hex(" ")
    ascii_part = "".join(chr(b) if 32 <= b < 127 else "." for b in data)
    print(f"{prefix}[{len(data):3d} bytes] HEX: {hex_part}")
    print(f"{prefix}            ASCII: {ascii_part!r}")

def timestamp():
    return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]

def handle_command(cmd):
    """
    Handle SCPI commands. Returns response bytes or None.
    """
    cmd_upper = cmd.upper().strip()

    # Common SCPI commands
    responses = {
        b"*IDN?": b"MockInstrument,Model1,SN001,1.0",
        b"*OPC?": b"1",
        b"*STB?": b"0",
        b"*ESR?": b"0",
        b"*TST?": b"0",
    }

    # Commands that don't return anything
    no_response = [b"*RST", b"*CLS", b"*OPC", b"*WAI"]

    for pattern, response in responses.items():
        if pattern in cmd_upper:
            return response + TERMINATOR

    for pattern in no_response:
        if pattern in cmd_upper:
            print(f"  [{timestamp()}] Command acknowledged (no response): {cmd_upper}")
            return None

    # Unknown query - return something
    if b"?" in cmd:
        print(f"  [{timestamp()}] UNKNOWN QUERY - returning 0")
        return b"0" + TERMINATOR

    # Unknown command - no response
    print(f"  [{timestamp()}] UNKNOWN COMMAND (no response)")
    return None

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(('0.0.0.0', PORT))
    s.listen(1)

    print("=" * 60)
    print(f"Mock VISA Server - Port {PORT}")
    print(f"Terminator: {TERMINATOR!r}")
    print("=" * 60)
    print(f"Add in NI MAX: TCPIP::127.0.0.1::{PORT}::SOCKET")
    print("=" * 60)
    print()

    conn_count = 0
    while True:
        conn, addr = s.accept()
        conn_count += 1
        print(f"\n[{timestamp()}] === CONNECTION #{conn_count} from {addr} ===")

        msg_count = 0
        try:
            while True:
                data = conn.recv(4096)
                if not data:
                    print(f"[{timestamp()}] Client closed connection (empty recv)")
                    break

                msg_count += 1
                print(f"\n[{timestamp()}] RECV #{msg_count}:")
                hexdump(data, "  ")

                # Handle the command
                response = handle_command(data)
                if response:
                    print(f"[{timestamp()}] SEND:")
                    hexdump(response, "  ")
                    conn.send(response)

        except ConnectionResetError:
            print(f"[{timestamp()}] Connection reset by client")
        except Exception as e:
            print(f"[{timestamp()}] Error: {e}")
        finally:
            conn.close()
            print(f"[{timestamp()}] === CONNECTION #{conn_count} CLOSED ({msg_count} messages) ===\n")

if __name__ == "__main__":
    main()
