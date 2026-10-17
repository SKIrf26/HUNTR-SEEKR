# HUNTR/SEEKR for the Waveshare ESP32-S3-Touch-LCD-1.47 - Public Beta 1

First public release for the **Waveshare ESP32-S3-Touch-LCD-1.47**.

**HUNTR/SEEKR** turns the board into a pocket scanner. It listens for the Wi-Fi, Bluetooth and Drone Remote ID signals that cameras, trackers, smart glasses, drones and similar devices give off, matches them against built-in lists, and lets you follow a signal toward its source.

## Highlights
- Passive scanning of Wi-Fi, Bluetooth LE, and Drone Remote ID
- Wi-Fi and Bluetooth groups can be switched on and off in any order and keep working together
- Grouped results (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI) with signal strength
- Track screen with a live dBm display and audible feedback (optional buzzer)
- Wide maker lists including many camera, doorbell and tracker makers, gunshot sensors (Flock Raven, ShotSpotter), police body cameras beyond Axon and Flipper Zero detection, and your own rules (up to 128), plus a signature update file
- Rogue Wi-Fi alerts (de-authentication floods, open twins, name-hungry transmitters) and Bluetooth spam detection
- Tracker "following" alert, possible card-skimmer tags, session report, logs, ignore list
- SETUP > Power: SLEEP and RESTART, since the board has no power switch
- AIM direction finder, shown only if the board has a motion sensor (this board normally does not)
- Phone page over the board's own Wi-Fi (live view, clock sync, firmware update)
- Plain-text help and the full source code

## Install
Download `HUNTR-SEEKR_S3_LCD147_Public_Beta_1.bin` below and write it at `0x0`:

```
esptool --chip esp32s3 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_S3_LCD147_Public_Beta_1.bin
```

This resets saved settings; saved logs stay.
Already running a beta? Use `HUNTR-SEEKR_S3_LCD147_Public_Beta_1_app-only.bin` on the board's phone update page.

## Files
| File | Purpose |
|---|---|
| `HUNTR-SEEKR_S3_LCD147_Public_Beta_1.bin` | Complete image, write at `0x0` |
| `HUNTR-SEEKR_S3_LCD147_Public_Beta_1_app-only.bin` | Application only, for phone/OTA update |
| `SHA256SUMS.txt` | Checksums |

## Please know
This is a **beta**. This release has been flashed and run on a real Waveshare ESP32-S3-Touch-LCD-1.47 by the project. Not every feature has been tried one by one on the board yet, so some may have had less testing on hardware. There is no illustrated user guide for this board yet.

Use it responsibly and within your local laws. See the README.
