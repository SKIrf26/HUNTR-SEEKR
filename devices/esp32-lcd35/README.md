<div align="center">

# HUNTR/SEEKR

**Counter-surveillance signal scanner for the Waveshare ESP32-Touch-LCD-3.5**

![Status](https://img.shields.io/badge/status-Port%20Test%201-orange)
![Board](https://img.shields.io/badge/board-Waveshare%20ESP32--Touch--LCD--3.5-blue)
![Licence](https://img.shields.io/badge/licence-GPL%20v3-green)

</div>

> **Experimental port.** This is the current HUNTR/SEEKR firmware adapted to this board. It compiles, but it has **not yet been run on the
> real hardware**. Expect things that need adjusting (touch direction, screen corners, sound). Reports are welcome.

## What it is

HUNTR/SEEKR turns the Waveshare ESP32-Touch-LCD-3.5 (320 x 480 rounded-corner touch screen) into a radio scanner. It **listens** on the 2.4 GHz band for the wireless signals that certain kinds of devices give off, compares them with built-in lists of known makers and names, and shows you what it heard, how strong each signal is, and lets you follow a signal toward its source.

It is passive: it only receives. It does not jam, attack, or connect to anyone's network.

> **Results are hints, not proof.** Names and addresses can be changed or randomized, many devices are not in the lists, and a quiet device may simply be out of range. "Nothing found" never proves nothing is there.

## What it can find

- **Wi-Fi** devices such as security cameras, doorbells, smart glasses, drones and license-plate readers, from their network names and hardware addresses
- **Rogue Wi-Fi signs**: de-authentication floods, open "evil twin" copies of a protected network name, and one transmitter answering to many names - flagged as warnings, not proof
- **Bluetooth LE** devices such as trackers (AirTag and other Find My trackers, Tile, SmartTag, Google Find Hub, Chipolo, Pebblebee), body cameras, smart glasses, Flipper Zero devices, drones, beacons, wearables, smart lights and Fast Pair accessories, each with a type tag
- **Gunshot sensors and police cameras**: ShotSpotter (SoundThinking) and Flock Raven sensors (shown as GUNSHOT), and body or in-car cameras from makers other than Axon, such as Motorola Solutions and WatchGuard, Digital Ally, COBAN and Utility (shown as BODYCAM). Most are matched by address block or set-up network name, so confidence is lower than for Axon.
- **Drone Remote ID**: the serial number and position many drones broadcast, over Bluetooth, Wi-Fi beacons and Wi-Fi Aware
- **Possible card skimmers**: Bluetooth modules still wearing a factory name (the kind sometimes hidden in gas pumps and ATM readers). This is a clue, not a verdict.

## Features

- Eight detection groups with live counts: ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI
- Groups can be armed and disarmed in any order; Wi-Fi and Bluetooth groups keep working together, with a **Bluetooth share** option for when Bluetooth finds are slow
- **Track** screen: a live signal-strength (dBm) ring with audible feedback so you can walk toward a source
- Wide maker lists, including many camera, doorbell and tracker makers, gunshot sensors, device-fingerprint codes and Bluetooth spam detection
- Custom signatures (oui, ssid, ble, cid, uuid and ie rules) and a signature update file you can upload from your phone
- Tracker "following" alert, ignore list, session report with chart, and logs (saved to a TF card when one is fitted)
- Discreet mode, power saver, clock face, side-button controls, battery level
- Phone page over the board's own Wi-Fi hotspot: live view, clock sync, boot-sound upload
- Self-test screen that checks the hardware

**Differences from the other boards:** eight groups (the chip has no 802.15.4 radio, so no Zigbee/Thread), 2.4 GHz only, and no AIM direction feature (the board has no motion sensor).

## Install

You need the **Waveshare ESP32-Touch-LCD-3.5** board and a USB-C **data** cable. Not sure where to start? Read [READ_ME_FIRST.txt](READ_ME_FIRST.txt) and the [help folder](help/).

### Flash the firmware file

1. Download `HUNTR-SEEKR_ESP32_LCD35_Port_Test_6.bin` (4 MB layout) or `..._16MB.bin` (the board's full flash) and `SHA256SUMS.txt` from the [Releases page](https://github.com/SKIrf26/HUNTR-SEEKR/releases).
2. Install Python 3, then `pip install esptool`.
3. Plug in the board and run:

   ```
   esptool --chip esp32 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_ESP32_LCD35_Port_Test_6.bin
   ```
4. The board restarts. The boot screen shows **Port Test 6** at the bottom.

Writing the image resets saved settings; logs on a TF card stay. Step-by-step help, including checking the download, is in [help/2_FLASHING.txt](help/2_FLASHING.txt) and [firmware/README.md](firmware/README.md).

### Build it yourself

The complete source is in [`src/`](src/). [docs/BUILDING.md](docs/BUILDING.md) lists the libraries, the Arduino IDE settings (including **PSRAM: Enabled**) and the command-line build; the pin map and the few settings you may want to change are in [`config/`](config/).

## Status - what to expect

- **Hardware status.** Not yet tried on the board. Please report what you see, good or bad, with the Bug report form and the Serial Monitor output (115200 baud).
- Signal strength is only a rough guide to distance, and one antenna cannot tell which direction a signal comes from. Use the warmer / colder reading and walk around.
- The board listens on the 2.4 GHz band only. A device on 5 GHz Wi-Fi is not heard.
- The board has one radio shared by Wi-Fi and Bluetooth, so with both armed each hears a little less than it would alone. Turn on SETUP > Bluetooth share if Bluetooth finds are slow.
- GPS is off in the ready-made file (the header pins are not part of the published pin map); set `PIN_GPS_RX` / `PIN_GPS_TX` and build it yourself to use one.
- The detection lists are not exhaustive.

## Responsible use

This project is meant for defensive, privacy-protecting use: finding out what is around you. Receiving radio signals that are openly broadcast is lawful in many places, but laws differ, and some places restrict particular uses of scanners or of the information you collect. You are responsible for following the laws where you are. Do not use it to identify, follow or harass individual people. A match is a clue about a device, not evidence about a person or a place.

## Credits

Thanks to Alfa ([@AlfaBeta2000](https://github.com/AlfaBeta2000)), who reported the Watch's slowdowns and spontaneous reboots ([#4](https://github.com/SKIrf26/HUNTR-SEEKR/issues/4)), traced them in the serial monitor to Bluetooth duplicate reports and found the fix that is now in every board's firmware. See [CONTRIBUTORS.md](CONTRIBUTORS.md).

## Licence

HUNTR/SEEKR is free software released under the **GNU General Public License v3.0** - see [LICENSE](LICENSE). It comes with **no warranty**. Copyright (C) 2026 SKIrf Labs.

It is built with third-party components under their own licences (LGPL, Apache 2.0, BSD, SIL OFL): see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Waveshare, Espressif, Arduino and all brand and product names that appear in the detection lists belong to their owners. This project is not affiliated with, or endorsed by, any of them.

## Project files

| Path | Contents |
|---|---|
| `src/` | The firmware source (Arduino sketch) |
| `config/` | Pin map and the few settings you may want to change |
| `help/` | Plain-text help: getting started, flashing, troubleshooting, FAQ |
| `firmware/` | How the release `.bin` is flashed and checked |
| `docs/` | Build notes |
| `tools/` | Script that turns a build into the release `.bin` and checksum |
| `licenses/` | Licence texts of the third-party components |
| `CHANGELOG.md`, `RELEASE_NOTES.md` | What is in each release |
