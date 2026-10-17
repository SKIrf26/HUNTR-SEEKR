# HUNTR/SEEKR for the Waveshare ESP32-C6-Touch-LCD-1.47 - Public Beta 1

First public release for the **Waveshare ESP32-C6-Touch-LCD-1.47**.

**HUNTR/SEEKR** turns the board into a pocket scanner. It listens for the Wi-Fi, Bluetooth, Zigbee/Thread and Drone Remote ID signals that cameras, trackers, smart glasses, drones and similar devices give off, matches them against built-in lists, and lets you follow a signal toward its source.

## Highlights
- Passive scanning of Wi-Fi, Bluetooth LE, 802.15.4 and Drone Remote ID
- Wi-Fi and Bluetooth groups can be switched on and off in any order and keep working together
- Grouped results (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI, ZIGBEE/THREAD) with signal strength
- Track screen with a live dBm display and audible feedback
- **AIM** direction finder: gyro check, 5-point calibration, direction arrow with an uncertainty arc
- Rogue Wi-Fi alerts (de-authentication floods, open twins, name-hungry transmitters) and Bluetooth spam detection
- Wide maker lists including many camera, doorbell and tracker makers, gunshot sensors (Flock Raven, ShotSpotter), police body cameras beyond Axon and Flipper Zero detection, and your own rules (up to 128), plus a signature update file
- Tracker "following" alert, possible card-skimmer tags, session report, logs (a GPS module adds your position to the log), ignore list
- Phone page over the board's own Wi-Fi (live view, clock sync)
- Illustrated user guide included (PDF, normal and booklet), plain-text help and the full source code

## Install
Download `HUNTR-SEEKR_C6_LCD147_Public_Beta_1.bin` below and write it at `0x0`:

```
esptool --chip esp32c6 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_C6_LCD147_Public_Beta_1.bin
```

This resets saved settings; saved logs stay.
Updates are done the same way, over USB.

## Files
| File | Purpose |
|---|---|
| `HUNTR-SEEKR_C6_LCD147_Public_Beta_1.bin` | Complete image, write at `0x0` |
| `SHA256SUMS.txt` | Checksums |
| `HUNTR-SEEKR_C6_LCD147_Public_Beta_1_User_Guide.pdf` (+ two booklet versions) | User guide |

## Please know
This is a **beta**. AIM is the newest and least proven feature (an estimate, never better than +/-15 degrees). This release has been run on the real board by the project (start-up, scanning and the Track screen); the other screens were checked in a desktop simulation of the interface, so some features have had less testing on hardware. Please report what you see, good or bad, with the Bug report form. Detection lists are not exhaustive and **results are hints, not proof**.

Use it responsibly and within your local laws. See the README.

Please report problems with the **Bug report** form on the Issues tab, with the Serial Monitor output if you can.
