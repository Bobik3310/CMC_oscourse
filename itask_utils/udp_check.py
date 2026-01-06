#!/usr/bin/env python3
import socket
import argparse

def main():
    p = argparse.ArgumentParser(description="Simple UDP receiver")
    p.add_argument("--bind", default="0.0.0.0", help="IP to bind (default: all interfaces)")
    p.add_argument("--port", type=int, default=1234, help="UDP port to listen on")
    p.add_argument("--buf", type=int, default=4096, help="recv buffer size")
    args = p.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((args.bind, args.port))

    print(f"[+] Listening on UDP {args.bind}:{args.port} ... (Ctrl+C to stop)")
    while True:
        data, addr = s.recvfrom(args.buf)
        # show both raw and decoded (best-effort)
        print(f"\n[+] From {addr[0]}:{addr[1]}  ({len(data)} bytes)")
        print(f"    hex: {data.hex(' ')}")
        try:
            print(f"    txt: {data.decode('utf-8', errors='replace')!r}")
        except Exception as e:
            print(f"    txt: <decode error: {e}>")

if __name__ == "__main__":
    main()

