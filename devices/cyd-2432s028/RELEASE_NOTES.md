# HUNTR/SEEKR for the ESP32-2432S028 (CYD 2.8) - Public Beta 1

First public release for the **ESP32-2432S028 (CYD 2.8)**.

**HUNTR/SEEKR** turns the board into a pocket scanner. It listens for the Wi-Fi, Bluetooth and Drone Remote ID signals that cameras, trackers, smart glasses, drones and similar devices give off, matches them against built-in lists, and lets you follow a signal toward its source.

## Highlights
- Passive scanning of Wi-Fi, Bluetooth LE, and Drone Remote ID
- Wi-Fi and Bluetooth groups can be switched on and off in any order and keep working together
- Grouped results (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI) with signal strength
- Track screen with a live dBm display and audible feedback
- Rogue Wi-Fi alerts (de-authentication floods, open twins, name-hungry transmitters) and Bluetooth spam detection
- Wide maker lists including many camera and tracker makers, and your own rules (up to 128), plus a signature update file
- Tracker "following" alert, possible card-skimmer tags, session report, logs (with KML and GeoJSON map files when a GPS module is fitted), ignore list
- Phone page over the board's own Wi-Fi (live view, clock sync)
- Illustrated user guide included (PDF, normal and booklet), plain-text help and the full source code

## Install
Download `HUNTR-SEEKR_CYD_2432S028_Public_Beta_1.bin` below and write it at `0x0`:

```
esptool --chip esp32 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_CYD_2432S028_Public_Beta_1.bin
```

This resets saved settings (you will calibrate the touch screen again); logs on the microSD card stay.
Updates are done the same way, over USB.

## Files
| File | Purpose |
|---|---|
| `HUNTR-SEEKR_CYD_2432S028_Public_Beta_1.bin` | Complete image, write at `0x0` |
| `SHA256SUMS.txt` | Checksums |
| `HUNTR-SEEKR_CYD_2432S028_Public_Beta_1_User_Guide.pdf` (+ two booklet versions) | User guide |

## Please know
This is a **beta**. Signal strength is only a rough guide to distance, and the board cannot tell which direction a signal comes from. This release builds cleanly for the ESP32-2432S028 (CYD 2.8), and its screens were checked in a desktop simulation of the interface. It has not yet been run on the real board by the project. Please report what you see, good or bad, with the Bug report form. Detection lists are not exhaustive and **results are hints, not proof**.

Use it responsibly and within your local laws. See the README.

Please report problems with the **Bug report** form on the Issues tab, with the Serial Monitor output if you can.
