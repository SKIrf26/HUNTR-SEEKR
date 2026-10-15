# Code map

`src/HUNTR-SEEKR_CYD_2432S028/HUNTR-SEEKR_CYD_2432S028.ino` is a single Arduino sketch of about 15,091 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 76 | Terminal theme: palette, fonts, helpers |
| 171 | What the shared interface code needs to know about this board |
| 212 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 415 | Forward declarations |
| 471 | Panel (TFT_eSPI) + touch (XPT2046), behind the same calls the other boards use |
| 588 | Strip renderer state |
| 687 | Palette |
| 770 | Primitives (integer only) |
| 944 | Terminal theme: panel shape and the CRT pass |
| 1160 | Text (anti-aliased) |
| 1503 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1641 | Helpers |
| 1983 | Log queue (filled from radio callbacks, drained in loop) |
| 1995 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2167 | Backlight (PWM, gamma 2) + display sleep |
| 2184 | GPS + log writer (loop context only) |
| 2307 | Bluetooth / Wi-Fi device recognition |
| 7280 | 802.15.4 (Zigbee + Thread) receive path |
| 7875 | Share portal state |
| 7880 | Share portal (WPA2) |
| 8042 | Storage / sessions |
| 8120 | Preferences |
| 8209 | I2C helpers |
| 8234 | SD card (shared SPI bus), internal flash as the fallback |
| 8263 | Touch controller (CST816T: 6 bytes from register 0x01) |
| 8345 | Power: no gauge and usually no battery on this board |
| 8357 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 8478 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 8595 | Session statistics + 60-minute timeline |
| 8668 | Real-time clock (PCF85063 @0x51) |
| 8787 | Custom signatures (/sigs.txt: one rule per line) |
| 8937 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 8995 | What the alert is about (captured under the mux by upsert) |
| 9005 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9035 | Tracker "following you": seen continuously for followMin minutes |
| 9062 | Scenes (one tap arms a useful set of groups) |
| 9078 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9084 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9095 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 9345 | Findings record (what was found this session, saved on demand) |
| 9599 | Boot sound (WATCH): your own short sound on the boot screen |
| 9832 | Boot logo (all boards): your own picture on the boot screen |
| 10176 | UI |
| 10319 | Pill (status; tap = pause / resume) + battery |
| 10428 | Bottom bars |
| 10518 | Tiles (ARM page) |
| 10622 | Find list (FINDS page + pushed filtered list) |
| 10884 | Pushed, filtered list (long-press a tile) |
| 10892 | Hunt |
| 10895 | Terminal theme: the motion-tracker view of Track |
| 11127 | HUD theme: the blue HUD tracker of the Track screen |
| 11596 | Shared bits for text pages |
| 11656 | LOG page |
| 11657 | Groups panel: each group, live status, and what it searches for |
| 11907 | SETUP page |
| 12099 | Help |
| 12157 | Storage (internal flash) |
| 12288 | Share portal |
| 12311 | Boot: radar ping + wordmark |
| 12313 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 12421 | Features that need screen / UI state |
| 12428 | Find / tracker banner (slides in under the header, tap = track it) |
| 12466 | Alerts (find + tracker + battery) |
| 12536 | Scenes (one tap arms a useful set of groups) |
| 12554 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 12617 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 12659 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 12871 | One call from the main loop |
| 12884 | Banner, session report, self-test drawing |
| 12907 | Session report: totals, per-group bars, last-60-minutes chart |
| 13008 | Self-test: every subsystem at a glance, live |
| 13131 | Auto-rotate calibration wizard |
| 13180 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 13479 | Render driver (strip by strip) |
| 14100 | Actions |
| 14520 | Touch state machine |
| 14869 | Setup / loop |
