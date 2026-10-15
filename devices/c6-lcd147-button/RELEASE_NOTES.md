# HUNTR/SEEKR for the Waveshare ESP32-C6-LCD-1.47 (one-button edition) - Public Beta 1

First public release of the **one-button edition**, for the plain **Waveshare ESP32-C6-LCD-1.47** (no touch panel).

**HUNTR/SEEKR** turns the board into a pocket scanner. It listens for the Wi-Fi, Bluetooth, Zigbee/Thread and Drone Remote ID signals that cameras, trackers, smart glasses, drones and similar devices give off, matches them against built-in lists, and lets you follow a signal toward its source. The whole interface works with the board's **BOOT** button: **TAP** = next, **HOLD** = select, **LONG** = back.

## Highlights
- One-button interface with a hint strip on every screen and the RGB LED as the status light
- Scenes (STREET, AIRSPACE, PERSONAL, ZB/TH, ALL OFF) and a group-by-group picker
- Passive scanning of Wi-Fi, Bluetooth LE, 802.15.4 and Drone Remote ID, with the current detection lists and signatures
- Rogue Wi-Fi alerts, Bluetooth spam detection, tracker "following" alert, possible card-skimmer tags
- Finds list and a Track screen with a live dBm ring; the LED blinks faster as you get closer
- Findings are saved automatically and can be downloaded from the phone page
- Phone page over the board's own Wi-Fi (live view, clock sync, your own signatures, boot logo, log download)
- Optional buzzer and up to two optional extra buttons
- Illustrated user guide included (PDF, normal and booklet), plain-text help and the full source code

## Install
Download `HUNTR-SEEKR_C6_LCD147_BUTTON_Public_Beta_1.bin` below and write it at `0x0`:

```
esptool --chip esp32c6 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_C6_LCD147_BUTTON_Public_Beta_1.bin
```

This resets saved settings; saved logs stay. Updates are done the same way, over USB.

## Files
| File | Purpose |
|---|---|
| `HUNTR-SEEKR_C6_LCD147_BUTTON_Public_Beta_1.bin` | Complete image, write at `0x0` |
| `SHA256SUMS.txt` | Checksums |
| `HUNTR-SEEKR_C6_LCD147_BUTTON_Public_Beta_1_User_Guide.pdf` (+ two booklet versions) | User guide |

## Please know
This is a **beta**. The software was rebuilt on the current detection code for this release and its interface was checked in a desktop simulation, but this release has not yet been run on a real board. An earlier one-button build of the same interface (version 3.6-btn) has been run on a real plain ESP32-C6-LCD-1.47 by the project and works there, including the display settings, the LED and reading BOOT as the button. Please report what you see, good or bad, with the Bug report form.

This edition has no direction finder (AIM), landscape screen, settings pages, session report or self-test: those need a touch screen or a motion sensor. Detection lists are not exhaustive and **results are hints, not proof**.

Use it responsibly and within your local laws. See the README.

Please report problems with the **Bug report** form on the Issues tab, with the Serial Monitor output if you can.
