# Board configuration - Waveshare ESP32-C6-Touch-LCD-1.47

This board needs no display set-up file: GFX Library for Arduino is configured from the sketch itself.

## Arduino IDE settings

- Board: "ESP32C6 Dev Module"
- USB CDC On Boot: "Enabled"
- Flash Size: "4MB (32Mb)"
- Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"

(`arduino-cli` users: `--fqbn espressif:esp32:esp32c6:CDCOnBoot=cdc,FlashSize=4M,PartitionScheme=huge_app`)

The 4 MB layout is used on purpose even though the board has 8 MB of flash: the same file then works on 4 MB and 8 MB boards, and it survives flashers that rewrite the image to the flash size they detect. The firmware uses about 2 MB of the 3 MB application slot. Over-the-air update is not used on this board.

## Board definitions

The pins, bus speeds and screen geometry are `#define`s in the *Pins* section of `src/HUNTR-SEEKR_C6_LCD147/HUNTR-SEEKR_C6_LCD147.ino`. Change them there if your hardware differs. The values in use are:

| Name | Value | Note |
|---|---|---|
| `PIN_LCD_SCK` | `1` |  |
| `PIN_LCD_MOSI` | `2` |  |
| `PIN_LCD_CS` | `14` |  |
| `PIN_LCD_DC` | `15` |  |
| `PIN_LCD_RST` | `22` |  |
| `PIN_LCD_BL` | `23` |  |
| `PIN_TP_SDA` | `18` |  |
| `PIN_TP_SCL` | `19` |  |
| `PIN_TP_RST` | `20` |  |
| `PIN_TP_INT` | `21` |  |
| `TP_ADDR` | `0x63` | AXS5106L |
| `TP_I2C_HZ` | `100000` | touch bus speed (raise to 400000 once touch works) |
| `PIN_BAT_ADC` | `0` | VBAT = VADC x 3 |
| `PIN_GPS_RX` | `17` | UART0 header (optional GPS) |
| `PIN_GPS_TX` | `16` |  |
| `PIN_BUZZER` | `7` | passive buzzer: + to IO7 (header pad), - to GND. GPIO7 is the one header pin Waveshare lists as free. Set -1 if no buzzer fitted. |
| `HAS_5G` | `0` | the ESP32-C6 is a 2.4 GHz-only chip (Wi-Fi 6 on 2.4 GHz): no 5 GHz channel scan, no band switching |
| `FS_LABEL` | `"flash"` |  |
| `SAVETO_ROW` | `0` | no SD card on this board: no "Save to" choice |
| `SPI_HZ` | `40000000UL` | try 80000000UL for a faster push; drop back if you see noise |
| `BAND_H` | `32` | strip height: 320*32*2 = 20 KB |
| `DIM_AFTER_MS` | `30000UL` | dim after this much idle (only when Auto-dim is on) ... |
| `SLEEP_AFTER_MS` | `120000UL` | ... backlight OFF after this. Touch wakes it. |
| `SAFE_X` | `0` | this panel has square corners: no corner clearance needed |
| `SAFE_TOP` | `0` |  |
| `SAFE_BOT` | `0` |  |
| `CORNER_R` | `0` |  |
| `OPT_BOOT_BUTTON` | `0` | (the S3 build counts the BOOT button; not wired into this build) |
| `PERF_LOG` | `0` |  |
| `ENABLE_BLE` | `1` | 0 = never start BLE (test whether BLE scanning is crowding out Wi-Fi on the shared radio) |
| `RADIO_DEBUG` | `0` | 1 = print a radio status line every 2 s (frames seen per radio) |
| `TOUCH_DEBUG` | `0` | 1 = serial dump of raw touch bytes + a teal crosshair drawn at every touch (leave 0 for normal use) |

Hardware: 172 x 320 IPS (JD9853), AXS5106L touch, QMI8658A motion sensor, 802.15.4 radio.
