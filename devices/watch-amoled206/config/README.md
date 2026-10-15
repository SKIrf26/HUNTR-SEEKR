# Board configuration - Waveshare ESP32-C6-Touch-AMOLED-2.06

This board needs no display set-up file: GFX Library for Arduino is configured from the sketch itself.

## Arduino IDE settings

- Board: "ESP32C6 Dev Module"
- USB CDC On Boot: "Enabled"
- Flash Size: "16MB (128Mb)"
- Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)"

(`arduino-cli` users: `--fqbn espressif:esp32:esp32c6:CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB`)

## Board definitions

The pins, bus speeds and screen geometry are `#define`s in the *Pins* section of `src/HUNTR-SEEKR_Watch_AMOLED206/HUNTR-SEEKR_Watch_AMOLED206.ino`. Change them there if your hardware differs. The values in use are:

| Name | Value | Note |
|---|---|---|
| `PIN_LCD_SCLK` | `0` |  |
| `PIN_LCD_D0` | `1` |  |
| `PIN_LCD_D1` | `2` |  |
| `PIN_LCD_D2` | `3` |  |
| `PIN_LCD_D3` | `4` |  |
| `PIN_LCD_CS` | `5` |  |
| `PIN_LCD_RST` | `11` |  |
| `LCD_XOFF` | `22` | CO5300 column offset for this 410-wide panel. If the picture is shifted or cut at an edge, try 0. |
| `PIN_I2C_SDA` | `8` | shared: FT3168 touch 0x38, AXP2101 0x34, QMI8658 0x6B, PCF85063 0x51 |
| `PIN_I2C_SCL` | `7` |  |
| `PIN_TP_RST` | `10` |  |
| `PIN_TP_INT` | `15` |  |
| `TP_ADDR` | `0x38` | FT3168 |
| `PMU_ADDR` | `0x34` | AXP2101 |
| `PIN_GPS_RX` | `-1` | no GPS wired: the UART pads' GPIOs are not in the pin map, set both to use one |
| `PIN_GPS_TX` | `-1` |  |
| `HAS_SOUND` | `1` | ES8311 codec + speaker amplifier (see audio206 section) |
| `PIN_I2S_BCLK` | `20` | Waveshare verified pin map |
| `PIN_I2S_LRCK` | `22` |  |
| `PIN_I2S_DOUT` | `23` | DAC data (playback) |
| `PIN_PA` | `6` | speaker amplifier enable |
| `PIN_BOOT` | `9` | side button BOOT (Waveshare pinout: GPIO9, the boot-mode pin: an ordinary input once the sketch runs) |
| `PIN_PWR` | `18` | side button PWR (Waveshare pinout: GPIO18; it also goes to the power chip, which switches the watch off after 6 s) |
| `QSPI_HZ` | `40000000UL` |  |
| `BAND_H` | `24` | strip height (even, the QSPI panel wants even windows): 410*24*2 = 19.7 KB |
| `DIM_AFTER_MS` | `30000UL` | dim after this much idle ... |
| `SLEEP_AFTER_MS` | `120000UL` | ... display OFF after this (AMOLED burn-in + battery). Touch wakes it. |
| `SAFE_X` | `14` | logical px kept clear at the left/right ends of the header and the bottom bars (rounded screen corners) |
| `SAFE_TOP` | `16` | header content starts this far down (logical px) |
| `SAFE_BOT` | `14` | bottom bars sit this far above the bottom edge |
| `CORNER_R` | `140` | radius of the screen's rounded corners in PHYSICAL px (about 135, plus a margin). Tune here. |

Hardware: 410 x 502 AMOLED (CO5300), FT3168 touch, AXP2101 power chip, QMI8658 motion sensor, PCF85063 clock, ES8311 speaker.
