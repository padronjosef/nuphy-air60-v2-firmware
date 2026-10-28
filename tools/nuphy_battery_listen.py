#!/usr/bin/env python3
"""Listen for battery reports from a NuPhy Air60 V2 running the battery-report firmware.

The patched firmware sends a consumer report (report ID 3) whose 16-bit usage is
0x0300 | pct while discharging or 0x0400 | pct while charging. This script finds
the dongle hidraw node that carries report ID 3 and prints every battery report.
Read-only: it never writes to the device.
"""

import argparse
import glob
import os
import select
import sys
import time

VENDOR_ID = 0x19F5
PRODUCT_ID = 0x3247
CONSUMER_REPORT_ID = 3
DISCHARGING = 0x0300
CHARGING = 0x0400


def find_hidraw() -> str | None:
    wanted = f"{VENDOR_ID:08X}:{PRODUCT_ID:08X}"
    for node in sorted(glob.glob("/sys/class/hidraw/hidraw*")):
        try:
            with open(os.path.join(node, "device/uevent")) as uevent:
                if wanted not in uevent.read().upper():
                    continue
            with open(os.path.join(node, "device/report_descriptor"), "rb") as desc:
                descriptor = desc.read()
        except OSError:
            continue
        # Consumer page (05 0C) with Report ID 3 (85 03).
        if b"\x05\x0c" in descriptor and bytes([0x85, CONSUMER_REPORT_ID]) in descriptor:
            return "/dev/" + os.path.basename(node)
    return None


def decode(report: bytes) -> tuple[int, bool] | None:
    if len(report) < 3 or report[0] != CONSUMER_REPORT_ID:
        return None
    usage = report[1] | (report[2] << 8)
    kind = usage & 0xFF00
    pct = usage & 0x00FF
    if kind not in (DISCHARGING, CHARGING) or pct > 100:
        return None
    return pct, kind == CHARGING


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--timeout", type=float, default=0, help="stop after N seconds (0 = run forever)")
    args = parser.parse_args()

    path = find_hidraw()
    if path is None:
        print("NuPhy Air60 V2 dongle not found", file=sys.stderr)
        return 1
    print(f"listening on {path}", flush=True)

    deadline = time.monotonic() + args.timeout if args.timeout else None
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        while deadline is None or time.monotonic() < deadline:
            wait = 1.0 if deadline is None else max(0.0, min(1.0, deadline - time.monotonic()))
            ready, _, _ = select.select([fd], [], [], wait)
            if not ready:
                continue
            battery = decode(os.read(fd, 64))
            if battery is not None:
                pct, charging = battery
                state = "charging" if charging else "discharging"
                print(f"{time.strftime('%Y-%m-%dT%H:%M:%S%z')} battery={pct}% {state}", flush=True)
    except KeyboardInterrupt:
        pass
    finally:
        os.close(fd)
    return 0


if __name__ == "__main__":
    sys.exit(main())
