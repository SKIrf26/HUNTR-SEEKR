# Board configuration: Waveshare ESP32-Touch-LCD-3.5

This board needs no display-library set-up files. The pins and screen settings are `#define`s at the top of the sketch
(`src/HUNTR-SEEKR_ESP32_LCD35/HUNTR-SEEKR_ESP32_LCD35.ino`, section "Pins"), and the display driver is **GFX Library for Arduino**
(`Arduino_ST7796`), which is configured in code.

## Pin map used by the sketch

| Part | Pins |
|---|---|
| LCD (ST7796S, 320 x 480, SPI) | DC 27, CS 5, SCK 18, MOSI 23, MISO 19; backlight GPIO 25 (PWM); reset is on the I/O expander |
| TF card slot | CS 15, on the same SPI bus as the LCD (SCK 18, MOSI 23, MISO 19) |
| I2C bus | SDA 21, SCL 22 |
| Touch (FT6336, I2C 0x38) | polled over I2C; the interrupt line is not used; reset is on the I/O expander |
| I/O expander (TCA9554, I2C 0x20) | pin 0 = LCD reset, pin 1 = touch reset, pin 2 = speaker amplifier enable |
| Power chip (AXP2101, I2C 0x34) | battery level, PWR key events |
| Real-time clock (PCF85063, I2C 0x51) | clock |
| Audio codec (ES8311, I2C 0x18) | I2S: BCLK 2, LRCK 4, DOUT 12 (no MCLK) |
| Buttons | BOOT = GPIO 0; PWR is read through the power chip |
| GPS | not wired: `PIN_GPS_RX` / `PIN_GPS_TX` are -1 (see below) |

## Things you may want to change (all near the top of the sketch)

| Setting | Default | What it does |
|---|---|---|
| `PIN_GPS_RX`, `PIN_GPS_TX` | -1 (off) | Set to the header pins you wire a 9600-baud GPS module to. GPIO 13, 14, 26, 32 and 33 are not used by the board. |
| `TP_SWAP_XY`, `TP_FLIP_X`, `TP_FLIP_Y` | 0, 0, 0 | If a tap lands mirrored or swapped, set the matching one to 1 and rebuild. |
| `CORNER_R` | 36 | Radius of the rounded screen corners, in pixels. Content is kept out of the corners. |
| `BAND_H` | 16 | Height of the drawing strip. Larger is a little faster and uses more memory. |
| `LCD_HZ` | 40000000 | SPI clock for the display. |

If the picture colours look inverted, change the `true` (IPS flag) in the `new Arduino_ST7796(...)` line to `false`.
