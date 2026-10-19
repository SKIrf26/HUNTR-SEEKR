# HUNTR/SEEKR Watch - Public Beta 1

First public release for the **Waveshare ESP32-C6-Touch-AMOLED-2.06** watch.

**HUNTR/SEEKR** turns the watch into a pocket scanner for the 2.4 GHz band. It listens for the Wi-Fi, Bluetooth, Zigbee/Thread and Drone Remote ID signals that cameras, trackers, smart glasses, drones and similar devices give off, matches them against built-in lists, and lets you follow a signal to its source.

## Highlights
- Passive scanning of Wi-Fi, Bluetooth LE, 802.15.4 and Drone Remote ID
- Wi-Fi and Bluetooth groups can be switched on and off in any order and keep working together
- Grouped results (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI, ZIGBEE/THREAD) with signal strength
- Track screen with a live dBm ring and audible feedback
- **AIM** direction finder: gyro check, 5-point calibration, direction arrow with an uncertainty arc
- Rogue Wi-Fi alerts (de-authentication floods, open twins, name-hungry transmitters) and Bluetooth spam detection
- Wide maker lists including many camera, doorbell and tracker makers, gunshot sensors (Flock Raven, ShotSpotter), police body cameras beyond Axon and Flipper Zero detection, and your own rules (up to 128), plus a signature update file you can upload from your phone
- Tracker "following" alert, possible card-skimmer tags, session report, logs (with KML and GeoJSON map files when a GPS module is fitted), ignore list
- Phone page over the watch's own Wi-Fi (live view, clock sync, firmware update)
- Illustrated user guide included (PDF, normal and booklet), plain-text help and the full source code

## Install
Download `HUNTR-SEEKR_Watch_Public_Beta_1.bin` below and write it at `0x0`:

```
esptool --chip esp32c6 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_Watch_Public_Beta_1.bin
```

This resets saved settings. Updates are done the same way, over USB.

## Files
| File | Purpose |
|---|---|
| `HUNTR-SEEKR_Watch_Public_Beta_1.bin` | Complete image, write at `0x0` |
| `SHA256SUMS.txt` | Checksums |
| `HUNTR-SEEKR_Watch_Public_Beta_1_User_Guide.pdf` (+ two booklet versions) | User guide |

## Please know
This is a **beta**. AIM is the newest and least proven feature (an estimate, never better than +/-15 degrees). The long-press Power screen, sleep and wake, the low-battery rule and the watch face have had less testing. Detection lists are not exhaustive and **results are hints, not proof**.

Use it responsibly and within your local laws. See the README.

Please report problems with the **Bug report** form on the Issues tab, with the Serial Monitor output if you can.

## Thanks
Thanks to Alfa ([@AlfaBeta2000](https://github.com/AlfaBeta2000)), who reported the Watch's slowdowns and spontaneous reboots ([#4](https://github.com/SKIrf26/HUNTR-SEEKR/issues/4)), traced them in the serial monitor to Bluetooth duplicate reports and found the fix that is now in every board's firmware. See [CONTRIBUTORS.md](CONTRIBUTORS.md).
