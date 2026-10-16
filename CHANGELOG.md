# Changelog

All notable changes are listed here. Each board's folder has its own, more detailed `CHANGELOG.md`.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

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
