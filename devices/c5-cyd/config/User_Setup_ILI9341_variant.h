// ===========================================================================================================
//  ONLY IF THE ST7789 FILE GIVES A BLACK / WHITE / GARBLED SCREEN.
//  The board maker says the panel "can also be changed to the ILI9341 driver": some units are sold that way.
//  Use this file INSTEAD of User_Setup.h (same place, same name once copied). Provided for units sold with that panel.
//  If colours look inverted with this driver, add:   #define TFT_INVERSION_ON
//  If red and blue are swapped, add:                 #define TFT_RGB_ORDER TFT_BGR
// ===========================================================================================================
//  Part of HUNTR/SEEKR (GNU GPL v3, see LICENSE). Copyright (C) 2026 SKIrf Labs.
#define USER_SETUP_INFO "NM-CYD-C5 (ILI9341)"

#define ILI9341_DRIVER
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

#define TFT_MISO  2
#define TFT_MOSI  7
#define TFT_SCLK  6
#define TFT_CS    23
#define TFT_DC    24
#define TFT_RST   -1
#define TFT_BL    25
#define TFT_BACKLIGHT_ON HIGH
#define TOUCH_CS  1

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SPI_FREQUENCY       20000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
