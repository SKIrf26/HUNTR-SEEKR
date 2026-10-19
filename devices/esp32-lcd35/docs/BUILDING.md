# Building from source

The firmware is one Arduino sketch: `src/HUNTR-SEEKR_ESP32_LCD35/HUNTR-SEEKR_ESP32_LCD35.ino`, with two font data files beside it. The folder name must stay the same as the `.ino` name.

## What you need

- Arduino IDE 2.x, or `arduino-cli`
- The **esp32** board package by Espressif Systems, version 3.3.5 or newer (this port was compiled with 3.3.9)
- These libraries:

| Library | Version used | By | Licence | For |
|---|---|---|---|---|
| GFX Library for Arduino | 1.6.9 | moononournation | BSD | Display driver (ST7796) |
| NimBLE-Arduino | 2.3.6 | h2zero | Apache 2.0 | Bluetooth LE scanning |
| TinyGPSPlus | 1.1.0 | Mikal Hart | LGPL 2.1+ | GPS parsing (only used if a GPS module is wired) |

The Wi-Fi, file-system, SD and OTA code come with the board package. No display set-up files are needed (see [../config/README.md](../config/README.md)).

## Build in the Arduino IDE

1. Install the board package and the libraries above.
2. Open `src/HUNTR-SEEKR_ESP32_LCD35/HUNTR-SEEKR_ESP32_LCD35.ino`. The three files in that folder must stay together.
3. Choose **Tools** settings:
   - Board: "ESP32 Dev Module"
   - Flash Size: "16MB (128Mb)" (or "4MB (32Mb)")
   - Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)" on 16 MB, or "Huge APP (3MB No OTA/1MB SPIFFS)" on 4 MB
   - **PSRAM: "Enabled"** (the board has 2 MB; the firmware keeps its large tables there. Without it the tables use the normal memory and Bluetooth may not start)
   - CPU Frequency: "240MHz (WiFi/BT)"
4. Press **Upload**. The first build takes a few minutes. If the upload does not start, HOLD the BOOT button, plug the cable in, release BOOT, and upload again.
5. Open **Tools > Serial Monitor** at 115200 baud to see the start-up messages. The boot screen ends with `ready  Port Test 6`.

If you pick a chip other than the classic ESP32, the sketch stops at build time with a message that names the right board.

## Build with arduino-cli

```
arduino-cli compile --fqbn espressif:esp32:esp32:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=enabled,CPUFreq=240,FlashMode=qio --export-binaries src/HUNTR-SEEKR_ESP32_LCD35
arduino-cli upload  --fqbn espressif:esp32:esp32:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=enabled,CPUFreq=240,FlashMode=qio --port YOUR_PORT src/HUNTR-SEEKR_ESP32_LCD35
```

For the 4 MB layout use `FlashSize=4M,PartitionScheme=huge_app`. The compiled files are written to `src/HUNTR-SEEKR_ESP32_LCD35/build/espressif.esp32.esp32/`.

## Make the release image

`arduino-cli` and the Arduino IDE write a merged image padded with blank bytes to the full flash size, which would also wipe the storage area when written. `tools/make_release_bin.py` cuts it to the part that matters and writes the checksum list:

```
python3 tools/make_release_bin.py src/HUNTR-SEEKR_ESP32_LCD35/build/espressif.esp32.esp32 out
```

It writes `HUNTR-SEEKR_ESP32_LCD35_Port_Test_6.bin` and `SHA256SUMS.txt` into `out/`, after checking the layout (bootloader at 0x1000, partition table at 0x8000 and application at 0x10000).

## Memory notes

The classic ESP32 has about 124 KB for static data, and the Wi-Fi and Bluetooth stacks need most of the rest, so the large tables (log ledger, view list, Bluetooth caches and similar) are allocated at start-up, in PSRAM when it is enabled. The Serial Monitor prints one line at start (`psram: in use ...`) with the free memory.
