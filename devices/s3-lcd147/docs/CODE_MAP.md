# Code map

`src/HUNTR-SEEKR_S3_LCD147/HUNTR-SEEKR_S3_LCD147.ino` is a single Arduino sketch of about 18,730 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 73 | Terminal theme: palette, fonts, helpers |
| 88 | Pins (Waveshare ESP32-S3-Touch-LCD-1.47: schematic netlist + official Arduino demo) |
| 151 | What the shared interface code needs to know about this board |
| 196 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 331 | Forward declarations |
| 390 | Panel (JD9853 through the ST7789 class) |
| 393 | Strip renderer state |
| 495 | Palette |
| 579 | Primitives (integer only) |
| 754 | Terminal theme: panel shape and the CRT pass |
| 972 | Text (anti-aliased) |
| 1173 | vendor blocks from the IEEE registry (camera, doorbell and gunshot-sensor makers) |
| 1374 | police body and in-car camera makers (besides Axon) |
| 1555 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1735 | Helpers |
| 2083 | Log queue (filled from radio callbacks, drained in loop) |
| 2096 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2287 | Backlight (PWM, gamma 2) + display sleep |
| 2308 | GPS + log writer (loop context only) |
| 2431 | Detection callbacks |
| 2434 | Bluetooth / Wi-Fi device recognition |
| 7012 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 7113 | Sweep tools (walkthrough marks, baseline-then-sweep) and hunt upgrades |
| 7768 | 802.15.4 (Zigbee + Thread) receive path |
| 7995 | Radio control |
| 8413 | Share portal state |
| 8418 | Share portal (WPA2) |
| 8581 | Storage / sessions |
| 8658 | Preferences |
| 8757 | I2C helpers |
| 8783 | Touch controller (AXS5106L: reg 0x01, 14 bytes; same as Waveshare's driver) |
| 8963 | Sleep and restart (the board has no power switch) |
| 9088 | Battery (IO12 through a 200k/100k divider: VBAT = VADC x 3). No power-management chip on this board. |
| 9362 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 9485 | Core logic (no screen state in here) |
| 9488 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 9606 | Session statistics + 60-minute timeline |
| 9680 | Real-time clock (PCF85063 @0x51) |
| 9800 | Custom signatures (/sigs.txt: one rule per line) |
| 9964 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 10023 | What the alert is about (captured under the mux by upsert) |
| 10034 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 10065 | Tracker "following you": seen continuously for followMin minutes |
| 10093 | Scenes (one tap arms a useful set of groups) |
| 10110 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 10117 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 10129 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 10387 | Findings record (what was found this session, saved on demand) |
| 10643 | Boot sound (WATCH): your own short sound on the boot screen |
| 10878 | Boot logo (all boards): your own picture on the boot screen |
| 11224 | UI |
| 11379 | Pill (status; tap = pause / resume) + battery |
| 11491 | Bottom bars |
| 11623 | Tiles (ARM page) |
| 11729 | Find list (FINDS page + pushed filtered list) |
| 11993 | Pushed, filtered list (long-press a tile) |
| 12002 | Hunt |
| 12005 | Terminal theme: the motion-tracker view of Track |
| 12207 | HUD theme: the blue HUD tracker of the Track screen |
| 12526 | Shared bits for text pages |
| 12587 | LOG page |
| 12588 | Groups panel: each group, live status, and what it searches for |
| 12839 | SETUP page |
| 13059 | Power screen: two large buttons, so sleeping or restarting is easy on a small screen |
| 13093 | Help |
| 13152 | Storage (internal flash) |
| 13284 | Share portal |
| 13308 | Boot: radar ping + wordmark |
| 13310 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 13420 | Features that need screen / UI state |
| 13428 | Find / tracker banner (slides in under the header, tap = track it) |
| 13467 | Alerts (find + tracker + battery) |
| 13538 | Scenes (one tap arms a useful set of groups) |
| 13557 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 13621 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 13634 | Motion sensor: start-up retries, a plain-words reason when it is missing, and a retry later |
| 13722 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 13957 | DIRECTION SWEEP (experimental) |
| 15742 | One call from the main loop |
| 15757 | Banner, session report, self-test drawing |
| 15781 | Session report: totals, per-group bars, last-60-minutes chart |
| 15883 | Self-test: every subsystem at a glance, live |
| 16018 | Auto-rotate calibration wizard |
| 16068 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 16369 | Render driver (strip by strip) |
| 16417 | Sweep screen, baseline files, walkthrough rooms, hunt upgrades |
| 17068 | BOOT button (GPIO0) |
| 17714 | Actions |
| 18179 | Touch state machine |
| 18529 | Setup / loop |
