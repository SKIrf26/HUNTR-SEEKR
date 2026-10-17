# Code map

`src/HUNTR-SEEKR_CYD_2432S028/HUNTR-SEEKR_CYD_2432S028.ino` is a single Arduino sketch of about 15,480 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 81 | Terminal theme: palette, fonts, helpers |
| 176 | What the shared interface code needs to know about this board |
| 217 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 422 | Forward declarations |
| 478 | Panel (TFT_eSPI) + touch (XPT2046), behind the same calls the other boards use |
| 621 | Strip renderer state |
| 720 | Palette |
| 803 | Primitives (integer only) |
| 977 | Terminal theme: panel shape and the CRT pass |
| 1193 | Text (anti-aliased) |
| 1747 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1909 | Helpers |
| 2255 | Log queue (filled from radio callbacks, drained in loop) |
| 2267 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2439 | Backlight (PWM, gamma 2) + display sleep |
| 2456 | GPS + log writer (loop context only) |
| 2579 | Bluetooth / Wi-Fi device recognition |
| 7641 | 802.15.4 (Zigbee + Thread) receive path |
| 8236 | Share portal state |
| 8241 | Share portal (WPA2) |
| 8403 | Storage / sessions |
| 8481 | Preferences |
| 8572 | I2C helpers |
| 8597 | SD card (shared SPI bus), internal flash as the fallback |
| 8626 | Touch controller (CST816T: 6 bytes from register 0x01) |
| 8708 | Power: no gauge and usually no battery on this board |
| 8720 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 8841 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 8958 | Session statistics + 60-minute timeline |
| 9031 | Real-time clock (PCF85063 @0x51) |
| 9150 | Custom signatures (/sigs.txt: one rule per line) |
| 9300 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 9358 | What the alert is about (captured under the mux by upsert) |
| 9368 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9398 | Tracker "following you": seen continuously for followMin minutes |
| 9425 | Scenes (one tap arms a useful set of groups) |
| 9441 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9447 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9458 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 9708 | Findings record (what was found this session, saved on demand) |
| 9962 | Boot sound (WATCH): your own short sound on the boot screen |
| 10195 | Boot logo (all boards): your own picture on the boot screen |
| 10539 | UI |
| 10682 | Pill (status; tap = pause / resume) + battery |
| 10791 | Bottom bars |
| 10881 | Tiles (ARM page) |
| 10985 | Find list (FINDS page + pushed filtered list) |
| 11247 | Pushed, filtered list (long-press a tile) |
| 11255 | Hunt |
| 11258 | Terminal theme: the motion-tracker view of Track |
| 11490 | HUD theme: the blue HUD tracker of the Track screen |
| 11973 | Shared bits for text pages |
| 12033 | LOG page |
| 12034 | Groups panel: each group, live status, and what it searches for |
| 12284 | SETUP page |
| 12480 | Help |
| 12538 | Storage (internal flash) |
| 12669 | Share portal |
| 12692 | Boot: radar ping + wordmark |
| 12694 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 12802 | Features that need screen / UI state |
| 12809 | Find / tracker banner (slides in under the header, tap = track it) |
| 12847 | Alerts (find + tracker + battery) |
| 12917 | Scenes (one tap arms a useful set of groups) |
| 12935 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 12998 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 13040 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 13252 | One call from the main loop |
| 13265 | Banner, session report, self-test drawing |
| 13288 | Session report: totals, per-group bars, last-60-minutes chart |
| 13389 | Self-test: every subsystem at a glance, live |
| 13512 | Auto-rotate calibration wizard |
| 13561 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 13860 | Render driver (strip by strip) |
| 14481 | Actions |
| 14911 | Touch state machine |
| 15260 | Setup / loop |
