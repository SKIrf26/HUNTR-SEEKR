// ===========================================================================================================
//  User_Setup.h  for the ESP32-2432S028 ("cheap yellow display", classic ESP32)  -  HUNTR/SEEKR
//  Default panel: ILI9341, 2.8 inch, 240 x 320 (used landscape by the sketch).
//
//  Where it goes: replace  Documents/Arduino/libraries/TFT_eSPI/User_Setup.h  with this file, then restart the
//  Arduino IDE. Normal TFT_eSPI from the Library Manager is fine for this board (no patched copy needed).
// ===========================================================================================================
//  Part of HUNTR/SEEKR (GNU GPL v3, see LICENSE). Copyright (C) 2026 SKIrf Labs.
#define USER_SETUP_INFO "ESP32-2432S028 (ILI9341)"

#define ILI9341_2_DRIVER          // the variant of the ILI9341 driver that this board is normally used with
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

// If colours look like a photo negative, remove the // on the next line:
// #define TFT_INVERSION_ON
// If RED and BLUE are swapped, remove the // on the next line:
// #define TFT_RGB_ORDER TFT_BGR

// ---- pins: the display sits on the HSPI bus (touch and SD card are handled by the sketch itself) ----
#define TFT_MISO  12
#define TFT_MOSI  13
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   -1      // the display reset is tied to the board reset
// TFT_BL (backlight, IO21) is deliberately NOT set here: the sketch dims the backlight itself and keeps it off
// until the first picture is ready, so the screen does not flash on at power-up.
#define USE_HSPI_PORT     // required: the SD card uses the other SPI bus

// ---- fonts the library may load (the sketch draws with its own fonts; these are just the standard ones) ----
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
