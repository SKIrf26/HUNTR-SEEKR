# ESP32-2432S028: setting up the screen (TFT_eSPI)

A black or white screen after uploading almost always means the TFT_eSPI display library is not set up for this board. The sketch cannot set it up by itself: TFT_eSPI reads its pins and display driver from a file inside the *library*. (The sketch checks this when it is built and stops with a message if it is wrong.)

1. Install TFT_eSPI (by Bodmer) from the Arduino Library Manager. The normal release is fine; this board needs no patched copy.
2. Copy `User_Setup.h` from this folder over `Documents/Arduino/libraries/TFT_eSPI/User_Setup.h`. (If that library folder's `User_Setup_Select.h` points somewhere else, make it use `User_Setup.h`.) It selects the ILI9341 driver, 240 x 320, the HSPI bus and these pins:

   | Signal | Pin |
   |---|---|
   | SCLK | 14 |
   | MOSI | 13 |
   | MISO | 12 |
   | CS | 15 |
   | DC | 2 |
   | RST | none |
   | Backlight | 21 (driven by the sketch) |

3. Close the Arduino IDE completely, reopen it, open the sketch and upload again.

Other pins used by the sketch (touch, SD card, speaker, GPS) are listed in the *Pins* section of the sketch.

## If the picture appears but the colours are wrong

- Photo negative: in `User_Setup.h` remove the `//` before `#define TFT_INVERSION_ON`.
- Red and blue swapped: remove the `//` before `#define TFT_RGB_ORDER TFT_BGR`.

## If the screen stays black or shows noise

- Units with two USB sockets usually carry an ST7789 panel. Copy `User_Setup_ST7789_variant.h` over `User_Setup.h` (**keep the name `User_Setup.h`**) and upload again.
- Open the Serial Monitor at 115200 baud and press the board's EN (reset) button: the sketch prints its start-up lines. If you see them, the sketch is running and the display set-up is the problem. If you see nothing, check the cable and the board selection (ESP32 Dev Module).
- If the picture is noisy or flickers, lower `SPI_FREQUENCY` in `User_Setup.h` from 40000000 to 27000000.

## The files in this folder

| File | Use |
|---|---|
| `User_Setup.h` | The normal set-up: ILI9341 panel, 40 MHz display clock |
| `User_Setup_ST7789_variant.h` | For units sold with an ST7789 panel |

## Advanced

To skip the sketch's build-time check, add the line `#define SKI_SKIP_TFT_CHECK` at the top of the sketch.
