# Code map

`src/HUNTR-SEEKR_C6_LCD147/HUNTR-SEEKR_C6_LCD147.ino` is a single Arduino sketch of about 18,500 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 72 | Terminal theme: palette, fonts, helpers |
| 87 | Pins (Waveshare docs: ESP32-C6-Touch-LCD-1.47) |
| 137 | What the shared interface code needs to know about this board |
| 182 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 316 | Forward declarations |
| 374 | Panel (JD9853 through the ST7789 class) |
| 377 | Strip renderer state |
| 479 | Palette |
| 563 | Primitives (integer only) |
| 738 | Terminal theme: panel shape and the CRT pass |
| 956 | Text (anti-aliased) |
| 1539 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1719 | Helpers |
| 2067 | Log queue (filled from radio callbacks, drained in loop) |
| 2080 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2271 | Backlight (PWM, gamma 2) + display sleep |
| 2292 | GPS + log writer (loop context only) |
| 2415 | Detection callbacks |
| 2418 | Bluetooth / Wi-Fi device recognition |
| 6996 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 7097 | Sweep tools (walkthrough marks, baseline-then-sweep) and hunt upgrades |
| 7752 | 802.15.4 (Zigbee + Thread) receive path |
| 7979 | Radio control |
| 8397 | Share portal state |
| 8402 | Share portal (WPA2) |
| 8565 | Storage / sessions |
| 8642 | Preferences |
| 8741 | I2C helpers |
| 8767 | Touch controller (AXS5106L: reg 0x01, 14 bytes; same as Waveshare's driver) |
| 8947 | Battery (GPIO0 through a 200k/100k divider: VBAT = VADC x 3). No power-management chip on this board. |
| 8980 | Panel init (JD9853 register set from Waveshare) |
| 9202 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 9325 | Core logic (no screen state in here) |
| 9328 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 9446 | Session statistics + 60-minute timeline |
| 9520 | Real-time clock (PCF85063 @0x51) |
| 9640 | Custom signatures (/sigs.txt: one rule per line) |
| 9804 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 9863 | What the alert is about (captured under the mux by upsert) |
| 9874 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9905 | Tracker "following you": seen continuously for followMin minutes |
| 9933 | Scenes (one tap arms a useful set of groups) |
| 9950 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9957 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9969 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 10227 | Findings record (what was found this session, saved on demand) |
| 10483 | Boot sound (WATCH): your own short sound on the boot screen |
| 10718 | Boot logo (all boards): your own picture on the boot screen |
| 11064 | UI |
| 11216 | Pill (status; tap = pause / resume) + battery |
| 11328 | Bottom bars |
| 11460 | Tiles (ARM page) |
| 11566 | Find list (FINDS page + pushed filtered list) |
| 11830 | Pushed, filtered list (long-press a tile) |
| 11839 | Hunt |
| 11842 | Terminal theme: the motion-tracker view of Track |
| 12045 | HUD theme: the blue HUD tracker of the Track screen |
| 12365 | Shared bits for text pages |
| 12426 | LOG page |
| 12427 | Groups panel: each group, live status, and what it searches for |
| 12678 | SETUP page |
| 12890 | Help |
| 12949 | Storage (internal flash) |
| 13081 | Share portal |
| 13105 | Boot: radar ping + wordmark |
| 13107 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 13217 | Features that need screen / UI state |
| 13225 | Find / tracker banner (slides in under the header, tap = track it) |
| 13264 | Alerts (find + tracker + battery) |
| 13335 | Scenes (one tap arms a useful set of groups) |
| 13354 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 13418 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 13421 | Motion sensor: start-up retries, a plain-words reason when it is missing, and a retry later |
| 13509 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 13742 | DIRECTION SWEEP (experimental) |
| 15528 | One call from the main loop |
| 15543 | Banner, session report, self-test drawing |
| 15567 | Session report: totals, per-group bars, last-60-minutes chart |
| 15669 | Self-test: every subsystem at a glance, live |
| 15804 | Auto-rotate calibration wizard |
| 15854 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 16155 | Render driver (strip by strip) |
| 16203 | Sweep screen, baseline files, walkthrough rooms, hunt upgrades |
| 16854 | BOOT button (GPIO0) |
| 17495 | Actions |
| 17947 | Touch state machine |
| 18297 | Setup / loop |
