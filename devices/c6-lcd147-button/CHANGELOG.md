# Changelog

All notable changes to this project are listed here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Gunshot-sensor detection: ShotSpotter (SoundThinking), SDS and Flock's Raven sensor are now shown as GUNSHOT and listed under the ALPR group. Raven is recognised from its custom Bluetooth services (two or more together is high confidence, one alone is medium); the older 1.1.x firmware's Health and Location pair is only a low-confidence hint.
- Police body and in-car cameras from makers other than Axon are shown as BODYCAM and listed under the AXON group: Motorola Solutions and WatchGuard (V300, 4RE, VB and SI500 series), Digital Ally (FirstVu), COBAN, Utility, VIEVU, Pro-Vision, ICOP, L3 Mobile-Vision and Zepcam. Most are found by hardware-address block or set-up network name only, so their confidence is lower than Axon's.
- Many more home-camera and doorbell makers, added from the public IEEE address registry (Wyze, Amcrest, Canary, Eagle Eye, Jooan, Swann, Abode, Eufy and others), plus Bluetooth company IDs for Arlo, Hikvision, SimpliSafe, Vivint, Wyze and Anker.
- Flipper Zero detection over Bluetooth: its case-colour service (strong), a "Flipper" name (medium) or only its chip address (low), shown with a FLIPPER type tag.

### Changed
- Hardware-address rows that are shared with ordinary products now report low confidence, so they read as hints and not firm matches: the Liteon and Silicon Labs modules used by Flock, an Intel module used by Reolink, a WNC module used by Axon, and some Meta-glasses addresses.
- The Groups page cards now name the makers each group covers (ALPR: Flock, Raven, ShotSpotter, Penguin; CAMS: Ring, Wyze, Arlo, Blink, Hikvision, Dahua and more; AXON: the body-camera makers above).

## [Public Beta 1] - 2026-10-15

First public release of the one-button edition, for the plain **Waveshare ESP32-C6-LCD-1.47** (no touch panel).

### Included
- One-button interface on BOOT: TAP = next, HOLD = select, LONG = back; a hint strip on every screen that lights the gesture being made; the RGB LED as the status light (listening, find, tracking, hold boundaries).
- Home, Scene, Groups, More, Finds and Track screens, the find banner, and the phone page (Share) screen.
- Scenes (STREET, AIRSPACE, PERSONAL, ZB/TH, ALL OFF) and a group-by-group picker; the armed set comes back after a restart.
- The current detection code: Wi-Fi (2.4 GHz), Bluetooth LE, Zigbee/Thread (802.15.4) and Drone Remote ID; detection lists and signatures, rogue Wi-Fi alerts (DEAUTH, TWIN, KARMA), Bluetooth spam detection, Bluetooth skimmer alert, tracker "following" alert.
- Track screen with a live dBm ring, HOTTER/COLDER cue and history graph; TAP switches between the zoomed ring and the full range (or sound on/off when a buzzer is fitted).
- Findings are saved automatically (every 5 minutes and when scanning stops) and can be downloaded from the phone page.
- Phone page over the board's own Wi-Fi hotspot (live view, clock sync, custom signatures and signature update file, boot logo, log download). The hotspot name changes each time.
- Brightness levels, LED on/off and Discreet mode in More; choices remembered.
- Optional extra buttons (`PIN_BTN_EXT1`, `PIN_BTN_EXT2`) and an optional buzzer (`PIN_BUZZER`).
- Flash layout: the 4 MB "Huge APP" layout, so the image starts after flashers rewrite the flash size. No update slot: update over USB.
- Complete, illustrated user guide (PDF, normal and two booklet versions) and plain-text help files.
- Full source code, build notes and a code map.

### Known limitations
- Hardware status: This release has been flashed and run on a real plain ESP32-C6-LCD-1.47 by the project: it starts, shows the interface and finds Wi-Fi networks. An earlier one-button build of the same interface (version 3.6-btn) also ran on that board, including the display settings, the LED and reading BOOT as the button. Not every feature has been tried one by one on this release yet (Bluetooth, Zigbee/Thread, the phone page and the saved logs in particular). Please report what you see, good or bad, with the Bug report form.
- No direction finder (AIM), landscape screen, settings pages, ignore-list editing, session report or self-test: these need a touch screen or a motion sensor.
- No sound without a buzzer, no battery gauge, no clock chip (the clock is set from the phone page).
- The board listens on 2.4 GHz only, and Wi-Fi and Bluetooth share one radio.
- Updates are done over USB; the phone update page is not supported on this board.
- Detection lists are not exhaustive. Results are hints, not proof.
