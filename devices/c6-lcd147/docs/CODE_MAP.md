# Code map

`src/HUNTR-SEEKR_C6_LCD147/HUNTR-SEEKR_C6_LCD147.ino` is a single Arduino sketch of about 18,047 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 72 | Terminal theme: palette, fonts, helpers |
| 87 | Pins (Waveshare docs: ESP32-C6-Touch-LCD-1.47) |
| 137 | What the shared interface code needs to know about this board |
| 182 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 314 | Forward declarations |
| 372 | Panel (JD9853 through the ST7789 class) |
| 375 | Strip renderer state |
| 477 | Palette |
| 561 | Primitives (integer only) |
| 736 | Terminal theme: panel shape and the CRT pass |
| 954 | Text (anti-aliased) |
| 1326 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1483 | Helpers |
| 1827 | Log queue (filled from radio callbacks, drained in loop) |
| 1840 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2031 | Backlight (PWM, gamma 2) + display sleep |
| 2052 | GPS + log writer (loop context only) |
| 2175 | Detection callbacks |
| 2178 | Bluetooth / Wi-Fi device recognition |
| 6756 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 6857 | Sweep tools (walkthrough marks, baseline-then-sweep) and hunt upgrades |
| 7423 | 802.15.4 (Zigbee + Thread) receive path |
| 7650 | Radio control |
| 8068 | Share portal state |
| 8073 | Share portal (WPA2) |
| 8236 | Storage / sessions |
| 8313 | Preferences |
| 8412 | I2C helpers |
| 8438 | Touch controller (AXS5106L: reg 0x01, 14 bytes; same as Waveshare's driver) |
| 8618 | Battery (GPIO0 through a 200k/100k divider: VBAT = VADC x 3). No power-management chip on this board. |
| 8651 | Panel init (JD9853 register set from Waveshare) |
| 8873 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 8996 | Core logic (no screen state in here) |
| 8999 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 9117 | Session statistics + 60-minute timeline |
| 9191 | Real-time clock (PCF85063 @0x51) |
| 9311 | Custom signatures (/sigs.txt: one rule per line) |
| 9475 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 9534 | What the alert is about (captured under the mux by upsert) |
| 9545 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9576 | Tracker "following you": seen continuously for followMin minutes |
| 9604 | Scenes (one tap arms a useful set of groups) |
| 9621 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9628 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9640 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 9898 | Findings record (what was found this session, saved on demand) |
| 10154 | Boot sound (WATCH): your own short sound on the boot screen |
| 10389 | Boot logo (all boards): your own picture on the boot screen |
| 10735 | UI |
| 10887 | Pill (status; tap = pause / resume) + battery |
| 10999 | Bottom bars |
| 11131 | Tiles (ARM page) |
| 11237 | Find list (FINDS page + pushed filtered list) |
| 11501 | Pushed, filtered list (long-press a tile) |
| 11510 | Hunt |
| 11513 | Terminal theme: the motion-tracker view of Track |
| 11716 | HUD theme: the blue HUD tracker of the Track screen |
| 12201 | Shared bits for text pages |
| 12262 | LOG page |
| 12263 | Groups panel: each group, live status, and what it searches for |
| 12514 | SETUP page |
| 12726 | Help |
| 12785 | Storage (internal flash) |
| 12917 | Share portal |
| 12941 | Boot: radar ping + wordmark |
| 12943 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 13053 | Features that need screen / UI state |
| 13061 | Find / tracker banner (slides in under the header, tap = track it) |
| 13100 | Alerts (find + tracker + battery) |
| 13171 | Scenes (one tap arms a useful set of groups) |
| 13190 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 13254 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 13257 | Motion sensor: start-up retries, a plain-words reason when it is missing, and a retry later |
| 13345 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 13578 | DIRECTION SWEEP (experimental) |
| 15077 | One call from the main loop |
| 15092 | Banner, session report, self-test drawing |
| 15116 | Session report: totals, per-group bars, last-60-minutes chart |
| 15218 | Self-test: every subsystem at a glance, live |
| 15353 | Auto-rotate calibration wizard |
| 15403 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 15704 | Render driver (strip by strip) |
| 15752 | Sweep screen, baseline files, walkthrough rooms, hunt upgrades |
| 16403 | BOOT button (GPIO0) |
| 17044 | Actions |
| 17496 | Touch state machine |
| 17846 | Setup / loop |
