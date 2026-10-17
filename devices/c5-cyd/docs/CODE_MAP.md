# Code map

`src/HUNTR-SEEKR_C5_CYD/HUNTR-SEEKR_C5_CYD.ino` is a single Arduino sketch of about 16,350 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 78 | Terminal theme: palette, fonts, helpers |
| 111 | Pins (NM-CYD-C5) |
| 166 | What the shared interface code needs to know about this board |
| 207 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 341 | Forward declarations |
| 399 | Panel (TFT_eSPI) + touch (XPT2046), behind the same calls the other boards use |
| 451 | Strip renderer state |
| 550 | Palette |
| 633 | Primitives (integer only) |
| 807 | Terminal theme: panel shape and the CRT pass |
| 1023 | Text (anti-aliased) |
| 1733 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1919 | Helpers |
| 2265 | Log queue (filled from radio callbacks, drained in loop) |
| 2277 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2472 | Backlight (PWM, gamma 2) + display sleep |
| 2489 | GPS + log writer (loop context only) |
| 2612 | Bluetooth / Wi-Fi device recognition |
| 7239 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 7905 | 802.15.4 (Zigbee + Thread) receive path |
| 8126 | Radio control |
| 8823 | Share portal state |
| 8858 | Share portal (WPA2) |
| 9031 | Storage / sessions |
| 9109 | Preferences |
| 9206 | I2C helpers |
| 9231 | SD card (shared SPI bus), internal flash as the fallback |
| 9260 | Touch controller (CST816T: 6 bytes from register 0x01) |
| 9343 | Power: no gauge and usually no battery on this board |
| 9355 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 9476 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 9593 | Session statistics + 60-minute timeline |
| 9666 | Real-time clock (PCF85063 @0x51) |
| 9785 | Custom signatures (/sigs.txt: one rule per line) |
| 9973 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 10031 | What the alert is about (captured under the mux by upsert) |
| 10041 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 10071 | Tracker "following you": seen continuously for followMin minutes |
| 10098 | Scenes (one tap arms a useful set of groups) |
| 10114 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 10120 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 10131 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 10388 | Findings record (what was found this session, saved on demand) |
| 10744 | Boot sound (WATCH): your own short sound on the boot screen |
| 10977 | Boot logo (all boards): your own picture on the boot screen |
| 11321 | UI |
| 11463 | Pill (status; tap = pause / resume) + battery |
| 11590 | Bottom bars |
| 11680 | Tiles (ARM page) |
| 11784 | Find list (FINDS page + pushed filtered list) |
| 12046 | Pushed, filtered list (long-press a tile) |
| 12054 | Hunt |
| 12057 | Terminal theme: the motion-tracker view of Track |
| 12289 | HUD theme: the blue HUD tracker of the Track screen |
| 12772 | Shared bits for text pages |
| 12832 | LOG page |
| 12833 | Groups panel: each group, live status, and what it searches for |
| 13083 | SETUP page |
| 13282 | Help |
| 13340 | Storage (internal flash) |
| 13471 | Share portal |
| 13494 | Boot: radar ping + wordmark |
| 13496 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 13604 | Features that need screen / UI state |
| 13611 | Find / tracker banner (slides in under the header, tap = track it) |
| 13649 | Alerts (find + tracker + battery) |
| 13719 | Scenes (one tap arms a useful set of groups) |
| 13737 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 13797 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 13839 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 14051 | One call from the main loop |
| 14064 | Banner, session report, self-test drawing |
| 14087 | Session report: totals, per-group bars, last-60-minutes chart |
| 14188 | Self-test: every subsystem at a glance, live |
| 14341 | Auto-rotate calibration wizard |
| 14390 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 14689 | Render driver (strip by strip) |
| 15338 | Actions |
| 15797 | Touch state machine |
| 16146 | Setup / loop |
