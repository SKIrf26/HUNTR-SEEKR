<div align="center">

# HUNTR/SEEKR

**Counter-surveillance signal scanner for the NM-CYD-C5 ("cheap yellow display", ESP32-C5)**

![Status](https://img.shields.io/badge/status-Public%20Beta%201-orange)
![Board](https://img.shields.io/badge/board-NM--CYD--C5-blue)
![Licence](https://img.shields.io/badge/licence-GPL%20v3-green)

![The scanner: groups, finds, track and report screens](docs/images/overview.png)

<sub>Screen pictures are renderings of the interface produced by a simulator, not photographs of the board.</sub>

</div>

## What it is

HUNTR/SEEKR turns the NM-CYD-C5 board (ESP32-C5, 2.8 inch touchscreen) into a pocket radio scanner. It **listens** for the wireless signals that certain kinds of devices give off, compares them with built-in lists of known makers and names, and shows you what it heard and how strong each signal is. A Track screen lets you walk toward a signal.

It is passive: it only receives. It does not jam, attack, or connect to anyone's network.

> **Results are hints, not proof.** Names and addresses can be changed or randomized, many devices are not in the lists, and a quiet device may simply be out of range. "Nothing found" never proves nothing is there.

## What it can find

- **Wi-Fi** devices such as security cameras, doorbells, smart glasses, drones and license-plate readers, from their network names and hardware addresses, on both the **2.4 GHz and 5 GHz** bands
- **Bluetooth LE** devices such as trackers (AirTag and other Find My trackers, Tile, SmartTag, Google Find Hub, Chipolo, Pebblebee, TrackR, Jiobit), body cameras, smart glasses, drones, beacons, wearables, smart lights and Fast Pair accessories, each with a type tag
- **Zigbee and Thread** smart-home radios (802.15.4)
- **Drone Remote ID**: the serial number and position many drones broadcast
- **Rogue Wi-Fi signs**: a de-authentication flood (DEAUTH), an open copy of a protected network (TWIN), and a device that answers to any network name (KARMA). These are warnings, not proof.
- **Devices asking for a network**: fixed-address devices looking for a named Wi-Fi network (PROBE), and camera-style network names that low-cost hidden-camera modules often use (listed quietly as low confidence)
- **Possible card skimmers**: Bluetooth modules still wearing a factory name (the kind sometimes hidden in gas pumps and ATM readers). This is a clue, not a verdict.

## Features

- Detection groups with live counts: ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI, ZIGBEE/THREAD
- **Track** screen: a live signal-strength (dBm) display so you can walk toward a source
- Tracker "following" alert, rogue Wi-Fi alert, 5 GHz scan switch, ignore list, scenes, custom signatures (your own `/sigs` rules plus an optional `sigs_update.txt` update file on the card), session report and logs (saved to a microSD card or the board's flash)
- Discreet mode, power saver, landscape and portrait screens, touch calibration
- Phone page over the board's own Wi-Fi hotspot: live view and clock sync
- Self-test screen that checks every part of the hardware
- Optional extras: a small passive buzzer for sound, a GPS module, a microSD card

<table>
<tr>
<td><img src="docs/images/arm.png" width="230" alt="Groups"></td>
<td><img src="docs/images/finds.png" width="230" alt="Finds"></td>
<td><img src="docs/images/hunt.png" width="230" alt="Track"></td>
<td><img src="docs/images/report.png" width="230" alt="Report"></td>
</tr>
<tr align="center"><td>Groups</td><td>Finds</td><td>Track</td><td>Report</td></tr>
</table>

## Install

You need an **NM-CYD-C5** board and a USB-C **data** cable. Not sure where to start? Read [READ_ME_FIRST.txt](READ_ME_FIRST.txt) and the [help folder](help/).

### Flash the firmware file

1. Download `HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin` and `SHA256SUMS.txt` from the [Releases page](../../releases).
2. Install Python 3, then `pip install esptool`.
3. Plug in the board and run:

   ```
   esptool --chip esp32c5 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin
   ```
4. The board restarts. The boot screen shows **Public Beta 1** at the bottom. The first start asks you to calibrate the touch screen: press and hold each of the four crosses.

Writing the image resets saved settings; logs on the microSD card stay. Step-by-step help, including checking the download, is in [help/2_FLASHING.txt](help/2_FLASHING.txt).

## Documentation

The illustrated user guide covers set-up, every screen, the settings, troubleshooting and care:

- [User guide (PDF)](docs/HUNTR-SEEKR_C5_CYD_Public_Beta_1_User_Guide.pdf)
- Booklet versions for printing two-sided: [flip on long edge](docs/HUNTR-SEEKR_C5_CYD_Public_Beta_1_User_Guide_BOOKLET_flip-long-edge.pdf) / [flip on short edge](docs/HUNTR-SEEKR_C5_CYD_Public_Beta_1_User_Guide_BOOKLET_flip-short-edge.pdf)
- Plain-text help: [help/](help/) (getting started, flashing, troubleshooting, FAQ)

## Beta status - what to expect

This is the **first public release**. It is a beta: expect rough edges, and expect some behaviour to change.

- Signal strength is only a rough guide to distance, and one antenna cannot tell which direction a signal comes from. Use the warmer / colder reading and walk around.
- Sound needs a small **passive** buzzer or piezo fitted to the board (see the user guide).
- Update over USB: the phone update page is not supported on this board in this release.
- The detection lists are not exhaustive.

Found a problem? Open an issue with the **Bug report** form and include what the Serial Monitor (115200 baud) printed.

## Responsible use

This project is meant for defensive, privacy-protecting use: finding out what is around you. Receiving radio signals that are openly broadcast is lawful in many places, but laws differ, and some places restrict particular uses of scanners or of the information you collect. You are responsible for following the laws where you are. Do not use it to identify, follow or harass individual people. A match is a clue about a device, not evidence about a person or a place.

## Licence

HUNTR/SEEKR is free software released under the **GNU General Public License v3.0** - see [LICENSE](LICENSE). It comes with **no warranty**.

It is built with third-party components under their own licences (LGPL, BSD, MIT, SIL OFL): see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

RockBase, NMTech, Espressif, Arduino and all brand and product names that appear in the detection lists or screenshots belong to their owners. This project is not affiliated with, or endorsed by, any of them.

## Project files

| Path | Contents |
|---|---|
| `help/` | Plain-text help: getting started, flashing, troubleshooting, FAQ |
| `firmware/` | How the release `.bin` is flashed and checked |
| `docs/` | User guide PDFs, images, optional browser installer page |
| `licenses/` | Licence texts of the third-party components |
| `CHANGELOG.md`, `RELEASE_NOTES.md` | What is in each release |
