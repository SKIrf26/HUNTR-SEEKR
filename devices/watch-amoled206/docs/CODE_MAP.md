# Code map

`src/HUNTR-SEEKR_Watch_AMOLED206/HUNTR-SEEKR_Watch_AMOLED206.ino` is a single Arduino sketch of about 19,856 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 74 | Terminal theme: palette, fonts, helpers |
| 89 | Pins (Waveshare verified pin map for this board) |
| 161 | What the shared interface code needs to know about this board |
| 219 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 354 | Forward declarations |
| 417 | Panel (CO5300 over QSPI) |
| 428 | Strip renderer state |
| 536 | Palette |
| 620 | Primitives (integer only) |
| 795 | Terminal theme: panel shape and the CRT pass |
| 1013 | Text (anti-aliased) |
| 1550 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1722 | Helpers |
| 2073 | Log queue (filled from radio callbacks, drained in loop) |
| 2086 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2277 | Brightness + sleep (AMOLED: panel register 0x51, display on/off 0x29/0x28) |
| 2301 | GPS + log writer (loop context only) |
| 2424 | Detection callbacks |
| 2427 | Bluetooth / Wi-Fi device recognition |
| 7058 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 7159 | Sweep tools (walkthrough marks, baseline-then-sweep) and hunt upgrades |
| 7881 | 802.15.4 (Zigbee + Thread) receive path |
| 8108 | Radio control |
| 8813 | Share portal state |
| 8851 | Share portal (WPA2) |
| 9023 | Storage / sessions |
| 9100 | Preferences |
| 9199 | I2C helpers |
| 9234 | FT3168 capacitive touch -- follows Waveshare's working demo exactly |
| 9483 | AXP2101 power management and the two side buttons |
| 9873 | Audio: ES8311 codec + speaker amplifier (beeps) |
| 10383 | Core logic (no screen state in here) |
| 10386 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 10504 | Session statistics + 60-minute timeline |
| 10578 | Real-time clock (PCF85063 @0x51) |
| 10698 | Custom signatures (/sigs.txt: one rule per line) |
| 10885 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 10944 | What the alert is about (captured under the mux by upsert) |
| 10955 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 10986 | Tracker "following you": seen continuously for followMin minutes |
| 11014 | Scenes (one tap arms a useful set of groups) |
| 11031 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 11038 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 11050 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 11309 | Findings record (what was found this session, saved on demand) |
| 11667 | Boot sound (WATCH): your own short sound on the boot screen |
| 11902 | Boot logo (all boards): your own picture on the boot screen |
| 12248 | UI |
| 12403 | Pill (status; tap = pause / resume) + battery |
| 12521 | Bottom bars |
| 12653 | Tiles (ARM page) |
| 12759 | Find list (FINDS page + pushed filtered list) |
| 13023 | Pushed, filtered list (long-press a tile) |
| 13032 | Hunt |
| 13035 | Terminal theme: the motion-tracker view of Track |
| 13243 | HUD theme: the blue HUD tracker of the Track screen |
| 13553 | Shared bits for text pages |
| 13614 | LOG page |
| 13615 | Groups panel: each group, live status, and what it searches for |
| 13910 | SETUP page |
| 14130 | Help |
| 14189 | Storage (internal flash) |
| 14321 | Share portal |
| 14345 | Boot: radar ping + wordmark |
| 14347 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 14457 | Features that need screen / UI state |
| 14465 | Find / tracker banner (slides in under the header, tap = track it) |
| 14504 | Alerts (find + tracker + battery) |
| 14575 | Scenes (one tap arms a useful set of groups) |
| 14595 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 14656 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 14699 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 14914 | DIRECTION SWEEP (beta) |
| 16041 | Terminal look of AIM: the scope with the real bearing (once calibrated) |
| 16836 | One call from the main loop |
| 16851 | Banner, session report, self-test drawing |
| 16875 | Session report: totals, per-group bars, last-60-minutes chart |
| 16979 | Self-test: every subsystem at a glance, live |
| 17094 | Power screen: two large buttons, so shutting down is easy on a small screen |
| 17127 | Auto-rotate calibration wizard |
| 17177 | Render driver (strip by strip) |
| 17225 | Watch face (BOOT short press): an analog watch, the clock chip's time, the boot logo in the middle |
| 17386 | Terminal theme: the watch face (a wrist-unit clock: 7-segment time inside a scanner bezel) |
| 17628 | Sweep screen, baseline files, walkthrough rooms, hunt upgrades |
| 18785 | Actions |
| 19236 | Touch state machine |
| 19594 | Setup / loop |
