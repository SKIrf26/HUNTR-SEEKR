# HUNTR/SEEKR for the Waveshare ESP32-Touch-LCD-3.5 - Port Test 6

First build for the **Waveshare ESP32-Touch-LCD-3.5** (320 x 480 rounded-corner touch screen). It is the current HUNTR/SEEKR firmware adapted to this board.

> **Not yet tried on the real board.** It compiles and follows Waveshare's pin map and examples, but nobody has run it on the hardware yet.

## Highlights
- Passive scanning of Wi-Fi, Bluetooth LE, and Drone Remote ID
- Eight groups (ALPR, AXON, CAMS, GLASS, DRONE, BT, TAGS, WIFI) with signal strength
- Track screen with a live dBm display and audible feedback
- Rogue Wi-Fi alerts and Bluetooth spam detection
- Wide maker lists including camera, doorbell and tracker makers, gunshot sensors, police body cameras beyond Axon and Flipper Zero detection, and your own rules
- Tracker "following" alert, possible card-skimmer tags, session report, logs on a TF card, ignore list
- Phone page over the board's own Wi-Fi (live view, clock sync, boot sound)

## Install
Download a `.bin` below and write it at `0x0`:

```
esptool --chip esp32 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_ESP32_LCD35_Port_Test_6.bin
```

This resets saved settings; logs on a TF card stay.

## Files
| File | Purpose |
|---|---|
| `HUNTR-SEEKR_ESP32_LCD35_Port_Test_6.bin` | 4 MB layout image, write at `0x0` (works with the web flasher) |
| `HUNTR-SEEKR_ESP32_LCD35_Port_Test_6_16MB.bin` | 16 MB layout image, write at `0x0` (esptool / Arduino IDE; phone updates) |
| `SHA256SUMS.txt` | Checksums |

## New in Test 4
- The Track screens use the whole 320 x 480 / 480 x 320 panel: bigger gauge or scope, bigger digits and a signal history graph where there used to be empty space.

## Please know
This is a **test port**. Things to watch for: whether a tap lands where the finger is (touch direction), the screen corners, colours, sound, and the TF card. Signal strength is only a rough guide to distance. Detection lists are not exhaustive and **results are hints, not proof**.

Use it responsibly and within your local laws. See the README.

Please report problems with the **Bug report** form on the Issues tab, with the Serial Monitor output if you can.

## Thanks
Thanks to Alfa ([@AlfaBeta2000](https://github.com/AlfaBeta2000)), who reported the Watch's slowdowns and spontaneous reboots ([#4](https://github.com/SKIrf26/HUNTR-SEEKR/issues/4)), traced them in the serial monitor to Bluetooth duplicate reports and found the fix that is now in every board's firmware. See [CONTRIBUTORS.md](CONTRIBUTORS.md).
