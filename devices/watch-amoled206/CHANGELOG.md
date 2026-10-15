# Changelog

All notable changes to this project are listed here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Public Beta 1] - 2026-10-14

First public release, for the **Waveshare ESP32-C6-Touch-AMOLED-2.06** watch board.

### Included
- Passive 2.4 GHz scanning: Wi-Fi, Bluetooth LE, Zigbee/Thread (802.15.4), and Drone Remote ID (Bluetooth, Wi-Fi beacon and Wi-Fi Aware).
- Detection groups (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI, ZIGBEE/THREAD) with built-in maker and name lists, including the camera makers Amcrest, Lorex, Hikvision, Dahua, Axis, Uniview, Vivotek and Hanwha, and many tracker makers.
- Rogue Wi-Fi alerts (DEAUTH, TWIN, KARMA), fixed-address probe listing (PROBE), device fingerprint codes, Bluetooth spam detection.
- Bluetooth share option for running Wi-Fi and Bluetooth groups together.
- Reliable radio start and stop: groups can be armed and disarmed in any order. The Wi-Fi radio is switched off when only Bluetooth is armed, Bluetooth listens quietly while Wi-Fi runs (with a short name-reading scan every 25 seconds), and a radio that does not start is restarted automatically.
- Self-test names the step the watch was in if it restarted unexpectedly.
- Lighter radio handling: what a repeating device matches is remembered, so the long maker lists are walked once rather than for every frame and advert. The screen stays smoother with Wi-Fi and Bluetooth groups armed.
- Custom signatures (oui, ssid, ble, cid, uuid, ie; up to 128 rules) and a signature update file (sigs_update.txt) uploaded from the phone page.
- Finds list with signal strength, Track (HUNT) screen with a dBm ring and audible feedback.
- **AIM**: a gyro check, a 5-point calibration (turn right 90, 180, 270 degrees and back), then a direction arrow with an uncertainty arc. REDO, FLIP and NUM buttons.
- Possible card-skimmer tags (Bluetooth modules still wearing a factory name), Bluetooth type tags, tracker "following" alert.
- Session report, logs (KML and GeoJSON map files when a GPS module is fitted), ignore list, scenes, discreet mode, power saver, watch face.
- Phone page over the watch's own Wi-Fi hotspot (live view, clock sync, firmware update).
- Setup, self-test and touch calibration screens.
- Full illustrated user guide (PDF, normal and two booklet versions) and plain-text help files.
- Full source code, build notes and a code map.

### Known limitations
- AIM is the newest feature and the least proven. It is an estimate, never better than +/-15 degrees.
- Less real-world testing: the long-press Power screen, sleep and wake, the low-battery rule and the watch face.
- The watch listens on 2.4 GHz only (the chip has no 5 GHz Wi-Fi), and Wi-Fi and Bluetooth share one radio.
- Detection lists are not exhaustive. Results are hints, not proof.
