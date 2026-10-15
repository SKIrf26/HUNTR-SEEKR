# Changelog

All notable changes to this project are listed here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Public Beta 1] - 2026-10-14

First public release, for the **NM-CYD-C5** (ESP32-C5, 2.8 inch touch screen) board.

### Included
- Passive scanning: Wi-Fi (2.4 GHz or 5 GHz, chosen in SETUP), Bluetooth LE, Zigbee/Thread (802.15.4), and Drone Remote ID (Bluetooth, Wi-Fi beacon and Wi-Fi Aware).
- Detection groups (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI, ZIGBEE/THREAD) with built-in maker and name lists, including the camera makers Amcrest, Lorex, Hikvision, Dahua, Axis, Uniview, Vivotek and Hanwha, and many tracker makers.
- Rogue Wi-Fi alerts (DEAUTH, TWIN, KARMA), fixed-address probe listing (PROBE), device fingerprint codes, Bluetooth spam detection.
- Bluetooth share option for running Wi-Fi and Bluetooth groups together.
- Wi-Fi band label (2.4 GHz or 5 GHz) at the top of the main pages, beside the status pill, following the SETUP choice.
- Reliable radio start and stop: groups can be armed and disarmed in any order. The Wi-Fi radio is switched off when only Bluetooth is armed, Bluetooth listens quietly while Wi-Fi runs (with a short name-reading scan every 25 seconds), and a radio that does not start is restarted automatically.
- Self-test shows Radio timing (first Wi-Fi and Bluetooth signal, restarts) and Last start (power on, or the step in progress after a crash or reset).
- Custom signatures (oui, ssid, ble, cid, uuid, ie; up to 128 rules) and a `sigs_update.txt` file read after them.
- Finds list with signal strength and a Track screen with a live dBm display and audible feedback (optional passive buzzer); Bluetooth tracking keeps its sound steady.
- Possible card-skimmer tags (Bluetooth modules still wearing a factory name), Bluetooth type tags, tracker "following" alert.
- Session report, logs (microSD card or flash; KML and GeoJSON map files when a GPS module is fitted), ignore list, scenes, discreet mode, power saver, landscape and portrait screens.
- Four-point touch calibration on first start and from SETUP.
- Phone page over the board's own Wi-Fi hotspot (live view, clock sync).
- Setup and self-test screens.
- Optional GPS module and microSD card support.
- Full illustrated user guide (PDF, normal and two booklet versions) and plain-text help files.
- Full source code, build notes and a code map.

### Known limitations
- Signal strength is a rough guide to distance; the board has one antenna and cannot tell which direction a signal comes from.
- The Wi-Fi sweep listens on one band at a time, and 5 GHz channels that need a radar check (52 to 144) are not scanned.
- The phone update page is not supported on this board in this release: update over USB.
- Detection lists are not exhaustive. Results are hints, not proof.
