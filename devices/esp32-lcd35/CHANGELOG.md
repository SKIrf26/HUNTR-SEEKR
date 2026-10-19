# Changelog

All notable changes to this project are listed here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Port Test 6]

### Fixed
- Bluetooth reports each device once per scan cycle and the scan restarts every 2 s (0.5 s while tracking), which follows the fix found by Alfa ([@AlfaBeta2000](https://github.com/AlfaBeta2000), [#4](https://github.com/SKIrf26/HUNTR-SEEKR/issues/4)) for spontaneous restarts.
- Radios (Wi-Fi / Bluetooth) not coming back after using the phone page (Share): the page handlers were added again on every Share, Bluetooth could be switched off for good if it was started while the hotspot held memory, and a failed restart of the radios was never retried. Handlers are now added once, Bluetooth is retried after a low-memory skip, a failed Wi-Fi start is retried automatically, and the serial log prints free memory when Share starts, stops and when the radios are back.
- SETUP > Self-test > Radios row: says "paused while sharing" (green) during Share, "starting again..." or "Wi-Fi driver would not start" (red) when something is really wrong, and "(BLE off)" when only Wi-Fi is scanning.

## [Port Test 5]

### Changed
- Boot logo / boot sound upload: if the SD card refuses a file the card is mounted again and the write retried; the phone page now says which storage was used and whether the card passed a write test. Uploads no longer ask a big SD card for its free space (slow).
- SETUP > Self-test > SD card row shows the card type, size and whether it can be written (red if not).
- Track screens: the gauge or scope, the digits and the text now grow to the size of the 320 x 480 / 480 x 320 panel, and a signal history graph fills the space that used to be empty (portrait and landscape).
- SETUP > Self-test: the Motion row says "no sensor on this board" (green).

## [Port Test 2]

### Changed
- Faster screen: strips of the picture that did not change are no longer sent to the display; the page-slide animation is off on this board (the new page appears at once).
- More free memory: the second strip buffer is only made when the screen is turned; custom rules are kept in PSRAM.
- SETUP > Self-test has a new **SD card** row (card found or not, type, size, whether logs go there). The Memory row shows the lowest free memory seen and the PSRAM left; the Speed row shows the share of strips skipped.

## [Port Test 1]

First build for the **Waveshare ESP32-Touch-LCD-3.5**. Not yet tried on the real board.

### Included
- The current HUNTR/SEEKR scanner: Wi-Fi (2.4 GHz), Bluetooth LE and Drone Remote ID detection, eight groups, rogue Wi-Fi alerts, Bluetooth spam detection, gunshot-sensor, body-camera and Flipper Zero detection, custom rules, session report, logs, phone page.
- Board support: ST7796S display with PWM backlight, FT6336 touch (polled), TCA9554 I/O expander (display and touch resets, speaker amplifier), AXP2101 power chip, PCF85063 clock, ES8311 audio codec, TF card (used first for logs when present).
- Bluetooth scanning through a queue: the Bluetooth task only copies each advert and the main loop decodes it. Each device is reported once per scan cycle and the scan restarts every 2 s (every 0.5 s while tracking a Bluetooth device) so signal strength stays live.
- The Report and Hidden-list screens scroll with a finger, with a scroll bar.
- Large tables are kept in the board's PSRAM when it is enabled.

### Not on this board
- Zigbee/Thread (the chip has no 802.15.4 radio) and 5 GHz Wi-Fi.
- AIM direction finding (no motion sensor).
