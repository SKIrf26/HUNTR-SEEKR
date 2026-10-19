# HUNTR/SEEKR for the NM-CYD-C5 - Public Beta 1

First public release for the **NM-CYD-C5** board (ESP32-C5, 2.8 inch touch screen).

**HUNTR/SEEKR** turns the board into a pocket scanner. It listens for the Wi-Fi, Bluetooth, Zigbee/Thread and Drone Remote ID signals that cameras, trackers, smart glasses, drones and similar devices give off, matches them against built-in lists, and lets you follow a signal toward its source.

## Highlights
- Passive scanning of Wi-Fi (2.4 GHz or 5 GHz, shown by a band label at the top of the screen), Bluetooth LE, 802.15.4 and Drone Remote ID
- Wi-Fi and Bluetooth groups can be switched on and off in any order and keep working together
- Grouped results (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI, ZIGBEE/THREAD) with signal strength
- Track screen with a live dBm display and audible feedback
- Rogue Wi-Fi alerts (de-authentication floods, open twins, name-hungry transmitters) and Bluetooth spam detection
- Wide maker lists including many camera, doorbell and tracker makers, gunshot sensors (Flock Raven, ShotSpotter), police body cameras beyond Axon and Flipper Zero detection, and your own rules (up to 128) with a `sigs_update.txt` file
- Tracker "following" alert, possible card-skimmer tags, session report, logs (with KML and GeoJSON map files when a GPS module is fitted), ignore list
- Phone page over the board's own Wi-Fi (live view, clock sync)
- **SETUP > Screen colors** corrects a board whose display shows swapped or inverted colours (Normal, Swap R/B, Invert, Swap + invert)
- Illustrated user guide included (PDF, normal and booklet), plain-text help and the full source code

## Install
Download `HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin` below and write it at `0x0`:

```
esptool --chip esp32c5 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin
```

This resets saved settings (you will calibrate the touch screen again); logs on the microSD card stay. Updates are done the same way, over USB.

## Files
| File | Purpose |
|---|---|
| `HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin` | Complete image, write at `0x0` |
| `SHA256SUMS.txt` | Checksum |
| `HUNTR-SEEKR_C5_CYD_Public_Beta_1_User_Guide.pdf` (+ two booklet versions) | User guide |

## Please know
This is a **beta**. Signal strength is only a rough guide to distance, and the board cannot tell which direction a signal comes from. Sound needs a small passive buzzer or piezo on IO26. Detection lists are not exhaustive and **results are hints, not proof**.

Use it responsibly and within your local laws. See the README.

Please report problems with the **Bug report** form on the Issues tab, with the Serial Monitor output if you can.

## Thanks
Thanks to Alfa ([@AlfaBeta2000](https://github.com/AlfaBeta2000)), who reported the Watch's slowdowns and spontaneous reboots ([#4](https://github.com/SKIrf26/HUNTR-SEEKR/issues/4)), traced them in the serial monitor to Bluetooth duplicate reports and found the fix that is now in every board's firmware. See [CONTRIBUTORS.md](CONTRIBUTORS.md).
