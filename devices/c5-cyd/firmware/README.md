# Firmware files

The release `.bin` files are **attached to the GitHub Release**, not committed in this repository (they are large and change every release). To make them yourself from the source, see [../docs/BUILDING.md](../docs/BUILDING.md).

## What is in a release

| File | What it is | When to use it |
|---|---|---|
| `HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin` | One complete image (bootloader, partition table, application) | First install, or a clean re-flash. Write at address `0x0`. Resets saved **settings**; saved **logs** stay. |
| `SHA256SUMS.txt` | Checksums of the file | Check your download is intact |

## Flash the image

1. Install Python 3, then `pip install esptool`.
2. Plug the board into the computer with a USB **data** cable.
3. Find the port (Windows: `COM3` or similar in Device Manager; macOS: `/dev/cu.usbmodem...`; Linux: `/dev/ttyACM0` or `/dev/ttyUSB0`).
4. Run:

   ```
   esptool --chip esp32c5 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin
   ```
5. If the board is not found or the upload cannot connect, unplug the cable, HOLD the board's BOOT button, plug the cable in, release BOOT, and run the command again (this puts the chip in download mode).
6. After it finishes the board restarts. The boot screen shows **Public Beta 1** in small text at the bottom.

The board has more than one USB-C port. Use the **left** one (the USB-UART port): the computer can put the chip into flashing mode by itself there. The middle port is the chip's own USB port and needs the BOOT + RESET sequence by hand.

Check the download first: on Linux/macOS `shasum -a 256 -c SHA256SUMS.txt`, on Windows PowerShell `Get-FileHash .\HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin -Algorithm SHA256`.

## Browser flasher

The project's [web flasher](https://skirf-labs.tech/) writes the same image from Google Chrome or Microsoft Edge over USB, with nothing to install.
