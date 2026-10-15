# Code map

`src/HUNTR-SEEKR_C5_CYD/HUNTR-SEEKR_C5_CYD.ino` is a single Arduino sketch of about 16,010 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 78 | Terminal theme: palette, fonts, helpers |
| 111 | Pins (NM-CYD-C5) |
| 166 | What the shared interface code needs to know about this board |
| 207 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 339 | Forward declarations |
| 397 | Panel (TFT_eSPI) + touch (XPT2046), behind the same calls the other boards use |
| 449 | Strip renderer state |
| 548 | Palette |
| 631 | Primitives (integer only) |
| 805 | Terminal theme: panel shape and the CRT pass |
| 1021 | Text (anti-aliased) |
| 1520 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1686 | Helpers |
| 2028 | Log queue (filled from radio callbacks, drained in loop) |
| 2040 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2235 | Backlight (PWM, gamma 2) + display sleep |
| 2252 | GPS + log writer (loop context only) |
| 2375 | Bluetooth / Wi-Fi device recognition |
| 7002 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 7579 | 802.15.4 (Zigbee + Thread) receive path |
| 7800 | Radio control |
| 8497 | Share portal state |
| 8532 | Share portal (WPA2) |
| 8705 | Storage / sessions |
| 8783 | Preferences |
| 8880 | I2C helpers |
| 8905 | SD card (shared SPI bus), internal flash as the fallback |
| 8934 | Touch controller (CST816T: 6 bytes from register 0x01) |
| 9017 | Power: no gauge and usually no battery on this board |
| 9029 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 9150 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 9267 | Session statistics + 60-minute timeline |
| 9340 | Real-time clock (PCF85063 @0x51) |
| 9459 | Custom signatures (/sigs.txt: one rule per line) |
| 9647 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 9705 | What the alert is about (captured under the mux by upsert) |
| 9715 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9745 | Tracker "following you": seen continuously for followMin minutes |
| 9772 | Scenes (one tap arms a useful set of groups) |
| 9788 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9794 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9805 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 10062 | Findings record (what was found this session, saved on demand) |
| 10418 | Boot sound (WATCH): your own short sound on the boot screen |
| 10651 | Boot logo (all boards): your own picture on the boot screen |
| 10995 | UI |
| 11137 | Pill (status; tap = pause / resume) + battery |
| 11264 | Bottom bars |
| 11354 | Tiles (ARM page) |
| 11458 | Find list (FINDS page + pushed filtered list) |
| 11720 | Pushed, filtered list (long-press a tile) |
| 11728 | Hunt |
| 11731 | Terminal theme: the motion-tracker view of Track |
| 11963 | HUD theme: the blue HUD tracker of the Track screen |
| 12432 | Shared bits for text pages |
| 12492 | LOG page |
| 12493 | Groups panel: each group, live status, and what it searches for |
| 12743 | SETUP page |
| 12942 | Help |
| 13000 | Storage (internal flash) |
| 13131 | Share portal |
| 13154 | Boot: radar ping + wordmark |
| 13156 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 13264 | Features that need screen / UI state |
| 13271 | Find / tracker banner (slides in under the header, tap = track it) |
| 13309 | Alerts (find + tracker + battery) |
| 13379 | Scenes (one tap arms a useful set of groups) |
| 13397 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 13457 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 13499 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 13711 | One call from the main loop |
| 13724 | Banner, session report, self-test drawing |
| 13747 | Session report: totals, per-group bars, last-60-minutes chart |
| 13848 | Self-test: every subsystem at a glance, live |
| 14001 | Auto-rotate calibration wizard |
| 14050 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 14349 | Render driver (strip by strip) |
| 14998 | Actions |
| 15457 | Touch state machine |
| 15806 | Setup / loop |
