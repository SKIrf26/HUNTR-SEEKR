# Code map

`src/HUNTR-SEEKR_C6_LCD147_BUTTON/HUNTR-SEEKR_C6_LCD147_BUTTON.ino` is a single Arduino sketch of about 18,484 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order. The one-button interface (the button reader, the status LED, the hint strip and the Home, Scene, Groups and More screens) is the section **One-button interface (this board has no touch panel)**, just before *Setup / loop*.

| Line | Section |
|---:|---|
| 72 | Terminal theme: palette, fonts, helpers |
| 87 | Pins (Waveshare ESP32-C6-LCD-1.47: interface table on the product page + the pin-out picture) |
| 144 | What the shared interface code needs to know about this board |
| 211 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 343 | Forward declarations |
| 401 | Panel (ST7789, 172x320, IPS) |
| 404 | Strip renderer state |
| 506 | Palette |
| 590 | Primitives (integer only) |
| 765 | Terminal theme: panel shape and the CRT pass |
| 983 | Text (anti-aliased) |
| 1355 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1512 | Helpers |
| 1856 | Log queue (filled from radio callbacks, drained in loop) |
| 1869 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2060 | Backlight (PWM, gamma 2) + display sleep |
| 2081 | GPS + log writer (loop context only) |
| 2204 | Detection callbacks |
| 2207 | Bluetooth / Wi-Fi device recognition |
| 6785 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 6886 | Sweep tools (walkthrough marks, baseline-then-sweep) and hunt upgrades |
| 7452 | 802.15.4 (Zigbee + Thread) receive path |
| 7679 | Radio control |
| 8097 | Share portal state |
| 8102 | Share portal (WPA2) |
| 8265 | Storage / sessions |
| 8342 | Preferences |
| 8441 | I2C helpers |
| 8467 | Touch: this board has none. These stand-ins keep the shared code compiling; the button layer does the input. |
| 8488 | Battery: none (the board runs from USB or the 5V pad through a plain regulator) |
| 8503 | Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) |
| 8626 | Core logic (no screen state in here) |
| 8629 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 8747 | Session statistics + 60-minute timeline |
| 8821 | Real-time clock (PCF85063 @0x51) |
| 8941 | Custom signatures (/sigs.txt: one rule per line) |
| 9105 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 9164 | What the alert is about (captured under the mux by upsert) |
| 9175 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 9206 | Tracker "following you": seen continuously for followMin minutes |
| 9234 | Scenes (one tap arms a useful set of groups) |
| 9251 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 9258 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 9270 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 9528 | Findings record (what was found this session, saved on demand) |
| 9784 | Boot sound (WATCH): your own short sound on the boot screen |
| 10019 | Boot logo (all boards): your own picture on the boot screen |
| 10365 | UI |
| 10517 | Pill (status; tap = pause / resume) + battery |
| 10630 | Bottom bars |
| 10762 | Tiles (ARM page) |
| 10868 | Find list (FINDS page + pushed filtered list) |
| 11134 | Pushed, filtered list (long-press a tile) |
| 11142 | Hunt |
| 11145 | Terminal theme: the motion-tracker view of Track |
| 11346 | HUD theme: the blue HUD tracker of the Track screen |
| 11831 | Shared bits for text pages |
| 11892 | LOG page |
| 11893 | Groups panel: each group, live status, and what it searches for |
| 12144 | SETUP page |
| 12356 | Help |
| 12415 | Storage (internal flash) |
| 12547 | Share portal |
| 12570 | Boot: radar ping + wordmark |
| 12572 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 12682 | Features that need screen / UI state |
| 12690 | Find / tracker banner (slides in under the header, tap = track it) |
| 12729 | Alerts (find + tracker + battery) |
| 12800 | Scenes (one tap arms a useful set of groups) |
| 12819 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 12883 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 12886 | Motion sensor: start-up retries, a plain-words reason when it is missing, and a retry later |
| 12974 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 13207 | DIRECTION SWEEP (experimental) |
| 14706 | One call from the main loop |
| 14721 | Banner, session report, self-test drawing |
| 14745 | Session report: totals, per-group bars, last-60-minutes chart |
| 14848 | Self-test: every subsystem at a glance, live |
| 14983 | Auto-rotate calibration wizard |
| 15033 | Touch calibration (4 corner points: 3 define the map, the 4th checks it) |
| 15334 | Render driver (strip by strip) |
| 15382 | Sweep screen, baseline files, walkthrough rooms, hunt upgrades |
| 16033 | BOOT button (GPIO0) |
| 16674 | Actions |
| 17126 | Touch state machine |
| 17479 | ONE-BUTTON INTERFACE (this board has no touch panel) |
| 17604 | RGB status LED (the on-board WS2812 on IO8) |
| 17748 | The hint strip along the bottom (also the live hold meter) |
| 17789 | Screens |
| 17959 | Actions |
| 18195 | Per-loop housekeeping: first selection, scroll follow, periodic redraw |
| 18272 | Setup / loop |
