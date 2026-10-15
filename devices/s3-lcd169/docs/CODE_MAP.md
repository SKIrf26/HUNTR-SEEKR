# Code map

`src/HUNTR-SEEKR_S3_LCD169/HUNTR-SEEKR_S3_LCD169.ino` is a single Arduino sketch of about 17,840 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 70 | Terminal theme: palette, fonts, helpers |
| 85 | Pins (Waveshare HARDWARE_REFERENCE: ESP32-S3-Touch-LCD-1.69) |
| 143 | What the shared interface code needs to know about this board |
| 188 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 320 | Forward declarations |
| 378 | Panel (ST7789V2 240x280) |
| 381 | Strip renderer state |
| 483 | Palette |
| 567 | Primitives (integer only) |
| 742 | Terminal theme: panel shape and the CRT pass |
| 960 | Text (anti-aliased) |
| 1332 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1489 | Helpers |
| 1833 | Log queue (filled from radio callbacks, drained in loop) |
| 1846 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2037 | Backlight (PWM, gamma 2) + display sleep |
| 2058 | GPS + log writer (loop context only) |
| 2181 | Detection callbacks |
| 2184 | Bluetooth / Wi-Fi device recognition |
| 6762 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 6863 | Sweep tools (walkthrough marks, baseline-then-sweep) and hunt upgrades |
| 7429 | 802.15.4 (Zigbee + Thread) receive path |
| 7656 | Radio control |
| 8074 | Share portal state |
| 8079 | Share portal (WPA2) |
| 8242 | Storage / sessions |
| 8319 | Preferences |
| 8418 | I2C helpers |
| 8444 | Touch controller (CST816T: 6 bytes from register 0x01) |
| 8546 | Battery (GPIO0 through a 200k/100k divider: VBAT = VADC x 3). No power-management chip on this board. |
| 8581 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 8704 | Core logic (no screen state in here) |
| 8707 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 8825 | Session statistics + 60-minute timeline |
| 8899 | Real-time clock (PCF85063 @0x51) |
| 9019 | Custom signatures (/sigs.txt: one rule per line) |
| 9183 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 9242 | What the alert is about (captured under the mux by upsert) |
| 9253 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9284 | Tracker "following you": seen continuously for followMin minutes |
| 9312 | Scenes (one tap arms a useful set of groups) |
| 9329 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9336 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9348 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 9606 | Findings record (what was found this session, saved on demand) |
| 9862 | Boot sound (WATCH): your own short sound on the boot screen |
| 10097 | Boot logo (all boards): your own picture on the boot screen |
| 10443 | UI |
| 10595 | Pill (status; tap = pause / resume) + battery |
| 10705 | Bottom bars |
| 10837 | Tiles (ARM page) |
| 10943 | Find list (FINDS page + pushed filtered list) |
| 11207 | Pushed, filtered list (long-press a tile) |
| 11216 | Hunt |
| 11219 | Terminal theme: the motion-tracker view of Track |
| 11454 | HUD theme: the blue HUD tracker of the Track screen |
| 11764 | Shared bits for text pages |
| 11825 | LOG page |
| 11826 | Groups panel: each group, live status, and what it searches for |
| 12077 | SETUP page |
| 12277 | Help |
| 12336 | Storage (internal flash) |
| 12468 | Share portal |
| 12492 | Boot: radar ping + wordmark |
| 12494 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 12604 | Features that need screen / UI state |
| 12612 | Find / tracker banner (slides in under the header, tap = track it) |
| 12651 | Alerts (find + tracker + battery) |
| 12722 | Scenes (one tap arms a useful set of groups) |
| 12741 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 12805 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 12808 | Motion sensor: start-up retries, a plain-words reason when it is missing, and a retry later |
| 12896 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 13129 | DIRECTION SWEEP (experimental) |
| 14926 | One call from the main loop |
| 14941 | Banner, session report, self-test drawing |
| 14965 | Session report: totals, per-group bars, last-60-minutes chart |
| 15067 | Self-test: every subsystem at a glance, live |
| 15202 | Auto-rotate calibration wizard |
| 15252 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 15553 | Render driver (strip by strip) |
| 15601 | Sweep screen, baseline files, walkthrough rooms, hunt upgrades |
| 16192 | BOOT button (GPIO0) |
| 16833 | Actions |
| 17285 | Touch state machine |
| 17635 | Setup / loop |
