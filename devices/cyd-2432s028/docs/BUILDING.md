# Building from source

The firmware is one Arduino sketch: `src/HUNTR-SEEKR_CYD_2432S028/HUNTR-SEEKR_CYD_2432S028.ino`, with two font data files beside it. The folder name must stay the same as the `.ino` name.

## What you need

- Arduino IDE 2.x, or `arduino-cli`
- The **esp32** board package by Espressif Systems, version 3.3.5 or newer (this release was built with 3.3.9)
- These libraries:

| Library | Version used | By | Licence | For |
|---|---|---|---|---|
| TFT_eSPI | 2.5.44 | Bodmer | BSD 2-clause (FreeBSD) | Display driver |
| NimBLE-Arduino | 2.3.6 | h2zero | Apache 2.0 | Bluetooth LE scanning |
| TinyGPSPlus | 1.1.0 | Mikal Hart | LGPL 2.1+ | GPS parsing (only used if a GPS module is wired) |

The Bluetooth stack (NimBLE) and the Wi-Fi, file-system and OTA code come with the board package.

## Display library (required)

TFT_eSPI reads its pins and display driver from a file inside the library. Follow [../config/README.md](../config/README.md): install TFT_eSPI from the Library Manager (the normal release is fine), copy `config/User_Setup.h` over the library's `User_Setup.h`, and restart the Arduino IDE. The sketch checks this at build time and stops with a message that says what is wrong, instead of producing a black or white screen. (To skip the check, add `#define SKI_SKIP_TFT_CHECK` at the top of the sketch.)

## Build in the Arduino IDE

1. Install the board package and the libraries above.
2. Open `src/HUNTR-SEEKR_CYD_2432S028/HUNTR-SEEKR_CYD_2432S028.ino`. The three files in that folder must stay together.
3. Choose **Tools** settings:
   - Board: "ESP32 Dev Module"
   - Flash Size: "4MB (32Mb)"
   - Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
   - PSRAM: "Disabled"
   - CPU Frequency: "240MHz (WiFi/BT)"
4. Press **Upload**. The first build takes a few minutes. If the upload does not start, unplug the cable, HOLD the board's BOOT button (on the back), plug the cable in, release BOOT, and upload again.
5. Open **Tools > Serial Monitor** at 115200 baud to see the start-up messages. The boot screen ends with `ready  Public Beta 1`.

If you pick the wrong board, the sketch stops at build time with a message that names the right one.

## Build with arduino-cli

```
arduino-cli compile --fqbn espressif:esp32:esp32:FlashSize=4M,PartitionScheme=huge_app,PSRAM=disabled,CPUFreq=240,FlashMode=qio --export-binaries src/HUNTR-SEEKR_CYD_2432S028
arduino-cli upload  --fqbn espressif:esp32:esp32:FlashSize=4M,PartitionScheme=huge_app,PSRAM=disabled,CPUFreq=240,FlashMode=qio --port YOUR_PORT src/HUNTR-SEEKR_CYD_2432S028
```

The compiled files are written to `src/HUNTR-SEEKR_CYD_2432S028/build/espressif.esp32.esp32/`.

## Make the release image

`arduino-cli` and the Arduino IDE write a merged image padded with blank bytes to the full flash size, which would also wipe the storage area when written. `tools/make_release_bin.py` cuts it to the part that matters and writes the checksum list:

```
python3 tools/make_release_bin.py src/HUNTR-SEEKR_CYD_2432S028/build/espressif.esp32.esp32 out
```

It writes `HUNTR-SEEKR_CYD_2432S028_Public_Beta_1.bin` and `SHA256SUMS.txt` into `out/`, after checking the layout (bootloader, partition table and application at their expected addresses).

## Where things are

[CODE_MAP.md](CODE_MAP.md) lists every section of the sketch with its line number. The options that are meant to be changed are `#define`s near the top of the sketch, each with a comment.
