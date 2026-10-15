# Board configuration - Waveshare ESP32-C6-LCD-1.47 (no touch)

This board needs no display set-up file: GFX Library for Arduino is configured from the sketch itself.

## Arduino IDE settings

- Board: "ESP32C6 Dev Module"
- USB CDC On Boot: "Enabled"
- Flash Size: "4MB (32Mb)"
- Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"

(`arduino-cli` users: `--fqbn espressif:esp32:esp32c6:CDCOnBoot=cdc,FlashSize=4M,PartitionScheme=huge_app`)

The firmware uses about 1.9 MB of the 3 MB application slot. Over-the-air update is not used on this board.

## Board definitions

The pins, bus speeds and screen geometry are `#define`s in the *Pins* section of `src/HUNTR-SEEKR_C6_LCD147_BUTTON/HUNTR-SEEKR_C6_LCD147_BUTTON.ino`. Change them there if your hardware differs. The values in use are:

| Name | Value | Note |
|---|---|---|
| `PIN_LCD_MOSI` | `6` | the LCD and the TF card slot share SCLK and MOSI |
| `PIN_LCD_SCK` | `7` |  |
| `PIN_LCD_CS` | `14` |  |
| `PIN_LCD_DC` | `15` |  |
| `PIN_LCD_RST` | `21` |  |
| `PIN_LCD_BL` | `22` | backlight, HIGH = on |
| `PIN_SD_CS` | `4` | TF card chip-select (the card is not used: its select line is held HIGH so it never answers on the display's lines) |
| `PIN_LED_RGB` | `8` | the on-board WS2812 RGB LED |
| `PIN_BTN_BOOT` | `9` | BOOT button, to GND when pressed (RESET only restarts the chip: the app cannot read it) |
| `PIN_BTN_EXT1` | `-1` | OPTIONAL extra button between a free header pin and GND: NEXT (BOOT then means BACK) |
| `PIN_BTN_EXT2` | `-1` | OPTIONAL extra button: SELECT. Free pins: IO0 IO1 IO2 IO3 IO18 IO19 IO20 IO23 |
| `PIN_BAT_ADC` | `-1` | no battery measurement on this board |
| `PIN_GPS_RX` | `17` | UART0 header (optional GPS) |
| `PIN_GPS_TX` | `16` |  |
| `PIN_BUZZER` | `-1` | no speaker on this board; wire a passive buzzer between a free header pin (for example IO3) and GND and put that number here |
| `HAS_5G` | `0` | the ESP32-C6 is a 2.4 GHz-only chip: no 5 GHz channel scan |
| `FS_LABEL` | `"flash"` |  |
| `SAVETO_ROW` | `0` | no SD card is used: no "Save to" choice |
| `LCD_IPS` | `1` | 1 = colours inverted the way an IPS ST7789 needs. If the picture looks like a photo negative, set 0. |
| `LCD_COL_OFS` | `34` | a 172 px wide panel sits 34 columns into the controller's 240 px memory. If the picture is shifted sideways, change this. |
| `LED_LEVEL` | `64` | RGB LED brightness cap, 0..255 (the LED is very bright through the clear cover) |
| `SPI_HZ` | `40000000UL` | try 80000000UL for a faster push; drop back if you see noise |
| `BAND_H` | `32` | strip height: 320*32*2 = 20 KB |
| `DIM_AFTER_MS` | `30000UL` | dim after this much idle (only in Discreet mode) ... |
| `SLEEP_AFTER_MS` | `120000UL` | ... backlight OFF after this. Any button press wakes it. |
| `BTN_SEL_MS` | `500` | a press at least this long (ms) is HOLD = SELECT |
| `BTN_BACK_MS` | `1500` | a press at least this long (ms) is LONG = BACK |
| `ENABLE_BLE` | `1` | 0 = never start BLE (test whether BLE scanning is crowding out Wi-Fi on the shared radio) |
| `RADIO_DEBUG` | `0` | 1 = print a radio status line every 2 s (frames seen per radio) |

Hardware: 172 x 320 IPS (ST7789), BOOT button, WS2812 RGB LED, TF card slot (not used), 802.15.4 radio. No touch panel, no motion sensor, no battery gauge.
