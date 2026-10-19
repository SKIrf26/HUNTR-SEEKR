# Changelog

All notable changes are listed here. Each board's folder has its own, more detailed `CHANGELOG.md`.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- New board: **Waveshare ESP32-S3-Touch-LCD-1.47**, in `devices/s3-lcd147/`. It has the same detection as the other boards, plus a Power screen (SLEEP / RESTART) because the board has no power switch. AIM is shown only if the board has a motion sensor; this one normally does not.
- Gunshot-sensor detection: ShotSpotter (SoundThinking), SDS and Flock's Raven sensor are now shown as GUNSHOT and listed under the ALPR group. Raven is recognised from its custom Bluetooth services (two or more together is high confidence, one alone is medium); the older 1.1.x firmware's Health and Location pair is only a low-confidence hint.
- Police body and in-car cameras from makers other than Axon are shown as BODYCAM and listed under the AXON group: Motorola Solutions and WatchGuard (V300, 4RE, VB and SI500 series), Digital Ally (FirstVu), COBAN, Utility, VIEVU, Pro-Vision, ICOP, L3 Mobile-Vision and Zepcam. Most are found by hardware-address block or set-up network name only, so their confidence is lower than Axon's.
- Many more home-camera and doorbell makers, added from the public IEEE address registry (Wyze, Amcrest, Canary, Eagle Eye, Jooan, Swann, Abode, Eufy and others), plus Bluetooth company IDs for Arlo, Hikvision, SimpliSafe, Vivint, Wyze and Anker.
- Flipper Zero detection over Bluetooth: its case-colour service (strong), a "Flipper" name (medium) or only its chip address (low), shown with a FLIPPER type tag.
- New board (in field testing): **Waveshare ESP32-Touch-LCD-3.5**, in `devices/esp32-lcd35/`. The same detection on a 320 x 480 touch screen, with the Track screens and phone page fitted to the larger panel.

### Changed
- Hardware-address rows that are shared with ordinary products now report low confidence, so they read as hints and not firm matches: the Liteon and Silicon Labs modules used by Flock, an Intel module used by Reolink, a WNC module used by Axon, and some Meta-glasses addresses.
- The Groups page cards now name the makers each group covers (ALPR: Flock, Raven, ShotSpotter, Penguin; CAMS: Ring, Wyze, Arlo, Blink, Hikvision, Dahua and more; AXON: the body-camera makers above).

### Fixed
- Spontaneous restarts and slowdowns in busy areas, on every board: Bluetooth now reports each device once per scan cycle instead of every advertisement, which flooded the Bluetooth task (small stack) and starved the rest of the firmware. The scan restarts briefly (every 2 s, every 0.5 s while tracking a Bluetooth device) so signal strength and "still here" keep updating. Found and fixed by Alfa ([@AlfaBeta2000](https://github.com/AlfaBeta2000), [#4](https://github.com/SKIrf26/HUNTR-SEEKR/issues/4)); run on the Watch for more than 5 hours with every group armed and no restart. The other boards have the same change and have not been run with it yet.
- Watch: the Report and Ignored pages scroll, with a scroll bar.

These apply to all boards. Each board's own changelog has the details.

## [Public Beta 1] - 2026-10-15

### Added
- One-button edition for the plain Waveshare ESP32-C6-LCD-1.47 (no touch panel), in `devices/c6-lcd147-button/`: the whole interface runs on the BOOT button (TAP = next, HOLD = select, LONG = back). Built on the current detection code and run on a real board (starts, shows the interface, finds Wi-Fi networks).

### Changed
- Hardware status updated: the Waveshare ESP32-S3-Touch-LCD-1.69 and the ESP32-2432S028 (CYD 2.8) have now been run on real boards by the project and work, including Wi-Fi scanning and Bluetooth. The earlier CYD Bluetooth start-up fault is fixed. Status wording updated in the README, release notes, changelogs and start-here files.

## [Public Beta 1] - 2026-10-14

### Added
- First public release, for five boards: Waveshare ESP32-C6-Touch-AMOLED-2.06, Waveshare ESP32-S3-Touch-LCD-1.69, Waveshare ESP32-C6-Touch-LCD-1.47, NM-CYD-C5 and ESP32-2432S028.
- Full source code, user guide (PDF, normal and two booklet versions), plain-text help, build notes and a code map for each board.
- GNU GPL v3 licence, with third-party notices and licence texts.
