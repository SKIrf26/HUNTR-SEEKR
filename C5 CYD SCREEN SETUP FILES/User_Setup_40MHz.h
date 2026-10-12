// ===========================================================================================================
//  THIS IS THE SAME AS User_Setup.h EXCEPT THAT THE DISPLAY RUNS AT 40 MHz INSTEAD OF 20 MHz (SPI_FREQUENCY below): about twice as fast to redraw.
//  Experimental. If the picture shows noise, speckles, wrong colours or stays black, go back to the normal User_Setup.h.
//  To use it: copy it over Documents/Arduino/libraries/TFT_eSPI/User_Setup.h (keep the name User_Setup.h), restart the IDE and upload again.
// ===========================================================================================================
//  User_Setup_40MHz.h  (OPTIONAL, TRY-IT) for the NM-CYD-C5 ("cheap yellow display" with ESP32-C5, RockBase / NMTech)  -  HUNTR/SEEKR
//  Default panel on this board: ST7789, 2.8 inch, 240 x 320 (used landscape by the sketch).
//
//  Where it goes: replace  Documents/Arduino/libraries/TFT_eSPI/User_Setup.h  with this file
//  (use the ESP32-C5-capable TFT_eSPI described in README.txt, then restart the Arduino IDE).
//  Pins and settings below are from the board maker's pinout and their documented TFT_eSPI setup.
// ===========================================================================================================
#define USER_SETUP_INFO "NM-CYD-C5 (ST7789)"

#define ST7789_DRIVER
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

// The ST7789 start-up table turns colour inversion ON; this board needs it OFF or colours look wrong.
#define TFT_INVERSION_OFF
// If RED and BLUE are swapped on your unit, remove the two slashes on the next line:
// #define TFT_RGB_ORDER TFT_BGR

// ---- pins (display, touch and SD card share one SPI bus: SCK 6, MISO 2, MOSI 7) ----
#define TFT_MISO  2
#define TFT_MOSI  7
#define TFT_SCLK  6
#define TFT_CS    23
#define TFT_DC    24
#define TFT_RST   -1      // the display reset is tied to the board reset
#define TFT_BL    25      // backlight
#define TFT_BACKLIGHT_ON HIGH
#define TOUCH_CS  1       // (the sketch reads the touch chip itself with XPT2046_Touchscreen)

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
#define SPI_TOUCH_FREQUENCY  2500000
