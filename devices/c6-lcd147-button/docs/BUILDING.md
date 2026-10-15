# Building from source

The firmware is one Arduino sketch: `src/HUNTR-SEEKR_C6_LCD147_BUTTON/HUNTR-SEEKR_C6_LCD147_BUTTON.ino`, with two font data files beside it. The folder name must stay the same as the `.ino` name.

## What you need

- Arduino IDE 2.x, or `arduino-cli`
- The **esp32** board package by Espressif Systems, version 3.3.5 or newer (this release was built with 3.3.9)
- These libraries:

| Library | Version used | By | Licence | For |
|---|---|---|---|---|
| GFX Library for Arduino | 1.6.9 | moononournation | BSD (Adafruit GFX based) | Display driver (ST7789) |
| TinyGPSPlus | 1.1.0 | Mikal Hart | LGPL 2.1+ | GPS parsing (only used if a GPS module is wired) |

The Bluetooth stack and the Wi-Fi, file-system and OTA code come with the board package.

## Display library

The display is driven by GFX Library for Arduino, which is configured entirely from the sketch: there is no `User_Setup.h` to edit. The board definitions (pins, screen) are in [../config/README.md](../config/README.md) and at the top of the sketch.

## Build in the Arduino IDE

1. Install the board package and the libraries above.
2. Open `src/HUNTR-SEEKR_C6_LCD147_BUTTON/HUNTR-SEEKR_C6_LCD147_BUTTON.ino`. The three files in that folder must stay together.
3. Choose **Tools** settings:
   - Board: "ESP32C6 Dev Module"
   - USB CDC On Boot: "Enabled"
   - Flash Size: "4MB (32Mb)"
   - Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
4. Press **Upload**. The first build takes a few minutes. If the upload does not start, unplug the cable, HOLD the board's BOOT button, plug the cable in, release BOOT, and upload again.
5. Open **Tools > Serial Monitor** at 115200 baud to see the start-up messages. The boot screen ends with `ready  Public Beta 1`.
The boards with native USB need **USB CDC On Boot: Enabled** to print to the Serial Monitor.

If you pick the wrong board, the sketch stops at build time with a message that names the right one.

## Build with arduino-cli

```
arduino-cli compile --fqbn espressif:esp32:esp32c6:CDCOnBoot=cdc,FlashSize=4M,PartitionScheme=huge_app --export-binaries src/HUNTR-SEEKR_C6_LCD147_BUTTON
arduino-cli upload  --fqbn espressif:esp32:esp32c6:CDCOnBoot=cdc,FlashSize=4M,PartitionScheme=huge_app --port YOUR_PORT src/HUNTR-SEEKR_C6_LCD147_BUTTON
```

The compiled files are written to `src/HUNTR-SEEKR_C6_LCD147_BUTTON/build/espressif.esp32.esp32c6/`.

## Make the release image

`arduino-cli` and the Arduino IDE write a merged image padded with blank bytes to the full flash size, which would also wipe the storage area when written. `tools/make_release_bin.py` cuts it to the part that matters and writes the checksum list:

```
python3 tools/make_release_bin.py src/HUNTR-SEEKR_C6_LCD147_BUTTON/build/espressif.esp32.esp32c6 out
```

It writes `HUNTR-SEEKR_C6_LCD147_BUTTON_Public_Beta_1.bin` and `SHA256SUMS.txt` into `out/`, after checking the layout (bootloader, partition table and application at their expected addresses).

## Where things are

[CODE_MAP.md](CODE_MAP.md) lists every section of the sketch with its line number. The options that are meant to be changed are `#define`s near the top of the sketch, each with a comment.
