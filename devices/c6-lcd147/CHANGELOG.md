# Changelog

All notable changes to this project are listed here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Public Beta 1] - 2026-10-14

First public release, for the **Waveshare ESP32-C6-Touch-LCD-1.47**.

### Included
- Passive scanning: Wi-Fi (2.4 GHz), Bluetooth LE, Zigbee/Thread (802.15.4), and Drone Remote ID.
- Detection groups (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI, ZIGBEE/THREAD) with built-in maker and name lists, including the camera makers Amcrest, Lorex, Hikvision, Dahua, Axis, Uniview, Vivotek and Hanwha, and many tracker makers.
- Rogue Wi-Fi alerts (DEAUTH, TWIN, KARMA), fixed-address probe listing (PROBE), device fingerprint codes, Bluetooth spam detection.
- Bluetooth share option for running Wi-Fi and Bluetooth groups together.
- Custom signatures (oui, ssid, ble, cid, uuid, ie; up to 128 rules) and a signature update file (sigs_update.txt).
- Finds list with signal strength, Track screen with a live dBm display and audible feedback.
- **AIM**: a gyro check, a 5-point calibration, then a direction arrow with an uncertainty arc. REDO, FLIP and NUM buttons.
- SWEEP page: walkthrough marks, a baseline of a place and a list of what is new.
- Possible card-skimmer tags (Bluetooth modules still wearing a factory name), Bluetooth type tags, tracker "following" alert.
- Session report, logs (KML and GeoJSON map files when a GPS module is fitted), ignore list, scenes, discreet mode, power saver, landscape and portrait screens, touch calibration.
- Phone page over the board's own Wi-Fi hotspot (live view, clock sync).
- Setup and self-test screens.
- Complete, illustrated user guide (PDF, normal and two booklet versions) and plain-text help files.
- Full source code, build notes and a code map.

### Known limitations
- AIM is the newest feature and the least proven. It is an estimate, never better than +/-15 degrees.
- Hardware status: This release builds cleanly for the Waveshare ESP32-C6-Touch-LCD-1.47, and its screens were checked in a desktop simulation of the interface. It has not yet been run on the real board by the project. Please report what you see, good or bad, with the Bug report form.
- The board listens on 2.4 GHz only (the chip has no 5 GHz Wi-Fi), and Wi-Fi and Bluetooth share one radio.
- Updates are done over USB; the phone update page is not supported on this board in this release.
- Detection lists are not exhaustive. Results are hints, not proof.
