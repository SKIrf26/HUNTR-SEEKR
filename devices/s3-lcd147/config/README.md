# Board configuration - Waveshare ESP32-S3-Touch-LCD-1.47

This board needs no display set-up file: GFX Library for Arduino is configured from the sketch itself.

## Arduino IDE settings

- Board: "ESP32S3 Dev Module"
- USB CDC On Boot: "Enabled"
- PSRAM: "OPI PSRAM"
- Flash Size: "16MB (128Mb)"
- Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)"

(`arduino-cli` users: `--fqbn espressif:esp32:esp32s3:CDCOnBoot=cdc,PSRAM=opi,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB`)

## Board definitions

The pins, bus speeds and screen geometry are `#define`s in the *Pins* section of `src/HUNTR-SEEKR_S3_LCD147/HUNTR-SEEKR_S3_LCD147.ino`. Change them there if your hardware differs. The values in use are:

| Name | Value | Note |
|---|---|---|
| `HAS_ZB` | `0` | the ESP32-S3 has no 802.15.4 radio: 8 groups, no Zigbee/Thread code is built |
| `HAS_5G` | `0` | the ESP32-S3 is a 2.4 GHz-only chip: no 5 GHz channel scan, no band switching |
| `HAS_IMU_BOARD` | `1` | a motion sensor (QMI8658) is looked for at start-up; AIM is shown only if one answers |
| `FS_LABEL` | `"flash"` |  |
| `SAVETO_ROW` | `0` | no SD card on this board: no "Save to" choice |
| `PIN_LCD_SCK` | `38` |  |
| `PIN_LCD_MOSI` | `39` |  |
| `PIN_LCD_CS` | `21` |  |
| `PIN_LCD_DC` | `45` |  |
| `PIN_LCD_RST` | `40` | per the schematic |
| `PIN_LCD_BL` | `46` | through a transistor, high = on |
| `PIN_TP_SDA` | `42` | touch bus: AXS5106L touch controller at 0x63 |
| `PIN_TP_SCL` | `41` |  |
| `PIN_TP_RST` | `47` |  |
| `PIN_TP_INT` | `48` |  |
| `TP_ADDR` | `0x63` | AXS5106L |
| `TP_I2C_HZ` | `100000` |  |
| `PIN_BAT_ADC` | `12` | VBAT = VADC x 3 (200k / 100k divider) |
| `PIN_GPS_RX` | `44` | optional GPS on the UART pads |
| `PIN_GPS_TX` | `43` |  |
| `PIN_BUZZER` | `4` | optional passive buzzer on header pin IO4; set to `-1` if none is fitted |
| `PIN_BOOT` | `0` | the BOOT button wakes a dimmed screen; held about 2 s it sleeps the board |
| `SPI_HZ` | `40000000UL` | if you ever see snow/noise, lower it |
| `PANEL_W` x `PANEL_H` | `172` x `320` | native panel size, portrait |
| `BAND_H` | `32` | strip height: 320*32*2 = 20 KB |
| `DIM_AFTER_MS` | `30000UL` | dim after this much idle (only when Auto-dim is switched on in Setup) ... |
| `SLEEP_AFTER_MS` | `120000UL` | ... backlight OFF after this. Touch wakes it. |
| `SAFE_X`, `SAFE_TOP`, `SAFE_BOT`, `CORNER_R` | `0` | this panel has square corners: no corner clearance |

Hardware: 172 x 320 IPS (JD9853), AXS5106L touch, ESP32-S3R8 (16 MB flash, 8 MB PSRAM), optional buzzer. No motion sensor and no clock chip are listed for this board.
