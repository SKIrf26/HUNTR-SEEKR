# Changelog

All notable changes to this project are listed here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Gunshot-sensor detection: ShotSpotter (SoundThinking), SDS and Flock's Raven sensor are now shown as GUNSHOT and listed under the ALPR group. Raven is recognised from its custom Bluetooth services (two or more together is high confidence, one alone is medium); the older 1.1.x firmware's Health and Location pair is only a low-confidence hint.
- Police body and in-car cameras from makers other than Axon are shown as BODYCAM and listed under the AXON group: Motorola Solutions and WatchGuard (V300, 4RE, VB and SI500 series), Digital Ally (FirstVu), COBAN, Utility, VIEVU, Pro-Vision, ICOP, L3 Mobile-Vision and Zepcam. Most are found by hardware-address block or set-up network name only, so their confidence is lower than Axon's.
- Many more home-camera and doorbell makers, added from the public IEEE address registry (Wyze, Amcrest, Canary, Eagle Eye, Jooan, Swann, Abode, Eufy and others), plus Bluetooth company IDs for Arlo, Hikvision, SimpliSafe, Vivint, Wyze and Anker.
- Flipper Zero detection over Bluetooth: its case-colour service (strong), a "Flipper" name (medium) or only its chip address (low), shown with a FLIPPER type tag.
- **SETUP > Screen colors**: corrects a board whose display shows the wrong colours. It steps through Normal, Swap R/B (red and blue traded), Invert (photo negative) and Swap + invert; the picture changes at once and the choice is kept.

### Fixed
- Bluetooth scanning failed to start ("Bluetooth failed to start") with Arduino core 3.3.9, because the core freed the Bluetooth memory at start-up. The sketch now tells the core that it uses Bluetooth LE.

### Changed
- Hardware-address rows that are shared with ordinary products now report low confidence, so they read as hints and not firm matches: the Liteon and Silicon Labs modules used by Flock, an Intel module used by Reolink, a WNC module used by Axon, and some Meta-glasses addresses.
- The Groups page cards now name the makers each group covers (ALPR: Flock, Raven, ShotSpotter, Penguin; CAMS: Ring, Wyze, Arlo, Blink, Hikvision, Dahua and more; AXON: the body-camera makers above).

## [Public Beta 1] - 2026-10-14

First public release, for the **ESP32-2432S028 (CYD 2.8)**.

### Included
- Passive scanning: Wi-Fi (2.4 GHz), Bluetooth LE, and Drone Remote ID.
- Detection groups (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI) with built-in maker and name lists, including the camera makers Amcrest, Lorex, Hikvision, Dahua, Axis, Uniview, Vivotek and Hanwha, and many tracker makers.
- Rogue Wi-Fi alerts (DEAUTH, TWIN, KARMA), fixed-address probe listing (PROBE), device fingerprint codes, Bluetooth spam detection.
- Bluetooth share option for running Wi-Fi and Bluetooth groups together.
- Custom signatures (oui, ssid, ble, cid, uuid, ie; up to 128 rules) and a signature update file (sigs_update.txt).
- Finds list with signal strength, Track screen with a live dBm display and audible feedback.
- Possible card-skimmer tags (Bluetooth modules still wearing a factory name), Bluetooth type tags, tracker "following" alert.
- Session report, logs (a GPS module adds your position to the log), ignore list, scenes, discreet mode, power saver, landscape and portrait screens, touch calibration.
- Phone page over the board's own Wi-Fi hotspot (live view, clock sync).
- Setup and self-test screens.
- Complete, illustrated user guide (PDF, normal and two booklet versions) and plain-text help files.
- Full source code, build notes and a code map.

### Known limitations
- Hardware status: This release has been run on the real board by the project and works, including Wi-Fi scanning and Bluetooth. A Bluetooth start-up fault found on an earlier build is fixed. Not every feature has been tried one by one on the board yet, so some may have had less testing on hardware. Please report what you see, good or bad, with the Bug report form.
- The board listens on 2.4 GHz only (the chip has no 5 GHz Wi-Fi), and Wi-Fi and Bluetooth share one radio.
- Updates are done over USB; the phone update page is not supported on this board in this release.
- Detection lists are not exhaustive. Results are hints, not proof.
