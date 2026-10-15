// ===========================================================================================================
//  ONLY IF THE ILI9341 FILE GIVES A BLACK / WHITE / GARBLED SCREEN.
//  Some ESP32-2432S028 units (typically the ones with BOTH a micro-USB and a USB-C socket) are sold with an
//  ST7789 panel instead of the ILI9341. Use this file INSTEAD of User_Setup.h (copy it over
//  Documents/Arduino/libraries/TFT_eSPI/User_Setup.h and KEEP THE NAME User_Setup.h).
//  Settings are from community notes for that board revision (ST7789, no colour inversion, normal colour order);
//  they have not been tried on a unit here.
//  Wrong colours?   Photo negative: swap  TFT_INVERSION_OFF  for  TFT_INVERSION_ON.
//                   Red and blue swapped: remove the // before  TFT_RGB_ORDER.
// ===========================================================================================================
//  Part of HUNTR/SEEKR (GNU GPL v3, see LICENSE). Copyright (C) 2026 SKIrf Labs.
#define USER_SETUP_INFO "ESP32-2432S028 (ST7789)"

#define ST7789_DRIVER
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

#define TFT_INVERSION_OFF
// #define TFT_RGB_ORDER TFT_BGR

#define TFT_MISO  12
#define TFT_MOSI  13
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   -1
#define USE_HSPI_PORT

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
