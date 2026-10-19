# Changelog

All notable changes to this project are listed here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Fixed
- Spontaneous restarts and slowdowns in busy areas: Bluetooth now reports each device once per scan cycle instead of every advertisement, which flooded the Bluetooth task (small stack). The scan restarts briefly (every 2 s, every 0.5 s while tracking a Bluetooth device) so signal strength and "still here" keep updating. Found and fixed by Alfa ([@AlfaBeta2000](https://github.com/AlfaBeta2000), [#4](https://github.com/SKIrf26/HUNTR-SEEKR/issues/4)). The same change is in this board's firmware; it has not been run on this board yet.

## [Public Beta 1] - 2026-10-17

First public release, for the **Waveshare ESP32-S3-Touch-LCD-1.47**.

### Included
- Passive scanning: Wi-Fi (2.4 GHz), Bluetooth LE, and Drone Remote ID.
- Detection groups (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI) with built-in maker and name lists, including the camera makers Amcrest, Lorex, Hikvision, Dahua, Axis, Uniview, Vivotek and Hanwha, home-camera and doorbell makers, and many tracker makers.
- Gunshot-sensor detection: ShotSpotter (SoundThinking), SDS and Flock's Raven sensor are shown as GUNSHOT and listed under the ALPR group. Raven is recognised from its custom Bluetooth services (two or more together is high confidence, one alone is medium).
- Police body and in-car cameras from makers other than Axon are shown as BODYCAM and listed under the AXON group: Motorola Solutions and WatchGuard, Digital Ally, COBAN, Utility, VIEVU, Pro-Vision, ICOP, L3 Mobile-Vision and Zepcam. Most are found by hardware-address block or set-up network name only, so their confidence is lower than Axon's.
- Flipper Zero detection over Bluetooth, shown with a FLIPPER type tag.
- Rogue Wi-Fi alerts (DEAUTH, TWIN, KARMA), fixed-address probe listing (PROBE), device fingerprint codes, Bluetooth spam detection.
- Bluetooth share option for running Wi-Fi and Bluetooth groups together.
- Custom signatures (oui, ssid, ble, cid, uuid, ie; up to 128 rules) and a signature update file (sigs_update.txt).
- Finds list with signal strength, Track screen with a live dBm display and audible feedback (optional buzzer on IO4).
- **AIM**: a direction arrow with an uncertainty arc, after a 5-point calibration. It needs a motion sensor (QMI8658), which Waveshare does not list for this board: the sketch looks for one on the touch bus at start-up and only shows the AIM button if one answers.
- SWEEP page: walkthrough marks, a baseline of a place and a list of what is new.
- Possible card-skimmer tags (Bluetooth modules still wearing a factory name), Bluetooth type tags, tracker "following" alert.
- **SETUP > Power**: SLEEP (deep sleep, the closest this board gets to off) and RESTART, each with a 2.5 second countdown that any touch cancels. Holding BOOT for about 2 seconds also sleeps the board; a short BOOT press wakes a dimmed screen. An empty battery (under 3.30 V, not on USB) sleeps by itself.
- Battery readings on IO12 ignore the failed reads the Wi-Fi radio can cause on that pin.
- Session report, logs, ignore list, scenes, discreet mode, power saver, landscape and portrait screens, touch calibration.
- Phone page over the board's own Wi-Fi hotspot (live view, clock sync, firmware update).
- Setup and self-test screens.
- Plain-text help files, full source code, build notes and a code map.

### Known limitations
- AIM is the newest feature and the least proven. It is an estimate, never better than +/-15 degrees, and only works if the board has a motion sensor.
- Hardware status: this release has been flashed and run on a real Waveshare ESP32-S3-Touch-LCD-1.47 by the project. Not every feature has been tried one by one on the board yet, so some may have had less testing on hardware. Please report what you see, good or bad, with the Bug report form.
- There is no illustrated user guide for this board yet; the guide for the ESP32-S3-Touch-LCD-1.69 describes the same screens.
- The board listens on 2.4 GHz only (the chip has no 5 GHz Wi-Fi), and Wi-Fi and Bluetooth share one radio.
- Detection lists are not exhaustive. Results are hints, not proof.
