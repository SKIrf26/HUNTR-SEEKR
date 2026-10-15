# NM-CYD-C5: setting up the screen (TFT_eSPI)

Use this only if you build the sketch yourself. (The ready-made `.bin` needs none of this.)

A black screen after uploading almost always means the TFT_eSPI display library is not set up for this board. The sketch cannot set it up by itself: TFT_eSPI reads its pins and display driver from a file inside the *library*.

You need **both** of these:

## 1. A TFT_eSPI that supports the ESP32-C5

The stock library from the Library Manager does not. The board maker supplies the changed files:
<https://github.com/RockBase-iot/NM-CYD-C5> -> `Demos/Arduino/libraries/TFT_eSPI`

That folder may hold only the changed files (for example `Processors/TFT_eSPI_ESP32_C5.c` and `.h`, `TFT_eSPI.cpp`, `TFT_eSPI.h`, `User_Setup-NM-CYD-C5.h`) rather than a complete library. So:

1. Install the normal TFT_eSPI (by Bodmer) first, from the Arduino Library Manager, if it is not there.
2. On the maker's page choose **Code > Download ZIP**, unzip it, open `Demos/Arduino/libraries/TFT_eSPI` and copy *its contents* into `Documents/Arduino/libraries/TFT_eSPI/`, answering "replace" when asked.

If the folder you downloaded is a complete library (it contains `User_Setup_Select.h`, a `Fonts` folder, etc.) then replacing the whole TFT_eSPI folder is also fine. Use Arduino-ESP32 core 3.3.5 or newer.

## 2. The right User_Setup.h

Copy `User_Setup.h` from this folder over `Documents/Arduino/libraries/TFT_eSPI/User_Setup.h`. (If that library folder's `User_Setup_Select.h` points somewhere else, make it use `User_Setup.h`.) It selects the ST7789 driver, 240 x 320, colour inversion off, and these pins:

| Signal | Pin |
|---|---|
| SCLK | 6 |
| MOSI | 7 |
| MISO | 2 |
| CS | 23 |
| DC | 24 |
| RST | none |
| Backlight | 25 |
| Touch CS | 1 |
| SD card CS | 10 |

Then close the Arduino IDE completely, reopen it, open the sketch and upload again.

The sketch checks this when it is built: if TFT_eSPI is not set up for this board, the build stops with a message that says what is wrong, instead of producing a black screen.

## The files in this folder

| File | Use |
|---|---|
| `User_Setup.h` | The normal set-up: ST7789 panel, 20 MHz display clock |
| `User_Setup_40MHz.h` | Same, with a 40 MHz display clock (optional, faster drawing) |
| `User_Setup_ILI9341_variant.h` | For units sold with an ILI9341 panel |

To use one of the other two, copy it over `User_Setup.h` in the library and **keep the name `User_Setup.h`**.

## If the picture appears but the colours are wrong

The easy fix needs no rebuild: open **SETUP > Screen colors** on the board and tap it until the colours look right (Swap R/B, Invert, or both). The choice is kept. The notes below are for building your own.

- Red and blue swapped: in `User_Setup.h` remove the `//` before `#define TFT_RGB_ORDER TFT_BGR`.
- Everything looks like a photo negative: make sure `#define TFT_INVERSION_OFF` is present (it is, in this file).

## If the screen stays black or shows noise

- Some units are sold with an ILI9341 panel. Copy `User_Setup_ILI9341_variant.h` over `User_Setup.h` and upload again.
- Open the Serial Monitor at 115200 baud and press the board's reset button: the sketch prints its start-up lines. If you see them, the sketch is running and the display set-up is the problem. If you see nothing, check the cable, the board selection (ESP32C5 Dev Module) and USB CDC On Boot = Enabled.
- `User_Setup_40MHz.h` draws faster. If the picture turns to noise, flickers or has wrong colours, put the normal `User_Setup.h` back.

## Advanced

To skip the sketch's build-time check, add the line `#define SKI_SKIP_TFT_CHECK` at the top of the sketch.
