# Board configuration - Waveshare ESP32-S3-Touch-LCD-1.69

This board needs no display set-up file: GFX Library for Arduino is configured from the sketch itself.

## Arduino IDE settings

- Board: "ESP32S3 Dev Module"
- USB CDC On Boot: "Enabled"
- PSRAM: "OPI PSRAM"
- Flash Size: "16MB (128Mb)"
- Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)"

(`arduino-cli` users: `--fqbn espressif:esp32:esp32s3:CDCOnBoot=cdc,PSRAM=opi,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB`)

## Board definitions

The pins, bus speeds and screen geometry are `#define`s in the *Pins* section of `src/HUNTR-SEEKR_S3_LCD169/HUNTR-SEEKR_S3_LCD169.ino`. Change them there if your hardware differs. The values in use are:

| Name | Value | Note |
|---|---|---|
| `HAS_ZB` | `0` | the ESP32-S3 has no 802.15.4 radio: 8 groups, no Zigbee/Thread code is built |
| `HAS_5G` | `0` | the ESP32-S3 is a 2.4 GHz-only chip: no 5 GHz channel scan, no band switching |
| `FS_LABEL` | `"flash"` |  |
| `SAVETO_ROW` | `0` | no SD card on this board: no "Save to" choice |
| `PIN_LCD_DC` | `4` |  |
| `PIN_LCD_CS` | `5` |  |
| `PIN_LCD_SCK` | `6` |  |
| `PIN_LCD_MOSI` | `7` |  |
| `PIN_LCD_RST` | `8` |  |
| `PIN_LCD_BL` | `15` |  |
| `PIN_SYS_EN` | `41` | power latch: must be driven high |
| `PIN_TP_SDA` | `11` | shared I2C: CST816T touch 0x15, QMI8658C IMU 0x6B, PCF85063 RTC 0x51 |
| `PIN_TP_SCL` | `10` |  |
| `PIN_TP_RST` | `13` |  |
| `PIN_TP_INT` | `14` |  |
| `CST816_ADDR` | `0x15` |  |
| `TP_I2C_HZ` | `400000` |  |
| `PIN_BAT_ADC` | `1` | VBAT = VADC x 3 (200k / 100k divider) |
| `PIN_GPS_RX` | `44` | optional GPS on the UART pads |
| `PIN_GPS_TX` | `43` |  |
| `PIN_BUZZER` | `42` | on-board buzzer |
| `SPI_HZ` | `80000000UL` | if you ever see snow/noise, drop to 40000000UL |
| `BAND_H` | `32` | strip height: 280*32*2 = 17.5 KB |
| `DIM_AFTER_MS` | `60000UL` | dim after this much idle (only when Auto-dim is switched on in Setup) ... |
| `SLEEP_AFTER_MS` | `120000UL` | ... backlight OFF after this. Touch wakes it. |
| `SAFE_X` | `0` |  |
| `SAFE_TOP` | `0` |  |
| `SAFE_BOT` | `0` |  |
| `CORNER_R` | `34` | the glass has rounded corners; physical px (this panel is drawn 1:1). Tune here. |

Hardware: 240 x 280 IPS (ST7789V2), CST816T touch, QMI8658C motion sensor, PCF85063 clock, buzzer.
