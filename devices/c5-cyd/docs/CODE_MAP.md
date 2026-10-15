# Code map

`src/HUNTR-SEEKR_C5_CYD/HUNTR-SEEKR_C5_CYD.ino` is a single Arduino sketch of about 15,970 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

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
| 423 | Strip renderer state |
| 522 | Palette |
| 605 | Primitives (integer only) |
| 779 | Terminal theme: panel shape and the CRT pass |
| 995 | Text (anti-aliased) |
| 1494 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1660 | Helpers |
| 2002 | Log queue (filled from radio callbacks, drained in loop) |
| 2014 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2209 | Backlight (PWM, gamma 2) + display sleep |
| 2226 | GPS + log writer (loop context only) |
| 2349 | Bluetooth / Wi-Fi device recognition |
| 6976 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 7553 | 802.15.4 (Zigbee + Thread) receive path |
| 7774 | Radio control |
| 8471 | Share portal state |
| 8506 | Share portal (WPA2) |
| 8679 | Storage / sessions |
| 8757 | Preferences |
| 8852 | I2C helpers |
| 8877 | SD card (shared SPI bus), internal flash as the fallback |
| 8906 | Touch controller (CST816T: 6 bytes from register 0x01) |
| 8989 | Power: no gauge and usually no battery on this board |
| 9001 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 9122 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 9239 | Session statistics + 60-minute timeline |
| 9312 | Real-time clock (PCF85063 @0x51) |
| 9431 | Custom signatures (/sigs.txt: one rule per line) |
| 9619 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 9677 | What the alert is about (captured under the mux by upsert) |
| 9687 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9717 | Tracker "following you": seen continuously for followMin minutes |
| 9744 | Scenes (one tap arms a useful set of groups) |
| 9760 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9766 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9777 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 10034 | Findings record (what was found this session, saved on demand) |
| 10390 | Boot sound (WATCH): your own short sound on the boot screen |
| 10623 | Boot logo (all boards): your own picture on the boot screen |
| 10967 | UI |
| 11109 | Pill (status; tap = pause / resume) + battery |
| 11236 | Bottom bars |
| 11326 | Tiles (ARM page) |
| 11430 | Find list (FINDS page + pushed filtered list) |
| 11692 | Pushed, filtered list (long-press a tile) |
| 11700 | Hunt |
| 11703 | Terminal theme: the motion-tracker view of Track |
| 11935 | HUD theme: the blue HUD tracker of the Track screen |
| 12404 | Shared bits for text pages |
| 12464 | LOG page |
| 12465 | Groups panel: each group, live status, and what it searches for |
| 12715 | SETUP page |
| 12910 | Help |
| 12968 | Storage (internal flash) |
| 13099 | Share portal |
| 13122 | Boot: radar ping + wordmark |
| 13124 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 13232 | Features that need screen / UI state |
| 13239 | Find / tracker banner (slides in under the header, tap = track it) |
| 13277 | Alerts (find + tracker + battery) |
| 13347 | Scenes (one tap arms a useful set of groups) |
| 13365 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 13425 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 13467 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 13679 | One call from the main loop |
| 13692 | Banner, session report, self-test drawing |
| 13715 | Session report: totals, per-group bars, last-60-minutes chart |
| 13816 | Self-test: every subsystem at a glance, live |
| 13969 | Auto-rotate calibration wizard |
| 14018 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 14317 | Render driver (strip by strip) |
| 14966 | Actions |
| 15415 | Touch state machine |
| 15764 | Setup / loop |
