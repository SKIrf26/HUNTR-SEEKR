# Code map

`src/HUNTR-SEEKR_CYD_2432S028/HUNTR-SEEKR_CYD_2432S028.ino` is a single Arduino sketch of about 15,140 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 81 | Terminal theme: palette, fonts, helpers |
| 176 | What the shared interface code needs to know about this board |
| 217 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 420 | Forward declarations |
| 476 | Panel (TFT_eSPI) + touch (XPT2046), behind the same calls the other boards use |
| 619 | Strip renderer state |
| 718 | Palette |
| 801 | Primitives (integer only) |
| 975 | Terminal theme: panel shape and the CRT pass |
| 1191 | Text (anti-aliased) |
| 1534 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1672 | Helpers |
| 2014 | Log queue (filled from radio callbacks, drained in loop) |
| 2026 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2198 | Backlight (PWM, gamma 2) + display sleep |
| 2215 | GPS + log writer (loop context only) |
| 2338 | Bluetooth / Wi-Fi device recognition |
| 7311 | 802.15.4 (Zigbee + Thread) receive path |
| 7906 | Share portal state |
| 7911 | Share portal (WPA2) |
| 8073 | Storage / sessions |
| 8151 | Preferences |
| 8242 | I2C helpers |
| 8267 | SD card (shared SPI bus), internal flash as the fallback |
| 8296 | Touch controller (CST816T: 6 bytes from register 0x01) |
| 8378 | Power: no gauge and usually no battery on this board |
| 8390 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 8511 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 8628 | Session statistics + 60-minute timeline |
| 8701 | Real-time clock (PCF85063 @0x51) |
| 8820 | Custom signatures (/sigs.txt: one rule per line) |
| 8970 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 9028 | What the alert is about (captured under the mux by upsert) |
| 9038 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9068 | Tracker "following you": seen continuously for followMin minutes |
| 9095 | Scenes (one tap arms a useful set of groups) |
| 9111 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9117 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9128 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 9378 | Findings record (what was found this session, saved on demand) |
| 9632 | Boot sound (WATCH): your own short sound on the boot screen |
| 9865 | Boot logo (all boards): your own picture on the boot screen |
| 10209 | UI |
| 10352 | Pill (status; tap = pause / resume) + battery |
| 10461 | Bottom bars |
| 10551 | Tiles (ARM page) |
| 10655 | Find list (FINDS page + pushed filtered list) |
| 10917 | Pushed, filtered list (long-press a tile) |
| 10925 | Hunt |
| 10928 | Terminal theme: the motion-tracker view of Track |
| 11160 | HUD theme: the blue HUD tracker of the Track screen |
| 11629 | Shared bits for text pages |
| 11689 | LOG page |
| 11690 | Groups panel: each group, live status, and what it searches for |
| 11940 | SETUP page |
| 12136 | Help |
| 12194 | Storage (internal flash) |
| 12325 | Share portal |
| 12348 | Boot: radar ping + wordmark |
| 12350 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 12458 | Features that need screen / UI state |
| 12465 | Find / tracker banner (slides in under the header, tap = track it) |
| 12503 | Alerts (find + tracker + battery) |
| 12573 | Scenes (one tap arms a useful set of groups) |
| 12591 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 12654 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 12696 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 12908 | One call from the main loop |
| 12921 | Banner, session report, self-test drawing |
| 12944 | Session report: totals, per-group bars, last-60-minutes chart |
| 13045 | Self-test: every subsystem at a glance, live |
| 13168 | Auto-rotate calibration wizard |
| 13217 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 13516 | Render driver (strip by strip) |
| 14137 | Actions |
| 14567 | Touch state machine |
| 14916 | Setup / loop |
