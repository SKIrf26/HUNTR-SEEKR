#!/usr/bin/env python3
# Copyright (C) 2026 SKIrf Labs. Part of HUNTR/SEEKR, free software under the GNU GPL v3 or later (see ../LICENSE).
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Turn an Arduino build folder into the release file(s) and a checksum list.

  python3 make_release_bin.py  <build folder>  [output folder]

<build folder> is the folder that holds  *.ino.bin  and  *.ino.merged.bin
(arduino-cli compile --export-binaries, or Arduino IDE: Sketch > Export Compiled Binary).

Outputs (default folder: ./out):
  HUNTR-SEEKR_S3_LCD147_Public_Beta_1.bin   single image, write at address 0x0
  HUNTR-SEEKR_S3_LCD147_Public_Beta_1_app-only.bin   application only (phone update page)
  SHA256SUMS.txt

Why not just use the *.merged.bin? It is padded with blank bytes to the full flash size, so writing it also
wipes the storage area. The single image made here ends where the application ends.
Pure Python 3, no extra packages.
"""
import glob
import hashlib
import os
import sys

NAME = "HUNTR-SEEKR_S3_LCD147_Public_Beta_1"
APP_ONLY_NAME = NAME + "_app-only.bin"
CHIP = "esp32s3"
BOOT_OFFSET = 0x0  # where the bootloader image (starts with 0xE9) sits in the merged file
PART_OFFSET = 0x8000  # partition table
APP_OFFSET = 0x10000  # application (first app slot)
LAYOUT_HINT = "Board: ESP32S3 Dev Module, Flash Size 16MB, Partition Scheme 16M Flash (3MB APP/9.9MB FATFS)"


def one(folder, pattern):
    hits = glob.glob(os.path.join(folder, pattern))
    if pattern == "*.ino.bin":
        hits = [h for h in hits if not h.endswith(".merged.bin")]
    if len(hits) != 1:
        sys.exit("Expected exactly one file matching %s in %s, found %d" % (pattern, folder, len(hits)))
    return hits[0]


def sha(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    src = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
    with open(one(src, "*.ino.bin"), "rb") as f:
        app = f.read()
    with open(one(src, "*.ino.merged.bin"), "rb") as f:
        merged = f.read()

    # sanity checks: this must be the expected layout
    if merged[BOOT_OFFSET] != 0xE9:
        sys.exit("No bootloader image (0xE9) at 0x%x. Is this a build for the right board? %s" % (BOOT_OFFSET, LAYOUT_HINT))
    if merged[PART_OFFSET:PART_OFFSET + 2] != b"\xaa\x50":
        sys.exit("No partition table found at 0x8000. Wrong partition scheme? %s" % LAYOUT_HINT)
    end = APP_OFFSET + len(app)
    if merged[APP_OFFSET:end] != app:
        sys.exit("The application is not at 0x10000 in the merged file. %s" % LAYOUT_HINT)
    if any(b != 0xFF for b in merged[end:]):
        sys.exit("Unexpected data after the application in the merged file.")

    os.makedirs(out, exist_ok=True)
    full = os.path.join(out, NAME + ".bin")
    with open(full, "wb") as f:
        f.write(merged[:end])
    files = [full]
    app_only = os.path.join(out, APP_ONLY_NAME)
    with open(app_only, "wb") as f:
        f.write(app)
    files.append(app_only)
    sums = os.path.join(out, "SHA256SUMS.txt")
    with open(sums, "w") as f:
        for p in files:
            f.write("%s  %s\n" % (sha(p), os.path.basename(p)))
    files.append(sums)
    print("Wrote:")
    for p in files:
        print("  %s  (%d bytes)" % (p, os.path.getsize(p)))
    print("\nFlash it with:\n  esptool --chip %s --port <PORT> --baud 460800 write-flash 0x0 %s" % (CHIP, os.path.basename(full)))


if __name__ == "__main__":
    main()
