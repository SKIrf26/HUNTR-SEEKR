# Code map

`src/HUNTR-SEEKR_S3_LCD169/HUNTR-SEEKR_S3_LCD169.ino` is a single Arduino sketch of about 18,180 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 70 | Terminal theme: palette, fonts, helpers |
| 85 | Pins (Waveshare HARDWARE_REFERENCE: ESP32-S3-Touch-LCD-1.69) |
| 143 | What the shared interface code needs to know about this board |
| 188 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 322 | Forward declarations |
| 380 | Panel (ST7789V2 240x280) |
| 383 | Strip renderer state |
| 485 | Palette |
| 569 | Primitives (integer only) |
| 744 | Terminal theme: panel shape and the CRT pass |
| 962 | Text (anti-aliased) |
| 1545 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1725 | Helpers |
| 2073 | Log queue (filled from radio callbacks, drained in loop) |
| 2086 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2277 | Backlight (PWM, gamma 2) + display sleep |
| 2298 | GPS + log writer (loop context only) |
| 2421 | Detection callbacks |
| 2424 | Bluetooth / Wi-Fi device recognition |
| 7002 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 7103 | Sweep tools (walkthrough marks, baseline-then-sweep) and hunt upgrades |
| 7758 | 802.15.4 (Zigbee + Thread) receive path |
| 7985 | Radio control |
| 8403 | Share portal state |
| 8408 | Share portal (WPA2) |
| 8571 | Storage / sessions |
| 8648 | Preferences |
| 8747 | I2C helpers |
| 8773 | Touch controller (CST816T: 6 bytes from register 0x01) |
| 8875 | Battery (GPIO0 through a 200k/100k divider: VBAT = VADC x 3). No power-management chip on this board. |
| 8910 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 9033 | Core logic (no screen state in here) |
| 9036 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 9154 | Session statistics + 60-minute timeline |
| 9228 | Real-time clock (PCF85063 @0x51) |
| 9348 | Custom signatures (/sigs.txt: one rule per line) |
| 9512 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 9571 | What the alert is about (captured under the mux by upsert) |
| 9582 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9613 | Tracker "following you": seen continuously for followMin minutes |
| 9641 | Scenes (one tap arms a useful set of groups) |
| 9658 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9665 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9677 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 9935 | Findings record (what was found this session, saved on demand) |
| 10191 | Boot sound (WATCH): your own short sound on the boot screen |
| 10426 | Boot logo (all boards): your own picture on the boot screen |
| 10772 | UI |
| 10924 | Pill (status; tap = pause / resume) + battery |
| 11034 | Bottom bars |
| 11166 | Tiles (ARM page) |
| 11272 | Find list (FINDS page + pushed filtered list) |
| 11536 | Pushed, filtered list (long-press a tile) |
| 11545 | Hunt |
| 11548 | Terminal theme: the motion-tracker view of Track |
| 11783 | HUD theme: the blue HUD tracker of the Track screen |
| 12104 | Shared bits for text pages |
| 12165 | LOG page |
| 12166 | Groups panel: each group, live status, and what it searches for |
| 12417 | SETUP page |
| 12617 | Help |
| 12676 | Storage (internal flash) |
| 12808 | Share portal |
| 12832 | Boot: radar ping + wordmark |
| 12834 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 12944 | Features that need screen / UI state |
| 12952 | Find / tracker banner (slides in under the header, tap = track it) |
| 12991 | Alerts (find + tracker + battery) |
| 13062 | Scenes (one tap arms a useful set of groups) |
| 13081 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 13145 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 13148 | Motion sensor: start-up retries, a plain-words reason when it is missing, and a retry later |
| 13236 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 13469 | DIRECTION SWEEP (experimental) |
| 15270 | One call from the main loop |
| 15285 | Banner, session report, self-test drawing |
| 15309 | Session report: totals, per-group bars, last-60-minutes chart |
| 15411 | Self-test: every subsystem at a glance, live |
| 15546 | Auto-rotate calibration wizard |
| 15596 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 15897 | Render driver (strip by strip) |
| 15945 | Sweep screen, baseline files, walkthrough rooms, hunt upgrades |
| 16536 | BOOT button (GPIO0) |
| 17177 | Actions |
| 17629 | Touch state machine |
| 17979 | Setup / loop |
