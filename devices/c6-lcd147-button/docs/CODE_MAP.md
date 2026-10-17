# Code map

`src/HUNTR-SEEKR_C6_LCD147_BUTTON/HUNTR-SEEKR_C6_LCD147_BUTTON.ino` is a single Arduino sketch of about 18,830 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order. The one-button interface (the button reader, the status LED, the hint strip and the Home, Scene, Groups and More screens) is the section **One-button interface (this board has no touch panel)**, just before *Setup / loop*.

| Line | Section |
|---:|---|
| 79 | Terminal theme: palette, fonts, helpers |
| 94 | Pins (Waveshare ESP32-C6-LCD-1.47: interface table on the product page + the pin-out picture) |
| 151 | What the shared interface code needs to know about this board |
| 218 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 352 | Forward declarations |
| 410 | Panel (ST7789, 172x320, IPS) |
| 413 | Strip renderer state |
| 515 | Palette |
| 599 | Primitives (integer only) |
| 774 | Terminal theme: panel shape and the CRT pass |
| 992 | Text (anti-aliased) |
| 1575 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1755 | Helpers |
| 2103 | Log queue (filled from radio callbacks, drained in loop) |
| 2116 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2307 | Backlight (PWM, gamma 2) + display sleep |
| 2328 | GPS + log writer (loop context only) |
| 2451 | Detection callbacks |
| 2454 | Bluetooth / Wi-Fi device recognition |
| 7032 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 7133 | Sweep tools (walkthrough marks, baseline-then-sweep) and hunt upgrades |
| 7788 | 802.15.4 (Zigbee + Thread) receive path |
| 8015 | Radio control |
| 8433 | Share portal state |
| 8438 | Share portal (WPA2) |
| 8601 | Storage / sessions |
| 8678 | Preferences |
| 8777 | I2C helpers |
| 8803 | Touch: this board has none. These stand-ins keep the shared code compiling; the button layer does the input. |
| 8824 | Battery: none (the board runs from USB or the 5V pad through a plain regulator) |
| 8839 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 8962 | Core logic (no screen state in here) |
| 8965 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 9083 | Session statistics + 60-minute timeline |
| 9157 | Real-time clock (PCF85063 @0x51) |
| 9277 | Custom signatures (/sigs.txt: one rule per line) |
| 9441 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 9500 | What the alert is about (captured under the mux by upsert) |
| 9511 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9542 | Tracker "following you": seen continuously for followMin minutes |
| 9570 | Scenes (one tap arms a useful set of groups) |
| 9587 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9594 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9606 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 9864 | Findings record (what was found this session, saved on demand) |
| 10120 | Boot sound (WATCH): your own short sound on the boot screen |
| 10355 | Boot logo (all boards): your own picture on the boot screen |
| 10701 | UI |
| 10853 | Pill (status; tap = pause / resume) + battery |
| 10966 | Bottom bars |
| 11098 | Tiles (ARM page) |
| 11204 | Find list (FINDS page + pushed filtered list) |
| 11470 | Pushed, filtered list (long-press a tile) |
| 11478 | Hunt |
| 11481 | Terminal theme: the motion-tracker view of Track |
| 11682 | HUD theme: the blue HUD tracker of the Track screen |
| 12181 | Shared bits for text pages |
| 12242 | LOG page |
| 12243 | Groups panel: each group, live status, and what it searches for |
| 12494 | SETUP page |
| 12706 | Help |
| 12765 | Storage (internal flash) |
| 12897 | Share portal |
| 12920 | Boot: radar ping + wordmark |
| 12922 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 13032 | Features that need screen / UI state |
| 13040 | Find / tracker banner (slides in under the header, tap = track it) |
| 13079 | Alerts (find + tracker + battery) |
| 13150 | Scenes (one tap arms a useful set of groups) |
| 13169 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 13233 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 13236 | Motion sensor: start-up retries, a plain-words reason when it is missing, and a retry later |
| 13324 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 13557 | DIRECTION SWEEP (experimental) |
| 15056 | One call from the main loop |
| 15071 | Banner, session report, self-test drawing |
| 15095 | Session report: totals, per-group bars, last-60-minutes chart |
| 15198 | Self-test: every subsystem at a glance, live |
| 15333 | Auto-rotate calibration wizard |
| 15383 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 15684 | Render driver (strip by strip) |
| 15732 | Sweep screen, baseline files, walkthrough rooms, hunt upgrades |
| 16383 | BOOT button (GPIO0) |
| 17024 | Actions |
| 17476 | Touch state machine |
| 17829 | ONE-BUTTON INTERFACE (this board has no touch panel) |
| 17954 | RGB status LED (the on-board WS2812 on IO8) |
| 18098 | The hint strip along the bottom (also the live hold meter) |
| 18139 | Screens |
| 18309 | Actions |
| 18545 | Per-loop housekeeping: first selection, scroll follow, periodic redraw |
| 18622 | Setup / loop |
