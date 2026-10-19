# Firmware files

The release `.bin` files are **attached to the GitHub Release**, not committed in this repository (they are large and change every release). To make them yourself from the source, see [../docs/BUILDING.md](../docs/BUILDING.md).

## What is in a release

| File | What it is | When to use it |
|---|---|---|
| `HUNTR-SEEKR_ESP32_LCD35_Port_Test_6.bin` | One complete image for a **4 MB** flash layout (bootloader, partition table, application) | Works on every flasher. Write at address `0x0`. |
| `HUNTR-SEEKR_ESP32_LCD35_Port_Test_6_16MB.bin` | The same, for the board's full **16 MB** flash (two app slots, phone updates, about 9.9 MB storage) | esptool or the Arduino IDE only. Write at address `0x0`. |
| `SHA256SUMS.txt` | Checksums of the files | Check your download is intact |

Writing an image resets saved **settings**; saved logs on a TF card stay. Moving between the 4 MB and 16 MB files also resets the internal storage (the partition table changes).

## Flash the image

1. Install Python 3, then `pip install esptool`.
2. Plug the board into the computer with a USB **data** cable.
3. Find the port (Windows: `COM3` or similar in Device Manager; macOS: `/dev/cu.usbserial-...`; Linux: `/dev/ttyUSB0`).
4. Run:

   ```
   esptool --chip esp32 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_ESP32_LCD35_Port_Test_6.bin
   ```
5. If the board is not found or cannot connect, unplug the cable, HOLD the BOOT button, plug the cable in, release BOOT, and run the command again.
6. After it finishes the board restarts. The boot screen shows **Port Test 6** in small text at the bottom.

Check the download first: on Linux/macOS `shasum -a 256 -c SHA256SUMS.txt`, on Windows PowerShell `Get-FileHash .\HUNTR-SEEKR_ESP32_LCD35_Port_Test_6.bin -Algorithm SHA256`.
