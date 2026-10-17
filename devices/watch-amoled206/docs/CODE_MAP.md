# Code map

`src/HUNTR-SEEKR_Watch_AMOLED206/HUNTR-SEEKR_Watch_AMOLED206.ino` is a single Arduino sketch of about 20,200 lines. The Arduino IDE builds every `.ino` in a folder as one unit and generates function prototypes for it, so the types come first and the sketch stays in one file.

Beside it are `Fonts_Standard.h` and `Fonts_Terminal.h`: generated glyph data (see [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), not meant to be edited by hand.

The sections below are the `// ----` headings in the sketch, in file order.

| Line | Section |
|---:|---|
| 74 | Terminal theme: palette, fonts, helpers |
| 89 | Pins (Waveshare verified pin map for this board) |
| 161 | What the shared interface code needs to know about this board |
| 219 | Types (must precede every function: Arduino IDE auto-prototypes) |
| 356 | Forward declarations |
| 419 | Panel (CO5300 over QSPI) |
| 430 | Strip renderer state |
| 538 | Palette |
| 622 | Primitives (integer only) |
| 797 | Terminal theme: panel shape and the CRT pass |
| 1015 | Text (anti-aliased) |
| 1763 | Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs |
| 1955 | Helpers |
| 2310 | Log queue (filled from radio callbacks, drained in loop) |
| 2323 | Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name |
| 2514 | Brightness + sleep (AMOLED: panel register 0x51, display on/off 0x29/0x28) |
| 2538 | GPS + log writer (loop context only) |
| 2661 | Detection callbacks |
| 2664 | Bluetooth / Wi-Fi device recognition |
| 7295 | Rogue Wi-Fi checks (only the sniffer touches these tables) |
| 7396 | Sweep tools (walkthrough marks, baseline-then-sweep) and hunt upgrades |
| 8207 | 802.15.4 (Zigbee + Thread) receive path |
| 8434 | Radio control |
| 9139 | Share portal state |
| 9177 | Share portal (WPA2) |
| 9349 | Storage / sessions |
| 9426 | Preferences |
| 9525 | I2C helpers |
| 9560 | FT3168 capacitive touch -- follows Waveshare's working demo exactly |
| 9809 | AXP2101 power management and the two side buttons |
| 10199 | Audio: ES8311 codec + speaker amplifier (beeps) |
| 10709 | Core logic (no screen state in here) |
| 10712 | Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) |
| 10830 | Session statistics + 60-minute timeline |
| 10904 | Real-time clock (PCF85063 @0x51) |
| 11024 | Custom signatures (/sigs.txt: one rule per line) |
| 11211 | Drone Remote ID decoding (ASTM F3411 / OpenDroneID) |
| 11270 | What the alert is about (captured under the mux by upsert) |
| 11281 | A different tune for each group (3 notes: tone, tone/gap, tone) |
| 11312 | Tracker "following you": seen continuously for followMin minutes |
| 11340 | Scenes (one tap arms a useful set of groups) |
| 11357 | Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family |
| 11364 | 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard |
| 11376 | Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot) |
| 11635 | Findings record (what was found this session, saved on demand) |
| 11993 | Boot sound (WATCH): your own short sound on the boot screen |
| 12228 | Boot logo (all boards): your own picture on the boot screen |
| 12574 | UI |
| 12729 | Pill (status; tap = pause / resume) + battery |
| 12847 | Bottom bars |
| 12979 | Tiles (ARM page) |
| 13085 | Find list (FINDS page + pushed filtered list) |
| 13349 | Pushed, filtered list (long-press a tile) |
| 13358 | Hunt |
| 13361 | Terminal theme: the motion-tracker view of Track |
| 13569 | HUD theme: the blue HUD tracker of the Track screen |
| 13890 | Shared bits for text pages |
| 13951 | LOG page |
| 13952 | Groups panel: each group, live status, and what it searches for |
| 14247 | SETUP page |
| 14467 | Help |
| 14526 | Storage (internal flash) |
| 14658 | Share portal |
| 14682 | Boot: radar ping + wordmark |
| 14684 | Terminal theme: boot screen (a terminal start-up with the scope) |
| 14794 | Features that need screen / UI state |
| 14802 | Find / tracker banner (slides in under the header, tap = track it) |
| 14841 | Alerts (find + tracker + battery) |
| 14912 | Scenes (one tap arms a useful set of groups) |
| 14932 | Discreet mode + power saver (display timers, brightness, BLE duty) |
| 14993 | Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause |
| 15036 | Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' |
| 15251 | DIRECTION SWEEP (beta) |
| 16378 | Terminal look of AIM: the scope with the real bearing (once calibrated) |
| 17177 | One call from the main loop |
| 17192 | Banner, session report, self-test drawing |
| 17216 | Session report: totals, per-group bars, last-60-minutes chart |
| 17320 | Self-test: every subsystem at a glance, live |
| 17435 | Power screen: two large buttons, so shutting down is easy on a small screen |
| 17468 | Auto-rotate calibration wizard |
| 17518 | Render driver (strip by strip) |
| 17566 | Watch face (BOOT short press): an analog watch, the clock chip's time, the boot logo in the middle |
| 17727 | Terminal theme: the watch face (a wrist-unit clock: 7-segment time inside a scanner bezel) |
| 17969 | Sweep screen, baseline files, walkthrough rooms, hunt upgrades |
| 19126 | Actions |
| 19577 | Touch state machine |
| 19935 | Setup / loop |
