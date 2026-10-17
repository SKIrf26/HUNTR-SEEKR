/*
 * HUNTR/SEEKR  -  ESP32-2432S028 ("cheap yellow display")
 * Passive radio scanner for cameras, trackers, drones and other gadgets. Receive only.
 * Version: Public Beta 1
 *
 * Copyright (C) 2026 SKIrf Labs   (SPDX-License-Identifier: GPL-3.0-or-later)
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program (the
 * file LICENSE).  If not, see <https://www.gnu.org/licenses/>.
 *
 * Third-party components keep their own licences: see THIRD_PARTY_NOTICES.md and the licenses folder.
 *
 * Hardware : 2.8 inch 240x320 TFT (ST7789 or ILI9341), XPT2046 resistive touch (read directly), microSD slot, speaker socket
 *
 * Build (Arduino IDE 2.x, board package esp32 by Espressif 3.3.5 or newer):
 *   Board    : "ESP32 Dev Module"
 *   Flash Size: 4MB
 *   Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
 *   PSRAM: Disabled
 *   CPU Frequency: 240MHz (WiFi/BT)
 * Libraries:
 *   TFT_eSPI 2.5.x, set up with config/User_Setup.h
 *   NimBLE-Arduino 2.x
 *   TinyGPSPlus 1.1.x
 * Files in this folder (the folder must have the same name as the .ino):
 *   HUNTR-SEEKR_CYD_2432S028.ino, Fonts_Standard.h (standard fonts), Fonts_Terminal.h (fonts for the terminal look)
 *
 * Layout of this file: see docs/CODE_MAP.md (every section, with line numbers).
 */

#include <Arduino.h>
#ifndef CYD_TOUCH_DEBUG
#define CYD_TOUCH_DEBUG 0
#endif
#ifndef CYD_TOUCH_USE_IRQ
#define CYD_TOUCH_USE_IRQ 1
#endif
#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "Wrong board selected for this sketch. In the Arduino IDE use: Tools > Board > esp32 > ESP32 Dev Module"
#endif
#include <SPI.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#if __has_include(<esp_random.h>)
#include <esp_random.h>
#endif
#include <SD.h>
#include <TFT_eSPI.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <NimBLEDevice.h>
#include <esp_bt.h>
// Tell the Arduino core that this sketch uses Bluetooth LE. Newer cores free the Bluetooth memory at start-up
// unless a library says it is in use, and NimBLE no longer does, so Bluetooth then fails to start.
#if __has_include("esp32-hal-alloc-ble-mem.h")
#include "esp32-hal-alloc-ble-mem.h"
#endif
#include <esp_heap_caps.h>
#include <Preferences.h>
#include "esp_timer.h"
#include <math.h>
#include <time.h>
#include <sys/time.h>
// (no esp_ieee802154.h here: the ESP32-S3 has no 802.15.4 radio. The toolchain ships that header for every chip but the library only for chips with the radio.)
#include "Fonts_Standard.h"
#include "Fonts_Terminal.h"
// ---- Terminal theme: palette, fonts, helpers -------------------------------
// terminal is a second look for the same app: green-phosphor terminal colours, square panels, terminal fonts, scanlines, and a motion-tracker scope on the Track screen.
// Switch it in SETUP > terminal. With it off, every colour, font and shape is exactly the normal interface's.
static bool termOn = false;
// when the hunt last pinged (the scope flashes with it)
static uint32_t huntPingAt = 0;
static const SkiFont* const F_STD[5] = {&SKI_SM, &SKI_MD, &SKI_MDB, &SKI_LG, &SKI_BIG};
static const SkiFont* const F_ALN[5] = {&SKI_A_SM, &SKI_A_MD, &SKI_A_MDB, &SKI_A_LG, &SKI_A_BIG};
static const SkiFont* fnt[5] = {&SKI_SM, &SKI_MD, &SKI_MDB, &SKI_LG, &SKI_BIG};
#define SKI_SM (*fnt[0])
#define SKI_MD (*fnt[1])
#define SKI_MDB (*fnt[2])
#define SKI_LG (*fnt[3])
#define SKI_BIG (*fnt[4])
// ---- TFT_eSPI is configured by a file INSIDE the library (not by this sketch). A wrong or missing setup shows up as a black screen, so check it here.
#ifndef SKI_SKIP_TFT_CHECK
#if !defined(TFT_CS) || !defined(TFT_DC) || !defined(TFT_MOSI) || !defined(TFT_SCLK)
#error                                                                                                                 \
    "TFT_eSPI is not set up for the ESP32-2432S028: copy config/User_Setup.h over Documents/Arduino/libraries/TFT_eSPI/User_Setup.h and restart the IDE (see config/README.md)"
#endif
#if !defined(ILI9341_DRIVER) && !defined(ILI9341_2_DRIVER) && !defined(ST7789_DRIVER)
#error "TFT_eSPI has no display driver selected for the ESP32-2432S028: use config/User_Setup.h (see config/README.md)"
#endif
#if (TFT_CS != 15) || (TFT_MOSI != 13) || (TFT_SCLK != 14) || (TFT_DC != 2)
#error                                                                                                                 \
    "TFT_eSPI pins do not match the ESP32-2432S028 (needs CS 15, DC 2, MOSI 13, SCLK 14): use config/User_Setup.h (see config/README.md). To skip this check: #define SKI_SKIP_TFT_CHECK"
#endif
#if !defined(USE_HSPI_PORT)
#error                                                                                                                 \
    "TFT_eSPI must use the HSPI port on the ESP32-2432S028 (the SD card uses the other one): use config/User_Setup.h (see config/README.md)"
#endif
#endif
#define PIN_TFT_BL 21
#define PIN_TFT_CS 15
#define PIN_SD_CS 5
#define PIN_GPS_RX 27
#define PIN_GPS_TX 22
// optional PASSIVE buzzer: IO26 (header P1 pin 3) to the buzzer's +, GND (P1 pin 4) to its -. IO26 is a boot-strapping pin: a piezo is safest. -1 = no buzzer
#define PIN_BUZZER 26
#define HAS_SOUND (PIN_BUZZER >= 0)
#define PIN_TOUCH_CS 33
#define PIN_TOUCH_CLK 25
#define PIN_TOUCH_MOSI 32
#define PIN_TOUCH_MISO 39
#define PIN_TOUCH_IRQ 36
#define PIN_SPI_SCK 18
#define PIN_SPI_MISO 19
#define PIN_SPI_MOSI 23
#define PIN_LED_R 4
#define PIN_LED_G 16
#define PIN_LED_B 17
// strip height (even, the QSPI panel wants even windows): 410*24*2 = 19.7 KB
#define BAND_H 24
// dim after this much idle (only when Auto-dim is switched on in Setup) ...
#define DIM_AFTER_MS 60000UL
// ... backlight OFF after this. Touch wakes it.
#define SLEEP_AFTER_MS 120000UL
// this panel has square corners: no corner clearance needed
#define SAFE_X 0
#define SAFE_TOP 0
#define SAFE_BOT 0
#define CORNER_R 0
#define APP_VERSION "Public Beta 1"
#define PERF_LOG 0
// 0 = never start BLE (test whether BLE scanning is crowding out Wi-Fi on the shared radio)
#define ENABLE_BLE 1
// 1 = print a radio status line every 2 s (frames seen per radio)
#define RADIO_DEBUG 0
// 1 = serial dump of raw touch bytes + a teal crosshair drawn at every touch (leave 0 for normal use)
#define TOUCH_DEBUG 0
#define HAS_ZB 0
// what is different about this board (the shared code reads these)
#define HAS_IMU_BOARD 0  // no motion sensor
#define HAS_I2C 0        // no I2C bus is wired: no clock chip either
// resistive touch needs the calibration screen
#define HAS_TOUCH_CAL 1
#define DEFAULT_ORIENT 1  // landscape: how a CYD is normally held
// resistive panels blink: require 3 empty reads before a release
#define TOUCH_MISS_N 3
#define BOARD_PREFS_SAVE prefs.putBytes("tcal", &tcal, sizeof(tcal));
#define BOARD_PREFS_LOAD                                                                                               \
  if (prefs.isKey("tcal") && prefs.getBytes("tcal", &tcal, sizeof(tcal)) != sizeof(tcal))                              \
    tcal.ok = false;
#define BOARD_FS_FIRST (sdMountFs())
// free-space figures come from the SD card when it is the log storage
#define BOARD_FS_SD 1
#define BOARD_BOOT_DONE                                                                                                \
  {                                                                                                                    \
    if (!tcal.ok)                                                                                                      \
      tcalStart();                                                                                                     \
    else                                                                                                               \
      navigate(SCR_MAIN);                                                                                              \
  }
#include <time.h>
#include <sys/time.h>
// ---- What the shared interface code needs to know about this board ---------
#define PANEL_W 240  // native panel size, portrait
#define PANEL_H 320
#define UI_W0 240  // layout space (logical px), portrait
#define UI_H0 320
// 1 = the layout is drawn at 1.5x on this panel
#define UI_SCALED 0
#define SOUND_OK_TXT "speaker ready  (BEEP below)"
#define SOUND_BAD_TXT "no speaker pin set"
#ifndef HAS_ZB
#ifdef HAS_IEEE154
// this chip has an 802.15.4 radio: Zigbee/Thread group available
#define HAS_ZB 1
#else
#define HAS_ZB 0  // no 802.15.4 radio: 8 groups
#endif
#endif
#ifndef BOARD_PREFS_SAVE
#define BOARD_PREFS_SAVE
#endif
#ifndef BOARD_PREFS_LOAD
#define BOARD_PREFS_LOAD
#endif
#ifndef DEFAULT_ORIENT
#define DEFAULT_ORIENT 0
#endif
#ifndef HAS_IMU_BOARD
#define HAS_IMU_BOARD 1
#endif
#ifndef HAS_I2C
#define HAS_I2C 1
#endif
#ifndef HAS_TOUCH_CAL
#define HAS_TOUCH_CAL 0
#endif
#ifndef TOUCH_MISS_N
#define TOUCH_MISS_N 2
#endif
#ifndef BOARD_BOOT_DONE
#define BOARD_BOOT_DONE navigate(SCR_MAIN);
#endif
// ---- Types (must precede every function: Arduino IDE auto-prototypes) ------
enum Family : uint8_t {
  FAM_UNKNOWN = 0,
  FAM_FLOCK,
  FAM_AXON,
  FAM_RING,
  FAM_WYZE,
  FAM_TAPO,
  FAM_REOLINK,
  FAM_EUFY,
  FAM_NEST,
  FAM_CAM,
  FAM_GLASS,
  FAM_DRONE,
  FAM_BT,
  FAM_TAG,
  FAM_WIFI,
  FAM_ZIGBEE,
  FAM_THREAD,
  FAM_154,
  FAM_GUNSHOT,
  FAM_BODYCAM,
  FAM_COUNT
};
enum Conf : uint8_t { CONF_LOW = 1, CONF_MED = 2, CONF_HIGH = 3 };
enum Radio : uint8_t { RAD_WIFI = 0, RAD_BLE = 1, RAD_ZB = 2 };
enum Screen : uint8_t {
  SCR_BOOT,
  SCR_MAIN,
  SCR_SCAN,
  SCR_HUNT,
  SCR_INFO,
  SCR_FILES,
  SCR_PORTAL,
  SCR_REPORT,
  SCR_TEST,
  SCR_CAL,
  SCR_TCAL,
  SCR_IGN
};
enum Cat : uint8_t { CAT_ALPR = 0, CAT_AXON, CAT_CAM, CAT_GLASS, CAT_DRONE, CAT_BT, CAT_TAG, CAT_WIFI, CAT_ZT, CAT_N };
struct SigOUI {
  uint8_t b0, b1, b2;
  Family fam;
  Conf conf;
  const char* tag;
};
struct SigText {
  const char* needle;
  Family fam;
  Conf conf;
};
struct SigCid {
  uint16_t id;
  Family fam;
  Conf conf;
  const char* tag;
};
struct SigOUIX {
  uint8_t p[5];
  uint8_t bits;
  Family fam;
  Conf conf;
  const char* tag;
};  // a 28 or 36 bit address prefix
struct SigPat {
  const char* pat;
  Family fam;
  Conf conf;
  const char* tag;
};                // an anchored pattern (tiny regex)
struct RidInfo {  // decoded drone Remote ID (ASTM F3411)
  char id[21];
  int32_t lat, lon, opLat, opLon;
  int16_t alt;
  uint16_t spd10;  // degrees*1e7, metres, m/s*10
  // 1 = has id, 2 = has drone location, 4 = has operator location
  uint8_t flags;
};
struct Contact {
  uint8_t mac[6];
  Family fam;
  Conf conf;
  Radio radio;
  int8_t rssi, peak;
  uint8_t ch, chOk;
  char label[20];
  char nick[24];
  char method[16];
  uint32_t lastMs, firstMs;
  uint16_t hits;
  bool logged;
  float ema;
  bool follow, followAlerted;  // tracker seen for a long time
  RidInfo rid;
};
// the find a chime/banner is about
struct FindNote {
  uint8_t mac[6];
  Family fam;
  Conf conf;
  int8_t rssi;
  char name[24];
  bool valid;
};
struct GroupStat {
  uint16_t count;
  int8_t best;
  uint32_t firstMs;
};  // per-group session statistics
// kind: 1 oui, 2 ssid, 3 ble name, 4 ble company id, 5 ble service uuid
struct CustomSig {
  uint8_t kind;
  Family fam;
  Conf conf;
  uint8_t plen;
  char pat[24];
  uint8_t oui[3];
  uint16_t cid;
};
// One Bluetooth advertisement as queued by the scan callback (defined here, before the IDE adds its function prototypes).
struct BleU16 {
  uint16_t u;
  BleU16(uint16_t x) : u(x) {}
};
#define BLE_PAY_MAX 62
#define BLE_Q_N 32
struct BleAdv {
  uint8_t mac[6];
  int8_t rssi;
  uint8_t plen;
  uint8_t pay[BLE_PAY_MAX];
  const uint8_t* getPayload() const { return pay; }
  int getPayloadLength() const { return plen; }
  int getRSSI() const { return rssi; }
  const uint8_t* field(uint8_t type, int* len, int nth = 0) const {
    int i = 0;
    while (i + 1 < plen) {
      int l = pay[i];
      if (!l || i + 1 + l > plen)
        break;
      if (pay[i + 1] == type && nth-- == 0) {
        *len = l - 1;
        return pay + i + 2;
      }
      i += l + 1;
    }
    return nullptr;
  }
  bool haveManufacturerData() const {
    int l = 0;
    return field(0xFF, &l) && l >= 0;
  }
  String getManufacturerData() const {
    String s;
    int l = 0;
    const uint8_t* d = field(0xFF, &l);
    if (d) {
      s.reserve(l);
      for (int k = 0; k < l; k++)
        s += (char)d[k];
    }
    return s;
  }
  String getName() const {
    String s;
    int l = 0;
    const uint8_t* d = field(0x09, &l);
    if (!d)
      d = field(0x08, &l);
    if (d) {
      s.reserve(l);
      for (int k = 0; k < l; k++)
        s += (char)d[k];
    }
    return s;
  }
  bool isAdvertisingService(const BleU16& u) const {
    for (int t = 0x02; t <= 0x03; t++) {
      for (int n = 0; n < 4; n++) {
        int l = 0;
        const uint8_t* d = field((uint8_t)t, &l, n);
        if (!d)
          break;
        for (int k = 0; k + 1 < l; k += 2)
          if ((uint16_t)(d[k] | (d[k + 1] << 8)) == u.u)
            return true;
      }
    }
    return false;
  }
};
struct Tone {
  uint16_t hz, ms;
};
struct AudioReq {
  uint16_t hz;
  uint16_t ms;
};  // one queued beep (audio task)
struct ZbHit {
  uint8_t raw[48];
  uint8_t rawLen;
  int8_t rssi;
  uint8_t ch;
};
// ---- Forward declarations --------------------------------------------------
static bool isIgnored(const uint8_t* mac);
static bool btKindTag(const char* m);
static void noteFind(const Contact& c);
static void ledgerNote(const Contact& c);
static bool ledgerLiveMark(const uint8_t* mac);
static void ledgerReset();
static int ledgerSave();
static bool ledgerUnsaved();
// a boot picture found on flash (see bootlogo)
struct LogoInfo {
  bool ok, bmp;
  uint16_t w, h;
  uint32_t dataOff, stride;
  bool bottomUp;
};
static void showToast(const char* m);
static void handleLogoForm();
static void handleLogoUpload();
static void handleLogoDone();
static void handleLogoShow();
static void handleLogoDel();
static void bootLogoCycle();
static void bootLogoLabel(char* out, size_t n);
static bool bootLogoDraw();
#ifndef HAS_BOOTSOUND
#define HAS_BOOTSOUND 0
#endif
#if HAS_BOOTSOUND
static void handleBootForm();
static void handleBootUpload();
static void handleBootDone();
static void handleBootPlay();
static void handleBootDel();
static void bootSoundPlay();
static void bootSoundCycle();
static void bootSoundLabel(char* out, size_t n);
static void showToast(const char* m);
#endif
static bool listFrozen();
static void cycleListView();
static void pendCapture(const Contact& c);
static bool customMatchWifi(const uint8_t* mac, const char* ssid, Family* fam, Conf* conf);
static bool customMatchBle(const uint8_t* mac, const char* name, uint16_t cid, Family* fam, Conf* conf);
static bool ridDecode(const uint8_t* d, int len, RidInfo* out);
static const char* timeStr();
static void statReset();
static bool wakeDisplay();
static void armChanged(bool wasOff);
static void applyRotation(uint8_t r, bool redraw);
static void navigate(Screen s);
static void goBack();
static void calLeave();
static void portalPlusRoutes();
static void portalPlusBle();
static void blSet(int lvl);
// ---- Panel (TFT_eSPI) + touch (XPT2046), behind the same calls the other boards use ----
TFT_eSPI tft;
struct TS_Point {
  int16_t x, y, z;
};
struct CydTouch {
  TS_Point pt = {0, 0, 0};
  bool down = false;
  uint32_t lastMs = 0;
  int dz1 = 0, dz2 = 0, dz = 0, dirq = 0;
  void begin() {
    pinMode(PIN_TOUCH_CS, OUTPUT);
    digitalWrite(PIN_TOUCH_CS, HIGH);
    pinMode(PIN_TOUCH_CLK, OUTPUT);
    digitalWrite(PIN_TOUCH_CLK, LOW);
    pinMode(PIN_TOUCH_MOSI, OUTPUT);
    digitalWrite(PIN_TOUCH_MOSI, LOW);
    pinMode(PIN_TOUCH_MISO, INPUT);
    pinMode(PIN_TOUCH_IRQ, INPUT);
  }
  uint16_t conv(uint8_t cmd) {
    for (int i = 7; i >= 0; i--) {
      digitalWrite(PIN_TOUCH_MOSI, (cmd >> i) & 1);
      delayMicroseconds(1);
      digitalWrite(PIN_TOUCH_CLK, HIGH);
      delayMicroseconds(1);
      digitalWrite(PIN_TOUCH_CLK, LOW);
    }
    digitalWrite(PIN_TOUCH_MOSI, LOW);
    uint16_t v = 0;
    for (int i = 0; i < 16; i++) {
      digitalWrite(PIN_TOUCH_CLK, HIGH);
      delayMicroseconds(1);
      v = (uint16_t)((v << 1) | (digitalRead(PIN_TOUCH_MISO) ? 1 : 0));
      digitalWrite(PIN_TOUCH_CLK, LOW);
      delayMicroseconds(1);
    }
    return (v >> 3) & 0x0FFF;
  }
  static int16_t best2(int a, int b, int c) {
    int d1 = abs(a - b), d2 = abs(a - c), d3 = abs(b - c);
    if (d1 <= d2 && d1 <= d3)
      return (int16_t)((a + b) / 2);
    if (d2 <= d3)
      return (int16_t)((a + c) / 2);
    return (int16_t)((b + c) / 2);
  }
  bool touched() {
    uint32_t now = millis();
    if (now - lastMs < 3)
      return down;
    lastMs = now;
    dirq = digitalRead(PIN_TOUCH_IRQ);
#if CYD_TOUCH_USE_IRQ
    if (dirq != LOW) {
      setDown(false);
      return false;
    }
#endif
    digitalWrite(PIN_TOUCH_CS, LOW);
#if CYD_TOUCH_DEBUG || !CYD_TOUCH_USE_IRQ
    int z1 = conv(0xB1), z2 = conv(0xC1);
    dz1 = z1;
    dz2 = z2;
    dz = z1 + 4095 - z2;
#endif
#if !CYD_TOUCH_USE_IRQ
    if (z1 < 120) {
      conv(0x90);
      digitalWrite(PIN_TOUCH_CS, HIGH);
      setDown(false);
      return false;
    }
#endif
    conv(0xD1);
    int x0 = conv(0xD1), y0 = conv(0x91), x1 = conv(0xD1), y1 = conv(0x91), x2 = conv(0xD1), y2 = conv(0x90);
    digitalWrite(PIN_TOUCH_CS, HIGH);
    pt.x = best2(x0, x1, x2);
    pt.y = best2(y0, y1, y2);
    pt.z = (int16_t)dz;
    setDown(true);
    return true;
  }
  void setDown(bool d) {
#if CYD_TOUCH_DEBUG
    if (d != down)
      Serial.printf("touchdbg: %s  irq=%d z1=%d z2=%d raw=%d,%d\n", d ? "DOWN" : "UP  ", dirq, dz1, dz2, (int)pt.x,
                    (int)pt.y);
#endif
    down = d;
  }
  TS_Point getPoint() { return pt; }
};
static CydTouch ts;
struct PanelShim {
  // always the native portrait; rotation is done in software
  bool begin(uint32_t hz = 0) {
    (void)hz;
    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true);
    return true;
  }
  int width() { return tft.width(); }
  int height() { return tft.height(); }
  void fillScreen(uint16_t c) { tft.fillScreen(c); }
  void draw16bitRGBBitmap(int x, int y, uint16_t* b, int w, int h) { tft.pushImage(x, y, w, h, b); }
};
PanelShim panelObj;
PanelShim* panel = &panelObj;
// Screen colours (SETUP > Screen colors): corrects a board whose panel wiring differs from the usual one.
// Bit 0 = swap red and blue, bit 1 = invert. 0 = the panel set-up in User_Setup.h.
static uint8_t colorMode = 0;
static void applyColorMode() {
#if defined(ILI9341_DRIVER) || defined(ILI9341_2_DRIVER)
  uint8_t madctl = TFT_MAD_MX | TFT_MAD_COLOR_ORDER;  // portrait, as the driver sets it
#else
  uint8_t madctl = TFT_MAD_COLOR_ORDER;
#endif
  if (colorMode & 1)
    madctl ^= TFT_MAD_BGR;  // flip the red / blue order
#if defined(TFT_INVERSION_ON)
  bool inv = true;
#elif defined(TFT_INVERSION_OFF)
  bool inv = false;
#elif defined(ST7789_DRIVER)
  bool inv = true;  // the ST7789 driver switches inversion on by itself
#else
  bool inv = false;
#endif
  if (colorMode & 2)
    inv = !inv;
  tft.writecommand(TFT_MADCTL);
  tft.writedata(madctl);
  tft.invertDisplay(inv);
}
// px = c0 + c1*rawX + c2*rawY ; py = c3 + c4*rawX + c5*rawY  (native panel pixels)
struct TCal {
  float c[6];
  bool ok;
};
static TCal tcal = {{0, 0, 0, 0, 0, 0}, false};
static bool sdMountFs();
// ---- Strip renderer state --------------------------------------------------
static uint16_t* fb = nullptr;  // ONE strip: PW x BAND_H (physical pixels)
// render-space physical size: swapped when the UI is rotated
static int PW = PANEL_W, PH = PANEL_H;
// 0 portrait, 1 landscape, 2 portrait flipped, 3 landscape flipped
static uint8_t rot = 0;
// logical UI size: layout code works here, drawing is scaled x1.5 (335 x 273 in landscape)
static int W = UI_W0, H = UI_H0;
static int bandY = 0;                                       // physical y of strip row 0
static int ucx0 = 0, ucy0 = 0, ucx1 = UI_W0, ucy1 = UI_H0;  // user clip, LOGICAL
// effective clip, PHYSICAL (user clip scaled, intersected with the strip)
static int clx0 = 0, cly0 = 0, clx1 = PW, cly1 = PH;
// horizontal scene offset in PHYSICAL px (pager drag)
static int gOx = 0;
static int dirtyY0 = 0, dirtyY1 = 0;  // dirty rows, PHYSICAL
static bool regBtns = true;
// frame timestamp (same value for every strip)
static uint32_t gNow = 0;
static inline int imin(int a, int b) {
  return a < b ? a : b;
}
static inline int imax(int a, int b) {
  return a > b ? a : b;
}
static inline int iabs(int a) {
  return a < 0 ? -a : a;
}
static inline float clamp01(float v) {
  return v < 0 ? 0 : (v > 1 ? 1 : v);
}
#if UI_SCALED
#define UI_SF 1.5f
// logical -> physical (x1.5, rounded; floors for negatives)
static inline int P(int v) {
  return (v * 3 + 1) >> 1;
}
#define PHYS16_TO_LOG(a) (((a) * 2 / 3 + 8) >> 4)
#define FONT_CAP_P(f) ((f)->capHp)
#else
#define UI_SF 1.0f  // this panel is drawn 1:1
static inline int P(int v) {
  return v;
}
#define PHYS16_TO_LOG(a) (((a) + 8) >> 4)
#define FONT_CAP_P(f) ((f)->capH)
#endif
static inline void recalcClip() {
  clx0 = imax(0, P(ucx0));
  clx1 = imin(PW, P(ucx1));
  cly0 = imax(P(ucy0), bandY);
  cly1 = imin(imin(P(ucy1), PH), bandY + BAND_H);
}
static inline void clipReset() {
  ucx0 = 0;
  ucy0 = 0;
  ucx1 = W;
  ucy1 = H;
  recalcClip();
}
static inline void clipSet(int x, int y, int w, int h) {
  ucx0 = imax(0, x);
  ucy0 = imax(0, y);
  ucx1 = imin(W, x + w);
  ucy1 = imin(H, y + h);
  recalcClip();
}
// LOGICAL rows in, physical rows stored (even-aligned for the QSPI panel)
static inline void markDirty(int y0, int y1) {
  int a = P(y0) & ~1, e = (P(y1) + 1) & ~1;
  if (a < 0)
    a = 0;
  if (e > PH)
    e = PH;
  if (e <= a)
    return;
  if (dirtyY1 <= dirtyY0) {
    dirtyY0 = a;
    dirtyY1 = e;
  } else {
    if (a < dirtyY0)
      dirtyY0 = a;
    if (e > dirtyY1)
      dirtyY1 = e;
  }
}
static inline uint16_t* rowp(int y) {
  return fb + (size_t)(y - bandY) * PW;
}
constexpr uint16_t RGB(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
static inline uint16_t mix(uint16_t fg, uint16_t bg, uint8_t a) {  // fg over bg, a = 0..255
  uint32_t al = (a + 4) >> 3;
  uint32_t f = fg, b = bg;
  f = (f | (f << 16)) & 0x07E0F81FUL;
  b = (b | (b << 16)) & 0x07E0F81FUL;
  uint32_t r = ((((f - b) * al) >> 5) + b) & 0x07E0F81FUL;
  return (uint16_t)((r >> 16) | r);
}
// ---- Palette ---------------------------------------------------------------
static uint16_t C_BG = RGB(0, 0, 0);
static uint16_t C_CARD = RGB(16, 19, 25);
static uint16_t C_CARD2 = RGB(28, 33, 42);
static uint16_t C_LINE = RGB(40, 47, 58);
static uint16_t C_INK = RGB(238, 241, 245);
static uint16_t C_MUTE = RGB(139, 148, 162);
// inactive tab icons: clearly visible on a dark panel, still quieter than the white text
static uint16_t C_ICON = RGB(178, 186, 200);
static uint16_t C_DIM = RGB(82, 90, 103);
static uint16_t C_AMBER = RGB(255, 165, 20);
static uint16_t C_TEAL = RGB(0, 232, 192);
static uint16_t C_ALERT = RGB(255, 69, 58);
static uint16_t C_OK = RGB(52, 211, 100);
// the 'on' colour of switches, chips and values: the normal theme's teal in EVERY look
static const uint16_t C_ON = RGB(0, 232, 192);
static const uint16_t CAT_STD[] = {0xFCE0, 0xF9C4, 0x07F9, 0x5DFF, 0xFEE0, 0xC41A, 0xFC00, 0x3D3F, 0xA7E0};
static uint16_t CAT_COL[] = {0xFCE0, 0xF9C4, 0x07F9, 0x5DFF, 0xFEE0, 0xC41A, 0xFC00, 0x3D3F, 0xA7E0};
static const uint16_t CAT_ALN[] = {RGB(255, 176, 32),  RGB(255, 84, 56),  RGB(40, 255, 110),
                                   RGB(150, 255, 205), RGB(232, 255, 64), RGB(132, 196, 64),
                                   RGB(255, 120, 24),  RGB(48, 214, 176), RGB(176, 255, 64)};
// the palette and the fonts, both read at draw time
static void applyTheme(bool a) {
  termOn = a;
  for (int i = 0; i < 5; i++)
    fnt[i] = a ? F_ALN[i] : F_STD[i];
  if (a) {
    C_BG = RGB(0, 0, 0);
    C_CARD = RGB(4, 26, 12);
    C_CARD2 = RGB(10, 50, 23);
    C_LINE = RGB(26, 122, 58);
    C_INK = RGB(218, 255, 230);
    C_MUTE = RGB(162, 234, 188);
    C_ICON = RGB(150, 236, 176);
    C_DIM = RGB(92, 182, 120);
    C_AMBER = RGB(255, 176, 32);
    C_TEAL = RGB(48, 255, 118);
    C_ALERT = RGB(255, 72, 50);
    C_OK = RGB(64, 255, 124);
    memcpy(CAT_COL, CAT_ALN, sizeof(CAT_COL));
  } else {
    C_BG = RGB(0, 0, 0);
    C_CARD = RGB(16, 19, 25);
    C_CARD2 = RGB(28, 33, 42);
    C_LINE = RGB(40, 47, 58);
    C_INK = RGB(238, 241, 245);
    C_MUTE = RGB(139, 148, 162);
    C_ICON = RGB(178, 186, 200);
    C_DIM = RGB(82, 90, 103);
    C_AMBER = RGB(255, 165, 20);
    C_TEAL = RGB(0, 232, 192);
    C_ALERT = RGB(255, 69, 58);
    C_OK = RGB(52, 211, 100);
    memcpy(CAT_COL, CAT_STD, sizeof(CAT_COL));
  }
}
// ---- HUD: a second look, blue, with a motion-tracker style Track screen. It shares everything else with terminal (terminal fonts, square panels, scan lines).
// true only in the HUD look (termOn is then true as well)
static bool hudOn = false;
static const uint16_t CAT_ALS[] = {RGB(88, 204, 255),  RGB(64, 150, 255), RGB(130, 120, 255),
                                   RGB(176, 232, 255), RGB(44, 222, 232), RGB(104, 176, 236),
                                   RGB(156, 156, 255), RGB(34, 192, 206), RGB(196, 206, 255)};
// 0 = normal, 1 = terminal (green), 2 = HUD (blue)
static void applyThemeMode(int m) {
  applyTheme(m != 0);
  hudOn = (m == 2);
  if (hudOn) {
    C_BG = RGB(0, 0, 0);
    C_CARD = RGB(3, 16, 36);
    C_CARD2 = RGB(8, 32, 68);
    C_LINE = RGB(24, 96, 176);
    C_INK = RGB(214, 238, 255);
    C_MUTE = RGB(140, 190, 236);
    C_ICON = RGB(130, 184, 238);
    C_DIM = RGB(72, 128, 192);
    C_AMBER = RGB(146, 212, 255);
    C_TEAL = RGB(58, 176, 255);
    C_ALERT = RGB(255, 84, 84);
    C_OK = RGB(96, 214, 255);
    memcpy(CAT_COL, CAT_ALS, sizeof(CAT_COL));
  }
}
static void termPanel(int x, int y, int w, int h, uint16_t c);  // (defined after the shape primitives)
// ---- Primitives (integer only) ---------------------------------------------
static inline void pxA(int x, int y, uint16_t c, uint8_t a) {
  x += gOx;
  if (x < clx0 || x >= clx1 || y < cly0 || y >= cly1 || a == 0)
    return;
  uint16_t* p = rowp(y) + x;
  *p = (a >= 248) ? c : mix(c, *p, a);
}
static void spanA(int y, int xa, int xb, uint16_t c, uint8_t a) {  // [xa, xb)
  if (y < cly0 || y >= cly1)
    return;
  xa += gOx;
  xb += gOx;
  xa = imax(xa, clx0);
  xb = imin(xb, clx1);
  if (xb <= xa)
    return;
  uint16_t* p = rowp(y) + xa;
  if (a >= 248) {
    for (int i = xa; i < xb; i++)
      *p++ = c;
  } else {
    for (int i = xa; i < xb; i++, p++)
      *p = mix(c, *p, a);
  }
}
static void fillRectP(int x, int y, int w, int h, uint16_t c) {
  int y0 = imax(y, cly0), y1 = imin(y + h, cly1);
  for (int j = y0; j < y1; j++)
    spanA(j, x, x + w, c, 255);
}
static void fillRectAP(int x, int y, int w, int h, uint16_t c, uint8_t a) {
  int y0 = imax(y, cly0), y1 = imin(y + h, cly1);
  for (int j = y0; j < y1; j++)
    spanA(j, x, x + w, c, a);
}
static inline bool rowsVisibleP(int y, int h) {
  return y < cly1 && y + h > cly0;
}
// ---- logical-coordinate API used by all the UI code
static void fillRect(int x, int y, int w, int h, uint16_t c) {
  fillRectP(P(x), P(y), P(x + w) - P(x), P(y + h) - P(y), c);
}
static void fillRectA(int x, int y, int w, int h, uint16_t c, uint8_t a) {
  fillRectAP(P(x), P(y), P(x + w) - P(x), P(y + h) - P(y), c, a);
}
static inline bool rowsVisible(int y, int h) {
  return P(y) < cly1 && P(y + h) > cly0;
}
// rounded corners: coverage lookup per radius, built lazily (floats used only once per radius)
#define RR_MAX 40
static uint8_t* rrLut[RR_MAX + 1];
static const uint8_t* cornerLut(int r) {
  if (!rrLut[r]) {
    uint8_t* t = (uint8_t*)malloc((size_t)r * r);
    if (!t)
      return nullptr;
    for (int i = 0; i < r; i++)
      for (int j = 0; j < r; j++) {
        float dx = r - (j + 0.5f), dy = r - (i + 0.5f);
        float d = sqrtf(dx * dx + dy * dy) - r;
        t[i * r + j] = (uint8_t)(clamp01(0.5f - d) * 255.0f + 0.5f);
      }
    rrLut[r] = t;
  }
  return rrLut[r];
}
static void rrFillP(int x, int y, int w, int h, int r, uint16_t c, uint8_t a) {
  if (w <= 0 || h <= 0 || !rowsVisibleP(y, h))
    return;
  if (r > RR_MAX)
    r = RR_MAX;
  if (r > w / 2)
    r = w / 2;
  if (r > h / 2)
    r = h / 2;
  if (r <= 0) {
    fillRectAP(x, y, w, h, c, a);
    return;
  }
  const uint8_t* lut = cornerLut(r);
  if (!lut) {
    fillRectAP(x, y, w, h, c, a);
    return;
  }
  int y0 = imax(y, cly0), y1 = imin(y + h, cly1);
  for (int yy = y0; yy < y1; yy++) {
    int i = yy - y;
    if (i < r || i >= h - r) {
      int ci = i < r ? i : (h - 1 - i);
      const uint8_t* cl = lut + ci * r;
      int j = 0;
      for (; j < r; j++) {
        uint8_t cv = cl[j];
        if (cv >= 250)
          break;
        if (cv) {
          uint8_t aa = (uint8_t)((cv * a) / 255);
          pxA(x + j, yy, c, aa);
          pxA(x + w - 1 - j, yy, c, aa);
        }
      }
      spanA(yy, x + j, x + w - j, c, a);
    } else
      spanA(yy, x, x + w, c, a);
  }
}
// ---- terminal / HUD: snipped corners (a 45 degree cut, like scissors on paper). Used by every button and panel of the two looks.
// size of the cut in physical px; 0 = too small, draw it square
static int chamK(int w, int h) {
  if (h < 18 || w < 24)
    return 0;
  int k = h / 5;
  if (k < 4)
    k = 4;
  if (k > 9)
    k = 9;
  if (k > w / 6)
    k = w / 6;
  return k;
}
static void chamferP(int x, int y, int w, int h, int k, uint16_t c, uint8_t a) {
  if (w <= 0 || h <= 0 || !rowsVisibleP(y, h))
    return;
  if (k * 2 > h)
    k = h / 2;
  if (k * 2 > w)
    k = w / 2;
  int y0 = imax(y, cly0), y1 = imin(y + h, cly1);
  for (int yy = y0; yy < y1; yy++) {
    int i = yy - y;
    int ins = imax(k - i, k - (h - 1 - i));
    if (ins < 0)
      ins = 0;
    spanA(yy, x + ins, x + w - ins, c, a);
    if (ins > 0) {
      pxA(x + ins - 1, yy, c, a >> 1);
      pxA(x + w - ins, yy, c, a >> 1);
    }  // the diagonal, anti-aliased
  }
}
static void rrFill(int x, int y, int w, int h, int r, uint16_t c, uint8_t a = 255) {
  if (termOn && r > 3) {
    int wp = P(x + w) - P(x), hp = P(y + h) - P(y), k = chamK(wp, hp);
    if (k) {
      if (a == 255 && (c == C_CARD || c == C_CARD2))
        termPanel(x, y, w, h, c);
      else
        chamferP(P(x), P(y), wp, hp, k, c, a);
      return;
    }
    r = 2;
    if (a == 255 && w >= 22 && h >= 16 && (c == C_CARD || c == C_CARD2)) {
      termPanel(x, y, w, h, c);
      return;
    }
  }
  rrFillP(P(x), P(y), P(x + w) - P(x), P(y + h) - P(y), P(r), c, a);
}
// card with a border: outer in edge colour, inner in fill colour, both anti-aliased (t = border width in physical px)
static void rrBorder(int x, int y, int w, int h, int r, uint16_t fill, uint16_t edge, int t = 1) {
  if (termOn && r > 3) {
    int xq = P(x), yq = P(y), wq = P(x + w) - xq, hq = P(y + h) - yq, k = chamK(wq, hq);
    if (k) {
      chamferP(xq, yq, wq, hq, k, edge, 255);
      chamferP(xq + t, yq + t, wq - 2 * t, hq - 2 * t, k > t ? k - t : 0, fill, 255);
      return;
    }
    r = 2;
  }
  int xp = P(x), yp = P(y), wp = P(x + w) - xp, hp = P(y + h) - yp, rp = P(r);
  rrFillP(xp, yp, wp, hp, rp, edge, 255);
  rrFillP(xp + t, yp + t, wp - 2 * t, hp - 2 * t, rp > t ? rp - t : 0, fill, 255);
}
// ---- Terminal theme: panel shape and the CRT pass --------------------------
// a terminal panel: snipped corners, a 1 px phosphor outline
static void termPanel(int x, int y, int w, int h, uint16_t c) {
  int xp = P(x), yp = P(y), wp = P(x + w) - xp, hp = P(y + h) - yp;
  {
    int k = chamK(wp, hp);
    if (k) {
      chamferP(xp, yp, wp, hp, k, C_LINE, 255);
      chamferP(xp + 1, yp + 1, wp - 2, hp - 2, k - 1, c, 255);
      return;
    }
  }
  rrFillP(xp, yp, wp, hp, 2, C_LINE, 255);
  rrFillP(xp + 1, yp + 1, wp - 2, hp - 2, 1, c, 255);
}
// the CRT: every second row about a fifth darker (scan lines), then the strip goes out
static void termPost(int by, int rows) {
  for (int r = 0; r < rows; r++) {
    if (!((by + r) & 1))
      continue;
    uint16_t* p = fb + (size_t)r * PW;
    for (int x = 0; x < PW; x++) {
      uint16_t c = p[x];
      p[x] = (uint16_t)(c - ((c >> 3) & 0x18E3) - ((c >> 4) & 0x0861));
    }
  }
}
// discs / rings / arcs: Q8 fixed point, distance ≈ (d²-r²)/(2r)
static inline int covQ(int32_t dist8) {
  int v = 128 - dist8;
  return v < 0 ? 0 : (v > 255 ? 255 : v);
}
static int32_t isqrtI(int32_t v) {
  if (v <= 0)
    return 0;
  int32_t r = (int32_t)sqrtf((float)v);
  while ((int64_t)r * r > v)
    r--;
  while ((int64_t)(r + 1) * (r + 1) <= v)
    r++;
  return r;
}
static void ringQ(float cxf, float cyf, float rf, float tf, uint16_t c, uint8_t a, bool filled, bool arc, int32_t p0,
                  int32_t span) {
  int32_t cx = (int32_t)(cxf * 256), cy = (int32_t)(cyf * 256), r8 = (int32_t)(rf * 256), t8 = (int32_t)(tf * 256);
  if (r8 < 64)
    r8 = 64;
  int32_t den = 2 * r8;
  int32_t ro = r8 + 256, ri = filled ? -1 : (r8 - t8 - 256);
  int32_t ro2 = ro * ro, ri2 = ri > 0 ? ri * ri : -1;
  int x0 = imax((int)((cx - ro) >> 8), clx0 - gOx), x1 = imin((int)((cx + ro) >> 8) + 1, clx1 - gOx);
  int y0 = imax((int)((cy - ro) >> 8), cly0), y1 = imin((int)((cy + ro) >> 8) + 1, cly1);
  int32_t r2 = r8 * r8;
  const bool solidOk = filled && !arc && a == 255;
  for (int j = y0; j < y1; j++) {
    int32_t dy = ((j << 8) + 128) - cy;
    int32_t dy2 = dy * dy, vo = ro2 - dy2;
    if (vo < 0)
      continue;
    int32_t hwo = isqrtI(vo);
    // the pixels of this row inside the outer edge
    int xa = imax(x0, (int)((cx - hwo - 128 + 255) >> 8)), xb = imin(x1, (int)((cx + hwo - 128) >> 8) + 1);
    // a run in the middle that needs no per-pixel test
    int ha = 0x7fffffff, hb = 0;
    // fully covered inside of a solid disc: plain spans
    if (solidOk) {
      int32_t vs = r2 - 120 * den - dy2;
      if (vs >= 0) {
        int32_t hws = isqrtI(vs);
        int sa = imax(xa, (int)((cx - hws - 128 + 255) >> 8)), sb = imin(xb, (int)((cx + hws - 128) >> 8) + 1);
        if (sb > sa) {
          spanA(j, sa, sb, c, 255);
          ha = sa;
          hb = sb;
        }
      }
    } else if (ri2 > 0) {  // the hole of a ring: nothing there
      int32_t vi = ri2 - dy2 - 1;
      if (vi >= 0) {
        int32_t hwi = isqrtI(vi);
        int sa = imax(xa, (int)((cx - hwi - 128 + 255) >> 8)), sb = imin(xb, (int)((cx + hwi - 128) >> 8) + 1);
        if (sb > sa) {
          ha = sa;
          hb = sb;
        }
      }
    }
    for (int i = xa; i < xb; i++) {
      if (i == ha) {
        i = hb - 1;
        continue;
      }
      int32_t dx = ((i << 8) + 128) - cx;
      // (wraps like the chip does; the pixel is then skipped)
      int32_t d2 = (int32_t)((uint32_t)(dx * dx) + (uint32_t)(dy * dy));
      if (d2 > ro2 || d2 < ri2)
        continue;
      int32_t dist8 = (d2 - r2) / den;
      int cv = covQ(dist8);
      if (!filled)
        cv -= covQ(dist8 + t8);
      if (cv <= 0)
        continue;
      if (arc) {
        int32_t ax = iabs(dx), ay = iabs(dy), s = ax + ay, pp;
        if (s == 0)
          continue;
        int32_t pq = (ay << 12) / s;
        if (dx >= 0 && dy >= 0)
          pp = pq;
        else if (dx < 0 && dy >= 0)
          pp = 8192 - pq;
        else if (dx < 0 && dy < 0)
          pp = 8192 + pq;
        else
          pp = 16384 - pq;
        int32_t rel = pp - p0;
        if (rel < 0)
          rel += 16384;
        if (rel > span)
          continue;
      }
      pxA(i, j, c, (uint8_t)((cv * a) / 255));
    }
  }
}
static void disc(float cx, float cy, float r, uint16_t c, uint8_t a = 255) {
  ringQ(cx * UI_SF, cy * UI_SF, r * UI_SF, 0, c, a, true, false, 0, 0);
}
static void ring(float cx, float cy, float r, float t, uint16_t c, uint8_t a = 255) {
  ringQ(cx * UI_SF, cy * UI_SF, r * UI_SF, t * UI_SF, c, a, false, false, 0, 0);
}
// same mapping as the per-pixel test, Q12 (0..16383)
static int32_t pseudoAngle(float ang) {
  int32_t dx = (int32_t)(cosf(ang) * 1000), dy = (int32_t)(sinf(ang) * 1000);
  int32_t ax = iabs(dx), ay = iabs(dy), s = ax + ay;
  if (!s)
    return 0;
  int32_t pq = (ay << 12) / s;
  if (dx >= 0 && dy >= 0)
    return pq;
  if (dx < 0 && dy >= 0)
    return 8192 - pq;
  if (dx < 0 && dy < 0)
    return 8192 + pq;
  return 16384 - pq;
}
// arc band [r-t, r], clockwise on screen from a0 to a1 (radians), round caps
static void arcBand(float cx, float cy, float r, float t, float a0, float a1, uint16_t c) {
  int32_t p0 = pseudoAngle(a0), p1 = pseudoAngle(a1);
  int32_t span = p1 - p0;
  if (span < 0)
    span += 16384;
  ringQ(cx * UI_SF, cy * UI_SF, r * UI_SF, t * UI_SF, c, 255, false, true, p0, span);
  float rm = r - t * 0.5f;
  disc(cx + cosf(a0) * rm, cy + sinf(a0) * rm, t * 0.5f, c);
  disc(cx + cosf(a1) * rm, cy + sinf(a1) * rm, t * 0.5f, c);
}
// Wu anti-aliased line, optionally 2 px thick
static void lineAA(float fx0, float fy0, float fx1, float fy1, float w, uint16_t c, uint8_t a = 255) {
  fx0 *= UI_SF;
  fy0 *= UI_SF;
  fx1 *= UI_SF;
  fy1 *= UI_SF;
  w *= UI_SF;
  int x0 = (int)(fx0 + 0.5f), y0 = (int)(fy0 + 0.5f), x1 = (int)(fx1 + 0.5f), y1 = (int)(fy1 + 0.5f);
  bool steep = iabs(y1 - y0) > iabs(x1 - x0);
  if (steep) {
    int t = x0;
    x0 = y0;
    y0 = t;
    t = x1;
    x1 = y1;
    y1 = t;
  }
  if (x0 > x1) {
    int t = x0;
    x0 = x1;
    x1 = t;
    t = y0;
    y0 = y1;
    y1 = t;
  }
  int dx = x1 - x0, dy = y1 - y0;
  int32_t grad = dx ? (dy << 16) / dx : 0;
  int32_t yf = y0 << 16;
  int th = w >= 1.8f ? 2 : 1;
  // (strip clip: the same pixels, without walking the parts of the line outside the strip)
  int xs = x0, xe = x1;
  if (steep) {
    xs = imax(xs, cly0);
    xe = imin(xe, cly1 - 1);
  } else {
    xs = imax(xs, clx0 - gOx);
    xe = imin(xe, clx1 - gOx - 1);
    if (imax(y0, y1) + th + 1 < cly0 || imin(y0, y1) >= cly1)
      return;
  }
  if (xe < xs)
    return;
  yf += grad * (xs - x0);
  for (int x = xs; x <= xe; x++, yf += grad) {
    int yi = yf >> 16;
    uint8_t f = (yf >> 8) & 255;
    for (int k = 0; k < th; k++) {
      uint8_t a1v = (uint8_t)(((255 - f) * a) / 255), a2v = (uint8_t)((f * a) / 255);
      if (steep) {
        pxA(yi + k, x, c, a1v);
        pxA(yi + k + 1, x, c, a2v);
      } else {
        pxA(x, yi + k, c, a1v);
        pxA(x, yi + k + 1, c, a2v);
      }
    }
  }
}
// ---- Text (anti-aliased) ---------------------------------------------------
static inline const SkiGlyph* glyphFor(const SkiFont* f, unsigned char ch) {
  if (ch < f->first || ch > f->last) {
    ch = '?';
    if (ch < f->first || ch > f->last)
      ch = f->first;
  }
  return &f->gl[ch - f->first];
}
static int textW(const SkiFont* f, const char* s) {
  int a = 0;
  for (; *s; s++)
    a += glyphFor(f, (unsigned char)*s)->adv16;
  return PHYS16_TO_LOG(a);  // physical 1/16 px -> logical px
}
static void drawText(const SkiFont* f, const char* s, int x, int y, uint16_t col, uint8_t alpha = 255) {
  if (!rowsVisible(y - 2, f->lineH + 4))
    return;  // whole line outside this strip
  int base = P(y) + FONT_CAP_P(f), pen = P(x) * 16;
  for (; *s; s++) {
    const SkiGlyph* g = glyphFor(f, (unsigned char)*s);
    if (g->w) {
      int gx = ((pen + 8) >> 4) + g->xo + gOx, gy = base + g->yo;
      int j0 = imax(0, cly0 - gy), j1 = imin((int)g->h, cly1 - gy);
      for (int j = j0; j < j1; j++) {
        const uint8_t* src = f->bmp + g->off + j * g->w;
        uint16_t* row = rowp(gy + j);
        for (int i = 0; i < g->w; i++) {
          int xx = gx + i;
          if (xx < clx0 || xx >= clx1)
            continue;
          uint8_t a = src[i];
          if (!a)
            continue;
          if (alpha != 255)
            a = (uint8_t)((a * alpha) / 255);
          row[xx] = (a >= 248) ? col : mix(col, row[xx], a);
        }
      }
    }
    pen += g->adv16;
  }
}
static inline void textC(const SkiFont* f, const char* s, int cx, int y, uint16_t col, uint8_t a = 255) {
  drawText(f, s, cx - textW(f, s) / 2, y, col, a);
}
static inline void textR(const SkiFont* f, const char* s, int rx, int y, uint16_t col, uint8_t a = 255) {
  drawText(f, s, rx - textW(f, s), y, col, a);
}
static inline void textMid(const SkiFont* f, const char* s, int cx, int cy, uint16_t col, uint8_t a = 255) {
  drawText(f, s, cx - textW(f, s) / 2, cy - f->capH / 2, col, a);
}
static const char* ellipsize(const SkiFont* f, const char* s, int maxW, char* buf, int n) {
  strncpy(buf, s, n - 3);
  buf[n - 3] = 0;
  if (textW(f, buf) <= maxW)
    return buf;
  int L = strlen(buf);
  while (L > 1) {
    buf[--L] = 0;
    char t[48];
    snprintf(t, sizeof(t), "%s..", buf);
    if (textW(f, t) <= maxW) {
      strcpy(buf, t);
      return buf;
    }
  }
  return buf;
}
static inline float easeOut(float t) {
  t = clamp01(t);
  float u = 1 - t;
  return 1 - u * u * u;
}
// clang-format off
static const SigOUI WIFI_OUI[] = {
  {0xB4,0x1E,0x52, FAM_FLOCK, CONF_HIGH,"FLOCK"},
  {0x70,0xC9,0x4E, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x3C,0x91,0x80, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0xD8,0xF3,0xBC, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x80,0x30,0x49, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x14,0x5A,0xFC, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x74,0x4C,0xA1, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x9C,0x2F,0x9D, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x94,0x08,0x53, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0xE4,0xAA,0xEA, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0xF4,0x6A,0xDD, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x00,0xF4,0x8D, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0xD0,0x39,0x57, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0xE8,0xD0,0xFC, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x58,0x8E,0x81, FAM_FLOCK, CONF_LOW,"FS-BATT"},
  {0xEC,0x1B,0xBD, FAM_FLOCK, CONF_LOW,"FS-BATT"},
  {0x82,0x6B,0xF2, FAM_FLOCK, CONF_MED,"FLOCK"},
  {0x00,0x25,0xDF, FAM_AXON,  CONF_HIGH,"AXON"},
  {0xE4,0x05,0x40, FAM_AXON,  CONF_HIGH,"AXON"},
  {0x28,0x24,0xFF, FAM_AXON,  CONF_LOW,"AXON"},
  {0x2C,0xAA,0x8E, FAM_WYZE,  CONF_MED,"WYZE"},
  {0x7C,0x78,0xB2, FAM_WYZE,  CONF_MED,"WYZE"},
  {0x80,0x48,0x2C, FAM_WYZE,  CONF_MED,"WYZE"},
  {0xD0,0x3F,0x27, FAM_WYZE,  CONF_MED,"WYZE"},
  {0xF0,0xC8,0x8B, FAM_WYZE,  CONF_MED,"WYZE"},
  {0x00,0xB4,0x63, FAM_RING,  CONF_MED,"RING"},
  {0x34,0x3E,0xA4, FAM_RING,  CONF_MED,"RING"},
  {0xAC,0x9F,0xC3, FAM_RING,  CONF_MED,"RING"},
  {0x18,0x7F,0x88, FAM_RING,  CONF_MED,"RING"},
  {0x54,0xE0,0x19, FAM_RING,  CONF_MED,"RING"},
  {0x5C,0x47,0x5E, FAM_RING,  CONF_MED,"RING"},
  {0x64,0x9A,0x63, FAM_RING,  CONF_MED,"RING"},
  {0x90,0x48,0x6C, FAM_RING,  CONF_MED,"RING"},
  {0x9C,0x76,0x13, FAM_RING,  CONF_MED,"RING"},
  {0xCC,0x3B,0xFB, FAM_RING,  CONF_MED,"RING"},
  {0xC4,0xDB,0xAD, FAM_RING,  CONF_MED,"RING"},
  {0x24,0x2B,0xD6, FAM_RING,  CONF_MED,"RING"},
  {0x50,0xE4,0x67, FAM_RING,  CONF_MED,"RING"},
  {0xEC,0x71,0xDB, FAM_REOLINK,CONF_MED,"REOLINK"},
  {0xB4,0x6D,0x83, FAM_REOLINK,CONF_LOW,"REOLINK"},
  {0x8C,0x85,0x80, FAM_EUFY,  CONF_LOW,"EUFY"},
  {0x98,0xF1,0x99, FAM_EUFY,  CONF_LOW,"EUFY"},
  {0x18,0xB4,0x30, FAM_NEST,  CONF_LOW,"NEST"},
  {0x7C,0x2A,0x9E, FAM_GLASS, CONF_MED,"META"},
  {0xCC,0x66,0x0A, FAM_GLASS, CONF_LOW,"META"},
  {0xF4,0x03,0x43, FAM_GLASS, CONF_LOW,"META"},
  {0x5C,0xE9,0x1E, FAM_GLASS, CONF_LOW,"META"},
  {0x98,0x59,0x49, FAM_GLASS, CONF_MED,"LUXOTTICA"},
  {0x98,0xDA,0x92, FAM_GLASS, CONF_MED,"VUZIX"},
  {0x60,0x60,0x1F, FAM_DRONE, CONF_HIGH,"DJI"},
  {0x34,0xD2,0x62, FAM_DRONE, CONF_HIGH,"DJI"},
  {0x48,0x1C,0xB9, FAM_DRONE, CONF_MED,"DJI"},
  {0xA0,0x14,0x3D, FAM_DRONE, CONF_MED,"PARROT"},
  {0x90,0x03,0xB7, FAM_DRONE, CONF_MED,"PARROT"},
  {0x00,0x12,0x1C, FAM_DRONE, CONF_MED,"PARROT"},
  {0x00,0x30,0x44, FAM_WIFI, CONF_MED,"CRADLEPOINT"},
  {0x00,0xE0,0x1C, FAM_WIFI, CONF_MED,"CRADLEPOINT"},
  {0x00,0xA0,0xD5, FAM_WIFI, CONF_MED,"SIERRA"},
  {0x00,0x14,0x3E, FAM_WIFI, CONF_MED,"SIERRA"},
  {0x28,0xA3,0x31, FAM_WIFI, CONF_MED,"SIERRA"},
  {0x50,0x13,0x9D, FAM_WIFI, CONF_MED,"SIERRA"},
  {0x64,0xCE,0x6E, FAM_WIFI, CONF_MED,"SIERRA"},
  {0x84,0xDB,0x2F, FAM_WIFI, CONF_MED,"SIERRA"},
  {0xCC,0x93,0x4A, FAM_WIFI, CONF_MED,"SIERRA"},
  {0x00,0x11,0x6E, FAM_WIFI, CONF_MED,"PEPLINK"},
  {0x10,0x56,0xCA, FAM_WIFI, CONF_MED,"PEPLINK"},
  {0x6C,0xA3,0xD3, FAM_WIFI, CONF_MED,"PEPLINK"},
  {0xD4,0x13,0xF8, FAM_WIFI, CONF_MED,"PEPLINK"},
  {0x00,0x23,0xBD, FAM_BODYCAM, CONF_MED,"DIGITAL ALLY"},
  {0x00,0x1D,0x96, FAM_BODYCAM, CONF_MED,"WATCHGUARD"},
  {0xD4,0x2D,0xC5, FAM_CAM, CONF_MED,"I-PRO"},
  {0x4C,0x36,0x4E, FAM_WIFI, CONF_LOW,"TOUGHBOOK"},
  {0xB8,0x20,0x8E, FAM_WIFI, CONF_LOW,"TOUGHBOOK"},
  {0xBC,0x3E,0x0B, FAM_WIFI, CONF_LOW,"TOUGHBOOK"},
  {0x00,0x05,0x12, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x00,0x15,0x70, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x00,0x23,0x68, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x00,0xA0,0xF8, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x40,0x83,0xDE, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x48,0x8E,0xB7, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x60,0x95,0x32, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x78,0xB8,0xD6, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x84,0x24,0x8D, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x88,0xBC,0xAC, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x90,0x75,0xDE, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x94,0xFB,0x29, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0xC4,0x7D,0xCC, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0xC8,0x1C,0xFE, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0xFC,0x59,0x7A, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x00,0x07,0x4D, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x74,0x93,0xA4, FAM_WIFI, CONF_LOW,"ZEBRA"},
  {0x00,0x07,0xBE, FAM_WIFI, CONF_LOW,"DATALOGIC"},
  {0x00,0x11,0x83, FAM_WIFI, CONF_LOW,"DATALOGIC"},
  {0x00,0x18,0x67, FAM_WIFI, CONF_LOW,"DATALOGIC"},
  {0x00,0x1F,0x9C, FAM_WIFI, CONF_LOW,"HAVIS"},
  {0x94,0x7B,0xBE, FAM_FLOCK, CONF_MED,"UBICQUIA"},
  {0xE0,0xA7,0x00, FAM_CAM, CONF_MED,"VERKADA"},
  {0x00,0xBF,0x15, FAM_FLOCK, CONF_LOW,"GENETEC"},
  {0x0C,0xBF,0x15, FAM_FLOCK, CONF_LOW,"GENETEC"},
  {0x70,0x1A,0xD5, FAM_CAM, CONF_LOW,"AVIGILON"},
  {0xD4,0x11,0xD6, FAM_GUNSHOT, CONF_MED,"SHOTSPOTTER"},
  {0xB8,0x35,0x32, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x08,0x3A,0x88, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0xC0,0x35,0x32, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x24,0xB2,0xB9, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0xE0,0x4F,0x43, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0xB8,0x1E,0xA4, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x70,0x08,0x94, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x3C,0x71,0xBF, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x58,0x00,0xE3, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x90,0x35,0xEA, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x5C,0x93,0xA2, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x64,0x6E,0x69, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0xE0,0x0A,0xF6, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x14,0xB5,0xCD, FAM_FLOCK, CONF_LOW,"FLOCK"},
  {0x0C,0x9A,0xE6, FAM_DRONE, CONF_LOW,"DJI"},
  {0x8C,0x58,0x23, FAM_DRONE, CONF_LOW,"DJI"},
  {0x04,0xA8,0x5A, FAM_DRONE, CONF_LOW,"DJI"},
  {0x58,0xB8,0x58, FAM_DRONE, CONF_LOW,"DJI"},
  {0xE4,0x7A,0x2C, FAM_DRONE, CONF_LOW,"DJI"},
  {0x38,0x1D,0x14, FAM_DRONE, CONF_MED,"SKYDIO"},
  {0x00,0x26,0x7E, FAM_DRONE, CONF_LOW,"PARROT"},
  {0x90,0x3A,0xE6, FAM_DRONE, CONF_LOW,"PARROT"},

  // ---- vendor blocks from the IEEE registry (camera, doorbell and gunshot-sensor makers) ----
  {0x3C,0xA0,0x70, FAM_CAM, CONF_MED,"BLINK"},
  {0x70,0xAD,0x43, FAM_CAM, CONF_MED,"BLINK"},
  {0x74,0xAB,0x93, FAM_CAM, CONF_MED,"BLINK"},
  {0xF0,0x74,0xC1, FAM_CAM, CONF_MED,"BLINK"},
  {0x48,0x62,0x64, FAM_CAM, CONF_MED,"ARLO"},
  {0xA4,0x11,0x62, FAM_CAM, CONF_MED,"ARLO"},
  {0xFC,0x9C,0x98, FAM_CAM, CONF_MED,"ARLO"},
  {0x00,0xBC,0x99, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x04,0x03,0x12, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x04,0xEE,0xCD, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x08,0x3B,0xC1, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x08,0x54,0x11, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x08,0xA1,0x89, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x08,0xCC,0x81, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x0C,0x75,0xD2, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x10,0x12,0xFB, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x18,0x68,0xCB, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x18,0x80,0x25, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x24,0x0F,0x9B, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x24,0x28,0xFD, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x24,0x32,0xAE, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x24,0x48,0x45, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x28,0x57,0xBE, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x2C,0xA5,0x9C, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x34,0x09,0x62, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x3C,0x1B,0xF8, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x40,0xAC,0xBF, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x44,0x19,0xB6, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x44,0x47,0xCC, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x44,0xA6,0x42, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x48,0x78,0x5B, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x4C,0x1F,0x86, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x4C,0x62,0xDF, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x4C,0xBD,0x8F, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x4C,0xF5,0xDC, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x50,0xE5,0x38, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x54,0x8C,0x81, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x54,0xC4,0x15, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x58,0x03,0xFB, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x58,0x50,0xED, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x5C,0x34,0x5B, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x64,0xDB,0x8B, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x68,0x6D,0xBC, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x74,0x3F,0xC2, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x80,0x48,0x9F, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x80,0x7C,0x62, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x80,0xBE,0xAF, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x80,0xF5,0xAE, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x84,0x94,0x59, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x84,0x9A,0x40, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x88,0xDE,0x39, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x8C,0x22,0xD2, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x8C,0xE7,0x48, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x94,0xE1,0xAC, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x98,0x8B,0x0A, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x98,0x9D,0xE5, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x98,0xDF,0x82, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x98,0xF1,0x12, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xA0,0xFF,0x0C, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xA4,0x14,0x37, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xA4,0x29,0x02, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xA4,0x4B,0xD9, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xA4,0xA4,0x59, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xA4,0xD5,0xC2, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xAC,0xB9,0x2F, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xAC,0xCB,0x51, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xB4,0xA3,0x82, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xBC,0x5E,0x33, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xBC,0x9B,0x5E, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xBC,0xAD,0x28, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xBC,0xBA,0xC2, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xC0,0x51,0x7E, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xC0,0x56,0xE3, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xC0,0x6D,0xED, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xC4,0x2F,0x90, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xC8,0xA7,0x02, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xD4,0xE8,0x53, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xDC,0x07,0xF8, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xDC,0xD2,0x6A, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xE0,0xBA,0xAD, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xE0,0xCA,0x3C, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xE0,0xDF,0x13, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xE4,0xD5,0x8B, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xE8,0xA0,0xED, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xEC,0xA9,0x71, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xEC,0xC8,0x9C, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xF8,0x4D,0xFC, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xFC,0x9F,0xFD, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x08,0xED,0xED, FAM_CAM, CONF_MED,"DAHUA"},
  {0x14,0xA7,0x8B, FAM_CAM, CONF_MED,"DAHUA"},
  {0x24,0x52,0x6A, FAM_CAM, CONF_MED,"DAHUA"},
  {0x38,0xAF,0x29, FAM_CAM, CONF_MED,"DAHUA"},
  {0x3C,0xE3,0x6B, FAM_CAM, CONF_MED,"DAHUA"},
  {0x3C,0xEF,0x8C, FAM_CAM, CONF_MED,"DAHUA"},
  {0x4C,0x11,0xBF, FAM_CAM, CONF_MED,"DAHUA"},
  {0x5C,0xF5,0x1A, FAM_CAM, CONF_MED,"DAHUA"},
  {0x64,0xFD,0x29, FAM_CAM, CONF_MED,"DAHUA"},
  {0x6C,0x1C,0x71, FAM_CAM, CONF_MED,"DAHUA"},
  {0x74,0xC9,0x29, FAM_CAM, CONF_MED,"DAHUA"},
  {0x8C,0xE9,0xB4, FAM_CAM, CONF_MED,"DAHUA"},
  {0x90,0x02,0xA9, FAM_CAM, CONF_MED,"DAHUA"},
  {0x98,0xF9,0xCC, FAM_CAM, CONF_MED,"DAHUA"},
  {0x9C,0x14,0x63, FAM_CAM, CONF_MED,"DAHUA"},
  {0xA0,0xBD,0x1D, FAM_CAM, CONF_MED,"DAHUA"},
  {0xB4,0x4C,0x3B, FAM_CAM, CONF_MED,"DAHUA"},
  {0xBC,0x32,0x5F, FAM_CAM, CONF_MED,"DAHUA"},
  {0xC0,0x39,0x5A, FAM_CAM, CONF_MED,"DAHUA"},
  {0xC4,0xAA,0xC4, FAM_CAM, CONF_MED,"DAHUA"},
  {0xD4,0x43,0x0E, FAM_CAM, CONF_MED,"DAHUA"},
  {0xE0,0x2E,0xFE, FAM_CAM, CONF_MED,"DAHUA"},
  {0xE0,0x50,0x8B, FAM_CAM, CONF_MED,"DAHUA"},
  {0xE4,0x24,0x6C, FAM_CAM, CONF_MED,"DAHUA"},
  {0xF4,0xB1,0xC2, FAM_CAM, CONF_MED,"DAHUA"},
  {0xFC,0x5F,0x49, FAM_CAM, CONF_MED,"DAHUA"},
  {0xFC,0xB6,0x9D, FAM_CAM, CONF_MED,"DAHUA"},
  {0x30,0xDD,0xAA, FAM_CAM, CONF_MED,"DAHUA"},
  {0x40,0x7A,0xA4, FAM_CAM, CONF_MED,"DAHUA"},
  {0x4C,0x99,0xE8, FAM_CAM, CONF_MED,"DAHUA"},
  {0xF8,0xCE,0x07, FAM_CAM, CONF_MED,"DAHUA"},
  {0xA8,0xCA,0x87, FAM_CAM, CONF_MED,"DAHUA"},
  {0x48,0xEA,0x63, FAM_CAM, CONF_MED,"UNIVIEW"},
  {0x6C,0xF1,0x7E, FAM_CAM, CONF_MED,"UNIVIEW"},
  {0x88,0x26,0x3F, FAM_CAM, CONF_MED,"UNIVIEW"},
  {0xC4,0x79,0x05, FAM_CAM, CONF_MED,"UNIVIEW"},
  {0x0C,0xA6,0x4C, FAM_CAM, CONF_MED,"EZVIZ"},
  {0x20,0xBB,0xBC, FAM_CAM, CONF_MED,"EZVIZ"},
  {0x34,0xC6,0xDD, FAM_CAM, CONF_MED,"EZVIZ"},
  {0x54,0xD6,0x0D, FAM_CAM, CONF_MED,"EZVIZ"},
  {0x58,0x8F,0xCF, FAM_CAM, CONF_MED,"EZVIZ"},
  {0x64,0x24,0x4D, FAM_CAM, CONF_MED,"EZVIZ"},
  {0x64,0xF2,0xFB, FAM_CAM, CONF_MED,"EZVIZ"},
  {0x78,0xA6,0xA0, FAM_CAM, CONF_MED,"EZVIZ"},
  {0x78,0xC1,0xAE, FAM_CAM, CONF_MED,"EZVIZ"},
  {0x94,0xEC,0x13, FAM_CAM, CONF_MED,"EZVIZ"},
  {0xAC,0x1C,0x26, FAM_CAM, CONF_MED,"EZVIZ"},
  {0xEC,0x97,0xE0, FAM_CAM, CONF_MED,"EZVIZ"},
  {0xF4,0x70,0x18, FAM_CAM, CONF_MED,"EZVIZ"},
  {0x00,0x1F,0x54, FAM_CAM, CONF_MED,"LOREX"},
  {0xBC,0x51,0xFE, FAM_CAM, CONF_MED,"SWANN"},
  {0x00,0x65,0x1E, FAM_CAM, CONF_MED,"AMCREST"},
  {0x9C,0x8E,0xCD, FAM_CAM, CONF_MED,"AMCREST"},
  {0xA0,0x60,0x32, FAM_CAM, CONF_MED,"AMCREST"},
  {0x00,0x02,0xD1, FAM_CAM, CONF_MED,"VIVOTEK"},
  {0x00,0x13,0xE2, FAM_CAM, CONF_MED,"GEOVISION"},
  {0x00,0x03,0xC5, FAM_CAM, CONF_MED,"MOBOTIX"},
  {0x00,0x40,0x8C, FAM_CAM, CONF_MED,"AXIS"},
  {0xAC,0xCC,0x8E, FAM_CAM, CONF_MED,"AXIS"},
  {0xB8,0xA4,0x4F, FAM_CAM, CONF_MED,"AXIS"},
  {0xE8,0x27,0x25, FAM_CAM, CONF_MED,"AXIS"},
  {0x8C,0x1D,0x55, FAM_CAM, CONF_MED,"HANWHA"},
  {0xE4,0x30,0x22, FAM_CAM, CONF_MED,"HANWHA"},
  {0x44,0xB4,0x23, FAM_CAM, CONF_MED,"HANWHA"},
  {0x00,0x09,0x18, FAM_CAM, CONF_MED,"HANWHA"},
  {0x54,0x2B,0x57, FAM_CAM, CONF_MED,"NIGHTOWL"},
  {0x04,0x5C,0x06, FAM_CAM, CONF_MED,"ZMODO"},
  {0xF8,0x51,0x28, FAM_CAM, CONF_MED,"SIMPLISAFE"},
  {0x68,0xF0,0xD0, FAM_CAM, CONF_MED,"SKYBELL"},
  {0x9C,0x54,0xDA, FAM_CAM, CONF_MED,"SKYBELL"},
  {0xD0,0xC1,0x93, FAM_CAM, CONF_MED,"SKYBELL"},
  {0x00,0xC0,0xFA, FAM_CAM, CONF_MED,"CANARY"},
  {0xD8,0x42,0xE2, FAM_CAM, CONF_MED,"CANARY"},
  {0x84,0xEB,0x3F, FAM_CAM, CONF_MED,"VIVINT"},
  {0x84,0xEB,0x3E, FAM_CAM, CONF_MED,"VIVINT"},
  {0x5C,0x2B,0xF5, FAM_CAM, CONF_MED,"VIVINT"},
  {0xA0,0xFE,0x61, FAM_CAM, CONF_MED,"VIVINT"},
  {0x58,0x70,0xC6, FAM_CAM, CONF_MED,"YI"},
  {0x70,0xEE,0x50, FAM_CAM, CONF_LOW,"NETATMO"},
  {0x64,0x16,0x66, FAM_NEST, CONF_LOW,"NEST"},
  {0x04,0x17,0xB6, FAM_EUFY, CONF_LOW,"EUFY"},
  {0x10,0x2C,0xB1, FAM_EUFY, CONF_LOW,"EUFY"},
  {0x2C,0x8D,0x48, FAM_EUFY, CONF_LOW,"EUFY"},
  {0x90,0xBF,0xD9, FAM_EUFY, CONF_LOW,"EUFY"},
  {0x7C,0xE9,0x13, FAM_EUFY, CONF_LOW,"EUFY"},
  {0xAC,0x12,0x2F, FAM_EUFY, CONF_LOW,"EUFY"},
  {0xE8,0xEE,0xCC, FAM_EUFY, CONF_LOW,"EUFY"},
  {0xF4,0x9D,0x8A, FAM_EUFY, CONF_LOW,"EUFY"},
  {0x00,0x0B,0xAA, FAM_CAM, CONF_LOW,"INTERCOM"},
  {0x7C,0x1E,0xB3, FAM_CAM, CONF_LOW,"INTERCOM"},
  {0x0C,0x11,0x05, FAM_CAM, CONF_LOW,"INTERCOM"},
  {0x64,0x74,0xF6, FAM_GUNSHOT, CONF_MED,"SDS"},
  {0x00,0x18,0xAE, FAM_CAM, CONF_MED,"TVT"},
  {0xBC,0xF8,0x11, FAM_CAM, CONF_LOW,"INTERCOM"},
  {0xBC,0x29,0x78, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x3C,0xDA,0x6D, FAM_CAM, CONF_MED,"TIANDY"},
  {0x58,0x5B,0x69, FAM_CAM, CONF_MED,"TVT"},
  {0xCC,0x13,0xF3, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x74,0x13,0x48, FAM_CAM, CONF_MED,"BLINK"},
  {0xFC,0x24,0x22, FAM_CAM, CONF_MED,"EZVIZ"},
  {0xB0,0xFF,0x0D, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x14,0xBA,0x88, FAM_CAM, CONF_MED,"UNIVIEW"},
  {0x5C,0xD1,0x9C, FAM_EUFY, CONF_LOW,"EUFY"},
  {0x00,0x7F,0x1D, FAM_EUFY, CONF_LOW,"EUFY"},
  {0x20,0x2C,0x05, FAM_CAM, CONF_MED,"DAHUA"},
  {0x38,0xF2,0x5D, FAM_CAM, CONF_MED,"EZVIZ"},
  {0xC8,0x19,0xD8, FAM_CAM, CONF_MED,"BLINK"},
  {0x24,0xB1,0x05, FAM_CAM, CONF_MED,"HIKVISION"},
  {0xE4,0xE6,0x6C, FAM_CAM, CONF_MED,"TIANDY"},
  {0x40,0xB5,0x70, FAM_CAM, CONF_MED,"HIKVISION"},
  {0x00,0x40,0xDE, FAM_FLOCK, CONF_LOW,"ELSAG"},
  {0xBC,0x51,0x6C, FAM_CAM, CONF_MED,"TIANDY"},
  // ---- police body and in-car camera makers (besides Axon) ----
  {0xFC,0x01,0x9E, FAM_BODYCAM, CONF_MED,"VIEVU"},
  {0x9C,0x83,0xBF, FAM_BODYCAM, CONF_MED,"PRO-VISION"},
  {0x00,0x1B,0xBE, FAM_BODYCAM, CONF_MED,"ICOP"},
  {0x48,0x46,0x8D, FAM_BODYCAM, CONF_MED,"ZEPCAM"},
};
// clang-format on
static const int WIFI_OUI_N = sizeof(WIFI_OUI) / sizeof(WIFI_OUI[0]);
// clang-format off
static const SigText SSID_RULE[] = {
  {"Flock",FAM_FLOCK,CONF_HIGH},{"FS Ext Battery",FAM_FLOCK,CONF_HIGH},
  {"Penguin",FAM_FLOCK,CONF_HIGH},{"Pigvision",FAM_FLOCK,CONF_HIGH},
  {"WYZE",FAM_WYZE,CONF_HIGH},{"wzap",FAM_WYZE,CONF_HIGH},
  {"Blink-",FAM_CAM,CONF_MED},{"Arlo",FAM_CAM,CONF_MED},
  {"AB2-",FAM_AXON,CONF_HIGH},{"AB3-",FAM_AXON,CONF_HIGH},
  {"AB4-",FAM_AXON,CONF_HIGH},{"AXON-",FAM_AXON,CONF_HIGH},
  {"Ring-",FAM_RING,CONF_HIGH},{"ring-doorbell",FAM_RING,CONF_HIGH},
  {"Tapo_C",FAM_TAPO,CONF_HIGH},{"Tapo_",FAM_TAPO,CONF_MED},
  {"Reolink",FAM_REOLINK,CONF_HIGH},{"eufyCam",FAM_EUFY,CONF_HIGH},
  {"eufy",FAM_EUFY,CONF_MED},{"Arlo",FAM_CAM,CONF_MED},
  {"Nest",FAM_NEST,CONF_MED},{"IPC-",FAM_CAM,CONF_LOW},
  {"Ray-Ban",FAM_GLASS,CONF_HIGH},{"RayBan",FAM_GLASS,CONF_HIGH},
  {"DJI-",FAM_DRONE,CONF_HIGH},{"DJI_",FAM_DRONE,CONF_HIGH},
  {"MAVIC",FAM_DRONE,CONF_HIGH},{"MINI-",FAM_DRONE,CONF_HIGH},
  {"AIR2",FAM_DRONE,CONF_HIGH},{"AVATA",FAM_DRONE,CONF_HIGH},
  {"Autel",FAM_DRONE,CONF_HIGH},{"Skydio",FAM_DRONE,CONF_HIGH},
  {"Parrot",FAM_DRONE,CONF_HIGH},{"ANAFI",FAM_DRONE,CONF_HIGH},
  {"Phantom",FAM_DRONE,CONF_HIGH},{"Inspire",FAM_DRONE,CONF_HIGH},
  {"Mini2",FAM_DRONE,CONF_HIGH},{"MINI2",FAM_DRONE,CONF_HIGH},
  {"FPV-",FAM_DRONE,CONF_MED},
  {"FIRSTNET",FAM_WIFI,CONF_LOW},
  {"LEnet",FAM_WIFI,CONF_LOW},
  {"test_flck",FAM_FLOCK,CONF_HIGH},
  {"flck",FAM_FLOCK,CONF_LOW},
};
// clang-format on
static const int SSID_RULE_N = sizeof(SSID_RULE) / sizeof(SSID_RULE[0]);
// clang-format off
static const SigOUI BLE_OUI[] = {
  {0x00,0x25,0xDF,FAM_AXON,CONF_HIGH,"AXON"},
  {0xB4,0x1E,0x52,FAM_FLOCK,CONF_HIGH,"FLOCK"},
  {0x7C,0x2A,0x9E,FAM_GLASS,CONF_MED,"META"},
  {0xCC,0x66,0x0A,FAM_GLASS,CONF_LOW,"META"},
  {0x98,0x59,0x49,FAM_GLASS,CONF_MED,"LUXOTTICA"},
  {0x98,0xDA,0x92,FAM_GLASS,CONF_MED,"VUZIX"},
  {0x60,0x60,0x1F,FAM_DRONE,CONF_HIGH,"DJI"},
  {0x34,0xD2,0x62,FAM_DRONE,CONF_HIGH,"DJI"},
  {0x00,0x30,0x44, FAM_BT, CONF_MED,"CRADLEPOINT"},
  {0x00,0xE0,0x1C, FAM_BT, CONF_MED,"CRADLEPOINT"},
  {0x00,0xA0,0xD5, FAM_BT, CONF_MED,"SIERRA"},
  {0x28,0xA3,0x31, FAM_BT, CONF_MED,"SIERRA"},
  {0x50,0x13,0x9D, FAM_BT, CONF_MED,"SIERRA"},
  {0x64,0xCE,0x6E, FAM_BT, CONF_MED,"SIERRA"},
  {0x84,0xDB,0x2F, FAM_BT, CONF_MED,"SIERRA"},
  {0xCC,0x93,0x4A, FAM_BT, CONF_MED,"SIERRA"},
  {0x00,0x11,0x6E, FAM_BT, CONF_MED,"PEPLINK"},
  {0x10,0x56,0xCA, FAM_BT, CONF_MED,"PEPLINK"},
  {0x6C,0xA3,0xD3, FAM_BT, CONF_MED,"PEPLINK"},
  {0xD4,0x13,0xF8, FAM_BT, CONF_MED,"PEPLINK"},
  {0x00,0x23,0xBD, FAM_BODYCAM, CONF_MED,"DIGITAL ALLY"},
  {0x00,0x1D,0x96, FAM_BODYCAM, CONF_MED,"WATCHGUARD"},
  {0xD4,0x2D,0xC5, FAM_CAM, CONF_MED,"I-PRO"},
  {0x00,0x09,0xBC, FAM_BODYCAM, CONF_MED,"UTILITY"},
  {0x00,0x16,0xED, FAM_BODYCAM, CONF_MED,"UTILITY"},
  {0x00,0x04,0x7D, FAM_BT, CONF_LOW,"MOTOROLA"},
  {0x00,0x18,0x85, FAM_BT, CONF_LOW,"MOTOROLA"},
  {0x00,0x1F,0x92, FAM_BT, CONF_LOW,"MOTOROLA"},
  {0x4C,0xCC,0x34, FAM_BT, CONF_LOW,"MOTOROLA"},
  {0x10,0x74,0x6F, FAM_BT, CONF_MED,"MOTOROLA"},
  {0xB8,0xE2,0x8C, FAM_BT, CONF_MED,"MOTOROLA"},
  {0x9C,0x86,0x2B, FAM_BT, CONF_MED,"MOTOROLA"},
  {0x9C,0x06,0x6E, FAM_BT, CONF_LOW,"HYTERA"},
  {0x64,0x69,0xBC, FAM_BT, CONF_LOW,"HYTERA"},
  {0x4C,0x36,0x4E, FAM_BT, CONF_LOW,"TOUGHBOOK"},
  {0xB8,0x20,0x8E, FAM_BT, CONF_LOW,"TOUGHBOOK"},
  {0xBC,0x3E,0x0B, FAM_BT, CONF_LOW,"TOUGHBOOK"},
  {0x00,0x05,0x12, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x00,0x15,0x70, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x00,0x23,0x68, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x00,0xA0,0xF8, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x40,0x83,0xDE, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x48,0x8E,0xB7, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x60,0x95,0x32, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x78,0xB8,0xD6, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x84,0x24,0x8D, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x88,0xBC,0xAC, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x90,0x75,0xDE, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x94,0xFB,0x29, FAM_BT, CONF_LOW,"ZEBRA"},
  {0xC4,0x7D,0xCC, FAM_BT, CONF_LOW,"ZEBRA"},
  {0xC8,0x1C,0xFE, FAM_BT, CONF_LOW,"ZEBRA"},
  {0xFC,0x59,0x7A, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x00,0x07,0x4D, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x74,0x93,0xA4, FAM_BT, CONF_LOW,"ZEBRA"},
  {0x00,0x07,0xBE, FAM_BT, CONF_LOW,"DATALOGIC"},
  {0x00,0x11,0x83, FAM_BT, CONF_LOW,"DATALOGIC"},
  {0x00,0x18,0x67, FAM_BT, CONF_LOW,"DATALOGIC"},
  {0x94,0x7B,0xBE, FAM_FLOCK, CONF_MED,"UBICQUIA"},
  {0xE0,0xA7,0x00, FAM_CAM, CONF_MED,"VERKADA"},
  {0x70,0x1A,0xD5, FAM_CAM, CONF_LOW,"AVIGILON"},
  {0xD4,0x11,0xD6, FAM_GUNSHOT, CONF_MED,"SHOTSPOTTER"},
  {0x18,0x7F,0x88, FAM_CAM, CONF_LOW,"RING"},
  {0x24,0x2B,0xD6, FAM_CAM, CONF_LOW,"RING"},
  {0x34,0x3E,0xA4, FAM_CAM, CONF_LOW,"RING"},
  {0x54,0xE0,0x19, FAM_CAM, CONF_LOW,"RING"},
  {0x5C,0x47,0x5E, FAM_CAM, CONF_LOW,"RING"},
  {0x64,0x9A,0x63, FAM_CAM, CONF_LOW,"RING"},
  {0x90,0x48,0x6C, FAM_CAM, CONF_LOW,"RING"},
  {0x9C,0x76,0x13, FAM_CAM, CONF_LOW,"RING"},
  {0xAC,0x9F,0xC3, FAM_CAM, CONF_LOW,"RING"},
  {0xC4,0xDB,0xAD, FAM_CAM, CONF_LOW,"RING"},
  {0xCC,0x3B,0xFB, FAM_CAM, CONF_LOW,"RING"},
  {0x64,0x74,0xF6, FAM_GUNSHOT, CONF_MED,"SDS"},
};
// clang-format on
static const int BLE_OUI_N = sizeof(BLE_OUI) / sizeof(BLE_OUI[0]);
// clang-format off
static const SigText BLE_NAME[] = {
  {"AXON",FAM_AXON,CONF_HIGH},{"TASER",FAM_AXON,CONF_HIGH},{"BWCDEVICE",FAM_AXON,CONF_HIGH},
  {"FS Ext Battery",FAM_FLOCK,CONF_HIGH},{"Penguin",FAM_FLOCK,CONF_HIGH},
  {"rayban",FAM_GLASS,CONF_HIGH},{"ray-ban",FAM_GLASS,CONF_HIGH},{"meta_rb",FAM_GLASS,CONF_HIGH},
  {"spectacles",FAM_GLASS,CONF_HIGH},{"vuzix",FAM_GLASS,CONF_HIGH},
  {"moverio",FAM_GLASS,CONF_HIGH},{"rayneo",FAM_GLASS,CONF_HIGH},
  {"DJI",FAM_DRONE,CONF_HIGH},{"MAVIC",FAM_DRONE,CONF_HIGH},
  {"Tile",FAM_TAG,CONF_HIGH},{"tile_",FAM_TAG,CONF_HIGH},
  {"AirTag",FAM_TAG,CONF_HIGH},{"SmartTag",FAM_TAG,CONF_HIGH},
  {"Chipolo",FAM_TAG,CONF_HIGH},{"Pebblebee",FAM_TAG,CONF_HIGH},
  {"BodyWorn Remote",FAM_CAM,CONF_MED},
  {"Wayfarer",FAM_GLASS,CONF_MED},
  {"Oakley Meta",FAM_GLASS,CONF_MED},
  {"FS Ext",FAM_FLOCK,CONF_HIGH},
};
// clang-format on
static const int BLE_NAME_N = sizeof(BLE_NAME) / sizeof(BLE_NAME[0]);
static const char* FAM_NAME[] = {"UNK", "FLOCK", "AXON",  "RING", "WYZE", "TAPO", "REOLINK", "EUFY",   "NEST",
                                 "CAM", "GLASS", "DRONE", "BT",   "TAG",  "WIFI", "ZIGBEE",  "THREAD", "802.15.4", "GUNSHOT", "BODYCAM"};
// clang-format off
static const SigCid BLE_CID[] = {
  {0x01AB,FAM_GLASS,CONF_HIGH,"META"},{0x0D53,FAM_GLASS,CONF_HIGH,"LUXOTTICA"},
  {0x03C2,FAM_GLASS,CONF_HIGH,"SNAP"},{0x060C,FAM_GLASS,CONF_HIGH,"VUZIX"},
  {0x058E,FAM_GLASS,CONF_MED,"META-TECH"},{0x0BC6,FAM_GLASS,CONF_MED,"RAYNEO"},
  {0x067C,FAM_TAG,CONF_HIGH,"TILE"},
  {0x034D,FAM_AXON,CONF_MED,"AXON"},
  {0x04EC,FAM_BT,CONF_LOW,"MOTOROLA"},
  {0x0008,FAM_BT,CONF_LOW,"MOTOROLA"},
  {0x09C8,FAM_FLOCK,CONF_HIGH,"FLOCK"},
  {0x0C19,FAM_CAM,CONF_MED,"ARLO"},{0x0E25,FAM_CAM,CONF_MED,"HIKVISION"},
  {0x06B1,FAM_CAM,CONF_MED,"SIMPLISAFE"},{0x0DE8,FAM_CAM,CONF_MED,"VIVINT"},
  {0x0870,FAM_WYZE,CONF_LOW,"WYZE"},{0x0CC2,FAM_EUFY,CONF_LOW,"ANKER"},
};
// clang-format on
static const int BLE_CID_N = sizeof(BLE_CID) / sizeof(BLE_CID[0]);
static const uint8_t CHANS[] = {1, 6, 11, 2, 7, 3, 8, 4, 9, 5, 10, 12, 13};  // 2.4 GHz
static const int CHANS_N = sizeof(CHANS) / sizeof(CHANS[0]);
// ---- Signatures from the LE signature files: longer prefixes, anchored patterns, Bluetooth service UUIDs ----
static const SigOUIX WIFI_OUIX[] = {
    {{0x08, 0x3C, 0x03, 0x00, 0x00}, 28, FAM_WIFI, CONF_LOW, "FEDSIGNAL"},
    {{0x00, 0x50, 0xC2, 0xE8, 0x20}, 36, FAM_WIFI, CONF_LOW, "XPLORE"},
    {{0xA4, 0xDA, 0x22, 0x20, 0x00}, 28, FAM_WYZE, CONF_MED, "WYZE"},
    {{0x34, 0x46, 0x63, 0x20, 0x00}, 28, FAM_CAM, CONF_MED, "AMCREST"},
    {{0x7C, 0x70, 0xBC, 0x50, 0x00}, 28, FAM_CAM, CONF_MED, "CANARY"},
    {{0x70, 0xB3, 0xD5, 0xEB, 0x20}, 36, FAM_GUNSHOT, CONF_MED, "SDS"},
    {{0x64, 0x33, 0xB5, 0x40, 0x00}, 28, FAM_CAM, CONF_MED, "EAGLEEYE"},
    {{0x18, 0x74, 0xE2, 0xB0, 0x00}, 28, FAM_CAM, CONF_MED, "JOOAN"},
    {{0x74, 0x33, 0x36, 0xA0, 0x00}, 28, FAM_CAM, CONF_MED, "JOOAN"},
    {{0xA0, 0x24, 0x90, 0xA0, 0x00}, 28, FAM_CAM, CONF_MED, "JOOAN"},
    {{0x4C, 0x81, 0xBE, 0x90, 0x00}, 28, FAM_CAM, CONF_MED, "JOOAN"},
    {{0xCC, 0xE7, 0xDE, 0x00, 0x00}, 28, FAM_CAM, CONF_MED, "JOOAN"},
    {{0x0C, 0x0E, 0xC1, 0x40, 0x00}, 28, FAM_CAM, CONF_MED, "SWANN"},
    {{0x0C, 0xCC, 0x47, 0x00, 0x00}, 28, FAM_CAM, CONF_MED, "JOOAN"},
    {{0xB0, 0xC5, 0xCA, 0x30, 0x00}, 28, FAM_CAM, CONF_MED, "ABODE"},
    {{0x9C, 0xE5, 0x49, 0x30, 0x00}, 28, FAM_CAM, CONF_MED, "JOOAN"},
    {{0x5C, 0x5C, 0x75, 0xD0, 0x00}, 28, FAM_CAM, CONF_MED, "JOOAN"},
    {{0x2C, 0x69, 0x1D, 0x20, 0x00}, 28, FAM_CAM, CONF_MED, "ABODE"},
    {{0x34, 0xC8, 0xD6, 0xA0, 0x00}, 28, FAM_CAM, CONF_MED, "JOOAN"},
    {{0x4C, 0x6E, 0x44, 0xE0, 0x00}, 28, FAM_CAM, CONF_MED, "JOOAN"},
    {{0x70, 0xB3, 0xD5, 0xC4, 0xB0}, 36, FAM_EUFY, CONF_LOW, "EUFY"},
    {{0x8C, 0x1F, 0x64, 0xB1, 0x30}, 36, FAM_CAM, CONF_MED, "ABODE"},
    {{0x70, 0xB3, 0xD5, 0x1C, 0x50}, 36, FAM_FLOCK, CONF_LOW, "ELSAG"},
    {{0x38, 0x73, 0xEA, 0x00, 0x00}, 28, FAM_BODYCAM, CONF_MED, "L3-MOBILEVISION"},
    {{0, 0, 0, 0, 0}, 0, FAM_UNKNOWN, CONF_LOW, ""},
};
static const int WIFI_OUIX_N = sizeof(WIFI_OUIX) / sizeof(WIFI_OUIX[0]) - 1;
static const SigOUIX BLE_OUIX[] = {
    {{0x08, 0x3C, 0x03, 0x00, 0x00}, 28, FAM_BT, CONF_LOW, "FEDSIGNAL"},
    {{0x00, 0x50, 0xC2, 0xE8, 0x20}, 36, FAM_BT, CONF_LOW, "XPLORE"},
    {{0x70, 0xB3, 0xD5, 0xEB, 0x20}, 36, FAM_GUNSHOT, CONF_MED, "SDS"},
    {{0, 0, 0, 0, 0}, 0, FAM_UNKNOWN, CONF_LOW, ""},
};
static const int BLE_OUIX_N = sizeof(BLE_OUIX) / sizeof(BLE_OUIX[0]) - 1;
// clang-format off
static const SigPat SSID_PAT[] = {
  {"^PEPWAVE_[0-9A-F]{4}$",FAM_WIFI,CONF_MED,"PEPLINK"},
  {"^IBR[0-9]{3,4}-[0-9A-Fa-f]{4}$",FAM_WIFI,CONF_LOW,"CRADLEPOINT"},
  {"^R[12][0-9]{3}-[0-9A-Fa-f]{4}$",FAM_WIFI,CONF_LOW,"CRADLEPOINT"},
  {"^Cradlepoint-[0-9A-Fa-f]{4}$",FAM_WIFI,CONF_LOW,"CRADLEPOINT"},
  {"^Sierra[0-9]+_[0-9A-Fa-f]{4}$",FAM_WIFI,CONF_LOW,"SIERRA"},
  {"^MP70_[0-9A-Fa-f]{4}$",FAM_WIFI,CONF_LOW,"SIERRA"},
  {"^RV50_[0-9A-Fa-f]{4}$",FAM_WIFI,CONF_LOW,"SIERRA"},
  {"^ABX-[0-9A-Fa-f]{6}$",FAM_AXON,CONF_LOW,"AXON"},
  {"^AB[34]-[0-9A-Fa-f]{6}$",FAM_AXON,CONF_LOW,"AXON"},
  {"^AF3-[0-9A-Fa-f]{6}$",FAM_AXON,CONF_LOW,"AXON"},
  {"^VM-[0-9A-Fa-f]{6,}$",FAM_BODYCAM,CONF_LOW,"MOTOROLA"},
  {"^SI500-[0-9A-Fa-f]{4,}$",FAM_BODYCAM,CONF_LOW,"MOTOROLA"},
  {"^V300-[0-9A-Fa-f]{4,}$",FAM_BODYCAM,CONF_LOW,"WATCHGUARD"},
  {"^4RE-[0-9A-Fa-f]{4,}$",FAM_BODYCAM,CONF_LOW,"WATCHGUARD"},
  {"^FirstVu-[0-9A-Fa-f]{4,}$",FAM_BODYCAM,CONF_LOW,"DIGITAL ALLY"},
  {"^COBAN[-_ ]?[0-9A-Fa-f]{3,}$",FAM_BODYCAM,CONF_LOW,"COBAN"},
  {"^PD[- _]?[A-Z]{2,4}$",FAM_WIFI,CONF_LOW,"LE NETWORK"},
  {"^SO[- _]?[A-Z]{2,4}$",FAM_WIFI,CONF_LOW,"LE NETWORK"},
  {"", FAM_UNKNOWN, CONF_LOW, ""},
};
// clang-format on
static const int SSID_PAT_N = sizeof(SSID_PAT) / sizeof(SSID_PAT[0]) - 1;
// clang-format off
static const SigPat BLE_PAT[] = {
  {"^XL200-[0-9A-Z]+$",FAM_BT,CONF_LOW,"L3HARRIS"},
  {"^TAIT-[0-9A-Z]+$",FAM_BT,CONF_LOW,"TAIT"},
  {"^Feniex 4200$",FAM_BT,CONF_HIGH,"FENIEX"},
  {"^ConfigureIT",FAM_BT,CONF_LOW,"SOUNDOFF"},
  {"^ECTBM",FAM_BT,CONF_LOW,"SOUNDOFF"},
  {"^SOS-",FAM_BT,CONF_LOW,"SOUNDOFF"},
  {"^SL\\+",FAM_BT,CONF_LOW,"WHELEN"},
  {"^ScanLock",FAM_BT,CONF_LOW,"WHELEN"},
  {"^PROGRAM2",FAM_BT,CONF_LOW,"WHELEN"},
  {"", FAM_UNKNOWN, CONF_LOW, ""},
};
// clang-format on
static const int BLE_PAT_N = sizeof(BLE_PAT) / sizeof(BLE_PAT[0]) - 1;
// 16-bit service UUIDs seen in advertisements
// clang-format off
static const SigCid BLE_UUID[] = {
  {0xFC81,FAM_AXON,CONF_MED,"AXON"},
  {0x3100,FAM_GUNSHOT,CONF_MED,"RAVEN"},
  {0x3200,FAM_GUNSHOT,CONF_MED,"RAVEN"},
  {0x3300,FAM_GUNSHOT,CONF_MED,"RAVEN"},
  {0x3400,FAM_GUNSHOT,CONF_MED,"RAVEN"},
  {0x3500,FAM_GUNSHOT,CONF_MED,"RAVEN"},
  {0x0000, FAM_UNKNOWN, CONF_LOW, ""},
};
// clang-format on
static const int BLE_UUID_N = sizeof(BLE_UUID) / sizeof(BLE_UUID[0]) - 1;
static const char* CAT_LABEL[] = {"ALPR", "AXON", "CAMS", "GLASS", "DRONE", "BT", "TAGS", "WIFI", "ZB/TH"};
static bool catOn[CAT_N] = {false, false, false, false, false, false, false, false, false};
static volatile uint16_t armedMask = 0;
TinyGPSPlus gps;
HardwareSerial GPS(1);
Preferences prefs;
static Contact contacts[48];
static int nContacts = 0;
static int filterCat = -1;
static Screen screen = SCR_BOOT;
static fs::FS* fsStore = nullptr;
// SETUP > Save to: 0 = Auto (SD card first, internal flash if there is no card), 1 = SD card only, 2 = internal flash only
static uint8_t storMode = 0;
static bool needRender = true;
static bool fsOk = false, gpsOn = false, scanning = true, radioOn = false, radioDirty = false;
static bool muted = false, hunting = false, needNewSession = false;
static uint32_t lastHop = 0, bootAt = 0;
static volatile uint32_t framesSeen = 0, bleSeen = 0, zbSeen = 0, sniffCalls = 0;
static volatile uint32_t uiSerial = 0;
static volatile bool findChime = false;
// has this group already alerted since it was armed?
static bool catSeen[CAT_N];
// 0 off, 1 chime on the first find in each armed group, 2 chime on every find
static uint8_t alertMode = 1;
static int chanIdx = 0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
// settings (persisted)
static int bright = 200;
// auto-dim is OFF by default (the screen stays on until you turn it on in Setup)
static bool soundOn = true, autoDim = false;
static int soundVol = 70;                                            // 0..100, 0 = silent (Setup slider)
static bool raiseWake = true, shakePause = false, discreet = false;  // motion features, discreet mode
// power saver: 0 off, 1 auto (battery <= 15%), 2 on
static uint8_t powerMode = 1;
static bool saverOn = false;  // power saver currently active
// 0 portrait, 1 landscape, 2 portrait flipped, 3 landscape flipped, 4 auto (motion sensor)
static uint8_t orient = 0;
// motion-sensor mounting learned by the calibration wizard
static bool calOk = false;
// sensor axis (0..2) and sign that point 'up the screen' and 'out of the screen' (defaults: +Y, +Z)
static int8_t calUi = 1, calUs = 1, calNi = 2, calNs = 1;
// quarter-turns added to the auto orientation (fixes the sensor's mounting direction)
static uint8_t autoOff = 0;
// tracker 'following you' alert after this many minutes (0 = off)
static uint8_t followMin = 10;
// last find that wanted an alert (written under mux, read by the loop)
static FindNote pendFind;
// selection / tracking are by MAC so list re-sorting never moves them
static uint8_t selMac[6];
static bool selValid = false;
static uint8_t huntMac[6];
static int sessionId = 0;
static char logPath[32] = "/skiscan_log_001.csv";
static char huntPath[32] = "/skiscan_hunt_001.csv";
static uint32_t logCount = 0;
// boot screen: 0 = the built-in radar and title, 1 = your own picture (SETUP > Boot logo)
static uint8_t bootLogoMode = 0;
// the picture has been drawn once; from then on only the progress bar is redrawn
static bool bootLogoDrawn = false;
// WATCH: 0 = no boot sound, 1 = built-in chime (default), 2 = your own sound file
static uint8_t bootSoundMode = 1;
// extra time the boot screen stays up so a long custom sound can finish
static uint32_t bootHoldMs = 0;
// finds list: 0 = strongest first, 1 = strong only (above -75 dBm), 2 = newest first, 3 = A-Z
static uint8_t listView = 0;
// how findings are saved: 0 = Manual (SAVE LOG button), 1 = Auto (every 5 min + when you stop), 2 = Live (every new find)
static uint8_t logMode = 0;
static bool logFull = false, logFileMade = false, huntFileMade = false;
static uint16_t logRows = 0;  // rows written to the live log this session
// a message for the screen, set from anywhere and shown by the main loop
static const char* logNotice = nullptr;
// the Live log (one row per new find); SAVE LOG writes logPath instead
static char livePath[32] = "/skiscan_live_001.csv";
#define LOG_CAP_ROWS 1500
// ---- Helpers ---------------------------------------------------------------
static uint16_t catColor(int i) {
  return (i < 0 || i >= CAT_N) ? C_TEAL : CAT_COL[i];
}
static uint16_t famColor(Family f) {
  switch (f) {
    case FAM_FLOCK:
    case FAM_GUNSHOT:
      return CAT_COL[CAT_ALPR];
    case FAM_AXON:
    case FAM_BODYCAM:
      return CAT_COL[CAT_AXON];
    case FAM_GLASS:
      return CAT_COL[CAT_GLASS];
    case FAM_DRONE:
      return CAT_COL[CAT_DRONE];
    case FAM_BT:
      return CAT_COL[CAT_BT];
    case FAM_TAG:
      return CAT_COL[CAT_TAG];
    case FAM_WIFI:
      return CAT_COL[CAT_WIFI];
    case FAM_ZIGBEE:
      return RGB(170, 255, 60);
    case FAM_THREAD:
      return RGB(60, 240, 200);
    case FAM_154:
      return RGB(150, 190, 90);
    default:
      return CAT_COL[CAT_CAM];
  }
}
static const char* famName(Family f) {
  return (f >= FAM_COUNT) ? "UNK" : FAM_NAME[f];
}
static bool macEq(const uint8_t* a, const uint8_t* b) {
  return memcmp(a, b, 6) == 0;
}
static void macStr(const uint8_t* m, char* out) {
  sprintf(out, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}
static bool localAdmin(const uint8_t* m) {
  return (m[0] & 0x02) != 0;
}
static int armedCount() {
  int n = 0;
  for (int i = 0; i < CAT_N; i++)
    if (catOn[i])
      n++;
  return n;
}
static void syncArmed() {
  uint16_t m = 0;
  for (int i = 0; i < CAT_N; i++)
    if (catOn[i])
      m |= (1 << i);
  armedMask = m;
  for (int i = 0; i < CAT_N; i++)
    if (!catOn[i])
      catSeen[i] = false;
}
static bool famWanted(Family f) {
  switch (f) {
    case FAM_FLOCK:
    case FAM_GUNSHOT:
      return catOn[CAT_ALPR];
    case FAM_AXON:
    case FAM_BODYCAM:
      return catOn[CAT_AXON];
    case FAM_GLASS:
      return catOn[CAT_GLASS];
    case FAM_DRONE:
      return catOn[CAT_DRONE];
    case FAM_BT:
      return catOn[CAT_BT];
    case FAM_TAG:
      return catOn[CAT_TAG];
    case FAM_WIFI:
      return catOn[CAT_WIFI];
    case FAM_ZIGBEE:
    case FAM_THREAD:
    case FAM_154:
      return catOn[CAT_ZT];
    case FAM_UNKNOWN:
      return false;
    default:
      return catOn[CAT_CAM];
  }
}
static bool famInCat(Family f, int cat) {
  if (cat < 0)
    return true;
  switch (cat) {
    case CAT_ALPR:
      return f == FAM_FLOCK || f == FAM_GUNSHOT;
    case CAT_AXON:
      return f == FAM_AXON || f == FAM_BODYCAM;
    case CAT_CAM:
      return f == FAM_RING || f == FAM_WYZE || f == FAM_TAPO || f == FAM_REOLINK || f == FAM_EUFY || f == FAM_NEST ||
             f == FAM_CAM;
    case CAT_GLASS:
      return f == FAM_GLASS;
    case CAT_DRONE:
      return f == FAM_DRONE;
    case CAT_BT:
      return f == FAM_BT;
    case CAT_TAG:
      return f == FAM_TAG;
    case CAT_WIFI:
      return f == FAM_WIFI;
    case CAT_ZT:
      return f == FAM_ZIGBEE || f == FAM_THREAD || f == FAM_154;
    default:
      return false;
  }
}
static int countCatHits(int cat) {
  int n = 0;
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < nContacts; i++)
    if (famInCat(contacts[i].fam, cat))
      n++;
  portEXIT_CRITICAL(&mux);
  return n;
}
// Wi-Fi sniffing and 802.15.4 share one radio, so they are mutually exclusive. BT and TAGS are BLE-only and work with either side.
static bool isWifiSide(int cat) {
  return cat == CAT_ALPR || cat == CAT_AXON || cat == CAT_CAM || cat == CAT_GLASS || cat == CAT_DRONE ||
         cat == CAT_WIFI;
}
static bool wifiSideArmed() {
  for (int i = 0; i < CAT_N; i++)
    if (isWifiSide(i) && catOn[i])
      return true;
  return false;
}
static bool tileBlocked(int i) {
  if (i == CAT_ZT)
    return wifiSideArmed();
  if (isWifiSide(i))
    return catOn[CAT_ZT];
  return false;
}
static bool ouiMatch(const SigOUI* t, int n, const uint8_t* mac, Family* fam, Conf* conf, const char** tag) {
  for (int i = 0; i < n; i++) {
    if (mac[0] == t[i].b0 && mac[1] == t[i].b1 && mac[2] == t[i].b2) {
      *fam = t[i].fam;
      *conf = t[i].conf;
      *tag = t[i].tag;
      return true;
    }
  }
  return false;
}
// first 'bits' bits of the address
static bool ouiMatchX(const SigOUIX* t, int n, const uint8_t* mac, Family* fam, Conf* conf, const char** tag) {
  for (int i = 0; i < n; i++) {
    int full = t[i].bits / 8, rem = t[i].bits % 8;
    if (memcmp(mac, t[i].p, full) != 0)
      continue;
    if (rem && ((mac[full] ^ t[i].p[full]) & (uint8_t)(0xFF << (8 - rem))))
      continue;
    *fam = t[i].fam;
    *conf = t[i].conf;
    *tag = t[i].tag;
    return true;
  }
  return false;
}
// a tiny pattern matcher: literals, [sets and ranges], . and \x escapes, the quantifiers ? * + {n} {n,} {n,m}, and the anchors ^ and $. Not case sensitive.
// (Alternation is expanded when the tables are generated, so there are no groups.)
static const char* rxAtomEnd(const char* p) {
  if (*p == '\\')
    return p[1] ? p + 2 : p + 1;
  if (*p == '[') {
    const char* q = p + 1;
    if (*q == ']')
      q++;
    while (*q && *q != ']')
      q++;
    return *q ? q + 1 : q;
  }
  return p + 1;
}
static bool rxAtomMatch(const char* a, const char* ae, char c) {
  c = (char)tolower((unsigned char)c);
  if (*a == '.')
    return true;
  if (*a == '\\') {
    char e = a[1];
    if (e == 'd')
      return isdigit((unsigned char)c) != 0;
    if (e == 's')
      return c == ' ';
    return tolower((unsigned char)e) == c;
  }
  if (*a == '[') {
    const char* q = a + 1;
    bool neg = false, in = false;
    if (*q == '^') {
      neg = true;
      q++;
    }
    const char* end = ae - 1;
    while (q < end) {
      if (q + 2 < end && q[1] == '-') {
        if (c >= tolower((unsigned char)q[0]) && c <= tolower((unsigned char)q[2]))
          in = true;
        q += 3;
      } else {
        if (tolower((unsigned char)*q) == c)
          in = true;
        q++;
      }
    }
    return in != neg;
  }
  return tolower((unsigned char)*a) == c;
}
static bool rxHere(const char* p, const char* s) {
  if (*p == 0)
    return true;
  if (*p == '$' && p[1] == 0)
    return *s == 0;
  const char* ae = rxAtomEnd(p);
  const char* q = ae;
  int mn = 1, mx = 1;
  if (*q == '?') {
    mn = 0;
    mx = 1;
    q++;
  } else if (*q == '*') {
    mn = 0;
    mx = 255;
    q++;
  } else if (*q == '+') {
    mn = 1;
    mx = 255;
    q++;
  } else if (*q == '{') {
    int a = 0, b = 0;
    const char* r = q + 1;
    while (isdigit((unsigned char)*r))
      a = a * 10 + (*r++ - '0');
    if (*r == ',') {
      r++;
      if (isdigit((unsigned char)*r)) {
        while (isdigit((unsigned char)*r))
          b = b * 10 + (*r++ - '0');
      } else
        b = 255;
    } else
      b = a;
    if (*r == '}') {
      mn = a;
      mx = b;
      q = r + 1;
    }
  }
  int n = 0;
  while (n < mx && s[n] && rxAtomMatch(p, ae, s[n]))
    n++;
  for (; n >= mn; n--)
    if (rxHere(q, s + n))
      return true;
  return false;
}
static bool rxMatch(const char* pat, const char* s) {
  if (*pat == '^')
    return rxHere(pat + 1, s);
  for (const char* t = s;; t++) {
    if (rxHere(pat, t))
      return true;
    if (!*t)
      break;
  }
  return false;
}
static bool patMatch(const SigPat* t, int n, const char* s, Family* fam, Conf* conf, const char** tag) {
  if (!s || !s[0])
    return false;
  for (int i = 0; i < n; i++)
    if (rxMatch(t[i].pat, s)) {
      *fam = t[i].fam;
      *conf = t[i].conf;
      *tag = t[i].tag;
      return true;
    }
  return false;
}
// ASCII text inside the service-data parts (AD types 0x16, 0x20, 0x21) of an advertisement
static bool adTextFind(const uint8_t* p, int n, const char* text) {
  size_t tl = strlen(text);
  int i = 0;
  while (p && i + 1 < n) {
    int len = p[i];
    if (len == 0 || i + len + 1 > n)
      break;
    uint8_t type = p[i + 1];
    if (type == 0x16 || type == 0x20 || type == 0x21)
      for (int k = i + 2; k + (int)tl <= i + len + 1; k++)
        if (memcmp(p + k, text, tl) == 0)
          return true;
    i += len + 1;
  }
  return false;
}
static bool textMatch(const SigText* t, int n, const char* s, Family* fam, Conf* conf) {
  if (!s || !s[0])
    return false;
  for (int i = 0; i < n; i++) {
    if (strcasestr(s, t[i].needle)) {
      *fam = t[i].fam;
      *conf = t[i].conf;
      return true;
    }
  }
  return false;
}
static int findContact(const uint8_t* mac) {  // call with mux held
  for (int i = 0; i < nContacts; i++)
    if (macEq(contacts[i].mac, mac))
      return i;
  return -1;
}
static bool getContact(const uint8_t* mac, Contact* out) {
  bool ok = false;
  portENTER_CRITICAL(&mux);
  int i = findContact(mac);
  if (i >= 0) {
    *out = contacts[i];
    ok = true;
  }
  portEXIT_CRITICAL(&mux);
  return ok;
}
static void bumpConf(Contact& c, Conf nc) {
  if (nc > c.conf)
    c.conf = nc;
}
static void requestUi() {
  uiSerial++;
}
static int sigBars(float rssi) {
  return rssi > -55 ? 4 : rssi > -67 ? 3 : rssi > -78 ? 2 : rssi > -90 ? 1 : 0;
}
// ---- Log queue (filled from radio callbacks, drained in loop) --------------
#define LQ_N 12
static Contact lq[LQ_N];
static volatile uint8_t lqH = 0, lqT = 0;
static bool lqPush(const Contact& c) {  // mux held
  uint8_t n = (lqH + 1) % LQ_N;
  if (n == lqT)
    return false;
  lq[lqH] = c;
  lqH = n;
  return true;
}
// ---- Bluetooth skimmer watch (gas pumps and ATMs): a Bluetooth module still wearing its factory name ----
static bool skimWatch = true;  // SETUP > Skimmer alert
struct SkimRec {
  uint8_t mac[6];
  uint32_t alertAt;
  uint8_t n;
};
static SkimRec skimRec[3];
static uint8_t skimQ = 0;
static uint8_t skimQMac[3][6];  // warnings waiting for the UI loop
// the last time a suspected skimmer was heard (confirmed sightings only)
static uint32_t skimSeenAt = 0;
// called from upsert (inside the critical section): is this sighting worth a warning?
static void skimNote(int idx) {
  Contact& c = contacts[idx];
  // a fixed device keeps advertising; one stray advert is not enough
  if (c.hits < 3 || (int32_t)(c.lastMs - c.firstMs) < 1500)
    return;
  if (c.peak < -90)
    return;  // far too faint to be anywhere near
  skimSeenAt = c.lastMs;
  int k = -1;
  for (int i = 0; i < 3; i++)
    if (macEq(skimRec[i].mac, c.mac))
      k = i;
  if (k < 0) {
    k = 0;
    for (int i = 1; i < 3; i++)
      if (skimRec[i].alertAt < skimRec[k].alertAt)
        k = i;
    memset(&skimRec[k], 0, sizeof(SkimRec));
    memcpy(skimRec[k].mac, c.mac, 6);
  }
  // again every minute while it stays in range, at most 4 times
  bool due = skimRec[k].n == 0 || (skimRec[k].n < 4 && (int32_t)(c.lastMs - skimRec[k].alertAt) > 60000);
  if (!due || skimQ >= 3)
    return;
  skimRec[k].n++;
  skimRec[k].alertAt = c.lastMs;
  memcpy(skimQMac[skimQ++], c.mac, 6);
}
// a suspected skimmer was heard within the last 3 minutes
static bool skimPresent() {
  return skimSeenAt && (int32_t)(millis() - skimSeenAt) < 180000;
}
static void upsert(const uint8_t* mac, Family fam, Conf conf, Radio radio, int8_t rssi, uint8_t ch, const char* label,
                   const char* method, const RidInfo* rid = nullptr) {
  if (!mac)
    return;
  // 0x80 = the channel came from the beacon itself, not from where the radio was listening
  bool chTrusted = (ch & 0x80) || radio != RAD_WIFI;
  ch &= 0x7F;
  // on the ignore list: never shown, counted or alerted
  if (isIgnored(mac))
    return;
  // a possible skimmer is recorded whichever groups are armed
  bool skimTag = skimWatch && method && strcmp(method, "SKIMMER") == 0;
  if (!famWanted(fam) && !skimTag)
    return;
  // PAUSE freezes the list (tracking keeps updating its target)
  if (!scanning && !hunting)
    return;
  portENTER_CRITICAL(&mux);
  int idx = findContact(mac);
  uint32_t now = millis();
  bool newborn = false;
  if (idx < 0) {
    if (nContacts >= (int)(sizeof(contacts) / sizeof(contacts[0]))) {
      int oldest = -1;
      for (int i = 0; i < nContacts; i++) {
        if (hunting && macEq(contacts[i].mac, huntMac))
          continue;  // never evict what we're tracking
        if (oldest < 0 || contacts[i].lastMs < contacts[oldest].lastMs)
          oldest = i;
      }
      idx = oldest < 0 ? 0 : oldest;
    } else
      idx = nContacts++;
    memset(&contacts[idx], 0, sizeof(Contact));
    memcpy(contacts[idx].mac, mac, 6);
    contacts[idx].fam = fam;
    contacts[idx].conf = conf;
    contacts[idx].radio = radio;
    contacts[idx].peak = rssi;
    contacts[idx].chOk = chTrusted ? 1 : 0;
    contacts[idx].ema = rssi;
    contacts[idx].firstMs = now;
    strncpy(contacts[idx].label, famName(fam), 19);
    // type tag (WEARABLE, LIGHT, IBEACON...) instead of plain BT / WIFI
    if ((fam == FAM_BT || fam == FAM_WIFI) && method && btKindTag(method)) {
      strncpy(contacts[idx].label, method, 19);
      contacts[idx].label[19] = 0;
    }
    strncpy(contacts[idx].method, method ? method : "OUI", 15);
    if (label && label[0] && strcmp(label, "OUI") && strcmp(label, "AP") && strcmp(label, "BT") &&
        strcmp(label, "SSID") && strcmp(label, "TX"))
      strncpy(contacts[idx].nick, label, 23);
    newborn = true;
  } else {
    bumpConf(contacts[idx], conf);
    if (fam != FAM_UNKNOWN && contacts[idx].fam == FAM_UNKNOWN)
      contacts[idx].fam = fam;
    if (fam == FAM_TAG && contacts[idx].fam == FAM_BT)
      contacts[idx].fam = FAM_TAG;
    if (label && label[0] && strcmp(label, "OUI") && strcmp(label, "AP") && strcmp(label, "BT") &&
        strcmp(label, "SSID") && strcmp(label, "TX") && strcmp(label, "RX") && strcmp(label, "WILDPROBE")) {
      if (!contacts[idx].nick[0] || strlen(label) > strlen(contacts[idx].nick))
        strncpy(contacts[idx].nick, label, 23);
    }
    contacts[idx].ema += 0.35f * ((float)rssi - contacts[idx].ema);
  }
  // decoded Remote ID: keep the newest data, show the drone's ID as its name
  if (rid && rid->flags) {
    RidInfo& r = contacts[idx].rid;
    if (rid->flags & 1) {
      strncpy(r.id, rid->id, 20);
      r.id[20] = 0;
      r.flags |= 1;
      strncpy(contacts[idx].nick, rid->id, 23);
      contacts[idx].nick[23] = 0;
    }
    if (rid->flags & 2) {
      r.lat = rid->lat;
      r.lon = rid->lon;
      r.alt = rid->alt;
      r.spd10 = rid->spd10;
      r.flags |= 2;
    }
    if (rid->flags & 4) {
      r.opLat = rid->opLat;
      r.opLon = rid->opLon;
      r.flags |= 4;
    }
  }
  contacts[idx].rssi = rssi;
  if (rssi > contacts[idx].peak)
    contacts[idx].peak = rssi;
  // a channel read from a beacon is never overwritten by a guess
  if (chTrusted || !contacts[idx].chOk) {
    contacts[idx].ch = ch;
    if (chTrusted)
      contacts[idx].chOk = 1;
  }
  contacts[idx].lastMs = now;
  contacts[idx].hits++;
  if (skimTag)
    skimNote(idx);
  if (!contacts[idx].logged && contacts[idx].conf >= CONF_MED) {
    if (lqPush(contacts[idx]))
      contacts[idx].logged = true;  // full queue -> retry on next hit
    if (newborn)
      noteFind(contacts[idx]);  // session statistics + timeline
    // find alert: default = only the FIRST find of each armed group
    if (newborn && alertMode) {
      bool fire = (alertMode == 2);
      for (int k = 0; k < CAT_N; k++)
        if (catOn[k] && famInCat(fam, k) && !catSeen[k]) {
          catSeen[k] = true;
          fire = true;
        }
      if (fire) {
        findChime = true;
        pendCapture(contacts[idx]);
      }
    }
  }
  // the session's findings record (saved on demand)
  ledgerNote(contacts[idx]);
  portEXIT_CRITICAL(&mux);
  if (newborn)
    requestUi();
}
// ---- Backlight (PWM, gamma 2) + display sleep ------------------------------
static int blLevel = 0, blTarget = 0;
static bool screenOff = false;
static void blSet(int lvl) {
  if (lvl < 0)
    lvl = 0;
  if (lvl > 255)
    lvl = 255;
  blLevel = lvl;
  ledcWrite(PIN_TFT_BL, (uint32_t)lvl * lvl * 1023UL / (255UL * 255UL));
}
// display off / on (0x28 / 0x29) and the backlight follows
static void panelSleep(bool off) {
  tft.writecommand(off ? 0x28 : 0x29);
  ledcWrite(PIN_TFT_BL, off ? 0 : (uint32_t)blLevel * blLevel * 1023UL / (255UL * 255UL));
  screenOff = off;
}
// ---- GPS + log writer (loop context only) ----------------------------------
// last fix, kept as plain numbers for the radio callbacks
static bool gpsHave = false;
static int32_t gpsLat = 0, gpsLon = 0;
static void serviceGps() {
  while (GPS.available())
    gps.encode(GPS.read());
  gpsOn = gps.location.isValid() && gps.location.age() < 3000;
  gpsHave = gpsOn;
  if (gpsOn) {
    gpsLat = (int32_t)(gps.location.lat() * 1e6);
    gpsLon = (int32_t)(gps.location.lng() * 1e6);
  }
}
// bytes used / total on whichever storage the logs go to
static bool fsSpace(uint64_t* used, uint64_t* total) {
  if (!fsOk || !fsStore)
    return false;
#ifdef BOARD_FS_SD
  if (fsStore == (fs::FS*)&SD) {
    *used = SD.usedBytes();
    *total = SD.totalBytes();
    return *total > 0;
  }
#endif
  *used = LittleFS.usedBytes();
  *total = LittleFS.totalBytes();
  return *total > 0;
}
// files are created when the first row is written, never before
static bool ensureLogFile(bool hunt) {
  bool& made = hunt ? huntFileMade : logFileMade;
  if (made)
    return true;
  File f = fsStore->open(hunt ? huntPath : livePath, FILE_WRITE);
  if (!f)
    return false;
  f.println(hunt ? "name,mac,ema,rssi,lat,lon"
                 : "time,name,mac,family,radio,method,rssi,ch,lat,lon,sats,rid_lat,rid_lon,rid_alt");
  f.close();
  made = true;
  return true;
}
// Live log guards: row cap and a free-space floor
static bool liveRoom() {
  if (logFull)
    return false;
  if (logRows >= LOG_CAP_ROWS) {
    logFull = true;
    logNotice = "Live log full for this session";
    return false;
  }
  uint64_t u = 0, t = 0;
  if (fsSpace(&u, &t) && t > u && (t - u) < 65536ULL) {
    logFull = true;
    logNotice = "Storage almost full: live log stopped";
    return false;
  }
  return true;
}
// Live mode only: one row per device, once per session
static void logContact(const Contact& c) {
  if (logMode != 2 || !fsOk)
    return;
  if (!ledgerLiveMark(c.mac))
    return;
  if (!liveRoom() || !ensureLogFile(false))
    return;
  File f = fsStore->open(livePath, FILE_APPEND);
  if (!f)
    return;
  char mac[20];
  macStr(c.mac, mac);
  const char* nm = (c.nick[0]) ? c.nick : famName(c.fam);
  const char* rd = c.radio == RAD_BLE ? "BLE" : (c.radio == RAD_ZB ? "ZB" : "WIFI");
  char line[256], rid[48] = ",,";
  if (c.rid.flags & 2)
    snprintf(rid, sizeof(rid), "%.6f,%.6f,%d", c.rid.lat / 1e7, c.rid.lon / 1e7, (int)c.rid.alt);
  if (gps.location.isValid())
    snprintf(line, sizeof(line), "%s,%s,%s,%s,%s,%s,%d,%u,%.6f,%.6f,%u,%s\n", timeStr(), nm, mac, famName(c.fam), rd,
             c.method, (int)c.rssi, c.ch, gps.location.lat(), gps.location.lng(), (unsigned)gps.satellites.value(),
             rid);
  else
    snprintf(line, sizeof(line), "%s,%s,%s,%s,%s,%s,%d,%u,,,%u,%s\n", timeStr(), nm, mac, famName(c.fam), rd, c.method,
             (int)c.rssi, c.ch, (unsigned)gps.satellites.value(), rid);
  f.print(line);
  f.close();
  logCount++;
  logRows++;
}
// Live mode only, and only with a GPS fix (a trail without positions is just noise)
static void logHuntBreadcrumb(const Contact& c, float ema) {
  if (logMode != 2 || !fsOk || !gpsHave || !liveRoom() || !ensureLogFile(true))
    return;
  File f = fsStore->open(huntPath, FILE_APPEND);
  if (!f)
    return;
  char mac[20];
  macStr(c.mac, mac);
  const char* nm = (c.nick[0]) ? c.nick : famName(c.fam);
  if (gps.location.isValid())
    f.printf("%s,%s,%.1f,%d,%.6f,%.6f\n", nm, mac, ema, (int)c.rssi, gps.location.lat(), gps.location.lng());
  else
    f.printf("%s,%s,%.1f,%d,,\n", nm, mac, ema, (int)c.rssi);
  f.close();
}
static void drainLogQueue() {
  for (int guard = 0; guard < 3; guard++) {
    Contact c;
    bool have = false;
    portENTER_CRITICAL(&mux);
    if (lqT != lqH) {
      c = lq[lqT];
      lqT = (lqT + 1) % LQ_N;
      have = true;
    }
    portEXIT_CRITICAL(&mux);
    if (!have)
      break;
    logContact(c);
  }
}
// =============================================================================
//  Bluetooth / Wi-Fi device recognition
// =============================================================================
// Recognises beacons (iBeacon, Eddystone, AltBeacon), wearables, smart lights, Fast Pair accessories, BT Mesh nodes and
// Google Find Hub trackers from the raw advertisement bytes, and names the maker from the company identifier.
// Company names: Bluetooth SIG assigned numbers, via Nordic Semiconductor's bluetooth-numbers-database (BSD 3-Clause licence), shortened for the display.
static const uint16_t BT_CO_ID[] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D,
    0x000E, 0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B,
    0x001C, 0x001D, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029,
    0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0036, 0x0037, 0x0038,
    0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046,
    0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054,
    0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 0x0060, 0x0061, 0x0062,
    0x0063, 0x0064, 0x0065, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072,
    0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007D, 0x007F, 0x0080, 0x0081, 0x0082,
    0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F, 0x0090, 0x0091,
    0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
    0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD,
    0x00AE, 0x00AF, 0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB,
    0x00BC, 0x00BD, 0x00BE, 0x00BF, 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9,
    0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF, 0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D5, 0x00D6, 0x00D7, 0x00D8,
    0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF, 0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E7,
    0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5,
    0x00F7, 0x00F8, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF, 0x0100, 0x0101, 0x0102, 0x0103, 0x0104, 0x0105,
    0x0106, 0x0107, 0x0108, 0x0109, 0x010A, 0x010B, 0x010C, 0x010D, 0x010E, 0x010F, 0x0110, 0x0111, 0x0112, 0x0113,
    0x0115, 0x0116, 0x0117, 0x0118, 0x011A, 0x011B, 0x011C, 0x011D, 0x011E, 0x011F, 0x0120, 0x0121, 0x0122, 0x0123,
    0x0124, 0x0125, 0x0126, 0x0127, 0x0128, 0x0129, 0x012A, 0x012B, 0x012C, 0x012D, 0x012E, 0x012F, 0x0130, 0x0131,
    0x0132, 0x0133, 0x0134, 0x0135, 0x0136, 0x0137, 0x0138, 0x0139, 0x013A, 0x013B, 0x013C, 0x013D, 0x013E, 0x0140,
    0x0141, 0x0142, 0x0143, 0x0144, 0x0145, 0x0146, 0x0147, 0x0148, 0x0149, 0x014A, 0x014B, 0x014C, 0x014D, 0x014E,
    0x014F, 0x0150, 0x0151, 0x0152, 0x0153, 0x0154, 0x0155, 0x0156, 0x0157, 0x0158, 0x0159, 0x015A, 0x015B, 0x015C,
    0x015D, 0x015E, 0x015F, 0x0160, 0x0161, 0x0162, 0x0163, 0x0164, 0x0165, 0x0166, 0x0167, 0x0168, 0x0169, 0x016A,
    0x016B, 0x016C, 0x016D, 0x016E, 0x016F, 0x0170, 0x0171, 0x0172, 0x0173, 0x0174, 0x0175, 0x0176, 0x0177, 0x0178,
    0x0179, 0x017A, 0x017B, 0x017C, 0x017D, 0x017E, 0x017F, 0x0180, 0x0181, 0x0183, 0x0184, 0x0187, 0x0188, 0x0189,
    0x018A, 0x018B, 0x018C, 0x018D, 0x018E, 0x0190, 0x0191, 0x0192, 0x0193, 0x0194, 0x0195, 0x0196, 0x0197, 0x0198,
    0x0199, 0x019A, 0x019B, 0x019C, 0x019D, 0x019E, 0x019F, 0x01A0, 0x01A1, 0x01A2, 0x01A4, 0x01A5, 0x01A6, 0x01A7,
    0x01A8, 0x01A9, 0x01AA, 0x01AB, 0x01AC, 0x01AD, 0x01AF, 0x01B0, 0x01B1, 0x01B2, 0x01B3, 0x01B4, 0x01B5, 0x01B6,
    0x01B7, 0x01B8, 0x01B9, 0x01BA, 0x01BB, 0x01BC, 0x01BD, 0x01BE, 0x01BF, 0x01C1, 0x01C2, 0x01C4, 0x01C5, 0x01C6,
    0x01C7, 0x01C8, 0x01C9, 0x01CA, 0x01CB, 0x01CC, 0x01CD, 0x01CE, 0x01CF, 0x01D0, 0x01D1, 0x01D2, 0x01D3, 0x01D4,
    0x01D5, 0x01D6, 0x01D8, 0x01D9, 0x01DA, 0x01DB, 0x01DD, 0x01DE, 0x01DF, 0x01E0, 0x01E1, 0x01E3, 0x01E4, 0x01E5,
    0x01E6, 0x01EA, 0x01EC, 0x01EE, 0x01EF, 0x01F0, 0x01F1, 0x01F2, 0x01F3, 0x01F4, 0x01F5, 0x01F6, 0x01F7, 0x01F8,
    0x01F9, 0x01FA, 0x01FB, 0x01FC, 0x01FD, 0x01FE, 0x01FF, 0x0200, 0x0201, 0x0202, 0x0203, 0x0206, 0x0207, 0x0208,
    0x0209, 0x020A, 0x020B, 0x020C, 0x020E, 0x020F, 0x0210, 0x0211, 0x0212, 0x0213, 0x0214, 0x0215, 0x0216, 0x0217,
    0x0219, 0x021A, 0x021B, 0x021C, 0x021D, 0x021E, 0x021F, 0x0220, 0x0221, 0x0222, 0x0223, 0x0224, 0x0225, 0x0226,
    0x0227, 0x0228, 0x0229, 0x022A, 0x022B, 0x022C, 0x022D, 0x022E, 0x0230, 0x0231, 0x0232, 0x0234, 0x0235, 0x0236,
    0x0237, 0x0238, 0x0239, 0x023A, 0x023B, 0x023D, 0x023E, 0x023F, 0x0240, 0x0241, 0x0243, 0x0244, 0x0245, 0x0246,
    0x0247, 0x0248, 0x024A, 0x024B, 0x024C, 0x024D, 0x024E, 0x024F, 0x0250, 0x0251, 0x0252, 0x0253, 0x0254, 0x0255,
    0x0256, 0x0257, 0x0258, 0x0259, 0x025A, 0x025B, 0x025C, 0x025D, 0x025E, 0x025F, 0x0261, 0x0262, 0x0264, 0x0265,
    0x0266, 0x0267, 0x0268, 0x0269, 0x026A, 0x026B, 0x026C, 0x026D, 0x026E, 0x026F, 0x0270, 0x0271, 0x0272, 0x0273,
    0x0274, 0x0275, 0x0276, 0x0277, 0x0278, 0x0279, 0x027A, 0x027B, 0x027C, 0x027D, 0x027E, 0x027F, 0x0280, 0x0281,
    0x0282, 0x0283, 0x0284, 0x0285, 0x0286, 0x0287, 0x0289, 0x028A, 0x028B, 0x028C, 0x028E, 0x028F, 0x0290, 0x0291,
    0x0293, 0x0294, 0x0295, 0x0296, 0x0297, 0x0298, 0x0299, 0x029A, 0x029B, 0x029C, 0x029D, 0x029E, 0x029F, 0x02A0,
    0x02A2, 0x02A3, 0x02A4, 0x02A5, 0x02A6, 0x02A7, 0x02A8, 0x02A9, 0x02AA, 0x02AB, 0x02AD, 0x02AE, 0x02AF, 0x02B0,
    0x02B1, 0x02B2, 0x02B3, 0x02B4, 0x02B5, 0x02B6, 0x02B7, 0x02B8, 0x02B9, 0x02BA, 0x02BB, 0x02BC, 0x02BD, 0x02BE,
    0x02C0, 0x02C1, 0x02C2, 0x02C3, 0x02C4, 0x02C5, 0x02C6, 0x02C7, 0x02C8, 0x02C9, 0x02CA, 0x02CB, 0x02CD, 0x02CE,
    0x02CF, 0x02D0, 0x02D1, 0x02D2, 0x02D3, 0x02D4, 0x02D5, 0x02D6, 0x02D7, 0x02D8, 0x02D9, 0x02DA, 0x02DB, 0x02DC,
    0x02DD, 0x02DE, 0x02DF, 0x02E0, 0x02E1, 0x02E2, 0x02E3, 0x02E4, 0x02E5, 0x02E6, 0x02E7, 0x02E8, 0x02E9, 0x02EA,
    0x02EB, 0x02ED, 0x02EE, 0x02EF, 0x02F0, 0x02F1, 0x02F2, 0x02F3, 0x02F4, 0x02F5, 0x02F6, 0x02F8, 0x02F9, 0x02FB,
    0x02FC, 0x02FE, 0x02FF, 0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 0x0307, 0x0308, 0x0309, 0x030A,
    0x030B, 0x030C, 0x030D, 0x030F, 0x0310, 0x0311, 0x0312, 0x0313, 0x0314, 0x0315, 0x0316, 0x0317, 0x0318, 0x0319,
    0x031A, 0x031B, 0x031C, 0x031D, 0x031E, 0x031F, 0x0320, 0x0323, 0x0324, 0x0326, 0x0327, 0x0328, 0x0329, 0x032A,
    0x032B, 0x032C, 0x032D, 0x032E, 0x032F, 0x0330, 0x0331, 0x0333, 0x0334, 0x0335, 0x0336, 0x0337, 0x0338, 0x0339,
    0x033A, 0x033B, 0x033D, 0x033E, 0x033F, 0x0341, 0x0342, 0x0343, 0x0344, 0x0345, 0x0346, 0x0347, 0x0349, 0x034B,
    0x034C, 0x034D, 0x034F, 0x0350, 0x0351, 0x0352, 0x0353, 0x0354, 0x0355, 0x0356, 0x0357, 0x0358, 0x0359, 0x035A,
    0x035B, 0x035C, 0x035D, 0x035E, 0x035F, 0x0360, 0x0361, 0x0362, 0x0363, 0x0364, 0x0365, 0x0366, 0x0368, 0x0369,
    0x036A, 0x036B, 0x036C, 0x036D, 0x036E, 0x036F, 0x0370, 0x0372, 0x0373, 0x0374, 0x0375, 0x0376, 0x0377, 0x0378,
    0x0379, 0x037A, 0x037B, 0x037C, 0x037D, 0x037E, 0x037F, 0x0380, 0x0381, 0x0382, 0x0383, 0x0385, 0x0386, 0x0387,
    0x0388, 0x0389, 0x038A, 0x038B, 0x038C, 0x038D, 0x038E, 0x038F, 0x0390, 0x0391, 0x0392, 0x0393, 0x0395, 0x0396,
    0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F, 0x03A0, 0x03A1, 0x03A2, 0x03A3, 0x03A4,
    0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03AA, 0x03AB, 0x03AD, 0x03AE, 0x03AF, 0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4,
    0x03B5, 0x03B6, 0x03B7, 0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF, 0x03C0, 0x03C1, 0x03C2,
    0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7, 0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03CC, 0x03CD, 0x03CE, 0x03CF, 0x03D0,
    0x03D1, 0x03D2, 0x03D3, 0x03D4, 0x03D5, 0x03D6, 0x03D7, 0x03D8, 0x03D9, 0x03DA, 0x03DB, 0x03DC, 0x03DD, 0x03DE,
    0x03DF, 0x03E0, 0x03E1, 0x03E2, 0x03E3, 0x03E4, 0x03E5, 0x03E6, 0x03E7, 0x03E9, 0x03EA, 0x03EB, 0x03EC, 0x03ED,
    0x03EE, 0x03F0, 0x03F1, 0x03F2, 0x03F4, 0x03F5, 0x03F6, 0x03F7, 0x03F8, 0x03F9, 0x03FA, 0x03FB, 0x03FC, 0x03FD,
    0x03FE, 0x03FF, 0x0400, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0407, 0x0408, 0x040A, 0x040B, 0x040D, 0x040E,
    0x040F, 0x0410, 0x0411, 0x0412, 0x0413, 0x0416, 0x0417, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D,
    0x042E, 0x042F, 0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B,
    0x043D, 0x043E, 0x043F, 0x0440, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B,
    0x044C, 0x044D, 0x044E, 0x044F, 0x0450, 0x0451, 0x0452, 0x0453, 0x0454, 0x0456, 0x0457, 0x0458, 0x045A, 0x045C,
    0x045D, 0x045E, 0x045F, 0x0460, 0x0461, 0x0462, 0x0463, 0x0464, 0x0465, 0x0467, 0x0468, 0x0469, 0x046A, 0x046B,
    0x046C, 0x046D, 0x046E, 0x046F, 0x0470, 0x0471, 0x0472, 0x0473, 0x0474, 0x0475, 0x0477, 0x0478, 0x0479, 0x047A,
    0x047B, 0x047D, 0x047E, 0x047F, 0x0481, 0x0482, 0x0484, 0x0485, 0x0486, 0x0487, 0x0488, 0x0489, 0x048A, 0x048C,
    0x048D, 0x048E, 0x048F, 0x0490, 0x0491, 0x0492, 0x0493, 0x0494, 0x0496, 0x0497, 0x0498, 0x0499, 0x049A, 0x049B,
    0x049C, 0x049D, 0x049E, 0x049F, 0x04A2, 0x04A3, 0x04A4, 0x04A5, 0x04A6, 0x04A7, 0x04A8, 0x04AA, 0x04AB, 0x04AC,
    0x04AD, 0x04AE, 0x04AF, 0x04B1, 0x04B2, 0x04B3, 0x04B4, 0x04B5, 0x04B6, 0x04B7, 0x04B8, 0x04B9, 0x04BA, 0x04BB,
    0x04BC, 0x04BD, 0x04BE, 0x04BF, 0x04C0, 0x04C1, 0x04C2, 0x04C3, 0x04C4, 0x04C5, 0x04C6, 0x04C7, 0x04C8, 0x04C9,
    0x04CA, 0x04CB, 0x04CD, 0x04CE, 0x04CF, 0x04D0, 0x04D1, 0x04D2, 0x04D3, 0x04D4, 0x04D5, 0x04D6, 0x04D7, 0x04D8,
    0x04D9, 0x04DA, 0x04DB, 0x04DC, 0x04DD, 0x04DE, 0x04DF, 0x04E0, 0x04E1, 0x04E3, 0x04E4, 0x04E5, 0x04E6, 0x04E8,
    0x04E9, 0x04EA, 0x04EB, 0x04EC, 0x04EE, 0x04EF, 0x04F0, 0x04F1, 0x04F2, 0x04F3, 0x04F4, 0x04F5, 0x04F6, 0x04F7,
    0x04F8, 0x04F9, 0x04FA, 0x04FB, 0x04FC, 0x04FD, 0x04FE, 0x04FF, 0x0500, 0x0501, 0x0502, 0x0503, 0x0504, 0x0505,
    0x0506, 0x0508, 0x0509, 0x050A, 0x050B, 0x050C, 0x050D, 0x050E, 0x050F, 0x0510, 0x0511, 0x0512, 0x0513, 0x0514,
    0x0515, 0x0516, 0x0517, 0x0518, 0x0519, 0x051A, 0x051C, 0x051D, 0x051E, 0x051F, 0x0520, 0x0521, 0x0522, 0x0523,
    0x0524, 0x0525, 0x0526, 0x0527, 0x0528, 0x0529, 0x052A, 0x052B, 0x052C, 0x052D, 0x052E, 0x052F, 0x0530, 0x0531,
    0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B, 0x053C, 0x053D, 0x053E, 0x053F,
    0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0546, 0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E,
    0x054F, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0x0557, 0x0558, 0x0559, 0x055A, 0x055B, 0x055C, 0x055D,
    0x055E, 0x055F, 0x0560, 0x0561, 0x0562, 0x0563, 0x0564, 0x0566, 0x0567, 0x0568, 0x056A, 0x056B, 0x056C, 0x056D,
    0x056E, 0x056F, 0x0570, 0x0571, 0x0572, 0x0573, 0x0574, 0x0575, 0x0576, 0x0577, 0x0578, 0x057A, 0x057B, 0x057C,
    0x057D, 0x057E, 0x057F, 0x0581, 0x0582, 0x0583, 0x0584, 0x0585, 0x0586, 0x0587, 0x0588, 0x0589, 0x058A, 0x058B,
    0x058C, 0x058D, 0x058E, 0x058F, 0x0590, 0x0591, 0x0592, 0x0593, 0x0594, 0x0595, 0x0596, 0x0597, 0x0598, 0x0599,
    0x059A, 0x059B, 0x059C, 0x059D, 0x059E, 0x059F, 0x05A0, 0x05A1, 0x05A2, 0x05A3, 0x05A4, 0x05A5, 0x05A6, 0x05A7,
    0x05A8, 0x05A9, 0x05AA, 0x05AB, 0x05AC, 0x05AD, 0x05AE, 0x05AF, 0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5,
    0x05B6, 0x05B7, 0x05B8, 0x05B9, 0x05BA, 0x05BB, 0x05BC, 0x05BD, 0x05BF, 0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05C4,
    0x05C5, 0x05C7, 0x05C8, 0x05C9, 0x05CA, 0x05CB, 0x05CC, 0x05CD, 0x05CE, 0x05CF, 0x05D0, 0x05D1, 0x05D2, 0x05D3,
    0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF, 0x05E0, 0x05E1,
    0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x05EB, 0x05EC, 0x05ED, 0x05EF, 0x05F0,
    0x05F1, 0x05F2, 0x05F3, 0x05F4, 0x05F5, 0x05F6, 0x05F7, 0x05F8, 0x05F9, 0x05FA, 0x05FB, 0x05FC, 0x05FD, 0x05FE,
    0x05FF, 0x0600, 0x0601, 0x0602, 0x0603, 0x0605, 0x0606, 0x0607, 0x0608, 0x0609, 0x060A, 0x060B, 0x060C, 0x060D,
    0x060E, 0x060F, 0x0610, 0x0611, 0x0612, 0x0613, 0x0614, 0x0615, 0x0616, 0x0618, 0x0619, 0x061A, 0x061B, 0x061C,
    0x061D, 0x061E, 0x061F, 0x0620, 0x0621, 0x0622, 0x0624, 0x0625, 0x0626, 0x0627, 0x0628, 0x0629, 0x062B, 0x062C,
    0x062D, 0x062E, 0x062F, 0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 0x0638, 0x0639, 0x063A,
    0x063B, 0x063C, 0x063D, 0x063E, 0x063F, 0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, 0x0648,
    0x0649, 0x064A, 0x064B, 0x064C, 0x064D, 0x064E, 0x064F, 0x0650, 0x0651, 0x0652, 0x0653, 0x0654, 0x0655, 0x0656,
    0x0657, 0x0658, 0x0659, 0x065A, 0x065B, 0x065C, 0x065D, 0x065E, 0x065F, 0x0660, 0x0661, 0x0662, 0x0663, 0x0664,
    0x0665, 0x0666, 0x0668, 0x0669, 0x066A, 0x066B, 0x066C, 0x066D, 0x066E, 0x066F, 0x0670, 0x0671, 0x0672, 0x0673,
    0x0674, 0x0676, 0x0677, 0x0678, 0x0679, 0x067A, 0x067B, 0x067C, 0x067D, 0x067F, 0x0680, 0x0681, 0x0682, 0x0683,
    0x0684, 0x0685, 0x0686, 0x0687, 0x0688, 0x0689, 0x068A, 0x068B, 0x068E, 0x068F, 0x0690, 0x0691, 0x0692, 0x0693,
    0x0694, 0x0695, 0x0696, 0x0697, 0x0698, 0x0699, 0x069A, 0x069B, 0x069C, 0x069E, 0x069F, 0x06A0, 0x06A1, 0x06A2,
    0x06A3, 0x06A4, 0x06A5, 0x06A6, 0x06A7, 0x06A8, 0x06A9, 0x06AA, 0x06AB, 0x06AC, 0x06AD, 0x06AE, 0x06AF, 0x06B0,
    0x06B1, 0x06B2, 0x06B4, 0x06B5, 0x06B6, 0x06B7, 0x06B8, 0x06B9, 0x06BA, 0x06BB, 0x06BC, 0x06BD, 0x06BE, 0x06C0,
    0x06C1, 0x06C2, 0x06C3, 0x06C4, 0x06C5, 0x06C6, 0x06C8, 0x06C9, 0x06CA, 0x06CB, 0x06CC, 0x06CD, 0x06CE, 0x06D0,
    0x06D1, 0x06D2, 0x06D5, 0x06D6, 0x06D7, 0x06D8, 0x06D9, 0x06DA, 0x06DB, 0x06DC, 0x06DD, 0x06DE, 0x06DF, 0x06E0,
    0x06E1, 0x06E2, 0x06E3, 0x06E4, 0x06E5, 0x06E6, 0x06E7, 0x06E8, 0x06E9, 0x06EA, 0x06EB, 0x06ED, 0x06EF, 0x06F0,
    0x06F1, 0x06F2, 0x06F3, 0x06F4, 0x06F5, 0x06F6, 0x06F7, 0x06F8, 0x06F9, 0x06FA, 0x06FB, 0x06FC, 0x06FD, 0x06FE,
    0x06FF, 0x0700, 0x0701, 0x0702, 0x0703, 0x0704, 0x0705, 0x0706, 0x0707, 0x0708, 0x0709, 0x070B, 0x070C, 0x070D,
    0x070E, 0x070F, 0x0710, 0x0711, 0x0712, 0x0713, 0x0714, 0x0715, 0x0716, 0x0718, 0x0719, 0x071A, 0x071B, 0x071C,
    0x071D, 0x071E, 0x071F, 0x0720, 0x0721, 0x0722, 0x0723, 0x0724, 0x0725, 0x0726, 0x0728, 0x0729, 0x072A, 0x072B,
    0x072C, 0x072D, 0x072E, 0x072F, 0x0730, 0x0731, 0x0732, 0x0733, 0x0734, 0x0735, 0x0736, 0x0737, 0x0738, 0x0739,
    0x073A, 0x073B, 0x073D, 0x073E, 0x073F, 0x0741, 0x0742, 0x0743, 0x0744, 0x0745, 0x0746, 0x0747, 0x0748, 0x0749,
    0x074A, 0x074B, 0x074D, 0x074E, 0x074F, 0x0750, 0x0751, 0x0752, 0x0753, 0x0754, 0x0755, 0x0756, 0x0757, 0x0758,
    0x0759, 0x075A, 0x075B, 0x075C, 0x075D, 0x075E, 0x075F, 0x0760, 0x0761, 0x0762, 0x0763, 0x0764, 0x0765, 0x0766,
    0x0767, 0x0768, 0x0769, 0x076A, 0x076B, 0x076C, 0x076D, 0x076E, 0x076F, 0x0770, 0x0771, 0x0772, 0x0774, 0x0775,
    0x0776, 0x0777, 0x0778, 0x0779, 0x077A, 0x077C, 0x077D, 0x077E, 0x077F, 0x0780, 0x0781, 0x0782, 0x0783, 0x0784,
    0x0785, 0x0788, 0x0789, 0x078A, 0x078B, 0x078C, 0x078D, 0x078E, 0x078F, 0x0790, 0x0791, 0x0792, 0x0793, 0x0794,
    0x0795, 0x0796, 0x0797, 0x0798, 0x0799, 0x079A, 0x079B, 0x079C, 0x079D, 0x079E, 0x079F, 0x07A0, 0x07A2, 0x07A4,
    0x07A5, 0x07A6, 0x07A7, 0x07A8, 0x07A9, 0x07AA, 0x07AB, 0x07AC, 0x07AD, 0x07AE, 0x07AF, 0x07B0, 0x07B1, 0x07B2,
    0x07B3, 0x07B4, 0x07B5, 0x07B6, 0x07B7, 0x07B8, 0x07BA, 0x07BB, 0x07BD, 0x07BE, 0x07BF, 0x07C0, 0x07C2, 0x07C3,
    0x07C4, 0x07C5, 0x07C6, 0x07C7, 0x07C8, 0x07C9, 0x07CB, 0x07CC, 0x07CD, 0x07CE, 0x07CF, 0x07D0, 0x07D1, 0x07D2,
    0x07D3, 0x07D4, 0x07D5, 0x07D6, 0x07D7, 0x07D8, 0x07D9, 0x07DA, 0x07DB, 0x07DC, 0x07DD, 0x07DE, 0x07DF, 0x07E0,
    0x07E2, 0x07E3, 0x07E4, 0x07E5, 0x07E6, 0x07E7, 0x07E8, 0x07E9, 0x07EA, 0x07EB, 0x07EC, 0x07ED, 0x07EE, 0x07EF,
    0x07F0, 0x07F1, 0x07F2, 0x07F3, 0x07F4, 0x07F5, 0x07F6, 0x07F7, 0x07F8, 0x07FA, 0x07FB, 0x07FC, 0x07FD, 0x07FE,
    0x07FF, 0x0800, 0x0801, 0x0802, 0x0803, 0x0804, 0x0805, 0x0806, 0x0807, 0x0808, 0x0809, 0x080A, 0x080B, 0x080C,
    0x080D, 0x080E, 0x080F, 0x0810, 0x0811, 0x0812, 0x0813, 0x0814, 0x0815, 0x0816, 0x0817, 0x0818, 0x0819, 0x081A,
    0x081B, 0x081C, 0x081D, 0x081E, 0x081F, 0x0820, 0x0821, 0x0822, 0x0824, 0x0825, 0x0826, 0x0827, 0x0828, 0x0829,
    0x082A, 0x082B, 0x082C, 0x082D, 0x082E, 0x082F, 0x0831, 0x0832, 0x0833, 0x0834, 0x0835, 0x0836, 0x0837, 0x0838,
    0x0839, 0x083A, 0x083B, 0x083C, 0x083D, 0x083E, 0x083F, 0x0840, 0x0841, 0x0842, 0x0843, 0x0844, 0x0845, 0x0846,
    0x0847, 0x0848, 0x0849, 0x084A, 0x084B, 0x084C, 0x084D, 0x084E, 0x084F, 0x0850, 0x0851, 0x0852, 0x0853, 0x0854,
    0x0855, 0x0856, 0x0857, 0x0858, 0x0859, 0x085A, 0x085B, 0x085C, 0x085D, 0x085E, 0x0860, 0x0861, 0x0862, 0x0863,
    0x0864, 0x0865, 0x0866, 0x0867, 0x0868, 0x0869, 0x086A, 0x086B, 0x086C, 0x086D, 0x086E, 0x086F, 0x0870, 0x0871,
    0x0872, 0x0873, 0x0874, 0x0875, 0x0876, 0x0877, 0x0878, 0x0879, 0x087A, 0x087B, 0x087C, 0x087D, 0x087E, 0x087F,
    0x0880, 0x0881, 0x0882, 0x0883, 0x0884, 0x0886, 0x0887, 0x0888, 0x0889, 0x088A, 0x088B, 0x088C, 0x088D, 0x088F,
    0x0890, 0x0891, 0x0892, 0x0893, 0x0894, 0x0895, 0x0896, 0x0897, 0x0899, 0x089A, 0x089B, 0x089C, 0x089D, 0x089E,
    0x089F, 0x08A0, 0x08A1, 0x08A2, 0x08A3, 0x08A4, 0x08A6, 0x08A7, 0x08A8, 0x08A9, 0x08AA, 0x08AB, 0x08AC, 0x08AD,
    0x08AE, 0x08AF, 0x08B0, 0x08B1, 0x08B2, 0x08B3, 0x08B4, 0x08B6, 0x08B7, 0x08B8, 0x08B9, 0x08BA, 0x08BB, 0x08BC,
    0x08BD, 0x08BE, 0x08BF, 0x08C0, 0x08C1, 0x08C2, 0x08C3, 0x08C5, 0x08C7, 0x08C8, 0x08C9, 0x08CA, 0x08CB, 0x08CC,
    0x08CD, 0x08CE, 0x08CF, 0x08D0, 0x08D1, 0x08D2, 0x08D3, 0x08D4, 0x08D5, 0x08D7, 0x08D8, 0x08D9, 0x08DA, 0x08DB,
    0x08DC, 0x08DD, 0x08DE, 0x08DF, 0x08E0, 0x08E1, 0x08E2, 0x08E3, 0x08E4, 0x08E5, 0x08E6, 0x08E7, 0x08E8, 0x08E9,
    0x08EA, 0x08EB, 0x08EC, 0x08ED, 0x08EE, 0x08EF, 0x08F0, 0x08F1, 0x08F2, 0x08F3, 0x08F4, 0x08F5, 0x08F6, 0x08F7,
    0x08F8, 0x08F9, 0x08FB, 0x08FC, 0x08FD, 0x08FE, 0x08FF, 0x0900, 0x0901, 0x0902, 0x0903, 0x0904, 0x0905, 0x0906,
    0x0907, 0x0908, 0x0909, 0x090A, 0x090B, 0x090C, 0x090E, 0x090F, 0x0910, 0x0911, 0x0912, 0x0913, 0x0914, 0x0915,
    0x0916, 0x0917, 0x0919, 0x091A, 0x091B, 0x091C, 0x091D, 0x091E, 0x091F, 0x0921, 0x0922, 0x0923, 0x0925, 0x0926,
    0x0927, 0x0928, 0x0929, 0x092A, 0x092B, 0x092C, 0x092D, 0x092E, 0x092F, 0x0930, 0x0931, 0x0933, 0x0934, 0x0935,
    0x0936, 0x0937, 0x0938, 0x0939, 0x093A, 0x093B, 0x093C, 0x093D, 0x093E, 0x093F, 0x0940, 0x0941, 0x0942, 0x0944,
    0x0945, 0x0946, 0x0947, 0x0948, 0x0949, 0x094A, 0x094B, 0x094C, 0x094D, 0x094E, 0x094F, 0x0950, 0x0951, 0x0952,
    0x0953, 0x0954, 0x0955, 0x0956, 0x0957, 0x0958, 0x0959, 0x095B, 0x095C, 0x095D, 0x095E, 0x095F, 0x0960, 0x0961,
    0x0962, 0x0963, 0x0964, 0x0965, 0x0966, 0x0967, 0x0968, 0x0969, 0x096A, 0x096B, 0x096D, 0x096E, 0x0970, 0x0971,
    0x0973, 0x0974, 0x0975, 0x0976, 0x0977, 0x0978, 0x0979, 0x097A, 0x097B, 0x097C, 0x097D, 0x097E, 0x097F, 0x0980,
    0x0981, 0x0982, 0x0983, 0x0984, 0x0986, 0x0987, 0x0988, 0x0989, 0x098A, 0x098B, 0x098C, 0x098E, 0x098F, 0x0990,
    0x0991, 0x0992, 0x0993, 0x0994, 0x0995, 0x0996, 0x0997, 0x0998, 0x0999, 0x099A, 0x099B, 0x099C, 0x099D, 0x099E,
    0x099F, 0x09A0, 0x09A1, 0x09A2, 0x09A3, 0x09A4, 0x09A5, 0x09A6, 0x09A7, 0x09A8, 0x09A9, 0x09AA, 0x09AB, 0x09AC,
    0x09AD, 0x09AE, 0x09AF, 0x09B0, 0x09B1, 0x09B2, 0x09B3, 0x09B4, 0x09B5, 0x09B6, 0x09B7, 0x09B8, 0x09B9, 0x09BA,
    0x09BB, 0x09BC, 0x09BD, 0x09BE, 0x09BF, 0x09C0, 0x09C1, 0x09C2, 0x09C3, 0x09C4, 0x09C5, 0x09C6, 0x09C7, 0x09C8,
    0x09C9, 0x09CA, 0x09CB, 0x09CC, 0x09CE, 0x09CF, 0x09D0, 0x09D1, 0x09D2, 0x09D4, 0x09D5, 0x09D6, 0x09D7, 0x09D8,
    0x09D9, 0x09DA, 0x09DB, 0x09DD, 0x09DE, 0x09DF, 0x09E1, 0x09E3, 0x09E4, 0x09E5, 0x09E6, 0x09E7, 0x09E8, 0x09E9,
    0x09EA, 0x09EB, 0x09EC, 0x09ED, 0x09EE, 0x09EF, 0x09F0, 0x09F1, 0x09F2, 0x09F3, 0x09F4, 0x09F5, 0x09F6, 0x09F7,
    0x09F8, 0x09F9, 0x09FA, 0x09FB, 0x09FC, 0x09FE, 0x09FF, 0x0A00, 0x0A01, 0x0A02, 0x0A03, 0x0A04, 0x0A05, 0x0A06,
    0x0A07, 0x0A08, 0x0A0A, 0x0A0C, 0x0A0D, 0x0A0E, 0x0A0F, 0x0A10, 0x0A11, 0x0A12, 0x0A13, 0x0A14, 0x0A15, 0x0A17,
    0x0A18, 0x0A19, 0x0A1A, 0x0A1B, 0x0A1C, 0x0A1D, 0x0A1E, 0x0A1F, 0x0A20, 0x0A21, 0x0A22, 0x0A23, 0x0A24, 0x0A25,
    0x0A26, 0x0A28, 0x0A29, 0x0A2A, 0x0A2B, 0x0A2C, 0x0A2D, 0x0A2F, 0x0A30, 0x0A31, 0x0A32, 0x0A33, 0x0A34, 0x0A35,
    0x0A36, 0x0A37, 0x0A38, 0x0A39, 0x0A3B, 0x0A3C, 0x0A3D, 0x0A3F, 0x0A41, 0x0A42, 0x0A43, 0x0A44, 0x0A45, 0x0A46,
    0x0A47, 0x0A48, 0x0A4A, 0x0A4B, 0x0A4C, 0x0A4D, 0x0A4E, 0x0A4F, 0x0A50, 0x0A51, 0x0A52, 0x0A53, 0x0A54, 0x0A55,
    0x0A56, 0x0A57, 0x0A58, 0x0A59, 0x0A5A, 0x0A5B, 0x0A5C, 0x0A5D, 0x0A5F, 0x0A60, 0x0A61, 0x0A62, 0x0A64, 0x0A65,
    0x0A67, 0x0A68, 0x0A69, 0x0A6A, 0x0A6B, 0x0A6C, 0x0A6E, 0x0A6F, 0x0A70, 0x0A71, 0x0A72, 0x0A73, 0x0A74, 0x0A75,
    0x0A76, 0x0A77, 0x0A78, 0x0A79, 0x0A7A, 0x0A7B, 0x0A7C, 0x0A7D, 0x0A7E, 0x0A7F, 0x0A80, 0x0A81, 0x0A82, 0x0A83,
    0x0A84, 0x0A85, 0x0A86, 0x0A87, 0x0A88, 0x0A89, 0x0A8A, 0x0A8B, 0x0A8C, 0x0A8D, 0x0A8E, 0x0A8F, 0x0A90, 0x0A91,
    0x0A92, 0x0A93, 0x0A94, 0x0A95, 0x0A98, 0x0A99, 0x0A9A, 0x0A9B, 0x0A9C, 0x0A9D, 0x0A9F, 0x0AA0, 0x0AA1, 0x0AA2,
    0x0AA3, 0x0AA4, 0x0AA5, 0x0AA6, 0x0AA7, 0x0AA8, 0x0AA9, 0x0AAA, 0x0AAB, 0x0AAC, 0x0AAD, 0x0AAE, 0x0AAF, 0x0AB0,
    0x0AB1, 0x0AB2, 0x0AB3, 0x0AB4, 0x0AB5, 0x0AB6, 0x0AB7, 0x0AB8, 0x0AB9, 0x0ABA, 0x0ABB, 0x0ABC, 0x0ABD, 0x0ABE,
    0x0ABF, 0x0AC0, 0x0AC1, 0x0AC2, 0x0AC3, 0x0AC4, 0x0AC5, 0x0AC6, 0x0AC7, 0x0AC8, 0x0AC9, 0x0ACA, 0x0ACB, 0x0ACC,
    0x0ACD, 0x0ACE, 0x0ACF, 0x0AD0, 0x0AD2, 0x0AD3, 0x0AD4, 0x0AD5, 0x0AD6, 0x0AD7, 0x0AD8, 0x0ADA, 0x0ADB, 0x0ADC,
    0x0ADD, 0x0ADE, 0x0ADF, 0x0AE3, 0x0AE4, 0x0AE5, 0x0AE6, 0x0AE7, 0x0AE8, 0x0AE9, 0x0AEA, 0x0AEB, 0x0AEC, 0x0AED,
    0x0AEE, 0x0AEF, 0x0AF0, 0x0AF1, 0x0AF3, 0x0AF4, 0x0AF5, 0x0AF6, 0x0AF7, 0x0AF8, 0x0AF9, 0x0AFA, 0x0AFB, 0x0AFC,
    0x0AFD, 0x0AFE, 0x0AFF, 0x0B00, 0x0B01, 0x0B02, 0x0B03, 0x0B05, 0x0B06, 0x0B07, 0x0B08, 0x0B0A, 0x0B0B, 0x0B0C,
    0x0B0D, 0x0B0E, 0x0B0F, 0x0B10, 0x0B11, 0x0B12, 0x0B13, 0x0B14, 0x0B15, 0x0B16, 0x0B17, 0x0B18, 0x0B19, 0x0B1A,
    0x0B1C, 0x0B1D, 0x0B1E, 0x0B1F, 0x0B20, 0x0B22, 0x0B23, 0x0B24, 0x0B25, 0x0B26, 0x0B27, 0x0B29, 0x0B2B, 0x0B2C,
    0x0B2D, 0x0B2E, 0x0B2F, 0x0B30, 0x0B31, 0x0B32, 0x0B33, 0x0B34, 0x0B35, 0x0B36, 0x0B37, 0x0B38, 0x0B39, 0x0B3A,
    0x0B3B, 0x0B3C, 0x0B3D, 0x0B3E, 0x0B3F, 0x0B40, 0x0B41, 0x0B42, 0x0B43, 0x0B44, 0x0B45, 0x0B47, 0x0B48, 0x0B49,
    0x0B4A, 0x0B4B, 0x0B4C, 0x0B4D, 0x0B4E, 0x0B4F, 0x0B50, 0x0B51, 0x0B52, 0x0B53, 0x0B54, 0x0B55, 0x0B56, 0x0B57,
    0x0B58, 0x0B5B, 0x0B5D, 0x0B5E, 0x0B5F, 0x0B60, 0x0B61, 0x0B62, 0x0B63, 0x0B64, 0x0B65, 0x0B66, 0x0B68, 0x0B69,
    0x0B6A, 0x0B6B, 0x0B6C, 0x0B6D, 0x0B6E, 0x0B70, 0x0B71, 0x0B72, 0x0B73, 0x0B74, 0x0B75, 0x0B76, 0x0B77, 0x0B78,
    0x0B79, 0x0B7A, 0x0B7B, 0x0B7C, 0x0B7D, 0x0B7E, 0x0B7F, 0x0B80, 0x0B81, 0x0B82, 0x0B83, 0x0B84, 0x0B85, 0x0B86,
    0x0B87, 0x0B88, 0x0B89, 0x0B8A, 0x0B8B, 0x0B8C, 0x0B8D, 0x0B8E, 0x0B8F, 0x0B91, 0x0B93, 0x0B94, 0x0B96, 0x0B97,
    0x0B98, 0x0B99, 0x0B9A, 0x0B9C, 0x0B9D, 0x0B9E, 0x0B9F, 0x0BA0, 0x0BA1, 0x0BA2, 0x0BA3, 0x0BA4, 0x0BA5, 0x0BA6,
    0x0BA7, 0x0BA8, 0x0BA9, 0x0BAA, 0x0BAB, 0x0BAC, 0x0BAD, 0x0BAE, 0x0BAF, 0x0BB0, 0x0BB1, 0x0BB2, 0x0BB3, 0x0BB4,
    0x0BB5, 0x0BB6, 0x0BB7, 0x0BB8, 0x0BB9, 0x0BBA, 0x0BBB, 0x0BBC, 0x0BBE, 0x0BBF, 0x0BC0, 0x0BC1, 0x0BC2, 0x0BC3,
    0x0BC4, 0x0BC5, 0x0BC6, 0x0BC7, 0x0BC8, 0x0BC9, 0x0BCA, 0x0BCB, 0x0BCC, 0x0BCD, 0x0BCE, 0x0BCF, 0x0BD0, 0x0BD1,
    0x0BD2, 0x0BD3, 0x0BD4, 0x0BD5, 0x0BD6, 0x0BD8, 0x0BDA, 0x0BDB, 0x0BDC, 0x0BDD, 0x0BDE, 0x0BDF, 0x0BE0, 0x0BE2,
    0x0BE3, 0x0BE4, 0x0BE5, 0x0BE6, 0x0BE7, 0x0BE8, 0x0BE9, 0x0BEA, 0x0BEB, 0x0BED, 0x0BEE, 0x0BEF, 0x0BF0, 0x0BF1,
    0x0BF2, 0x0BF3, 0x0BF5, 0x0BF6, 0x0BF7, 0x0BF8, 0x0BFA, 0x0BFB, 0x0BFC, 0x0BFD, 0x0BFE, 0x0BFF, 0x0C00, 0x0C01,
    0x0C02, 0x0C03, 0x0C04, 0x0C05, 0x0C06, 0x0C07, 0x0C08, 0x0C09, 0x0C0A, 0x0C0B, 0x0C0C, 0x0C0D, 0x0C0E, 0x0C0F,
    0x0C10, 0x0C11, 0x0C12, 0x0C13, 0x0C14, 0x0C15, 0x0C16, 0x0C17, 0x0C18, 0x0C19, 0x0C1A, 0x0C1B, 0x0C1C, 0x0C1D,
    0x0C1E, 0x0C1F, 0x0C20, 0x0C21, 0x0C22, 0x0C23, 0x0C24, 0x0C25, 0x0C26, 0x0C27, 0x0C28, 0x0C29, 0x0C2A, 0x0C2B,
    0x0C2C, 0x0C2D, 0x0C2E, 0x0C2F, 0x0C30, 0x0C31, 0x0C32, 0x0C33, 0x0C34, 0x0C35, 0x0C37, 0x0C38, 0x0C39, 0x0C3B,
    0x0C3C, 0x0C3D, 0x0C3E, 0x0C3F, 0x0C40, 0x0C41, 0x0C42, 0x0C43, 0x0C44, 0x0C45, 0x0C47, 0x0C48, 0x0C49, 0x0C4A,
    0x0C4B, 0x0C4C, 0x0C4D, 0x0C4E, 0x0C4F, 0x0C50, 0x0C51, 0x0C52, 0x0C53, 0x0C54, 0x0C55, 0x0C56, 0x0C57, 0x0C58,
    0x0C59, 0x0C5A, 0x0C5B, 0x0C5C, 0x0C5D, 0x0C5E, 0x0C5F, 0x0C60, 0x0C61, 0x0C62, 0x0C63, 0x0C64, 0x0C65, 0x0C67,
    0x0C68, 0x0C6A, 0x0C6B, 0x0C6C, 0x0C6D, 0x0C6E, 0x0C6F, 0x0C70, 0x0C71, 0x0C72, 0x0C73, 0x0C74, 0x0C75, 0x0C77,
    0x0C78, 0x0C79, 0x0C7A, 0x0C7B, 0x0C7C, 0x0C7D, 0x0C7F, 0x0C80, 0x0C81, 0x0C82, 0x0C83, 0x0C84, 0x0C85, 0x0C86,
    0x0C87, 0x0C88, 0x0C89, 0x0C8A, 0x0C8B, 0x0C8C, 0x0C8D, 0x0C8E, 0x0C8F, 0x0C90, 0x0C91, 0x0C92, 0x0C93, 0x0C94,
    0x0C95, 0x0C96, 0x0C97, 0x0C99, 0x0C9A, 0x0C9B, 0x0C9C, 0x0C9D, 0x0C9F, 0x0CA0, 0x0CA1, 0x0CA2, 0x0CA3, 0x0CA4,
    0x0CA5, 0x0CA6, 0x0CA7, 0x0CA8, 0x0CA9, 0x0CAA, 0x0CAB, 0x0CAC, 0x0CAD, 0x0CAE, 0x0CAF, 0x0CB0, 0x0CB1, 0x0CB2,
    0x0CB3, 0x0CB4, 0x0CB5, 0x0CB6, 0x0CB7, 0x0CB9, 0x0CBA, 0x0CBB, 0x0CBD, 0x0CBF, 0x0CC0, 0x0CC1, 0x0CC2, 0x0CC3,
    0x0CC4, 0x0CC5, 0x0CC6, 0x0CC7, 0x0CC8, 0x0CC9, 0x0CCA, 0x0CCB, 0x0CCC, 0x0CCD, 0x0CCE, 0x0CCF, 0x0CD0, 0x0CD1,
    0x0CD2, 0x0CD3, 0x0CD4, 0x0CD5, 0x0CD6, 0x0CD7, 0x0CD8, 0x0CD9, 0x0CDA, 0x0CDB, 0x0CDC, 0x0CDD, 0x0CDF, 0x0CE0,
    0x0CE1, 0x0CE2, 0x0CE3, 0x0CE4, 0x0CE5, 0x0CE6, 0x0CE7, 0x0CE8, 0x0CE9, 0x0CEA, 0x0CEB, 0x0CEC, 0x0CED, 0x0CEE,
    0x0CEF, 0x0CF0, 0x0CF1, 0x0CF3, 0x0CF4, 0x0CF5, 0x0CF6, 0x0CF7, 0x0CF8, 0x0CF9, 0x0CFA, 0x0CFB, 0x0CFC, 0x0CFE,
    0x0D01, 0x0D02, 0x0D03, 0x0D04, 0x0D05, 0x0D06, 0x0D07, 0x0D08, 0x0D09, 0x0D0A, 0x0D0B, 0x0D0C, 0x0D0D, 0x0D0E,
    0x0D0F, 0x0D10, 0x0D12, 0x0D13, 0x0D14, 0x0D15, 0x0D16, 0x0D17, 0x0D18, 0x0D19, 0x0D1A, 0x0D1B, 0x0D1C, 0x0D1D,
    0x0D1E, 0x0D1F, 0x0D20, 0x0D21, 0x0D22, 0x0D23, 0x0D24, 0x0D25, 0x0D26, 0x0D27, 0x0D28, 0x0D29, 0x0D2A, 0x0D2B,
    0x0D2C, 0x0D2D, 0x0D2E, 0x0D2F, 0x0D30, 0x0D31, 0x0D32, 0x0D33, 0x0D34, 0x0D35, 0x0D36, 0x0D37, 0x0D38, 0x0D3A,
    0x0D3B, 0x0D3C, 0x0D3D, 0x0D3E, 0x0D3F, 0x0D40, 0x0D41, 0x0D42, 0x0D43, 0x0D44, 0x0D45, 0x0D46, 0x0D47, 0x0D48,
    0x0D49, 0x0D4A, 0x0D4B, 0x0D4D, 0x0D4E, 0x0D4F, 0x0D50, 0x0D51, 0x0D52, 0x0D53, 0x0D54, 0x0D55, 0x0D56, 0x0D57,
    0x0D58, 0x0D59, 0x0D5A, 0x0D5B, 0x0D5D, 0x0D5E, 0x0D5F, 0x0D60, 0x0D61, 0x0D62, 0x0D63, 0x0D64, 0x0D65, 0x0D66,
    0x0D67, 0x0D68, 0x0D69, 0x0D6A, 0x0D6B, 0x0D6D, 0x0D6E, 0x0D6F, 0x0D70, 0x0D71, 0x0D72, 0x0D73, 0x0D74, 0x0D75,
    0x0D76, 0x0D77, 0x0D78, 0x0D79, 0x0D7A, 0x0D7B, 0x0D7C, 0x0D7D, 0x0D7E, 0x0D7F, 0x0D80, 0x0D81, 0x0D82, 0x0D83,
    0x0D84, 0x0D85, 0x0D86, 0x0D87, 0x0D89, 0x0D8A, 0x0D8B, 0x0D8C, 0x0D8D, 0x0D8E, 0x0D8F, 0x0D90, 0x0D91, 0x0D92,
    0x0D93, 0x0D95, 0x0D96, 0x0D97, 0x0D98, 0x0D99, 0x0D9A, 0x0D9B, 0x0D9C, 0x0D9D, 0x0D9E, 0x0D9F, 0x0DA0, 0x0DA1,
    0x0DA2, 0x0DA3, 0x0DA4, 0x0DA5, 0x0DA6, 0x0DA7, 0x0DA8, 0x0DA9, 0x0DAA, 0x0DAB, 0x0DAC, 0x0DAE, 0x0DAF, 0x0DB0,
    0x0DB1, 0x0DB2, 0x0DB3, 0x0DB4, 0x0DB5, 0x0DB6, 0x0DB7, 0x0DB8, 0x0DB9, 0x0DBA, 0x0DBB, 0x0DBC, 0x0DBD, 0x0DBE,
    0x0DBF, 0x0DC0, 0x0DC1, 0x0DC2, 0x0DC3, 0x0DC4, 0x0DC5, 0x0DC6, 0x0DC7, 0x0DC8, 0x0DC9, 0x0DCA, 0x0DCB, 0x0DCC,
    0x0DCD, 0x0DCE, 0x0DCF, 0x0DD0, 0x0DD1, 0x0DD2, 0x0DD3, 0x0DD4, 0x0DD5, 0x0DD6, 0x0DD7, 0x0DD9, 0x0DDA, 0x0DDB,
    0x0DDC, 0x0DDD, 0x0DDE, 0x0DDF, 0x0DE0, 0x0DE1, 0x0DE2, 0x0DE3, 0x0DE4, 0x0DE5, 0x0DE6, 0x0DE7, 0x0DE8, 0x0DE9,
    0x0DEA, 0x0DEB, 0x0DEC, 0x0DED, 0x0DEE, 0x0DEF, 0x0DF0, 0x0DF1, 0x0DF2, 0x0DF3, 0x0DF4, 0x0DF5, 0x0DF6, 0x0DF7,
    0x0DF8, 0x0DF9, 0x0DFA, 0x0DFB, 0x0DFC, 0x0DFD, 0x0DFE, 0x0DFF, 0x0E00, 0x0E01, 0x0E02, 0x0E03, 0x0E04, 0x0E05,
    0x0E06, 0x0E07, 0x0E08, 0x0E09, 0x0E0B, 0x0E0C, 0x0E0D, 0x0E0F, 0x0E10, 0x0E11, 0x0E12, 0x0E13, 0x0E14, 0x0E15,
    0x0E16, 0x0E17, 0x0E18, 0x0E19, 0x0E1A, 0x0E1B, 0x0E1C, 0x0E1D, 0x0E1E, 0x0E1F, 0x0E20, 0x0E21, 0x0E22, 0x0E23,
    0x0E24, 0x0E25, 0x0E26, 0x0E27, 0x0E28, 0x0E29, 0x0E2A, 0x0E2B, 0x0E2C, 0x0E2D, 0x0E2E, 0x0E2F, 0x0E30, 0x0E31,
    0x0E32, 0x0E33, 0x0E34, 0x0E35, 0x0E36, 0x0E37, 0x0E38, 0x0E39, 0x0E3A, 0x0E3B, 0x0E3C, 0x0E3D, 0x0E3E, 0x0E3F,
    0x0E40, 0x0E41, 0x0E42, 0x0E43, 0x0E44, 0x0E45, 0x0E46, 0x0E47, 0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C, 0x0E4D,
    0x0E4E, 0x0E4F, 0x0E50, 0x0E51, 0x0E52, 0x0E53, 0x0E54, 0x0E55, 0x0E56, 0x0E57, 0x0E58, 0x0E59, 0x0E5A, 0x0E5B,
    0x0E5C, 0x0E5D, 0x0E5E, 0x0E5F, 0x0E60, 0x0E61, 0x0E62, 0x0E63, 0x0E64, 0x0E65, 0x0E66, 0x0E67, 0x0E69, 0x0E6A,
    0x0E6B, 0x0E6C, 0x0E6F, 0x0E70, 0x0E71, 0x0E73, 0x0E74, 0x0E75, 0x0E76, 0x0E77, 0x0E78, 0x0E79, 0x0E7A, 0x0E7B,
    0x0E7C, 0x0E7D, 0x0E7E, 0x0E7F, 0x0E80, 0x0E81, 0x0E82, 0x0E83, 0x0E84, 0x0E85, 0x0E86, 0x0E87, 0x0E88, 0x0E89,
    0x0E8A, 0x0E8B, 0x0E8C, 0x0E8D, 0x0E8E, 0x0E8F, 0x0E90, 0x0E91, 0x0E92, 0x0E93, 0x0E94, 0x0E95, 0x0E96, 0x0E97,
    0x0E98, 0x0E99, 0x0E9A, 0x0E9B, 0x0E9C, 0x0E9D, 0x0E9E, 0x0E9F, 0x0EA0, 0x0EA1, 0x0EA2, 0x0EA3, 0x0EA4, 0x0EA6,
    0x0EA7, 0x0EA8, 0x0EA9, 0x0EAB, 0x0EAC, 0x0EAD, 0x0EAE, 0x0EAF, 0x0EB0, 0x0EB1, 0x0EB2, 0x0EB3, 0x0EB4, 0x0EB5,
    0x0EB6, 0x0EB7, 0x0EBA, 0x0EBB, 0x0EBC, 0x0EBD, 0x0EBE, 0x0EBF, 0x0EC0, 0x0EC1, 0x0EC2, 0x0EC3, 0x0EC4, 0x0EC5,
    0x0EC6, 0x0EC7, 0x0EC8, 0x0EC9, 0x0ECA, 0x0ECB, 0x0ECC, 0x0ECD, 0x0ECE, 0x0ECF, 0x0ED0, 0x0ED1, 0x0ED2, 0x0ED3,
    0x0ED4, 0x0ED5, 0x0ED6, 0x0ED7, 0x0ED8, 0x0ED9, 0x0EDA, 0x0EDB, 0x0EDC, 0x0EDD, 0x0EDE, 0x0EDF, 0x0EE0, 0x0EE1,
    0x0EE2, 0x0EE3, 0x0EE4, 0x0EE5, 0x0EE6, 0x0EE7, 0x0EE8, 0x0EE9, 0x0EEA, 0x0EEB, 0x0EEC, 0x0EED, 0x0EEE, 0x0EEF,
    0x0EF0, 0x0EF1, 0x0EF3, 0x0EF5, 0x0EF6, 0x0EF7, 0x0EF8, 0x0EF9, 0x0EFA, 0x0EFB, 0x0EFC, 0x0EFD, 0x0EFE, 0x0EFF,
    0x0F00, 0x0F01, 0x0F02, 0x0F03, 0x0F04, 0x0F05, 0x0F06, 0x0F07, 0x0F08, 0x0F09, 0x0F0A, 0x0F0B, 0x0F0C, 0x0F0D,
    0x0F0E, 0x0F0F, 0x0F10, 0x0F11, 0x0F12, 0x0F13, 0x0F14, 0x0F15, 0x0F16, 0x0F17, 0x0F18, 0x0F19, 0x0F1A, 0x0F1B,
    0x0F1C, 0x0F1D, 0x0F1E, 0x0F1F, 0x0F20, 0x0F21, 0x0F22, 0x0F23, 0x0F24, 0x0F25, 0x0F26, 0x0F27, 0x0F28, 0x0F29,
    0x0F2A, 0x0F2B, 0x0F2C, 0x0F2D, 0x0F2E, 0x0F2F, 0x0F30, 0x0F31, 0x0F32, 0x0F33, 0x0F34, 0x0F35, 0x0F36, 0x0F37,
    0x0F38, 0x0F39, 0x0F3A, 0x0F3B, 0x0F3C, 0x0F3D, 0x0F3E, 0x0F3F, 0x0F40, 0x0F41, 0x0F42, 0x0F43, 0x0F44, 0x0F45,
    0x0F46, 0x0F47, 0x0F49, 0x0F4A, 0x0F4B, 0x0F4C, 0x0F4D, 0x0F4E, 0x0F4F, 0x0F50, 0x0F51, 0x0F52, 0x0F53, 0x0F54,
    0x0F55, 0x0F56, 0x0F57, 0x0F58, 0x0F59, 0x0F5A, 0x0F5B, 0x0F5C, 0x0F5D, 0x0F5E, 0x0F5F, 0x0F61, 0x0F62, 0x0F63,
    0x0F64, 0x0F65, 0x0F66, 0x0F67, 0x0F68, 0x0F69, 0x0F6A, 0x0F6B, 0x0F6C, 0x0F6D, 0x0F6E, 0x0F6F, 0x0F70, 0x0F71,
    0x0F72, 0x0F73, 0x0F74, 0x0F75, 0x0F76, 0x0F77, 0x0F78, 0x0F79, 0x0F7A, 0x0F7B, 0x0F7C, 0x0F7D, 0x0F7E, 0x0F7F,
    0x0F80, 0x0F81, 0x0F82, 0x0F83, 0x0F84, 0x0F85, 0x0F86, 0x0F87, 0x0F88, 0x0F89, 0x0F8A, 0x0F8B, 0x0F8C, 0x0F8D,
    0x0F8E, 0x0F8F, 0x0F90, 0x0F91, 0x0F92, 0x0F93, 0x0F94, 0x0F95, 0x0F96, 0x0F97, 0x0F98, 0x0F99, 0x0F9A, 0x0F9B,
    0x0F9C, 0x0F9D, 0x0F9E, 0x0F9F, 0x0FA0, 0x0FA1, 0x0FA2, 0x0FA3, 0x0FA4, 0x0FA5, 0x0FA6, 0x0FA7, 0x0FA8, 0x0FA9,
    0x0FAA, 0x0FAB, 0x0FAC, 0x0FAD, 0x0FAE, 0x0FAF, 0x0FB0, 0x0FB1, 0x0FB2, 0x0FB3, 0x0FB4, 0x0FB5, 0x0FB6, 0x0FB7,
    0x0FB8, 0x0FB9, 0x0FBA, 0x0FBB, 0x0FBC, 0x0FBD, 0x0FBE, 0x0FBF, 0x0FC0, 0x0FC1, 0x0FC2, 0x0FC3, 0x0FC4, 0x0FC5,
    0x0FC6, 0x0FC7, 0x0FC8, 0x0FC9, 0x0FCA, 0x0FCB, 0x0FCC, 0x0FCD, 0x0FCE, 0x0FCF, 0x0FD0, 0x0FD1, 0x0FD2, 0x0FD3,
    0x0FD4, 0x0FD5, 0x0FD6, 0x0FD7, 0x0FD8, 0x0FD9, 0x0FDA, 0x0FDC, 0x103A, 0x103D, 0x103E, 0x103F, 0x1040, 0x1041,
    0x1042, 0x1043, 0x1044, 0x1045, 0x1046, 0x1047, 0x1048, 0x1049, 0x104A, 0x104B, 0x104C, 0x104D, 0x104E, 0x104F,
    0x1050, 0x1051, 0x1052, 0x1053, 0x1054, 0x1055, 0x1056, 0x1057, 0x1058, 0x1059, 0x105A, 0x105B, 0x105C, 0x105D,
    0x105E, 0x105F, 0x1060, 0x1061, 0x1062, 0x1063, 0x1064, 0x1065, 0x1066, 0x1067, 0x1068, 0x1069, 0x106A, 0x106B,
    0x106C, 0x106D, 0x106E, 0x106F, 0x1070, 0x1071, 0x1072, 0x1073, 0x1074, 0x1075, 0x1076, 0x1077, 0x1078, 0x1079,
    0x107A, 0x107B, 0x107C, 0x107D, 0x107E, 0x107F, 0x1080, 0x1081, 0x1082, 0x1083, 0x1084, 0x1085, 0x1086, 0x1087,
    0x1088, 0x1089, 0x108A, 0x108B, 0x108C, 0x108D, 0x108E, 0x108F, 0x1090, 0x1091, 0x1092, 0x1093, 0x1094, 0x1095,
    0x1096, 0x1097, 0x1098, 0x1099, 0x109A, 0x109B, 0x109C, 0x109D, 0x109E, 0x109F, 0x10A0, 0x10A1, 0x10A2, 0x10A3,
    0x10A4, 0x10A5, 0x10A6, 0x10A7, 0x10A8, 0x10A9, 0x10AA, 0x10AB, 0x10AC, 0x10AD, 0x10AE, 0x10AF, 0x10B0, 0x10B1,
    0x10B2, 0x10B3, 0x10B4, 0x10B5, 0x10B6, 0x10B7, 0x10B8, 0x10B9, 0x10BA, 0x10BB, 0x10BC, 0x10BD, 0x10BE, 0x10BF,
    0x10C0, 0x10C1, 0x10C2, 0x10C3, 0x10C4, 0x10C5, 0x10C6, 0x10C7, 0x10C8, 0x10C9, 0x10CA, 0x10CB, 0x10CC, 0x10CD,
    0x10CE, 0x10CF, 0x10D0, 0x10D1, 0x10D2, 0x10D3, 0x10D4, 0x10D5, 0x10D6, 0x10D7, 0x10D8, 0x10D9, 0x10DA, 0x10DB,
    0x10DC, 0x10DD, 0x10DE, 0x10DF, 0x10E0, 0x10E1, 0x10E2, 0x10E3, 0x10E4, 0x10E5, 0x10E6, 0x10E7, 0x10E8, 0x10E9,
    0x10EA, 0x10EB, 0x10EC, 0x10ED, 0x10EE, 0x10EF, 0x10F0, 0x10F1, 0x10F2, 0x10F3, 0x10F4, 0x10F5, 0x10F6, 0x10F7,
    0x10F8, 0x10F9, 0x10FA, 0x10FB, 0x10FC, 0x10FD, 0x10FE, 0x10FF, 0x1100, 0x1101, 0x1102, 0x1103, 0x1104, 0x1105,
    0x1106, 0x1107, 0x1108, 0x1109, 0x110A, 0x110B, 0x110C, 0x110D, 0x110E, 0x110F, 0x1110, 0x1111, 0x1112, 0x1113,
    0x1114, 0x1115, 0x1116, 0xFFFF};
// clang-format off
static const char* const BT_CO_NAME[] = {
"Ericsson",
"Nokia Mobile Phones",
"Intel",
"IBM",
"Toshiba",
"3Com",
"Microsoft",
"Lucent",
"Motorola",
"Infineon Tech",
"Qualcomm Tech Intl",
"Silicon Wave",
"Digianswer",
"Texas Instruments",
"Parthus Tech",
"Broadcom",
"Mitel Semi",
"Widcomm",
"Zeevo",
"Atmel",
"Mitsubishi Electric",
"RTX",
"KC Tech",
"Newlogic",
"Transilica",
"Rohde & Schwarz",
"TTPCom",
"Signia Tech",
"Conexant Sys",
"Qualcomm",
"Inventel",
"AVM Berlin",
"BandSpeed",
"Mansella",
"NEC",
"WavePlus Tech",
"Alcatel",
"NXP",
"C Tech",
"Open Interface",
"R F Micro Devices",
"Hitachi",
"Symbol Tech",
"Tenovis",
"Macronix Intl",
"GCT Semi",
"Norwood Sys",
"MewTel Tech",
"ST Micro",
"Synopsys",
"Red-M",
"Commil",
"Computer Access",
"Renesas Elec",
"Mobilian",
"Syntronix",
"Integrated System",
"Panasonic",
"Gennum",
"BlackBerry",
"IPextreme",
"Sys and Chips",
"Bluetooth SIG",
"Seiko Epson",
"Integrated Silicon",
"CONWISE Tech",
"PARROT AUTOMOTIVE",
"Socket Mobile",
"Atheros Comm",
"MediaTek",
"Bluegiga",
"Marvell Tech",
"3DSP",
"Accel Semi",
"AUMOVIO Sys",
"Apple",
"Staccato Comm",
"Avago Tech",
"APT",
"SiRF Tech",
"Tzero Tech",
"J&M",
"Free2move",
"3DiJoy",
"Plantronics",
"Sony Ericsson",
"Harman Intl Ind",
"Vizio",
"Nordic Semi ASA",
"EM",
"Ralink Tech",
"Belkin",
"Realtek Semi",
"Stonestreet One",
"Wicentric",
"RivieraWaves S.A.S",
"RDA Micro",
"Gibson Guitars",
"MiCommand",
"Band XI Intl",
"HP",
"GN Hearing",
"General Motors",
"A&D Engineering",
"LTM",
"Polar",
"Beautiful",
"Summit Data Comm",
"Sound ID",
"Monster",
"connectBlue",
"ShangHai Super",
"Sense",
"Zomm",
"Samsung",
"Creative Tech",
"Laird Connectivity",
"Nike",
"lesswire",
"MStar Semi",
"Hanlynn Tech",
"Seers Tech",
"Autonet Mobile",
"DeLorme Publishing",
"WuXi Vimicro",
"DSEA",
"TimeKeeping Sys",
"Ludus Helsinki",
"BlueRadios",
"Equinux",
"Garmin",
"GN Hearing",
"Jawbone",
"Topcon Positioning",
"Gimbal",
"Zscan Software",
"Quintic",
"Telit Wireless Sol",
"Funai Electric",
"Advanced PANMOBIL",
"ThinkOptics",
"Universal Elec",
"Airoha Tech",
"NEC Lighting",
"ODM Tech",
"ConnecteDevice",
"zero1.tv",
"i.Tech Dynamic",
"Alpwise",
"Jiangsu Toppower",
"Colorfy",
"Geoforce",
"Bose",
"Suunto",
"Kensington Computer",
"SR-Medizinelektroni",
"Vertu",
"Meta Watch",
"LINAK",
"OTL Dynamics",
"Panda Ocean",
"Visteon",
"ARP Devices",
"MARELLI EUROPE",
"CAEN RFID",
"Ingenieur-Systemgru",
"Green Throttle",
"Peter Systemtechnik",
"Omegawave",
"Cinetix",
"Passif Semi",
"Saris Cycling",
"Bekey",
"Clarinox Tech",
"BDE Tech",
"Swirl Net",
"Meso Intl",
"TreLab",
"Qualcomm Innovation",
"Johnson Controls",
"Starkey Hearing",
"S-Power Elec",
"Ace Sensor",
"Aplix",
"AAMP of America",
"Stalmart Tech",
"AMICCOM Elec",
"Shenzhen Excelsecu",
"Geneq",
"adidas",
"LG Elec",
"Onset Computer",
"Selfly",
"Quuppa",
"GeLo",
"Evluma",
"MC10",
"Binauric",
"Beats Elec",
"Microchip Tech",
"Eve Sys",
"ARCHOS",
"Dexcom",
"Polar",
"Renesas Design",
"Taixingbang Tech Co",
"Austco Comm Sys",
"Timex",
"Qualcomm Tech",
"Qualcomm Connected",
"Voyetra Turtle",
"txtr",
"Snuza (Pty)",
"Procter & Gamble",
"Hosiden",
"Muzik",
"Misfit Wearables",
"Google",
"Danlers",
"Semilink",
"inMusic Brands",
"L.S. Research",
"Eden Software",
"KS Tech",
"ACTS Tech",
"Vtrack Sys",
"Nielsen-Kellerman",
"Server Tech",
"BioResearch",
"Jolly Logic",
"Above Average",
"Bitsplitters",
"PayPal",
"Witron Tech",
"Morse Project",
"Kent Displays",
"Nautilus",
"Smartifier",
"VSN Tech",
"AceUni",
"Crystal Alarm",
"KOUKAAM a.s",
"Delphi",
"ValenceTech",
"Stanley Black and",
"Typo Products",
"TomTom Intl",
"Fugoo",
"Keiser",
"Bang & Olufsen",
"PLUS Location Sys",
"Ubiquitous",
"Innovative Yachtter",
"Demant",
"Chicony Elec",
"Atus",
"Codegate",
"ERi",
"Transducers Direct",
"DENSO TEN",
"Audi",
"HiSilicon Tech",
"Nippon Seiki",
"Steelseries",
"Visybl",
"Openbrain Tech",
"e.solutions",
"10AK Tech",
"Wimoto Tech",
"Radius Net",
"Qualcomm Labs",
"Hewlett Packard",
"Baidu",
"Arendi",
"Skoda Auto a.s",
"Volkswagen",
"Porsche",
"Sino Wealth Elec",
"AirTurn",
"Kinsa",
"HID Global",
"SEAT es",
"Promethean",
"Salutica Allied Sol",
"GPSI",
"Nimble Devices",
"Changzhou Yongse",
"SportIQ",
"TEMEC Instruments",
"Sony",
"ASSA ABLOY",
"Clarion",
"Warehouse",
"Cypress Semi",
"MADS",
"Blue Maestro",
"Resolution Products",
"Aireware",
"Silvair",
"Prestigio Plaza",
"NTEO",
"Focus Sys",
"Tencent",
"Allegion",
"Murata Mfg",
"WirelessWERX",
"Nod",
"Alpine Elec (China)",
"FedEx Services",
"Grape Sys",
"Bkon Connect",
"Lintech",
"Novatel Wireless",
"Ciright",
"Mighty Cast",
"Ambimat Elec",
"Perytons",
"Tivoli Audio",
"Master Lock",
"Mesh-Net",
"HUIZHOU DESAY SV",
"Tangerine",
"B&W",
"Pioneer",
"OnBeep",
"Vernier Software &",
"ROL Ergo",
"Pebble Tech",
"NETATMO",
"Accumulate",
"Huami",
"Inmite s.r.o",
"ChefSteps",
"micas",
"Biomedical Research",
"Pitius Tec",
"Estimote",
"Unikey Tech",
"Timer Cap",
"AwoX",
"yikes",
"MADSGlobalNZ",
"PCH Intl",
"Qingdao Yeelink",
"Milwaukee Electric",
"MISHIK",
"Ascensia Diabetes",
"Spicebox",
"emberlight",
"Copeland Cold Chain",
"Qblinks",
"MYSPHERA",
"LifeScan",
"Volantic",
"Podo Labs",
"Roche Diabetes Care",
"Amazon",
"Connovate Tech",
"Kocomojo",
"Everykey",
"Dynamic Controls",
"SentriLock",
"I-SYST",
"Casio",
"LAPIS Semi",
"Telemonitor",
"taskit",
"Mercedes-Benz",
"BatAndCat",
"BluDotz",
"XTel Wireless",
"Gigaset Tech",
"Gecko Health",
"Walt Disney",
"Nectar",
"Seraphim Sense",
"Unico RBC",
"Physical",
"Able Trend Tech",
"Konica Minolta",
"Wilo",
"Extron Design",
"Google",
"Intelletto Tech",
"FDK",
"Cloudleaf",
"Maveric Automation",
"Acoustic Stream",
"Zuli",
"Paxton Access",
"WiSilica",
"VENGIT Korlatolt",
"SALTO Sys",
"TRON Forum",
"CUBETECH s.r.o",
"Cokiya",
"CVS Health",
"Ceruus",
"Strainstall",
"Channel Enterprises",
"FIAMM",
"GIGALANE.CO",
"MSA Innovation",
"Icon Health and",
"Wille Engineering",
"ENERGOUS",
"Taobao",
"Canon",
"Geophysical Tech",
"Meta",
"Trividia Health",
"FlightSafety Intl",
"Sunrise Micro",
"Star Micronics",
"Netizens Sp. z o.o",
"Nymi",
"Nytec",
"Trineo Sp. z o.o",
"Nest Labs",
"LM Tech",
"General Electric",
"i+D3",
"HANA Micron",
"SPIA Cycling",
"Cochlear Bone",
"SenionLab",
"Syszone",
"Pulsate Mobile",
"Hongkong OnMicro",
"BRADATECH",
"Transenergooil",
"DME Micro",
"Bitcraze",
"HASWARE",
"Abiogenix",
"Poly-Control",
"Avi-on",
"Laerdal Medical",
"Fetch My Pet",
"Sam Labs",
"Chengdu Synwing",
"HOUWA SYSTEM DESIGN",
"BSH",
"Primus Inter Pares",
"August Home",
"Gill Elec",
"Sky Wave Design",
"Newlab S.r.l",
"ELAD",
"G-wearables",
"Code",
"Savant Sys",
"Logitech Intl",
"Innblue Consulting",
"Philips",
"Minelab Elec",
"Bison",
"WS Audiology",
"Jolla",
"Caterpillar",
"Freedom Innovations",
"Dynamic Devices",
"Tech Sol (UK)",
"Advanced",
"Spreadtrum Comm",
"Valeo Service",
"Fullpower Tech",
"KloudNation",
"Zebra Tech",
"Itron",
"The University of",
"UTC Fire and",
"Cool Webthings",
"DJO Global",
"Gelliner",
"Anyka (Guangzhou)",
"Medtronic",
"Gozio",
"Form Lifting",
"Wahoo Fitness",
"Kontakt",
"Radio Sys",
"Freescale Semi",
"Verifone Sys Pte",
"AR Timing",
"Rigado",
"Kemppi",
"Otter Products",
"STEMP",
"LumiGeek",
"InvisionHeart",
"Macnica",
"Jaguar Land Rover",
"CoroWare Tech",
"Omron Healthcare",
"Comodule",
"ikeGPS",
"Telink Semi",
"Interplan",
"Wyler",
"IK Multimedia",
"Lukoton Experience",
"MTI",
"Tech4home Lda",
"DOTT",
"Blue Speck Labs",
"Cisco Sys",
"Mobicomm",
"Edamic",
"Goodnet",
"Luster Leaf",
"Manus Machina",
"Mobiquity Net",
"Praxis Dynamics",
"Philip Morris",
"Comarch",
"Nestlé Nespresso",
"Merlinia",
"LifeBEAM Tech",
"Twocanoes Labs",
"Muoverti",
"Stamer Musikanlagen",
"Tesla",
"Pharynks",
"Lupine",
"Siemens",
"Foster Electric",
"ETA",
"x-Senso Sol",
"FengFan Tech",
"Qrio",
"Pitpatpet",
"MSHeli s.r.l",
"Trakm8",
"JIN",
"Alatech Tehnology",
"Beijing CarePulse",
"ViCentra",
"Raven Ind",
"WaveWare Tech",
"Argenox Tech",
"Bragi",
"Masimo",
"Iotera",
"Endress+Hauser",
"ACKme Net",
"FiftyThree",
"Parker Hannifin",
"Uwatec",
"Orlan",
"Blue Clover Devices",
"M-Way Sol",
"Microtronics",
"Schneider",
"Sapphire Circuits",
"Lumo Bodytech",
"Restar",
"Xicato",
"Playbrush",
"Dai Nippon Printing",
"G24 Power",
"AdBabble Local",
"Devialet",
"ALTYOR",
"University of",
"Five Interactive",
"NetEase（Hangzhou）Ne",
"Lexmark Intl",
"Fluke",
"Yardarm Tech",
"SECVRE",
"Glacial Ridge Tech",
"DDS",
"SMK",
"Schawbel Tech",
"XMI Sys",
"Cerevo",
"Torrox GmbH &",
"Gemalto",
"DEKA Research &",
"Domster Tadeusz",
"Technogym",
"FLEURBAEY BVBA",
"Aptcode Sol",
"LSI ADL Tech",
"Animas",
"Alps Alpine",
"OCEASOFT",
"Motsai Research",
"Geotab",
"E.G.O",
"bewhere",
"Johnson Outdoors",
"steute Schaltgerate",
"Ekomini",
"DEFA",
"Aseptika",
"HUAWEI Tech",
"HabitAware",
"ruwido austria",
"ITEC",
"StoneL",
"Sonova",
"Maven Machines",
"Synapse Elec",
"WOWTech Canada",
"RF Code",
"Wally Ventures",
"SK Telecom",
"Jetro",
"Code Gears",
"NANOLINK",
"RF Digital",
"Church & Dwight",
"Multibit",
"CliniCloud",
"Blue Bite",
"ELIAS",
"Sivantos",
"Petzl",
"storm power",
"EISST",
"Inexess Tech Simma",
"Currant",
"C2 Development",
"Blue Sky Scientific",
"ALOTTAZS LABS",
"Kupson spol. s r.o",
"Areus Engineering",
"Impossible Camera",
"Sera4",
"Itude",
"Pacific Lock",
"Tendyron",
"Robert Bosch",
"Illuxtron Intl",
"miSport",
"Chargelib",
"Doppler Lab",
"BBPOS",
"Rx Net",
"WeatherFlow",
"Technicolor USA",
"Bestechnic(Shanghai",
"Raden",
"Oura",
"CLABER",
"Hyginex",
"HANSHIN ELECTRIC",
"Schneider Electric",
"Oort Tech",
"Chrono Therapeutics",
"Rinnai",
"Swissprime Tech",
"YOKOWO",
"Genevac",
"Chemtronics",
"Seguro Tech Sp. z",
"Dash Robotics",
"LINE",
"Guillemot",
"Techtronic Power",
"Wilson Sporting",
"Lenovo (Singapore)",
"Ayatan Sensors",
"Elec Tomorrow",
"OneSpan",
"PayRange",
"ABOV Semi",
"AINA-Wireless",
"BMA ergonomics",
"Teva Branded",
"Anima",
"3M",
"Empatica",
"Afero",
"Powercast",
"Secuyou",
"OMRON",
"Send Sol",
"NIPPON SYSTEMWARE",
"Neosfar",
"Fliegl Agrartechnik",
"Gilvader",
"Digi Intl Inc (R)",
"DeWalch Tech",
"Flint",
"Samsung SDS",
"Blur Product",
"University of",
"Victron Energy",
"NTT docomo",
"Carmanah Tech",
"Bytestorm",
"Espressif Sys",
"Unwire",
"Connected Yard",
"American Music",
"Sensogram Tech",
"Fujitsu",
"Ardic Tech",
"HTC",
"Citizen",
"SMART-INNOVATION.in",
"Blackrat Software",
"The Idea Cave",
"GoPro",
"AuthAir",
"Vensi",
"Indagem Tech",
"Intemo Tech",
"Runteq",
"IMAGINATION Tech",
"Clarius Mobile",
"Shanghai Frequen",
"Lierda Science &",
"Silicon Labs",
"World Moto",
"Giatec Scientific",
"Loop Devices",
"IACA electronique",
"Oura",
"Swipp",
"Life Laboratory",
"FUJI Ind",
"Surefire",
"Dolby Labs",
"Ellisys",
"Magnitude Lighting",
"Hilti",
"Devdata S.r.l",
"Shortcut Labs",
"SGL Italia S.r.l",
"PEEQ DATA",
"Ducere Tech Pvt",
"DiveNav",
"RIIG AI Sp. z o.o",
"Thermo Fisher",
"AG Measurematics",
"CHUO Elec",
"Aspenta Intl",
"Eugster Frismag",
"Wurth Elektronik",
"HQ",
"Lab Sensor Sol",
"Enterlab",
"Eyefi",
"MetaSystem",
"SONO ELECTRONICS",
"Rotor Bike",
"Astro",
"Healthwear Tech",
"Essex Elec",
"Grundfos",
"Eargo",
"Elec Design Lab",
"ESYLUX",
"NIPPON SMT.CO",
"BM innovations",
"indoormap",
"OttoQ",
"North Pole",
"3flares Tech",
"Mul-T-Lock",
"Airthings ASA",
"Enlighted",
"GISTIC",
"AJP2",
"COBI",
"Blue Sky Scientific",
"Appception",
"Courtney Thorne",
"TPV Tech",
"Monitra",
"Automation",
"Etesian Tech",
"GERTEC BRASIL",
"Drekker Development",
"Whirl",
"Locus Positioning",
"Acuity Brands",
"Prevent Biometrics",
"VersaMe",
"Libratone",
"HM Elec",
"TASER Intl",
"Heartland Payment",
"Bitstrata Sys",
"Pieps",
"iRiding(Xiamen)Tech",
"Alpha Audiotronics",
"TOPPAN",
"Sigma Designs",
"Spectrum Brands",
"Polymap Wireless",
"MagniWare",
"Novotec Medical",
"Phillips-Medisize",
"Matrix",
"Eaton",
"KYS",
"Naya Health",
"Acromag",
"Insulet",
"Wellinks",
"ON Semi",
"FREELAP",
"Favero Elec",
"BioMech Sensor",
"BOLTT Sports Tech",
"Metormote",
"littleBits",
"SetPoint Medical",
"BRControls Products",
"Zipcar",
"AirBolt",
"MOTIVE Tech",
"Motiv",
"Wazombi Labs OÜ",
"Nixie Labs",
"AppNearMe",
"Holman Ind",
"Expain",
"Elec Temperature",
"Plejd",
"Propeller Health",
"Shenzhen iMCO Elec",
"Algoria",
"Apption Labs",
"Cronologics",
"MICRODIA",
"lulabytes",
"Société des",
"LLC \"MEGA-F",
"Sharp",
"Precision Outcomes",
"Kronos",
"Embedded Elec Sol",
"Aterica",
"BluStor PMC",
"Kapsch TrafficCom",
"ActiveBlu",
"Kohler Mira",
"Noke",
"Appion",
"Resmed",
"Crownstone",
"Xiaomi",
"INFOTECH s.r.o",
"Thingsquare",
"T&D",
"LAVAZZA",
"SDATAWAY",
"BLOKS",
"LEGO System",
"Thetatronics",
"Nikon",
"NeST",
"South Silicon",
"ALE Intl",
"CareView Comm",
"SchoolBoard",
"Molex",
"IVT Wireless",
"Alpine Labs",
"Candura Instruments",
"SmartMovt Tech",
"Token Zero",
"ACE CAD Enterprise",
"Medela",
"AeroScout",
"Esrille",
"Exon Sp. z o.o",
"Meizu Tech",
"XiQ",
"Allswell",
"Comm-N-Sense Corp",
"VIBRADORM",
"Otodata Wireless",
"Propagation Sys",
"Midwest Instruments",
"Alpha Nodus",
"petPOMM",
"Mattel",
"Airbly",
"A-Safe",
"FREDERIQUE CONSTANT",
"Maxscend Micro",
"Abbott",
"ASB Bank",
"amadas",
"Applied Science",
"iLumi Sol",
"Arch Sys",
"Ember Tech",
"Snap",
"Casambi Tech",
"Pico Tech",
"St. Jude Medical",
"Intricon",
"Structural Health",
"Avvel Intl",
"Gallagher",
"In2things",
"SYSDEV",
"Vonkil Tech",
"Wynd Tech",
"CONTRINEX",
"MIRA",
"Watteam",
"Density",
"IOT Pot India",
"Sigma Connectivity",
"PEG PEREGO",
"Wyzelink Sys",
"Yota Devices",
"FINSECUR",
"Zen-Me Labs",
"3IWare",
"EnOcean",
"Instabeat",
"Nima Labs",
"Andreas Stihl AG &",
"Nathan Rhoades",
"Grob Tech",
"Actions Tech",
"SPD Development",
"Sensoan",
"Qualcomm Life",
"Chip-ing",
"ffly4u",
"IoT Instruments",
"TRUE Fitness Tech",
"SHENZHEN LEMONJOY",
"Hello",
"Ozo Edu",
"Jigowatts",
"BASIC MICRO.COM",
"CUBE Tech",
"CLINK",
"Hestan Smart",
"WindowMaster",
"PAL Tech",
"WHERE",
"Iton Tech",
"Owl Labs",
"Rockford",
"Becon Tech",
"Vyassoft Tech",
"Nox Medical",
"Kimberly-Clark",
"Trimble",
"Littelfuse",
"Withings",
"i-developer IT",
"Relations",
"Sears",
"Gantner Elec",
"Authomate",
"Vertex Intl",
"Swiss Audio",
"ToGetHome",
"ZF OPENMATICS s.r.o",
"Jana Care",
"NorthStar Battery",
"SKF (U.K.)",
"CO-AX Tech",
"Fender Musical",
"Luidia",
"SEFAM",
"Wireless Cables",
"SODA",
"Fatigue Science",
"Novalogy",
"Friday Labs",
"OrthoAccel Tech",
"WaterGuru",
"Benning",
"Dell Computer",
"Kopin",
"TecBakery",
"Backbone Labs",
"DELSEY",
"Chargifi",
"Trainesense",
"Unify Software and",
"Husqvarna",
"Focus fleet and",
"SmallLoop",
"Prolon",
"BD Medical",
"iMicroMed",
"Ticto",
"Meshtech",
"MemCachier",
"Danfoss",
"SnapStyk",
"Alticor",
"Silk Labs",
"Pillsy",
"Hatch Baby",
"Blocks Wearables",
"Drayson Tech",
"eBest IOT",
"Helvar",
"Radiance Tech",
"Nuheara",
"Appside",
"Coiler",
"Thermomedics",
"Tentacle Sync",
"Valencell",
"SECOM",
"Tucker Intl",
"Metanate",
"Kobian Canada",
"NETGEAR",
"Fabtronics",
"Grand Centrix",
"1UP USA.com",
"SHIMANO",
"Nain",
"LifeStyle Lock",
"VEGA Grieshaber",
"Xtrava",
"TTS Tooltechnic Sys",
"Teenage Engineering",
"Tunstall Nordic",
"Svep Design Center",
"Qorvo Utrecht",
"Sphinx Elec GmbH &",
"Nemik Consulting",
"RF INNOVATION",
"Mini Solution",
"2048450 Ontario",
"Delta T",
"Boston Scientific",
"Nuviz",
"Real Time",
"Kolibree",
"vhf elektronik",
"Bonsai Sys",
"Fathom Sys",
"Bellman & Symfon",
"Intl Forte",
"Codenex",
"Kynesim",
"Palago",
"INSIGMA",
"PMD Sol",
"Qingdao Realtime",
"BEGA",
"Pambor",
"Develco Products",
"iDesign s.r.l",
"TiVo",
"Control-J",
"Steelcase",
"iApartment",
"Icom",
"Blue Spark Tech",
"FarSite Comm",
"mywerk system",
"Sinosun Tech",
"MIYOSHI Elec",
"Occly",
"OurHub Dev IvS",
"Pro-Mark",
"Quintrax",
"POS Tuning Udo",
"Revol Tech",
"SKIDATA",
"DEV TECNOLOGIA",
"Centrica Connected",
"Automotive Data Sol",
"Igarashi",
"Taelek",
"Vectronix",
"S-Labs Sp. z o.o",
"Companion Medical",
"BlueKitchen",
"Matting",
"SOREX - Wireless",
"ADC Tech",
"Lynxemi",
"SENNHEISER Elec",
"Polymorphic Labs",
"Cochlear",
"METER Inc. USA",
"Ruuvi Innovations",
"Situne",
"nVisti",
"DyOcean",
"Uhlmann & Zacher",
"AND!XOR",
"Popper Pay",
"ovrEngineered",
"GT-tronics",
"Herbert Waldmann",
"Guangzhou FiiO Elec",
"Vinetech",
"Dallas Logic",
"BioTex",
"LINKIO",
"Harbortronics",
"Undagrid",
"Shure",
"ERM Elec Sys",
"BIOROWER",
"Kartographers Tech",
"The Shadow on the",
"mobike (Hong Kong)",
"Inuheat",
"Swiftronix",
"Diagnoptics Tech",
"Analog Devices",
"Soraa",
"CSR Building",
"Crestron Elec",
"Neatebox",
"Draegerwerk AG &",
"AlbynMedical",
"Averos FZCO",
"VIT Initiative",
"Statsports Intl",
"Sospitas s.r.o",
"Dmet Products",
"Mantracourt Elec",
"TeAM Hutchins",
"Seibert Williams",
"Insta",
"Svantek Sp. z o.o",
"Shanghai Flyco",
"Thornwave Labs",
"Steiner-Optik",
"Novo Nordisk",
"Safetech Products",
"GOOOLED S.R.L",
"DOM",
"Olympus",
"KTS",
"Anloq Tech",
"Queercon",
"TASKA PROSTHETICS",
"Gooee",
"LUGLOC",
"Blincam",
"FUJIFILM",
"RM Acquisition",
"Franceschi Marina",
"Engineered Audio",
"IOTTIVE (OPC)",
"4MOD Tech",
"Lutron",
"Lutron",
"Guardtec",
"REACTEC",
"Under Armour",
"Woodenshark",
"Avack",
"Smart Solution Tech",
"STABILO Intl",
"Busch Jaeger",
"Pacific Bioscience",
"Bird Home",
"Motorola Sol",
"Auxivia",
"DaisyWorks",
"Kosi",
"Theben",
"InDreamer Techsol",
"Cerevast Medical",
"ZanCompute",
"Pirelli Tyre",
"McLear",
"Shenzhen Goodix",
"Convergence Sys",
"Interactio",
"Androtec",
"Benchmark Drives",
"SwingLync L. L. C",
"Tapkey",
"Woosim Sys",
"Microsemi",
"Wiliot",
"Polaris IND",
"Specifi-Kali",
"Locoroll",
"PHYPLUS",
"InPlay",
"Hager",
"Axes System sp. z",
"Garage Smart",
"Shake-on",
"Vibrissa",
"OSRAM",
"TRSystems",
"Yichip Micro",
"Foundation",
"UNI-ELECTRONICS",
"Brookfield Equinox",
"Soprod",
"9974091 Canada",
"FIBRO",
"RB Controls",
"Footmarks",
"Amtronic Sverige",
"MAMORIO.inc",
"Tyto Life",
"Leica Camera",
"EDPS",
"OFF Line",
"Detect Blue",
"Setec",
"Target",
"IAI",
"NS Tech",
"MTG",
"Hangzhou iMagic",
"HONGKONG NANO IC",
"Honeywell Intl",
"Albrecht JUNG",
"Lunera Lighting",
"Lumen",
"Keynes Controls",
"Novartis",
"Geosatis",
"EXFO",
"LEDVANCE",
"Center ID",
"Adolene",
"D&M",
"CRESCO Wireless",
"Nura Operations",
"Frontiergadget",
"Smart Component",
"ZTR Control Sys",
"MetaLogics",
"Medela",
"OPPLE Lighting",
"Savitech",
"prodigy",
"Screenovate Tech",
"TESA",
"CLIM8",
"Silergy",
"SilverPlus",
"Sharknet",
"Mist Sys",
"MIWA LOCK",
"OrthoSensor",
"Apexar Tech",
"LOGICDATA Elec &",
"Knick Elektronische",
"Smart Tech and",
"Linough",
"Advanced Elec",
"Carefree Scott",
"Sensome",
"FORTRONIK storitve",
"Sinnoz",
"Sylero",
"Avempace",
"Nintendo",
"National",
"KROHNE Messtechnik",
"Otodynamics",
"Arwin Tech",
"benegear",
"Newcon Optik",
"CANDY HOUSE",
"FRANKLIN Tech",
"Lely",
"Valve",
"Hekatron Vertriebs",
"PROTECH S.A.S. DI",
"Sarita CareTech",
"Finder",
"Thalmic Labs",
"Steinel",
"Beghelli",
"CORE TRANSPORT Tech",
"Xiamen Everesports",
"Bodyport",
"Flipnavi",
"Rion",
"Long Range Sys",
"Redmond Ind",
"VIZPIN",
"BikeFinder",
"Consumer Sleep Sol",
"PSIKICK",
"AntTail.com",
"Lighting Science",
"AFFORDABLE Elec",
"Integral Memroy",
"Globalstar",
"True Wearables",
"Wellington Drive",
"OMNI Remotes",
"Duracell U.S",
"Toor Tech",
"Instinct",
"Beco",
"Scuf Gaming Intl",
"LYS Tech",
"Breakwall Analytics",
"Code Blue Comm",
"Gira Giersiepen",
"Hearing Lab Tech",
"LEGRAND",
"Derichs",
"ALT-TEKNIK",
"Star Tech",
"START TODAY",
"Maxim Integrated",
"Fracarro",
"Jungheinrich",
"Meta",
"HENDON Semi",
"Pur3",
"Viasat",
"IZITHERM",
"Spaulding Clinical",
"Kohler",
"Inor Process",
"My Smart Blinds",
"RadioPulse",
"rapitag",
"Lazlo326",
"Teledyne Lecroy",
"Dataflow Sys",
"Macrogiga Elec",
"Tandem Diabetes",
"Polycom",
"Fisher & Paykel",
"Dream Devices Tech",
"Shanghai Xiaoyi",
"ADHERIUM(NZ)",
"Axiomware Sys",
"O. E. M. Controls",
"Kiiroo",
"Telecon Mobile",
"Sonos",
"Tom Allebrandi",
"Monidor",
"Tramex",
"Nofence",
"GoerTek Dynaudio",
"INIA",
"CARMATE MFG.CO",
"OV LOOP",
"NewTec",
"Medallion",
"CAREL Ind",
"Parabit Sys",
"White Horse",
"verisilicon",
"Elecs Industry",
"Beijing Pinecone",
"Ambystoma Labs",
"Suzhou Pairlink",
"igloohome",
"Oxford Metrics",
"Leviton Mfg",
"ULC Robotics",
"Real-World-Systems",
"Nalu Medical",
"P.I.Engineering",
"Grote Ind",
"Runtime",
"Codecoup sp. z o.o",
"SELVE GmbH &",
"Lippert Components",
"SOMFY",
"TBS Elec",
"MHL Custom",
"LucentWear",
"WATTS Elec",
"RJ Brands",
"V-ZUG",
"Biowatch",
"Anova Applied Elec",
"Lindab",
"frogblue Tech",
"Acurable",
"LAMPLIGHT",
"TEGAM",
"Zhuhai Jieli Tech",
"modum.io",
"Farm Jenny",
"Toyo Elec",
"Applied Neural",
"Avid Identification",
"Petronics",
"essentim",
"QT Medical",
"VIRTUALCLINIC.DIREC",
"Viper Design",
"Human",
"stAPPtronics",
"Elemental Machines",
"Taiyo Yuden",
"INEO ENERGY& Sys",
"Motion Instruments",
"PressurePro",
"COWBOY",
"iconmobile",
"ACS-Control-System",
"Bayerische Motoren",
"Gycom Svenska",
"Fuji Xerox",
"SIKOM",
"beken",
"The Linux",
"Try and E",
"SeeScan",
"Clearity",
"GS TAG",
"DPTechnics",
"TRACMO",
"Anki",
"Hagleitner Hygiene",
"Konami Sports Life",
"Arblet",
"Masbando",
"Innoseis",
"Niko",
"Wellnomics",
"iRobot",
"Schrader Elec",
"Geberit Intl",
"Fourth Evolution",
"FMW Elec Futterer",
"John Deere",
"Rookery Tech",
"KeySafe-Cloud",
"BUCHI Labortechnik",
"IQAir",
"Triax Tech",
"Vuzix",
"TDK",
"Blueair",
"Signify (Hue)",
"ADH GUARDIAN USA",
"Beurer",
"Playfinity",
"Hans Dinslage",
"OnAsset",
"INTER ACTION",
"OS42 UG",
"Audio-Technica",
"Six Guys Labs s.r.o",
"R.W. Beckett",
"silex Tech",
"Univations",
"SENS Innovation",
"Diamond Kinetics",
"Phrame",
"Forciot",
"Noordung d.o.o",
"Beam Labs",
"Biovotion",
"Square Panda",
"Amplifico",
"WEG",
"Ensto",
"PHONEPE PVT",
"MinebeaMitsumi",
"ASPion",
"Vossloh-Schwabe",
"Procept",
"ONKYO",
"Asthrea D.O.O",
"Fortiori Design",
"Hugo Muller GmbH &",
"Wangi Lai PLT",
"Fanstel",
"Crookwood",
"ELECTRONICA",
"GiP Innovation",
"LX Sol",
"Shenzhen Minew Tech",
"Prolojik",
"Kromek",
"Contec Medical Sys",
"Xradio Tech",
"The Indoor Lab",
"LDL Tech",
"Dish Network",
"Revenue Collection",
"Bluetrum Tech",
"makita",
"Apogee Instruments",
"BM3",
"SGV Holding GmbH &",
"MED-EL",
"Ultune Tech",
"Ryeex Tech",
"Open Research",
"Scale-Tec",
"Zumtobel",
"iLOQ",
"KRUXWorks Tech",
"Digital Matter",
"Coravin",
"Stasis Labs",
"ITZ Innovations",
"Meggitt",
"Ledlenser GmbH &",
"Renishaw",
"ZhuHai AdvanPro",
"Meshtronix",
"Payex Norge",
"UnSeen Tech",
"Marshall",
"Sesam Sol",
"PixArt Imaging",
"Panduit",
"Alo",
"Ricoh",
"RTC Ind",
"Mode Lighting",
"Particle Ind",
"Advanced Telemetry",
"RHA Tech",
"Pure Intl",
"WTO",
"Bleb Tech",
"Livanova USA",
"Brady Worldwide",
"DewertOkin",
"Ztove",
"Venso EcoSolutions",
"Eurotronik Kranj",
"Hug Tech",
"Gema Switzerland",
"Buzz Products",
"Kopi",
"Innova Ideas",
"BeSpoon",
"Expai Sol Private",
"Innovation First",
"SABIK Offshore",
"4iiii Innovations",
"The Energy",
"I.FARM",
"Tile",
"Form Athletica",
"NETGRID S.N.C. DI",
"Mannkind",
"Trade FIDES a.s",
"Photron",
"Eltako",
"Dermalapps",
"Greenwald Ind",
"inQs",
"Cherry",
"Amsted Digital Sol",
"Tacx",
"Raytac",
"Jiangsu Teranovo",
"Razer",
"JRM",
"Eccrine Sys",
"Curie Point",
"Georg Fischer",
"Hach - Danaher",
"T&A Labs",
"Koki",
"Gunakar Private",
"Stemco Products",
"Wood IT Security",
"RandomLab",
"Adero",
"Dragonchip",
"Noomi",
"Delta Elec",
"FlowMotion Tech",
"OBIQ Location Tech",
"Cardo Sys",
"Globalworx",
"Nymbus",
"LIMNO",
"TEKZITEL",
"Roambee",
"Chipsea Tech",
"GD Midea",
"Soundmax Elec",
"Produal",
"HMS Ind Net",
"Ingchips Tech",
"InnovaSea Sys",
"SenseQ",
"Shoof Tech",
"BRK Brands",
"SimpliSafe",
"Tussock Innovation",
"Sencilion",
"Wabilogic",
"Sociometric Sol",
"iCOGNIZE",
"ShadeCraft",
"Beflex",
"Beaconzone",
"Leaftronix Analogic",
"TWS",
"ABB",
"HitSeed",
"CAME",
"Alarm.com",
"Measurlogic",
"King I",
"Dream Labs",
"Urban Compass",
"Simm Tronic",
"Storz & Bickel GmbH",
"MYLAPS",
"Shenzhen Zhongguang",
"Dyeware",
"Dongguan",
"DIG",
"FIOR & GENTZ",
"Etekcity",
"Meyer Sound Labs",
"CeoTronics",
"Sensirion",
"JCT Healthcare",
"FUBA Automotive",
"AW",
"Shanghai Mountain",
"Zliide Tech",
"Automatic Labs",
"Ind Network",
"Intellithings",
"Navcast",
"HLI Sol",
"Avaya",
"Milestone AV Tech",
"Alango Tech",
"Spinlock",
"Aluna",
"OPTEX",
"NIHON DENGYO",
"VELUX",
"Almendo Tech",
"Zmartfun Elec",
"SafeLine Sweden",
"Houston Radar",
"J Neades",
"ALCARE",
"Chargy Tech",
"Shibutani",
"Trapper Data",
"Alfred Intl",
"Touché Tech",
"Vigil Tech",
"Vitulo Plus",
"WILKA",
"BodyPlus Tech",
"happybrush",
"Enequi",
"Sartorius",
"Tom Comm Ind",
"ESS Embedded System",
"Mahr",
"Redpine Signals",
"TraqFreq",
"PAFERS TECH",
"Akciju sabiedriba",
"Beijing Jingdong",
"JBX Designs",
"AB Electrolux",
"Wernher von Braun",
"Essity Hygiene and",
"Be Interactive",
"Carewear",
"Element Products",
"Beijing Winner",
"SmartSnugg",
"FiveCo",
"California Things",
"Audiodo",
"ABAX",
"Bull",
"Respiri",
"MindPeace Safety",
"MBARC LABS",
"Altonics",
"IDIBAIX",
"COREIOT",
"REVSMART WEARABLE",
"Precor",
"F5 Sports",
"exoTIC Sys",
"DONGGUAN HELE Elec",
"Dongguan Liesheng",
"Oculeve",
"Clover Network",
"Xiamen Eholder Elec",
"Ford Motor",
"Guangzhou",
"Tedee Sp. z o.o",
"PHC",
"Eli Lilly and",
"SwaraLink Tech",
"JMR embedded Sys",
"Bitkey",
"GWA Hygiene",
"Safera",
"Open Platform Sys",
"OnePlus Elec",
"Wildlife Acoustics",
"ABLIC",
"Dairy Tech",
"Iguanavation",
"DiUS Computing",
"UpRight Tech",
"Luna XIO",
"LLC Navitek",
"Glass Security",
"Jiangsu Qinheng",
"Chandler Sys",
"Fantini Cosmi",
"Beijing Hao Heng",
"Bluepack S.R.L",
"Beijing Unisoc Tech",
"MAC",
"DML",
"Sanofi",
"SOCOMEC",
"WiZ",
"Seitec Elektronik",
"OR Tech",
"GuangZhou KuGou",
"DIAODIAO Tech",
"Illusory Studios",
"Sarvavid Software",
"Amtech Sys",
"EAGLE DETECTION",
"MEDIATECH S.R.L",
"Hamilton",
"Changsha JEMO IC",
"Elatec",
"JLG Ind",
"Michael Parkin",
"Brother Ind",
"Lumens For Less",
"ELA Innovation",
"umanSense",
"Shanghai InGeek",
"HARMAN",
"Smart Sensor",
"Antitronics",
"RHOMBUS Sys",
"Katerra",
"Remote Solution",
"Vimar",
"Mantis Tech",
"TerOpta",
"PIKOLIN",
"WWZN Info Tech",
"Voxx Intl",
"ART AND PROGRAM",
"NITTO DENKO ASIA",
"Peloton Interactive",
"Force Impact Tech",
"Dmac Mobile",
"Engineered Medical",
"Noodle Tech",
"Graesslin",
"WuQi Tech",
"Successful",
"InnoCon Medical",
"Corvex Connected",
"Thirdwayv",
"C-MAX Asia",
"4eBusiness",
"Cyber Transport",
"Cue",
"KOAMTAC",
"Loopshore",
"Niruha Sys Private",
"radius",
"Sensority s.r.o",
"Sparkage",
"Glenview Software",
"Finch Tech",
"Qingping Tech",
"DeviceDrive",
"ESEMBER LIMITED",
"audifon GmbH &",
"O2 Micro",
"BubblyNet",
"PCB Piezotronics",
"The Wildflower",
"Optikam Tech",
"MINIBREW HOLDING",
"Cybex",
"FUJIMIC NIIGATA",
"Hanna Instruments",
"KOMPAN",
"Scosche Ind",
"Cricut",
"AEV spol. s r.o",
"The Coca-Cola",
"GASTEC",
"StarLeaf",
"Water-i.d",
"HoloKit",
"PlantChoir",
"GuangDong Oppo",
"CST Elec",
"Sky UK",
"Digibale",
"Smartloxx",
"Pune Scientific",
"Regent",
"Roku",
"Xiamen Mage Info",
"RAB Lighting",
"Musen Connect",
"Zume",
"conbee",
"Bruel & Kjaer Sound",
"The Kroger",
"Granite River Sol",
"LoupeDeck",
"New H3C Tech",
"Aurea Solucoes",
"Hong Kong Bouffalo",
"GV Concepts",
"Thomas Dynamics",
"Moeco IOT",
"2N TELEKOMUNIKACE",
"Hormann KG",
"CRONO CHIP",
"Soundbrenner",
"ETABLISSEMENTS",
"iSwip",
"Battery-Biz",
"EPIC S.R.L",
"Genedrive",
"Axentia Tech",
"REGULA",
"Biral",
"Radinn",
"CIMTechniques",
"Johnson Health Tech",
"June Life",
"Bluenetics",
"iaconicDesign",
"WRLDS Creations",
"Skullcandy",
"West Pharmaceutical",
"Barnacle Sys",
"Smart Wave Tech",
"Shanghai Top-Chip",
"NeoSensory",
"Tuya",
"Shanghai Panchip",
"React Accessibility",
"LIVNEX",
"Kano Computing",
"hoots classic",
"ecobee",
"Nanjing Qinheng",
"Sol AMBRA",
"Micro-Design",
"STARLITE",
"Remedee Labs",
"ThingOS GmbH &",
"Linear Circuits",
"Unlimited",
"Snap-on",
"Edifier Intl",
"Alfred Kaercher SE",
"Airoha Tech",
"Geeksme",
"Minut",
"Waybeyond",
"Komfort IQ",
"Packetcraft",
"Häfele GmbH &",
"ShapeLog",
"NOVABASE S.R.L",
"Frecce",
"Joule IQ",
"KidzTek",
"Aktiebolaget",
"e-moola.com",
"Zimi Innovations",
"SERENE",
"DIGISINE ENERGYTECH",
"MEDIRLAB",
"Byton North America",
"Shenzhen",
"Cesar Sys",
"quip NYC",
"Klipsch",
"Access",
"Renault",
"JSK",
"BIROTA",
"maxon motor",
"Optek",
"CRONUS Elec",
"NantSound",
"Domintell",
"Andon Health",
"Urbanminded",
"TYRI Sweden",
"ECD Elec Components",
"SISTEMAS KERN",
"Trulli Audio",
"Altaneos",
"Nanoleaf",
"Ingy",
"Azbil",
"TATTCOM",
"Paradox Engineering",
"LECO",
"Becker Antriebe",
"Mstream",
"Flextronics Intl",
"Ossur hf",
"SKC",
"SPICA Sys",
"Wangs Alliance",
"tatwah",
"Hunter Douglas",
"Shenzhen Conex",
"DIM3",
"Bobrick Washroom",
"Potrykus and",
"iNFORM Tech",
"eSenseLab",
"Brilliant Home Tech",
"INOVA Geophysical",
"adafruit Ind",
"8Power",
"CME",
"Hyundai Motor",
"Kickmaker",
"Shanghai Suisheng",
"HEXAGON METROLOGY",
"Mitutoyo",
"shenzhen fitcare",
"INGICS Tech",
"INCUS PERFORMANCE",
"ABB",
"Blippit",
"Foxble",
"Intermotive",
"Conneqtech",
"RIKEN KEIKI",
"Canopy Growth",
"Bitwards",
"vivo Mobile Comm",
"Etymotic Research",
"A puissance 3",
"BPW Bergische",
"Piaggio Fast",
"BeerTech",
"Tokenize",
"Zorachka",
"D-Link",
"Down Range Sys",
"General Luminaire",
"Tangshan HongJia",
"FRAGRANCE DELIVERY",
"Pepperl + Fuchs",
"Dometic",
"USound",
"DNANUDGE",
"JUJU JOINTS CANADA",
"Dopple Tech",
"ARCOM",
"Biotechware",
"ORSO",
"SafePort",
"Carol Cole",
"Embedded Fitness",
"Yealink (Xiamen)",
"Subeca",
"Cognosos",
"Pektron",
"Tap Sound System",
"Helios Sports",
"Canopy Growth",
"Parsyl",
"SOUNDBOKS",
"BlueUp",
"DAKATECH",
"Nisshinbo Micro",
"ACOS",
"Guilin Zhishen Info",
"Krog Sys",
"Alflex Products",
"SmartSensor Labs",
"SmartDrive",
"Yo-tronics Tech",
"Rafaelmicro",
"Emergency Lighting",
"LAONZ",
"Western Digital",
"WIOsense GmbH &",
"EVVA",
"Odic",
"Pacific Track",
"Revvo Tech",
"Biometrika d.o.o",
"Vorwerk",
"Trackunit",
"Wyze Labs",
"Dension",
"11 Health & Tech",
"Innophase",
"Treegreen",
"Berner Intl",
"SmartResQ",
"Valtech",
"The Chamberlain",
"MIZUNO",
"ZRF",
"BYSTAMP",
"Crosscan",
"Konftel",
"1bar.net",
"Phillips Connect",
"imagiLabs",
"Optalert",
"PSYONIC",
"Wintersteiger",
"Controlid Industria",
"Movella Tech",
"Hydro-Gear Limited",
"EnPointe Fencing",
"XANTHIO",
"sclak s.r.l",
"Tricorder Arraay",
"GB Solution",
"Soliton Sys",
"Tait Intl",
"NICHIEI INTEC",
"SmartWireless GmbH",
"Ingenieurbuero",
"Maytronics",
"EPIFIT",
"Gimer medical",
"Nokian Renkaat",
"Current Lighting",
"SFS unimarket",
"Private limited",
"Saucon Tech",
"Embedded Devices",
"J-J.A.D.E",
"i-SENS",
"Witschi Elec",
"Aclara Tech",
"EXEO TECH",
"Epic Sys",
"Hoffmann",
"Realme Chongqing",
"Intelligenceworks",
"TGR 1.618",
"Shanghai Kfcube",
"Fraunhofer IIS",
"SZ DJI Tech",
"Coburn Tech",
"Topre",
"Kayamatics",
"Moticon ReGo",
"Polidea Sp. z o.o",
"Trivedi Advanced",
"CORE|vision",
"PF",
"IONIQ Skincare GmbH",
"Sengled",
"Boehringer",
"ABB",
"Check Tech Sol",
"U-Shin",
"HYPER ICE",
"Tokai-rika",
"Prevayl",
"bf1systems",
"ubisys Tech",
"SIRC",
"Accent Advanced Sys",
"Rayden.Earth",
"Lindinvent",
"CHIPOLO d.o.o",
"J. Wagner",
"Monadnock Sys",
"Liteboxer Tech",
"Noventa",
"Nubia Tech",
"JT INNOVATIONS",
"TGM Tech",
"ifly",
"ZIMI",
"betternotstealmybik",
"ESTOM Infotech",
"Sensovium",
"Virscient",
"Novel Bits",
"ADATA Tech",
"KEYes",
"Inovonics",
"WARES",
"Pointr Labs",
"Miridia Tech",
"Tertium Tech",
"SHENZHEN AUKEY E",
"code-Q",
"TE Connectivity",
"IRIS OHYAMA",
"Philia Tech",
"KOZO KEIKAKU",
"Shenzhen Simo Tech",
"Republic Wireless",
"Rashidov",
"Crowd Connected",
"Eneso Tecnologia de",
"Barrot Tech",
"Naonext",
"Taiwan Smart Home",
"COWBELL ENGINEERING",
"Beijing Big Moment",
"Denso",
"IMI Hydronic",
"Askey Computer",
"Cumulus Digital Sys",
"Joovv",
"The L.S. Starrett",
"Microoled",
"PSP - Pauli",
"Kodimo Tech",
"Tymtix Tech Private",
"Dermal Photonics",
"MTD Products Inc &",
"instagrid",
"Spacelabs Medical",
"Darkglass Elec",
"Hill-Rom",
"BioIntelliSense",
"Ketronixs",
"Plastimold Products",
"Beijing Zizai Tech",
"Lucimed",
"TSC Auto-ID Tech",
"DATAMARS",
"SUNCORPORATION",
"Yandex Services",
"Scope Logistical",
"User Hello",
"Pinpoint",
"70mai",
"Zhuhai Hoksi Tech",
"EMBR labs",
"Radiawave Tech",
"OPTIMUSIOT TECH",
"VC",
"ASR Micro",
"Douglas Lighting",
"Nerbio Medical",
"Braveheart Wireless",
"INEO-SENSE",
"Honda Motor",
"Ambient Sensors",
"ASR",
"NO SMD",
"Albertronic",
"Luminostics",
"Oblamatik",
"Innokind",
"Melbot Studios",
"Myzee Tech",
"KAHA",
"Shanghai MXCHIP",
"JSB TECH",
"Yukai Engineering",
"Gooligum Tech",
"ROOQ",
"AiRISTA",
"Qingdao Haier Tech",
"Sappl Verwaltungs",
"TekHome",
"PCI Private",
"Leggett & Platt",
"PS",
"C.O.B.O",
"James Walker",
"BREATHINGS",
"SRAM",
"KiteSpring",
"Reconnect",
"Elekon",
"RealThingks",
"Henway Tech",
"ASTEM",
"LinkedSemi Micro",
"ENSESO",
"Xenoma",
"Adolf Wuerth GmbH &",
"Catalyft Labs",
"JEPICO",
"Hero Workout",
"Rivian Automotive",
"TRANSSION",
"Agitron d.o.o",
"Globe (Jiangsu)",
"AMC Intl Alfa",
"First Light Tech",
"Wearable Link",
"Metronom Health",
"Zwift",
"Kindeva Drug",
"GimmiSys",
"tkLABS",
"PassiveBolt",
"Limited Liability",
"Capetech",
"PPRS",
"Apptricity",
"LogiLube",
"Julbo",
"Breville",
"Kerlink",
"Ohsung Elec",
"ZTE",
"HerdDogg",
"Lismore Instruments",
"LogiLube",
"Elec Theatre",
"BioEchoNet",
"NUANCE HEARING",
"Sena Tech",
"Linkura",
"GL Sol",
"Moonbird",
"Countrymate Tech",
"Asahi Kasei",
"PointGuard",
"Neo Materials and",
"Actev Motors",
"Woan Tech",
"dricos",
"Guide ID",
"Gunwerks",
"Band Ind",
"IBA Dosimetry",
"GA",
"Popit",
"ABEYE",
"BlueIOT(Beijing)",
"Fauna Audio",
"TOYOTA motor",
"ZifferEins GmbH &",
"BIOTRONIK SE &",
"CORE",
"CTEK Sweden",
"Thorley Ind",
"CLB",
"SonicSensory",
"ISEMAR S.R.L",
"DEKRA TESTING AND",
"Bernard Krone",
"ELPRO-BUCHS",
"Feedback Sports",
"TeraTron",
"Cello Hill",
"TSE BRAKES",
"BHM-Tech",
"WIKA Alexander",
"Biovigil",
"Mequonic",
"bGrid",
"ADVEEZ",
"Aktiebolaget Regin",
"Anton Paar",
"Telenor ASA",
"Big Kaiser",
"Absolute Audio Labs",
"VT42",
"Bronkhorst",
"C. & E. Fein",
"NextMind",
"Pixie Dust Tech",
"eTactica ehf",
"New Audio",
"Sendum Wireless",
"deister Elec",
"YKK AP",
"Step One",
"Koya Medical",
"Proof Diagnostics",
"VOS Sys",
"ENGAGENOW DATA",
"ARDUINO",
"KUMHO ELECTRICS",
"Security",
"BEIJING ELECTRIC",
"Paybuddy",
"KHN Sol",
"Nippon Ceramic",
"PHOTODYNAMIC",
"DashLogic",
"Ambiq",
"Narhwall",
"Pozyx",
"ifLink Open",
"Deublin",
"BLINQY",
"DYPHI",
"BlueX Micro",
"PentaLock",
"AUTEC Gesellschaft",
"Pegasus Tech",
"Bout Labs",
"PlayerData",
"SAVOY Elec LIGHTING",
"Elimo Engineering",
"SkyStream",
"Aerosens",
"Centre Suisse",
"Vessel",
"Span.IO",
"AnotherBrain",
"Rosewill",
"Universal Audio",
"JAPAN TOBACCO",
"UVISIO",
"HungYi Micro",
"Honor Device",
"Combustion",
"XUNTONG",
"CrowdGlow",
"Mobitrace",
"Hx Engineering",
"Senso4s d.o.o",
"Julius Blum",
"BlueStreak IoT",
"Chess Wise",
"ABLEPAY Tech",
"Temperature",
"ORBIS",
"GEAR RADIO Elec",
"EAR TEKNIK ISITME",
"Coyotta",
"Synergy Tecnologia",
"VivoSensMedical",
"Nagravision",
"Bionic Avionics",
"Innoware",
"JLD Tech Sol",
"Magnus Tech",
"Tag-N-Trac",
"Friday Home",
"CPS",
"Mobilogix",
"Masonite",
"Kabushikigaisha",
"Melange Sys Pvt",
"LumenRadio",
"Athlos",
"KEAN Elec",
"Yukon advanced",
"Sibel",
"OJMAR",
"Steinel Sol",
"WatchGas",
"OM Digital Sol",
"Audeara",
"Beijing Zero Zero",
"Spectrum Tech",
"OKI Electric",
"Mobile Action Tech",
"SENSATEC",
"R.O. S.R.L",
"Hangzhou Yaguan",
"Listen Tech",
"TOITU",
"Confidex",
"Lichtvision",
"AIRSTAR",
"Ampler Bikes OU",
"Cleveron",
"Ayxon-Dynamics",
"donutrobotics",
"Flosonics Medical",
"Southwire",
"Shanghai wuqi Micro",
"Reflow",
"Oras",
"Volan Tech",
"Shanghai Yidian",
"Blue Peacock",
"Roland",
"LIXIL",
"SUBARU",
"Sensolus",
"Dyson Tech",
"Tec4med LifeScience",
"CROXEL",
"Syng",
"Plume Design",
"Cambridge Animal",
"Maxell",
"Link Labs",
"Embrava",
"INPEAK sp. z o.o",
"API-K",
"CombiQ",
"DeVilbiss",
"Jiangxi Innotech",
"Apollogic Sp. z o.o",
"DAIICHIKOSHO",
"BIXOLON",
"Atmosic Tech",
"Eran Financial",
"Louis Vuitton",
"NanoFlex Power",
"Worthcloud Tech",
"Yamaha",
"PaceBait IVS",
"Shenzhen H&T Smart",
"Shenzhen Feasycom",
"Instamic",
"Air-Weigh",
"Nevro",
"Pinnacle Tech",
"WMF",
"Luxer",
"safectory",
"NGK SPARK PLUG",
"2587702 Ontario",
"Bouffalo Lab",
"BLUETICKETING",
"Galileo Tech",
"Siteco",
"DELABIE",
"Shenzhen Yopeak",
"OPEX",
"Motionalysis",
"Busch Sys Intl",
"Novidan",
"3SI Security Sys",
"Beijing HC-Infinite",
"The Wand",
"JRC Mobility",
"Map Large",
"MistyWest Energy",
"SiFli Tech",
"Lockn Tech Private",
"Toytec",
"VANMOOF Global",
"Nextscape",
"CSIRO",
"Follow Sense Europe",
"KKM",
"SQL Tech",
"Inugo Sys",
"ambie",
"Meizhou Guo Wei",
"Indigo Diabetes",
"TourBuilt",
"Sontheim Industrie",
"LEGIC Identsystems",
"Innovative Design",
"MG Energy Sys",
"stryker",
"DATANG Semi Tech",
"Smart Parks",
"MOKO Tech",
"Geopal system",
"Lytx",
"Beijing SuperHexa",
"Focus Ingenieria",
"HAPPIEST BABY",
"Scribble Design",
"Olympic Ophthalmics",
"Pokkels",
"Pac Sane",
"Warner Bros",
"Ooma",
"Senquip",
"Jumo GmbH &",
"Innohome",
"MICROSON",
"Delta Cycle",
"Synaptics",
"AXTRO",
"Shenzhen Sunricher",
"Webasto",
"Emlid",
"UniqAir",
"WAFERLOCK",
"Freedman Elec",
"KEBA Handover",
"Intuity Medical",
"Cleer",
"Universal",
"Corsair",
"Rivata",
"Greennote",
"Snowball Tech",
"ALIZENT Intl",
"Shanghai Smart",
"PSA Peugeot Citroen",
"VusionGroup",
"HAINBUCH GMBH",
"SANlight",
"DelpSys s.r.o",
"JCM Tech",
"Perfect",
"TOTO",
"Shenzhen Grandsun",
"Monarch Intl",
"Carestream Dental",
"GiPStech S.r.l",
"OOBIK",
"Pamex",
"Foil",
"Shanghai",
"TEMKIN ASSOCIATES",
"Eello",
"Xi'an Fengyu Info",
"Canon Finetech",
"ista Intl",
"Loy Tec Elec",
"LINCOGN Tech",
"Care Bloom",
"DIC",
"FAZEPRO",
"Shenzhen Uascent",
"Realityworks",
"Urbanista",
"Zencontrol",
"Spintly",
"Computime Intl",
"Anhui Listenai",
"OSM",
"Adevo Consulting",
"PS Engineering",
"AIAIAI",
"Visiontronic s.r.o",
"InVue Security",
"TouchTronics",
"INNER RANGE",
"Ellenby Tech",
"Elstat Elec",
"Xenter",
"LogTag North",
"Sens.ai",
"STL",
"Open Bionics",
"R-DAS s.r.o",
"KCCS Mobile",
"Inventas",
"Robkoo Info & Tech",
"PAUL HARTMANN",
"Omni-ID USA",
"Shenzhen Jingxun",
"RealMega Micro Tech",
"Kenzen",
"CODIUM",
"Flexoptix",
"Barnes",
"Chengdu Aich Tech",
"Keepin",
"Swedlock",
"Shenzhen CoolKit",
"ise Individuelle",
"Nuvoton",
"Visuallex Sport",
"KOBATA GAUGE MFG",
"CACI Tech",
"Nordic Strong",
"Lautsprecher Teufel",
"SSV Software Sys",
"Zhuhai Pantum",
"Streamit",
"nymea",
"AL-KO Geraete",
"Franz Kaldewei",
"Codefabrik",
"Reelables",
"Duravit",
"Boss Audio",
"Vocera Comm",
"Douglas Dynamics",
"GlobalMed",
"DALI Alliance",
"unu",
"Hexology",
"Sunplus Tech",
"LEVEL s.r.o",
"FLIR Sys",
"Borda Tech",
"Square",
"FUTEK ADVANCED",
"Saxonar",
"Velentium",
"GLP German Light",
"Leupold & Stevens",
"CRADERS",
"701x",
"Radioworks Micro",
"Unitech Elec",
"AMETEK",
"Irdeto",
"First Design System",
"Unisto",
"Chengdu Ambit Tech",
"SMT ELEKTRONIK",
"Cerebrum Sensor",
"Weber Sensors",
"Earda Tech",
"FUSEAWARE",
"Flaircomm Micro",
"RESIDEO Tech",
"IORA Tech",
"Precision Triathlon",
"Marquardt",
"FAZUA",
"Workaround",
"Shenzhen Qianfenyi",
"Belun Tech",
"Sanistaal",
"BluPeak",
"SANYO DENKO",
"Minebea Access Sol",
"B.E.A",
"Alfa Laval",
"ThermoWorks",
"ToughBuilt Ind",
"IOTOOLS",
"Olumee",
"NAOS JAPAN",
"Guard RFID Sol",
"SIG SAUER",
"DECATHLON",
"WBS PROJECT H",
"Roca Sanitario",
"Nanoleq",
"Accelerated Sys",
"PB",
"Beijing ESWIN",
"TKH Security",
"Hygiene IQ",
"iRhythm Tech",
"BeiJing ZiJie",
"NIBROTECH",
"Baracoda Daily",
"Lumi United Tech",
"Tech-Venom",
"MAINBOT",
"ILLUMAGEAR",
"REDARC Elec",
"MOCA System",
"Duke Mfg",
"ART",
"Silver Wolf",
"Hala Sys",
"ARMATURA",
"CONZUMEX Ind",
"BH SENS",
"SINTEF",
"Omnivoltaic Energy",
"WISYCOM S.R.L",
"Red 100 Lighting",
"Impact Biosystems",
"AIC Semi",
"Dodge Ind",
"REALTIMEID",
"ISEO Serrature",
"MindRhythm",
"Havells India",
"Sentrax",
"TSI",
"INCITAT",
"nFore Tech",
"Elec Sensors",
"Gentex",
"NIO USA",
"SkyHawke Tech",
"Nomono",
"EMS Integrators",
"BiosBob.Biz",
"Adam Hall",
"ICP Sys",
"Breezi.io",
"Mesh Sys",
"FUN FACTORY",
"ZIIP",
"SHENZHEN KAADAS",
"Emotion Fitness",
"H G M Automotive",
"BORA - Vertriebs",
"CONVERTRONIX Tech",
"TOKAI-DENSHI",
"Shenzhen",
"Fujian Newland",
"CELLCONTROL",
"Rivieh",
"RATOC Sys",
"Sentek",
"NOVEA ENERGIES",
"Innolux",
"NingBo klite",
"The Apache Software",
"MITSUBISHI ELECTRIC",
"Quha",
"Addaday",
"Dymo",
"Samsara Net",
"Sensitech",
"SOLUM",
"React Mobile",
"JDRF Electromag",
"lilbit ODM",
"Geeknet",
"HARADA INDUSTRY",
"BQN",
"Triple W Japan",
"MAX-co",
"Aixlink(Chengdu)",
"FIELD DESIGN",
"Sankyo Air Tech",
"Shenzhen KTC Tech",
"Hardcoder",
"Scangrip",
"FoundersLane",
"Offcode",
"ICU tech",
"AXELIFE",
"SCM",
"Mammut Sports",
"Taiga Motors",
"Presidio Medical",
"VIMANA TECH",
"Trek Bicycle",
"Ampetronic",
"Muguang (Guangdong)",
"Rotronic",
"Seiko Instruments",
"American Tech",
"MOTREX",
"Pertech Ind",
"Gentle Energy",
"Senscomm Semi",
"Alfen ICU",
"Hangzhou BroadLink",
"Dreem",
"Telecom Design",
"SILVER TREE LABS",
"Gymstory",
"The Goodyear Tire &",
"Beijing Wisepool",
"Komatsu",
"Sensoria",
"Audio Partnership",
"Lotus",
"Data Sciences Intl",
"Bunn-O-Matic",
"TireCheck",
"Sonova Consumer",
"Vervent Audio",
"SONICOS ENTERPRISES",
"Nissan Motor",
"hearX (Pty)",
"GLOWFORGE",
"Allterco Robotics",
"Infinitegra",
"Grandex Intl",
"Machfu",
"Roambotics",
"Soma Labs",
"NITTO KOGYO",
"Ecolab",
"Beijing ranxin",
"Fjorden Electra",
"Flender",
"New Cosmos USA",
"Xirgo Tech",
"Build With Robots",
"IONA Tech",
"INNOVAG",
"SaluStim",
"Huso",
"SWISSINNO Sol",
"T2REALITY Sol",
"SAAB Aktiebolag",
"HIMSA II K/S",
"READY FOR SKY",
"Miele & Cie",
"EntWick",
"MCOT",
"TECHTICS",
"Aperia Tech",
"TCL / RayNeo",
"Signtle",
"OTF Distribution",
"Neuvatek",
"Perimeter Tech",
"Divesoft s.r.o",
"Sylvac",
"Amiko",
"Neurosity",
"LL Tec",
"Durag",
"Hubei Yuan Times",
"IDEC",
"Procon Analytics",
"ndd Medizintechnik",
"Super B Lithium",
"Shenzhen Injoinic",
"PURA SCENTS",
"Aardex",
"CHAR-BROIL",
"TWINKLY",
"Coroflo",
"Yale",
"WINKEY ENTERPRISE",
"Koizumi Lighting",
"OTC engineering",
"Comtel Sys",
"Deepfield Connect",
"ZWILLING J.A",
"Puratap",
"Fresnel Tech",
"Sensormate",
"Shindengen Electric",
"Twenty Five Seven",
"Luna Health",
"CORAL-TAIYI",
"LINKSYS USA",
"Safetytest",
"KIDO SPORTS",
"Site IQ",
"Angel Medical Sys",
"PONE BIOMETRICS",
"T5 tek",
"greenTEG",
"Wacker Neuson",
"Innovacionnye",
"CleanBands Sys",
"Dodam Enersys",
"T+A elektroakustik",
"Esmé Sol",
"Media-Cartec",
"Ratio Electric",
"MQA",
"NEOWRK SISTEMAS",
"Loomanet",
"Puff",
"Happy Health",
"Montage Connect",
"LED Smart",
"CONSTRUKTS",
"limited liability",
"Senic",
"Automated Pet Care",
"aconno",
"Mendeltron",
"Mereltron",
"ALEX DENKO",
"AETERLINK",
"Cosmed s.r.l",
"Gordon Murray",
"IoSA",
"Scandinavian Health",
"Fasetto",
"Geva Sol",
"TYKEE",
"SomnoMed",
"CORROHM",
"Arlo Tech",
"Catapult Intl",
"Rockchip Elec",
"GEMU",
"OFF Line Japan",
"EC sense",
"LVI",
"COMELIT",
"Foshan Viomi",
"Glamo",
"KEYTEC",
"SMARTD Tech",
"JURA",
"Performance Elec",
"Pal Elec",
"Embecta",
"DENSO AIRCOOL",
"Caresix",
"GigaDevice Semi",
"Zeku Tech",
"OTF Product",
"Easee",
"BEEHERO",
"McIntosh",
"KINDOO",
"Xian Yisuobao Elec",
"Exeger Operations",
"BYD",
"Thermokon-Sensortec",
"SignalQuest",
"Noritz",
"TIGER",
"ORB Innovations",
"Classified Cycling",
"Wrmth",
"BELLDESIGN",
"Stinger Equipment",
"HORIBA",
"Control Sol",
"Heath Consultants",
"Berlinger &",
"ONCELABS",
"Brose Verwaltung SE",
"Epsilon Elec lnc",
"VALEO MANAGEMENT",
"twopounds",
"atSpiro",
"ADTRAN",
"Orpyx Medical Tech",
"Seekwave Tech",
"Tactile Engineering",
"SharkNinja",
"Imostar Tech",
"INNOVA S.R.L",
"ESCEA",
"Taco",
"HiViz Lighting",
"Zintouch",
"Rheem Sales",
"UNEEG medical",
"Hykso",
"CYBERDYNE",
"Lockswitch",
"Alban Giacomo",
"MGM WIRELESSS",
"StepUp Sol",
"BlueID",
"Wuxi Linkpower",
"KEBA Energy",
"NNOXX",
"Phiaton",
"phg Peter Hengstler",
"dormakaba Holding",
"WAKO",
"TRACKTING S.R.L",
"Emerja",
"CONSORCIO TRUST",
"GILSON",
"SNIFF LOGIC",
"Fidure",
"Sensa",
"Parakey",
"SCARAB Sol",
"BitGreen Technolabz",
"StreetCar ORV",
"Truma Gerätetechnik",
"yupiteru",
"Embedded",
"TEAC",
"CHARGTRON IOT",
"Zhuhai Smartlink",
"Triductor Tech",
"PT SADAMAYA GRAHA",
"Mopeka Products",
"3ALogics",
"Rochester Sensors",
"CARDIOID - Tech LDA",
"Carrier",
"NACON",
"Watchdog Sys",
"MAXON Ind",
"Amlogic",
"Qingdao Eastsoft",
"Weltek Tech",
"Nextivity",
"AGZZX",
"A.GLOBAL",
"Heavys",
"T-Mobile USA",
"tonies",
"Technocon",
"Radar Automobile",
"WESCO",
"Yashu Sys",
"Kesseböhmer",
"Movesense",
"Baxter Healthcare",
"Gemstone Lights",
"H+B Hightech",
"Deako",
"Vire Health",
"ALF",
"NTT sonority",
"Sunstone-RTLS Ipari",
"Ribbiot",
"Dragonfly Energy",
"BIGBEN",
"YAMAHA MOTOR",
"XSENSE",
"MAQUET",
"MITSUBISHI ELECTRIC",
"Princess Cruise",
"Megger",
"Verve InfoTec",
"Sonas",
"Mievo Tech Private",
"Shenzhen Poseidon",
"HERUTU Elec",
"Shenzhen Shokz",
"Shenzhen",
"Evident",
"NEURINNOV",
"SwipeSense",
"RF Creations",
"SHINKAWA Sensor",
"janova",
"Eberspaecher",
"Racketry d. o. o",
"THE EELECTRIC",
"Cucumber Lighting",
"seca GmbH &",
"Ameso Tech (OPC)",
"Emlid Tech",
"Pricer",
"Forward Thinking",
"Garnet Instruments",
"CLEIO",
"Anker Innovations",
"HMD Global",
"ABUS August",
"Open Road Sol",
"Serial Tech",
"SB C&S",
"TrikThom",
"Innocent Tech",
"Cyclops Marine",
"NOTHING Tech",
"Kord Defence",
"YanFeng",
"SENOSPACE",
"Shenzhen CESI Info",
"MooreSilicon Semi",
"Imagine Marketing",
"EQOM SSC",
"TechSwipe",
"Reoqoo IoT Tech",
"Numa Products",
"HHO (Hangzhou)",
"Maztech Ind",
"SIA Mesh",
"Minami acoustics",
"Wolf Steel",
"Circus World",
"Ypsomed",
"Alif Semi",
"SHENZHEN CHENYUN",
"VODALOGIC",
"Regal Beloit",
"CORVENT MEDICAL",
"Taiwan Fuhsing",
"Off-Highway",
"Amina Distribution",
"mwConnect",
"TAG HEUER",
"Dongguan Yougo Elec",
"PEAG LLC dba JLab",
"HAYWARD Ind",
"Shenzhen Tingting",
"Pacific Coast",
"CV. NURI TEKNIK",
"MadgeTech",
"POGS",
"THOTAKA",
"Midmark",
"Radio Sound",
"SOLUX",
"BOS Balance of",
"OJ Elec",
"TVS Motor",
"core sensing",
"Tamblue",
"Protect Animals",
"Tyromotion",
"ElectronX design",
"Thule",
"KEEPEN",
"Rocky Mountain",
"MakuSafe",
"Bartec Auto Id",
"Energy Tech and",
"doubleO",
"Datalogic S.r.l",
"Datalogic USA",
"Leica Geosystems",
"CATEYE",
"Research Products",
"Planmeca",
"C.Ed. Schulte GmbH",
"PetVoice",
"Timebirds Australia",
"JVC KENWOOD",
"Spartek Sys",
"MERRY Elec",
"Merry Elec (S)",
"Spark",
"Nations Tech",
"Akix S.r.l",
"Bioliberty",
"C.G. Air Systemes",
"Maturix",
"RACHIO",
"LIMBOID",
"Electronics4All",
"FESTINA LOTUS",
"Synkopi",
"SCIENTERRA",
"Cennox",
"Cedarware",
"GREE Electric",
"Japan Display",
"System Elite",
"Burkert Werke GmbH",
"velocitux",
"FUJITSU COMPONENT",
"MIYAKAWA ELECTRIC",
"PhysioLogic Devices",
"Sensoryx",
"SIL System",
"Cooler Pro",
"Advanced Elec",
"Delta Development",
"Laxmi Therapeutic",
"SYNCHRON",
"Badger Meter",
"Micropower",
"ZILLIOT Tech",
"Universidad",
"XIHAO INTELLIGENGT",
"Zerene",
"CycLock",
"Frost Sol",
"Lone Star Marine",
"SIRONA Dental Sys",
"bHaptics",
"LUMINOAH",
"Vogels Products",
"SignalFire",
"CPAC Sys",
"TEKTRO Tech",
"Gosuncn Tech",
"Ex Makhina",
"Odeon",
"Thales Simulation &",
"Shenzhen DOKE Elec",
"Vemcon",
"Refrigerated",
"Rockpile Sol",
"Soundwave Hearing",
"Optec",
"NIKAT Sol PRIVATE",
"Movano",
"NINGBO FOTILE",
"Genetus",
"DIVAN TRADING",
"Luxottica",
"ISEKI FRANCE S.A.S",
"NO CLIMB PRODUCTS",
"Wellang.Co",
"Nanjing",
"ifm Elec",
"HYUPSUNG MACHINERY",
"Gunnebo Aktiebolag",
"Axis Comm",
"Stogger",
"Pella",
"SiChuan Homme Smart",
"Smart Products",
"F.I.P. FORMATURA",
"MEBSTER s.r.o",
"SKF France",
"Southco",
"Molnlycke Health",
"Hendrickson USA",
"BLACK BOX NETWORK",
"Status Audio",
"AIR AROMA Intl",
"Helge Kaiser",
"Crane Payment",
"DYNAMOX S/A",
"Look Cycle Intl",
"Closed Joint Stock",
"Kindhome",
"Kiteras",
"Earfun Tech",
"iota Biosciences",
"ANUME s.r.o",
"Indistinguishable",
"i-focus",
"DualNetworks",
"MITACHI",
"VIVIWARE JAPAN",
"Xiamen Intretech",
"MindMaze",
"BeiJing SmartChip",
"Taiko Audio",
"Daihatsu Motor",
"Konova",
"Gravaa",
"Beyerdynamic GmbH &",
"VELCO",
"ATLANTIC SOCIETE",
"Testo SE & Co. KGaA",
"SEW-EURODRIVE GmbH",
"ROCKWELL AUTOMATION",
"Quectel Wireless",
"Nanohex",
"Simply Embedded",
"Software",
"Ultimea Tech",
"RF Elec",
"Optivolt Labs",
"Canon Elec",
"LAAS",
"Beamex",
"TACHIKAWA",
"HagerEnergy",
"Hunter Ind",
"NEOKOHM SISTEMAS",
"Zhejiang Huanfu",
"E.F. Johnson",
"Caire",
"Yeasound (Xiamen)",
"Boxyz",
"Skytech Creations",
"Cear",
"Impulse Wellness",
"MML US",
"SICK",
"Fen Sys",
"KIWI.KI",
"Airgraft",
"HP Tuners",
"PIXELA",
"Generac",
"Novoferm tormatic",
"Airwallet",
"Inventronics",
"Shenzhen EBELONG",
"Efento",
"ITALTRACTOR ITM",
"TITUM AUDIO",
"Hexagon Aura",
"Invisalert Sol",
"TELE System Comm",
"Whirlpool",
"SHENZHEN REFLYING",
"Franklin Control",
"Djup",
"SAFEGUARD EQUIPMENT",
"Morningstar",
"Shenzhen Chuangyuan",
"CompanyDeep",
"Veo Tech",
"Nexis Link Tech",
"Felion Tech",
"MAATEL",
"HELLA GmbH & Co",
"HWM-Water",
"Shenzhen Jahport",
"NACHI-FUJIKOSHI",
"Cirrus Research",
"GEARBAC Tech",
"Hangzhou",
"DHL",
"Levita",
"MORNINGSTAR FX",
"ETO GRUPPE Tech",
"farmunited",
"Aptener",
"GEOPH",
"Trotec",
"Astra LED",
"NOVAFON",
"KUBU SMART",
"ESNAH",
"OrangeMicro",
"Fresh n Rebel",
"Global Satellite",
"KOQOON GmbH & Co.KG",
"BEEPINGS",
"MODULAR MEDICAL",
"Xiant Tech",
"SCHELL GmbH &",
"Minebea Intec",
"KAGA FEI",
"AUTHOR-ALARM razvoj",
"Tozoa",
"SHENZHEN DNS Ind",
"Shenzhen Lunci Tech",
"KNOG",
"Outshiny India",
"TAMADIC",
"Shenzhen MODSEMI",
"EMBEINT",
"Ehong Tech",
"DEXATEK Tech",
"Dendro Tech",
"Vivint",
"General Laser",
"Kathrein Sol",
"Fitz",
"ATEGENOS",
"Flextronic",
"Safety Swim",
"SING SUN Tech",
"Woncan (Hong Kong)",
"iFLYTEK (Suzhou)",
"Weber-Stephen",
"hDrop Tech",
"REEKON TOOLS",
"Delta Faucet",
"Mutrack",
"Hangzhou Zhaotong",
"Chengdu CSCT Micro",
"Belusun Tech",
"Shenzhen Matches",
"Beidou Smart",
"SOJI Elec JOINT",
"BH Tech",
"Haptech",
"WaveRF",
"SHENZHEN SOUNDSOUL",
"Wuhu Mengbo Tech",
"PROSYS DEV",
"Shenzhen eMeet Tech",
"Doro",
"SUREPULSE MEDICAL",
"iodyne",
"Pinpoint",
"Heinrich Kopp",
"Evolutive Sys",
"Sounding Audio Ind",
"Yuanfeng Tech",
"FrontAct",
"SenseWorks",
"Eko Health",
"Wanzl GmbH & Co",
"CLEVER LOGGER Tech",
"ASYSTOM",
"Heilongjiang",
"Eastern Partner",
"Xiamen RUI YI Da",
"Ad Hoc Elec",
"Hangzhou Microimage",
"Hive-Zox Intl",
"Sensovo",
"Time Location Sys",
"SHENZHEN DIGITECH",
"Capte",
"9512-5837 QUEBEC",
"Blecon",
"CFLAB TEKNOLOJI",
"FOGO",
"HITO",
"MS kajak7 UG",
"Avedis Zildjian",
"Hangzhou Hikvision",
"LIHJOEN SPEED METER",
"NextSense",
"PatchRx",
"Flipper Devices",
"Huizhou Foryou",
"JE Elec",
"9313-7263 Quebec",
"ECARX (Hubei) Tech",
"NIHON KOHDEN",
"ONWI",
"Primax Elec",
"AlphaTheta",
"PACIFIC Ind",
"Crescent",
"Vermis software Sol",
"SNAPPWISH",
"Cousins and Sears",
"CESYS Gesellschaft",
"SLOC",
"IRES Infrarot",
"OFIVE",
"Swift IOT Tech",
"Viselabs",
"Walmart",
"VANBOX",
"Wiser Devices",
"WKD Labs",
"Asustek Computer",
"Z-ONE Tech",
"InnoVision Medical",
"QUANTATEC",
"Filo",
"SOUNDUCT",
"Hosiden Besson",
"KARLUNA MUHENDISLIK",
"Deone Comm & Tech",
"Nitto Denko",
"PUDSEY DIAMOND",
"Luxshare Precision",
"Brooksee",
"QSC",
"eBet Gaming Sytems",
"Zhejiang Desman",
"Dyaco Intl",
"Aquana",
"MINIRIG",
"After Tech",
"New Cosmos Electric",
"INEPRO Metering",
"Altina",
"Urban Armor Gear",
"Loewe Tech",
"OmniWave Micro",
"Wuhu Hongjing Elec",
"Rocoto",
"L.T.H. Elec",
"SHAPER TOOLS",
"Ruptela",
"Ant",
"Queclink Wireless",
"GOKI",
"LAST LOCK",
"HuiTong",
"Daikin Ind",
"Shenzhen Baseus",
"NEXT DEVICES",
"Megatronix Tech",
"Hyena",
"Shenzhen Goodocom",
"RIGH",
"OpConnect",
"Powerstick.com",
"ENABLEWEAR",
"Adventures of the",
"Rocky Radios",
"Le Touch Elec",
"Guangdong Nanguang",
"MOBILE TECH",
"HONG KONG Comm",
"Neptune First OU",
"Vivago",
"Circular",
"final",
"Jiangsu XinTongda",
"Archon Controls",
"SZR-Dev UG",
"IQNEXXT Sol",
"Guangdong Hengqin",
"CHEVALIER TECH",
"SPRiNTUS",
"Tymphany",
"TigerLight",
"Mercury Marine a",
"OpenTech Alliance",
"Skewered Fencing",
"Brudden",
"Tele-Radio i",
"Allgon",
"Gopod Holding",
"Celebrities",
"Nobest",
"Avetos Design",
"RainMaker Sol",
"Identita",
"Glutz",
"MIV Elec",
"Tactrix",
"Viaanix",
"Skeed",
"kokoromil",
"Chromatic",
"Medibound",
"Scanbro OU",
"Panasonic",
"Andrews & Arnold",
"Audinor",
"Travelxp India",
"Owlet Baby Care",
"ENLESS Wireless",
"Culligan Intl",
"QIKCONNEX",
"OLIS Elec",
"BiTECH Automotive",
"AMG Lab",
"BrickXter",
"Dongguan Trangjan",
"Makichie",
"STEYR Sport",
"Dynetrex Sol",
"OPTRON",
"BHClears Micro",
"Unfolded Circle",
"FactorySense",
"WearNex",
"PRADCO Outdoor",
"Zucchetti Axess",
"BLUEFIN DATA",
"Preseed Japan",
"Server Products",
"Embedded Sol",
"THERMY",
"Asahi Denso",
"GP Acoustics Intl",
"Tongfang Health",
"Deity Acoustic Tech",
"Schulte-Schlagbaum",
"WEST inx",
"Crossdoor",
"High Entropy",
"Herschel Infrared",
"PACIFIC MARINE",
"Alibaba (China)",
"AuthGate",
"Canyon Bicycles",
"Codie",
"NeuroPace",
"NexRev",
"Zhong Shan City",
"Shenzhen NEOECO",
"Nature",
"Guangzhou Honor",
"GGEC America",
"Gibson",
"VINYL MATT MEDIA",
"SHENZHEN BESTWAY",
"Lichens Innovation",
"Goerdyna",
"Relish Tech",
"Quintessential",
"CS INSTRUMENTS GmbH",
"DORAN MFG",
"Overhead Door",
"Kodira",
"ShenZhen",
"Shenzhen Zoqin Tech",
"Ceridwen",
"Sony Honda Mobility",
"Dynaudio",
"MAERSK CONTAINER",
"MA MICRO",
"shenzhen hongever",
"TAMRON",
"CAPTEMP LDA",
"Ambient Life",
"NOCTRIX HEALTH",
"RICKARD AIR",
"SG Armaturen",
"PIXEL TI IND. E COM",
"Core Devices",
"Fujita Electric",
"Willow Labs",
"BBC Bircher",
"ELLEA INGEGNERIA",
"Q42 Internet",
"Seaward Elec",
"Wuxi Does IOT",
"Monil",
"LS ELECTRIC",
"Shenzhen Cyber",
"Trackonomy Sys",
"IoT Sol Malta",
"Aseptico",
"Sensear",
"BLUEPROVIDERZ",
"Jano Life",
"PRIMES",
"Teledyne",
"Healthcare Tech",
"TRACERCO",
"HAPPLABS SOFTWARE",
"GE HEALTHCARE Tech",
"Olibra",
"Lumen Labs",
"SHENZHEN GWSTAI",
"SHENZHEN POWEROAK",
"Tech OVN Private",
"Lezyne USA",
"ANC CHINA",
"LION",
"Brightway",
"Hitachi Ind",
"Maennl Elektronik",
"IDEATRONIK Limited",
"caive",
"ShenZhen Doctors of",
"Deep and Steep",
"Endur ID",
"Freshape",
"PreEvnt",
"Oval",
"ADHD Friendly",
"RORENTECH",
"iKeyless",
"Shenzhen SuperSound",
"FLO SCIENCES",
"PalatiumCare",
"Edge Semi",
"ZIMMERMANN PV-Steel",
"Yolni",
"Ninebot (Changzhou)",
"Beijing Spring",
"Rokk",
"ONESPACE Tech (PTY)",
"Yuquan Semi",
"Easy Measure",
"Sichuan Changhong",
"Centromere Holding",
"Global Link",
"Amp Fit Israel",
"Trezor Company",
"KELLER",
"ElitEngineering",
"Vision",
"ClipsClips",
"BRITZ Intl",
"ThingCo",
"Opal Camera",
"Tech FOR FREERIDE",
"FLINTEC UK",
"TETNET",
"NINGBO SHARKWARD",
"Audeze",
"NODER Joint Stock",
"Schueco Intl",
"GILL INSTRUMENTS",
"Seitron",
"Southern Audio",
"SANWA NEWTEC",
"Mobile Tech Sol",
"Vitio Medical",
"Salyx Medical",
"Landig + Lava GmbH",
"Health Data Insight",
"BONX",
"LINKEDCHIP Tech",
"12mm Health Tech",
"MAVERICK ENERGY Sol",
"Tactica Defense",
"Huizhou Meicanxin",
"Lodestar Tech",
"IYO",
"Mitsubishi Motors",
"desamisCo",
"JL WORLD",
"Qulinda",
"Ningbo Dooya",
"G-Vision",
"Lantern Innovations",
"ebm-papst Mulfingen",
"Shenzhen Yunke",
"Fledt & Meiton",
"ContiTech",
"Sensoteq",
"Astute Access",
"Silverlake Tech",
"Shenzhen Guo-link",
"Silicon Vandals",
"Vibe Energy",
"shanghai fudan Elec",
"Micro Tech Services",
"Leapcraft",
"WUXI WEIDA Smart",
"ROBSON",
"HyolimXE",
"Kehwin Tech",
"WATTER",
"SafeNow",
"CZV",
"BIGREDBEE",
"White Eagle Sonic",
"Navico",
"Ancoe Industry",
"Roam Devices",
"Skywalk",
"Biogents",
"MODRETRO",
"iSi Wearable Safety",
"onanoff",
"TECHFLITTER Sol",
"PIRITIZ",
"FLORLINK",
"Keycafe",
"Sperry Labs",
"Senseonics",
"simatec",
"CROSS BRAIN",
"Also",
"Walter Mueller Inc",
"SHENZHEN GUANG QI",
"SKYROAM",
"Polk Audio",
"eQ-3",
"shenzhen Holyiot",
"Badass eBikes",
"Hydro Elec Devices",
"IN Phase Intl",
"Amon",
"Calpeda",
"KT Micro",
"Hangzhou Sciener",
"Lakecrest",
"Anacove",
"Kohler Ventures",
"STOREIO Tech",
"Innogando",
"A.Y. McDonald Mfg",
"ROYAL PARTS",
"JUREN",
"Biceek",
"SIGNUM INTELLIGENCE",
"MOBI7 TECNOLOGIA EM",
"Valostra Digital",
"Ethos",
"Aktiebolaget Ebeco",
"IDX",
"Dongguang Huasoo",
"SEIKOIST",
"SPX Aids to",
"Fitnexa",
"DEZINE",
"Eiritsu Elec",
"Hanshow Tech",
"WePower Tech",
"IRTrans",
"Hibino",
"Seiko Future",
"DP IOT",
"PALRED RETAIL",
"BIA NEUROSCIENCE",
"B.E.G. Brueck Elec",
"Nice",
"Avanos Medical",
"Ugreen",
"spheretek japan",
"Senti Tech Private",
"Ranch Sys",
"Suzhou Fanxi Tech",
"Geo Radar AI",
"Flitsmeister",
"The AI Toy",
"Capacite",
"Terasite Tech",
"General Resistance",
"Avivomed",
"PLAUD",
"Hooked Society",
"StoneDevices",
"Hangzhou Nano IC",
"TQ-Systems",
"Hapn",
"SITERWELL Elec",
"Shenzhen Mutual",
"Cosonic Smart Tech",
"Rollease Acmeda",
"ATANS Tech",
"Locus Robotics",
"TaigaIoT",
"Tridentify",
"Littlebird",
"Air Automotive",
"ISSPRO",
"Shenzhen Jimi loT",
"FulScience",
"Hypershell",
"ASD Lighting",
"Pharox",
"Eforthink Tech",
"Zirbel Bike",
"Legato Audio",
"Biocorp Production",
"Neurotherapeutics",
"Wimate Tech Sol Pvt",
"NOJA Power",
"Nestlab",
"Stark Future",
"Auranova",
"Domteknika",
"YDIIT",
"QRITAGYA",
"Telemacy",
"LIOTYS",
"Hondata",
"SIVA Inotec",
"REMDEVICE S.R.L",
"NIVELCO PROCESS",
"Beijing Hongsi Elec",
"FOSS Analytical",
"Blahaj",
"Nextorage",
"infyni",
"Raspberry Pi",
"Shenzhen Huasheng",
"Namara Water Tech",
"Shanghai Holychip",
"Rotarex",
"VOTRONIC",
"EXELIO S.R.L",
"Yamaha Motor eBike",
"Ferrari",
"Anhui Ubico",
"Mueller-BBM Rail",
"Yoto",
"LAUMAS Elettronica",
"BUND MEDIA",
"LSC Control Sys",
"Safety Lighting",
"Hangzhou Sneuro",
"UnlimitedIRL",
"CHOZEN Intl",
"Sensorworx",
"Romcor",
"NewNet",
"CLM-Solutions",
"RSA Security",
"Gravity",
"Shenzhen Shuye Tech",
"ArjoHuntleigh",
"Fairbanks Scales",
"YANMAR",
"Williams Sound",
"Zuuka",
"Chip-pump Micro",
"TOMOE VALVE",
"VCS Vision Control",
"Framason Audio",
"Field Line",
"Shearwater Research",
"HIGH HIT ENTERPRISE",
"Algiz",
"Shenzhen FHX",
"Hotron",
"Remoticom",
"OSKEY",
"GORDON MURRAY",
"Global Elec",
"Rokstar",
"STRYDE",
"Prosaris Sol",
"TLV",
"IZYBAT",
"ALTESSA Sol",
"Hong Kong Future",
"CenTrak",
"TM-TECHNOLOGIES JSC",
"ALPHADIF",
"Shenzhen Muyu Tech",
"augment AI",
"Digimax Innovative",
"CodaHD",
"E.J. Brooks",
"Martin.Care",
"IMPACT-BZ",
"Belford investment",
"Anton Paar",
"Knit Sound",
"Australis",
"AIR PRODUCTS AND",
"SYSTEM LOCO",
"24x8",
"FUTURE INTELLIGENCE",
"M3SH Tech",
"Matisse Interactif",
"Sesame AI",
"DEER MANAGEMENT Sys",
"Grayhill",
"PERUN TECH SP. Z",
"Tiny Displays",
"DEWINE Labs",
"Ind Computing",
"SkyCell",
"NAGANO KEIKI",
"COSEL ELEKTRONIK",
"BXB DIGITAL",
"Mindspire",
"Personal Products",
"CardiaMetrics",
"Aktv8",
"Knox Associates",
"smart-TEC GmbH &",
"Selco",
"MONNIT",
"Tensoli",
"KIBLE",
"terumo",
"SHIBAURA Elec",
"Shenzhen Hali-Power",
"inContAlert",
"flexlog",
"Audio Research",
"BQT Sol (NZ)",
"RAPIDISE Tech",
"Cebreo Medical",
"Lightnovo",
"Pedigree Tech",
"Zhejiang Wanyou",
"Mallow",
"Sigenergy Tech",
"ObjectSpectrum",
"ATsens",
"CoreHW Semi",
"Raycon",
"Hangzhou SDIC Micro",
"VSC Synapse",
"Senops Tracker",
"Shenzhen Xunzong",
"HOSEOTELNET",
"Solabs Tech",
"SpartanLync Tech",
"SHENZHEN CHINA",
"Fosilicon",
"Palarum",
"CYNC Labs",
"Noble HiFi",
"VERGESENSE",
"Harley-Davidson",
"Variowell",
"UNIVERSAL REMOTE",
"All Inspire Health",
"Minda",
"Shenzhen",
"SainStore Tech",
"TECH FASS s.r.o",
"OPICA",
"Cardinal Scale Mfg",
"Piscisea Tech",
"CHIYODA TECHNOL",
"mylife Diabetes",
"Tractian Tech",
"FairPhone B.V",
"The90",
"Linde",
"Kumho Tire",
"Inito Health",
"Tempur World",
"Band24",
"Acer",
"Hehku",
"AGCO",
"Victor Hasselblad",
"IntelliDesign",
"UltronSMART",
"Arashi Vision",
"Westfalen",
"Liebherr-Hausgeraet",
"SKAIChips",
"SZ Avinox",
"Spectra Precision",
"Triumph Designs",
"Gastron",
"Hawkeye Surgical",
"Arnold & Richter",
"Neurable",
"CAREHAWK",
"Vybe Audio",
"Smart Tech ULC",
"Durin",
"KWANG YANG MOTOR",
"Placenet Internet",
"Liontron GmbH &",
"Hankook Tire & Tech",
"Guangdong Youhong",
"Fontaine Fifth",
"ZELP",
"BRITA",
"Shanghai XYLink",
"Keto-Check",
"RFID N PRINT",
"Ralston Instruments",
"CHIGEE Tech",
"EURODIMA GmbH &",
"SUPERARK",
"Shenzhen DeepX Tech",
"Pressure Sys Intl",
"HEX INNOVATE (UK)",
"IONLINE INTERNET",
"Even Realities",
"QPlay",
"Hydratune Australia",
"CITECH",
"Neurabody",
"Daisen Elec Ind",
"Sonus Faber",
"miThings",
"Nearfaces UG",
"Temple Private",
"BRISANT SECURE",
"Daiichi Elektronik",
"Cirrus Design",
"Flicfit",
"Optrel",
"BEMER Int",
"Hamodaz",
"OnRace Motorsports",
"SIGMA",
"rescuetrack",
"ARETA AI",
"Gigawit Elec",
"Nylint",
"Hamaton",
"RAPSODO",
"JustTec",
"ONXMAPS",
"Metabowerke",
"Thermia Aktiebolag",
"ALLRA",
"Bluetooth SIG"
};
// clang-format on
static const int BT_CO_N = (int)(sizeof(BT_CO_ID) / sizeof(BT_CO_ID[0]));
// binary search; nullptr when the number is not assigned
static const char* btCompanyName(uint16_t id) {
  int lo = 0, hi = BT_CO_N - 1;
  while (lo <= hi) {
    int mid = (lo + hi) >> 1;
    if (BT_CO_ID[mid] == id)
      return BT_CO_NAME[mid];
    if (BT_CO_ID[mid] < id)
      lo = mid + 1;
    else
      hi = mid - 1;
  }
  return nullptr;
}
static bool btIcontains(const char* hay, const char* needle) {
  if (!hay || !needle || !*needle)
    return false;
  size_t n = strlen(needle);
  for (const char* p = hay; *p; p++)
    if (strncasecmp(p, needle, n) == 0)
      return true;
  return false;
}
static bool btNameHas(const char* name, const char* const* list, int n) {
  if (!name || !name[0])
    return false;
  for (int i = 0; i < n; i++)
    if (btIcontains(name, list[i]))
      return true;
  return false;
}
static const char* const BT_LIGHT_NAMES[] = {
    "govee",       "ihoment",     "hue ",      "hue_",     "hue-",       "philips hue", "elk-",
    "melk",        "bledom",      "bledob",    "ledble",   "lednet",     "triones",     "magic light",
    "magiclight",  "duoco",       "sp110e",    "sp107e",   "yeelight",   "yldp",        "nanoleaf",
    "lifx",        "wizlight",    "smartlife", "sengled",  "lutron",     "led strip",   "ledstrip",
    "light strip", "strip light", "led light", "led bulb", "smart bulb", "bulb",        "lamp"};
static const char* const BT_WEAR_NAMES[] = {
    "smartwatch", "watch",     "fitbit",     "garmin",   "forerunner", "fenix",   "vivoactive",
    "vivosmart",  "vivofit",   "venu",       "instinct", "amazfit",    "mi band", "mi smart band",
    "smart band", "smartband", "galaxy fit", "gear fit", "gear s",     "suunto",  "coros",
    "whoop",      "oura",      "polar ",     "tickr",    "hrm-",       "hrm ",    "heart rate"};
// Garmin, Huami, Polar x2, Suunto, Withings, Oura x2, Jawbone, Misfit, Pebble, Wahoo, Meta Watch, Timex, Casio
static const uint16_t BT_WEAR_CIDS[] = {0x0087, 0x0157, 0x006B, 0x00D1, 0x009F, 0x03FF, 0x02B2, 0x0304,
                                        0x008A, 0x00DF, 0x0154, 0x01FC, 0x00A3, 0x00D6, 0x0178};
// Signify, Philips, OSRAM, LEDVANCE, OPPLE, WiZ, Nanoleaf, Sengled, Lutron, NEC Lighting, Acuity, Lunera, LSG, Mode
static const uint16_t BT_LIGHT_CIDS[] = {0x060F, 0x01DD, 0x050C, 0x052E, 0x0539, 0x0745, 0x080B,
                                         0x08B4, 0x04DE, 0x0095, 0x0346, 0x0528, 0x0573, 0x0661};
static bool btCidIn(const uint16_t* list, int n, uint16_t cid) {
  for (int i = 0; i < n; i++)
    if (list[i] == cid)
      return true;
  return false;
}
// One AD structure of the given type (first match). Returns the data after the type byte.
static bool btAd(const uint8_t* pl, int pn, uint8_t type, const uint8_t** d, int* dl, int skip = 0) {
  int seen = 0;
  for (int i = 0; pl && i + 1 < pn;) {
    int l = pl[i];
    if (!l || i + 1 + l > pn)
      break;
    if (pl[i + 1] == type) {
      if (seen++ == skip) {
        *d = pl + i + 2;
        *dl = l - 1;
        return true;
      }
    }
    i += l + 1;
  }
  return false;
}
// service data for a 16-bit UUID: bytes after the UUID
static bool btServiceData(const uint8_t* pl, int pn, uint16_t uuid, const uint8_t** d, int* dl) {
  for (int k = 0; k < 6; k++) {
    const uint8_t* x;
    int xl;
    if (!btAd(pl, pn, 0x16, &x, &xl, k))
      return false;
    if (xl >= 2 && (uint16_t)(x[0] | (x[1] << 8)) == uuid) {
      *d = x + 2;
      *dl = xl - 2;
      return true;
    }
  }
  return false;
}
static bool btHasService16(const uint8_t* pl, int pn, uint16_t uuid) {  // in a 16-bit UUID list or as service data
  for (int t = 0x02; t <= 0x03; t++) {
    const uint8_t* x;
    int xl;
    if (btAd(pl, pn, (uint8_t)t, &x, &xl))
      for (int i = 0; i + 1 < xl; i += 2)
        if ((uint16_t)(x[i] | (x[i + 1] << 8)) == uuid)
          return true;
  }
  const uint8_t* d;
  int dl;
  return btServiceData(pl, pn, uuid, &d, &dl);
}
// Eddystone-URL frame -> readable host/path without the scheme
static void btEddyUrl(const uint8_t* d, int dl, char* out, int cap) {
  static const char* const EXP[14] = {".com/", ".org/", ".edu/", ".net/", ".info/", ".biz/", ".gov/",
                                      ".com",  ".org",  ".edu",  ".net",  ".info",  ".biz",  ".gov"};
  int o = 0;
  // d[0] frame type, d[1] tx power, d[2] scheme
  for (int i = 2; i < dl && o < cap - 1; i++) {
    if (i == 2)
      continue;
    uint8_t ch = d[i];
    if (ch < 14) {
      const char* e = EXP[ch];
      while (*e && o < cap - 1)
        out[o++] = *e++;
    } else if (ch >= 0x21 && ch <= 0x7E)
      out[o++] = (char)ch;
    else
      break;
  }
  out[o] = 0;
}
// Returns true when something specific was recognised. *kind is one of:
// FINDHUB (a tracker, family TAG), IBEACON, EDDYSTONE, ALTBEACON, FASTPAIR, MESH, LIGHT, WEARABLE.  shown = a short description for unnamed devices.
static bool btClassify(const uint8_t* pl, int pn, const uint8_t* md, int mn, uint16_t cid, const char* name,
                       Family* fam, Conf* conf, const char** kind, char* shown, int cap) {
  *fam = FAM_BT;
  *conf = CONF_LOW;
  *kind = nullptr;
  shown[0] = 0;
  const uint8_t* sd;
  int sl;
  // 0xFEAA = Eddystone UUID, reused by Google's Find Hub trackers
  if (btServiceData(pl, pn, 0xFEAA, &sd, &sl) && sl >= 1) {
    uint8_t ft = sd[0];
    if (ft == 0x40 || ft == 0x41) {
      *fam = FAM_TAG;
      *conf = CONF_MED;
      *kind = "FINDHUB";
      snprintf(shown, cap, "Find Hub tag");
      return true;
    }
    if (ft == 0x00 || ft == 0x10 || ft == 0x20 || ft == 0x30) {
      *kind = "EDDYSTONE";
      if (ft == 0x00 && sl >= 14)
        snprintf(shown, cap, "Eddy UID %02X%02X%02X%02X", sd[2], sd[3], sd[4], sd[5]);
      else if (ft == 0x10 && sl >= 4) {
        char u[20];
        btEddyUrl(sd, sl, u, sizeof(u));
        snprintf(shown, cap, "%s", u[0] ? u : "Eddy URL");
      } else if (ft == 0x20)
        snprintf(shown, cap, "Eddy TLM");
      else if (ft == 0x30)
        snprintf(shown, cap, "Eddy EID");
      else
        snprintf(shown, cap, "Eddystone");
      return true;
    }
  }
  // Apple iBeacon: uuid(16) major(2) minor(2) power(1)
  if (md && mn >= 25 && cid == 0x004C && md[2] == 0x02 && md[3] == 0x15) {
    *kind = "IBEACON";
    snprintf(shown, cap, "iB %02X%02X %u/%u", md[4], md[5], (unsigned)((md[20] << 8) | md[21]),
             (unsigned)((md[22] << 8) | md[23]));
    return true;
  }
  // AltBeacon: same layout after the 0xBEAC code
  if (md && mn >= 26 && md[2] == 0xBE && md[3] == 0xAC) {
    *kind = "ALTBEACON";
    snprintf(shown, cap, "Alt %02X%02X %u/%u", md[4], md[5], (unsigned)((md[20] << 8) | md[21]),
             (unsigned)((md[22] << 8) | md[23]));
    return true;
  }
  const uint8_t* ap;
  int al;
  int appCat = -1;
  // GAP appearance: category = value >> 6
  if (btAd(pl, pn, 0x19, &ap, &al) && al >= 2)
    appCat = (int)((uint16_t)(ap[0] | (ap[1] << 8)) >> 6);
  if (appCat == 22 || appCat == 31 || btHasService16(pl, pn, 0xFE0F) ||
      btCidIn(BT_LIGHT_CIDS, sizeof(BT_LIGHT_CIDS) / 2, cid) ||
      btNameHas(name, BT_LIGHT_NAMES, sizeof(BT_LIGHT_NAMES) / sizeof(BT_LIGHT_NAMES[0]))) {
    *kind = "LIGHT";
    snprintf(shown, cap, "Smart light");
    return true;
  }
  if (appCat == 3 || appCat == 13 || btCidIn(BT_WEAR_CIDS, sizeof(BT_WEAR_CIDS) / 2, cid) ||
      btNameHas(name, BT_WEAR_NAMES, sizeof(BT_WEAR_NAMES) / sizeof(BT_WEAR_NAMES[0]))) {
    *kind = "WEARABLE";
    snprintf(shown, cap, "Wearable");
    return true;
  }
  // Google Fast Pair: earbuds and other accessories, not a tracker by itself
  if (btHasService16(pl, pn, 0xFE2C)) {
    *kind = "FASTPAIR";
    snprintf(shown, cap, "Fast Pair");
    return true;
  }
  if (btHasService16(pl, pn, 0x1827) || btHasService16(pl, pn, 0x1828)) {
    *kind = "MESH";
    snprintf(shown, cap, "BT Mesh");
    return true;
  }
  return false;
}
// the extra type tags that replace the plain "BT" / "WIFI" label in lists
static bool btKindTag(const char* m) {
  static const char* const K[] = {"IBEACON", "EDDYSTONE", "ALTBEACON", "FASTPAIR",
                                  "MESH",    "LIGHT",     "WEARABLE",  "SKIMMER", "FLIPPER"};
  if (!m)
    return false;
  for (int i = 0; i < 9; i++)
    if (strcmp(m, K[i]) == 0)
      return true;
  return false;
}
// names that the cheap serial Bluetooth modules used in card skimmers broadcast when nobody changed their settings (HC-05/06 are the classic ones, HC-08 is the BLE one, RNBT-xxxx is a Roving Networks default)
static bool skimmerName(const char* n) {
  if (!n)
    return false;
  while (*n == ' ')
    n++;
  char b[16];
  int k = 0;
  for (; *n && k < 15; n++)
    b[k++] = (char)toupper((unsigned char)*n);
  if (*n)
    return false;  // longer than any of these names
  while (k > 0 && b[k - 1] == ' ')
    k--;
  b[k] = 0;
  static const char* const N[] = {"HC-03", "HC-05", "HC-06", "HC-08", "HC05", "HC06", "LINVOR"};
  for (int i = 0; i < 7; i++)
    if (strcmp(b, N[i]) == 0)
      return true;
  if (k == 9 && strncmp(b, "RNBT-", 5) == 0) {
    for (int i = 5; i < 9; i++)
      if (!isxdigit((unsigned char)b[i]))
        return false;
    return true;
  }
  return false;
}
// Wi-Fi smart bulbs that host their own network (set-up mode) or use a recognisable name
static bool wifiLightSsid(const char* s) {
  static const char* const L[] = {
      "wiz_",       "lifx",      "kasa_smart bulb", "tp-link_smart bulb", "yeelink-light", "yeelight", "smartlife",
      "smart life", "magichome", "lednet",          "nanoleaf",           "govee",         "hue"};
  if (!s || !s[0])
    return false;
  for (int i = 0; i < 13; i++)
    if (btIcontains(s, L[i]))
      return true;
  return false;
}
static void IRAM_ATTR sniffCb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA)
    return;
  if (!armedMask || (!scanning && !hunting))
    return;
  sniffCalls++;
  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t* p = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;
  if (len < 24)
    return;
  framesSeen++;
  uint8_t ftype = p[0] & 0xFC;
  const uint8_t* a1 = p + 4;
  const uint8_t* a2 = p + 10;
  int8_t rssi = pkt->rx_ctrl.rssi;
  uint8_t ch = pkt->rx_ctrl.channel;
  char ssid[33] = {0};
  bool wild = false;
  if (ftype == 0x40 && len >= 26) {
    uint8_t sl = p[25];
    if (sl == 0)
      wild = true;
    else if (sl < 33 && 26 + sl <= len) {
      memcpy(ssid, p + 26, sl);
      ssid[sl] = 0;
    }
  } else if ((ftype == 0x80 || ftype == 0x50) && len >= 38) {
    uint8_t sl = p[37];
    if (sl > 0 && sl < 33 && 38 + sl <= len) {
      memcpy(ssid, p + 38, sl);
      ssid[sl] = 0;
    }
  }
  // a beacon / probe response states the channel the network REALLY uses (DS Parameter Set);
  uint8_t chTrust = 0;
  // the radio's own channel is only where we happened to be listening: a strong network is also heard 1-2 channels away
  if ((ftype == 0x80 || ftype == 0x50) && len >= 44) {
    for (int i = 36; i + 2 <= len - 4;) {
      uint8_t id = p[i], l = p[i + 1];
      if (i + 2 + l > len)
        break;
      if (id == 3 && l == 1) {
        uint8_t d = p[i + 2];
        if (d >= 1 && d <= 14)
          chTrust = (uint8_t)(d | 0x80);
        break;
      }
      i += 2 + l;
    }
  }
  auto consider = [&](const uint8_t* mac, const char* how) {
    if ((mac[0] & 0x01) || mac[0] == 0xFF)
      return;
    Family fam = FAM_UNKNOWN;
    Conf conf = CONF_LOW;
    const char* tag = "OUI";
    bool hit = ouiMatch(WIFI_OUI, WIFI_OUI_N, mac, &fam, &conf, &tag);
    if (!hit)
      hit = ouiMatchX(WIFI_OUIX, WIFI_OUIX_N, mac, &fam, &conf, &tag);
    Family sf;
    Conf sc;
    const char* ptag = nullptr;
    if (textMatch(SSID_RULE, SSID_RULE_N, ssid, &sf, &sc)) {
      fam = sf;
      if (sc > conf)
        conf = sc;
      hit = true;
      tag = "SSID";
    } else if (patMatch(SSID_PAT, SSID_PAT_N, ssid, &sf, &sc, &ptag)) {
      fam = sf;
      if (sc > conf || !hit)
        conf = sc;
      hit = true;
      tag = ptag;
    }
    if (wild && hit && fam == FAM_FLOCK && conf < CONF_HIGH)
      conf = CONF_HIGH;
    if (!hit) {
      Family cf;
      Conf cc;
      if (customMatchWifi(mac, ssid, &cf, &cc)) {
        fam = cf;
        conf = cc;
        hit = true;
        tag = "CUSTOM";
      }
    }
    if (!hit && catOn[CAT_WIFI] && ssid[0] && (ftype == 0x80 || ftype == 0x50)) {
      fam = FAM_WIFI;
      conf = CONF_LOW;
      hit = true;
      tag = wifiLightSsid(ssid) ? "LIGHT" : "AP";
    }
    if (!hit)
      return;
    if (localAdmin(mac) && fam != FAM_FLOCK && fam != FAM_GUNSHOT && fam != FAM_AXON && fam != FAM_BODYCAM && fam != FAM_GLASS && fam != FAM_DRONE &&
        fam != FAM_WIFI)
      return;
    upsert(mac, fam, conf, RAD_WIFI, rssi, (mac == a2 && chTrust) ? chTrust : ch, ssid[0] ? ssid : tag,
           (fam == FAM_WIFI && wifiLightSsid(ssid)) ? "LIGHT" : how);
  };
  // Remote ID over Wi-Fi: vendor element FA-0B-BC type 0x0D in a beacon
  if (ftype == 0x80 && len >= 44) {
    for (int i = 36; i + 2 <= len - 4;) {
      uint8_t id = p[i], l = p[i + 1];
      if (i + 2 + l > len)
        break;
      if (id == 221 && l >= 7 && p[i + 2] == 0xFA && p[i + 3] == 0x0B && p[i + 4] == 0xBC && p[i + 5] == 0x0D) {
        RidInfo rid;
        memset(&rid, 0, sizeof rid);
        if (ridDecode(p + i + 7, l - 5, &rid) && !(a2[0] & 0x01)) {  // skip the message counter byte
          upsert(a2, FAM_DRONE, CONF_HIGH, RAD_WIFI, rssi, chTrust ? chTrust : ch, rid.id[0] ? rid.id : "RemoteID",
                 "RID", &rid);
          return;
        }
      }
      i += 2 + l;
    }
  }
  consider(a2, wild ? "WILDPROBE" : ((ftype == 0x80 || ftype == 0x50) ? "AP" : "TX"));
  if (ftype != 0x80 && ftype != 0x50)
    consider(a1, "RX");
}
// The Bluetooth stack calls back on a task with a very small stack, so the callback only copies
// the raw advertisement into a queue. loop() decodes the queue (bleDrain) with the full stack.
static BleAdv bleQ[BLE_Q_N];
static volatile uint8_t bleQHead = 0, bleQTail = 0;
static volatile uint32_t bleQDrop = 0;
// Flipper Zero, from its own firmware (gap.c, serial_profile.c, furi_hal_version.c) and the open detectors (ESP32 Marauder, GhostESP):
//  - service UUID 0x3080 | case colour: 0x3081 black, 0x3082 white, 0x3083 transparent (0x3080 = colour not set)
//  - the advertised name is "Flipper " + the unit's name (the owner can change it with some firmware)
//  - the Bluetooth address is built from the STM32WB55's ST company id + device id: 80:E1:26:xx:xx:xx (or 80:E1:27). Other products on
//    the same chip can share it, so on its own it is only a weak hint
// Returns the Flipper UUID found in the advert's UUID lists (16-bit, 32-bit, or 128-bit in the Bluetooth base form; partial or complete), else 0.
static uint16_t flipperUuid(const uint8_t* pl, int n) {
  // the Bluetooth base UUID 0000xxxx-0000-1000-8000-00805F9B34FB, least significant byte first, bytes 0..11 (xxxx follows)
  static const uint8_t BASE_LE[12] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00};
  uint16_t best = 0;
  for (int i = 0; i + 1 < n;) {
    int l = pl[i];
    if (!l || i + 1 + l > n)
      break;
    uint8_t t = pl[i + 1];
    const uint8_t* d = pl + i + 2;
    int dl = l - 1;
    int step = (t == 0x02 || t == 0x03) ? 2 : ((t == 0x04 || t == 0x05) ? 4 : ((t == 0x06 || t == 0x07) ? 16 : 0));
    for (int k = 0; step && k + step <= dl; k += step) {
      uint16_t u = 0;
      if (step == 2)
        u = (uint16_t)(d[k] | (d[k + 1] << 8));
      else if (step == 4 && d[k + 2] == 0 && d[k + 3] == 0)
        u = (uint16_t)(d[k] | (d[k + 1] << 8));
      else if (step == 16 && memcmp(d + k, BASE_LE, 12) == 0 && d[k + 14] == 0 && d[k + 15] == 0)
        u = (uint16_t)(d[k + 12] | (d[k + 13] << 8));
      if (u >= 0x3080 && u <= 0x3083 && u > best)
        best = u;
    }
    i += l + 1;
  }
  return best;
}
// the Bluetooth address of a Flipper's STM32WB55 (80:E1:26 or 80:E1:27; mac[0] is the first byte as written)
static bool flipperMac(const uint8_t* mac) {
  return mac[0] == 0x80 && mac[1] == 0xE1 && (mac[2] == 0x26 || mac[2] == 0x27);
}
// "Flipper" anywhere in the name, any case
static bool flipperName(const char* n) {
  for (; *n; n++)
    if (tolower((unsigned char)n[0]) == 'f' && strncasecmp(n, "flipper", 7) == 0)
      return true;
  return false;
}
class BleCb : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* d) override {
    bleSeen++;
    if (!armedMask || (!scanning && !hunting))
      return;
    uint8_t h = bleQHead, nx = (uint8_t)((h + 1) % BLE_Q_N);

    if (nx == bleQTail) {
      bleQDrop++;
      return;
    }
    BleAdv& a = bleQ[h];
    const uint8_t* v = d->getAddress().getVal();
    for (int i = 0; i < 6; i++)
      a.mac[i] = v[5 - i];
    a.rssi = d->getRSSI();
    const std::vector<uint8_t>& p = d->getPayload();
    size_t n = p.size();
    if (n > BLE_PAY_MAX)
      n = BLE_PAY_MAX;
    if (n)
      memcpy(a.pay, p.data(), n);
    a.plen = (uint8_t)n;
    __sync_synchronize();
    bleQHead = nx;
  }
};
static void bleProcess(const BleAdv& adv) {
  uint8_t mac[6];
  memcpy(mac, adv.mac, 6);
  Family fam = FAM_UNKNOWN;
  Conf conf = CONF_LOW;
  const char* tag = "BLE";
  bool hit = ouiMatch(BLE_OUI, BLE_OUI_N, mac, &fam, &conf, &tag);
  if (!hit)
    hit = ouiMatchX(BLE_OUIX, BLE_OUIX_N, mac, &fam, &conf, &tag);
  String name = adv.getName();
  uint16_t cid = 0;
  uint8_t mtype = 0;
  const char* vend = nullptr;
  // manufacturer data bytes (company id first), for beacon recognition
  uint8_t mdb[40];
  int mdn = 0;
  if (adv.haveManufacturerData()) {
    String md = adv.getManufacturerData();
    mdn = md.length() < (int)sizeof(mdb) ? (int)md.length() : (int)sizeof(mdb);
    for (int q = 0; q < mdn; q++)
      mdb[q] = (uint8_t)md[q];
    if (md.length() >= 2) {
      cid = (uint8_t)md[0] | ((uint16_t)(uint8_t)md[1] << 8);
      if (md.length() >= 3)
        mtype = (uint8_t)md[2];
      if (cid == 0x004C)
        vend = "Apple";
      else if (cid == 0x0075)
        vend = "Samsung";
      else if (cid == 0x0006)
        vend = "MSFT";
      else if (cid == 0x00E0)
        vend = "Google";
      // Tile, Inc. (0x00B3 is a different company)
      else if (cid == 0x067C)
        vend = "Tile";
    }
  }
  char shown[20] = {0};
  const char* coName = cid ? btCompanyName(cid) : nullptr;  // maker name from the Bluetooth SIG list
  if (name.length() > 0)
    strncpy(shown, name.c_str(), 19);
  else if (coName)
    strncpy(shown, coName, 19);
  else if (vend)
    strncpy(shown, vend, 19);
  else if (cid)
    snprintf(shown, 19, "CID %04X", cid);
  else
    snprintf(shown, 19, "%02X%02X%02X", mac[3], mac[4], mac[5]);
  Family nf;
  Conf nc;
  const char* ntag = nullptr;
  if (textMatch(BLE_NAME, BLE_NAME_N, name.c_str(), &nf, &nc)) {
    fam = nf;
    conf = nc;
    hit = true;
    tag = "BNAME";
  } else if (patMatch(BLE_PAT, BLE_PAT_N, name.c_str(), &nf, &nc, &ntag)) {
    fam = nf;
    conf = nc;
    hit = true;
    tag = ntag;
  }
  if (adv.isAdvertisingService(BleU16((uint16_t)0xFE6C))) {
    fam = FAM_AXON;
    conf = CONF_HIGH;
    hit = true;
    tag = "FE6C";
  }
  if (adv.isAdvertisingService(BleU16((uint16_t)0xFD5F))) {
    fam = FAM_GLASS;
    conf = CONF_HIGH;
    hit = true;
    tag = "FD5F";
  }
  if (adv.isAdvertisingService(BleU16((uint16_t)0xFEED)) || adv.isAdvertisingService(BleU16((uint16_t)0xFEEC))) {
    fam = FAM_TAG;
    conf = CONF_HIGH;
    hit = true;
    tag = "TILE";
    if (!name.length())
      strncpy(shown, "Tile", 19);
  }
  if (adv.isAdvertisingService(BleU16((uint16_t)0xFD5A))) {
    fam = FAM_TAG;
    conf = CONF_HIGH;
    hit = true;
    tag = "STAG";
    if (!name.length())
      strncpy(shown, "SmartTag", 19);
  }
  if (adv.isAdvertisingService(BleU16((uint16_t)0xFFFA))) {
    fam = FAM_DRONE;
    conf = CONF_HIGH;
    hit = true;
    tag = "RID";
    if (!name.length())
      strncpy(shown, "RemoteID", 19);
  }
  if (adv.isAdvertisingService(BleU16((uint16_t)0xFFE0))) {
    fam = FAM_DRONE;
    conf = CONF_MED;
    hit = true;
    tag = "DJI-BLE";
    if (!name.length())
      strncpy(shown, "DJI", 19);
  }
  for (int i = 0; i < BLE_UUID_N; i++)
    if (adv.isAdvertisingService(BleU16((uint16_t)BLE_UUID[i].id))) {
      fam = BLE_UUID[i].fam;
      conf = BLE_UUID[i].conf;
      hit = true;
      tag = BLE_UUID[i].tag;
      if (!name.length())
        strncpy(shown, BLE_UUID[i].tag, 19);
    }
    // Raven (Flock's gunshot sensor): two or more of its five custom services together is a firm sign, one alone is medium.
    // The older 1.1.x firmware advertised the Health and Location services instead; both together is only a hint.
    {
      int rv = 0;
      for (uint16_t u = 0x3100; u <= 0x3500; u += 0x100)
        if (adv.isAdvertisingService(BleU16(u)))
          rv++;
      if (rv >= 2) {
        fam = FAM_GUNSHOT;
        conf = CONF_HIGH;
        hit = true;
        tag = "RAVEN";
        if (!name.length())
          strncpy(shown, "Raven", 19);
      } else if (!hit && adv.isAdvertisingService(BleU16((uint16_t)0x1809)) &&
                 adv.isAdvertisingService(BleU16((uint16_t)0x1819))) {
        fam = FAM_GUNSHOT;
        conf = CONF_LOW;
        hit = true;
        tag = "RAVEN-OLD";
        if (!name.length())
          strncpy(shown, "Raven 1.1?", 19);
      }
    }
  // ASCII inside the service data (Axon body camera, MAC-agnostic)
  if (adTextFind(adv.getPayload(), adv.getPayloadLength(), "BWCDEVICE")) {
    fam = FAM_AXON;
    conf = CONF_HIGH;
    hit = true;
    tag = "BWC";
    if (!name.length())
      strncpy(shown, "Axon body cam", 19);
  }
  // Find My (offline finding); type 0x10 is 'Nearby Info' that every iPhone/iPad/watch sends
  if (cid == 0x004C && mtype == 0x12) {
    fam = FAM_TAG;
    conf = CONF_HIGH;
    hit = true;
    tag = "FIND";
    if (!name.length())
      strncpy(shown, "Apple Find", 19);
  }
  if (cid == 0x067C) {
    fam = FAM_TAG;
    conf = CONF_HIGH;
    hit = true;
    tag = "TILE";
    if (!name.length())
      strncpy(shown, "Tile", 19);
  }
  if (cid) {
    for (int i = 0; i < BLE_CID_N; i++) {
      if (BLE_CID[i].id == cid) {
        if (!(fam == FAM_TAG && BLE_CID[i].fam == FAM_GLASS)) {
          fam = BLE_CID[i].fam;
          if (BLE_CID[i].conf > conf)
            conf = BLE_CID[i].conf;
          hit = true;
          tag = BLE_CID[i].tag;
        }
        break;
      }
    }
  }
  // beats every other reading of the same advert (a BLE serial module also looks like a drone: service 0xFFE0)
  if (skimWatch && skimmerName(name.c_str())) {
    fam = FAM_BT;
    conf = CONF_MED;
    hit = true;
    tag = "SKIMMER";
  }
  // Flipper Zero (see flipperUuid): its case-colour service is the strong sign, the name is a medium one, the address prefix a weak one
    {
      uint16_t fu = flipperUuid(adv.getPayload(), adv.getPayloadLength());
      for (uint16_t u = 0x3083; !fu && u >= 0x3080; u--)
        if (adv.isAdvertisingService(BleU16(u)))
          fu = u;
      bool fName = flipperName(name.c_str()), fMac = flipperMac(mac);
      if (fu || fName || (fMac && !hit)) {
        int score = (fu >= 0x3081 ? 3 : (fu ? 2 : 0)) + (fName ? 2 : 0) + (fMac ? 1 : 0);
        fam = FAM_BT;
        conf = score >= 3 ? CONF_HIGH : (score == 2 ? CONF_MED : CONF_LOW);
        hit = true;
        tag = "FLIPPER";
        if (!name.length()) {
          const char* fs = fu == 0x3081 ? "Flipper (black)" : (fu == 0x3082 ? "Flipper (white)" : (fu == 0x3083 ? "Flipper (clear)" : (fu ? "Flipper Zero" : "Flipper?")));
          memset(shown, 0, sizeof(shown));
          strncpy(shown, fs, 19);
        }
      }
    }
  if (!hit) {
    Family cf;
    Conf cc;
    if (customMatchBle(mac, name.c_str(), cid, &cf, &cc)) {
      fam = cf;
      conf = cc;
      hit = true;
      tag = "CUSTOM";
    }
  }
  const char* btKind = nullptr;
  char btShown[20] = {0};
  // recognise beacons, wearables, lights, Fast Pair, Find Hub trackers
  {
    Family bf;
    Conf bc;
    const char* bk = nullptr;
    char bs[20];
    if (btClassify(adv.getPayload(), adv.getPayloadLength(), mdb, mdn, cid, name.c_str(), &bf, &bc, &bk, bs,
                   sizeof(bs))) {
      if (bf == FAM_TAG) {
        if (!hit) {
          fam = FAM_TAG;
          conf = bc;
          hit = true;
          tag = bk;
          if (!name.length())
            strncpy(shown, bs, 19);
        }
      } else {
        btKind = bk;
        strncpy(btShown, bs, 19);
      }
    }
  }
  if (!hit && catOn[CAT_BT] && (name.length() > 0 || cid || vend || btKind)) {
    fam = FAM_BT;
    conf = CONF_LOW;
    hit = true;
    tag = btKind ? btKind : "BT";
    if (btKind && !name.length()) {
      strncpy(shown, ((coName && (!strcmp(btKind, "LIGHT") || !strcmp(btKind, "WEARABLE"))) ? coName : btShown), 19);
      shown[19] = 0;
    }
  }
  if (!hit)
    return;
  RidInfo rid;
  memset(&rid, 0, sizeof rid);
  bool haveRid = false;
  // Remote ID over Bluetooth: service data, UUID 0xFFFA, app code 0x0D
  if (fam == FAM_DRONE && adv.getPayloadLength() > 6) {
    const uint8_t* pl = adv.getPayload();
    int n = adv.getPayloadLength();
    for (int i = 0; i + 1 < n;) {
      int l = pl[i];
      if (!l || i + 1 + l > n)
        break;
      if (pl[i + 1] == 0x16 && l >= 6 && pl[i + 2] == 0xFA && pl[i + 3] == 0xFF && pl[i + 4] == 0x0D) {
        haveRid = ridDecode(pl + i + 6, l - 5, &rid);
        break;
      }
      i += l + 1;
    }
  }
  upsert(mac, fam, conf, RAD_BLE, (int8_t)adv.getRSSI(), 0, (haveRid && rid.id[0]) ? rid.id : shown, tag,
         haveRid ? &rid : nullptr);
}
static BleCb bleCb;
static void bleDrain() {
  for (int k = 0; k < 12 && bleQTail != bleQHead; k++) {
    BleAdv a = bleQ[bleQTail];
    __sync_synchronize();
    bleQTail = (uint8_t)((bleQTail + 1) % BLE_Q_N);
    if (!armedMask || (!scanning && !hunting))
      continue;
    bleProcess(a);
  }
}
// ---- 802.15.4 (Zigbee + Thread) receive path -------------------------------
static const uint8_t ZB_CHANS[] = {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};
static const int ZB_CHANS_N = 16;
static int zbChanIdx = 0;
static bool zbOn = false;
#ifdef HAS_IEEE154
static volatile int zbHead = 0, zbTail = 0;
static ZbHit zbQ[16];
extern "C" void IRAM_ATTR esp_ieee802154_receive_done(uint8_t* frame, esp_ieee802154_frame_info_t* info) {
  if (!frame || !info)
    return;
  uint8_t len = frame[0];
  if (len >= 5 && armedMask && (scanning || hunting)) {
    int n = (zbHead + 1) & 15;
    if (n != zbTail) {
      uint8_t copy = len + 1;
      if (copy > 48)
        copy = 48;
      for (uint8_t i = 0; i < copy; i++)
        zbQ[zbHead].raw[i] = frame[i];
      zbQ[zbHead].rawLen = copy;
      zbQ[zbHead].rssi = info->rssi;
      zbQ[zbHead].ch = ZB_CHANS[zbChanIdx % ZB_CHANS_N];
      zbHead = n;
    }
  }
  esp_ieee802154_receive_handle_done(frame);
}
extern "C" void IRAM_ATTR esp_ieee802154_receive_failed(uint16_t e) {
  (void)e;
}
static void zbStart() {
  if (zbOn)
    return;
  esp_ieee802154_enable();
  esp_ieee802154_set_promiscuous(true);
  esp_ieee802154_set_rx_when_idle(true);
  zbChanIdx = 0;
  esp_ieee802154_set_channel(ZB_CHANS[0]);
  esp_ieee802154_receive();
  zbOn = true;
}
static void zbStop() {
  if (!zbOn)
    return;
  // stop receiving and put the radio to sleep BEFORE the module is disabled,
  esp_ieee802154_set_rx_when_idle(false);
  // so Wi-Fi gets the radio back in a clean state
  esp_ieee802154_set_promiscuous(false);
  esp_ieee802154_sleep();
  esp_ieee802154_disable();
  zbOn = false;
}
#else
static void zbStart() {
  zbOn = true;
}
static void zbStop() {
  zbOn = false;
}
#endif
// A PAN keeps its identity: once one frame proves it is Zigbee or Thread, ACK-ish / ambiguous frames inherit it.
struct PanKind {
  uint16_t pan;
  uint8_t ch;
  Family fam;
  uint32_t at;
};
static PanKind panK[10];
static Family panLookup(uint16_t pan, uint8_t ch) {
  for (int i = 0; i < 10; i++)
    if (panK[i].fam != FAM_UNKNOWN && panK[i].pan == pan && panK[i].ch == ch)
      return panK[i].fam;
  return FAM_UNKNOWN;
}
static void panRemember(uint16_t pan, uint8_t ch, Family f) {
  int slot = 0;
  for (int i = 0; i < 10; i++) {
    if (panK[i].fam != FAM_UNKNOWN && panK[i].pan == pan && panK[i].ch == ch) {
      slot = i;
      break;
    }
    if (panK[i].at < panK[slot].at)
      slot = i;
  }
  panK[slot] = {pan, ch, f, millis()};
}
// Parses one captured frame. Layout of h.raw: [0]=PHR length, [1..2]=FCF, [3]=seq, then addressing fields.
//   Beacon   : Thread beacons carry protocol id 0x03 (+ 16-char network name), Zigbee beacons protocol id 0x00.
//   Data     : Thread secures at the MAC layer (security-enabled bit); Zigbee does not (NWK security instead).
//              Unsecured Zigbee NWK header has protocol version 2 in bits 2-5 of its first byte;
//              6LoWPAN dispatch bytes (IPHC 011xxxxx, 0x41, mesh 10xxxxxx, frag 11xxxxxx) mean Thread-family.
static void drainZb() {
#ifdef HAS_IEEE154
  while (zbTail != zbHead) {
    ZbHit h = zbQ[zbTail];
    zbTail = (zbTail + 1) & 15;
    const uint8_t* f = h.raw;
    int fl = h.rawLen;
    if (fl < 5)
      continue;
    zbSeen++;
    uint16_t fcf = (uint16_t)f[1] | ((uint16_t)f[2] << 8);
    uint8_t ftype = fcf & 7, destMode = (fcf >> 10) & 3, srcMode = (fcf >> 14) & 3;
    bool secEn = (fcf >> 3) & 1, panComp = (fcf >> 6) & 1;
    // ACKs and source-less frames have no identity to track
    if (ftype == 2 || srcMode == 0)
      continue;
    int off = 4;
    uint16_t pan = 0;
    if (destMode) {
      if (off + 2 <= fl) {
        pan = f[off] | (f[off + 1] << 8);
        off += 2;
      }
      if (destMode == 2)
        off += 2;
      else if (destMode == 3)
        off += 8;
      if (off > fl)
        continue;
    }
    if (!panComp) {
      if (off + 2 <= fl) {
        pan = f[off] | (f[off + 1] << 8);
        off += 2;
      }
    }
    uint8_t mac[6] = {0x15, 0x4A, h.ch, 0, 0, 0};
    if (srcMode == 2) {
      if (off + 2 > fl)
        continue;
      mac[3] = f[off];
      mac[4] = f[off + 1];
      mac[5] = (uint8_t)(pan & 0xFF);
      off += 2;
    } else if (srcMode == 3) {
      if (off + 8 > fl)
        continue;
      memcpy(mac, f + off + 2, 6);
      off += 8;
    } else
      continue;
    Family fam = FAM_154;
    Conf conf = CONF_LOW;
    const char* meth = "802.15.4";
    char lab[24];
    lab[0] = 0;
    if (ftype == 0) {  // beacon
      int p = off;
      if (p + 2 <= fl)
        p += 2;  // superframe specification
      if (p < fl) {
        uint8_t gts = f[p++];
        p += (gts & 7) * 3;
      }
      if (p < fl) {
        uint8_t pend = f[p++];
        p += (pend & 7) * 2 + ((pend >> 4) & 7) * 8;
      }
      if (p < fl) {
        uint8_t proto = f[p];
        if (proto == 0x03) {
          fam = FAM_THREAD;
          conf = CONF_HIGH;
          meth = "beacon";
          // protocol id, version/flags, then a 16-byte network name
          int avail = fl - (p + 2);
          if (avail > 0) {
            int k = 0;
            for (int i = 0; i < 16 && i < avail && k < 20; i++) {
              uint8_t c = f[p + 2 + i];
              if (c >= 32 && c < 127)
                lab[k++] = (char)c;
              else
                break;
            }
            lab[k] = 0;
          }
        } else if (proto == 0x00) {
          fam = FAM_ZIGBEE;
          conf = CONF_HIGH;
          meth = "beacon";
        }
      }
    } else if (ftype == 1) {  // data
      if (secEn) {
        fam = FAM_THREAD;
        conf = CONF_MED;
        meth = "MAC sec";
      } else if (off < fl) {
        uint8_t b = f[off];
        if ((b & 0x3C) == 0x08) {
          fam = FAM_ZIGBEE;
          conf = CONF_HIGH;
          meth = "NWK v2";
        } else if ((b & 0xE0) == 0x60 || b == 0x41 || (b & 0xC0) == 0x80 || (b & 0xC0) == 0xC0) {
          fam = FAM_THREAD;
          conf = CONF_MED;
          meth = "6LoWPAN";
        }
      }
    } else if (ftype == 3)
      meth = "MAC cmd";
    if (fam != FAM_154)
      panRemember(pan, h.ch, fam);
    else {
      Family k = panLookup(pan, h.ch);
      if (k != FAM_UNKNOWN) {
        fam = k;
        conf = CONF_MED;
      }
    }
    if (!lab[0] && pan)
      snprintf(lab, sizeof(lab), "PAN %04X", pan);
    if (!lab[0])
      snprintf(lab, sizeof(lab), "ch %u", h.ch);
    upsert(mac, fam, conf, RAD_ZB, h.rssi, h.ch, lab, meth);
  }
#endif
}
static NimBLEScan* bleScan = nullptr;
static bool bleInitFailed = false;
#define BLE_MIN_HEAP 50000
#define BLE_MIN_BLOCK 16000
// Starts Bluetooth once. Returns false (and leaves Wi-Fi scanning running) if memory is too tight.
static bool bleBringUp() {
#if ENABLE_BLE
  if (bleScan)
    return true;
  if (bleInitFailed)
    return false;
  uint32_t fh = (uint32_t)ESP.getFreeHeap(), lb = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  Serial.printf("ble: before start free heap %u, largest block %u\n", (unsigned)fh, (unsigned)lb);
  if (fh < BLE_MIN_HEAP || lb < BLE_MIN_BLOCK) {
    Serial.println("ble: not enough free memory - Bluetooth scanning is off (Wi-Fi scanning still works)");
    bleInitFailed = true;
    showToast("Bluetooth off: low memory");
    return false;
  }
  if (!NimBLEDevice::init("")) {
    Serial.println("ble: init failed - Bluetooth scanning is off");
    bleInitFailed = true;
    showToast("Bluetooth failed to start");
    return false;
  }
  bleScan = NimBLEDevice::getScan();
  bleScan->setScanCallbacks(&bleCb, true);
  bleScan->setMaxResults(0);
  bleScan->setActiveScan(true);
  bleScan->setInterval(200);
  bleScan->setWindow(100);
  Serial.printf("ble: ready, free heap %u\n", (unsigned)ESP.getFreeHeap());
  return true;
#else
  return false;
#endif
}
static bool radioInit = false, bleHeld = false, bleRunning = false;
static uint32_t radioDirtyAt = 0, radioOnAt = 0;
// sniffer stayed silent: use periodic access-point scans instead
static bool wifiFallback = false;
static bool scanRunning = false;
static uint32_t scanNextAt = 0;
static void bleStartScan() {
  if (!bleScan || bleRunning || bleHeld)
    return;
  bleScan->start(0, false, true);
  bleRunning = true;
}
static void bleStopScan() {
  if (bleScan && bleRunning)
    bleScan->stop();
  bleRunning = false;
}
// While tracking a Wi-Fi target the BLE scan is paused (less coexistence noise).
// While tracking a BLE/Zigbee target it keeps running.
static void bleHold(bool on) {
  if (on == bleHeld)
    return;
  bleHeld = on;
  if (!radioOn)
    return;
  if (on)
    bleStopScan();
  else
    bleStartScan();
}
// the BLE lib keeps every advert it has seen -> free it
static void bleHousekeeping() {
  static uint32_t last = 0;
  bleDrain();
  if (!radioOn || !bleScan || !bleRunning || bleHeld || millis() - last < 15000)
    return;
  last = millis();
  bleScan->stop();
  bleScan->clearResults();
  bleScan->start(0, false, true);
}
// Wi-Fi sniffing and 802.15.4 share ONE radio. When both are armed they take turns: a Wi-Fi slice sweeps channels 1-13,
// then a Zigbee/Thread slice sweeps channels 11-26, and so on, so every armed category keeps finding things.
// which side owns the radio right now (only used when both are armed)
static bool sliceWifi = true;
static uint32_t sliceAt = 0;
static bool wantWifiNow() {
  return catOn[CAT_ALPR] || catOn[CAT_AXON] || catOn[CAT_CAM] || catOn[CAT_GLASS] || catOn[CAT_DRONE] ||
         catOn[CAT_WIFI];
}
static bool bothArmed() {
  return wantWifiNow() && catOn[CAT_ZT] && zbOn;
}
// silence watchdog: when did the sniffer last prove it is alive?
static uint32_t sniffSince = 0, sniffFramesAt = 0, fallbackAt = 0;
static uint8_t sniffRestarts = 0;
static bool sniffOn = false;
// access-point scans in a row that found nothing
static uint8_t scanEmptyN = 0;
// (re)register everything the Wi-Fi sniffer needs: after 802.15.4 has used the radio nothing is assumed to survive
static void wifiSnifferArm(bool resetClock) {
  wifi_promiscuous_filter_t filt = {};
  filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffCb);
  if (resetClock) {
    sniffSince = millis();
    sniffFramesAt = framesSeen;
  }
}
static inline esp_err_t forceBand24() {
#if defined(CONFIG_SOC_WIFI_SUPPORT_5G) || defined(SOC_WIFI_SUPPORT_5G)
  return esp_wifi_set_band_mode(WIFI_BAND_MODE_2G_ONLY);
#else
  return ESP_OK;
#endif
}
// last resort when frames stop: restart the Wi-Fi driver itself
static void wifiSnifferRestart() {
  Serial.printf("wifi: no frames for %lu s -> restarting the Wi-Fi driver (restart %u)\n",
                (unsigned long)((millis() - sniffSince) / 1000), (unsigned)(sniffRestarts + 1));
  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_OFF);
  delay(60);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect();
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);
  forceBand24();
  wifiSnifferArm(true);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(CHANS[chanIdx % CHANS_N], WIFI_SECOND_CHAN_NONE);
  sniffOn = true;
  sniffRestarts++;
}
// promiscuous mode follows the current slice
static void applyWifiState() {
  if (!radioInit)
    return;
  bool on = !wifiFallback && wantWifiNow() && (!catOn[CAT_ZT] || !zbOn || sliceWifi);
  // callback + filter are registered again every time Wi-Fi takes the radio over
  if (on)
    wifiSnifferArm(!sniffOn);
  sniffOn = on;
  esp_err_t pe = esp_wifi_set_promiscuous(on);
  esp_err_t ce = on ? esp_wifi_set_channel(CHANS[chanIdx % CHANS_N], WIFI_SECOND_CHAN_NONE) : ESP_OK;
  if (pe != ESP_OK || ce != ESP_OK)
    Serial.printf("wifi: promiscuous(%d)=%d channel=%d\n", (int)on, (int)pe, (int)ce);
}
// Bring the radios in line with what is armed. Safe to call any time (the port only did this once).
static void radioApply() {
  bool wantZb = catOn[CAT_ZT];
  if (!radioInit) {
    // Wi-Fi is brought up the standard Arduino way (netif + event loop + driver), idle STA, then sniffing is enabled on top.
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect();
    WiFi.setSleep(false);
    esp_err_t e1 = esp_wifi_set_ps(WIFI_PS_NONE);
    forceBand24();
    wifi_promiscuous_filter_t filt = {};
    filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
    esp_err_t e2 = esp_wifi_set_promiscuous_filter(&filt);
    esp_err_t e3 = esp_wifi_set_promiscuous_rx_cb(&sniffCb);
    Serial.printf("wifi init: ps=%d filter=%d cb=%d  free heap %u\n", (int)e1, (int)e2, (int)e3,
                  (unsigned)ESP.getFreeHeap());
#if ENABLE_BLE
    bleBringUp();
#endif
    radioInit = true;
  }
  radioOn = true;
  radioOnAt = millis();
  bleStartScan();
  // Wi-Fi lets go of the radio before 802.15.4 takes it
  if (wantZb) {
    if (radioInit)
      esp_wifi_set_promiscuous(false);
    sniffOn = false;
    zbStart();
  }
  // 802.15.4 shuts down completely before Wi-Fi takes it back
  else if (zbOn) {
    zbStop();
    delay(30);
  }
  // a new arrangement gets a fresh chance at sniffing
  wifiFallback = false;
  scanRunning = false;
  sniffRestarts = 0;
  sliceWifi = true;
  sliceAt = millis();
  applyWifiState();
  scanning = true;
}
static void radioStop() {
  wifiFallback = false;
  scanRunning = false;
  hunting = false;
  bleHeld = false;
  bleStopScan();
  if (radioInit)
    esp_wifi_set_promiscuous(false);
  sniffOn = false;
  zbStop();
  radioOn = false;
}
static void hopChannels() {
  if (!scanning || hunting || !radioOn || armedMask == 0)
    return;
  uint32_t now = millis();
  bool both = bothArmed();
  // hand the radio over when the current slice is up
  if (both) {
    if ((int32_t)(now - sliceAt) > (sliceWifi ? 3000 : 3400)) {
      sliceWifi = !sliceWifi;
      sliceAt = now;
      applyWifiState();
    }
  }
  bool zbSide = catOn[CAT_ZT] && zbOn && (!both || !sliceWifi);
  if ((int32_t)(now - lastHop) < (both ? 210 : (saverOn ? 600 : 330)))
    return;
  lastHop = now;
  if (zbSide) {
#ifdef HAS_IEEE154
    zbChanIdx = (zbChanIdx + 1) % ZB_CHANS_N;
    esp_ieee802154_set_channel(ZB_CHANS[zbChanIdx]);
#endif
    return;
  }
  if (!wantWifiNow() || wifiFallback)
    return;
  chanIdx = (chanIdx + 1) % CHANS_N;
  esp_wifi_set_channel(CHANS[chanIdx], WIFI_SECOND_CHAN_NONE);
}
static uint8_t huntLockedCh = 0;  // the Wi-Fi channel tracking is parked on
// the tracked network has gone quiet and the radio is looking for it again
static bool huntSearching = false;
static void lockHuntChannel(const Contact& c) {
  huntSearching = false;
  huntLockedCh = c.ch;
  if (!radioOn)
    return;
  if (c.radio == RAD_WIFI) {
    // this radio belongs to Wi-Fi while we track
    if (radioInit && !wifiFallback)
      esp_wifi_set_promiscuous(true);
    if (c.ch >= 1 && c.ch <= 14)
      esp_wifi_set_channel(c.ch, WIFI_SECOND_CHAN_NONE);
  } else if (c.radio == RAD_ZB) {
    // ...and to 802.15.4 for a Zigbee/Thread target
    if (radioInit)
      esp_wifi_set_promiscuous(false);
#ifdef HAS_IEEE154
    if (c.ch >= 11 && c.ch <= 26)
      esp_ieee802154_set_channel(c.ch);  // the port poked the Wi-Fi channel here
#endif
  }
}
static void radioDebugTick(uint32_t now) {
#if RADIO_DEBUG
  static uint32_t last = 0;
  if (!radioOn || (int32_t)(now - last) < 2000)
    return;
  last = now;
  bool prom = false;
  uint8_t pch = 0;
  wifi_second_chan_t sc2;
  esp_wifi_get_promiscuous(&prom);
  esp_wifi_get_channel(&pch, &sc2);
  Serial.printf(
      "radio: prom=%d ch=%u  sniff calls=%lu  wifi frames=%lu  ble adverts=%lu  802.15.4=%lu  contacts=%d  heap=%u\n",
      (int)prom, (unsigned)pch, (unsigned long)sniffCalls, (unsigned long)framesSeen, (unsigned long)bleSeen,
      (unsigned long)zbSeen, nContacts, (unsigned)ESP.getFreeHeap());
#endif
}
// Same matching rules as the sniffer, applied to scan results (BSSID + SSID only).
static void ingestAp(const uint8_t* bssid, const char* ssid, int8_t rssi, uint8_t ch) {
  if (!bssid || (bssid[0] & 0x01))
    return;
  Family fam = FAM_UNKNOWN;
  Conf conf = CONF_LOW;
  const char* tag = "OUI";
  bool hit = ouiMatch(WIFI_OUI, WIFI_OUI_N, bssid, &fam, &conf, &tag);
  Family sf;
  Conf sc;
  if (textMatch(SSID_RULE, SSID_RULE_N, ssid, &sf, &sc)) {
    fam = sf;
    if (sc > conf)
      conf = sc;
    hit = true;
    tag = "SSID";
  }
  if (!hit && catOn[CAT_WIFI] && ssid[0]) {
    fam = FAM_WIFI;
    conf = CONF_LOW;
    hit = true;
    tag = "AP";
  }
  if (!hit)
    return;
  if (localAdmin(bssid) && fam != FAM_FLOCK && fam != FAM_GUNSHOT && fam != FAM_AXON && fam != FAM_BODYCAM && fam != FAM_GLASS && fam != FAM_DRONE &&
      fam != FAM_WIFI)
    return;
  upsert(bssid, fam, conf, RAD_WIFI, rssi, (uint8_t)(ch | 0x80), ssid[0] ? ssid : tag,
         (fam == FAM_WIFI && wifiLightSsid(ssid)) ? "LIGHT" : "AP scan");
}
static void wifiFallbackService(uint32_t now) {
  // 1) watchdog: while Wi-Fi sniffing is wanted, any frame proves the sniffer is alive. About 7 s of silence (one channel sweep + 2.5 s) -> restart
  //    the Wi-Fi driver once; still silent for the same time after that -> use access-point scans instead.
  //
  bool sniffWanted = radioOn && radioInit && wantWifiNow() && !catOn[CAT_ZT] && scanning && !hunting;
  if (!wifiFallback && sniffWanted) {
    if (framesSeen != sniffFramesAt) {
      sniffFramesAt = framesSeen;
      sniffSince = now;
      sniffRestarts = 0;
    }
    uint32_t silent = now - sniffSince;
    // one full pass over all channels: a lone access point is heard only once per pass
    uint32_t sweepMs = (uint32_t)CHANS_N * (saverOn ? 600 : 330);
    uint32_t limit = sweepMs + 2500;
    if (limit < 6500)
      limit = 6500;
    if (sniffRestarts == 0 && silent > limit) {
      wifiSnifferRestart();
      sniffSince = now;
    } else if (sniffRestarts >= 1 && silent > limit) {
      wifiFallback = true;
      scanNextAt = now;
      fallbackAt = now;
      sniffOn = false;
      esp_wifi_set_promiscuous(false);
      Serial.printf(
          "wifi: sniffer still silent after a driver restart (callback calls=%lu) -> switching to access-point scans\n",
          (unsigned long)sniffCalls);
      needRender = true;
    }
  }
  // 1b) in fallback: every 60 s give the sniffer another chance (the radio may have recovered)
  if (wifiFallback && radioOn && wantWifiNow() && !catOn[CAT_ZT] && !scanRunning &&
      (int32_t)(now - fallbackAt) > 60000) {
    // (one driver restart per arrangement is enough; a second silence goes straight back to scans)
    wifiFallback = false;
    sniffRestarts = 1;
    Serial.println("wifi: trying the sniffer again");
    applyWifiState();
    needRender = true;
  }
  // 2) run scans back to back, ingesting each result set
  if (!wifiFallback || !radioOn || !wantWifiNow() || (!scanning && !hunting))
    return;
  if (!scanRunning) {
    if ((int32_t)(now - scanNextAt) < 0)
      return;
    WiFi.scanNetworks(true, true, false, 110);
    scanRunning = true;
    return;
  }
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING)
    return;
  scanRunning = false;
  scanNextAt = now + 600;
  if (n > 0) {
    for (int i = 0; i < n; i++)
      ingestAp(WiFi.BSSID(i), WiFi.SSID(i).c_str(), (int8_t)WiFi.RSSI(i), (uint8_t)WiFi.channel(i));
    framesSeen += n;
    scanEmptyN = 0;
  } else if (n == 0 && scanEmptyN < 250)
    scanEmptyN++;
  WiFi.scanDelete();
}
// ---- Share portal state ----------------------------------------------------
static WebServer web(80);
static bool portalOn = false;
static char portalSsid[20] = "HUNTR-SEEKR";
static char portalPass[12] = "";
// ---- Share portal (WPA2) ---------------------------------------------------
static const char* PAGE_HEAD =
    "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<style>body{font-family:-apple-system,system-ui,sans-serif;background:#000;color:#eef1f5;margin:0;padding:20px}"
    "h2{margin:0 0 4px}h2 b{color:#ffa514}.s{color:#8b94a2;font-size:13px;margin-bottom:18px}"
    ".c{background:#10131a;border:1px solid #28303a;border-radius:14px;padding:12px 14px;margin:8px 0;display:flex;justify-content:space-between;gap:10px}"
    "a{color:#00e8c0;text-decoration:none;word-break:break-all}.z{color:#8b94a2;font-size:13px;white-space:nowrap}"
    "button,input[type=submit]{background:#1c2129;color:#eef1f5;border:1px solid #28303a;border-radius:12px;padding:10px 16px;font-size:15px}"
    ".w{color:#ff453a}</style>";
static void handlePortalRoot() {
  String html;
  html.reserve(1800);
  html += PAGE_HEAD;
  html += "<h2><b>HUNTR</b>/SEEKR</h2><div class=s>files on device</div>";
  html +=
      "<div class=c><a href='/live'>Live view</a><a href='/sigs'>Signatures</a><a href='/update'>Update firmware</a><a href='/bootlogo'>Boot logo</a>";
#if HAS_BOOTSOUND
  html += "<a href='/boot'>Boot sound</a>";
#endif
  html += "</div>";
  if (!fsOk || !fsStore)
    html += "<div class=c>no filesystem</div>";
  else {
    File root = fsStore->open("/");
    File e = root.openNextFile();
    int n = 0;
    while (e) {
      String name = e.name();
      if (name.startsWith("/"))
        name.remove(0, 1);
      html += "<div class=c><a href='/dl?f=";
      html += name;
      html += "'>";
      html += name;
      html += "</a><span class=z>";
      html += String((unsigned long)e.size());
      html += " B</span></div>";
      n++;
      e.close();
      e = root.openNextFile();
    }
    root.close();
    if (!n)
      html += "<div class=c>no files yet</div>";
  }
  html += "<form method=POST action='/upload' enctype='multipart/form-data' style='margin:22px 0 10px'>";
  html += "<div class=s>put a csv or a 24-bit boot.bmp on the device (the Boot logo page is nicer for pictures)</div>";
  html += "<input type=file name=f accept='.bmp,.csv,.txt'> <input type=submit value=Upload></form>";
  html += "<form method=POST action='/wipe' onsubmit=\"return confirm('Delete all scan logs?')\">";
  html += "<button class=w>Wipe all logs</button></form>";
  web.send(200, "text/html", html);
}
static void handlePortalDl() {
  if (!fsOk || !fsStore) {
    web.send(500, "text/plain", "no fs");
    return;
  }
  String name = web.arg("f");
  name.replace("..", "");
  if (name.startsWith("/"))
    name.remove(0, 1);
  String path = "/" + name;
  if (!fsStore->exists(path)) {
    web.send(404, "text/plain", "missing");
    return;
  }
  File f = fsStore->open(path, FILE_READ);
  web.streamFile(f, name.endsWith(".csv") ? "text/csv" : "application/octet-stream");
  f.close();
}
static void handlePortalWipe() {
  if (!fsOk || !fsStore) {
    web.send(500, "text/plain", "no fs");
    return;
  }
  File root = fsStore->open("/");
  File e = root.openNextFile();
  while (e) {
    String name = e.name();
    e.close();
    if (name.indexOf("skiscan_") >= 0) {
      if (!name.startsWith("/"))
        name = "/" + name;
      fsStore->remove(name);
    }
    e = root.openNextFile();
  }
  root.close();
  web.sendHeader("Location", "/");
  web.send(303, "text/plain", "ok");
}
static File portalUp;
static String portalUpName;
static void handlePortalUpload() {
  HTTPUpload& up = web.upload();
  if (up.status == UPLOAD_FILE_START) {
    portalUpName = up.filename;
    portalUpName.replace("\\", "/");
    int sl = portalUpName.lastIndexOf('/');
    if (sl >= 0)
      portalUpName = portalUpName.substring(sl + 1);
    portalUpName.replace("..", "");
    if (!portalUpName.length())
      portalUpName = "upload.bin";
    String path = "/" + portalUpName;
    if (portalUp)
      portalUp.close();
    if (fsStore) {
      if (fsStore->exists(path))
        fsStore->remove(path);
      portalUp = fsStore->open(path, FILE_WRITE);
    }
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (portalUp)
      portalUp.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END) {
    if (portalUp)
      portalUp.close();
  }
}
static void handlePortalUploadDone() {
  web.sendHeader("Location", "/");
  web.send(303, "text/plain", "ok");
}
static void portalStart() {
  radioStop();
  // Arduino's WiFi library owns the driver: it switches STA -> AP itself
  radioInit = false;
  static const char alpha[] = "abcdefghjkmnpqrstuvwxyz23456789";
  for (int i = 0; i < 8; i++)
    portalPass[i] = alpha[esp_random() % (sizeof(alpha) - 1)];
  portalPass[8] = 0;
  WiFi.mode(WIFI_AP);
  // new name each Share: a phone cannot reuse a password it saved from an earlier Share
  {
    unsigned r4 = (unsigned)(esp_random() & 0xFFFF);
    snprintf(portalSsid, sizeof(portalSsid), "HUNTR-SEEKR-%04X", r4);
  }
  WiFi.softAP(portalSsid, portalPass, 1, 0, 2);
  web.on("/", handlePortalRoot);
  web.on("/dl", handlePortalDl);
  web.on("/wipe", HTTP_POST, handlePortalWipe);
  web.on("/upload", HTTP_POST, handlePortalUploadDone, handlePortalUpload);
  // live view, JSON, clock sync, signatures, update
  portalPlusRoutes();
  web.begin();
  portalPlusBle();  // Bluetooth keeps scanning while sharing
  portalOn = true;
}
static void portalStop() {
  if (!portalOn)
    return;
  web.stop();
  bleStopScan();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  portalOn = false;
  if (armedMask) {
    radioDirty = true;
    radioDirtyAt = millis();
  }  // resume scanning after sharing
}
// ---- Storage / sessions ----------------------------------------------------
static void startNewSession() {
  // the previous session's findings were never saved
  if (ledgerUnsaved() && fsOk) {
    if (logMode == 1)
      ledgerSave();
    else
      logNotice = "Last session's finds were not saved";
  }
  logFileMade = false;
  huntFileMade = false;
  logRows = 0;
  logFull = false;
  ledgerReset();
  if (!fsOk) {
    sessionId++;
    snprintf(logPath, sizeof(logPath), "/skiscan_log_%03d.csv", sessionId < 1 ? 1 : sessionId);
    snprintf(huntPath, sizeof(huntPath), "/skiscan_hunt_%03d.csv", sessionId < 1 ? 1 : sessionId);
    snprintf(livePath, sizeof(livePath), "/skiscan_live_%03d.csv", sessionId < 1 ? 1 : sessionId);
    return;
  }
  int maxn = 0;
  File root = fsStore->open("/");
  if (root) {
    File e = root.openNextFile();
    while (e) {
      const char* raw = e.name();
      const char* n = strrchr(raw, '/');
      n = n ? n + 1 : raw;
      int v = 0;
      if ((sscanf(n, "skiscan_log_%d", &v) == 1 || sscanf(n, "skiscan_live_%d", &v) == 1 ||
           sscanf(n, "skiscan_hunt_%d", &v) == 1) &&
          v > maxn)
        maxn = v;
      e.close();
      e = root.openNextFile();
    }
    root.close();
  }
  sessionId = maxn + 1;
  snprintf(logPath, sizeof(logPath), "/skiscan_log_%03d.csv", sessionId);
  snprintf(huntPath, sizeof(huntPath), "/skiscan_hunt_%03d.csv", sessionId);
  // (no files are created yet: they appear when something is saved)
  snprintf(livePath, sizeof(livePath), "/skiscan_live_%03d.csv", sessionId);
  portENTER_CRITICAL(&mux);
  nContacts = 0;
  lqH = lqT = 0;
  memset(contacts, 0, sizeof(contacts));
  portEXIT_CRITICAL(&mux);
  selValid = false;
  hunting = false;
  logCount = 0;
  // per-group statistics and the timeline start with the session
  statReset();
  needNewSession = false;
  Serial.printf("session %d  %s\n", sessionId, logPath);
}
static bool mountFs() {
  // SETUP > Save to: the SD card first (Auto, SD card only)
  if (storMode != 2 && sdMountFs())
    return true;
  if (storMode == 1) {
    Serial.println("storage: Save to = SD card only, and no card was found");
    return false;
  }
  if (LittleFS.begin(true)) {
    fsStore = &LittleFS;
    Serial.println("LittleFS ok");
    return true;
  }
  if (LittleFS.begin(true, "/littlefs", 10, "ffat")) {
    fsStore = &LittleFS;
    Serial.println("LittleFS ok (ffat partition)");
    return true;
  }
  Serial.println("LittleFS fail");
  return false;
}
// ---- Preferences -----------------------------------------------------------
static void savePrefs() {
  prefs.begin("skiplus", false);
  prefs.putInt("bright", bright);
  prefs.putInt("vol", soundVol);
  prefs.putUChar("alert", alertMode);
  prefs.putBool("raise", raiseWake);
  prefs.putBool("shake", shakePause);
  prefs.putBool("disc", discreet);
  prefs.putUChar("pmode", powerMode);
  prefs.putUChar("follow", followMin);
  prefs.putBool("skim", skimWatch);
  prefs.putUChar("stor", storMode);
  prefs.putUChar("ori", orient);
  prefs.putUChar("oriOff", autoOff);
  prefs.putBool("calOk", calOk);
  prefs.putUChar("calU", (uint8_t)(calUi * 2 + (calUs < 0)));
  prefs.putUChar("calN", (uint8_t)(calNi * 2 + (calNs < 0)));
  prefs.putUChar("logm", logMode);
  prefs.putUChar("lview", listView);
  prefs.putUChar("bsnd", bootSoundMode);
  prefs.putUChar("blg", bootLogoMode);
  prefs.putBool("alien", termOn && !hudOn);
  prefs.putBool("aliens", hudOn);
  // "dim2": a new key, so the new default (off) applies to boards that saved the old one
  prefs.putBool("dim2", autoDim);
  prefs.putUChar("colr", colorMode);
  BOARD_PREFS_SAVE
  prefs.end();
}
static void loadPrefs() {
  prefs.begin("skiplus", true);
  bright = prefs.getInt("bright", 200);
  soundVol = prefs.getInt("vol", prefs.getBool("snd", true) ? 70 : 0);  // old builds stored an on/off switch
  if (soundVol < 0)
    soundVol = 0;
  if (soundVol > 100)
    soundVol = 100;
  soundOn = soundVol > 0;
  alertMode = prefs.getUChar("alert", 1);
  if (alertMode > 2)
    alertMode = 1;
  raiseWake = prefs.getBool("raise", true);
  shakePause = prefs.getBool("shake", false);
  discreet = prefs.getBool("disc", false);
  powerMode = prefs.getUChar("pmode", 1);
  if (powerMode > 2)
    powerMode = 1;
  orient = prefs.getUChar("ori", DEFAULT_ORIENT);
  if (orient > 4)
    orient = 0;
  autoOff = prefs.getUChar("oriOff", 0) & 3;
  calOk = prefs.getBool("calOk", false);
  if (calOk) {
    uint8_t u = prefs.getUChar("calU", 2), n = prefs.getUChar("calN", 4);
    calUi = (u >> 1) % 3;
    calUs = (u & 1) ? -1 : 1;
    calNi = (n >> 1) % 3;
    calNs = (n & 1) ? -1 : 1;
    if (calUi == calNi)
      calOk = false;
  }
  followMin = prefs.getUChar("follow", 10);
  if (followMin != 0 && followMin != 5 && followMin != 10 && followMin != 20)
    followMin = 10;
  skimWatch = prefs.getBool("skim", true);
  storMode = prefs.getUChar("stor", 0);
  if (storMode > 2)
    storMode = 0;
  logMode = prefs.getUChar("logm", 0);
  if (logMode > 2)
    logMode = 0;
  listView = prefs.getUChar("lview", 0);
  if (listView > 3)
    listView = 0;
  bootSoundMode = prefs.getUChar("bsnd", 1);
  if (bootSoundMode > 2)
    bootSoundMode = 1;
  bootLogoMode = prefs.getUChar("blg", 0);
  if (bootLogoMode > 1)
    bootLogoMode = 0;
  applyThemeMode(prefs.getBool("aliens", false) ? 2 : (prefs.getBool("alien", false) ? 1 : 0));
  autoDim = prefs.getBool("dim2", false);
  colorMode = prefs.getUChar("colr", 0) & 3;
  BOARD_PREFS_LOAD
  prefs.end();
  if (bright < 20)
    bright = 20;
  if (bright > 255)
    bright = 255;
}
// ---- I2C helpers -----------------------------------------------------------
static bool i2cRead(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t n) {
  if (!HAS_I2C)
    return false;  // boards without an I2C bus (the C5)
  Wire.beginTransmission(addr);
  Wire.write(reg);
  // STOP, then read: no repeated-start dependency
  if (Wire.endTransmission() != 0)
    return false;
  delayMicroseconds(30);
  if (Wire.requestFrom((uint8_t)addr, (uint8_t)n) < n)
    return false;
  for (uint8_t i = 0; i < n; i++)
    buf[i] = Wire.read();
  return true;
}
static bool i2cWrite(uint8_t addr, uint8_t reg, uint8_t v) {
  if (!HAS_I2C)
    return false;
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(v);
  return Wire.endTransmission() == 0;
}
static uint8_t imuAddr = 0x6B;  // QMI8658: 0x6B or 0x6A (found at start-up)
// ---- SD card (shared SPI bus), internal flash as the fallback --------------
static bool trySd(SPIClass& bus, int sck, int miso, int mosi, uint32_t hz) {
  bus.begin(sck, miso, mosi, PIN_SD_CS);
  bool ok = SD.begin(PIN_SD_CS, bus, hz);
  if (ok && SD.cardType() == CARD_NONE) {
    SD.end();
    return false;
  }
  return ok;
}
static bool sdMountFs() {
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);
  pinMode(PIN_TFT_CS, OUTPUT);
  digitalWrite(PIN_TFT_CS, HIGH);
  pinMode(PIN_TOUCH_CS, OUTPUT);
  digitalWrite(PIN_TOUCH_CS, HIGH);
  bool ok = trySd(SPI, PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, 4000000);
  if (!ok) {
    SD.end();
    ok = trySd(SPI, PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, 1000000);
  }
  if (ok) {
    fsStore = &SD;
    Serial.println("SD card ok");
  } else
    Serial.println("no SD card: using internal flash");
  return ok;
}
// ---- Touch controller (CST816T: 6 bytes from register 0x01) ----------------
static bool touchOk = false;
static volatile uint32_t tpIsrN = 0;
static uint32_t tpOkN = 0;
static int tpDbgX = 0, tpDbgY = 0;
static uint32_t tpDbgAt = 0;
// loop / frame timing shown by the optional debug line
static uint32_t dbgLps = 0, dbgFrameMs = 0;
// strips sent to the panel / skipped because they had not changed (Self-test > Speed)
static uint32_t stripSent = 0, stripSkip = 0;
static void touchWake() {
  ts.begin();
  touchOk = true;
}
static void touchRetry(uint32_t now) {
  (void)now;
}
static void touchDebugTick(uint32_t now) {
  (void)now;
}
static bool rawTouch(int16_t* rx, int16_t* ry) {
  if (!ts.touched())
    return false;
  TS_Point p = ts.getPoint();
  tpOkN++;
  *rx = p.x;
  *ry = p.y;
  return true;
}
static bool tcalMap(float a, float b, int* px, int* py) {  // raw -> native panel pixel
  float x = tcal.c[0] + tcal.c[1] * a + tcal.c[2] * b, y = tcal.c[3] + tcal.c[4] * a + tcal.c[5] * b;
  *px = constrain((int)(x + 0.5f), 0, PANEL_W - 1);
  *py = constrain((int)(y + 0.5f), 0, PANEL_H - 1);
  return true;
}
// returns LOGICAL ui coordinates (the panel is 410x502 physical, the UI 273x335)
static bool touched(int* x, int* y) {
  if (!tcal.ok)
    return false;
  int16_t rx, ry;
  if (!rawTouch(&rx, &ry))
    return false;
  // resistive panels jitter: light smoothing, none on a fresh press
  static float sa = 0, sb = 0;
  static uint32_t lastAt = 0;
  uint32_t now = millis();
  if (now - lastAt > 60) {
    sa = rx;
    sb = ry;
  } else {
    sa += 0.55f * (rx - sa);
    sb += 0.55f * (ry - sb);
  }
  lastAt = now;
  int px, py, gx, gy;
  tcalMap(sa, sb, &px, &py);
  // native panel position -> position in the (rotated) drawing
  switch (rot) {
    case 0:
      gx = px;
      gy = py;
      break;
    case 2:
      gx = PANEL_W - 1 - px;
      gy = PANEL_H - 1 - py;
      break;
    case 1:
      gx = py;
      gy = PANEL_W - 1 - px;
      break;
    default:
      gx = PANEL_H - 1 - py;
      gy = px;
      break;
  }
  *x = constrain(gx, 0, W - 1);
  *y = constrain(gy, 0, H - 1);
  tpDbgX = *x;
  tpDbgY = *y;
  tpDbgAt = now;
  return true;
}
// ---- Power: no gauge and usually no battery on this board ------------------
static bool pmuOk = true, pmuBat = false, pmuCharging = false, pmuVbus = true;
static int pmuPct = -1;
static void pmuInit() {
  pmuOk = true;
}
static void servicePmu(uint32_t now) {
  (void)now;
}
static int batPct() {
  return pmuPct;
}  // -1 = no battery / unknown
// ---- Sound (optional buzzer on PIN_BUZZER; silent when PIN_BUZZER < 0) -----
// A passive buzzer plays one note at a time. Tunes are stepped by a hardware timer (not by the UI loop), so their timing does not stretch
// when a screen redraw takes a while; a UI blip is skipped while a tune is playing (the tune is more important).
static bool audioReady = false;
// loudness: a passive buzzer on one pin is loudest at 50% duty, and a smaller duty is quieter, so the Volume slider sets the duty.
// The slider follows a curve (amplitude = volume^1.6) so the steps sound even. All the way right = 50% duty = the loudest this drive can be; all the way left = silent.
// 0..512 of 1024
static uint32_t volDuty() {
  float a = powf(soundVol / 100.0f, 1.6f);
  if (a > 1.0f)
    a = 1.0f;
  return (uint32_t)(asinf(a) / 3.14159265f * 1024.0f + 0.5f);
}
// set the note's frequency, then its strength (duty) from the Volume slider
static inline void buzz(int hz) {
  if (PIN_BUZZER < 0)
    return;
  if (hz <= 0) {
    ledcWrite(PIN_BUZZER, 0);
    return;
  }
  if (ledcChangeFrequency(PIN_BUZZER, hz, 10) == 0) {
    static bool warned = false;
    if (!warned) {
      warned = true;
      Serial.println("sound: ledcChangeFrequency failed (is the buzzer pin attached?)");
    }
    return;
  }
  ledcWrite(PIN_BUZZER, volDuty());
}
static const Tone SND_ON[] = {{1700, 30}, {2500, 45}};
static const Tone SND_OFF[] = {{2500, 30}, {1500, 50}};
static const Tone SND_FIND[] = {{1900, 50}, {0, 25}, {2800, 80}};
static const Tone SND_BOOT[] = {{1400, 55}, {1900, 55}, {2600, 110}};
static const Tone SND_BACK[] = {{2000, 20}, {1500, 30}};
#define BLIP_HZ 2200
#define BLIP_MS 24
static const Tone* volatile sndSeq = nullptr;
static volatile uint8_t sndN = 0, sndI = 0;
static esp_timer_handle_t toneTimer = nullptr, seqTimer = nullptr;
static void toneOffCb(void*) {
  if (!sndSeq)
    buzz(0);
}  // end of a blip (never cuts a tune short)
// one note of the tune, then arm the timer for the next
static void seqStepCb(void*) {
  if (sndI >= sndN) {
    buzz(0);
    sndSeq = nullptr;
    return;
  }
  const Tone& t = sndSeq[sndI++];
  buzz(t.hz);
  esp_timer_start_once(seqTimer, (uint64_t)(t.ms ? t.ms : 1) * 1000);
}
static void tonePulse(int hz, int usec) {  // non-blocking: a timer switches it off
  if (PIN_BUZZER < 0 || sndSeq)
    return;
  if (!toneTimer) {
    esp_timer_create_args_t a = {};
    a.callback = &toneOffCb;
    a.name = "tone";
    esp_timer_create(&a, &toneTimer);
  }
  esp_timer_stop(toneTimer);
  buzz(hz);
  esp_timer_start_once(toneTimer, (uint64_t)usec);
}
static void applyVolume() {
  soundOn = soundVol > 0;
}
static void audioBeep(int hz, int ms) {
  if (!audioReady || !soundOn || discreet || hz <= 0)
    return;
  tonePulse(hz, ms * 1000);
}
static void sndPlay(const Tone* s, uint8_t n) {
  if (!audioReady || !soundOn || discreet || n == 0)
    return;
  if (!seqTimer) {
    esp_timer_create_args_t a = {};
    a.callback = &seqStepCb;
    a.name = "tune";
    esp_timer_create(&a, &seqTimer);
  }
  esp_timer_stop(seqTimer);
  if (toneTimer)
    esp_timer_stop(toneTimer);
  sndSeq = s;
  sndN = n;
  sndI = 0;
  seqStepCb(nullptr);  // first note sounds right now
}
// (kept for the shared loop: the timer steps the notes)
static void sndService() {}
static void audioWarm() {}
static void audioStopFile() {}  // (only the WATCH can play a sound file)
static void audioPlayFile() {}
// this touch already played its blip on finger-down
static bool pressTicked = false;
static void pressTick() {
  pressTicked = true;
  audioBeep(BLIP_HZ, BLIP_MS);
}
static void tapBlip() {
  if (!pressTicked)
    audioBeep(BLIP_HZ, BLIP_MS);
}
static bool audioInit() {
  if (PIN_BUZZER < 0)
    return false;
  if (!ledcAttach(PIN_BUZZER, 2000, 10)) {
    Serial.printf("sound: ledcAttach FAILED on IO%d\n", PIN_BUZZER);
    return false;
  }
  ledcWrite(PIN_BUZZER, 0);
  Serial.printf("sound: buzzer ready on IO%d\n", PIN_BUZZER);
  applyVolume();
  return true;
}
// ---- Ignore list (survives reboots: /ignore2.bin; the old MAC-only /ignore.bin is migrated) ----
// Ignored devices are hidden from lists, counts, alerts and logs. They are NOT forgotten: SETUP > Ignore list, or the "hidden" chip / row on the
// FINDS page, shows each one by name and brings it back with a tap.
struct IgnEntry {
  uint8_t mac[6];
  uint8_t fam;
  char name[22];
};
static IgnEntry ign[32];
static uint8_t nIgnore = 0;
static bool isIgnored(const uint8_t* mac) {
  for (int i = 0; i < nIgnore; i++)
    if (memcmp(ign[i].mac, mac, 6) == 0)
      return true;
  return false;
}
static void ignoreSave() {
  if (!fsOk || !fsStore)
    return;
  File f = fsStore->open("/ignore2.bin", FILE_WRITE);
  if (!f)
    return;
  const uint8_t hdr[3] = {'I', 'G', 2};
  f.write(hdr, 3);
  f.write(&nIgnore, 1);
  if (nIgnore)
    f.write((const uint8_t*)ign, (size_t)nIgnore * sizeof(IgnEntry));
  f.close();
}
static void ignoreLoad() {
  nIgnore = 0;
  if (!fsOk || !fsStore)
    return;
  if (fsStore->exists("/ignore2.bin")) {
    File f = fsStore->open("/ignore2.bin", FILE_READ);
    if (!f)
      return;
    uint8_t hdr[3] = {0}, n = 0;
    if (f.read(hdr, 3) == 3 && hdr[0] == 'I' && hdr[1] == 'G' && hdr[2] == 2 && f.read(&n, 1) == 1 && n <= 32) {
      int got = f.read((uint8_t*)ign, (size_t)n * sizeof(IgnEntry));
      if (got == (int)(n * sizeof(IgnEntry))) {
        nIgnore = n;
        for (int i = 0; i < nIgnore; i++)
          ign[i].name[21] = 0;
      }
    }
    f.close();
    return;
  }
  // an older version saved only the addresses: keep them, names unknown
  if (fsStore->exists("/ignore.bin")) {
    File f = fsStore->open("/ignore.bin", FILE_READ);
    if (!f)
      return;
    uint8_t n = 0;
    if (f.read(&n, 1) == 1 && n <= 32) {
      uint8_t buf[32 * 6];
      int got = f.read(buf, n * 6);
      if (got == n * 6) {
        for (int i = 0; i < n; i++) {
          memset(&ign[i], 0, sizeof(IgnEntry));
          memcpy(ign[i].mac, buf + i * 6, 6);
        }
        nIgnore = n;
      }
    }
    f.close();
    if (nIgnore)
      ignoreSave();
  }
}
// also removes it from the live list and stops tracking it
static void ignoreAdd(const uint8_t* mac, const char* name = nullptr, uint8_t fam = 0) {
  if (isIgnored(mac))
    return;
  char nm[22] = {0};
  if (name && name[0])
    snprintf(nm, sizeof(nm), "%s", name);
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < nContacts; i++)
    if (macEq(contacts[i].mac, mac)) {
      if (!nm[0])
        snprintf(nm, sizeof(nm), "%s", contacts[i].nick[0] ? contacts[i].nick : contacts[i].label);
      if (!fam)
        fam = (uint8_t)contacts[i].fam;
      memmove(&contacts[i], &contacts[i + 1], (size_t)(nContacts - i - 1) * sizeof(Contact));
      nContacts--;
      break;
    }
  if (selValid && macEq(selMac, mac))
    selValid = false;
  if (hunting && macEq(huntMac, mac))
    hunting = false;
  portEXIT_CRITICAL(&mux);
  if (nIgnore >= 32) {
    memmove(&ign[0], &ign[1], 31 * sizeof(IgnEntry));
    nIgnore = 31;
  }  // full: forget the oldest entry
  memset(&ign[nIgnore], 0, sizeof(IgnEntry));
  memcpy(ign[nIgnore].mac, mac, 6);
  ign[nIgnore].fam = fam;
  memcpy(ign[nIgnore].name, nm, sizeof(nm));
  nIgnore++;
  ignoreSave();
}
// bring one device back: it shows up again the next time it is heard
static void ignoreRemoveAt(int i) {
  if (i < 0 || i >= nIgnore)
    return;
  memmove(&ign[i], &ign[i + 1], (size_t)(nIgnore - i - 1) * sizeof(IgnEntry));
  nIgnore--;
  ignoreSave();
}
static void ignoreClear() {
  nIgnore = 0;
  ignoreSave();
}
// ---- Session statistics + 60-minute timeline -------------------------------
static GroupStat gstat[CAT_N];
// detections per minute, oldest first, [59] = this minute
static uint16_t minBucket[60];
static uint32_t sessionStartMs = 0, lastBucketMin = 0;
static uint16_t sessionFinds = 0;
static void statReset() {
  memset(gstat, 0, sizeof(gstat));
  memset(minBucket, 0, sizeof(minBucket));
  sessionStartMs = millis();
  lastBucketMin = 0;
  sessionFinds = 0;
  for (int i = 0; i < CAT_N; i++)
    gstat[i].best = -127;
}
static void bucketAdvance(uint32_t now) {
  uint32_t m = (now - sessionStartMs) / 60000UL;
  if (m > lastBucketMin) {
    uint32_t d = m - lastBucketMin;
    if (d >= 60)
      memset(minBucket, 0, sizeof(minBucket));
    else {
      memmove(minBucket, minBucket + d, (size_t)(60 - d) * sizeof(uint16_t));
      memset(minBucket + 60 - d, 0, (size_t)d * sizeof(uint16_t));
    }
    lastBucketMin = m;
  }
}
static void noteFind(const Contact& c) {  // called with mux held
  uint32_t now = millis();
  bucketAdvance(now);
  if (minBucket[59] < 65535)
    minBucket[59]++;
  sessionFinds++;
  for (int k = 0; k < CAT_N; k++)
    if (catOn[k] && famInCat(c.fam, k)) {
      if (!gstat[k].count) {
        gstat[k].firstMs = now;
        gstat[k].best = -127;
      }
      if (gstat[k].count < 65535)
        gstat[k].count++;
      if (c.rssi > gstat[k].best)
        gstat[k].best = c.rssi;
    }
}
// strongest recently-seen signal in a group (for the bars on the ARM tiles): -127 when nothing was heard in the last 10 s
static int bestRecentRssi(int cat) {
  uint32_t now = millis();
  int best = -127;
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < nContacts; i++) {
    const Contact& c = contacts[i];
    if (!famInCat(c.fam, cat) || (int32_t)(now - c.lastMs) > 10000)
      continue;
    int r = (int)c.ema;
    if (r > best)
      best = r;
  }
  portEXIT_CRITICAL(&mux);
  return best;
}
static int indexOfMac(const uint8_t* mac) {  // mux NOT held by the caller
  int r = -1;
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < nContacts; i++)
    if (macEq(contacts[i].mac, mac)) {
      r = i;
      break;
    }
  portEXIT_CRITICAL(&mux);
  return r;
}
// ---- Real-time clock (PCF85063 @0x51) --------------------------------------
struct RtcTime {
  uint16_t y;
  uint8_t mo, d, h, mi, s;
};
static RtcTime clk = {2000, 1, 1, 0, 0, 0};
static bool rtcOk = false, rtcApprox = false;
static char clkStr[24] = "";
static uint8_t bcd2bin(uint8_t v) {
  return (uint8_t)((v >> 4) * 10 + (v & 15));
}
static uint8_t bin2bcd(uint8_t v) {
  return (uint8_t)(((v / 10) << 4) | (v % 10));
}
static bool rtcChip = false, clockSet = false;  // PCF85063 present? / software clock set
static bool rtcReadChip() {
  uint8_t b[7];
  if (!i2cRead(0x51, 0x04, b, 7))
    return false;
  // oscillator-stop flag: the time is not trustworthy
  bool osc = (b[0] & 0x80) != 0;
  clk.s = bcd2bin(b[0] & 0x7F);
  clk.mi = bcd2bin(b[1] & 0x7F);
  clk.h = bcd2bin(b[2] & 0x3F);
  clk.d = bcd2bin(b[3] & 0x3F);
  clk.mo = bcd2bin(b[5] & 0x1F);
  clk.y = 2000 + bcd2bin(b[6]);
  bool sane = clk.y >= 2024 && clk.mo >= 1 && clk.mo <= 12 && clk.d >= 1 && clk.d <= 31 && clk.h < 24 && clk.mi < 60 &&
              clk.s < 60;
  if (!osc && sane)
    snprintf(clkStr, sizeof(clkStr), "%04u-%02u-%02u %02u:%02u:%02u", clk.y, clk.mo, clk.d, clk.h, clk.mi, clk.s);
  return !osc && sane;
}
static void rtcWriteChip(int y, int mo, int d, int h, int mi, int s) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  int yy = mo < 3 ? y - 1 : y;
  int wd = (yy + yy / 4 - yy / 100 + yy / 400 + t[mo - 1] + d) % 7;  // 0 = Sunday
  Wire.beginTransmission(0x51);
  Wire.write(0x04);
  Wire.write(bin2bcd((uint8_t)s));
  Wire.write(bin2bcd((uint8_t)mi));
  Wire.write(bin2bcd((uint8_t)h));
  Wire.write(bin2bcd((uint8_t)d));
  Wire.write((uint8_t)wd);
  Wire.write(bin2bcd((uint8_t)mo));
  Wire.write(bin2bcd((uint8_t)(y - 2000)));
  Wire.endTransmission();
}
// the ESP32's own clock, used if the chip does not answer
static bool softRead() {
  if (!clockSet)
    return false;
  time_t t = time(nullptr);
  struct tm tmv;
  localtime_r(&t, &tmv);
  clk.y = (uint16_t)(tmv.tm_year + 1900);
  clk.mo = (uint8_t)(tmv.tm_mon + 1);
  clk.d = (uint8_t)tmv.tm_mday;
  clk.h = (uint8_t)tmv.tm_hour;
  clk.mi = (uint8_t)tmv.tm_min;
  clk.s = (uint8_t)tmv.tm_sec;
  snprintf(clkStr, sizeof(clkStr), "%04u-%02u-%02u %02u:%02u:%02u", clk.y, clk.mo, clk.d, clk.h, clk.mi, clk.s);
  return true;
}
static bool rtcRead() {
  return rtcChip ? rtcReadChip() : softRead();
}
static void rtcWrite(int y, int mo, int d, int h, int mi, int s) {
  if (rtcChip)
    rtcWriteChip(y, mo, d, h, mi, s);
  else {
    struct tm tmv;
    memset(&tmv, 0, sizeof(tmv));
    tmv.tm_year = y - 1900;
    tmv.tm_mon = mo - 1;
    tmv.tm_mday = d;
    tmv.tm_hour = h;
    tmv.tm_min = mi;
    tmv.tm_sec = s;
    tmv.tm_isdst = 0;
    struct timeval tv;
    tv.tv_sec = mktime(&tmv);
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
    clockSet = true;
  }
  rtcOk = rtcRead();
  rtcApprox = false;
}
static void rtcInit() {
  uint8_t b[1];
  rtcChip = i2cRead(0x51, 0x04, b, 1);
  rtcOk = rtcRead();
  // never set (or the battery ran flat): start from the build time, marked approximate
  if (!rtcOk) {
    static const char* mon = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char m3[4] = {0};
    int d = 1, y = 2026, hh = 0, mm = 0, ss = 0;
    sscanf(__DATE__, "%3s %d %d", m3, &d, &y);
    sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);
    const char* p = strstr(mon, m3);
    int mo = p ? (int)((p - mon) / 3) + 1 : 1;
    rtcWrite(y, mo, d, hh, mm, ss);
    rtcApprox = true;
  }
  Serial.printf("RTC: %s %s %s\n", rtcChip ? "PCF85063" : "no chip (software clock)", rtcOk ? clkStr : "not available",
                rtcApprox ? "(approximate: open the Live page on your phone to set it)" : "");
}
static void rtcService(uint32_t now) {
  static uint32_t last = 0;
  if (last && (int32_t)(now - last) < 1000)
    return;
  last = now ? now : 1;
  if (rtcRead())
    rtcOk = true;
}
static const char* timeStr() {
  return rtcOk ? clkStr : "";
}
// ---- Custom signatures (/sigs.txt: one rule per line) ----------------------
//   oui  B4:1E:52  FLOCK [low|med|high]        Wi-Fi or BLE address prefix
//   ssid Flock     FLOCK                       Wi-Fi name contains (case-insensitive)
//   ble  AXON      AXON                        Bluetooth name contains
//   cid  0x0171    GLASS                       Bluetooth company id
static CustomSig csig[64];
static uint8_t ncsig = 0;
static int csigErrors = 0;
static Family famFromName(const char* s) {
  for (int f = 1; f < FAM_COUNT; f++)
    if (strcasecmp(FAM_NAME[f], s) == 0)
      return (Family)f;
  if (!strcasecmp(s, "CAMS"))
    return FAM_CAM;
  if (!strcasecmp(s, "ALPR"))
    return FAM_FLOCK;
  if (!strcasecmp(s, "TAGS"))
    return FAM_TAG;
  return FAM_UNKNOWN;
}
static bool containsCI(const char* hay, const char* needle) {
  size_t n = strlen(needle);
  if (!n)
    return false;
  for (const char* p = hay; *p; p++)
    if (strncasecmp(p, needle, n) == 0)
      return true;
  return false;
}
static void sigsParse(const char* text) {
  ncsig = 0;
  csigErrors = 0;
  const char* p = text;
  while (*p && ncsig < 64) {
    char line[96];
    int n = 0;
    while (*p && *p != '\n' && n < 95)
      line[n++] = *p++;
    while (*p && *p != '\n')
      p++;
    if (*p == '\n')
      p++;
    line[n] = 0;
    char* c = line;
    while (*c == ' ' || *c == '\t' || *c == '\r')
      c++;
    if (!*c || *c == '#')
      continue;
    char kind[8] = {0}, pat[32] = {0}, fam[16] = {0}, cf[8] = {0};
    int got = sscanf(c, "%7s %31s %15s %7s", kind, pat, fam, cf);
    Family f = got >= 3 ? famFromName(fam) : FAM_UNKNOWN;
    if (got < 3 || f == FAM_UNKNOWN) {
      csigErrors++;
      continue;
    }
    CustomSig s;
    memset(&s, 0, sizeof(s));
    s.fam = f;
    s.conf = CONF_MED;
    if (got >= 4) {
      if (!strcasecmp(cf, "high"))
        s.conf = CONF_HIGH;
      else if (!strcasecmp(cf, "low"))
        s.conf = CONF_LOW;
    }
    if (!strcasecmp(kind, "oui")) {
      unsigned a, b, d;
      if (sscanf(pat, "%x:%x:%x", &a, &b, &d) != 3 && sscanf(pat, "%2x%2x%2x", &a, &b, &d) != 3) {
        csigErrors++;
        continue;
      }
      s.kind = 1;
      s.oui[0] = a;
      s.oui[1] = b;
      s.oui[2] = d;
    } else if (!strcasecmp(kind, "ssid") || !strcasecmp(kind, "ble")) {
      s.kind = !strcasecmp(kind, "ssid") ? 2 : 3;
      strncpy(s.pat, pat, 23);
      s.plen = (uint8_t)strlen(s.pat);
      // very short patterns would match everything
      if (s.plen < 3) {
        csigErrors++;
        continue;
      }
    } else if (!strcasecmp(kind, "cid")) {
      unsigned v;
      if (sscanf(pat, "%x", &v) != 1 || v > 0xFFFF) {
        csigErrors++;
        continue;
      }
      s.kind = 4;
      s.cid = (uint16_t)v;
    } else {
      csigErrors++;
      continue;
    }
    csig[ncsig] = s;
    ncsig++;
  }
}
static void sigsLoad() {
  ncsig = 0;
  csigErrors = 0;
  if (!fsOk || !fsStore || !fsStore->exists("/sigs.txt"))
    return;
  File f = fsStore->open("/sigs.txt", FILE_READ);
  if (!f)
    return;
  size_t sz = f.size();
  if (sz > 6000)
    sz = 6000;
  char* buf = (char*)malloc(sz + 1);
  if (!buf) {
    f.close();
    return;
  }
  int got = f.read((uint8_t*)buf, (int)sz);
  if (got < 0)
    got = 0;
  buf[got] = 0;
  f.close();
  sigsParse(buf);
  free(buf);
  Serial.printf("custom signatures: %d rules (%d bad lines)\n", ncsig, csigErrors);
}
static bool customMatchWifi(const uint8_t* mac, const char* ssid, Family* fam, Conf* conf) {
  int n = ncsig;
  for (int i = 0; i < n; i++) {
    const CustomSig& s = csig[i];
    if ((s.kind == 1 && memcmp(mac, s.oui, 3) == 0) || (s.kind == 2 && ssid && ssid[0] && containsCI(ssid, s.pat))) {
      *fam = s.fam;
      *conf = s.conf;
      return true;
    }
  }
  return false;
}
static bool customMatchBle(const uint8_t* mac, const char* name, uint16_t cid, Family* fam, Conf* conf) {
  int n = ncsig;
  for (int i = 0; i < n; i++) {
    const CustomSig& s = csig[i];
    if ((s.kind == 1 && memcmp(mac, s.oui, 3) == 0) || (s.kind == 3 && name && name[0] && containsCI(name, s.pat)) ||
        (s.kind == 4 && cid && cid == s.cid)) {
      *fam = s.fam;
      *conf = s.conf;
      return true;
    }
  }
  return false;
}
// ---- Drone Remote ID decoding (ASTM F3411 / OpenDroneID) -------------------
// One 25-byte message: byte0 = type<<4 | version. Type 0 Basic ID (UAS id, 20 chars), 1 Location, 4 System (operator location), 0xF message pack.
static void ridMsg(const uint8_t* m, RidInfo* o) {
  uint8_t type = m[0] >> 4;
  if (type == 0) {
    char id[21];
    memcpy(id, m + 2, 20);
    id[20] = 0;
    for (int i = 0; i < 20; i++)
      if ((uint8_t)id[i] < 32 || (uint8_t)id[i] > 126) {
        id[i] = 0;
        break;
      }
    if (id[0]) {
      memcpy(o->id, id, 21);
      o->flags |= 1;
    }
  } else if (type == 1) {
    int32_t lat = (int32_t)((uint32_t)m[5] | (uint32_t)m[6] << 8 | (uint32_t)m[7] << 16 | (uint32_t)m[8] << 24);
    int32_t lon = (int32_t)((uint32_t)m[9] | (uint32_t)m[10] << 8 | (uint32_t)m[11] << 16 | (uint32_t)m[12] << 24);
    if (lat || lon) {
      o->lat = lat;
      o->lon = lon;
      o->flags |= 2;
      // geodetic altitude: value*0.5 - 1000 m (0xFFFF = unknown)
      uint16_t ga = (uint16_t)(m[15] | (m[16] << 8));
      o->alt = ga == 0xFFFF ? 0 : (int16_t)(ga / 2 - 1000);
      // speed multiplier flag
      uint16_t sp10 = (m[1] & 1) ? (uint16_t)((m[3] * 3 + 255) * 10 / 4) : (uint16_t)(m[3] * 10 / 4);
      o->spd10 = sp10;
    }
  } else if (type == 4) {
    int32_t lat = (int32_t)((uint32_t)m[2] | (uint32_t)m[3] << 8 | (uint32_t)m[4] << 16 | (uint32_t)m[5] << 24);
    int32_t lon = (int32_t)((uint32_t)m[6] | (uint32_t)m[7] << 8 | (uint32_t)m[8] << 16 | (uint32_t)m[9] << 24);
    if (lat || lon) {
      o->opLat = lat;
      o->opLon = lon;
      o->flags |= 4;
    }
  }
}
// d points at a single message or a message pack
static bool ridDecode(const uint8_t* d, int len, RidInfo* out) {
  if (len < 25)
    return false;
  if ((d[0] >> 4) == 0xF) {
    int sz = d[1], cnt = d[2];
    if (sz != 25 || cnt < 1 || cnt > 9)
      return false;
    for (int i = 0; i < cnt; i++) {
      if (3 + (i + 1) * 25 > len)
        break;
      ridMsg(d + 3 + i * 25, out);
    }
  } else
    ridMsg(d, out);
  return out->flags != 0;
}
// ---- What the alert is about (captured under the mux by upsert) ------------
static void pendCapture(const Contact& c) {
  memcpy(pendFind.mac, c.mac, 6);
  pendFind.fam = c.fam;
  pendFind.conf = c.conf;
  pendFind.rssi = c.rssi;
  strncpy(pendFind.name, c.nick[0] ? c.nick : (c.label[0] ? c.label : famName(c.fam)), 23);
  pendFind.name[23] = 0;
  pendFind.valid = true;
}
// ---- A different tune for each group (3 notes: tone, tone/gap, tone) -------
// clang-format off
static const Tone GTUNE[CAT_N][3] = {
  {{1300, 45}, {1700, 45}, {2300, 80}},  // ALPR   rising
  {{2500, 45}, {2500, 45}, {1800, 90}},  // AXON   two high, one low
  {{1800, 45}, {0, 45}, {1800, 90}},  // CAMS   double blip
  {{2200, 45}, {1600, 45}, {2200, 80}},  // GLASS  up-down-up
  {{900, 60}, {1300, 60}, {1700, 90}},  // DRONE  low rising
  {{1600, 60}, {0, 20}, {0, 0}},  // BT     single soft
  {{2800, 45}, {2000, 45}, {2800, 80}},  // TAGS   high-low-high
  {{1400, 40}, {0, 30}, {1400, 40}},  // WIFI   two soft
  {{2000, 40}, {2400, 40}, {2000, 80}},  // ZB/TH
};
// clang-format on
// tracker following you: urgent
static const Tone SND_FOLLOW[] = {{3000, 80}, {0, 60}, {3000, 80}, {0, 60}, {3000, 80}, {0, 60}, {3000, 160}};
// siren: a possible card skimmer is in range
static const Tone SND_SKIM[] = {{2800, 120}, {1900, 120}, {2800, 120}, {1900, 120}, {2800, 120},
                                {1900, 120}, {2800, 120}, {1900, 120}, {2800, 120}, {1900, 240}};
static const Tone SND_LOWBAT[] = {{1200, 90}, {900, 140}};
// first armed group that wants this family, else any
static int groupOf(Family f) {
  for (int k = 0; k < CAT_N; k++)
    if (catOn[k] && famInCat(f, k))
      return k;
  for (int k = 0; k < CAT_N; k++)
    if (famInCat(f, k))
      return k;
  return 0;
}
// ---- Tracker "following you": seen continuously for followMin minutes ------
static uint8_t followMacs[4][6];
static uint8_t followQ = 0;  // alerts waiting for the UI loop
static void followService(uint32_t now) {
  static uint32_t last = 0;
  if ((int32_t)(now - last) < 1000)
    return;
  last = now;
  if (!followMin)
    return;
  uint32_t need = (uint32_t)followMin * 60000UL;
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < nContacts; i++) {
    Contact& c = contacts[i];
    if (c.fam != FAM_TAG || c.followAlerted)
      continue;
    // it went away: dwell time only counts while it is still around
    if ((int32_t)(now - c.lastMs) > 90000)
      continue;
    if ((int32_t)(c.lastMs - c.firstMs) >= (int32_t)need && followQ < 4) {
      c.follow = true;
      c.followAlerted = true;
      memcpy(followMacs[followQ++], c.mac, 6);
    }
  }
  portEXIT_CRITICAL(&mux);
}
// ---- Scenes (one tap arms a useful set of groups) --------------------------
// clang-format off
static const uint16_t SCENE_MASK[6] = {
  (1 << CAT_ALPR) | (1 << CAT_AXON) | (1 << CAT_CAM) | (1 << CAT_GLASS),  // STREET
  (1 << CAT_DRONE),  // AIRSPACE
  (1 << CAT_TAG) | (1 << CAT_GLASS),  // PERSONAL
  (1 << CAT_ZT),  // ZB/TH
  0,  // ALL OFF
  // SKIMMER (Bluetooth only; the skimmer alert is switched on with it)
  (1 << CAT_BT),
};
// clang-format on
static const char* SCENE_NAME[6] = {"STREET", "AIRSPACE", "PERSONAL", "ZB/TH", "ALL OFF", "SKIMMER"};
static bool sceneActive(int n) {
  return (n == 4) ? armedMask == 0 : (armedMask == SCENE_MASK[n]);
}
// ---- Label shown in lists: a type tag (WEARABLE, LIGHT, IBEACON...) for generic BT / Wi-Fi finds, else the family ----
static const char* kindLabel(const Contact& c) {
  if ((c.fam == FAM_BT || c.fam == FAM_WIFI) && btKindTag(c.method))
    return c.method;
  return famName(c.fam);
}
// ---- 4-bar signal strength icon (logical px), rssi in dBm; -127 = nothing heard ----
static void tileBars(int x, int y, int rssi, uint16_t col) {
  int lvl = rssi > -55 ? 4 : (rssi > -65 ? 3 : (rssi > -75 ? 2 : (rssi > -88 ? 1 : 0)));
  if (rssi <= -127)
    lvl = 0;
  for (int i = 0; i < 4; i++) {
    int bh = 4 + i * 3;
    rrFill(x + i * 5, y + 13 - bh, 3, bh, 1, i < lvl ? col : C_LINE);
  }
}
// =============================================================================
//  Share-portal extras (open http://192.168.4.1 on the phone that joined the hotspot)
// =============================================================================
// keep JSON valid whatever a device calls itself
static void jsonEsc(const char* in, char* out, size_t n) {
  size_t k = 0;
  for (; *in && k + 1 < n; in++) {
    unsigned char ch = (unsigned char)*in;
    out[k++] = (ch < 32 || ch == '"' || ch == '\\' || ch > 126) ? '_' : (char)ch;
  }
  out[k] = 0;
}
// Unix seconds -> calendar (Howard Hinnant's algorithm)
static void civilFromEpoch(uint32_t t, int* y, int* mo, int* d, int* h, int* mi, int* s) {
  uint32_t days = t / 86400, rem = t % 86400;
  *h = (int)(rem / 3600);
  *mi = (int)((rem % 3600) / 60);
  *s = (int)(rem % 60);
  int64_t z = (int64_t)days + 719468;
  int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  int64_t doe = z - era * 146097;
  int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int64_t yy = yoe + era * 400;
  int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  int64_t mp = (5 * doy + 2) / 153;
  *d = (int)(doy - (153 * mp + 2) / 5 + 1);
  *mo = (int)(mp < 10 ? mp + 3 : mp - 9);
  *y = (int)(yy + (*mo <= 2));
}
static void handleApiSetTime() {
  long t = web.arg("t").toInt(), off = web.arg("o").toInt();
  if (t < 1700000000L || off < -840 || off > 840) {
    web.send(400, "text/plain", "bad time");
    return;
  }
  int y, mo, d, h, mi, s;
  civilFromEpoch((uint32_t)(t + off * 60L), &y, &mo, &d, &h, &mi, &s);
  rtcWrite(y, mo, d, h, mi, s);
  web.send(200, "text/plain", timeStr());
}
static void handleApiFinds() {
  String j;
  j.reserve(2600);
  char b[160], nm[40];
  snprintf(b, sizeof(b), "{\"time\":\"%s\",\"bat\":%d,\"scanning\":%s,\"discreet\":%s,\"n\":%d,\"finds\":[", timeStr(),
           batPct(), scanning ? "true" : "false", discreet ? "true" : "false", nContacts);
  j += b;
  int shown = 0;
  uint32_t now = millis();
  // small chunks: the loop task's stack is only 8 KB
  static Contact snap[8];
  for (int start = nContacts - 1; start >= 0 && shown < 40;) {
    int n = 0;
    portENTER_CRITICAL(&mux);
    // newest first
    for (int i = start; i >= 0 && n < 8 && start - i < 8; i--) {
      if (i < nContacts)
        snap[n++] = contacts[i];
    }
    portEXIT_CRITICAL(&mux);
    start -= 8;
    for (int i = 0; i < n && shown < 40; i++) {
      const Contact& c = snap[i];
      if (shown++)
        j += ",";
      char mac[20];
      macStr(c.mac, mac);
      jsonEsc(c.nick[0] ? c.nick : c.label, nm, sizeof(nm));
      snprintf(
          b, sizeof(b),
          "{\"g\":\"%s\",\"name\":\"%s\",\"mac\":\"%s\",\"rssi\":%d,\"conf\":%d,\"radio\":\"%s\",\"age\":%lu,\"follow\":%s",
          kindLabel(c), nm, mac, (int)c.rssi, (int)c.conf,
          c.radio == RAD_BLE ? "BLE" : (c.radio == RAD_ZB ? "802.15.4" : "WIFI"),
          (unsigned long)((now - c.lastMs) / 1000), c.follow ? "true" : "false");
      j += b;
      if (c.rid.flags & 2) {
        snprintf(b, sizeof(b), ",\"lat\":%.6f,\"lon\":%.6f,\"alt\":%d", c.rid.lat / 1e7, c.rid.lon / 1e7,
                 (int)c.rid.alt);
        j += b;
      }
      if (c.rid.flags & 4) {
        snprintf(b, sizeof(b), ",\"olat\":%.6f,\"olon\":%.6f", c.rid.opLat / 1e7, c.rid.opLon / 1e7);
        j += b;
      }
      j += "}";
    }
  }
  j += "]}";
  web.send(200, "application/json", j);
}
static void handleLive() {
  String h;
  h.reserve(3200);
  h += PAGE_HEAD;
  h +=
      "<h2><b>HUNTR</b>/SEEKR <span style='color:#8b94a2;font-size:15px'>live</span></h2><div class=s id=st>connecting...</div><div id=l></div>";
  h +=
      "<div class=s style='margin-top:14px'>Wi-Fi scanning pauses while this hotspot is on; Bluetooth keeps scanning. Everything already found stays listed.</div>";
  h +=
      "<script>const l=document.getElementById('l'),st=document.getElementById('st');"
      "async function tick(){try{const r=await fetch('/api/finds');const j=await r.json();"
      "st.textContent=(j.time||'clock not set')+'   '+j.n+' finds   battery '+(j.bat<0?'--':j.bat+'%')+'   '+(j.scanning?'scanning':'paused')+(j.discreet?'   discreet':'');"
      "l.innerHTML=j.finds.map(f=>{let x='<div class=c><div><b>'+f.g+'</b> '+f.name+(f.follow?' <span class=w>FOLLOWING</span>':'')+"
      "'<div class=z>'+f.mac+'  '+f.radio+'  '+f.age+'s ago'+(f.lat!==undefined?'<br>drone '+f.lat+', '+f.lon+' at '+f.alt+' m':'')+(f.olat!==undefined?'<br>pilot '+f.olat+', '+f.olon:'')+'</div></div>'+"
      "'<div style=\"text-align:right\"><b>'+f.rssi+'</b><div class=z>dBm</div></div></div>';return x}).join('')}catch(e){st.textContent='offline'}}"
      "setInterval(tick,1500);tick();"
      "fetch('/api/settime?t='+Math.floor(Date.now()/1000)+'&o='+(-new Date().getTimezoneOffset()));</script>";
  web.send(200, "text/html", h);
}
static void handleSigsForm() {
  String h;
  h.reserve(3600);
  h += PAGE_HEAD;
  h +=
      "<h2><b>Signatures</b></h2><div class=s>extra devices to look for. One rule per line: kind pattern GROUP [low|med|high]</div>";
  h +=
      "<form method=POST action='/sigs'><textarea name=s rows=14 style='width:100%;background:#10131a;color:#eef1f5;border:1px solid #28303a;border-radius:12px;padding:10px;font:13px monospace'>";
  String cur;
  if (fsOk && fsStore && fsStore->exists("/sigs.txt")) {
    File f = fsStore->open("/sigs.txt", FILE_READ);
    if (f) {
      size_t sz = f.size();
      if (sz > 6000)
        sz = 6000;
      char* buf = (char*)malloc(sz + 1);
      if (buf) {
        int g = f.read((uint8_t*)buf, (int)sz);
        buf[g < 0 ? 0 : g] = 0;
        cur = buf;
        free(buf);
      }
      f.close();
    }
  }
  if (!cur.length())
    cur =
        "# kind    pattern      group   [confidence]\n# oui   B4:1E:52    ALPR    high     (Wi-Fi or Bluetooth address prefix)\n# ssid  MyCamNet    CAMS             (Wi-Fi name contains)\n# ble   MyTag       TAGS             (Bluetooth name contains)\n# cid   0x0171      GLASS            (Bluetooth company id)\n";
  cur.replace("<", "&lt;");
  h += cur;
  h +=
      "</textarea><div class=s>groups: ALPR AXON CAMS GLASS DRONE BT TAGS WIFI GUNSHOT BODYCAM and the camera brands (RING WYZE TAPO REOLINK EUFY NEST)</div>";
  h += "<input type=submit value='Save'> &nbsp; <a href='/'>back</a></form>";
  h += "<div class=s style='margin-top:14px'>loaded now: " + String((int)ncsig) + " rules" +
       (csigErrors ? String(", ") + String(csigErrors) + " lines ignored" : String("")) + "</div>";
  web.send(200, "text/html", h);
}
static void handleSigsSave() {
  String s = web.arg("s");
  if (s.length() > 6000) {
    web.send(400, "text/plain", "too long");
    return;
  }
  if (fsOk && fsStore) {
    File f = fsStore->open("/sigs.txt", FILE_WRITE);
    if (f) {
      f.write((const uint8_t*)s.c_str(), s.length());
      f.close();
    }
  }
  sigsLoad();
  web.sendHeader("Location", "/sigs");
  web.send(303, "text/plain", "saved");
}
// ---- firmware update over the hotspot (needs a partition layout with two app slots)
static bool otaOk = false;
static size_t otaBytes = 0;
static bool otaSlotAvailable() {
  return esp_ota_get_next_update_partition(NULL) != NULL;
}
static void handleUpdateForm() {
  String h;
  h.reserve(1800);
  h += PAGE_HEAD;
  h += "<h2><b>Update</b> firmware</h2>";
  if (!otaSlotAvailable()) {
    h +=
        "<div class=c><span class=w>This partition scheme has no update slot.</span></div><div class=s>In the Arduino IDE choose Tools > Partition Scheme > <b>Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)</b>, upload once over USB, then updates work from here.</div>";
  } else if (!(pmuVbus || batPct() >= 30)) {
    h +=
        "<div class=c><span class=w>Battery too low to update safely.</span></div><div class=s>Connect USB power or charge above 30% first.</div>";
  } else {
    h +=
        "<div class=s>Sketch > <b>Export Compiled Binary</b>, then pick the file ending in <b>.ino.bin</b> (not the bootloader or merged file). Keep the device powered until it restarts by itself.</div>";
    h +=
        "<form method=POST action='/update' enctype='multipart/form-data'><input type=file name=fw accept='.bin'> <input type=submit value='Install'></form>";
  }
  h += "<div class=s style='margin-top:14px'><a href='/'>back</a></div>";
  web.send(200, "text/html", h);
}
static void handleUpdateUpload() {
  HTTPUpload& u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    otaBytes = 0;
    otaOk = otaSlotAvailable() && (pmuVbus || batPct() >= 30) && Update.begin(UPDATE_SIZE_UNKNOWN);
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (otaOk) {
      if (Update.write(u.buf, u.currentSize) != u.currentSize)
        otaOk = false;
      otaBytes += u.currentSize;
    }
  } else if (u.status == UPLOAD_FILE_END) {
    // a real firmware image is far bigger than 100 kB
    if (otaOk)
      otaOk = Update.end(true) && otaBytes > 100000;
  }
}
static void handleUpdateDone() {
  String h;
  h += PAGE_HEAD;
  h +=
      otaOk
          ? "<h2><b>Installed.</b></h2><div class=s>Restarting now...</div>"
          : "<h2 class=w>Update failed</h2><div class=s>Nothing was changed. Check the file and try again.</div><a href='/update'>back</a>";
  web.send(200, "text/html", h);
  if (otaOk) {
    delay(400);
    ESP.restart();
  }
}
static void portalPlusRoutes() {
  web.on("/live", handleLive);
  web.on("/api/finds", handleApiFinds);
  web.on("/api/settime", handleApiSetTime);
  web.on("/sigs", handleSigsForm);
  web.on("/sigs", HTTP_POST, handleSigsSave);
  web.on("/update", handleUpdateForm);
  web.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  web.on("/bootlogo", handleLogoForm);
  web.on("/bootlogoup", HTTP_POST, handleLogoDone, handleLogoUpload);
  web.on("/bootlogoshow", handleLogoShow);
  web.on("/bootlogodel", HTTP_POST, handleLogoDel);
#if HAS_BOOTSOUND
  web.on("/boot", handleBootForm);
  web.on("/bootsound", HTTP_POST, handleBootDone, handleBootUpload);
  web.on("/bootplay", handleBootPlay);
  web.on("/bootdel", HTTP_POST, handleBootDel);
#endif
}
// Bluetooth keeps scanning while the hotspot is up, so the live view stays alive
static void portalPlusBle() {
#if ENABLE_BLE
  if (!bleBringUp())
    return;
  bleHeld = false;
  bleRunning = false;
  scanning = true;
  if (armedMask)
    bleStartScan();
#endif
}
// =============================================================================
//  Findings record (what was found this session, saved on demand)
// =============================================================================
// Every network / device found this session is kept in a compact ledger in RAM (bounded: 192 entries). It is written to ONE file when you
// tap SAVE LOG (or automatically, see SETUP > Save log). The file is rewritten, never appended to, so it cannot grow without limit.
struct LedgerEntry {
  // flags: 1 = has GPS fix, 2 = already written to the live log, 4 = has drone position
  uint8_t mac[6];
  uint8_t fam, conf, radio, ch, flags;
  int8_t best, last;
  uint16_t hits;
  uint32_t firstMs, lastMs;
  // degrees * 1e6 (position of this device), drone position * 1e7
  int32_t lat, lon, rlat, rlon;
  int16_t ralt;
  char name[22];
  char method[12];
};
#define LEDGER_N 192
static LedgerEntry ledger[LEDGER_N];
static uint16_t ledgerN = 0, ledgerDropped = 0, ledgerSavedN = 0;
static bool ledgerDirty = false, ledgerEverSaved = false, lwasScanning = false;
static char ledgerSavedAt[12] = "";
static uint32_t ledgerAutoAt = 0;
enum { SAVE_OK = 0, SAVE_NONE = 1, SAVE_NOFS = 2, SAVE_FULL = 3, SAVE_FAIL = 4 };
static bool ledgerUnsaved() {
  return ledgerN && ledgerDirty;
}
static void ledgerReset() {
  ledgerN = 0;
  ledgerDropped = 0;
  ledgerSavedN = 0;
  ledgerDirty = false;
  ledgerEverSaved = false;
  ledgerSavedAt[0] = 0;
  ledgerAutoAt = millis();
}
// true the first time a device is asked about (so the live log writes each device once)
static bool ledgerLiveMark(const uint8_t* mac) {
  for (int k = 0; k < ledgerN; k++)
    if (memcmp(ledger[k].mac, mac, 6) == 0) {
      if (ledger[k].flags & 2)
        return false;
      ledger[k].flags |= 2;
      return true;
    }
  return true;
}
// called for every hit, from the radio callbacks: must stay quick
static void ledgerNote(const Contact& c) {
  int i = -1;
  for (int k = 0; k < ledgerN; k++)
    if (memcmp(ledger[k].mac, c.mac, 6) == 0) {
      i = k;
      break;
    }
  bool fresh = false;
  if (i < 0) {
    if (ledgerN < LEDGER_N)
      i = ledgerN++;
    // full: the entry not heard for longest makes room (and the report says how many were left out)
    else {
      i = 0;
      for (int k = 1; k < LEDGER_N; k++)
        if (ledger[k].lastMs < ledger[i].lastMs)
          i = k;
      ledgerDropped++;
    }
    memset(&ledger[i], 0, sizeof(LedgerEntry));
    memcpy(ledger[i].mac, c.mac, 6);
    ledger[i].fam = (uint8_t)c.fam;
    ledger[i].radio = (uint8_t)c.radio;
    ledger[i].firstMs = millis();
    ledger[i].best = c.rssi;
    snprintf(ledger[i].method, sizeof(ledger[i].method), "%s", c.method);
    if (gpsHave) {
      ledger[i].lat = gpsLat;
      ledger[i].lon = gpsLon;
      ledger[i].flags |= 1;
    }
    fresh = true;
  }
  LedgerEntry& e = ledger[i];
  e.last = c.rssi;
  if (c.rssi > e.best)
    e.best = c.rssi;
  if (c.conf > e.conf)
    e.conf = (uint8_t)c.conf;
  e.hits = c.hits;
  e.lastMs = millis();
  e.ch = c.ch;
  const char* nm = c.nick[0] ? c.nick : c.label;
  // an SSID can show up after the first sighting
  if (fresh || (strlen(nm) > strlen(e.name) && strcmp(nm, e.name) != 0)) {
    snprintf(e.name, sizeof(e.name), "%s", nm);
    ledgerDirty = true;
  }
  if (c.rid.flags & 2) {
    e.rlat = c.rid.lat;
    e.rlon = c.rid.lon;
    e.ralt = c.rid.alt;
    e.flags |= 4;
  }
}
static uint32_t epochFromCivil(int y, int mo, int d, int h, int mi, int s) {
  y -= mo <= 2;
  int era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153u * (unsigned)(mo + (mo > 2 ? -3 : 9)) + 2) / 5 + (unsigned)d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long days = (long)era * 146097L + (long)doe - 719468L;
  return (uint32_t)(days * 86400L + h * 3600L + mi * 60L + s);
}
// a moment in the session as a clock time (if the clock is set) or as +HH:MM:SS from the start
static void tsFromMs(uint32_t ms, char* out, size_t n) {
  uint32_t nowMs = millis();
  if (rtcOk) {
    uint32_t ep = epochFromCivil(clk.y, clk.mo, clk.d, clk.h, clk.mi, clk.s) - (nowMs - ms) / 1000UL;
    int y, mo, d, h, mi, s;
    civilFromEpoch(ep, &y, &mo, &d, &h, &mi, &s);
    snprintf(out, n, "%04d-%02d-%02d %02d:%02d:%02d", y, mo, d, h, mi, s);
  } else {
    uint32_t r = (ms - sessionStartMs) / 1000UL;
    snprintf(out, n, "+%02u:%02u:%02u", (unsigned)(r / 3600), (unsigned)((r / 60) % 60), (unsigned)(r % 60));
  }
}
// quotes a field if it holds a comma, quote or line break
static void csvField(char* out, size_t n, const char* s) {
  bool q = false;
  for (const char* p = s; *p; p++)
    if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r')
      q = true;
  size_t o = 0;
  if (q && o < n - 1)
    out[o++] = '"';
  for (const char* p = s; *p && o < n - 3; p++) {
    if (*p == '"')
      out[o++] = '"';
    out[o++] = *p;
  }
  if (q && o < n - 2)
    out[o++] = '"';
  out[o] = 0;
}
// writes (replaces) the session's log file; returns a SAVE_* code
static int ledgerSave() {
  if (!ledgerN)
    return SAVE_NONE;
  if (!fsOk || !fsStore)
    return SAVE_NOFS;
  uint64_t u = 0, t = 0;
  if (fsSpace(&u, &t) && t > u && (t - u) < 40000ULL)
    return SAVE_FULL;  // always leave 40 KB free
  File f = fsStore->open(logPath, FILE_WRITE);
  if (!f)
    return SAVE_FAIL;
  f.println(
      "first_seen,last_seen,name,mac,family,type,radio,channel,best_rssi,last_rssi,hits,confidence,lat,lon,rid_lat,rid_lon,rid_alt");
  static uint8_t order[LEDGER_N];
  for (int i = 0; i < ledgerN; i++)
    order[i] = (uint8_t)i;
  for (int i = 1; i < ledgerN; i++) {
    uint8_t v = order[i];
    int j = i - 1;
    while (j >= 0 && ledger[order[j]].firstMs > ledger[v].firstMs) {
      order[j + 1] = order[j];
      j--;
    }
    order[j + 1] = v;
  }
  for (int k = 0; k < ledgerN; k++) {
    const LedgerEntry& e = ledger[order[k]];
    char t1[24], t2[24], mc[20], nm[56], la[16] = "", lo[16] = "", rl[16] = "", rn[16] = "", ra[8] = "", line[320];
    tsFromMs(e.firstMs, t1, sizeof(t1));
    tsFromMs(e.lastMs, t2, sizeof(t2));
    macStr(e.mac, mc);
    csvField(nm, sizeof(nm), e.name);
    if (e.flags & 1) {
      snprintf(la, sizeof(la), "%.6f", e.lat / 1e6);
      snprintf(lo, sizeof(lo), "%.6f", e.lon / 1e6);
    }
    if (e.flags & 4) {
      snprintf(rl, sizeof(rl), "%.6f", e.rlat / 1e7);
      snprintf(rn, sizeof(rn), "%.6f", e.rlon / 1e7);
      snprintf(ra, sizeof(ra), "%d", (int)e.ralt);
    }
    snprintf(line, sizeof(line), "%s,%s,%s,%s,%s,%s,%s,%u,%d,%d,%u,%s,%s,%s,%s,%s,%s\n", t1, t2, nm, mc,
             famName((Family)e.fam), e.method, e.radio == RAD_BLE ? "BLE" : (e.radio == RAD_ZB ? "ZB" : "WIFI"),
             (unsigned)e.ch, (int)e.best, (int)e.last, (unsigned)e.hits,
             e.conf >= CONF_HIGH ? "HIGH" : (e.conf == CONF_MED ? "MED" : "LOW"), la, lo, rl, rn, ra);
    f.print(line);
  }
  f.close();
  ledgerSavedN = ledgerN;
  ledgerDirty = false;
  ledgerEverSaved = true;
  ledgerAutoAt = millis();
  if (rtcOk)
    snprintf(ledgerSavedAt, sizeof(ledgerSavedAt), "%02u:%02u", clk.h, clk.mi);
  else
    snprintf(ledgerSavedAt, sizeof(ledgerSavedAt), "just now");
  return SAVE_OK;
}
// Auto mode: refresh the file every 5 minutes (if anything changed) and when scanning stops
static void ledgerService(uint32_t now) {
  bool sc = scanning && armedMask != 0;
  if (logMode == 1 && ledgerN && ledgerDirty) {
    bool stopped = lwasScanning && !sc;
    if (stopped || (int32_t)(now - ledgerAutoAt) > 300000) {
      int r = ledgerSave();
      ledgerAutoAt = now;
      if (r == SAVE_FULL)
        logNotice = "Storage full: log not saved";
      else if (r != SAVE_OK)
        logNotice = "Log could not be saved";
    }
  }
  lwasScanning = sc;
}
// removes every skiscan_* file (reports, live logs, tracking logs); settings files are not touched
static int logDeleteAll() {
  if (!fsOk || !fsStore)
    return 0;
  static char names[48][40];
  int n = 0;
  File root = fsStore->open("/");
  if (root) {
    File e = root.openNextFile();
    while (e && n < 48) {
      const char* raw = e.name();
      const char* b = strrchr(raw, '/');
      b = b ? b + 1 : raw;
      if (strncmp(b, "skiscan_", 8) == 0) {
        snprintf(names[n], sizeof(names[n]), "/%s", b);
        n++;
      }
      e.close();
      e = root.openNextFile();
    }
    root.close();
  }
  for (int i = 0; i < n; i++)
    fsStore->remove(names[i]);
  logFileMade = false;
  huntFileMade = false;
  logRows = 0;
  logFull = false;
  ledgerEverSaved = false;
  ledgerSavedN = 0;
  ledgerSavedAt[0] = 0;
  ledgerDirty = ledgerN > 0;
  return n;
}
#if HAS_BOOTSOUND
// =============================================================================
//  Boot sound (WATCH): your own short sound on the boot screen
// =============================================================================
// The phone page converts any audio file to 16 kHz mono 16-bit WAV (first 6 s) in the browser and uploads it here; it is kept as /bootsound.wav
// and streamed from flash while it plays. SETUP > Boot sound chooses Off / Chime (the built-in tune) / Custom.
// length of the stored sound in ms (0 = none / unusable), -1 = not checked yet
static int32_t bsMsCache = -1;
static int32_t bsInfoMs() {
  if (bsMsCache >= 0)
    return bsMsCache;
  bsMsCache = 0;
  if (!fsOk || !fsStore || !fsStore->exists(BS_PATH))
    return 0;
  File f = fsStore->open(BS_PATH, FILE_READ);
  if (!f)
    return 0;
  uint8_t h[44];
  uint32_t dl = 0;
  // 32 bytes per millisecond
  if (f.read(h, 44) == 44 && bootSoundHeaderCheck(h, (uint32_t)f.size(), &dl) == 0)
    bsMsCache = (int32_t)(dl / 32);
  f.close();
  return bsMsCache;
}
static void bootSoundLabel(char* out, size_t n) {
  if (bootSoundMode == 0)
    snprintf(out, n, "Off");
  else if (bootSoundMode == 1)
    snprintf(out, n, "Chime");
  else {
    int32_t ms = bsInfoMs();
    if (ms > 0)
      snprintf(out, n, "Custom %d.%d s", (int)(ms / 1000), (int)((ms % 1000) / 100));
    else
      snprintf(out, n, "Chime");
  }
}
static void bootSoundPlay() {  // called once at start-up
  bootHoldMs = 0;
  if (bootSoundMode == 0)
    return;
  if (bootSoundMode == 2 && audioReady && soundOn && !discreet) {
    int32_t ms = bsInfoMs();
    // keep the boot screen up until the sound is done
    if (ms > 0) {
      audioPlayFile();
      bootHoldMs = ms > 1700 ? (uint32_t)(ms - 1700) : 0;
      return;
    }
  }
  // no custom sound (or it cannot be read): the built-in chime
  sndPlay(SND_BOOT, 3);
}
// SETUP > Boot sound: Off -> Chime -> Custom (only if one is stored) -> Off
static void bootSoundCycle() {
  uint8_t m = (uint8_t)((bootSoundMode + 1) % 3);
  if (m == 2 && bsInfoMs() <= 0)
    m = 0;
  bootSoundMode = m;
  savePrefs();
  audioStopFile();
  char b[20];
  bootSoundLabel(b, sizeof(b));
  char t[40];
  snprintf(t, sizeof(t), "Boot sound: %s", b);
  showToast(t);
  if (m == 1)
    sndPlay(SND_BOOT, 3);
  else if (m == 2)
    audioPlayFile();  // a preview of what you chose
  needRender = true;
}
// ---- the phone page
static bool bsUpOk = false;
static const char* bsUpErr = nullptr;
static File bsUpFile;
static bool bsUpOpen = false;
static uint8_t bsUpHdr[44];
static int bsUpN = 0;
static uint32_t bsUpLen = 0, bsUpGot = 0;
static void handleBootForm() {
  String h;
  h.reserve(4200);
  h += PAGE_HEAD;
  h += "<h2><b>Boot</b> sound</h2>";
  int32_t ms = bsInfoMs();
  char cur[80];
  if (ms > 0)
    snprintf(cur, sizeof(cur), "Stored: %d.%d seconds (%d KB).  Boot sound setting: %s", (int)(ms / 1000),
             (int)((ms % 1000) / 100), (int)((ms * 32 + 44) / 1024),
             bootSoundMode == 2 ? "Custom" : (bootSoundMode == 1 ? "Chime" : "Off"));
  else
    snprintf(cur, sizeof(cur), "No custom sound stored: the built-in chime plays.");
  h += "<div class=c><span>";
  h += cur;
  h += "</span></div>";
  h +=
      "<div class=s>Pick any short audio file (MP3, WAV, M4A, OGG ...). It is converted here, in your browser, to what the watch plays: mono, 16 kHz, first 6 seconds. Then Upload.</div>";
  h +=
      R"BSJS(<input type=file id=f accept="audio/*,.wav,.mp3,.m4a,.ogg"><div class=s id=info>no file chosen</div><audio id=pv controls style="width:100%;display:none"></audio>
<div style="margin:10px 0"><button id=up disabled>Upload to the watch</button></div>
<script>
const RATE=16000,MAXS=6;let blob=null;const $=i=>document.getElementById(i);
function wav(f){const n=f.length,b=new ArrayBuffer(44+n*2),v=new DataView(b);const w=(o,s)=>{for(let i=0;i<s.length;i++)v.setUint8(o+i,s.charCodeAt(i))};
w(0,'RIFF');v.setUint32(4,36+n*2,true);w(8,'WAVE');w(12,'fmt ');v.setUint32(16,16,true);v.setUint16(20,1,true);v.setUint16(22,1,true);v.setUint32(24,RATE,true);v.setUint32(28,RATE*2,true);v.setUint16(32,2,true);v.setUint16(34,16,true);w(36,'data');v.setUint32(40,n*2,true);
for(let i=0;i<n;i++){const s=Math.max(-1,Math.min(1,f[i]));v.setInt16(44+i*2,s<0?s*32768:s*32767,true)}return new Blob([b],{type:'audio/wav'})}
$('f').onchange=async()=>{const file=$('f').files[0];blob=null;$('up').disabled=true;if(!file)return;$('info').textContent='converting...';
try{const ac=new (window.AudioContext||window.webkitAudioContext)();const buf=await ac.decodeAudioData(await file.arrayBuffer());const dur=Math.min(buf.duration,MAXS),n=Math.max(1600,Math.floor(dur*RATE));
const oc=new OfflineAudioContext(1,n,RATE),src=oc.createBufferSource();src.buffer=buf;src.connect(oc.destination);src.start(0);const out=(await oc.startRendering()).getChannelData(0);
let pk=0;for(let i=0;i<n;i++)pk=Math.max(pk,Math.abs(out[i]));const g=pk>0?Math.min(0.95/pk,4):1;const fo=Math.floor(RATE*0.04);for(let i=0;i<n;i++){let s=out[i]*g;if(n-i<fo)s*=(n-i)/fo;out[i]=s}
blob=wav(out);$('info').textContent=dur.toFixed(1)+' s, '+Math.round(blob.size/1024)+' KB'+(buf.duration>MAXS?' (trimmed from '+buf.duration.toFixed(1)+' s)':'')+(pk>0?'':' - silent?');
$('pv').src=URL.createObjectURL(blob);$('pv').style.display='block';$('up').disabled=false}catch(e){$('info').textContent='This file could not be read as audio.'}};
$('up').onclick=async()=>{if(!blob)return;$('up').disabled=true;$('info').textContent='uploading...';const fd=new FormData();fd.append('f',blob,'bootsound.wav');
try{const r=await fetch('/bootsound',{method:'POST',body:fd});const t=await r.text();$('info').textContent=t;if(t.indexOf('OK')==0)setTimeout(()=>location.reload(),1800);else $('up').disabled=false}catch(e){$('info').textContent='Upload failed: connection lost';$('up').disabled=false}};
</script>)BSJS";
  h += "<div class=c><a href='/bootplay'>Play it on the watch</a>";
  if (ms > 0)
    h +=
        "<form method=POST action='/bootdel' onsubmit=\"return confirm('Delete the custom boot sound?')\" style='margin:0'><button class=w>Delete</button></form>";
  h += "</div><div class=s style='margin-top:14px'><a href='/'>back</a></div>";
  web.send(200, "text/html", h);
}
static void handleBootUpload() {
  HTTPUpload& u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    bsUpErr = nullptr;
    bsUpOk = false;
    bsUpN = 0;
    bsUpGot = 0;
    bsUpLen = 0;
    if (bsUpOpen) {
      bsUpFile.close();
      bsUpOpen = false;
    }
    if (!fsOk || !fsStore)
      bsUpErr = "No storage on this device";
    audioStopFile();
  } else if (u.status == UPLOAD_FILE_WRITE && !bsUpErr) {
    size_t i = 0;
    while (bsUpN < 44 && i < u.currentSize)
      bsUpHdr[bsUpN++] = u.buf[i++];
    // header complete: decide before touching the flash
    if (bsUpN == 44 && !bsUpOpen && bsUpGot == 0) {
      int r = bootSoundHeaderCheck(bsUpHdr, 0, &bsUpLen);
      if (r)
        bsUpErr = BS_ERR[r];
      else {
        uint64_t used = 0, tot = 0;
        size_t oldSz = 0;
        if (fsStore->exists(BS_PATH)) {
          File o = fsStore->open(BS_PATH, FILE_READ);
          if (o) {
            oldSz = o.size();
            o.close();
          }
        }
        if (fsSpace(&used, &tot) && tot > used && (tot - used + oldSz) < (uint64_t)(44 + bsUpLen + 40000))
          bsUpErr = "Not enough free storage (delete logs first)";
      }
      if (!bsUpErr) {
        if (fsStore->exists(BS_PATH))
          fsStore->remove(BS_PATH);
        bsUpFile = fsStore->open(BS_PATH, FILE_WRITE);
        if (!bsUpFile)
          bsUpErr = "Could not write the file";
        else {
          bsUpOpen = true;
          bsUpFile.write(bsUpHdr, 44);
          bsUpGot = 44;
        }
      }
    }
    if (!bsUpErr && bsUpOpen && i < u.currentSize) {
      size_t n = u.currentSize - i;
      if (bsUpGot + n > 44UL + bsUpLen)
        bsUpErr = BS_ERR[5];
      else {
        bsUpFile.write(u.buf + i, n);
        bsUpGot += (uint32_t)n;
      }
    }
    if (bsUpErr && bsUpOpen) {
      bsUpFile.close();
      bsUpOpen = false;
      fsStore->remove(BS_PATH);
    }
  } else if (u.status == UPLOAD_FILE_END) {
    if (bsUpOpen) {
      bsUpFile.close();
      bsUpOpen = false;
    }
    if (!bsUpErr && bsUpN < 44)
      bsUpErr = BS_ERR[1];
    else if (!bsUpErr && bsUpGot != 44UL + bsUpLen) {
      bsUpErr = "Upload was cut short";
      fsStore->remove(BS_PATH);
    }
    bsUpOk = !bsUpErr;
    bsMsCache = -1;
  }
}
static void handleBootDone() {
  if (bsUpOk) {
    bootSoundMode = 2;
    savePrefs();  // you uploaded it: use it
    char m[100];
    snprintf(m, sizeof(m), "OK - saved %d.%d s. The boot sound is now set to Custom.", (int)(bsInfoMs() / 1000),
             (int)((bsInfoMs() % 1000) / 100));
    web.send(200, "text/plain", m);
  } else {
    String m = "Not saved: ";
    m += bsUpErr ? bsUpErr : "unknown error";
    web.send(200, "text/plain", m);
  }
}
static void handleBootPlay() {
  audioPlayFile();
  web.sendHeader("Location", "/boot");
  web.send(303, "text/plain", "ok");
}
static void handleBootDel() {
  audioStopFile();
  if (fsOk && fsStore && fsStore->exists(BS_PATH))
    fsStore->remove(BS_PATH);
  bsMsCache = -1;
  if (bootSoundMode == 2) {
    bootSoundMode = 1;
    savePrefs();
  }
  web.sendHeader("Location", "/boot");
  web.send(303, "text/plain", "ok");
}
#endif
// =============================================================================
//  Boot logo (all boards): your own picture on the boot screen
// =============================================================================
// Two sources, both streamed from flash row by row while the boot screen is drawn (nothing is held in RAM):
//   /bootlogo.bin  made by the phone page (any picture, converted in the browser): "BLG1", width, height (16-bit each), then RGB565 rows
//   /boot.bmp      a plain 24-bit uncompressed Windows BMP (uploaded from the home page)
// SETUP > Boot logo chooses Default (radar + HUNTR/SEEKR) or Custom. A picture bigger than the room is scaled down to fit, never up.
#define LOGO_PATH_BIN "/bootlogo.bin"
#define LOGO_PATH_BMP "/boot.bmp"
static LogoInfo logoCache;
static bool logoChecked = false;
// largest picture the page will make (physical pixels): fits the narrow side of every board
static int logoMaxDim() {
  return imin(280, PANEL_W - 8);
}
static const char* const LG_ERR[] = {"", "Not a boot logo file", "Picture too big for this screen", "Picture too small",
                                     "File size does not match its header"};
static uint32_t lgLe32(const uint8_t* p) {
  return p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint16_t lgLe16(const uint8_t* p) {
  return (uint16_t)(p[0] | (p[1] << 8));
}
// 0 = ok, else an index into LG_ERR
static int bootLogoBinCheck(const uint8_t* h, uint32_t fileSize, uint16_t* w, uint16_t* hh) {
  if (memcmp(h, "BLG1", 4))
    return 1;
  uint16_t ww = lgLe16(h + 4), hg = lgLe16(h + 6);
  if (ww < 8 || hg < 8)
    return 3;
  if (ww > logoMaxDim() || hg > logoMaxDim())
    return 2;
  if (fileSize && fileSize != 8UL + (uint32_t)ww * hg * 2)
    return 4;
  if (w)
    *w = ww;
  if (hh)
    *hh = hg;
  return 0;
}
static bool bootLogoReadInfo(const char* path, bool bmp, LogoInfo* li) {
  memset(li, 0, sizeof(*li));
  if (!fsOk || !fsStore || !fsStore->exists(path))
    return false;
  File f = fsStore->open(path, FILE_READ);
  if (!f)
    return false;
  uint8_t h[54];
  uint32_t sz = (uint32_t)f.size();
  bool ok = false;
  if (!bmp) {
    if (f.read(h, 8) == 8) {
      uint16_t w = 0, hg = 0;
      if (bootLogoBinCheck(h, sz, &w, &hg) == 0) {
        li->ok = true;
        li->bmp = false;
        li->w = w;
        li->h = hg;
        li->dataOff = 8;
        ok = true;
      }
    }
  } else if (f.read(h, 54) == 54 && h[0] == 'B' && h[1] == 'M') {
    uint32_t off = lgLe32(h + 10), dib = lgLe32(h + 14);
    int32_t bw = (int32_t)lgLe32(h + 18), bh = (int32_t)lgLe32(h + 22);
    bool top = bh < 0;
    if (top)
      bh = -bh;
    if (dib >= 40 && lgLe16(h + 26) == 1 && lgLe16(h + 28) == 24 && lgLe32(h + 30) == 0 && bw >= 8 && bw <= 1024 &&
        bh >= 8 && bh <= 1024) {
      uint32_t stride = ((uint32_t)bw * 3 + 3) & ~3UL;
      if (off >= 54 && sz >= off + stride * (uint32_t)bh) {
        li->ok = true;
        li->bmp = true;
        li->w = (uint16_t)bw;
        li->h = (uint16_t)bh;
        li->dataOff = off;
        li->stride = stride;
        li->bottomUp = !top;
        ok = true;
      }
    }
  }
  f.close();
  return ok;
}
// the picture to use (cached): the page's file first, then boot.bmp
static const LogoInfo* bootLogoGet() {
  if (!logoChecked) {
    logoChecked = true;
    if (!bootLogoReadInfo(LOGO_PATH_BIN, false, &logoCache) && !bootLogoReadInfo(LOGO_PATH_BMP, true, &logoCache))
      memset(&logoCache, 0, sizeof(logoCache));
  }
  return logoCache.ok ? &logoCache : nullptr;
}
// draws the custom picture into the current strip; false = use the default boot screen
static bool bootLogoDraw() {
  if (bootLogoMode != 1)
    return false;
  const LogoInfo* li = bootLogoGet();
  if (!li)
    return false;
  // keep clear of the progress bar and the version line
  int barTop = P(H - 72), roomW = PW - 16, roomH = barTop - 16;
  if (roomW < 16 || roomH < 16)
    return false;
  int dw = li->w, dh = li->h;
  if (dw > roomW || dh > roomH) {
    float sc = fminf((float)roomW / dw, (float)roomH / dh);
    dw = imax(1, (int)(li->w * sc));
    dh = imax(1, (int)(li->h * sc));
  }
  int x0 = (PW - dw) / 2, y0 = 8 + (roomH - dh) / 2;
  int ya = imax(imax(y0, bandY), cly0), yb = imin(imin(y0 + dh, bandY + BAND_H), cly1);
  // from now on only the progress bar needs redrawing (see the main loop)
  bootLogoDrawn = true;
  if (yb <= ya)
    return true;  // this strip does not touch the picture
  File f = fsStore->open(li->bmp ? LOGO_PATH_BMP : LOGO_PATH_BIN, FILE_READ);
  if (!f)
    return false;
  // one source row: the widest BMP accepted (1024 px x 3 bytes)
  static uint8_t srcRow[1024 * 3 + 8];
  int lastSy = -1;
  for (int y = ya; y < yb; y++) {
    int sy = (int)((int32_t)(y - y0) * li->h / dh);
    if (sy >= li->h)
      sy = li->h - 1;
    if (sy != lastSy) {
      uint32_t pos = li->bmp ? li->dataOff + (uint32_t)(li->bottomUp ? (li->h - 1 - sy) : sy) * li->stride
                             : li->dataOff + (uint32_t)sy * li->w * 2;
      int need = li->bmp ? li->w * 3 : li->w * 2;
      f.seek(pos);
      // damaged file: leave the rest of the picture blank rather than crash
      if (f.read(srcRow, need) != need) {
        f.close();
        return true;
      }
      lastSy = sy;
    }
    uint16_t* dst = rowp(y);
    int xa = imax(x0, clx0), xb = imin(x0 + dw, clx1);
    for (int x = xa; x < xb; x++) {
      int sx = (int)((int32_t)(x - x0) * li->w / dw);
      dst[x] = li->bmp ? RGB(srcRow[sx * 3 + 2], srcRow[sx * 3 + 1], srcRow[sx * 3])
                       : (uint16_t)(srcRow[sx * 2] | (srcRow[sx * 2 + 1] << 8));
    }
  }
  f.close();
  return true;
}
static void bootLogoLabel(char* out, size_t n) {
  const LogoInfo* li = bootLogoGet();
  if (bootLogoMode != 1 || !li)
    snprintf(out, n, "Default");
  else
    snprintf(out, n, "Custom %dx%d", (int)li->w, (int)li->h);
}
// shows the boot screen for a moment (it ends like a normal boot screen)
static void bootLogoPreview() {
  screen = SCR_BOOT;
  bootAt = millis();
  bootHoldMs = 0;
  bootLogoDrawn = false;
  needRender = true;
}
// SETUP > Boot logo: Default <-> Custom (only if a picture is stored)
static void bootLogoCycle() {
  uint8_t m = bootLogoMode ? 0 : 1;
  if (m == 1 && !bootLogoGet()) {
    showToast("No custom logo yet: open 192.168.4.1/bootlogo");
    return;
  }
  bootLogoMode = m;
  savePrefs();
  bootLogoPreview();
}
// ---- the phone page
static bool lgUpOk = false, lgUpOpen = false;
static const char* lgUpErr = nullptr;
static File lgUpFile;
static uint8_t lgUpHdr[8];
static int lgUpN = 0;
static uint32_t lgUpTotal = 0, lgUpGot = 0;
static void handleLogoForm() {
  String h;
  h.reserve(4600);
  h += PAGE_HEAD;
  h += "<h2><b>Boot</b> logo</h2>";
  const LogoInfo* li = bootLogoGet();
  char cur[96];
  if (li)
    snprintf(cur, sizeof(cur), "Stored: %d x %d picture (%s).  Boot logo setting: %s", (int)li->w, (int)li->h,
             li->bmp ? "boot.bmp" : "bootlogo.bin", bootLogoMode == 1 ? "Custom" : "Default");
  else
    snprintf(cur, sizeof(cur), "No custom logo stored: the built-in radar and title are shown.");
  h += "<div class=c><span>";
  h += cur;
  h += "</span></div>";
  h +=
      "<div class=s>Pick any picture (PNG, JPG, BMP, GIF, WebP). It is converted here, in your browser, and shrunk to fit this screen (largest ";
  h += String(logoMaxDim());
  h += " x ";
  h += String(logoMaxDim());
  h += " px). Transparent areas become black. Then Upload.</div>";
  h += "<script>const MAXD=";
  h += String(logoMaxDim());
  h += ";</script>";
  h +=
      R"BLJS(<input type=file id=f accept="image/*,.bmp"><div class=s id=info>no file chosen</div><canvas id=cv style="display:none;max-width:100%;border:1px solid #28303a;border-radius:8px;background:#000"></canvas>
<div style="margin:10px 0"><button id=up disabled>Upload to the device</button></div>
<script>
const $=i=>document.getElementById(i);let blob=null;
async function load(file){if(window.createImageBitmap)return await createImageBitmap(file);return await new Promise((ok,no)=>{const im=new Image();im.onload=()=>ok(im);im.onerror=no;im.src=URL.createObjectURL(file)})}
$('f').onchange=async()=>{const file=$('f').files[0];blob=null;$('up').disabled=true;if(!file)return;$('info').textContent='converting...';
try{const im=await load(file),iw=im.width,ih=im.height,s=Math.min(MAXD/iw,MAXD/ih),w=Math.max(8,Math.round(iw*s)),h=Math.max(8,Math.round(ih*s));
const c=$('cv');c.width=w;c.height=h;const x=c.getContext('2d');x.fillStyle='#000';x.fillRect(0,0,w,h);x.drawImage(im,0,0,w,h);const d=x.getImageData(0,0,w,h).data;
const b=new ArrayBuffer(8+w*h*2),v=new DataView(b);'BLG1'.split('').forEach((ch,i)=>v.setUint8(i,ch.charCodeAt(0)));v.setUint16(4,w,true);v.setUint16(6,h,true);
for(let i=0,o=8;i<w*h;i++,o+=2){const r=d[i*4],g=d[i*4+1],bl=d[i*4+2];v.setUint16(o,((r&0xF8)<<8)|((g&0xFC)<<3)|(bl>>3),true)}
blob=new Blob([b]);$('info').textContent=w+' x '+h+' px, '+Math.round(blob.size/1024)+' KB'+((iw>MAXD||ih>MAXD)?' (shrunk from '+iw+' x '+ih+')':'');c.style.display='block';$('up').disabled=false}catch(e){$('info').textContent='This file could not be read as a picture.'}};
$('up').onclick=async()=>{if(!blob)return;$('up').disabled=true;$('info').textContent='uploading...';const fd=new FormData();fd.append('f',blob,'bootlogo.bin');
try{const r=await fetch('/bootlogoup',{method:'POST',body:fd});const t=await r.text();$('info').textContent=t;if(t.indexOf('OK')==0)setTimeout(()=>location.reload(),1800);else $('up').disabled=false}catch(e){$('info').textContent='Upload failed: connection lost';$('up').disabled=false}};
</script>)BLJS";
  h += "<div class=c><a href='/bootlogoshow'>Show it on the device</a>";
  if (li && !li->bmp)
    h +=
        "<form method=POST action='/bootlogodel' onsubmit=\"return confirm('Delete the custom boot logo?')\" style='margin:0'><button class=w>Delete</button></form>";
  h += "</div>";
  if (li && li->bmp)
    h +=
        "<div class=s>This picture is your uploaded <b>boot.bmp</b>. Upload a picture here to replace it, or delete boot.bmp from the home page.</div>";
  h += "<div class=s style='margin-top:14px'><a href='/'>back</a></div>";
  web.send(200, "text/html", h);
}
static void handleLogoUpload() {
  HTTPUpload& u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    lgUpErr = nullptr;
    lgUpOk = false;
    lgUpN = 0;
    lgUpGot = 0;
    lgUpTotal = 0;
    if (lgUpOpen) {
      lgUpFile.close();
      lgUpOpen = false;
    }
    if (!fsOk || !fsStore)
      lgUpErr = "No storage on this device";
  } else if (u.status == UPLOAD_FILE_WRITE && !lgUpErr) {
    size_t i = 0;
    while (lgUpN < 8 && i < u.currentSize)
      lgUpHdr[lgUpN++] = u.buf[i++];
    if (lgUpN == 8 && !lgUpOpen && lgUpGot == 0) {
      uint16_t w = 0, hg = 0;
      int r = bootLogoBinCheck(lgUpHdr, 0, &w, &hg);
      if (r)
        lgUpErr = LG_ERR[r];
      else {
        lgUpTotal = 8UL + (uint32_t)w * hg * 2;
        uint64_t used = 0, tot = 0;
        size_t oldSz = 0;
        if (fsStore->exists(LOGO_PATH_BIN)) {
          File o = fsStore->open(LOGO_PATH_BIN, FILE_READ);
          if (o) {
            oldSz = o.size();
            o.close();
          }
        }
        if (fsSpace(&used, &tot) && tot > used && (tot - used + oldSz) < (uint64_t)lgUpTotal + 40000)
          lgUpErr = "Not enough free storage (delete logs first)";
      }
      if (!lgUpErr) {
        if (fsStore->exists(LOGO_PATH_BIN))
          fsStore->remove(LOGO_PATH_BIN);
        lgUpFile = fsStore->open(LOGO_PATH_BIN, FILE_WRITE);
        if (!lgUpFile)
          lgUpErr = "Could not write the file";
        else {
          lgUpOpen = true;
          lgUpFile.write(lgUpHdr, 8);
          lgUpGot = 8;
        }
      }
    }
    if (!lgUpErr && lgUpOpen && i < u.currentSize) {
      size_t n = u.currentSize - i;
      if (lgUpGot + n > lgUpTotal)
        lgUpErr = LG_ERR[4];
      else {
        lgUpFile.write(u.buf + i, n);
        lgUpGot += (uint32_t)n;
      }
    }
    if (lgUpErr && lgUpOpen) {
      lgUpFile.close();
      lgUpOpen = false;
      fsStore->remove(LOGO_PATH_BIN);
    }
  } else if (u.status == UPLOAD_FILE_END) {
    if (lgUpOpen) {
      lgUpFile.close();
      lgUpOpen = false;
    }
    if (!lgUpErr && lgUpN < 8)
      lgUpErr = LG_ERR[1];
    else if (!lgUpErr && lgUpGot != lgUpTotal) {
      lgUpErr = "Upload was cut short";
      fsStore->remove(LOGO_PATH_BIN);
    }
    lgUpOk = !lgUpErr;
    logoChecked = false;
  }
}
static void handleLogoDone() {
  if (lgUpOk) {
    bootLogoMode = 1;
    savePrefs();  // you uploaded it: use it
    const LogoInfo* li = bootLogoGet();
    char m[100];
    snprintf(m, sizeof(m), "OK - saved %d x %d. The boot logo is now set to Custom.", li ? (int)li->w : 0,
             li ? (int)li->h : 0);
    web.send(200, "text/plain", m);
  } else {
    String m = "Not saved: ";
    m += lgUpErr ? lgUpErr : "unknown error";
    web.send(200, "text/plain", m);
  }
}
static void handleLogoShow() {
  bootLogoPreview();
  web.sendHeader("Location", "/bootlogo");
  web.send(303, "text/plain", "ok");
}
static void handleLogoDel() {
  if (fsOk && fsStore && fsStore->exists(LOGO_PATH_BIN))
    fsStore->remove(LOGO_PATH_BIN);
  logoChecked = false;
  if (bootLogoMode == 1 && !bootLogoGet()) {
    bootLogoMode = 0;
    savePrefs();
  }
  web.sendHeader("Location", "/bootlogo");
  web.send(303, "text/plain", "ok");
}
// =============================================================================
//  UI
// =============================================================================
struct Btn {
  int16_t x, y, w, h;
  uint8_t id;
  int8_t arg;
};
enum BtnId : uint8_t {
  B_NONE = 0,
  B_TILE,
  B_NAV,
  B_TAB,
  B_PILL,
  B_SLIDER,
  B_VOL,
  B_ALERT,
  B_GROUP,
  B_BANNER,
  B_SCENE,
  B_REPORT,
  B_SETROW,
  B_ORI,
  B_SND,
  B_DIM,
  B_HELP,
  B_STORE,
  B_SHARE,
  B_TRACK,
  B_TCAL,
  B_IGN,
  B_IGNCHIP,
  B_SAVE,
  B_SORT
};
static Btn btns[24];
static int nBtn = 0;
static uint8_t pressId = B_NONE;
static int8_t pressArg = 0;
static void addBtn(uint8_t id, int8_t arg, int x, int y, int w, int h) {
  // registered once per frame, only when the scene is at rest
  if (!regBtns || gOx != 0)
    return;
  int x0 = imax(x, ucx0), y0 = imax(y, ucy0), x1 = imin(x + w, ucx1), y1 = imin(y + h, ucy1);
  if (x1 <= x0 || y1 <= y0 || nBtn >= 24)
    return;
  btns[nBtn++] = {(int16_t)x0, (int16_t)y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0), id, arg};
}
static int hitBtn(int x, int y) {
  for (int i = nBtn - 1; i >= 0; i--)
    if (x >= btns[i].x && x < btns[i].x + btns[i].w && y >= btns[i].y && y < btns[i].y + btns[i].h)
      return i;
  return -1;
}
static bool isPressed(uint8_t id, int8_t arg) {
  return pressId == id && pressArg == arg;
}
// ---- geometry: portrait is the native orientation (172x320); landscape (320x172) gets a compact layout
static inline bool compact() {
  return W > H;
}  // landscape: wide and short
static inline bool tiny() {
  return H < 200;
}  // landscape on the 172 px tall panel
static inline int hdrH() {
  return tiny() ? 32 : (compact() ? 46 : 40 + SAFE_TOP);
}
static inline int navH() {
  return tiny() ? 26 : (compact() ? 30 : 34);
}
static inline int navY() {
  return H - navH() - (tiny() ? 6 : (compact() ? 12 : 10 + SAFE_BOT));
}
static inline int contentTop() {
  return hdrH() + 2;
}
static inline int contentBot() {
  return navY() - (compact() ? 4 : 8);
}
// ---- rounded-corner safety: how far in from the side must something sit, this close to the top/bottom edge?
// d = physical px from the nearest top/bottom edge
static int cornerInsetPx(int d) {
  if (d >= CORNER_R)
    return 0;
  if (d < 0)
    d = 0;
  int dy = CORNER_R - d;
  return CORNER_R - (int)sqrtf((float)(CORNER_R * CORNER_R - dy * dy));
}
// side margin so a bar/button of corner radius r at rows [y,y+h) clears the bottom corners
static int barMargin(int y, int h, int r) {
  // its bottom tangent point sits r in from the left edge
  int m1 = (cornerInsetPx(PH - P(y + h)) * 2 + 2) / 3 - r;
  int m2 = (cornerInsetPx(PH - P(y + h - r)) * 2 + 2) / 3;  // lowest point of the straight left edge
  return imax(12, imax(m1, m2) + 7);
}
// logical inset for content occupying logical rows [y, y+h)
static int safeX(int y, int h) {
  int d = imin(P(y), PH - P(y + h));  // its edge-most row decides
  return (cornerInsetPx(d) * 2 + 2) / 3 + 4;
}
static uint32_t lastTouchMs = 0;
static bool dimmed = false;
struct Scroller {
  float y = 0, v = 0, max = 0;
  bool drag = false;
};
#define N_PAGES_MAX 5
// 0..N_PAGES-1 = pager pages, then one per pushed screen
static Scroller scArr[N_PAGES_MAX + 8];
static Scroller* scp = &scArr[0];
#define sc (*scp)
static const int LIST_PITCH = 40, LIST_CARD = 36;
static char toastMsg[40] = "";
static uint32_t toastUntil = 0;  // short message above the tab bar
static int shakeIdx = -1;
static uint32_t shakeAt = 0;  // tile shake when a blocked tile is tapped
// pager: 0 ARM, 1 FINDS, 2 GROUPS, 3 LOG, 4 SETUP
#define N_PAGES 5
static float pagePos = 0;  // continuous page position (0..3)
static int pageTarget = 0;
static bool pageDragging = false;
static float pageVel = 0;
static const char* PAGE_TAB[N_PAGES] = {"ARM", "FINDS", "GROUPS", "LOG", "SETUP"};
static inline int curPage() {
  int p = pageTarget;
  return p < 0 ? 0 : (p > N_PAGES - 1 ? N_PAGES - 1 : p);
}
static float tileT[CAT_N] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static Contact vlist[48];
static int vcount = 0;
static Contact hc;
static bool hcOk = false;
static float huntEma = -90, huntShown = -90;
static uint32_t huntLastSeen = 0, huntSampleMs = 0;
static int8_t hist[60];
static int histN = 0;
static int sliderX = 20, sliderW = 100;
static char fileName[24][28];
static uint32_t fileSize[24];
static int fileN = 0;
// last time each radio delivered something
static uint32_t actW = 0, actB = 0, actZ = 0, lastW = 0, lastB = 0, lastZ = 0;
static int viewFilter = -1;  // filter used to build vlist this frame
// ---- Pill (status; tap = pause / resume) + battery -------------------------
static bool pillLive() {
  return scanning && radioOn && !hunting && armedMask != 0;
}
// right edge of the title at rest (narrow screens hide the battery icon instead of drawing over it)
static int hdrTitleRight = 0;
// right edge rx, vertical centre cy (logical)
static void drawBatteryChip(int rx, int cy) {
  int bp = batPct();
  if (bp < 0)
    return;
  int bw = 22, bh = 11, x = rx - bw - 3, y = cy - bh / 2;
  if (W < 230 && x < hdrTitleRight + 6)
    return;
  uint16_t col = pmuCharging ? C_TEAL : (bp < 15 ? C_ALERT : (bp < 35 ? C_AMBER : C_OK));
  rrFill(x, y, bw, bh, 3, C_CARD2);
  int fw = imax(2, (bw - 4) * bp / 100);
  rrFill(x + 2, y + 2, fw, bh - 4, 2, col);
  rrFill(x + bw, y + 3, 2, bh - 6, 1, C_CARD2);
}
static void drawPill() {
  bool live = pillLive();
  const char* t =
      live ? (discreet ? "QUIET" : (saverOn ? "SAVER" : "LIVE")) : (hunting ? "HUNT" : (armedMask ? "PAUSED" : "IDLE"));
  bool skim = skimPresent() && armedMask;
  if (skim)
    t = "SKIM?";
  int tw = textW(&SKI_SM, t), full = tw + 30, ph = 22;
  // narrow screens: on the ARM page the pill shrinks to a dot (morphs while paging)
  float grow = 1.0f;
  if (W < 230 && screen == SCR_MAIN)
    grow = clamp01(pagePos * 1.4f);
  int pw = 22 + (int)((full - 22) * grow);
  int y = (hdrH() - ph) / 2 + (compact() ? 0 : 2) + 2;
  int x = W - imax(12, safeX(y, ph)) - pw;
  bool hot = live || hunting;
  uint16_t fillc = skim ? mix(C_ALERT, C_BG, 40) : (hot ? mix(C_AMBER, C_BG, 34) : C_CARD);
  rrFill(x, y, pw, ph, 11, fillc);
  float pulse = live ? 0.5f + 0.5f * sinf(gNow * 0.0042f) : 0.0f;
  uint16_t dot =
      skim ? C_ALERT : (live ? mix(C_AMBER, fillc, (uint8_t)(150 + 105 * pulse)) : (hunting ? C_AMBER : C_DIM));
  int dx = x + 11 + (int)((12 - 11) * grow);
  disc(dx, y + ph / 2.0f, 3.3f, dot);
  if (live)
    ring(dx, y + ph / 2.0f, 3.3f + 4.0f * pulse, 1.2f, C_AMBER, (uint8_t)(110 * (1 - pulse)));
  if (grow > 0.55f)
    drawText(&SKI_SM, t, x + 22, y + (ph - SKI_SM.capH) / 2, hot ? C_AMBER : C_MUTE,
             (uint8_t)(255 * clamp01((grow - 0.55f) / 0.45f)));
  addBtn(B_PILL, 0, x - 4, y - 4, pw + 8, ph + 8);
  drawBatteryChip(x - 8, y + ph / 2);
}
static void drawTitle(const char* title, bool brand) {
  int y = 9;
  if (termOn) {
    int ty = tiny() ? 7 : y + (compact() ? 6 : SAFE_TOP);
    if (brand) {
      int yy = tiny() ? 9 : 12 + (compact() ? 6 : SAFE_TOP), x = imax(14, safeX(yy, SKI_MDB.capH));
      drawText(&SKI_MDB, "HUNTR", x, yy, C_AMBER);
      x += textW(&SKI_MDB, "HUNTR");
      drawText(&SKI_MDB, "/", x, yy, C_DIM);
      x += textW(&SKI_MDB, "/");
      drawText(&SKI_MDB, "SEEKR", x, yy, C_INK);
      x += textW(&SKI_MDB, "SEEKR");
      fillRect(x + 4, yy, 6, SKI_MDB.capH, C_TEAL);
      if (gOx == 0)
        hdrTitleRight = x + 10;
    } else {
      int tx = imax(14, safeX(ty, SKI_LG.capH));
      // a long title steps down a size so it clears the battery and the pill
      const SkiFont* tf = (tx + textW(&SKI_LG, ">") + 4 + textW(&SKI_LG, title) + 16 > 120) ? &SKI_MDB : &SKI_LG;
      int tyy = ty + SKI_LG.capH - tf->capH;
      drawText(tf, ">", tx, tyy, C_AMBER);
      tx += textW(tf, ">") + 4;
      drawText(tf, title, tx, tyy, C_INK);
      tx += textW(tf, title);
      fillRect(tx + 4, ty + SKI_LG.capH - 3, 9, 3, C_TEAL);
      if (gOx == 0)
        hdrTitleRight = tx + 14;
    }
    if (!tiny() && rowsVisible(ty + SKI_LG.capH + 3, 4)) {
      int ry = ty + SKI_LG.capH + 7;
      fillRect(12, ry, W - 24, 1, C_LINE);
      fillRect(12, ry - 1, 18, 3, C_AMBER);
      fillRect(W - 30, ry - 1, 18, 3, C_AMBER);
    }
    return;
  }
  if (brand) {
    int yy = tiny() ? 9 : 12 + (compact() ? 6 : SAFE_TOP);
    int x = imax(14, safeX(yy, SKI_MDB.capH));
    drawText(&SKI_MDB, "HUNTR", x, yy, C_AMBER);
    x += textW(&SKI_MDB, "HUNTR");
    drawText(&SKI_MDB, "/", x, yy, C_DIM);
    x += textW(&SKI_MDB, "/");
    drawText(&SKI_MDB, "SEEKR", x, yy, C_INK);
    if (gOx == 0)
      hdrTitleRight = x + textW(&SKI_MDB, "SEEKR");
  } else {
    int ty = tiny() ? 7 : y + (compact() ? 6 : SAFE_TOP);
    int tx = imax(14, safeX(ty, SKI_LG.capH));
    drawText(&SKI_LG, title, tx, ty, C_INK);
    if (gOx == 0)
      hdrTitleRight = tx + textW(&SKI_LG, title);
  }
}
static void drawHeader(const char* title, bool brand) {
  drawTitle(title, brand);
  drawPill();
}
// ---- Bottom bars -----------------------------------------------------------
static void drawNav(int n, const char* const* labs, int primary) {  // used by pushed screens
  int y = navY(), h = navH(), gap = 8, m = barMargin(y, h, 12);
  int w = (W - 2 * m - (n - 1) * gap) / n;
  for (int i = 0; i < n; i++) {
    int x = m + i * (w + gap);
    bool pr = isPressed(B_NAV, i);
    rrBorder(x, y, w, h, 12, pr ? RGB(44, 52, 64) : C_CARD, i == primary ? mix(C_AMBER, C_BG, 150) : C_LINE);
    textMid(&SKI_MDB, labs[i], x + w / 2, y + h / 2, i == primary ? C_AMBER : C_INK);
    addBtn(B_NAV, i, x, y, w, h);
  }
}
// tab bar: icons for every page, the active one expands into a labelled amber pill that slides with the pager
static void tabLayout(float pos, int* tx, int* tw) {
  int m = barMargin(navY(), navH(), navH() / 2), pad = 3, avail = W - 2 * m - 2 * pad;
  // narrow screen: icon-only tabs, all the same width
  bool nar = W < 230;
  int base = nar ? avail / N_PAGES : imin(64, (avail - 68) / N_PAGES);
  int extra = nar ? 0 : avail - N_PAGES * base;
  int x = m + pad;
  for (int i = 0; i < N_PAGES; i++) {
    float d = fabsf(pos - i);
    float c = 1.0f - (d > 1 ? 1 : d);
    tw[i] = base + (int)(extra * c + 0.5f);
    tx[i] = x;
    x += tw[i];
  }
}
static void drawTabIcon(int i, float cx, float cy, uint16_t col, uint16_t bg) {
  switch (i) {
    case 0:  // crosshair (arm)
      ring(cx, cy, 6.2f, 1.5f, col);
      disc(cx, cy, 2.0f, col);
      lineAA(cx, cy - 9, cx, cy - 5, 1.4f, col);
      lineAA(cx, cy + 5, cx, cy + 9, 1.4f, col);
      lineAA(cx - 9, cy, cx - 5, cy, 1.4f, col);
      lineAA(cx + 5, cy, cx + 9, cy, 1.4f, col);
      break;
    case 1:  // list (finds)
      rrFill((int)cx - 7, (int)cy - 6, 14, 2, 1, col);
      rrFill((int)cx - 7, (int)cy - 1, 10, 2, 1, col);
      rrFill((int)cx - 7, (int)cy + 4, 14, 2, 1, col);
      break;
    case 2:  // radar sweep (sweep)
      rrFill((int)cx - 7, (int)cy - 7, 6, 6, 2, col);
      rrFill((int)cx + 1, (int)cy - 7, 6, 6, 2, col);
      rrFill((int)cx - 7, (int)cy + 1, 6, 6, 2, col);
      rrFill((int)cx + 1, (int)cy + 1, 6, 6, 2, col);
      break;
    case 3:  // document (log)
      rrFill((int)cx - 6, (int)cy - 8, 12, 16, 3, col);
      rrFill((int)cx - 5, (int)cy - 7, 10, 14, 2, bg);
      rrFill((int)cx - 3, (int)cy - 3, 6, 1, 0, col);
      rrFill((int)cx - 3, (int)cy, 6, 1, 0, col);
      rrFill((int)cx - 3, (int)cy + 3, 4, 1, 0, col);
      break;
    default:  // sliders (setup)
      lineAA(cx - 7, cy - 3.5f, cx + 7, cy - 3.5f, 1.5f, col);
      lineAA(cx - 7, cy + 3.5f, cx + 7, cy + 3.5f, 1.5f, col);
      disc(cx - 2.5f, cy - 3.5f, 2.6f, col);
      disc(cx + 3, cy + 3.5f, 2.6f, col);
      break;
  }
}
static void drawTabBar(float pos) {
  int y = navY(), h = navH(), m = barMargin(y, h, h / 2), pad = 3;
  rrBorder(m, y, W - 2 * m, h, h / 2, C_CARD, C_LINE);
  int tx[N_PAGES], tw[N_PAGES];
  float p = pos < 0 ? 0 : (pos > N_PAGES - 1 ? N_PAGES - 1 : pos);
  tabLayout(p, tx, tw);
  int i0 = (int)floorf(p), i1 = i0 + 1 < N_PAGES ? i0 + 1 : i0;
  float fr = p - i0;
  int ix = (int)(tx[i0] + (tx[i1] - tx[i0]) * fr + 0.5f), iw = (int)(tw[i0] + (tw[i1] - tw[i0]) * fr + 0.5f);
  rrFill(ix, y + pad, iw, h - 2 * pad, (h - 2 * pad) / 2, C_AMBER);
  for (int i = 0; i < N_PAGES; i++) {
    float d = fabsf(p - i);
    float c = 1.0f - (d > 1 ? 1 : d);
    // selected icon: dark on the amber pill; the others: light grey
    uint16_t col = mix(C_BG, C_ICON, (uint8_t)(255 * c));
    uint16_t bgc = mix(C_AMBER, C_CARD, (uint8_t)(255 * c));
    int lw = textW(&SKI_SM, PAGE_TAB[i]);
    float total = 14 + (W < 230 ? 0 : c * (4 + lw));
    float cx0 = tx[i] + tw[i] / 2.0f - total / 2.0f;
    drawTabIcon(i, cx0 + 7, y + h / 2.0f, col, bgc);
    if (W >= 230 && c > 0.35f)
      drawText(&SKI_SM, PAGE_TAB[i], (int)(cx0 + 18), y + h / 2 - SKI_SM.capH / 2, C_BG,
               (uint8_t)(255 * clamp01((c - 0.35f) / 0.65f)));
    addBtn(B_TAB, i, tx[i], y, tw[i], h);
  }
}
// ---- Tiles (ARM page) ------------------------------------------------------
// the Zigbee/Thread tile only exists where there is an 802.15.4 radio
#define N_TILES (HAS_ZB ? CAT_N : CAT_N - 1)
// the Zigbee/Thread group spans the last row
static bool tileWide(int i) {
  return HAS_ZB && i == CAT_ZT && !compact();
}
static void tileRect(int i, int* x, int* y, int* w, int* h) {
  int top = contentTop(), bot = contentBot(), gap = 6, m = 12;
  if (compact()) {  // 3 x 3, single-line tiles
    int tw = (W - 2 * m - 2 * gap) / 3, th = (bot - top - 2 * gap) / 3;
    *x = m + (i % 3) * (tw + gap);
    *y = top + (i / 3) * (th + gap);
    *w = tw;
    *h = th;
    return;
  }
  int rows = HAS_ZB ? 5 : 4;
  int tw = (W - 2 * m - gap) / 2, th = (bot - top - (rows - 1) * gap) / rows;
  if (tileWide(i)) {
    *x = m;
    *y = top + 4 * (th + gap);
    *w = W - 2 * m;
    *h = th;
    return;
  }
  *x = m + (i % 2) * (tw + gap);
  *y = top + (i / 2) * (th + gap);
  *w = tw;
  *h = th;
}
static bool tickTiles(float dt) {
  bool moving = false;
  for (int i = 0; i < CAT_N; i++) {
    float tgt = catOn[i] ? 1.0f : 0.0f;
    float d = tgt - tileT[i];
    if (fabsf(d) > 0.004f) {
      tileT[i] += d * (1 - expf(-dt * 16.0f));
      moving = true;
    } else
      tileT[i] = tgt;
  }
  return moving;
}
static void drawPageArm() {
  drawTitle("", true);
  for (int i = 0; i < N_TILES; i++) {
    int x, y, w, h;
    tileRect(i, &x, &y, &w, &h);
    bool cp = h < 40;
    // register first: hit areas must exist whatever strip we are in
    addBtn(B_TILE, i, x, y, w, h);
    if (!rowsVisible(y, h))
      continue;
    uint16_t col = (i == CAT_ZT) ? RGB(170, 255, 60) : CAT_COL[i];
    float t = tileT[i];
    uint8_t ta = (uint8_t)(t * 255);
    bool blk = tileBlocked(i);
    uint16_t fillc = mix(mix(col, C_BG, 42), C_CARD, ta);
    uint16_t edge = mix(col, C_LINE, ta);
    if (blk) {
      fillc = RGB(24, 9, 10);
      edge = RGB(104, 36, 38);
      ta = 0;
    }
    if (isPressed(B_TILE, i))
      fillc = mix(C_INK, fillc, 26);
    int og = gOx;
    if (i == shakeIdx) {
      int32_t dtm = (int32_t)(gNow - shakeAt);
      if (dtm >= 0 && dtm < 380)
        gOx += (int)lroundf(6.0f * sinf(dtm * 0.045f) * (1.0f - dtm / 380.0f));
    }
    rrBorder(x, y, w, h, 12, fillc, edge);
    const char* lab = (tileWide(i)) ? "ZIGBEE / THREAD" : CAT_LABEL[i];
    int cnt = catOn[i] ? countCatHits(i) : 0;
    char nb[8];
    snprintf(nb, sizeof(nb), "%d", cnt);
    uint16_t labc = blk ? RGB(150, 78, 80) : mix(C_INK, C_MUTE, ta);
    if (cp) {  // one line: label left, count / dot right
      drawText(&SKI_MDB, lab, x + 9, y + (h - SKI_MDB.capH) / 2, labc);
      if (blk) {
        lineAA(x + w - 16, y + h / 2.0f - 4, x + w - 8, y + h / 2.0f + 4, 1.6f, RGB(150, 60, 62));
        lineAA(x + w - 8, y + h / 2.0f - 4, x + w - 16, y + h / 2.0f + 4, 1.6f, RGB(150, 60, 62));
      } else if (cnt > 0)
        textR(&SKI_MDB, nb, x + w - 9, y + (h - SKI_MDB.capH) / 2, col);
      else if (t > 0.02f)
        disc(x + w - 12, y + h / 2.0f, 3.2f, col, ta);
    } else {
      int mg = h < 42 ? 6 : 8;
      drawText(&SKI_MDB, lab, x + 11, y + mg, labc);
      drawText(&SKI_SM, blk ? "blocked" : (catOn[i] ? "ON" : "off"), x + 11, y + h - mg - SKI_SM.capH,
               blk ? RGB(150, 62, 64) : mix(col, C_DIM, ta));
      if (catOn[i] && !blk && w >= 80)
        tileBars(x + 11 + textW(&SKI_SM, "ON") + 9, y + h - mg - 13, bestRecentRssi(i), col);
      if (blk) {
      } else if (cnt > 0)
        textR(&SKI_LG, nb, x + w - 11, y + h - mg - SKI_LG.capH + 1, col);
      else if (t > 0.02f)
        disc(x + w - 14, y + h - 14, 3.5f, col, ta);
    }
    gOx = og;
  }
}
// ---- Find list (FINDS page + pushed filtered list) -------------------------
// the order the rows were last shown in; how many weak ones the "strong only" view is hiding
static uint8_t ordMac[48][6];
static int ordN = 0;
static uint16_t vWeak = 0;
static const char* const LV_NAME[4] = {"Strongest", "Strong only", "Newest", "A-Z"};
static const char* nameOfContact(const Contact& c) {
  return c.nick[0] ? c.nick : kindLabel(c);
}
static bool listBefore(const Contact& a, const Contact& b) {  // should row a come before row b?
  if (listView == 2)
    return a.firstMs > b.firstMs;  // newest find first
  if (listView == 3) {
    int c = strcasecmp(nameOfContact(a), nameOfContact(b));
    if (c)
      return c < 0;
    return a.ema > b.ema;
  }
  // strongest first, quantised to 5 dB so noise doesn't shuffle rows
  int ba = (int)floorf((a.ema + 100) / 5.0f), bb = (int)floorf((b.ema + 100) / 5.0f);
  if (ba != bb)
    return ba > bb;
  return a.lastMs > b.lastMs;
}
static void buildView(int filter) {  // once per frame (not per strip)
  viewFilter = filter;
  vcount = 0;
  vWeak = 0;
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < nContacts; i++)
    if (famInCat(contacts[i].fam, filter)) {
      // the selected row is never filtered away
      bool keep = listView != 1 || contacts[i].ema > -75.0f || (selValid && macEq(contacts[i].mac, selMac));
      if (keep)
        vlist[vcount++] = contacts[i];
      else
        vWeak++;
    }
  portEXIT_CRITICAL(&mux);
  // a finger is on the list (or it was just touched / is scrolled): rows stay where they are, new ones join at the end
  if (ordN && listFrozen()) {
    int p = 0;
    for (int o = 0; o < ordN && p < vcount; o++)
      for (int i = p; i < vcount; i++)
        if (macEq(vlist[i].mac, ordMac[o])) {
          if (i != p) {
            Contact t = vlist[p];
            vlist[p] = vlist[i];
            vlist[i] = t;
          }
          p++;
          break;
        }
  } else {
    for (int i = 1; i < vcount; i++) {
      Contact k = vlist[i];
      int j = i - 1;
      while (j >= 0 && listBefore(k, vlist[j])) {
        vlist[j + 1] = vlist[j];
        j--;
      }
      vlist[j + 1] = k;
    }
  }
  ordN = vcount;
  for (int i = 0; i < vcount; i++)
    memcpy(ordMac[i], vlist[i].mac, 6);
}
static void listLabel(char* out, size_t n) {
  static const char* const W4[4] = {"nearby", "strong", "newest", "A-Z"};
  snprintf(out, n, "%d %s", vcount, W4[listView & 3]);
}
static int selIndex() {
  if (!selValid)
    return -1;
  for (int i = 0; i < vcount; i++)
    if (macEq(vlist[i].mac, selMac))
      return i;
  return -1;
}
static void drawSignalBars(int x, int baseY, int bars, uint16_t col, uint8_t a) {
  for (int b = 0; b < 4; b++) {
    int bh = 4 + b * 3;
    if (b < bars)
      rrFill(x + b * 5, baseY - bh, 3, bh, 1, col, a);
    else
      rrFill(x + b * 5, baseY - bh, 3, bh, 1, C_LINE);
  }
}
static const char* radioTag(Radio r) {
  return r == RAD_BLE ? "BLE" : (r == RAD_ZB ? "15.4" : "WIFI");
}
// draws sub-header + list inside [top,bot]; filter/accent decide colours. Uses the current scroller.
static void drawFindList(int filter, uint16_t accent, int top, int bot, bool chip) {
  if (rowsVisible(top - 18, 16)) {
    char l[24];
    listLabel(l, sizeof(l));
    int sx = imax(14, safeX(top - 17, SKI_SM.capH));
    drawText(&SKI_SM, l, sx, top - 17, accent);
    // a small chevron: the label is a button (tap to change the view)
    {
      int cx3 = sx + textW(&SKI_SM, l) + 8, cy3 = top - 12;
      lineAA(cx3 - 3, cy3 - 1, cx3, cy3 + 2, 1.4f, accent);
      lineAA(cx3, cy3 + 2, cx3 + 3, cy3 - 1, 1.4f, accent);
    }
    // radio activity: green = data arrived in the last 1.5 s, amber = armed but silent, grey = not in use
    struct {
      const char* lab;
      bool armed;
      uint32_t act;
    } rd[3] = {
        {"W", wantWifiNow() && radioOn, actW}, {"B", ENABLE_BLE && radioOn, actB}, {"Z", catOn[CAT_ZT] && zbOn, actZ}};
    int rx = W - sx;
    for (int k = HAS_ZB ? 2 : 1; k >= 0; k--) {
      bool live = rd[k].armed && (int32_t)(gNow - rd[k].act) < 1500;
      uint16_t dc = !rd[k].armed ? C_LINE : (live ? C_OK : C_AMBER);
      int lw = textW(&SKI_SM, rd[k].lab);
      rx -= lw;
      drawText(&SKI_SM, rd[k].lab, rx, top - 17, rd[k].armed ? C_MUTE : C_DIM);
      rx -= 7;
      disc(rx, top - 12, live ? 3.6f : 3.0f, dc);
      rx -= 9;
    }
    // "2 hidden": tap to see / bring back what was ignored (only drawn if it fits)
    if (nIgnore) {
      char hb[20];
      snprintf(hb, sizeof(hb), "%d hidden", (int)nIgnore);
      int hw = textW(&SKI_SM, hb), hx = sx + textW(&SKI_SM, l) + 20;
      if (hx + hw < rx - 6)
        drawText(&SKI_SM, hb, hx, top - 17, C_AMBER);
    }
  }
  // the chip's hit area is registered whatever strip is being drawn
  if (nIgnore) {
    char l2[24], hb2[20];
    listLabel(l2, sizeof(l2));
    snprintf(hb2, sizeof(hb2), "%d hidden", (int)nIgnore);
    int sx2 = imax(14, safeX(top - 17, SKI_SM.capH)), dotsW = 0;
    {
      const char* labs[3] = {"W", "B", "Z"};
      for (int k = 0; k < (HAS_ZB ? 3 : 2); k++)
        dotsW += textW(&SKI_SM, labs[k]) + 16;
    }
    int hw2 = textW(&SKI_SM, hb2), hx2 = sx2 + textW(&SKI_SM, l2) + 20;
    if (hx2 + hw2 < W - sx2 - dotsW - 6)
      addBtn(B_IGNCHIP, 0, hx2 - 8, top - 22, hw2 + 16, 22);
  }
  int viewH = bot - top;
  // tap the count to change the view
  {
    char l3[24];
    listLabel(l3, sizeof(l3));
    int sx3 = imax(14, safeX(top - 17, SKI_SM.capH));
    addBtn(B_SORT, 0, sx3 - 6, top - 24, textW(&SKI_SM, l3) + 20, 24);
  }
  // a "N hidden" row always closes the list, so what was ignored is never out of reach
  bool hidShow = nIgnore > 0;
  int contentH = (vcount + (hidShow && vcount ? 1 : 0)) * LIST_PITCH;
  sc.max = contentH > viewH ? (float)(contentH - viewH) : 0;
  clipSet(0, top, W, viewH);
  int si = selIndex();
  int emptyTop = top + ((hidShow && vcount == 0) ? LIST_PITCH : 0);
  if (hidShow) {
    int y = vcount ? top + vcount * LIST_PITCH - (int)sc.y : top;
    addBtn(B_IGNCHIP, 1, 12, y, W - 26, LIST_CARD);
    if (rowsVisible(y, LIST_CARD)) {
      bool pr = isPressed(B_IGNCHIP, 1);
      rrFill(12, y, W - 26, LIST_CARD, 10, pr ? C_CARD2 : C_CARD);
      char hb[24];
      snprintf(hb, sizeof(hb), "%d hidden", (int)nIgnore);
      drawText(&SKI_MD, hb, 26, y + (LIST_CARD - SKI_MD.capH) / 2, C_AMBER);
      textR(&SKI_SM, W < 230 ? "manage" : "tap to manage", W - 40, y + (LIST_CARD - SKI_SM.capH) / 2, C_MUTE);
      lineAA(W - 32, y + LIST_CARD / 2 - 5, W - 26, y + LIST_CARD / 2, 1.8f, C_MUTE);
      lineAA(W - 26, y + LIST_CARD / 2, W - 32, y + LIST_CARD / 2 + 5, 1.8f, C_MUTE);
    }
  }
  if (vcount == 0) {
    float cx = W / 2.0f, cy = emptyTop + (bot - emptyTop) * 0.36f;
    float tt = gNow * 0.00055f;
    for (int k = 0; k < 3; k++) {
      float ph = fmodf(tt + k / 3.0f, 1.0f);
      ring(cx, cy, 8 + ph * 34, 1.6f, accent, (uint8_t)((1 - ph) * (1 - ph) * 170));
    }
    disc(cx, cy, 4.5f, accent);
    bool noWifi = armedMask && scanning && radioOn && wifiSideArmed() && framesSeen == 0 &&
                  (int32_t)(gNow - radioOnAt) > 8000 && !wifiFallback;
    // scans keep finding nothing: the radio itself is not hearing
    bool scanDeaf = armedMask && scanning && radioOn && wifiSideArmed() && wifiFallback && scanEmptyN >= 3;
    bool grpOff = filter >= 0 && !catOn[filter], grpBlk = filter >= 0 && tileBlocked(filter);
    char t1[32], weakMsg[40];
    bool weakOnly = (listView == 1 && vWeak > 0 && !grpOff);
    snprintf(weakMsg, sizeof(weakMsg), "%u weaker hidden: tap the count", (unsigned)vWeak);
    if (weakOnly)
      snprintf(t1, sizeof(t1), "No strong signals");
    else if (grpOff)
      snprintf(t1, sizeof(t1), grpBlk ? "%s is blocked" : "%s is off", CAT_LABEL[filter]);
    else if (filter >= 0)
      snprintf(t1, sizeof(t1), "Listening for %s", CAT_LABEL[filter]);
    else
      snprintf(t1, sizeof(t1), "%s", armedMask ? "Listening for signals" : "Nothing armed");
    textC(&SKI_MD, t1, (int)cx, (int)cy + 46, grpOff ? RGB(170, 90, 92) : C_INK);
    textC(&SKI_SM,
          weakOnly ? weakMsg
          : grpOff ? (grpBlk ? "turn off the other radio side first" : "arm it on the ARM page")
                   : (scanDeaf ? "Wi-Fi hears nothing: unplug USB power 10 s"
                      : noWifi ? "Wi-Fi radio: no frames yet"
                               : (armedMask ? (scanning ? "nothing found yet" : "paused") : "arm a category on ARM")),
          (int)cx, (int)cy + 64, weakOnly ? C_AMBER : (grpOff ? C_MUTE : ((noWifi || scanDeaf) ? C_AMBER : C_MUTE)));
  }
  for (int i = 0; i < vcount; i++) {
    int y = top + i * LIST_PITCH - (int)sc.y;
    if (!rowsVisible(y, LIST_CARD))
      continue;
    const Contact& c = vlist[i];
    uint16_t fc = (c.fam == FAM_BT && !strcmp(c.method, "SKIMMER")) ? C_ALERT : famColor(c.fam);
    bool on = (i == si);
    int32_t age = (int32_t)(gNow - c.lastMs);
    if (age < 0)
      age = 0;
    uint8_t fade = age < 6000 ? 255 : (age < 20000 ? 170 : 105);
    int x = 12, w = W - 26;
    if (on)
      rrBorder(x, y, w, LIST_CARD, 10, C_CARD2, fc, 1);
    else
      rrFill(x, y, w, LIST_CARD, 10, C_CARD);
    rrFill(x + 8, y + 8, 3, LIST_CARD - 16, 1, fc, fade);
    char b1[40], b2[40];
    ellipsize(&SKI_MDB, kindLabel(c), w - 76, b1, sizeof(b1));
    drawText(&SKI_MDB, b1, x + 18, y + 6, mix(C_INK, C_CARD, fade));
    const char* sub = c.nick[0] && strcmp(c.nick, kindLabel(c)) ? c.nick : c.method;
    ellipsize(&SKI_SM, sub, w - 84, b2, sizeof(b2));
    drawText(&SKI_SM, b2, x + 18, y + 21, mix(C_MUTE, C_CARD, fade));
    int rx = x + w - 10;
    int bars = sigBars(c.ema);
    drawSignalBars(rx - 18, y + 18, bars, bars >= 3 ? C_OK : (bars == 2 ? C_AMBER : C_ALERT), fade);
    char rs[8];
    snprintf(rs, sizeof(rs), "%d", (int)c.rssi);
    textR(&SKI_MD, rs, rx - 24, y + 6, mix(C_INK, C_CARD, fade));
    if (c.follow)
      textR(&SKI_SM, W < 230 ? "FOLLOW" : "FOLLOWING", rx - 24, y + 21, C_ALERT);
    else
      textR(&SKI_SM, radioTag(c.radio), rx - 24, y + 21, C_DIM);
  }
  if (sc.max > 0) {
    float frac = (float)viewH / contentH;
    int bh = imax(18, (int)(viewH * frac));
    int by = top + (int)((viewH - bh) * clamp01(sc.y / sc.max));
    rrFill(W - 5, by, 3, bh, 1, C_LINE);
  }
  clipReset();
  if (chip && si >= 0) {  // floating TRACK chip over the list
    int cw = 88, chh = 28, cx0 = (W - cw) / 2, cy0 = bot - chh - 4;
    bool pr = isPressed(B_TRACK, 0);
    rrFill(cx0, cy0, cw, chh, 14, pr ? mix(C_AMBER, C_BG, 200) : C_AMBER);
    textMid(&SKI_MDB, "TRACK", cx0 + cw / 2, cy0 + chh / 2, C_BG);
    addBtn(B_TRACK, 0, cx0, cy0, cw, chh);
  }
}
static void drawPageFinds() {
  drawTitle("Finds", false);
  drawFindList(-1, C_AMBER, contentTop() + 18, contentBot(), true);
}
// ---- Pushed, filtered list (long-press a tile) -----------------------------
static void drawScan() {
  uint16_t accent = (filterCat < 0) ? C_AMBER : (filterCat == CAT_ZT ? RGB(170, 255, 60) : catColor(filterCat));
  drawFindList(filterCat, accent, contentTop() + 18, contentBot(), false);
  drawHeader(filterCat < 0 ? "All finds" : CAT_LABEL[filterCat], false);
  const char* n[3] = {"TRACK", "BACK", scanning ? "PAUSE" : "SCAN"};
  drawNav(3, n, vcount ? 0 : -1);
}
// ---- Hunt ------------------------------------------------------------------
static const float G_A0 = 2.3562f;     // 135 deg
static const float G_SWEEP = 4.7124f;  // 270 deg
// ---- Terminal theme: the motion-tracker view of Track ----------------------
// A forward fan with range rings and a sweeping line. The blip's distance from you follows the signal strength; its bearing is NOT measured (this device has
// one antenna), so the screen says RANGE ONLY. The big digits are a rough distance guess from signal strength (open air), not a measurement.
static float termRangeM(float dbm) {
  float d = powf(10.0f, (-45.0f - dbm) / 27.0f);  // 1 m at -45 dBm, 10 m at -72, 100 m at -99
  return d < 1.0f ? 1.0f : (d > 99.0f ? 99.0f : d);
}
// one 7-segment digit (d = 0..9, anything else blank)
static void termDigit(int x, int y, int w, int h, int t, int d, uint16_t on, uint16_t off) {
  static const uint8_t SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};  // bits a b c d e f g
  uint8_t m = (d >= 0 && d <= 9) ? SEG[d] : 0;
  int mid = y + (h - t) / 2, vh = mid - y - t, vh2 = (y + h - t) - (mid + t);
  fillRect(x + t, y, w - 2 * t, t, (m & 1) ? on : off);          // a
  fillRect(x + w - t, y + t, t, vh, (m & 2) ? on : off);         // b
  fillRect(x + w - t, mid + t, t, vh2, (m & 4) ? on : off);      // c
  fillRect(x + t, y + h - t, w - 2 * t, t, (m & 8) ? on : off);  // d
  fillRect(x, mid + t, t, vh2, (m & 16) ? on : off);             // e
  fillRect(x, y + t, t, vh, (m & 32) ? on : off);                // f
  fillRect(x + t, mid, w - 2 * t, t, (m & 64) ? on : off);       // g
}
// the fan (180 degrees, straight ahead = up): glass, range rings, spokes, sweep with its fading trail
static void termScope(float cx, float cy, float R, float ph) {
  const float PI_ = 3.14159265f;
  // the glass: the upper half of a disc, one row at a time (cheaper than a full anti-aliased disc, the outer ring covers the edge)
  {
    int r0 = (int)R, x0 = (int)cx, y0 = (int)cy;
    for (int dy = 0; dy <= r0; dy++) {
      int hw = (int)sqrtf((float)(r0 * r0 - dy * dy));
      fillRect(x0 - hw, y0 - dy, 2 * hw + 1, 1, C_CARD);
    }
  }
  for (int k = 1; k <= 3; k++)
    arcBand(cx, cy, R * k / 3.0f, k == 3 ? 2.0f : 1.0f, PI_, 2 * PI_, k == 3 ? C_LINE : mix(C_LINE, C_CARD, 150));
  for (int k = -2; k <= 2; k++) {
    float a = 1.5f * PI_ + k * PI_ / 6.0f;
    lineAA(cx, cy, cx + cosf(a) * R * 0.98f, cy + sinf(a) * R * 0.98f, 1.0f, C_LINE, k == 0 ? 130 : 70);
  }
  fillRect((int)(cx - R - 3), (int)cy, (int)(2 * R + 6), 2, C_LINE);  // base line, with a tick every quarter
  for (int k = -4; k <= 4; k++)
    fillRect((int)(cx + k * R / 4.0f) - 0, (int)cy + 2, 1, k % 2 ? 3 : 5, C_LINE);
  float sa = PI_ + PI_ * ph;  // sweep: left to right, then again
  // (18 wider lines instead of 34: the same glow for about half the drawing)
  for (int k = 18; k >= 1; k--) {
    float a = sa - k * 0.030f;
    if (a < PI_)
      continue;
    float f = 1.0f - k / 19.0f;
    lineAA(cx, cy, cx + cosf(a) * R * 0.99f, cy + sinf(a) * R * 0.99f, 3.2f, C_TEAL, (uint8_t)(90.0f * f * f));
  }
  lineAA(cx, cy, cx + cosf(sa) * R, cy + sinf(sa) * R, 1.6f, C_INK);
  disc(cx, cy, 2.4f, C_TEAL);  // you
}
// rad01: 0 = on top of you, 1 = the edge
static void termBlip(float cx, float cy, float R, float ph, float rad01, float theta, uint16_t col, float flash) {
  const float PI_ = 3.14159265f;
  // how long ago the sweep passed it (fraction of a turn)
  float since = ph - (theta - PI_) / PI_;
  since -= floorf(since);
  float lit = 0.30f + 0.70f * expf(-since * 3.4f);
  if (flash > lit)
    lit = flash;
  float rr = R * (0.08f + 0.88f * rad01), bx = cx + cosf(theta) * rr, by = cy + sinf(theta) * rr;
  ring(bx, by, 6.0f + 4.0f * flash, 1.2f, col, (uint8_t)(150.0f * lit));
  disc(bx, by, 3.8f, col, (uint8_t)(255.0f * lit));
}
static void termDot(float cx, float cy, float R, float ph, float rad01, float theta, uint16_t col) {
  const float PI_ = 3.14159265f;
  float since = ph - (theta - PI_) / PI_;
  since -= floorf(since);
  float lit = 0.22f + 0.78f * expf(-since * 3.0f);
  float rr = R * (0.08f + 0.88f * rad01);
  disc(cx + cosf(theta) * rr, cy + sinf(theta) * rr, 2.3f, col, (uint8_t)(230.0f * lit));
}
static void drawHuntTerm() {
  const float PI_ = 3.14159265f;
  const Contact& c = hc;
  bool ok = hcOk;
  bool wide = W > H;
  int top = contentTop(), bot = contentBot();
  bool ridShow = ok && (c.rid.flags & 7);
  int ridLn = ridShow ? (((c.rid.flags & 1) ? 1 : 0) + ((c.rid.flags & 2) ? 1 : 0) + ((c.rid.flags & 4) ? 1 : 0)) : 0;
  uint16_t fam = ok ? famColor(c.fam) : C_TEAL;
  float norm = clamp01((huntShown + 100.0f) / 70.0f);
  int hTrend = 0;
  if (histN > 9) {
    float d = huntEma - hist[histN - 9];
    hTrend = d > 2.5f ? 1 : (d < -2.5f ? -1 : 0);
  }
  bool hPassed = false;
  uint16_t hot = huntShown > -55 ? C_ALERT : (huntShown > -72 ? C_AMBER : C_TEAL);
  float ph = fmodf(gNow * 0.00042f, 1.0f);  // 2.4 s per sweep
  int32_t sincePing = (int32_t)(gNow - huntPingAt);
  float flash = (sincePing >= 0 && sincePing < 220) ? 1.0f - sincePing / 220.0f : 0.0f;
  int mg = 12;
  char b[48], nb[8];
  snprintf(nb, sizeof(nb), "%d", (int)lroundf(huntShown));
  char mac[20] = "--";
  if (ok)
    macStr(c.mac, mac);
  float R, cx, cy;
  if (wide) {
    R = fminf((float)(bot - top - 14), (W - 96) / 2.0f);
    cx = mg - 4 + R;
    cy = bot - 6;
  } else {
    R = fminf((float)(bot - top - 16 - 8 - 58 - 14 - ridLn * 13 - (ridLn ? 6 : 0)), (W - 2 * mg) / 2.0f);
    cx = W / 2.0f;
    cy = top + 16 + R + 2;
  }
  if (R < 40)
    R = 40;
  termScope(cx, cy, R, ph);
  {
    uint8_t om[8][6];
    float oe[8];
    int on = 0;
    portENTER_CRITICAL(&mux);
    for (int i = 0; i < nContacts && on < 8; i++)
      if (!macEq(contacts[i].mac, huntMac) && (int32_t)(gNow - contacts[i].lastMs) < 20000) {
        memcpy(om[on], contacts[i].mac, 6);
        oe[on++] = contacts[i].ema;
      }
    portEXIT_CRITICAL(&mux);
    for (int i = 0; i < on; i++) {
      float th = 1.5f * PI_ + (((om[i][5] * 37 + om[i][4] * 11 + om[i][3]) & 255) / 255.0f - 0.5f) * 1.7f;
      termDot(cx, cy, R, ph, 1.0f - clamp01((oe[i] + 100.0f) / 70.0f), th, mix(C_TEAL, C_BG, 150));
    }
  }
  bool contact = ok && !huntSearching;
  if (contact) {
    // a steady, arbitrary bearing for this device: the real one is not known
    float theta = 1.5f * PI_ + (((c.mac[5] * 37 + c.mac[4] * 11 + c.mac[3]) & 255) / 255.0f - 0.5f) * 1.7f;
    for (int j = 5; j >= 1; j--) {  // where it was 1, 2 ... 5 s ago
      int ix = histN - 1 - 4 * j;
      if (ix < 0)
        continue;
      float n2 = clamp01((hist[ix] + 100.0f) / 70.0f);
      float rr = R * (0.08f + 0.88f * (1.0f - n2));
      disc(cx + cosf(theta) * rr, cy + sinf(theta) * rr, 1.9f, hot, (uint8_t)(70 - j * 10));
    }
    termBlip(cx, cy, R, ph, 1.0f - norm, theta, hot, flash);
  } else if ((gNow >> 8) & 2) {
    textC(&SKI_MDB, huntSearching ? "SEARCHING" : "NO CONTACT", (int)cx, (int)(cy - R * 0.42f), C_AMBER);
  }
  if (hPassed)
    textC(&SKI_MDB, "PASSED IT - TURN BACK", (int)cx, (int)(cy - R * 0.62f), C_AMBER);
  // ---- readouts
  int dm = (int)lroundf(termRangeM(huntShown));
  bool far99 = huntShown <= -99;
  // the CYD's 320 px wide landscape has room for bigger digits
  bool big = wide && W >= 300;
  int dw = wide ? (big ? 19 : 15) : 20, dh = wide ? (big ? 34 : 26) : 34, dt = wide ? (big ? 4 : 3) : 4;
  uint16_t dim = mix(hot, C_BG, 30);
  const char* zone = huntSearching
                         ? "SEARCHING"
                         : (hPassed ? "PASSED IT" : (huntShown > -55 ? "CLOSE" : (huntShown > -80 ? "NEAR" : "FAR")));
  const char* tt =
      huntSearching ? "" : (hPassed ? "TURN BACK" : (hTrend > 0 ? "^ HOTTER" : (hTrend < 0 ? "v COLDER" : "STEADY")));
  uint16_t tc = hPassed ? C_AMBER : (hTrend > 0 ? C_ALERT : (hTrend < 0 ? C_MUTE : C_DIM));
  if (wide) {
    int rx = (int)(cx + R) + 14, rw = W - rx - 8, y0 = top + 2;
    {
      char kb[40];
      ellipsize(&SKI_SM, ok ? kindLabel(c) : "LOST", rw, kb, sizeof(kb));
      drawText(&SKI_SM, kb, rx, y0, C_INK);
    }
    drawText(&SKI_SM, "RNG EST", rx, y0 + 14, C_MUTE);
    int dy = y0 + 27;
    termDigit(rx, dy, dw, dh, dt, far99 ? -1 : dm / 10, hot, dim);
    termDigit(rx + dw + 5, dy, dw, dh, dt, far99 ? -1 : dm % 10, hot, dim);
    drawText(&SKI_SM, "M", rx + 2 * dw + 9, dy + dh - SKI_SM.capH, C_MUTE);
    snprintf(b, sizeof(b), "%s DBM", nb);
    drawText(&SKI_MDB, b, rx, dy + dh + 8, hot);
    drawText(&SKI_SM, zone, rx, dy + dh + 26, hot);
    drawText(&SKI_SM, tt, rx, dy + dh + 40, tc);
    drawText(&SKI_SM, "RANGE ONLY", rx, dy + dh + 54, C_DIM);
    (void)rw;
  } else {
    int ty = top;
    disc(mg + 3, ty + 5, 3.2f, fam);
    ellipsize(&SKI_MDB, ok ? kindLabel(c) : "LOST", W / 2 - 12, b, sizeof(b));
    drawText(&SKI_MDB, b, mg + 12, ty, C_INK);
    if (ok && c.nick[0]) {
      ellipsize(&SKI_SM, c.nick, W / 2 - 18, b, sizeof(b));
      textR(&SKI_SM, b, W - mg, ty + 1, C_MUTE);
    }
    int ry = (int)cy + 12;
    drawText(&SKI_SM, "RNG EST", mg, ry - 2, C_MUTE);
    termDigit(mg, ry + 11, dw, dh, dt, far99 ? -1 : dm / 10, hot, dim);
    termDigit(mg + dw + 5, ry + 11, dw, dh, dt, far99 ? -1 : dm % 10, hot, dim);
    drawText(&SKI_SM, "M", mg + 2 * dw + 9, ry + 11 + dh - SKI_SM.capH, C_MUTE);
    int rx = W - mg;
    snprintf(b, sizeof(b), "%s DBM", nb);
    textR(&SKI_LG, b, rx, ry + 3, hot);
    snprintf(b, sizeof(b), "%s  %s", zone, tt);
    textR(&SKI_SM, b, rx, ry + 26, tc == C_DIM ? hot : tc);
    char pk[24];
    snprintf(pk, sizeof(pk), "PEAK %d", ok ? (int)c.peak : 0);
    textR(&SKI_SM, pk, rx, ry + 40, C_MUTE);
    int by0 = bot - 11;
    if (ridShow) {
      int ty2 = bot - ridLn * 13 + 2;
      char rb[48];
      by0 = ty2 - 14;
      if (c.rid.flags & 1) {
        snprintf(rb, sizeof(rb), "ID  %s", c.rid.id);
        ellipsize(&SKI_SM, rb, W - 24, b, sizeof(b));
        drawText(&SKI_SM, b, mg, ty2, C_INK);
        ty2 += 13;
      }
      if (c.rid.flags & 2) {
        snprintf(rb, sizeof(rb), "%.5f %.5f %dM", c.rid.lat / 1e7, c.rid.lon / 1e7, (int)c.rid.alt);
        drawText(&SKI_SM, rb, mg, ty2, C_MUTE);
        ty2 += 13;
      }
      if (c.rid.flags & 4) {
        snprintf(rb, sizeof(rb), "PILOT %.5f %.5f", c.rid.opLat / 1e7, c.rid.opLon / 1e7);
        drawText(&SKI_SM, rb, mg, ty2, C_AMBER);
      }
    }
    drawText(&SKI_SM, mac, mg, by0, C_DIM);
    textR(&SKI_SM, "RANGE ONLY", W - mg, by0, C_DIM);
  }
  drawHeader("Track", false);
  if (HAS_SOUND) {
    const char* n[2] = {"BACK", muted ? "SOUND" : "QUIET"};
    drawNav(2, n, -1);
  } else {
    const char* n[1] = {"BACK"};
    drawNav(1, n, -1);
  }
}
// ---- HUD theme: the blue HUD tracker of the Track screen -------------------
// A half-circle HUD (arcs, spokes, brackets) on a dark navy glass. Every ping a soft ring spreads out from the bottom centre; where it passes the contact an echo smudge lights up and fades.
// The red number in the notch is a rough distance guess from signal strength (open air), not a measurement. The bearing is NOT measured (one antenna): the echo sits at an arbitrary, steady angle.
// one ping every 0.83 s (the rhythm of the reference clip), however strong the signal
#define ALS_CYCLE_MS 830
// a ring segment, no end caps (angles: radians, clockwise from 3 o'clock, straight up = 1.5 pi)
static void hudArc(float cx, float cy, float r, float t, float a0, float a1, uint16_t col, uint8_t al = 255) {
  int32_t p0 = pseudoAngle(a0), p1 = pseudoAngle(a1);
  int32_t span = p1 - p0;
  if (span < 0)
    span += 16384;
  ringQ(cx * UI_SF, cy * UI_SF, r * UI_SF, t * UI_SF, col, al, false, true, p0, span);
}
// a small tilted rectangle on a ring: centre at radius r and angle th from straight up, rh = half length along the radius, tw = half width along the ring
static void hudBox(float ox, float oy, float r, float th, float rh, float tw, uint16_t col) {
  float s = sinf(th), c = cosf(th);
  float ux = s, uy = -c, vx = c, vy = s;  // outward and along the ring
  float px = ox + ux * r, py = oy + uy * r;
  float ax = px - ux * rh - vx * tw, ay = py - uy * rh - vy * tw, bx = px + ux * rh - vx * tw,
        by = py + uy * rh - vy * tw;
  float cx2 = px + ux * rh + vx * tw, cy2 = py + uy * rh + vy * tw, dx = px - ux * rh + vx * tw,
        dy = py - uy * rh + vy * tw;
  lineAA(ax, ay, bx, by, 1.0f, col);
  lineAA(bx, by, cx2, cy2, 1.0f, col);
  lineAA(cx2, cy2, dx, dy, 1.0f, col);
  lineAA(dx, dy, ax, ay, 1.0f, col);
}
// the upper half of a disc, a row at a time (only the rows of the strip being drawn)
static void hudSolidDisc(int cx, int cy, int r, int yEnd, uint16_t col) {
  int ya = imax(cy - r, cly0), yb = imin(imin(cy + 1, yEnd), cly1);
  for (int y = ya; y < yb; y++) {
    int dy = cy - y;
    int hw = (int)sqrtf((float)(r * r - dy * dy));
    fillRect(cx - hw, y, 2 * hw + 1, 1, col);
  }
}
// the glass and the line art; (ox, oy) = where the pings start: the middle of the footer's top edge
static void hudHud(float ox, float oy, float R, uint16_t bgc) {
  const float PI_ = 3.14159265f, D = PI_ / 180.0f;
  const uint16_t LN = RGB(176, 214, 242), LD = RGB(96, 140, 190);
  int yo = (int)oy;
  // the glass: nearly black ...
  fillRect((int)(ox - R - 1), (int)(oy - 1.14f * R), (int)(2 * R + 2), (int)(1.14f * R) + 1, bgc);
  // ... with a navy glow round the origin, in six steps
  for (int i = 0; i < 6; i++) {
    static const uint8_t GR[6] = {0, 2, 4, 7, 9, 12}, GG[6] = {0, 8, 14, 21, 29, 37}, GB[6] = {0, 22, 38, 54, 72, 90};
    hudSolidDisc((int)ox, yo, (int)(R * (1.12f - 0.15f * i)), yo, RGB(GR[i], GG[i], GB[i]));
  }
  const uint16_t GRID = RGB(10, 36, 84);  // a faint grid
  for (int k = 1; k <= 8; k++) {
    float yy = oy - k * R / 7.0f;
    float dyy = oy - yy;
    if (dyy >= R)
      break;
    int hw = (int)sqrtf(R * R - dyy * dyy);
    fillRect((int)ox - hw, (int)yy, 2 * hw, 1, GRID);
  }
  for (int k = -6; k <= 6; k++) {
    if (k == 0)
      continue;
    float xx = ox + k * R / 7.0f;
    float hh = sqrtf(R * R - (xx - ox) * (xx - ox));
    fillRect((int)xx, (int)(oy - hh), 1, (int)hh, GRID);
  }
  const float A0 = PI_, A1 = 2 * PI_, UP = 1.5f * PI_;
  hudArc(ox, oy, R, 1.3f, A0, A1, LN);  // outer double arc
  hudArc(ox, oy, R * 0.953f, 1.0f, A0, A1, LD);
  hudArc(ox, oy, R * 0.5f, 1.3f, A0, A1, LN);  // inner double arc
  hudArc(ox, oy, R * 0.466f, 1.0f, A0, A1, LD);
  // centre line, with the little marker at its foot and a box at the top
  lineAA(ox, oy - R * 0.36f, ox, oy - R, 1.0f, LD);
  ring(ox, oy - R * 0.36f, 3.2f, 1.0f, LN);
  hudBox(ox, oy, R * 0.977f, 0.0f, R * 0.034f + 2.0f, 3.2f, LN);
  for (int s = -1; s <= 1; s += 2) {
    float a = s * 46.0f * D, sn = sinf(a), cs = cosf(a);
    // the two long spokes
    lineAA(ox + sn * R * 0.556f, oy - cs * R * 0.556f, ox + sn * R * 1.02f, oy - cs * R * 1.02f, 1.0f, LN);
    // ... each with a cross tick
    lineAA(ox + sn * R * 1.02f - cs * 6.0f, oy - cs * R * 1.02f - sn * 6.0f, ox + sn * R * 1.02f + cs * 6.0f,
           oy - cs * R * 1.02f + sn * 6.0f, 1.0f, LN);
    hudBox(ox, oy, R * 0.977f, s * 20.0f * D, R * 0.03f + 1.0f, 2.2f, LN);  // the top band's two small marks
    // the inner ring's band marks (about 45 and 66 degrees)
    for (int q = 0; q < 2; q++) {
      float th = s * (q ? 66.0f : 45.0f) * D;
      hudBox(ox, oy, R * 0.483f, th, R * 0.03f + 1.0f, 2.4f, LN);
    }
    hudBox(ox, oy, R * 0.977f, s * 69.0f * D, R * 0.03f + 1.0f, 3.4f, LD);  // the small brackets near the outer ends
  }
  // the top band: the two arcs, closed by a short tick at each end
  for (int s = -1; s <= 1; s += 2) {
    float a = s * 23.0f * D, sn = sinf(a), cs = cosf(a);
    lineAA(ox + sn * R * 0.94f, oy - cs * R * 0.94f, ox + sn * R * 1.015f, oy - cs * R * 1.015f, 1.0f, LN);
  }
  (void)UP;
}
// t = ms since the ping; the soft ring, its thin bright trailing ring and the small rings at the origin
static void hudWave(float ox, float oy, float R, int32_t t) {
  const float PI_ = 3.14159265f;
  const uint16_t HALO = RGB(150, 176, 205);
  if (t < 0 || t > 420)
    return;
  if (t < 190) {  // the small double ring at the origin
    uint8_t al = (uint8_t)(210 - t);
    hudArc(ox, oy, R * 0.075f, 1.1f, PI_, 2 * PI_, RGB(176, 214, 242), al);
    hudArc(ox, oy, R * 0.115f, 1.1f, PI_, 2 * PI_, RGB(176, 214, 242), (uint8_t)(al * 3 / 4));
  }
  if (t <= 300) {
    float r = R * (0.28f + 0.90f * t / 290.0f);
    uint8_t fade = t > 230 ? (uint8_t)(255 - (t - 230) * 3) : 255;  // leaves the screen
    // the broad soft ring
    for (int k = -3; k <= 3; k++) {
      uint8_t al = (uint8_t)((92 - iabs(k) * 20) * fade / 255);
      if (al)
        hudArc(ox, oy, r + k * R * 0.022f, R * 0.026f, PI_, 2 * PI_, HALO, al);
    }
    float r2 = r - R * 0.22f;
    if (r2 > R * 0.1f) {
      hudArc(ox, oy, r2, 1.6f, PI_, 2 * PI_, RGB(210, 232, 250), (uint8_t)(190 * fade / 255));
      hudArc(ox, oy, r2 + 2.0f, 3.0f, PI_, 2 * PI_, HALO, (uint8_t)(60 * fade / 255));
    }
  }
}
// a thin compass-style needle with an arrowhead; inward = the head points at the centre (wrong way)
static void hudNeedle(float ox, float oy, float R, float th, float f0, float f1, uint16_t col, uint8_t al, bool inward) {
  float s = sinf(th), c = cosf(th);
  float x0 = ox + s * R * f0, y0 = oy - c * R * f0, x1 = ox + s * R * f1, y1 = oy - c * R * f1;
  lineAA(x0, y0, x1, y1, 1.3f, col, al);
  float tx = inward ? x0 : x1, ty = inward ? y0 : y1, d = inward ? -1.0f : 1.0f;
  float hl = R * 0.11f, hw = R * 0.05f;
  float bx = tx - d * s * hl, by = ty + d * c * hl;
  lineAA(tx, ty, bx - c * hw, by - s * hw, 1.3f, col, al);
  lineAA(tx, ty, bx + c * hw, by + s * hw, 1.3f, col, al);
}
// the smudge a contact leaves when the ring passes it
static void hudEcho(float ox, float oy, float R, int32_t t, float rfrac, float theta, uint8_t strength, int seed) {
  float tp = (rfrac - 0.28f) / 0.90f * 290.0f;
  if (tp < 0)
    tp = 0;
  float age = t - tp;
  if (age < 0 || age > 700)
    return;
  float k = expf(-age / 330.0f);
  uint8_t al = (uint8_t)(strength * k);
  if (al < 6)
    return;
  for (int q = 0; q < 7; q++) {
    float th = theta + (q - 3) * 0.115f + ((seed >> q) & 3) * 0.012f - 0.018f;
    float rr = R * (rfrac + (((seed >> (q + 3)) & 7) - 3.5f) * 0.012f);
    float x = ox + sinf(th) * rr, y = oy - cosf(th) * rr;
    float sz = R * (0.072f + 0.018f * ((seed >> (q + 1)) & 3));
    disc(x, y, sz, RGB(150, 160, 172), al);
  }
}
static void drawHuntHud() {
  const float PI_ = 3.14159265f, UP = 1.5f * PI_;
  const Contact& c = hc;
  bool ok = hcOk;
  bool wide = W > H;
  int top = contentTop(), bot = contentBot();
  bool ridShow = ok && (c.rid.flags & 7);
  int ridLn = ridShow ? (((c.rid.flags & 1) ? 1 : 0) + ((c.rid.flags & 2) ? 1 : 0) + ((c.rid.flags & 4) ? 1 : 0)) : 0;
  uint16_t fam = ok ? famColor(c.fam) : C_TEAL;
  float norm = clamp01((huntShown + 100.0f) / 70.0f);
  int hTrend = 0;
  if (histN > 9) {
    float d = huntEma - hist[histN - 9];
    hTrend = d > 2.5f ? 1 : (d < -2.5f ? -1 : 0);
  }
  int32_t t = (int32_t)(gNow - huntPingAt);
  char b[48], nb[8];
  snprintf(nb, sizeof(nb), "%d", (int)lroundf(huntShown));
  char mac[20] = "--";
  if (ok)
    macStr(c.mac, mac);
  // ---- layout: the footer bar is 0.36 R high, the art above it 1.14 R
  float R, ox, oy;
  if (wide) {
    R = fminf((float)(bot - top - 4) / 1.50f, (W - 100) / 2.0f);
    ox = 6 + R;
  } else {
    R = fminf((float)(bot - top - 4 - 50 - ridLn * 13) / 1.50f, (W - 6) / 2.0f);
    ox = W / 2.0f;
  }
  if (R < 50)
    R = 50;
  oy = top + 2 + 1.14f * R;
  int fh = (int)(0.36f * R);
  hudHud(ox, oy, R, RGB(0, 0, 0));
  bool contact = ok && !huntSearching;
  float rangeM = termRangeM(huntShown);
  float rfrac = clamp01(rangeM / 16.0f);
  if (rfrac < 0.14f)
    rfrac = 0.14f;
  // a steady, arbitrary bearing: the real one is not known
  float theta = (((c.mac[5] * 37 + c.mac[4] * 11 + c.mac[3]) & 255) / 255.0f - 0.5f) * 1.5f;
  if (contact) {
    int seed = c.mac[5] * 7 + c.mac[4] * 13 + c.mac[3];
    hudEcho(ox, oy, R, t, rfrac, theta, (uint8_t)(110 + 120 * norm), seed);
    // no motion sensor here, so no real bearing: the needle shows the trend instead (up = hotter, in = colder)
    if (hTrend != 0)
      hudNeedle(ox, oy, R, 0.0f, hTrend > 0 ? 0.14f : 0.40f, hTrend > 0 ? 0.90f : 0.80f, hTrend > 0 ? RGB(120, 214, 255) : C_AMBER, 200, hTrend < 0);
  }
  // everything else heard lately: small fainter echoes
  {
    uint8_t om[6][6];
    float oe[6];
    int on = 0;
    portENTER_CRITICAL(&mux);
    for (int i = 0; i < nContacts && on < 6; i++)
      if (!macEq(contacts[i].mac, huntMac) && (int32_t)(gNow - contacts[i].lastMs) < 20000) {
        memcpy(om[on], contacts[i].mac, 6);
        oe[on++] = contacts[i].ema;
      }
    portEXIT_CRITICAL(&mux);
    for (int i = 0; i < on; i++) {
      float th = (((om[i][5] * 37 + om[i][4] * 11 + om[i][3]) & 255) / 255.0f - 0.5f) * 1.5f;
      float rf = clamp01(termRangeM(oe[i]) / 16.0f);
      if (rf < 0.14f)
        rf = 0.14f;
      float tp = (rf - 0.28f) / 0.90f * 290.0f;
      if (tp < 0)
        tp = 0;
      float age = t - tp;
      if (age >= 0 && age < 700) {
        float k = expf(-age / 330.0f);
        disc(ox + sinf(th) * R * rf, oy - cosf(th) * R * rf, R * 0.04f, RGB(120, 132, 146), (uint8_t)(120 * k));
      }
    }
  }
  hudWave(ox, oy, R, t);
  if (!contact && ((gNow >> 8) & 2))
    textC(&SKI_MDB, huntSearching ? "SEARCHING" : "NO CONTACT", (int)ox, (int)(oy - R * 0.55f), RGB(150, 214, 255));
  // ---- footer: the blue bar, the notch and the red range
  int fx = (int)(ox - R), fw = (int)(2 * R), fy = (int)oy;
  fillRect(fx, fy, fw, fh, RGB(0, 148, 255));
  int nw = (int)(0.50f * R), nh = (int)(0.27f * R) + 4;
  rrFillP((int)ox - nw / 2, fy - 1, nw, nh + 1, 6, RGB(4, 12, 44), 255);
  fillRect((int)ox - nw / 2, fy - 1, nw, 6, RGB(4, 12, 44));
  {
    bool far99 = !contact || huntShown <= -99;
    int whole = far99 ? 0 : (int)rangeM, dec = far99 ? 0 : (int)((rangeM - (int)rangeM) * 100.0f);
    char big[6], sm[6];
    snprintf(big, sizeof(big), "%d", whole);
    snprintf(sm, sizeof(sm), "%02d", dec);
    const uint16_t RED = RGB(255, 36, 36);
    int bw = textW(&SKI_LG, big), tw = bw + 3 + textW(&SKI_SM, sm);
    int x0 = (int)ox - tw / 2, ty = fy + 2;
    drawText(&SKI_LG, big, x0, ty, RED);
    drawText(&SKI_SM, sm, x0 + bw + 3, ty, RED);
    drawText(&SKI_SM, "m", x0 + bw + 3, ty + SKI_SM.capH + 3, RED);
  }
  {
    snprintf(b, sizeof(b), "SIG %s DBM", nb);
    drawText(&SKI_SM, b, fx + 5, fy + fh / 2 - SKI_SM.capH / 2 + 1, RGB(255, 255, 255));
  }
  {
    const char* kl = ok ? kindLabel(c) : "LOST";
    char kb[24];
    ellipsize(&SKI_SM, kl, (int)(R * 0.62f), kb, sizeof(kb));
    textR(&SKI_SM, kb, fx + fw - 5, fy + fh / 2 - SKI_SM.capH / 2 + 1, RGB(255, 255, 255));
  }
  // ---- the rest
  const char* zone = huntSearching ? "SEARCHING" : (huntShown > -55 ? "CLOSE" : (huntShown > -80 ? "NEAR" : "FAR"));
  const char* tt = huntSearching ? "" : (hTrend > 0 ? "^ HOTTER" : (hTrend < 0 ? "v COLDER" : "STEADY"));
  uint16_t hot = huntShown > -55 ? C_INK : (huntShown > -72 ? C_AMBER : C_TEAL);
  if (wide) {
    int rx = fx + fw + 10, y0 = top + 2;
    {
      char kb[40];
      ellipsize(&SKI_SM, ok ? kindLabel(c) : "LOST", W - rx - 4, kb, sizeof(kb));
      drawText(&SKI_SM, kb, rx, y0, C_INK);
    }
    if (ok && c.nick[0]) {
      ellipsize(&SKI_SM, c.nick, W - rx - 4, b, sizeof(b));
      drawText(&SKI_SM, b, rx, y0 + 14, C_MUTE);
    }
    snprintf(b, sizeof(b), "%s DBM", nb);
    drawText(&SKI_MDB, b, rx, y0 + 34, hot);
    drawText(&SKI_SM, zone, rx, y0 + 56, hot);
    drawText(&SKI_SM, tt, rx, y0 + 70, hTrend > 0 ? C_INK : C_MUTE);
    char pk[24];
    snprintf(pk, sizeof(pk), "PEAK %d", ok ? (int)c.peak : 0);
    drawText(&SKI_SM, pk, rx, y0 + 84, C_MUTE);
    drawText(&SKI_SM, "RANGE EST", rx, y0 + 98, C_DIM);
    drawText(&SKI_SM, mac, rx, bot - 12, C_DIM);
  } else {
    int mg = 12, ty = fy + fh + 6;
    disc(mg + 3, ty + 5, 3.2f, fam);
    ellipsize(&SKI_MDB, ok ? kindLabel(c) : "LOST", W / 2 - 12, b, sizeof(b));
    drawText(&SKI_MDB, b, mg + 12, ty, C_INK);
    if (ok && c.nick[0]) {
      ellipsize(&SKI_SM, c.nick, W / 2 - 18, b, sizeof(b));
      textR(&SKI_SM, b, W - mg, ty + 1, C_MUTE);
    }
    snprintf(b, sizeof(b), "%s  %s", zone, tt);
    drawText(&SKI_SM, b, mg, ty + 20, hot);
    char pk[24];
    snprintf(pk, sizeof(pk), "PEAK %d  RANGE EST", ok ? (int)c.peak : 0);
    textR(&SKI_SM, pk, W - mg, ty + 20, C_MUTE);
    int by0 = bot - 11;
    if (ridShow) {
      int ty2 = bot - ridLn * 13 + 2;
      char rb[48];
      by0 = ty2 - 14;
      if (c.rid.flags & 1) {
        snprintf(rb, sizeof(rb), "ID  %s", c.rid.id);
        ellipsize(&SKI_SM, rb, W - 24, b, sizeof(b));
        drawText(&SKI_SM, b, mg, ty2, C_INK);
        ty2 += 13;
      }
      if (c.rid.flags & 2) {
        snprintf(rb, sizeof(rb), "%.5f %.5f %dM", c.rid.lat / 1e7, c.rid.lon / 1e7, (int)c.rid.alt);
        drawText(&SKI_SM, rb, mg, ty2, C_MUTE);
        ty2 += 13;
      }
      if (c.rid.flags & 4) {
        snprintf(rb, sizeof(rb), "PILOT %.5f %.5f", c.rid.opLat / 1e7, c.rid.opLon / 1e7);
        drawText(&SKI_SM, rb, mg, ty2, C_AMBER);
      }
    }
    drawText(&SKI_SM, mac, mg, by0, C_DIM);
  }
  drawHeader("Track", false);
  if (HAS_SOUND) {
    const char* n[2] = {"BACK", muted ? "SOUND" : "QUIET"};
    drawNav(2, n, -1);
  } else {
    const char* n[1] = {"BACK"};
    drawNav(1, n, -1);
  }
}
static void drawHunt() {
  if (hudOn) {
    drawHuntHud();
    return;
  }
  if (termOn) {
    drawHuntTerm();
    return;
  }
  const Contact& c = hc;
  bool ok = hcOk;
  uint16_t fam = ok ? famColor(c.fam) : C_TEAL;
  bool wide = W > H, cp = compact();
  int top = contentTop(), bot = contentBot();
  bool ridShow = ok && (c.rid.flags & 7);
  float r = cp ? 40.0f : (ridShow ? 50.0f : 58.0f), t = cp ? 7.0f : 9.0f;
  float cx = wide ? 12 + r + 6 : W / 2.0f;
  float cy = wide ? top + (bot - top) / 2.0f : top + 54 + r;
  float norm = clamp01((huntShown + 100.0f) / 70.0f);
  uint16_t hot = huntShown > -55 ? C_ALERT : (huntShown > -72 ? C_AMBER : fam);
  arcBand(cx, cy, r, t, G_A0, G_A0 + G_SWEEP, C_CARD2);
  if (norm > 0.02f)
    arcBand(cx, cy, r, t, G_A0, G_A0 + G_SWEEP * norm, hot);
  float rip = clamp01((int32_t)(gNow - huntSampleMs) / 650.0f);
  if (rip < 1.0f)
    ring(cx, cy, r + 3 + rip * 12, 1.4f, hot, (uint8_t)((1 - rip) * 120));
  char nb[8];
  snprintf(nb, sizeof(nb), "%d", (int)lroundf(huntShown));
  if (cp)
    textC(&SKI_LG, nb, (int)cx, (int)cy - SKI_LG.capH / 2 - 3, C_INK);
  else
    textC(&SKI_BIG, nb, (int)cx, (int)cy - SKI_BIG.capH / 2 - 6, C_INK);
  const char* zone = huntSearching ? "SEARCHING" : (huntShown > -55 ? "CLOSE" : (huntShown > -80 ? "NEAR" : "FAR"));
  char zl[16];
  snprintf(zl, sizeof(zl), (cp || huntSearching) ? "%s" : "dBm  |  %s", zone);
  textC(&SKI_SM, zl, (int)cx, (int)cy + (cp ? 13 : 28), huntSearching ? C_AMBER : hot);
  char mac[20] = "--";
  if (ok)
    macStr(c.mac, mac);
  char pk[24];
  snprintf(pk, sizeof(pk), "peak %d", ok ? (int)c.peak : 0);
  char b[40];
  if (wide) {
    int ix = (int)(cx + r + 24);
    disc(ix + 4, top + 8, 4, fam);
    drawText(&SKI_MDB, ok ? kindLabel(c) : "lost", ix + 14, top + 2, C_INK);
    if (ok && c.nick[0]) {
      ellipsize(&SKI_SM, c.nick, W - ix - 26, b, sizeof(b));
      drawText(&SKI_SM, b, ix + 14, top + 20, C_MUTE);
    }
    drawText(&SKI_SM, mac, ix, top + 36, C_DIM);
    drawText(&SKI_SM, pk, ix, top + 50, C_MUTE);
    // drone Remote ID card between the text and the history graph
    if (ridShow) {
      int rw = W - ix - 12, ry = top + 66,
          ln = ((c.rid.flags & 1) ? 1 : 0) + ((c.rid.flags & 2) ? 2 : 0) + ((c.rid.flags & 4) ? 1 : 0);
      rrFill(ix, ry, rw, 8 + ln * 13, 8, C_CARD);
      int ty2 = ry + 5;
      char rb[44];
      if (c.rid.flags & 1) {
        snprintf(rb, sizeof(rb), "ID  %s", c.rid.id);
        ellipsize(&SKI_SM, rb, rw - 14, b, sizeof(b));
        drawText(&SKI_SM, b, ix + 8, ty2, C_INK);
        ty2 += 13;
      }
      if (c.rid.flags & 2) {
        snprintf(rb, sizeof(rb), "%.5f, %.5f", c.rid.lat / 1e7, c.rid.lon / 1e7);
        drawText(&SKI_SM, rb, ix + 8, ty2, C_MUTE);
        ty2 += 13;
        snprintf(rb, sizeof(rb), "%d m   %.1f m/s", (int)c.rid.alt, c.rid.spd10 / 10.0f);
        drawText(&SKI_SM, rb, ix + 8, ty2, C_MUTE);
        ty2 += 13;
      }
      if (c.rid.flags & 4) {
        snprintf(rb, sizeof(rb), "pilot %.5f, %.5f", c.rid.opLat / 1e7, c.rid.opLon / 1e7);
        drawText(&SKI_SM, rb, ix + 8, ty2, C_AMBER);
      }
    }
    int sx = ix, sw = W - ix - 12, sh = imin(34, bot - (top + 68)), sy = bot - sh;
    if (sh >= 20) {
      rrFill(sx, sy, sw, sh, 8, C_CARD);
      if (histN > 1) {
        float px = 0, py = 0;
        for (int i = 0; i < histN; i++) {
          float x = sx + 6 + (sw - 12) * (float)i / (float)(sizeof(hist) - 1);
          float y = sy + sh - 4 - (sh - 8) * clamp01((hist[i] + 100.0f) / 70.0f);
          if (i)
            lineAA(px, py, x, y, 1.6f, hot);
          px = x;
          py = y;
        }
        disc(px, py, 2.4f, hot);
      }
    }
  } else {
    int iy = top;
    disc(18, iy + 6, 4, fam);
    drawText(&SKI_MDB, ok ? kindLabel(c) : "lost", 30, iy, C_INK);
    textR(&SKI_SM, pk, W - 14, iy + 3, C_MUTE);
    if (ok && c.nick[0]) {
      ellipsize(&SKI_SM, c.nick, W - 40, b, sizeof(b));
      drawText(&SKI_SM, b, 30, iy + 18, C_MUTE);
    }
    drawText(&SKI_SM, mac, 30, iy + 33, C_DIM);
    int sx = 12, sw = W - 24, sh = 30, sy = bot - sh;
    // drone Remote ID: id, position, pilot position (replaces the history graph)
    if (ridShow) {
      int ln = ((c.rid.flags & 1) ? 1 : 0) + ((c.rid.flags & 2) ? 1 : 0) + ((c.rid.flags & 4) ? 1 : 0);
      int ch2 = 8 + ln * 13;
      sy = bot - ch2;
      rrFill(sx, sy, sw, ch2, 8, C_CARD);
      int ty2 = sy + 5;
      char rb[44];
      if (c.rid.flags & 1) {
        snprintf(rb, sizeof(rb), "ID  %s", c.rid.id);
        drawText(&SKI_SM, rb, sx + 8, ty2, C_INK);
        ty2 += 13;
      }
      if (c.rid.flags & 2) {
        snprintf(rb, sizeof(rb), "%.5f, %.5f  %dm  %.1fm/s", c.rid.lat / 1e7, c.rid.lon / 1e7, (int)c.rid.alt,
                 c.rid.spd10 / 10.0f);
        drawText(&SKI_SM, rb, sx + 8, ty2, C_MUTE);
        ty2 += 13;
      }
      if (c.rid.flags & 4) {
        snprintf(rb, sizeof(rb), "pilot  %.5f, %.5f", c.rid.opLat / 1e7, c.rid.opLon / 1e7);
        drawText(&SKI_SM, rb, sx + 8, ty2, C_AMBER);
      }
    } else
      rrFill(sx, sy, sw, sh, 8, C_CARD);
    if (!ridShow && histN > 1) {
      float px = 0, py = 0;
      for (int i = 0; i < histN; i++) {
        float x = sx + 8 + (sw - 16) * (float)i / (float)(sizeof(hist) - 1);
        float y = sy + sh - 5 - (sh - 10) * clamp01((hist[i] + 100.0f) / 70.0f);
        if (i)
          lineAA(px, py, x, y, 1.6f, hot);
        px = x;
        py = y;
      }
      disc(px, py, 2.6f, hot);
    }
  }
  drawHeader("Track", false);
  if (HAS_SOUND) {
    const char* n[2] = {"BACK", muted ? "SOUND" : "QUIET"};
    drawNav(2, n, -1);
  } else {
    const char* n[1] = {"BACK"};
    drawNav(1, n, -1);
  }
}
// ---- Shared bits for text pages --------------------------------------------
static void fmtCount(uint32_t n, char* out, size_t sz) {
  if (n >= 1000000)
    snprintf(out, sz, "%.1fM", n / 1e6f);
  else if (n >= 10000)
    snprintf(out, sz, "%luK", (unsigned long)(n / 1000));
  else
    snprintf(out, sz, "%lu", (unsigned long)n);
}
static void fmtSize(uint64_t n, char* out, size_t sz) {
  if (n >= 1073741824ULL)
    snprintf(out, sz, "%.1f GB", (double)n / 1073741824.0);
  else if (n >= 1048576ULL)
    snprintf(out, sz, "%.1f MB", (double)n / 1048576.0);
  else if (n >= 1024ULL)
    snprintf(out, sz, "%.1f KB", (double)n / 1024.0);
  else
    snprintf(out, sz, "%lu B", (unsigned long)n);
}
static int drawWrapped(const SkiFont* f, const char* s, int x, int y, int maxW, uint16_t col, int lineH, bool draw) {
  char line[96];
  line[0] = 0;
  int ll = 0, lines = 0;
  const char* p = s;
  while (*p) {
    const char* e = p;
    while (*e && *e != ' ')
      e++;
    int wl = (int)(e - p);
    char cand[96];
    snprintf(cand, sizeof(cand), "%s%s%.*s", line, ll ? " " : "", wl, p);
    if (ll && textW(f, cand) > maxW) {
      if (draw)
        drawText(f, line, x, y + lines * lineH, col);
      lines++;
      ll = 0;
      line[0] = 0;
      continue;
    }
    strncpy(line, cand, sizeof(line) - 1);
    line[sizeof(line) - 1] = 0;
    ll = strlen(line);
    p = e;
    while (*p == ' ')
      p++;
  }
  if (ll) {
    if (draw)
      drawText(f, line, x, y + lines * lineH, col);
    lines++;
  }
  return lines * lineH;
}
static void card(int x, int y, int w, int h) {
  rrFill(x, y, w, h, 12, C_CARD);
}
static void drawChev(int x, int cy) {
  lineAA(x - 6, cy - 5, x, cy, 1.8f, C_MUTE);
  lineAA(x, cy, x - 6, cy + 5, 1.8f, C_MUTE);
}
// ---- LOG page --------------------------------------------------------------
// ---- Groups panel: each group, live status, and what it searches for -------
// returns the block height; registers every chip as a button
static int sceneChips(int x, int y, int w, bool draw) {
  int cx = x, cy = y;
  const int CHH = 28, GAP = 5;
  for (int n = 0; n < 6; n++) {
    if (n == 3 && !HAS_ZB)
      continue;  // no ZB/TH scene on a chip without 802.15.4
    int cw = textW(&SKI_SM, SCENE_NAME[n]) + 22;
    if (cx + cw > x + w) {
      cx = x;
      cy += CHH + GAP;
    }
    addBtn(B_SCENE, n, cx, cy, cw, CHH);
    if (draw && rowsVisible(cy, CHH)) {
      bool on = sceneActive(n), pr = isPressed(B_SCENE, n);
      rrBorder(cx, cy, cw, CHH, 14, pr ? C_CARD2 : (on ? mix(C_ON, C_BG, 40) : C_CARD), on ? C_ON : C_LINE);
      textMid(&SKI_SM, SCENE_NAME[n], cx + cw / 2, cy + CHH / 2, on ? C_ON : (n == 4 ? C_MUTE : C_INK));
    }
    cx += cw + GAP;
  }
  return (cy - y) + CHH;
}
// strongest current targets of a group (insertion into a top-N)
static int topTargets(int cat, Contact* out, int maxn, int* totalOut) {
  int n = 0, tot = 0;
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < nContacts; i++) {
    if (!famInCat(contacts[i].fam, cat))
      continue;
    tot++;
    int pos = n;
    while (pos > 0 && contacts[i].ema > out[pos - 1].ema)
      pos--;
    if (pos < maxn) {
      if (n < maxn)
        n++;
      for (int k = n - 1; k > pos; k--)
        out[k] = out[k - 1];
      out[pos] = contacts[i];
    }
  }
  portEXIT_CRITICAL(&mux);
  *totalOut = tot;
  return n;
}
struct GroupRow {
  int cat;
  const char* text;
};
// clang-format off
static const GroupRow GROUPS[] = {
  {CAT_ALPR, "Flock, Raven, ShotSpotter, Penguin"},
  {CAT_AXON, "Police body and in-car cameras. Axon: bodycams, docks, TASER, AB2-AB4, ABX, AF3. Other makers (shown as BODYCAM): Motorola Solutions and WatchGuard (V300, 4RE, VB and SI500 series), Digital Ally (FirstVu), COBAN, Utility, VIEVU, Pro-Vision, ICOP, L3 Mobile-Vision and Zepcam. Most are found by address block or set-up network name only, so their confidence is lower than Axon's"},
  {CAT_CAM, "Ring, Wyze, Arlo, Blink, Hikvision, Dahua +more"},
  {CAT_GLASS, "Ray-Ban Meta, Spectacles, Vuzix"},
  {CAT_DRONE, "DJI, Autel, Skydio, Parrot, Remote ID beacons"},
  {CAT_BT, "Bluetooth devices by name or maker. Type tags: IBEACON, EDDYSTONE, ALTBEACON (beacons), WEARABLE (watches, bands, rings), LIGHT (bulbs, LED strips), FASTPAIR (earbuds), MESH, SKIMMER (a module with a card-skimmer name such as HC-05: possible gas-pump skimmer, Bluetooth LE only), FLIPPER (a Flipper Zero with its Bluetooth on: its case-colour service or a Flipper name, or just the chip address = low confidence)"},
  {CAT_TAG, "Tile, AirTag, SmartTag, Google Find Hub trackers, Chipolo, Pebblebee"},
  {CAT_WIFI, "Nearby 2.4 GHz access points; smart bulbs in set-up mode (WiZ, LIFX, Yeelight) are tagged LIGHT"},
  {CAT_ZT, "802.15.4 channels 11-26: Zigbee, Thread or plain 802.15.4, told apart by beacon IDs, MAC security and NWK / 6LoWPAN headers"},
};
// clang-format on
static void drawPageGroups() {
  drawTitle("Groups", false);
  int x = 12, w = W - 24, top = contentTop(), bot = contentBot(), viewH = bot - top;
  const int NG = sizeof(GROUPS) / sizeof(GROUPS[0]) - (HAS_ZB ? 0 : 1);  // ZB/TH is the last entry
  int chipsH = sceneChips(x, 0, w, false) + 18;
  int total = chipsH + 6;
  for (int i = 0; i < NG; i++) {
    int cat = GROUPS[i].cat;
    Contact tg[3];
    int tot = 0;
    int nt = (catOn[cat] && !tileBlocked(cat)) ? topTargets(cat, tg, 3, &tot) : 0;
    total += 30 + drawWrapped(&SKI_SM, GROUPS[i].text, 0, 0, w - 24, C_MUTE, 14, false) + 9 + 5 +
             (nt ? 6 + nt * 15 + (tot > nt ? 13 : 0) : 0);
  }
  int noteH = drawWrapped(&SKI_SM, "Tap a group to see what it has found. Arm groups on the ARM page.", 0, 0, w - 8,
                          C_DIM, 14, false);
  total += noteH + 4;
  sc.max = total > viewH ? (float)(total - viewH) : 0;
  clipSet(0, top, W, viewH);
  int y = top - (int)sc.y;
  drawText(&SKI_SM, "SCENES  -  one tap arms a set", x + 4, y + 1, C_DIM);
  y += 18;
  y += sceneChips(x, y, w, true) + 6;
  for (int i = 0; i < NG; i++) {
    int cat = GROUPS[i].cat;
    int th = drawWrapped(&SKI_SM, GROUPS[i].text, 0, 0, w - 24, C_MUTE, 14, false);
    bool blk = tileBlocked(cat);
    Contact tg[3];
    int tot = 0;
    int nt = (catOn[cat] && !blk) ? topTargets(cat, tg, 3, &tot) : 0;
    int ch = 30 + th + 9 + (nt ? 6 + nt * 15 + (tot > nt ? 13 : 0) : 0);
    uint16_t cc = (cat == CAT_ZT) ? RGB(170, 255, 60) : CAT_COL[cat];
    // registered on every strip pass: the whole card opens the group's finds
    addBtn(B_GROUP, cat, x, y, w, ch);
    if (rowsVisible(y, ch)) {
      bool pr = isPressed(B_GROUP, cat);
      rrFill(x, y, w, ch, 12, pr ? C_CARD2 : C_CARD);
      rrFill(x, y + 12, 3, 14, 1, blk ? RGB(120, 50, 52) : cc);
      drawText(&SKI_MDB, CAT_LABEL[cat], x + 14, y + 10, blk ? RGB(150, 78, 80) : cc);
      char st[20];
      uint16_t sc2;
      if (blk) {
        snprintf(st, sizeof(st), "blocked");
        sc2 = RGB(150, 62, 64);
      } else if (catOn[cat]) {
        int n = countCatHits(cat);
        if (n > 0)
          snprintf(st, sizeof(st), "%d found", n);
        else
          snprintf(st, sizeof(st), "armed");
        sc2 = n > 0 ? cc : C_TEAL;
      } else {
        snprintf(st, sizeof(st), "off");
        sc2 = C_DIM;
      }
      textR(&SKI_SM, st, x + w - 12, y + 12, sc2);
      drawWrapped(&SKI_SM, GROUPS[i].text, x + 14, y + 30, w - 24, C_MUTE, 14, true);
      // the strongest targets right here: name, signal, tracker flag
      if (nt) {
        int ty = y + 30 + th + 5;
        rrFill(x + 14, ty - 1, w - 28, 1, 0, C_LINE);
        for (int k = 0; k < nt; k++) {
          const Contact& c = tg[k];
          int32_t age = (int32_t)(gNow - c.lastMs);
          if (age < 0)
            age = 0;
          uint8_t fade = age < 6000 ? 255 : (age < 20000 ? 190 : 130);
          disc(x + 18, ty + 9 + k * 15, 2.6f, cc, fade);
          char nb[36];
          ellipsize(&SKI_SM, c.nick[0] ? c.nick : (c.label[0] ? c.label : famName(c.fam)), w - 104, nb, sizeof(nb));
          drawText(&SKI_SM, nb, x + 26, ty + 3 + k * 15, mix(C_INK, C_CARD, fade));
          char rb[10];
          snprintf(rb, sizeof(rb), "%d", (int)c.rssi);
          textR(&SKI_SM, rb, x + w - 12, ty + 3 + k * 15, mix(C_MUTE, C_CARD, fade));
          if (c.follow)
            textR(&SKI_SM, W < 230 ? "FOLLOW" : "FOLLOWING", x + w - 40, ty + 3 + k * 15, C_ALERT);
        }
        if (tot > nt) {
          char mb[20];
          snprintf(mb, sizeof(mb), "+%d more", tot - nt);
          drawText(&SKI_SM, mb, x + 26, ty + 3 + nt * 15, C_DIM);
        }
      }
    }
    y += ch + 5;
  }
  drawWrapped(&SKI_SM, "Tap a group to see what it has found. Arm groups on the ARM page.", x + 4, y + 2, w - 8, C_DIM,
              14, true);
  clipReset();
}
static void drawPageSaved() {
  drawTitle("Saved", false);
  int x = 12, w = W - 24, top = contentTop(), bot = contentBot(), viewH = bot - top;
  int hw = (w - 6) / 2;
  const int h1 = 48, h2 = 40, h3 = 30, hb = 34, G = 4;
  int total = h1 + G + 2 * (h2 + G) + h3 + G + hb + G + hb;
  sc.max = total > viewH ? (float)(total - viewH) : 0;
  clipSet(0, top, W, viewH);
  int y = top - (int)sc.y;
  char b[40], n[12];
  card(x, y, w, h1);
  drawText(&SKI_SM, "SESSION", x + 12, y + 7, C_MUTE);
  if (rtcOk) {
    char tb[12];
    snprintf(tb, sizeof(tb), "%02u:%02u", clk.h, clk.mi);
    textR(&SKI_SM, tb, x + w - 12, y + 7, rtcApprox ? C_AMBER : C_TEAL);
  }
  ellipsize(&SKI_MD, fsOk ? logPath + 1 : "no flash fs", w - 24, b, sizeof(b));
  drawText(&SKI_MD, b, x + 12, y + 18, fsOk ? C_INK : C_ALERT);
  // what has been saved: a line you can trust at a glance
  if (fsOk) {
    char st[48];
    uint16_t sc2 = C_DIM;
    const bool nr = W < 230;  // narrow screens get the short forms
    if (logMode == 2)
      snprintf(st, sizeof(st), "%s",
               logFull ? (nr ? "live log full" : "live log full (SETUP > Save log)")
                       : (nr ? "live log on" : "live log: every find is written"));
    else if (ledgerEverSaved) {
      if (nr)
        snprintf(st, sizeof(st), "saved %s (%u)%s", ledgerSavedAt, (unsigned)ledgerSavedN, ledgerDirty ? " +" : "");
      else
        snprintf(st, sizeof(st), "saved %s - %u devices%s", ledgerSavedAt, (unsigned)ledgerSavedN,
                 ledgerDirty ? " (+ new)" : "");
      sc2 = ledgerDirty ? C_AMBER : C_TEAL;
    } else
      snprintf(st, sizeof(st), "%s",
               ledgerN ? (nr ? "not saved yet" : "not saved yet - tap SAVE LOG") : "nothing found yet");
    ellipsize(&SKI_SM, st, w - 24, b, sizeof(b));
    drawText(&SKI_SM, b, x + 12, y + 33, sc2);
  }
  y += h1 + G;
  // the main action sits right under the session card: writes the findings record to one file
  {
    bool pr3 = isPressed(B_SAVE, 0);
    rrBorder(x, y, w, hb, 12, pr3 ? C_CARD2 : C_CARD, mix(C_OK, C_BG, 150));
    textMid(&SKI_MDB, "SAVE LOG", x + w / 2, y + hb / 2, C_OK);
    addBtn(B_SAVE, 0, x, y, w, hb);
  }
  y += hb + G;
  card(x, y, hw, h2);
  card(x + hw + 6, y, hw, h2);
  drawText(&SKI_SM, "WI-FI", x + 11, y + 7, C_MUTE);
  fmtCount(framesSeen, n, sizeof(n));
  drawText(&SKI_LG, n, x + 11, y + 19, C_INK);
  drawText(&SKI_SM, "BLE", x + hw + 17, y + 7, C_MUTE);
  fmtCount(bleSeen, n, sizeof(n));
  drawText(&SKI_LG, n, x + hw + 17, y + 19, C_INK);
  y += h2 + G;
  card(x, y, hw, h2);
  card(x + hw + 6, y, hw, h2);
  if (HAS_ZB) {
    drawText(&SKI_SM, "802.15.4", x + 11, y + 7, C_MUTE);
    fmtCount(zbSeen, n, sizeof(n));
    drawText(&SKI_LG, n, x + 11, y + 19, RGB(170, 255, 60));
  } else {
    drawText(&SKI_SM, "FINDS", x + 11, y + 7, C_MUTE);
    fmtCount(sessionFinds, n, sizeof(n));
    drawText(&SKI_LG, n, x + 11, y + 19, RGB(170, 255, 60));
  }
  drawText(&SKI_SM, "BATTERY", x + hw + 17, y + 7, C_MUTE);
  int bp = batPct();
  if (bp >= 0) {
    snprintf(n, sizeof(n), pmuCharging ? "%d%%+" : "%d%%", bp);
    drawText(&SKI_LG, n, x + hw + 17, y + 19, bp < 20 ? C_ALERT : (pmuCharging ? C_TEAL : C_INK));
  } else
    drawText(&SKI_MD, "USB", x + hw + 17, y + 21, C_TEAL);
  y += h2 + G;
  card(x, y, w, h3);
  char g[40];
  snprintf(g, sizeof(g), "%u found", (unsigned)ledgerN);
  drawText(&SKI_MD, g, x + 12, y + (h3 - SKI_MD.capH) / 2, C_INK);
  snprintf(g, sizeof(g), gpsOn ? "GPS  %u sats" : "GPS  --", (unsigned)gps.satellites.value());
  textR(&SKI_SM, g, x + w - 12, y + (h3 - SKI_SM.capH) / 2, gpsOn ? C_ON : C_MUTE);
  y += h3 + G;
  {
    int bw = (w - 6) / 2;
    bool pr = isPressed(B_SHARE, 0), pr2 = isPressed(B_REPORT, 0);
    rrBorder(x, y, bw, hb, 12, pr ? C_CARD2 : C_CARD, mix(C_AMBER, C_BG, 150));
    textMid(&SKI_MDB, "SHARE", x + bw / 2, y + hb / 2, C_AMBER);
    addBtn(B_SHARE, 0, x, y, bw, hb);
    rrBorder(x + bw + 6, y, bw, hb, 12, pr2 ? C_CARD2 : C_CARD, mix(C_TEAL, C_BG, 150));
    textMid(&SKI_MDB, "REPORT", x + bw + 6 + bw / 2, y + hb / 2, C_TEAL);
    addBtn(B_REPORT, 0, x + bw + 6, y, bw, hb);
  }
  clipReset();
}
// ---- SETUP page ------------------------------------------------------------
static void drawSwitch(int x, int y, bool on) {  // 40x24
  if (termOn) {
    rrBorder(x, y, 40, 24, 12, on ? C_ON : C_CARD2, on ? C_ON : C_LINE);
    textMid(&SKI_SM, on ? "ON" : "OFF", x + 20, y + 12, on ? RGB(0, 8, 6) : C_MUTE);
    return;
  }
  rrFill(x, y, 40, 24, 12, on ? C_ON : C_CARD2);
  disc(x + (on ? 28 : 12), y + 12, 9.0f, on ? C_BG : C_MUTE);
}
static void drawPageSetup() {
  drawTitle("Setup", false);
  int x = 12, w = W - 24, top = contentTop(), bot = contentBot(), viewH = bot - top;
  const int H1 = 56, HR = 44, G = 6;
  int rows =
      17 + (HAS_IMU_BOARD ? 2 : 0) + (HAS_TOUCH_CAL ? 1 : 0) + (HAS_BOOTSOUND ? 1 : 0) + 1 + (orient == 4 ? 2 : 0);
  int total = 2 * (H1 + G) + rows * (HR + G);
  sc.max = total > viewH ? (float)(total - viewH) : 0;
  clipSet(0, top, W, viewH);
  int y = top - (int)sc.y;
  char b[16];
  card(x, y, w, H1);
  drawText(&SKI_MD, "Brightness", x + 14, y + 10, C_INK);
  snprintf(b, sizeof(b), "%d%%", (bright - 20) * 100 / 235);
  textR(&SKI_SM, b, x + w - 14, y + 12, C_MUTE);
  sliderX = x + 16;
  sliderW = w - 32;
  float v = (bright - 20) / 235.0f;
  int ty = y + 42;
  rrFill(sliderX, ty - 2, sliderW, 5, 2, C_CARD2);
  rrFill(sliderX, ty - 2, imax(5, (int)(sliderW * v)), 5, 2, C_AMBER);
  disc(sliderX + sliderW * v, ty + 0.5f, 8.0f, C_INK);
  addBtn(B_SLIDER, 0, x, y + 20, w, H1 - 20);
  y += H1 + G;
  card(x, y, w, H1);
  drawText(&SKI_MD, "Volume", x + 14, y + 10, C_INK);
  if (soundVol > 0)
    snprintf(b, sizeof(b), "%d%%", soundVol);
  else
    snprintf(b, sizeof(b), "off");
  textR(&SKI_SM, b, x + w - 14, y + 12, C_MUTE);
  {
    float vv = soundVol / 100.0f;
    int vy = y + 42;
    rrFill(sliderX, vy - 2, sliderW, 5, 2, C_CARD2);
    rrFill(sliderX, vy - 2, imax(5, (int)(sliderW * vv)), 5, 2, C_TEAL);
    disc(sliderX + sliderW * vv, vy + 0.5f, 8.0f, C_INK);
  }
  addBtn(B_VOL, 0, x, y + 20, w, H1 - 20);
  y += H1 + G;
  // orientation: Portrait / Landscape / Portrait flipped / Landscape flipped / Auto
  {
    bool po = isPressed(B_SETROW, 7);
    static const char* ON[5] = {"Portrait", "Landscape", "Portrait flip", "Landscape flip", "Auto"};
    rrFill(x, y, w, HR, 12, po ? C_CARD2 : C_CARD);
    drawText(&SKI_MD, "Orientation", x + 14, y + (HR - SKI_MD.capH) / 2, C_INK);
    textR(&SKI_SM, ON[orient % 5], x + w - 14, y + (HR - SKI_SM.capH) / 2, orient == 4 ? C_ON : C_MUTE);
    addBtn(B_SETROW, 7, x, y, w, HR);
    y += HR + G;
    // only matters in Auto: fixes the sensor's mounting direction
    if (orient == 4) {
      bool pa2 = isPressed(B_SETROW, 8);
      static const char* AA[4] = {"0", "90", "180", "270"};
      {
        bool pc = isPressed(B_SETROW, 9);
        rrFill(x, y, w, HR, 12, pc ? C_CARD2 : C_CARD);
        drawText(&SKI_MD, "Calibrate Auto", x + 14, y + (HR - SKI_MD.capH) / 2, C_INK);
        textR(&SKI_SM, calOk ? "done" : "do this first", x + w - 14, y + (HR - SKI_SM.capH) / 2,
              calOk ? C_TEAL : C_AMBER);
        addBtn(B_SETROW, 9, x, y, w, HR);
        y += HR + G;
      }
      rrFill(x, y, w, HR, 12, pa2 ? C_CARD2 : C_CARD);
      drawText(&SKI_MD, "Auto angle", x + 14, y + (HR - SKI_MD.capH) / 2, C_INK);
      textR(&SKI_SM, AA[autoOff & 3], x + w - 14, y + (HR - SKI_SM.capH) / 2, C_MUTE);
      addBtn(B_SETROW, 8, x, y, w, HR);
      y += HR + G;
    }
  }
  {  // find alert: Off / New group / Every find
    bool pa = isPressed(B_ALERT, 0);
    static const char* AN[3] = {"Off", "New group", "Every find"};
    rrFill(x, y, w, HR, 12, pa ? C_CARD2 : C_CARD);
    drawText(&SKI_MD, "Find alert", x + 14, y + (HR - SKI_MD.capH) / 2, C_INK);
    textR(&SKI_SM, AN[alertMode % 3], x + w - 14, y + (HR - SKI_SM.capH) / 2, alertMode ? C_ON : C_MUTE);
    addBtn(B_ALERT, 0, x, y, w, HR);
    y += HR + G;
  }
  auto valueRow = [&](int id, const char* label, const char* val, uint16_t vc) {
    bool p = isPressed(B_SETROW, id);
    rrFill(x, y, w, HR, 12, p ? C_CARD2 : C_CARD);
    drawText(&SKI_MD, label, x + 14, y + (HR - SKI_MD.capH) / 2, C_INK);
    textR(&SKI_SM, val, x + w - 14, y + (HR - SKI_SM.capH) / 2, vc);
    addBtn(B_SETROW, id, x, y, w, HR);
    y += HR + G;
  };
  auto switchRow = [&](int id, const char* label, bool on) {
    card(x, y, w, HR);
    drawText(&SKI_MD, label, x + 14, y + (HR - SKI_MD.capH) / 2, C_INK);
    drawSwitch(x + w - 54, y + (HR - 24) / 2, on);
    addBtn(B_SETROW, id, x, y, w, HR);
    y += HR + G;
  };
#if HAS_TOUCH_CAL
  valueRow(10, "Calibrate touch", tcal.ok ? "done" : "do this first", tcal.ok ? C_TEAL : C_AMBER);
#endif
  {
    static const char* const CM[4] = {"Normal", "Swap R/B", "Invert", "Swap + invert"};
    valueRow(22, "Screen colors", CM[colorMode & 3], colorMode ? C_AMBER : C_MUTE);
  }
  {
    char fv[16];
    if (followMin)
      snprintf(fv, sizeof(fv), "%d min", followMin);
    else
      snprintf(fv, sizeof(fv), "Off");
    valueRow(4, "Tracker alert", fv, followMin ? C_ON : C_MUTE);
  }
  valueRow(15, "Skimmer alert", skimWatch ? "On" : "Off", skimWatch ? C_ON : C_MUTE);
  card(x, y, w, HR);
  drawText(&SKI_MD, "Auto-dim", x + 14, y + (HR - SKI_MD.capH) / 2, C_INK);
  drawSwitch(x + w - 54, y + (HR - 24) / 2, autoDim);
  addBtn(B_DIM, 0, x, y, w, HR);
  y += HR + G;
  // motion-sensor features exist only on boards with a motion sensor
  if (HAS_IMU_BOARD) {
    switchRow(0, "Raise to wake", raiseWake);
    switchRow(1, "Shake to pause", shakePause);
  }
  {
    static const char* PN[3] = {"Off", "Auto", "On"};
    valueRow(2, "Power saver", PN[powerMode % 3], powerMode ? C_ON : C_MUTE);
  }
  switchRow(3, "Discreet mode", discreet);
  {
    char iv[24];
    snprintf(iv, sizeof(iv), "%d hidden", (int)nIgnore);
    valueRow(5, "Ignore list", iv, nIgnore ? C_AMBER : C_MUTE);
  }
  valueRow(12, "List view", LV_NAME[listView & 3], listView ? C_ON : C_MUTE);
  {
    char lv[24];
    bootLogoLabel(lv, sizeof(lv));
    valueRow(14, "Boot logo", lv, bootLogoMode == 1 ? C_ON : C_MUTE);
  }
#if HAS_BOOTSOUND
  {
    char bv[20];
    bootSoundLabel(bv, sizeof(bv));
    valueRow(13, "Boot sound", bv, bootSoundMode == 2 ? C_TEAL : (bootSoundMode == 0 ? C_MUTE : C_INK));
  }
#endif
  {
    static const char* LM[3] = {"Manual", "Auto 5 min", "Live"};
    valueRow(11, "Save log", LM[logMode % 3], logMode == 2 ? C_AMBER : (logMode == 1 ? C_ON : C_MUTE));
  }
  {
    static const char* const SV[3] = {"Auto: SD first", "SD card only", "Flash only"};
    valueRow(16, "Save to", SV[storMode % 3], storMode == 1 ? C_TEAL : (storMode == 2 ? C_AMBER : C_MUTE));
  }
  {
    bool p = isPressed(B_SETROW, 6);
    rrFill(x, y, w, HR, 12, p ? C_CARD2 : C_CARD);
    drawText(&SKI_MD, "Self-test", x + 14, y + (HR - SKI_MD.capH) / 2, C_INK);
    drawChev(x + w - 14, y + HR / 2);
    addBtn(B_SETROW, 6, x, y, w, HR);
    y += HR + G;
  }
  const char* lk[2] = {"Help", "Storage"};
  for (int i = 0; i < 2; i++) {
    bool p2 = isPressed(i == 0 ? B_HELP : B_STORE, 0);
    rrFill(x, y, w, HR, 12, p2 ? C_CARD2 : C_CARD);
    drawText(&SKI_MD, lk[i], x + 14, y + (HR - SKI_MD.capH) / 2, C_INK);
    drawChev(x + w - 14, y + HR / 2);
    addBtn(i == 0 ? B_HELP : B_STORE, 0, x, y, w, HR);
    y += HR + G;
  }
  {  // the terminal theme
    card(x, y, w, HR);
    drawText(&SKI_MD, "ALIEN", x + 14, y + 7, C_INK);
    bool gOn = termOn && !hudOn;
    drawText(&SKI_SM, gOn ? "tracker interface: on" : "green tracker look", x + 14, y + 26, C_MUTE);
    drawSwitch(x + w - 54, y + (HR - 24) / 2, gOn);
    addBtn(B_SETROW, 20, x, y, w, HR);
    y += HR + G;
  }
  {  // the HUD theme (blue)
    card(x, y, w, HR);
    drawText(&SKI_MD, "ALIENS", x + 14, y + 7, C_INK);
    drawText(&SKI_SM, hudOn ? "blue HUD tracker: on" : "blue film-style tracker", x + 14, y + 26, C_MUTE);
    drawSwitch(x + w - 54, y + (HR - 24) / 2, hudOn);
    addBtn(B_SETROW, 21, x, y, w, HR);
    y += HR + G;
  }
  clipReset();
}
// ---- Help ------------------------------------------------------------------
struct HelpRow {
  int cat;
  const char* text;
};
// clang-format off
static const HelpRow HELP[] = {
  {CAT_ALPR, "Flock, Raven, ShotSpotter, Penguin"},
  {CAT_AXON, "Police body and in-car cameras. Axon: bodycams, docks, TASER, AB2-AB4, ABX, AF3. Other makers (shown as BODYCAM): Motorola Solutions and WatchGuard (V300, 4RE, VB and SI500 series), Digital Ally (FirstVu), COBAN, Utility, VIEVU, Pro-Vision, ICOP, L3 Mobile-Vision and Zepcam. Most are found by address block or set-up network name only, so their confidence is lower than Axon's"},
  {CAT_CAM, "Ring, Wyze, Arlo, Blink, Hikvision, Dahua +more"},
  {CAT_GLASS, "Ray-Ban Meta, Spectacles, Vuzix"},
  {CAT_DRONE, "DJI, Autel, Skydio, Parrot, Remote ID beacons"},
  {CAT_BT, "Bluetooth devices by name or maker. Type tags: IBEACON, EDDYSTONE, ALTBEACON (beacons), WEARABLE (watches, bands, rings), LIGHT (bulbs, LED strips), FASTPAIR (earbuds), MESH, SKIMMER (a module with a card-skimmer name such as HC-05: possible gas-pump skimmer, Bluetooth LE only), FLIPPER (a Flipper Zero with its Bluetooth on: its case-colour service or a Flipper name, or just the chip address = low confidence)"},
  {CAT_TAG, "Tile, AirTag, SmartTag, Google Find Hub trackers, Chipolo, Pebblebee"},
  {CAT_WIFI, "Nearby 2.4 GHz access points; smart bulbs in set-up mode (WiZ, LIFX, Yeelight) are tagged LIGHT"},
  {CAT_ZT, "802.15.4 channels 11-26, labelled Zigbee, Thread or plain 802.15.4 from beacon IDs, MAC security and NWK / 6LoWPAN headers. Zigbee/Thread and the Wi-Fi groups share one radio, so only one side can be armed at a time. The blocked side turns dim red."},
};
// clang-format on
static const char* HELP_TIPS =
    "Swipe left / right to change page. Long-press an ARM tile to open its finds. Tap LIVE to pause. Swipe right on any sub-screen to go back.";
static const char* HELP_NOTE =
    "High-confidence fingerprints only. Not a complete catalog. A miss is not proof the device is absent.";
static void drawInfo() {
  int x = 12, w = W - 24, top = contentTop(), bot = contentBot(), viewH = bot - top;
  int total = 0;
  const int NH_ = sizeof(HELP) / sizeof(HELP[0]) - (HAS_ZB ? 0 : 1);  // ZB/TH is the last entry
  int tipH = drawWrapped(&SKI_SM, HELP_TIPS, 0, 0, w - 24, C_MUTE, 14, false) + 24;
  total += tipH + 6;
  for (int i = 0; i < NH_; i++)
    total += 30 + drawWrapped(&SKI_SM, HELP[i].text, 0, 0, w - 24, C_MUTE, 14, false) + 10 + 6;
  total += drawWrapped(&SKI_SM, HELP_NOTE, 0, 0, w - 8, C_MUTE, 14, false) + 4;
  sc.max = total > viewH ? (float)(total - viewH) : 0;
  clipSet(0, top, W, viewH);
  int y = top - (int)sc.y;
  if (rowsVisible(y, tipH)) {
    rrBorder(x, y, w, tipH, 12, C_CARD, mix(C_AMBER, C_BG, 120));
    drawText(&SKI_MDB, "Tips", x + 12, y + 9, C_AMBER);
    drawWrapped(&SKI_SM, HELP_TIPS, x + 12, y + 28, w - 24, C_MUTE, 14, true);
  }
  y += tipH + 6;
  for (int i = 0; i < NH_; i++) {
    int th = drawWrapped(&SKI_SM, HELP[i].text, 0, 0, w - 24, C_MUTE, 14, false);
    int ch = 30 + th + 10;
    uint16_t cc = (HELP[i].cat == CAT_ZT) ? RGB(170, 255, 60) : CAT_COL[HELP[i].cat];
    if (rowsVisible(y, ch)) {
      card(x, y, w, ch);
      rrFill(x, y + 12, 3, 14, 1, cc);
      drawText(&SKI_MDB, CAT_LABEL[HELP[i].cat], x + 14, y + 10, cc);
      drawWrapped(&SKI_SM, HELP[i].text, x + 14, y + 30, w - 24, C_MUTE, 14, true);
    }
    y += ch + 6;
  }
  drawWrapped(&SKI_SM, HELP_NOTE, x + 4, y + 2, w - 8, C_DIM, 14, true);
  clipReset();
  drawHeader("Help", false);
  const char* n[1] = {"BACK"};
  drawNav(1, n, -1);
}
// ---- Storage (internal flash) ----------------------------------------------
static void loadFiles() {
  fileN = 0;
  if (!fsOk)
    return;
  File root = fsStore->open("/");
  if (!root)
    return;
  File e = root.openNextFile();
  while (e && fileN < 24) {
    const char* raw = e.name();
    const char* nm = strrchr(raw, '/');
    nm = nm ? nm + 1 : raw;
    strncpy(fileName[fileN], nm, 27);
    fileName[fileN][27] = 0;
    fileSize[fileN] = (uint32_t)e.size();
    fileN++;
    e.close();
    e = root.openNextFile();
  }
  root.close();
}
static void drawIgn() {
  int x = 12, w = W - 24, top = contentTop(), bot = contentBot(), viewH = bot - top;
  // narrow screens: shorter text, no RESTORE pills (the whole row is the button)
  const int ROWH = 44;
  const bool nar = W < 230;
  const char* noteTxt =
      nar ? "Tap a device to restore it." : "Hidden from lists, counts, alerts and logs. Tap one to bring it back.";
  const int NOTE = drawWrapped(&SKI_SM, noteTxt, 0, 0, w - 24, C_MUTE, 15, false) + 16;
  int total = NOTE + 6 + (nIgnore ? nIgnore * (ROWH + 6) : 56);
  sc.max = total > viewH ? (float)(total - viewH) : 0;
  clipSet(0, top, W, viewH);
  int y = top - (int)sc.y;
  if (rowsVisible(y, NOTE)) {
    card(x, y, w, NOTE);
    drawWrapped(&SKI_SM, noteTxt, x + 12, y + 8, w - 24, C_MUTE, 15, true);
  }
  y += NOTE + 6;
  if (!nIgnore) {
    card(x, y, w, 50);
    textC(&SKI_MD, "Nothing is hidden", W / 2, y + 9, C_INK);
    textC(&SKI_SM, nar ? "long-press a find" : "long-press a find to hide it", W / 2, y + 29, C_MUTE);
  }
  for (int k = nIgnore - 1; k >= 0; k--) {  // most recently hidden first
    addBtn(B_IGN, k, x, y, w, ROWH);
    if (rowsVisible(y, ROWH)) {
      bool pr = isPressed(B_IGN, k);
      rrFill(x, y, w, ROWH, 10, pr ? C_CARD2 : C_CARD);
      uint16_t fc = ign[k].fam ? famColor((Family)ign[k].fam) : C_MUTE;
      rrFill(x + 8, y + 8, 3, ROWH - 16, 1, fc);
      char mc[20], b1[40], b2[48], sb[48];
      macStr(ign[k].mac, mc);
      int pw = nar ? 0 : 64, textRoom = w - 36 - (pw ? pw + 8 : 0);
      ellipsize(&SKI_MDB, ign[k].name[0] ? ign[k].name : "(no name)", textRoom, b1, sizeof(b1));
      drawText(&SKI_MDB, b1, x + 18, y + 6, C_INK);
      if (ign[k].fam)
        snprintf(sb, sizeof(sb), "%s  %s", famName((Family)ign[k].fam), mc);
      else
        snprintf(sb, sizeof(sb), "%s", mc);
      ellipsize(&SKI_SM, sb, textRoom, b2, sizeof(b2));
      drawText(&SKI_SM, b2, x + 18, y + 24, C_MUTE);
      if (pw) {
        int px = x + w - pw - 8;
        rrFill(px, y + (ROWH - 24) / 2, pw, 24, 12, mix(C_AMBER, C_CARD, 44));
        textMid(&SKI_SM, "RESTORE", px + pw / 2, y + ROWH / 2, C_AMBER);
      } else {
        lineAA(x + w - 18, y + ROWH / 2 - 5, x + w - 12, y + ROWH / 2, 1.8f, C_AMBER);
        lineAA(x + w - 12, y + ROWH / 2, x + w - 18, y + ROWH / 2 + 5, 1.8f, C_AMBER);
      }
    }
    y += ROWH + 6;
  }
  clipReset();
  drawHeader("Ignored", false);
  const char* n[2] = {"BACK", nar ? "ALL" : "RESTORE ALL"};
  drawNav(nIgnore ? 2 : 1, n, -1);
}
static int logFileCount() {
  int n = 0;
  for (int i = 0; i < fileN; i++)
    if (strncmp(fileName[i], "skiscan_", 8) == 0)
      n++;
  return n;
}
static void drawFiles() {
  int x = 12, w = W - 24, top = contentTop(), bot = contentBot(), viewH = bot - top;
  int total = 60 + 6 + (fileN ? fileN * 42 : 46);
  sc.max = total > viewH ? (float)(total - viewH) : 0;
  clipSet(0, top, W, viewH);
  int y = top - (int)sc.y;
  card(x, y, w, 60);
  static uint64_t used = 0, tot = 1;
  static uint32_t stAt = 0;
  if (fsOk && (stAt == 0 || gNow - stAt > 5000)) {
    stAt = gNow ? gNow : 1;
    if (!fsSpace(&used, &tot)) {
      used = 0;
      tot = 1;
    }
  }
  char a[20], b[20], t[48];
  fmtSize(used, a, sizeof(a));
  fmtSize(tot, b, sizeof(b));
  snprintf(t, sizeof(t), "%s of %s", a, b);
  drawText(&SKI_SM, fsOk ? "FLASH USED" : "FLASH NOT MOUNTED", x + 12, y + 10, fsOk ? C_MUTE : C_ALERT);
  drawText(&SKI_MD, fsOk ? t : "-", x + 12, y + 25, C_INK);
  rrFill(x + 12, y + 47, w - 24, 5, 2, C_CARD2);
  float fr = clamp01((float)((double)used / (double)tot));
  rrFill(x + 12, y + 47, imax(5, (int)((w - 24) * fr)), 5, 2, fr > 0.85f ? C_ALERT : C_TEAL);
  y += 66;
  if (!fileN) {
    card(x, y, w, 40);
    textC(&SKI_SM, "no files yet", W / 2, y + 15, C_MUTE);
  }
  for (int i = 0; i < fileN; i++) {
    if (rowsVisible(y, 36)) {
      card(x, y, w, 36);
      char nb[32], sz[16];
      ellipsize(&SKI_MD, fileName[i], w - 78, nb, sizeof(nb));
      drawText(&SKI_MD, nb, x + 12, y + 12, C_INK);
      fmtSize(fileSize[i], sz, sizeof(sz));
      textR(&SKI_SM, sz, x + w - 10, y + 14, C_MUTE);
    }
    y += 42;
  }
  clipReset();
  drawHeader("Storage", false);
  const char* n[2] = {"BACK", W < 230 ? "DELETE" : "DELETE LOGS"};
  drawNav(logFileCount() ? 2 : 1, n, -1);
}
// ---- Share portal ----------------------------------------------------------
static void drawPortal() {
  int x = 12, w = W - 24, y = contentTop(), bot = contentBot();
  int rows = 3, rh = imin(52, (bot - y - (rows - 1) * 6) / rows);
  card(x, y, w, rh);
  drawText(&SKI_SM, "JOIN WI-FI", x + 12, y + 8, C_MUTE);
  // the longer name may need the smaller font on a narrow screen
  {
    bool fit = textW(&SKI_LG, portalSsid) <= w - 24;
    drawText(fit ? &SKI_LG : &SKI_MD, portalSsid, x + 12, y + rh - 12 - (fit ? SKI_LG.capH : SKI_MD.capH), C_INK);
  }
  y += rh + 6;
  card(x, y, w, rh);
  drawText(&SKI_SM, "PASSWORD", x + 12, y + 8, C_MUTE);
  drawText(&SKI_LG, portalPass, x + 12, y + rh - 12 - SKI_LG.capH, C_AMBER);
  y += rh + 6;
  card(x, y, w, rh);
  drawText(&SKI_SM, "THEN OPEN  -  live, sigs, update", x + 12, y + 8, C_MUTE);
  drawText(&SKI_LG, "192.168.4.1", x + 12, y + rh - 12 - SKI_LG.capH, C_TEAL);
  drawHeader("Share", false);
  const char* n[1] = {"STOP"};
  drawNav(1, n, 0);
}
// ---- Boot: radar ping + wordmark -------------------------------------------
#define BOOT_MS 1900
// ---- Terminal theme: boot screen (a terminal start-up with the scope) ------
static void drawBootTerm(int32_t t, bool customLogo) {
  float p = clamp01(t / (float)BOOT_MS);
  int mg = 14;
  float cxs = W / 2.0f, R = fminf(86.0f, (W - 2 * mg) / 2.0f);
  if (!customLogo) {
    float cys = H / 2.0f + 6;
    float ph = fmodf(t * 0.00046f, 1.0f);
    // a contact creeps in from the edge toward you
    float tb = clamp01((t - 700) / 900.0f);
    if (hudOn) {
      float RR = W > H ? 54.0f : 78.0f, oyb = W > H ? (float)(H - 106) : (float)(H / 2 - 4);
      hudHud(cxs, oyb, RR, RGB(0, 0, 0));
      int32_t tw0 = (int32_t)(t % ALS_CYCLE_MS);
      hudWave(cxs, oyb, RR, tw0);
      if (tb > 0.0f)
        hudEcho(cxs, oyb, RR, tw0, 0.9f - 0.5f * easeOut(tb), 0.55f, 200, 5);
      fillRect((int)(cxs - RR), (int)oyb, (int)(2 * RR), (int)(0.2f * RR), RGB(0, 148, 255));
    } else {
      termScope(cxs, cys, R, ph);
      if (tb > 0.0f)
        termBlip(cxs, cys, R, ph, 1.0f - 0.55f * easeOut(tb), 1.5f * 3.14159265f + 0.55f, C_AMBER, 0.0f);
    }
    uint8_t a = (uint8_t)(255 * easeOut(t / 620.0f));
    int ty = 12 + (compact() ? 0 : SAFE_TOP);
    int tw = textW(&SKI_LG, "HUNTR") + textW(&SKI_LG, "/") + textW(&SKI_LG, "SEEKR"), x = (W - tw) / 2;
    drawText(&SKI_LG, "HUNTR", x, ty, C_AMBER, a);
    x += textW(&SKI_LG, "HUNTR");
    drawText(&SKI_LG, "/", x, ty, C_DIM, a);
    x += textW(&SKI_LG, "/");
    drawText(&SKI_LG, "SEEKR", x, ty, C_INK, a);
    textC(&SKI_SM, "TRACKING INTERFACE", W / 2, ty + SKI_LG.capH + 7, C_MUTE, a);
  }
  static const char* const L[4] = {"SCANNER ARRAY", "TOUCH PANEL", "STORAGE", "INTERFACE"};
  bool okv[4] = {true, touchOk, fsOk, true};
  const char* res[4] = {"STANDBY", touchOk ? "OK" : "NOT FOUND", fsOk ? "OK" : "NONE", hudOn ? "ALIENS" : "ALIEN"};
  int ly = H - 100 + (customLogo ? 20 : 0);
  for (int i = 0; i < 4; i++) {
    int32_t at = 250 + i * 330;
    if (t < at)
      break;
    char ln[44], dots[24];
    int nd = 16 - (int)strlen(L[i]) - (int)strlen(res[i]) + 4;
    if (nd < 2)
      nd = 2;
    if (nd > 20)
      nd = 20;
    for (int k = 0; k < nd; k++)
      dots[k] = '.';
    dots[nd] = 0;
    snprintf(ln, sizeof(ln), "> %s %s ", L[i], dots);
    drawText(&SKI_SM, ln, mg, ly + i * 13, C_MUTE);
    if (t > at + 160)
      drawText(&SKI_SM, res[i], mg + textW(&SKI_SM, ln), ly + i * 13, okv[i] ? C_TEAL : C_ALERT);
  }
  int bw = W - 2 * mg, by = H - 38, cells = 24, on = (int)(cells * easeOut(p) + 0.5f);  // progress: a row of blocks
  for (int k = 0; k < cells; k++)
    fillRect(mg + k * bw / cells, by, bw / cells - 2, 8, k < on ? C_TEAL : C_CARD2);
  {
    char bl[44];
    snprintf(bl, sizeof(bl), "%s", APP_VERSION);
    textC(&SKI_SM, bl, W / 2, H - 22, C_DIM);
  }
}
static void drawBoot() {
  int32_t t = (int32_t)(gNow - bootAt);
  if (t < 0)
    t = 0;
  float e = easeOut(t / 620.0f);
  uint8_t a2 = (uint8_t)(255 * clamp01(((int)t - 200) / 420.0f));  // (the version line below fades in with it)
  // your own picture (if chosen and readable) replaces the radar and the title
  bool customLogo = bootLogoDraw();
  if (termOn) {
    drawBootTerm(t, customLogo);
    return;
  }
  float cxs = W / 2.0f, cys = H / 2.0f - 20;
  if (!customLogo) {
    for (int k = 0; k < 3; k++) {  // expanding radar rings
      float ph = fmodf(t * 0.00075f + k / 3.0f, 1.0f);
      ring(cxs, cys, 6 + ph * 70, 1.4f, C_AMBER, (uint8_t)((1 - ph) * (1 - ph) * 150 * clamp01(t / 400.0f)));
    }
    disc(cxs, cys, 3.6f, C_AMBER, (uint8_t)(255 * clamp01(t / 300.0f)));
    int ty = (int)cys + 84;
    int tw = textW(&SKI_LG, "HUNTR") + textW(&SKI_LG, "/") + textW(&SKI_LG, "SEEKR");
    int x = (W - tw) / 2, y = ty + (int)(12 * (1 - e));
    uint8_t a = (uint8_t)(255 * e);
    drawText(&SKI_LG, "HUNTR", x, y, C_AMBER, a);
    x += textW(&SKI_LG, "HUNTR");
    drawText(&SKI_LG, "/", x, y, C_DIM, a);
    x += textW(&SKI_LG, "/");
    drawText(&SKI_LG, "SEEKR", x, y, C_INK, a);
    textC(&SKI_SM, "COUNTER-RECON", W / 2, ty - 18, C_MUTE, a2);
  }
  float p = clamp01(t / (float)BOOT_MS);
  int bw = 96, bx = (W - bw) / 2, by = H - 44;
  rrFill(bx, by, bw, 4, 2, C_CARD2);
  rrFill(bx, by, imax(4, (int)(bw * easeOut(p))), 4, 2, C_AMBER);
  {
    char bl[40];
    if (touchOk)
      snprintf(bl, sizeof(bl), "%s   %s", fsOk ? "ready" : "no storage", APP_VERSION);
    else
      snprintf(bl, sizeof(bl), "touch NOT detected");
    textC(&SKI_SM, bl, W / 2, H - 28, (touchOk && fsOk) ? C_DIM : C_ALERT, a2);
  }
}
// =============================================================================
//  Features that need screen / UI state
// =============================================================================
static void showToast(const char* m) {
  snprintf(toastMsg, sizeof(toastMsg), "%s", m);
  toastUntil = millis() + 1900;
  needRender = true;
}
// ---- Find / tracker banner (slides in under the header, tap = track it) ----
static bool banOn = false, banTap = false, banUrgent = false;
static uint32_t banAt = 0;
static uint8_t banMac[6];
static char banT[28], banS[44];
static uint16_t banCol = 0;
static void bannerShow(const char* t, const char* s, uint16_t col, bool tappable, const uint8_t* mac, bool urgent) {
  strncpy(banT, t, 27);
  banT[27] = 0;
  strncpy(banS, s, 43);
  banS[43] = 0;
  banCol = col;
  banTap = tappable && mac;
  banUrgent = urgent;
  if (mac)
    memcpy(banMac, mac, 6);
  else
    memset(banMac, 0, 6);
  banOn = true;
  banAt = millis();
  needRender = true;
}
static bool bannerActive() {
  return banOn && (int32_t)(millis() - banAt) < (banUrgent ? 9000 : 5000);
}
static void bannerHide() {
  banOn = false;
  needRender = true;
}
static bool bannerAnimating() {
  if (!banOn)
    return false;
  int32_t age = (int32_t)(millis() - banAt), left = (banUrgent ? 9000 : 5000) - age;
  return age < 260 || left < 360;
}
static uint16_t groupColor(int cat) {
  return cat == CAT_ZT ? RGB(170, 255, 60) : CAT_COL[cat];
}
// ---- Alerts (find + tracker + battery) -------------------------------------
// a group's chime is due: sound, banner, maybe wake the screen
static void onFindAlert() {
  FindNote n;
  portENTER_CRITICAL(&mux);
  n = pendFind;
  pendFind.valid = false;
  portEXIT_CRITICAL(&mux);
  if (!n.valid)
    return;
  int g = groupOf(n.fam);
  if (!discreet)
    sndPlay(GTUNE[g], 3);
  char t[28], s[44];
  snprintf(t, sizeof(t), "%s: %s", CAT_LABEL[g], n.name);
  snprintf(s, sizeof(s), "%d dBm  -  tap to track", (int)n.rssi);
  if (!discreet)
    bannerShow(t, s, groupColor(g), true, n.mac, false);
  if (!discreet && n.conf >= CONF_HIGH && (screenOff || dimmed))
    wakeDisplay();  // strong find: light the screen
}
// a suspected skimmer: red banner, siren, wake the screen
static void skimUiService() {
  if (!skimQ)
    return;
  uint8_t mac[6];
  portENTER_CRITICAL(&mux);
  memcpy(mac, skimQMac[0], 6);
  skimQ--;
  memmove(skimQMac[0], skimQMac[1], (size_t)skimQ * 6);
  portEXIT_CRITICAL(&mux);
  int i = indexOfMac(mac);
  if (i < 0)
    return;
  char sub[44];
  snprintf(sub, sizeof(sub), "%s  %d dBm  -  tap to track", contacts[i].nick[0] ? contacts[i].nick : "BT module",
           (int)contacts[i].rssi);
  bannerShow("POSSIBLE SKIMMER", sub, C_ALERT, true, mac, true);
  if (!discreet) {
    sndPlay(SND_SKIM, sizeof(SND_SKIM) / sizeof(SND_SKIM[0]));
    if (screenOff || dimmed)
      wakeDisplay();
  }
}
static void followUiService() {
  if (!followQ)
    return;
  uint8_t mac[6];
  portENTER_CRITICAL(&mux);
  memcpy(mac, followMacs[0], 6);
  followQ--;
  memmove(followMacs[0], followMacs[1], (size_t)followQ * 6);
  portEXIT_CRITICAL(&mux);
  int i = indexOfMac(mac);
  char s[44] = "in range for a long time";
  const char* nm = "Tracker";
  if (i >= 0) {
    nm = contacts[i].nick[0] ? contacts[i].nick : contacts[i].label;
    snprintf(s, sizeof(s), "in range %u min  -  tap to track",
             (unsigned)((contacts[i].lastMs - contacts[i].firstMs) / 60000UL));
  }
  char t[28];
  snprintf(t, sizeof(t), "FOLLOWING: %s", nm);
  bannerShow(t, s, C_ALERT, true, mac, true);
  if (!discreet) {
    sndPlay(SND_FOLLOW, sizeof(SND_FOLLOW) / sizeof(SND_FOLLOW[0]));
    if (screenOff || dimmed)
      wakeDisplay();
  }
}
// ---- Scenes (one tap arms a useful set of groups) --------------------------
static void applyScene(int n) {
  if (n < 0 || n > 5)
    return;
  if (n == 5) {
    skimWatch = true;
    savePrefs();
  }
  bool wasOff = (armedMask == 0);
  for (int k = 0; k < CAT_N; k++)
    catOn[k] = (SCENE_MASK[n] >> k) & 1;
  syncArmed();
  armChanged(wasOff);
  sndPlay(SCENE_MASK[n] ? SND_ON : SND_OFF, 2);
  char m[40];
  snprintf(m, sizeof(m), n == 4 ? "All groups off" : "Scene: %s", SCENE_NAME[n]);
  showToast(m);
}
// ---- Discreet mode + power saver (display timers, brightness, BLE duty) ----
static bool lowWarn20 = false, lowWarn10 = false;
static bool saverActive() {
  return powerMode == 2 || (powerMode == 1 && batPct() >= 0 && batPct() <= 15 && !pmuCharging);
}
static int brightCap() {
  return discreet ? 30 : (saverOn ? 70 : 255);
}
static uint32_t dimAfterMs() {
  return discreet ? 8000UL : (saverOn ? 10000UL : DIM_AFTER_MS);
}
static uint32_t sleepAfterMs() {
  return discreet ? 20000UL : (saverOn ? 30000UL : SLEEP_AFTER_MS);
}
static bool dimEnabled() {
  return autoDim || saverOn || discreet;
}
static void applySaver() {
  if (bleScan && radioOn) {
    bleStopScan();
    bleScan->setWindow(saverOn ? 30 : 100);
    bleStartScan();
  }
  needRender = true;
}
static void powerService(uint32_t now) {
  static uint32_t last = 0;
  if ((int32_t)(now - last) < 2000)
    return;
  last = now;
  bool s = saverActive();
  if (s != saverOn) {
    saverOn = s;
    applySaver();
    showToast(s ? "Power saver on" : "Power saver off");
  }
  int bp = batPct();
  if (bp < 0 || pmuCharging || bp > 25) {
    lowWarn20 = lowWarn10 = false;
    return;
  }
  if (bp <= 10 && !lowWarn10) {
    lowWarn10 = lowWarn20 = true;
    bannerShow("Battery very low", "charge soon - scanning may stop", C_ALERT, false, nullptr, true);
    if (!discreet)
      sndPlay(SND_LOWBAT, 2);
  } else if (bp <= 20 && !lowWarn20) {
    lowWarn20 = true;
    bannerShow("Battery low", "20% left", C_AMBER, false, nullptr, false);
    if (!discreet)
      sndPlay(SND_LOWBAT, 2);
  }
}
static void setDiscreet(bool on) {
  discreet = on;
  if (on) {
    blTarget = imin(bright, brightCap());
  } else {
    blTarget = dimmed ? imax(12, bright / 6) : bright;
  }
  savePrefs();
  showToast(on ? "Discreet mode on" : "Discreet mode off");
}
// ---- Motion sensor (QMI8658 @0x6B): raise-to-wake and shake-to-pause -------
static bool imuOk = false;
static float imuAx = 0, imuAy = 0, imuAz = 0;
static bool imuInit() {
  const uint8_t cand[2] = {0x6B, 0x6A};
  uint8_t id = 0;
  bool found = false;
  for (int k = 0; k < 2 && !found; k++)
    if (i2cRead(cand[k], 0x00, &id, 1) && id == 0x05) {
      imuAddr = cand[k];
      found = true;
    }
  if (!found) {
    Serial.println("QMI8658: not found (motion features and Auto rotate disabled)");
    return false;
  }
  i2cWrite(imuAddr, 0x60, 0xB0);
  delay(30);                      // soft reset
  i2cWrite(imuAddr, 0x02, 0x40);  // register auto-increment
  i2cWrite(imuAddr, 0x03, 0x16);  // accelerometer +-4 g, 125 Hz
  i2cWrite(imuAddr, 0x08, 0x01);  // enable the accelerometer
  Serial.printf("QMI8658 ready @0x%02X\n", imuAddr);
  return true;
}
static bool imuRead() {
  uint8_t b[6];
  if (!i2cRead(imuAddr, 0x35, b, 6))
    return false;
  // +-4 g
  imuAx = (int16_t)(b[0] | (b[1] << 8)) / 8192.0f;
  imuAy = (int16_t)(b[2] | (b[3] << 8)) / 8192.0f;
  imuAz = (int16_t)(b[4] | (b[5] << 8)) / 8192.0f;
  return true;
}
static void togglePause() {
  if (!armedMask)
    return;
  scanning = !scanning;
  tapBlip();
  needRender = true;
  showToast(scanning ? "Resumed" : "Paused");
}
// ---- Sensor mounting: which sensor axes are 'up the screen', 'right' and 'out of the screen' ----
// gravity (accelerometer reading) expressed along the screen's own axes
static void screenAxes(float* aU, float* aR, float* aN) {
  float U[3] = {0, 0, 0}, N[3] = {0, 0, 0};
  U[calUi] = calUs;
  N[calNi] = calNs;
  // right = up x out (right-handed)
  float R[3] = {U[1] * N[2] - U[2] * N[1], U[2] * N[0] - U[0] * N[2], U[0] * N[1] - U[1] * N[0]};
  float a[3] = {imuAx, imuAy, imuAz};
  *aU = a[0] * U[0] + a[1] * U[1] + a[2] * U[2];
  *aR = a[0] * R[0] + a[1] * R[1] + a[2] * R[2];
  *aN = a[0] * N[0] + a[1] * N[1] + a[2] * N[2];
}
// ---- calibration wizard: pose 1 = upright (learns 'up'), pose 2 = flat, screen up (learns 'out of the screen'); 'right' follows
// 0 idle, 1 upright, 2 flat, 3 done, 4 failed
static uint8_t calStep = 0;
static float calSum[3], calPrev[3], calLive[3], calProg = 0;
static int calCnt = 0, calPoseUi = 0, calPoseUs = 1;
static uint32_t calHoldAt = 0, calAt = 0, calDoneAt = 0, calRenderAt = 0;
static bool calHolding = false;
static uint8_t calPrevRot = 0;
#define CAL_HOLD_MS 1500
static void calStart() {
  if (!imuOk) {
    showToast("Motion sensor not found");
    return;
  }
  calPrevRot = rot;
  calStep = 1;
  calHolding = false;
  calProg = 0;
  calCnt = 0;
  // the instructions are written for the normal portrait view
  if (rot != 0)
    applyRotation(0, true);
  navigate(SCR_CAL);
}
static void calLeave() {  // called when the wizard screen is left
  calStep = 0;
  uint8_t back = (orient < 4) ? orient : 0;
  if (rot != back)
    applyRotation(back, true);
}
static void calRestart() {
  calStep = 1;
  calHolding = false;
  calProg = 0;
  calCnt = 0;
  needRender = true;
}
static void calService(uint32_t now) {
  if (screen != SCR_CAL)
    return;
  if (calStep == 3) {
    if ((int32_t)(now - calDoneAt) > 2200)
      goBack();
    return;
  }
  if (calStep != 1 && calStep != 2)
    return;
  if ((int32_t)(now - calAt) < 20)
    return;
  calAt = now;
  if (!imuRead())
    return;
  float a[3] = {imuAx, imuAy, imuAz};
  memcpy(calLive, a, sizeof(a));
  float d = fmaxf(fabsf(a[0] - calPrev[0]), fmaxf(fabsf(a[1] - calPrev[1]), fabsf(a[2] - calPrev[2])));
  memcpy(calPrev, a, sizeof(a));
  int dom = 0;
  for (int k = 1; k < 3; k++)
    if (fabsf(a[k]) > fabsf(a[dom]))
      dom = k;
  bool steady = d < 0.05f && fabsf(a[dom]) > 0.75f;  // still, and gravity clearly along one axis
  if (steady) {
    if (!calHolding) {
      calHolding = true;
      calHoldAt = now;
      calCnt = 0;
      calSum[0] = calSum[1] = calSum[2] = 0;
    }
    for (int k = 0; k < 3; k++)
      calSum[k] += a[k];
    calCnt++;
    calProg = clamp01((float)(now - calHoldAt) / CAL_HOLD_MS);
    if ((int32_t)(now - calHoldAt) >= CAL_HOLD_MS) {
      float avg[3];
      for (int k = 0; k < 3; k++)
        avg[k] = calSum[k] / (float)calCnt;
      int ax = 0;
      for (int k = 1; k < 3; k++)
        if (fabsf(avg[k]) > fabsf(avg[ax]))
          ax = k;
      int sg = avg[ax] < 0 ? -1 : 1;
      calHolding = false;
      calProg = 0;
      if (calStep == 1) {
        calPoseUi = ax;
        calPoseUs = sg;
        calStep = 2;
        sndPlay(SND_ON, 2);
      }
      // the two poses looked the same: something was not held right
      else if (ax == calPoseUi) {
        calStep = 4;
        sndPlay(SND_OFF, 2);
      } else {
        calUi = calPoseUi;
        calUs = calPoseUs;
        calNi = ax;
        calNs = sg;
        calOk = true;
        savePrefs();
        calStep = 3;
        calDoneAt = now;
        sndPlay(SND_FIND, 3);
      }
      needRender = true;
    }
  } else {
    calHolding = false;
    calProg = 0;
  }
  if ((int32_t)(now - calRenderAt) > 90) {
    calRenderAt = now;
    needRender = true;
  }
}
static float azHist[10];
static uint8_t azHistN = 0, azHistI = 0;
static uint32_t azHistAt = 0, lastRaiseAt = 0, lastShakeAt = 0, shakeT[3];
static uint8_t shakeN = 0;
static uint8_t autoCand = 255;
static uint32_t autoSince = 0;
static void imuService(uint32_t now) {
  static uint32_t at = 0;
  if (!imuOk)
    return;
  bool needRaise = raiseWake && !discreet && (dimmed || screenOff), needShake = shakePause,
       needAuto = (orient == 4) && !screenOff;
  if (!needRaise && !needShake && !needAuto) {
    azHistN = 0;
    autoCand = 255;
    return;
  }
  if ((int32_t)(now - at) < ((needAuto && !needRaise && !needShake) ? 100 : 20))
    return;
  at = now;
  if (!imuRead())
    return;
  // Auto orientation: which screen edge is 'down'?
  if (needAuto) {
    float aU, aR, aN;
    screenAxes(&aU, &aR, &aN);
    float dx = fabsf(aR), dy = fabsf(aU);
    // not lying flat, and a clear winner between the two screen axes
    if (screen != SCR_CAL && fabsf(aN) < 0.8f && fabsf(dx - dy) > 0.25f) {
      // whichever screen edge points up becomes the top of the picture
      uint8_t r0 = dy > dx ? (aU > 0 ? 0 : 2) : (aR > 0 ? 1 : 3);
      uint8_t cand = (uint8_t)((r0 + autoOff) & 3);
      if (cand == rot)
        autoCand = 255;
      else if (cand != autoCand) {
        autoCand = cand;
        autoSince = now;
      } else if ((int32_t)(now - autoSince) > 700) {
        autoCand = 255;
        applyRotation(cand, true);
      }  // steady for 0.7 s: turn
    } else
      autoCand = 255;
  }
  float sU, sR, sN;
  screenAxes(&sU, &sR, &sN);
  if ((int32_t)(now - azHistAt) >= 80) {
    azHistAt = now;
    azHist[azHistI] = sN;
    azHistI = (azHistI + 1) % 10;
    if (azHistN < 10)
      azHistN++;
  }
  if (needRaise && azHistN >= 6 && (int32_t)(now - lastRaiseAt) > 4000) {
    // newest azHistN samples, in ring order
    float mn = 9;
    for (int k = 0; k < azHistN; k++) {
      float v = azHist[(azHistI + 10 - 1 - k) % 10];
      if (v < mn)
        mn = v;
    }
    // face turned up towards you
    if (sN > 0.65f && mn < 0.25f && fabsf(sR) < 0.95f) {
      lastRaiseAt = now;
      azHistN = 0;
      wakeDisplay();
    }
  }
  if (needShake) {
    float mag = sqrtf(imuAx * imuAx + imuAy * imuAy + imuAz * imuAz);
    if (mag > 2.4f && (int32_t)(now - lastShakeAt) > 60) {
      lastShakeAt = now;
      if (shakeN && (int32_t)(now - shakeT[shakeN - 1]) > 900)
        shakeN = 0;
      if (shakeN < 3)
        shakeT[shakeN++] = now;
      if (shakeN == 3 && (int32_t)(now - shakeT[0]) < 1200) {
        shakeN = 0;
        lastShakeAt = now + 1500;
        togglePause();
      }
    }
  }
}
// ---- One call from the main loop -------------------------------------------
static void plusService(uint32_t now) {
  rtcService(now);
  followService(now);
  followUiService();
  skimUiService();
  calService(now);
  imuService(now);
  powerService(now);
  if (banOn && !bannerActive())
    bannerHide();
}
// =============================================================================
//  Banner, session report, self-test drawing
// =============================================================================
static void drawBanner() {
  const int x = 12, w = W - 24, h = 40, y = hdrH() + 2;
  uint32_t age = millis() - banAt;
  float t = clamp01(age / 180.0f);
  int yy = y - (int)((1.0f - easeOut(t)) * (float)(h + y + 4));  // slides down from above the screen
  int32_t left = (int32_t)((banUrgent ? 9000 : 5000) - (int32_t)age);
  uint8_t a = left < 300 ? (uint8_t)(255 * (left < 0 ? 0 : left) / 300) : 255;
  gOx = 0;
  clipReset();
  rrFill(x, yy, w, h, 12, banCol, a);
  rrFill(x + 1, yy + 1, w - 2, h - 2, 11, RGB(18, 21, 29), a);
  rrFill(x + 1, yy + 12, 3, 16, 1, banCol, a);
  char b[40];
  ellipsize(&SKI_MDB, banT, w - 28, b, sizeof(b));
  drawText(&SKI_MDB, b, x + 14, yy + 6, banUrgent ? banCol : C_INK, a);
  ellipsize(&SKI_SM, banS, w - 28, b, sizeof(b));
  drawText(&SKI_SM, b, x + 14, yy + 23, C_MUTE, a);
  // final position; registered on every strip pass, used on the first
  if (banTap)
    addBtn(B_BANNER, 0, x, y, w, h);
}
// ---- Session report: totals, per-group bars, last-60-minutes chart ---------
static void fmtDuration(uint32_t ms, char* out, size_t n) {
  uint32_t s = ms / 1000, m = s / 60, h = m / 60;
  if (h)
    snprintf(out, n, "%uh %02um", (unsigned)h, (unsigned)(m % 60));
  else
    snprintf(out, n, "%um %02us", (unsigned)m, (unsigned)(s % 60));
}
static void drawReport() {
  int x = 12, w = W - 24, top = contentTop(), bot = contentBot(), viewH = bot - top;
  uint32_t now = millis();
  int active = 0, maxc = 1;
  uint16_t bmax = 1;
  for (int k = 0; k < CAT_N; k++)
    if (gstat[k].count) {
      active++;
      if (gstat[k].count > maxc)
        maxc = gstat[k].count;
    }
  for (int i = 0; i < 60; i++)
    if (minBucket[i] > bmax)
      bmax = minBucket[i];
  const int H1 = 52, RH = 30, CH = 92, G = 5;
  int total = H1 + G + 26 + (active ? active * RH : 30) + G + CH;
  sc.max = total > viewH ? (float)(total - viewH) : 0;
  clipSet(0, top, W, viewH);
  int y = top - (int)sc.y;
  char b[40];
  // --- totals
  card(x, y, w, H1);
  int cw = w / 3;
  drawText(&SKI_SM, "FINDS", x + 12, y + 7, C_MUTE);
  snprintf(b, sizeof(b), "%u", (unsigned)sessionFinds);
  drawText(&SKI_LG, b, x + 12, y + 22, C_INK);
  int gx0 = W < 230 ? x + cw - 2 : x + cw + 6;
  drawText(&SKI_SM, "GROUPS", gx0, y + 7, C_MUTE);
  snprintf(b, sizeof(b), "%d", active);
  drawText(&SKI_LG, b, gx0, y + 22, C_TEAL);
  // the report is built for 270 px; narrower screens use the short forms
  bool nar = W < 260;
  int tx0 = nar ? x + 2 * cw + 8 : x + 2 * cw;
  drawText(&SKI_SM, nar ? "TIME" : "RUNNING", tx0, y + 7, C_MUTE);
  fmtDuration(now - sessionStartMs, b, sizeof(b));
  drawText(nar ? &SKI_SM : &SKI_MD, b, tx0, y + 25, C_INK);
  y += H1 + G;
  // --- by group
  drawText(&SKI_SM, "BY GROUP", x + 4, y + 6, C_DIM);
  y += 26;
  if (!active) {
    drawText(&SKI_SM, "Nothing found yet this session.", x + 4, y + 4, C_MUTE);
    y += 30;
  } else {
    for (int k = 0; k < CAT_N; k++) {
      if (!gstat[k].count)
        continue;
      uint16_t cc = groupColor(k);
      if (rowsVisible(y, RH)) {
        card(x, y, w, RH - 3);
        drawText(&SKI_MDB, CAT_LABEL[k], x + 10, y + 3, cc);
        snprintf(b, sizeof(b), "%u", (unsigned)gstat[k].count);
        textR(&SKI_MDB, b, x + w - 10, y + 3, C_INK);
        if (nar)
          snprintf(b, sizeof(b), "%d dBm", (int)gstat[k].best);
        else
          snprintf(b, sizeof(b), "best %d dBm   first +%um", (int)gstat[k].best,
                   (unsigned)((gstat[k].firstMs - sessionStartMs) / 60000UL));
        textR(&SKI_SM, b, x + w - 40, y + 6, C_MUTE);
        int bwid = imax(3, (w - 20) * gstat[k].count / maxc);
        rrFill(x + 10, y + RH - 8, w - 20, 3, 1, C_CARD2);
        rrFill(x + 10, y + RH - 8, bwid, 3, 1, cc);
      }
      y += RH;
    }
  }
  y += G;
  // --- timeline of the last 60 minutes
  if (rowsVisible(y, CH)) {
    card(x, y, w, CH);
    drawText(&SKI_SM, nar ? "LAST HOUR" : "LAST 60 MINUTES", x + 12, y + 7, C_MUTE);
    if (!nar) {
      snprintf(b, sizeof(b), "peak %u / min", (unsigned)bmax);
      textR(&SKI_SM, b, x + w - 12, y + 7, C_DIM);
    }
    int cx0 = x + 12, cw2 = w - 24, cy1 = y + CH - 20, chh = 46;
    rrFill(cx0, cy1, cw2, 1, 0, C_LINE);
    for (int i = 0; i < 60; i++) {
      int bx = cx0 + i * cw2 / 60, bx2 = cx0 + (i + 1) * cw2 / 60;
      int bw = imax(2, bx2 - bx - 1);
      if (minBucket[i]) {
        int bh = imax(3, chh * minBucket[i] / bmax);
        rrFill(bx, cy1 - bh, bw, bh, 1, i == 59 ? C_TEAL : C_AMBER);
      }
    }
    drawText(&SKI_SM, "60 min ago", cx0, y + CH - 15, C_DIM);
    textR(&SKI_SM, "now", cx0 + cw2, y + CH - 15, C_DIM);
  }
  clipReset();
  drawHeader("Report", false);
  const char* n[1] = {"BACK"};
  drawNav(1, n, -1);
}
// ---- Self-test: every subsystem at a glance, live --------------------------
// st: 0 red = fault, 1 green = fine, 2 amber = look at it, 3 grey = not on this board
static void testRowS(int& y, int x, int w, uint8_t st, const char* label, const char* val) {
  const int RH = 34;
  if (rowsVisible(y, RH)) {
    card(x, y, w, RH - 3);
    disc(x + 14, y + (RH - 3) / 2.0f, 4.5f, st == 1 ? C_OK : (st == 2 ? C_AMBER : (st == 3 ? C_DIM : C_ALERT)));
    drawText(&SKI_MDB, label, x + 28, y + 3, st == 3 ? C_MUTE : C_INK);
    char b[48];
    ellipsize(&SKI_SM, val, w - 40, b, sizeof(b));
    drawText(&SKI_SM, b, x + 28, y + 17, st == 3 ? C_DIM : C_MUTE);
  }
  y += RH;
}
static void testRow(int& y, int x, int w, bool ok, const char* label, const char* val) {
  testRowS(y, x, w, ok ? 1 : 0, label, val);
}
// the SD card row of the Self-test: true when the card is where findings are saved
static bool sdStatusText(char* o, size_t n) {
  if (storMode == 2) {
    snprintf(o, n, "not used  (Save to: Flash only)");
    return false;
  }
  if (fsOk && fsStore == (fs::FS*)&SD) {
    char a[16], b[16];
    fmtSize(SD.totalBytes(), a, sizeof(a));
    fmtSize(SD.totalBytes() - SD.usedBytes(), b, sizeof(b));
    snprintf(o, n, "%s, %s free  -  saving here", a, b);
    return true;
  }
  snprintf(o, n, storMode == 1 ? "no card found: NOTHING is being saved" : "no card found: saving to flash");
  return false;
}
static void drawTest() {
  int x = 12, w = W - 24, top = contentTop(), bot = contentBot(), viewH = bot - top;
  const int ROWS = 11, RH = 34;
  sc.max = ROWS * RH > viewH ? (float)(ROWS * RH - viewH) : 0;
  clipSet(0, top, W, viewH);
  int y = top - (int)sc.y;
  char v[64];
  snprintf(v, sizeof(v), touchOk ? "answers  reads %lu  INT %lu" : "no answer from the chip", (unsigned long)tpOkN,
           (unsigned long)tpIsrN);
  testRow(y, x, w, touchOk, "Touch", v);
  snprintf(v, sizeof(v), "%dx%d  brightness %d%%", PANEL_W, PANEL_H, bright * 100 / 255);
  testRow(y, x, w, true, "Display", v);
  testRow(y, x, w, audioReady, "Sound", audioReady ? SOUND_OK_TXT : SOUND_BAD_TXT);
  // (a CYD has no motion sensor: that is normal, not a fault)
  if (!HAS_IMU_BOARD)
    testRowS(y, x, w, 3, "Motion", "not fitted on this board");
  else {
    if (imuOk)
      snprintf(v, sizeof(v), "x %.2f  y %.2f  z %.2f", imuAx, imuAy, imuAz);
    else
      snprintf(v, sizeof(v), "not found");
    testRow(y, x, w, imuOk, "Motion", v);
  }
  {
    bool clkGood = rtcOk && !rtcApprox;
    if (clkGood)
      snprintf(v, sizeof(v), "%s", clkStr);
    else
      snprintf(v, sizeof(v), rtcOk ? "approximate  -  set it from your phone" : "not set  -  set it from your phone");
    testRowS(y, x, w, clkGood ? 1 : 2, "Clock", v);
  }
  int bp = batPct();
  if (bp >= 0)
    snprintf(v, sizeof(v), "%d%%%s%s", bp, pmuCharging ? "  charging" : "", pmuVbus ? "  USB" : "");
  else
    snprintf(v, sizeof(v), pmuOk ? "no battery detected" : "power chip not found");
  testRow(y, x, w, pmuOk, "Battery", v);
  if (HAS_ZB)
    snprintf(v, sizeof(v), "Wi-Fi %lu  BLE %lu  15.4 %lu%s", (unsigned long)framesSeen, (unsigned long)bleSeen,
             (unsigned long)zbSeen, wifiFallback ? "  scan" : "");
  else
    snprintf(v, sizeof(v), "Wi-Fi %lu  BLE %lu%s", (unsigned long)framesSeen, (unsigned long)bleSeen,
             wifiFallback ? "  scan" : "");
  testRow(y, x, w, radioOn || !armedMask, "Radios", armedMask ? v : "nothing armed");
  {
    char sdv[64];
    bool sdUse = sdStatusText(sdv, sizeof(sdv));
    testRow(y, x, w, sdUse, "SD card", sdv);
  }
  // the figures of the storage in use (the SD card or the flash), not always the flash
  {
    uint64_t su = 0, stt = 0;
    if (fsSpace(&su, &stt)) {
      char a[16], b[16];
      fmtSize(su, a, sizeof(a));
      fmtSize(stt, b, sizeof(b));
      snprintf(v, sizeof(v), "%s: %s of %s  %d rules  %d hidden", fsStore == (fs::FS*)&SD ? "SD" : "flash", a, b,
               (int)ncsig, (int)nIgnore);
    } else if (fsOk)
      snprintf(v, sizeof(v), "%d rules  %d hidden", (int)ncsig, (int)nIgnore);
    else
      snprintf(v, sizeof(v), "no storage: nothing can be saved");
    testRow(y, x, w, fsOk, "Storage", v);
  }
  snprintf(v, sizeof(v), "%u KB free  up %lu min", (unsigned)(ESP.getFreeHeap() / 1024),
           (unsigned long)(millis() / 60000UL));
  testRow(y, x, w, ESP.getFreeHeap() > 40000, "Memory", v);
  {
    static uint32_t wMs = 0, wAt = 0;
    uint32_t nw = millis();  // worst frame of the last 10 s
    if (dbgFrameMs >= wMs || nw - wAt > 10000UL) {
      wMs = dbgFrameMs;
      wAt = nw;
    }
    uint32_t tot = stripSent + stripSkip;
    int sk = tot ? (int)(100UL * stripSkip / tot) : 0;
    snprintf(v, sizeof(v), "frame %lu ms  worst %lu ms  skipped %d%%", (unsigned long)dbgFrameMs, (unsigned long)wMs,
             sk);
    testRowS(y, x, w, wMs < 150 ? 1 : 2, "Speed", v);
  }
  // the real height of all the rows
  {
    int total = y + (int)sc.y - top;
    sc.max = total > viewH ? (float)(total - viewH) : 0;
  }
  clipReset();
  drawHeader("Self-test", false);
  const char* n[2] = {"BACK", "BEEP"};
  drawNav(2, n, -1);
}
// ---- Auto-rotate calibration wizard ----------------------------------------
static void drawCal() {
  int x = 12, w = W - 24, top = contentTop(), bot = contentBot();
  const char *step, *head, *body;
  uint16_t hc = C_INK;
  switch (calStep) {
    case 1:
      step = "STEP 1 OF 2";
      head = "Hold it UPRIGHT";
      body = "The way this screen reads now, top edge up, like a phone you are reading. Keep it still.";
      break;
    case 2:
      step = "STEP 2 OF 2";
      head = "Lay it FLAT";
      body = "On a table, screen facing the ceiling. Keep it still.";
      break;
    case 3:
      step = "DONE";
      head = "Calibrated";
      body = "Auto rotate now follows the way you turn the device.";
      hc = C_TEAL;
      break;
    default:
      step = "TRY AGAIN";
      head = "Positions looked alike";
      body = "Step 2 must be flat with the screen up, step 1 upright. Tap RESTART.";
      hc = C_ALERT;
      break;
  }
  card(x, top, w, bot - top);
  drawText(&SKI_SM, step, x + 14, top + 12, C_MUTE);
  drawText(&SKI_LG, head, x + 14, top + 30, hc);
  drawWrapped(&SKI_SM, body, x + 14, top + 60, w - 28, C_MUTE, 15, true);
  if (calStep == 1 || calStep == 2) {
    int by = bot - 58;
    rrFill(x + 14, by, w - 28, 8, 4, C_CARD2);
    int fw = (int)((w - 28) * calProg);
    if (fw > 2)
      rrFill(x + 14, by, fw, 8, 4, calProg >= 1.0f ? C_TEAL : C_AMBER);
    drawText(&SKI_SM, calHolding ? "steady - keep holding" : "waiting for it to be still", x + 14, by + 14,
             calHolding ? C_TEAL : C_DIM);
    char b[40];
    snprintf(b, sizeof(b), "x %.2f   y %.2f   z %.2f", calLive[0], calLive[1], calLive[2]);
    drawText(&SKI_SM, b, x + 14, by + 32, C_DIM);
  }
  drawHeader("Calibrate", false);
  const char* n[2] = {"BACK", "RESTART"};
  drawNav(2, n, -1);
}
// ---- Touch calibration (4 corner points: 3 define the map, the 4th checks it) ----
// 0..3 = target being touched, 4 = done, 5 = failed (restarts by itself)
static int tcalStep = 0;
static uint32_t tcalFailAt = 0;
// on the DONE screen the normal touch handling takes over (the CONTINUE button)
static bool tcalPolling() {
  return tcalStep < 4 || tcalStep == 5;
}
static float tcalRaw[4][2];  // averaged raw readings per target
static float tcalAcc[2];
static int tcalN = 0;
static uint32_t tcalDownAt = 0, tcalLastDown = 0;
static bool tcalWaitUp = false;
// targets sit near the four corners of the CURRENT picture: TL, TR, BL, BR
static void tcalTarget(int i, int* gx, int* gy) {
  int m = 30;
  *gx = (i == 1 || i == 3) ? W - m : m;
  *gy = (i >= 2) ? H - m : m;
}
// inverse of the touch mapping: where on the native panel is this picture position?
static void renderToNative(int gx, int gy, int* px, int* py) {
  switch (rot) {
    case 0:
      *px = gx;
      *py = gy;
      break;
    case 2:
      *px = PANEL_W - 1 - gx;
      *py = PANEL_H - 1 - gy;
      break;
    case 1:
      *py = gx;
      *px = PANEL_W - 1 - gy;
      break;
    default:
      *py = PANEL_H - 1 - gx;
      *px = gy;
      break;
  }
}
static void tcalStart() {
  tcalStep = 0;
  tcalN = 0;
  tcalWaitUp = true;
  tcalLastDown = millis();
  if (screen != SCR_TCAL)
    navigate(SCR_TCAL);
  needRender = true;
}
static void tcalLeave() {
  tcalN = 0;
}
// px = c0 + c1*a + c2*b through the three points (Cramer's rule), likewise py
static bool tcalSolve() {
  float A[3][3], px[3], py[3];
  for (int i = 0; i < 3; i++) {
    int gx, gy, nx, ny;
    tcalTarget(i, &gx, &gy);
    renderToNative(gx, gy, &nx, &ny);
    A[i][0] = 1;
    A[i][1] = tcalRaw[i][0];
    A[i][2] = tcalRaw[i][1];
    px[i] = (float)nx;
    py[i] = (float)ny;
  }
  auto det3 = [&](float m[3][3]) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
  };
  float D = det3(A);
  // the three presses were (nearly) on one line: no usable calibration
  if (fabsf(D) < 1000.0f)
    return false;
  // solved on a copy: a failed attempt must not wreck a calibration that works
  float cc[6];
  for (int k = 0; k < 2; k++) {
    float* t = k == 0 ? px : py;
    for (int c = 0; c < 3; c++) {
      float M[3][3];
      memcpy(M, A, sizeof(M));
      for (int r = 0; r < 3; r++)
        M[r][c] = t[r];
      cc[k * 3 + c] = det3(M) / D;
    }
  }
  // check: the map made from three presses must send the FOURTH press onto its own target (a stray or sloppy press cannot do that)
  {
    int gx, gy, nx, ny;
    tcalTarget(3, &gx, &gy);
    renderToNative(gx, gy, &nx, &ny);
    float a = tcalRaw[3][0], b = tcalRaw[3][1];
    int mx = (int)(cc[0] + cc[1] * a + cc[2] * b + 0.5f), my = (int)(cc[3] + cc[4] * a + cc[5] * b + 0.5f);
    if (abs(mx - nx) > 14 || abs(my - ny) > 14)
      return false;
  }
  memcpy(tcal.c, cc, sizeof(cc));
  tcal.ok = true;
  return true;
}
static void tcalPoll(uint32_t now) {
  int16_t rx, ry;
  bool d = rawTouch(&rx, &ry);
  // a failed attempt starts over by itself
  if (tcalStep == 5) {
    if ((int32_t)(now - tcalFailAt) > 2500)
      tcalStart();
    return;
  }
  if (tcalStep >= 4)
    return;
  // re-calibrating: the old calibration still lets CANCEL work
  if (d && tcalN == 0 && !tcalWaitUp && tcal.ok) {
    int cx, cy;
    if (touched(&cx, &cy)) {
      int bi = hitBtn(cx, cy);
      if (bi >= 0 && btns[bi].id == B_NAV) {
        goBack();
        return;
      }
    }
  }
  // wait for the finger to come off before the next target
  if (tcalWaitUp) {
    if (d) {
      tcalLastDown = now;
      return;
    }
    if (now - tcalLastDown > 250) {
      tcalWaitUp = false;
      tcalN = 0;
      needRender = true;
    }
    return;
  }
  // a press must be held, not tapped: noise and bounce are ignored
  if (!d) {
    tcalN = 0;
    return;
  }
  if (tcalN == 0) {
    tcalAcc[0] = tcalAcc[1] = 0;
    tcalDownAt = now;
  }
  tcalAcc[0] += rx;
  tcalAcc[1] += ry;
  tcalN++;
  tcalLastDown = now;
  if (tcalN >= 12 && now - tcalDownAt > 260) {
    tcalRaw[tcalStep][0] = tcalAcc[0] / tcalN;
    tcalRaw[tcalStep][1] = tcalAcc[1] / tcalN;
    tcalN = 0;
    tcalWaitUp = true;
    tcalStep++;
    tapBlip();
    // presses that are nearly on the same spot cannot be on different corners
    if (tcalStep >= 2 && tcalStep <= 3) {
      float d = fmaxf(fabsf(tcalRaw[tcalStep - 1][0] - tcalRaw[0][0]), fabsf(tcalRaw[tcalStep - 1][1] - tcalRaw[0][1]));
      if (tcalStep == 2 && d < 400.0f) {
        tcalStep = 5;
        tcalFailAt = now;
      }
    }
    if (tcalStep == 4) {
      if (tcalSolve()) {
        savePrefs();
        showToast("Touch calibrated");
      } else {
        tcalStep = 5;
        tcalFailAt = now;
      }
    }
    needRender = true;
  }
}
static void tcalFinish() {
  navigate(SCR_MAIN);
}
// word-wrapped text, each line centred on the screen
static int tcalLines(const SkiFont* f, const char* s, int maxW, int y, uint16_t col, int lineH, bool draw) {
  char line[96];
  line[0] = 0;
  int ll = 0, lines = 0;
  const char* p = s;
  while (*p) {
    const char* e = p;
    while (*e && *e != ' ')
      e++;
    int wl = (int)(e - p);
    char cand[96];
    snprintf(cand, sizeof(cand), "%s%s%.*s", line, ll ? " " : "", wl, p);
    if (ll && textW(f, cand) > maxW) {
      if (draw)
        textC(f, line, W / 2, y + lines * lineH, col);
      lines++;
      ll = 0;
      line[0] = 0;
      continue;
    }
    strncpy(line, cand, sizeof(line) - 1);
    line[sizeof(line) - 1] = 0;
    ll = strlen(line);
    p = e;
    while (*p == ' ')
      p++;
  }
  if (ll) {
    if (draw)
      textC(f, line, W / 2, y + lines * lineH, col);
    lines++;
  }
  return lines * lineH;
}
static void drawTcalCross(int gx, int gy, uint16_t col, bool big) {
  if (big) {
    ring(gx, gy, 12.0f, 1.8f, col);
    disc(gx, gy, 2.5f, col);
    lineAA(gx - 20, gy, gx - 6, gy, 1.8f, col);
    lineAA(gx + 6, gy, gx + 20, gy, 1.8f, col);
    lineAA(gx, gy - 20, gx, gy - 6, 1.8f, col);
    lineAA(gx, gy + 6, gx, gy + 20, 1.8f, col);
  } else {
    ring(gx, gy, 8.0f, 1.4f, col);
    disc(gx, gy, 2.0f, col);
  }
}
static void drawTcal() {
  // the whole screen belongs to the calibration: no header, no tab bar, no cards behind it
  fillRect(0, 0, W, H, C_BG);
  const char* step;
  const char* head;
  const char* body;
  uint16_t hc = C_INK;
  char sb[24];
  if (tcalStep < 4) {
    snprintf(sb, sizeof(sb), "POINT %d OF 4", tcalStep + 1);
    step = sb;
    head = "Press and hold the cross";
    body = "Use a fingertip or a stylus. Keep pressing until the cross turns green.";
  } else if (tcalStep == 4) {
    step = "DONE";
    head = "Touch calibrated";
    body = "Tap CONTINUE.";
    hc = C_TEAL;
  } else {
    step = "TRY AGAIN";
    head = "Those presses did not line up";
    body = "Press exactly on each cross, one corner at a time. Starting over in a moment.";
    hc = C_ALERT;
  }
  // the text sits in the middle, well clear of the four corner targets
  int tw = W - 40, cy = H / 2;
  const SkiFont* hf = textW(&SKI_LG, head) > tw ? &SKI_MDB : &SKI_LG;
  int hh = hf == &SKI_LG ? 26 : 22;
  int bh = tcalLines(&SKI_SM, body, tw, 0, C_MUTE, 15, false);
  int total = 16 + hh + 6 + bh;
  int y = cy - total / 2 - 14;
  textC(&SKI_SM, step, W / 2, y, C_MUTE);
  textC(hf, head, W / 2, y + 16 + (SKI_LG.capH - hf->capH), hc);
  tcalLines(&SKI_SM, body, tw, y + 16 + hh + 6, C_MUTE, 15, true);
  int by = y + total + 14;
  if (tcalStep < 4) {
    bool hold = tcalN > 0 && !tcalWaitUp;
    if (hold) {
      int bw = imin(200, W - 80);
      rrFill(W / 2 - bw / 2, by, bw, 6, 3, C_CARD2);
      rrFill(W / 2 - bw / 2, by, imax(3, (int)(bw * imin(tcalN, 12) / 12.0f)), 6, 3, C_TEAL);
    }
    // re-calibrating: the old calibration still works, so the way out is offered
    if (tcal.ok) {
      int cw = 110, ch = 32, cx = W / 2 - cw / 2, cyy = H - ch - 14;
      bool pr = isPressed(B_NAV, 0);
      rrBorder(cx, cyy, cw, ch, 12, pr ? RGB(44, 52, 64) : C_CARD, C_LINE);
      textMid(&SKI_MDB, "CANCEL", W / 2, cyy + ch / 2, C_INK);
      addBtn(B_NAV, 0, cx, cyy, cw, ch);
    }
  } else if (tcalStep == 4) {
    int cw = 150, ch = 36, cx = W / 2 - cw / 2;
    rrFill(cx, by, cw, ch, 12, C_CARD2);
    textMid(&SKI_MDB, "CONTINUE", W / 2, by + ch / 2, C_AMBER);
    addBtn(B_TCAL, 1, cx, by, cw, ch);
  }
  // all four targets are always visible: done = green, the one to press = bright, the rest = dim
  for (int i = 0; i < 4; i++) {
    int gx, gy;
    tcalTarget(i, &gx, &gy);
    if (tcalStep == 5)
      drawTcalCross(gx, gy, mix(C_ALERT, C_BG, 120), false);
    else if (tcalStep >= 4 || i < tcalStep)
      drawTcalCross(gx, gy, C_TEAL, false);
    else if (i > tcalStep)
      drawTcalCross(gx, gy, C_DIM, false);
  }
  if (tcalStep < 4) {
    int gx, gy;
    tcalTarget(tcalStep, &gx, &gy);
    bool hold = tcalN > 0 && !tcalWaitUp;
    drawTcalCross(gx, gy, hold ? C_TEAL : C_AMBER, true);  // drawn last: nothing can cover it
  }
}
// ---- Render driver (strip by strip) ----------------------------------------
static void drawPage(int i) {
  switch (i) {
    case 0:
      drawPageArm();
      break;
    case 1:
      drawPageFinds();
      break;
    case 2:
      drawPageGroups();
      break;
    case 3:
      drawPageSaved();
      break;
    default:
      drawPageSetup();
      break;
  }
}
static void drawMain() {
  float pos = pagePos;
  bool rest = fabsf(pos - roundf(pos)) < 0.002f;
  bool reg = regBtns;
  if (!rest)
    regBtns = false;
  int lo = (int)floorf(pos);
  // the (at most two) pages that touch the screen
  for (int p = lo; p <= lo + 1; p++) {
    if (p < 0 || p >= N_PAGES)
      continue;
    int off = (int)lroundf((p - pos) * W);
    if (off <= -W || off >= W)
      continue;
    scp = &scArr[p];
    gOx = P(off);
    drawPage(p);
  }
  gOx = 0;
  scp = &scArr[curPage()];
  drawPill();
  drawTabBar(pos);
  regBtns = reg;
}
static void drawScreen() {
  switch (screen) {
    case SCR_BOOT:
      drawBoot();
      break;
    case SCR_MAIN:
      drawMain();
      break;
    case SCR_SCAN:
      scp = &scArr[N_PAGES];
      drawScan();
      break;
    case SCR_HUNT:
      scp = &scArr[N_PAGES + 3];
      drawHunt();
      break;
    case SCR_INFO:
      scp = &scArr[N_PAGES + 1];
      drawInfo();
      break;
    case SCR_FILES:
      scp = &scArr[N_PAGES + 2];
      drawFiles();
      break;
    case SCR_PORTAL:
      scp = &scArr[N_PAGES + 3];
      drawPortal();
      break;
    case SCR_REPORT:
      scp = &scArr[N_PAGES + 4];
      drawReport();
      break;
    case SCR_TEST:
      scp = &scArr[N_PAGES + 5];
      drawTest();
      break;
    case SCR_IGN:
      scp = &scArr[N_PAGES + 7];
      drawIgn();
      break;
    case SCR_CAL:
      scp = &scArr[N_PAGES + 6];
      drawCal();
      break;
#if HAS_TOUCH_CAL
    case SCR_TCAL:
      scp = &scArr[N_PAGES + 6];
      drawTcal();
      break;
#else
    case SCR_TCAL:
      break;
#endif
  }
  // find / tracker banner (slides in under the header)
  if (bannerActive() && screen != SCR_BOOT && screen != SCR_PORTAL && screen != SCR_TCAL)
    drawBanner();
  // message pill, fades out
  if ((screen == SCR_MAIN || screen == SCR_IGN || screen == SCR_FILES) && (int32_t)(toastUntil - gNow) > 0) {
    int32_t left = (int32_t)(toastUntil - gNow);
    uint8_t ta = left < 400 ? (uint8_t)(255 * left / 400) : 255;
    gOx = 0;
    clipReset();
    int tw = imin(W - 16, textW(&SKI_SM, toastMsg) + 26), th = 26, tx = (W - tw) / 2, ty = navY() - th - 8;
    rrFill(tx, ty, tw, th, 13, RGB(190, 64, 66), ta);
    rrFill(tx + 1, ty + 1, tw - 2, th - 2, 12, RGB(44, 10, 12), ta);
    drawText(&SKI_SM, toastMsg, tx + 13, ty + (th - SKI_SM.capH) / 2, RGB(255, 150, 150), ta);
  }
#if TOUCH_DEBUG
  // diagnostics strip: always visible while TOUCH_DEBUG is on
  if (screen != SCR_BOOT) {
    char s1[48], s2[48], sh[80];
    snprintf(s1, sizeof(s1), "%s ok%lu f%lu i%lu", touchOk ? "id" : "NOID", (unsigned long)tpOkN,
             (unsigned long)tpFailN, (unsigned long)tpIsrN);
    snprintf(s2, sizeof(s2), "d%lu u%lu s%lu p%d D%d L%lu F%lu", (unsigned long)tpDnN, (unsigned long)tpUpN,
             (unsigned long)tpSynN, digitalRead(PIN_TP_INT), (int)tpDown, (unsigned long)dbgLps,
             (unsigned long)dbgFrameMs);
    {
      char t2[12];
      snprintf(t2, sizeof(t2), " m%02X", tpPwr);
      strncat(s2, t2, sizeof(s2) - strlen(s2) - 1);
    }
    int hi = tpHistI;  // oldest first
    snprintf(sh, sizeof(sh), "%02X%02X%02X%02X%02X %02X%02X%02X%02X%02X %02X%02X%02X%02X%02X", tpHist[hi][0],
             tpHist[hi][1], tpHist[hi][2], tpHist[hi][3], tpHist[hi][4], tpHist[(hi + 1) % 3][0],
             tpHist[(hi + 1) % 3][1], tpHist[(hi + 1) % 3][2], tpHist[(hi + 1) % 3][3], tpHist[(hi + 1) % 3][4],
             tpHist[(hi + 2) % 3][0], tpHist[(hi + 2) % 3][1], tpHist[(hi + 2) % 3][2], tpHist[(hi + 2) % 3][3],
             tpHist[(hi + 2) % 3][4]);
    gOx = 0;
    clipReset();
    // solid strip so the lines never fight the title (diagnostics only)
    fillRect(0, 0, W, 34, C_BG);
    textC(&SKI_SM, s1, W / 2, 1, C_AMBER);
    textC(&SKI_SM, s2, W / 2, 12, C_AMBER);
    textC(&SKI_SM, sh, W / 2, 23, C_AMBER);
  }
  // proves what the touch layer sees, whatever the UI does
  if (gNow - tpDbgAt < 700) {
    gOx = 0;
    clipReset();
    ring(tpDbgX, tpDbgY, 9, 1.6f, C_TEAL);
    fillRect(tpDbgX - 14, tpDbgY, 29, 1, C_TEAL);
    fillRect(tpDbgX, tpDbgY - 14, 1, 29, C_TEAL);
  }
#endif
}
// everything that must be identical for all strips of one frame
static void prepareFrame() {
  gNow = millis();
  if (framesSeen != lastW) {
    lastW = framesSeen;
    actW = gNow;
  }
  if (bleSeen != lastB) {
    lastB = bleSeen;
    actB = gNow;
  }
  if (zbSeen != lastZ) {
    lastZ = zbSeen;
    actZ = gNow;
  }
  if (screen == SCR_MAIN) {
    int lo = (int)floorf(pagePos);
    if (lo == 1 || lo + 1 == 1)
      buildView(-1);
  } else if (screen == SCR_SCAN)
    buildView(filterCat);
  else if (screen == SCR_HUNT)
    hcOk = getContact(huntMac, &hc);
}
// scratch for the transposed strip (rotated modes)
static uint16_t* fbT = nullptr;
#define STRIP_MAX 16
static uint32_t stripHash[STRIP_MAX], stripAt[STRIP_MAX];
static bool stripOk[STRIP_MAX];
static void stripsInvalidate() {
  for (int i = 0; i < STRIP_MAX; i++)
    stripOk[i] = false;
}
// n pixels of fb; true = the panel already shows exactly this
static bool stripUnchanged(int by, size_t n) {
  int si = by / BAND_H;
  if (si < 0 || si >= STRIP_MAX)
    return false;
  uint32_t h = 0x9747b28cu ^ (uint32_t)rot;
  const uint32_t* p = (const uint32_t*)fb;
  size_t w = n >> 1;
  // MurmurHash3 body: a changed pixel anywhere changes the result
  for (size_t i = 0; i < w; i++) {
    uint32_t k = p[i] * 0xcc9e2d51u;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593u;
    h ^= k;
    h = (h << 13) | (h >> 19);
    h = h * 5u + 0xe6546b64u;
  }
  if (n & 1) {
    uint32_t k = fb[n - 1] * 0xcc9e2d51u;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593u;
    h ^= k;
    h = (h << 13) | (h >> 19);
    h = h * 5u + 0xe6546b64u;
  }
  h ^= (uint32_t)n;
  h ^= h >> 16;
  h *= 0x85ebca6bu;
  h ^= h >> 13;
  h *= 0xc2b2ae35u;
  h ^= h >> 16;
  // (the frame's own time: the same for every strip)
  uint32_t now = gNow;
  bool same = stripOk[si] && stripHash[si] == h && (uint32_t)(now - stripAt[si]) < 8000u + (uint32_t)si * 400u;
  if (!same) {
    stripOk[si] = true;
    stripHash[si] = h;
    stripAt[si] = now;
  }
  return same;
}
// the strip covers render rows [by, by+rows); the panel is always PANEL_W x PANEL_H
static void pushStrip(int by, int rows) {
  size_t n = (size_t)PW * rows;
  if (stripUnchanged(by, n)) {
    stripSkip++;
    return;
  }
  stripSent++;
  switch (rot) {
    case 0:
      panel->draw16bitRGBBitmap(0, by, fb, PW, rows);
      break;
    // 180 degrees: reverse the whole strip, mirror its position
    case 2:
      for (size_t i = 0; i < n; i++)
        fbT[i] = fb[n - 1 - i];
      panel->draw16bitRGBBitmap(0, PANEL_H - by - rows, fbT, PW, rows);
      break;
    // 90 degrees: render (x,y) -> panel (PANEL_W-1-y, x); the strip becomes a vertical band
    case 1:
      for (int r = 0; r < rows; r++) {
        const uint16_t* s = fb + (size_t)r * PW;
        uint16_t* d = fbT + (rows - 1 - r);
        for (int x = 0; x < PW; x++) {
          *d = s[x];
          d += rows;
        }
      }
      panel->draw16bitRGBBitmap(PANEL_W - by - rows, 0, fbT, rows, PANEL_H);
      break;
    // 270 degrees: render (x,y) -> panel (y, PANEL_H-1-x)
    default:
      for (int r = 0; r < rows; r++) {
        const uint16_t* s = fb + (size_t)r * PW;
        for (int x = 0; x < PW; x++)
          fbT[(size_t)(PANEL_H - 1 - x) * rows + r] = s[x];
      }
      panel->draw16bitRGBBitmap(by, 0, fbT, rows, PANEL_H);
      break;
  }
}
static uint32_t huntLastPing = 0;
// (a tune must stay in memory while it plays)
static Tone hudSeq[14];
// HUD beeps: the closer the target, the FASTER the pings come and the HIGHER they are, with more blips per ping.
// While you walk toward it the blips of each ping climb (a rising chirp); walking away they fall. Standing still they stay level.
// ms between pings: 1.1 s far away, about 0.3 s right on top of it
static uint32_t hudIv(float ema, bool contact) {
  if (!contact)
    return 1100;
  float v = 1100.0f - (ema + 95.0f) * 14.5f;
  return (uint32_t)(v < 300.0f ? 300.0f : (v > 1100.0f ? 1100.0f : v));
}
// +1 closing in, -1 moving away, 0 level (signal change over the last ~2 s)
static int hudTrend() {
  if (histN > 9) {
    float d = huntEma - hist[histN - 9];
    return d > 2.0f ? 1 : (d < -2.0f ? -1 : 0);
  }
  return 0;
}
static void hudPing(float ema, bool contact, uint32_t iv) {
  int n = 0;
  if (contact) {
    float nrm = clamp01((ema + 100.0f) / 60.0f);  // 0 at -100 dBm, 1 at -40 dBm
    int base = 1300 + (int)(1900.0f * nrm);       // 1.3 kHz far .. 3.2 kHz close
    int nb = ema > -55.0f ? 3 : (ema > -72.0f ? 2 : 1);
    int bl = 56 - (int)(18.0f * nrm), gap = 18;
    int tr = hudTrend();
    float step = tr > 0 ? 0.17f : (tr < 0 ? -0.13f : 0.0f);
    // never longer than the time to the next ping
    while (nb > 1 && nb * (bl + gap) + 78 > (int)iv - 25)
      nb--;
    for (int j = 0; j < nb; j++) {
      int hz = (int)(base * (1.0f + step * j));
      if (hz > 4200)
        hz = 4200;
      if (hz < 900)
        hz = 900;
      hudSeq[n].hz = hz;
      hudSeq[n++].ms = bl;
      hudSeq[n].hz = 0;
      hudSeq[n++].ms = gap;
    }
  }
  // the low thump (a small piezo is thin down here)
  hudSeq[n].hz = 260;
  hudSeq[n++].ms = 28;
  hudSeq[n].hz = 160;
  hudSeq[n++].ms = 50;
  sndPlay(hudSeq, n);
}
// the Track ping, timed between the strips of a frame too: a slow frame must not make the beeps late
static void huntPingTick(uint32_t now) {
  if (!hunting)
    return;
  // the cycle runs (and the ring spreads) even when muted
  if (hudOn) {
    bool contact = !huntSearching;
    uint32_t iv = hudIv(huntEma, contact);
    if (now - huntLastPing >= iv) {
      huntLastPing = now;
      huntPingAt = now;
      if (!muted && soundOn && HAS_SOUND)
        hudPing(huntEma, contact, iv);
    }
    return;
  }
  // silent while searching: a falling tone would only sound like a weak signal
  if (muted || !soundOn || !HAS_SOUND || huntSearching)
    return;
  int interval = constrain((int)(90 + (huntEma + 40.0f) * -9.0f), 70, 900);
  if (now - huntLastPing >= (uint32_t)interval) {
    huntLastPing = now;
    huntPingAt = now;
    tonePulse((int)constrain(400 + (huntEma + 90) * 18, 220, 1800), 18000);
  }
}
static void renderRange(int y0, int y1) {  // PHYSICAL rows
  y0 = imax(0, y0);
  y1 = imin(PH, y1);
  if (y1 <= y0)
    return;
  prepareFrame();
  bool first = true;
  for (int by = (y0 / BAND_H) * BAND_H; by < y1; by += BAND_H) {
    bandY = by;
    clipReset();
    int rows = imin(BAND_H, PH - by);
    fillRectP(0, by, PW, rows, C_BG);
    if (first) {
      nBtn = 0;
      regBtns = true;
      first = false;
    } else
      regBtns = false;
    drawScreen();
    if (termOn)
      termPost(by, rows);
    pushStrip(by, rows);
    if (hunting && screen == SCR_HUNT)
      huntPingTick(millis());
  }
  regBtns = true;
}
static void render() {
  markDirty(0, H);
}
static void present() {
  if (dirtyY1 <= dirtyY0)
    return;
  int a = dirtyY0, b = dirtyY1;
  dirtyY0 = dirtyY1 = 0;
  renderRange(a, b);
}
static int depthOf(Screen s) {
  switch (s) {
    case SCR_BOOT:
      return -1;
    case SCR_MAIN:
      return 0;
    case SCR_HUNT:
      return 2;
    default:
      return 1;
  }
}
// the new screen slides over the old one (the old one stays on the panel: no second buffer needed)
static uint16_t slideTmp[PANEL_H * 2];
// brightness to restore after a dip transition
static int dipBl = -1;
// landscape: a strip of the picture is a vertical band of the panel, so a sideways slide is a band of panel rows that grows
static void slideStripLand(int by, int rows, int off, int dir) {
  // the columns of the new picture that are on show: [xa, xa + off)
  int xa = dir > 0 ? 0 : PW - off;
  // panel row = picture x, panel column inside the band = rows - 1 - r
  if (rot == 1) {
    for (int r = 0; r < rows; r++) {
      const uint16_t* s = fb + (size_t)r * PW + xa;
      uint16_t* d = fbT + (rows - 1 - r);
      for (int i = 0; i < off; i++) {
        *d = s[i];
        d += rows;
      }
    }
    panel->draw16bitRGBBitmap(PANEL_W - by - rows, dir > 0 ? PW - off : 0, fbT, rows, off);
    // rot 3: panel row = PANEL_H - 1 - picture x, panel column = r
  } else {
    for (int r = 0; r < rows; r++) {
      const uint16_t* s = fb + (size_t)r * PW + xa;
      for (int i = 0; i < off; i++)
        fbT[(size_t)(off - 1 - i) * rows + r] = s[i];
    }
    panel->draw16bitRGBBitmap(by, dir > 0 ? 0 : PANEL_H - off, fbT, rows, off);
  }
}
static void slideCover(int dir) {
  // the slide writes to the panel directly: every strip is sent again afterwards
  stripsInvalidate();
  // rotated: dip the brightness, the caller draws the new screen, then it comes back up
  if (rot == 2) {
    dipBl = blLevel;
    for (int i = 4; i >= 0; i--) {
      blSet(dipBl * i / 4);
      delay(10);
    }
    return;
  }
  const uint32_t DUR = 230;
  uint32_t t0 = millis();
  for (;;) {
    float t = (float)(int32_t)(millis() - t0) / (float)DUR;
    if (t >= 1.0f)
      break;
    int off = ((int)(easeOut(t) * PW)) & ~1;  // even
    if (off < 8)
      continue;
    if (off > PW)
      off = PW;
    prepareFrame();
    regBtns = false;
    for (int by = 0; by < PH; by += BAND_H) {
      bandY = by;
      clipReset();
      int rows = imin(BAND_H, PH - by);
      fillRectP(0, by, PW, rows, C_BG);
      drawScreen();
      if (termOn)
        termPost(by, rows);
      if (rot & 1) {
        slideStripLand(by, rows, off, dir);
        continue;
      }
      for (int r = 0; r + 1 < rows; r += 2) {  // two rows at a time, packed
        const uint16_t* s0 = fb + (size_t)r * PW;
        const uint16_t* s1 = fb + (size_t)(r + 1) * PW;
        if (dir > 0) {
          memcpy(slideTmp, s0, (size_t)off * 2);
          memcpy(slideTmp + off, s1, (size_t)off * 2);
          panel->draw16bitRGBBitmap(PW - off, by + r, slideTmp, off, 2);
        } else {
          memcpy(slideTmp, s0 + (PW - off), (size_t)off * 2);
          memcpy(slideTmp + off, s1 + (PW - off), (size_t)off * 2);
          panel->draw16bitRGBBitmap(0, by + r, slideTmp, off, 2);
        }
      }
    }
  }
  regBtns = true;
}
static Screen huntFrom = SCR_MAIN;
static void onLeave(Screen s) {
  if (s == SCR_CAL)
    calLeave();
#if HAS_TOUCH_CAL
  if (s == SCR_TCAL)
    tcalLeave();
#endif
  if (s == SCR_HUNT) {
    hunting = false;
    bleHold(false);
    sliceWifi = true;
    sliceAt = millis();
    applyWifiState();
  }
}
static void applyRotation(uint8_t r, bool redraw) {
  r &= 3;
  rot = r;
  bool land = (r & 1);
  PW = land ? PANEL_H : PANEL_W;
  PH = land ? PANEL_W : PANEL_H;
  // layout space (the AMOLED draws it at 1.5x)
  W = land ? UI_H0 : UI_W0;
  H = land ? UI_W0 : UI_H0;
  stripsInvalidate();
  bandY = 0;
  clipReset();
  nBtn = 0;
  pressId = B_NONE;
  for (int i = 0; i < N_PAGES + 7; i++) {
    scArr[i].y = scArr[i].v = 0;
    scArr[i].drag = false;
  }
  dirtyY0 = dirtyY1 = 0;
  if (redraw) {
    renderRange(0, PH);
    needRender = false;
  } else
    needRender = true;
}
static void navigate(Screen s) {
  if (s == screen)
    return;
  Screen old = screen;
  int dir = (depthOf(s) >= depthOf(old)) ? 1 : -1;
  onLeave(old);
  screen = s;
  for (int i = N_PAGES; i < N_PAGES + 8; i++) {
    scArr[i].y = scArr[i].v = 0;
    scArr[i].drag = false;
  }
  pressId = B_NONE;
  if (s == SCR_FILES)
    loadFiles();
  if (s == SCR_SCAN && old != SCR_HUNT)
    selValid = false;
  if (s != SCR_TCAL)
    slideCover(dir);
  dirtyY0 = dirtyY1 = 0;
  renderRange(0, PH);  // final frame + registers the buttons
  if (dipBl >= 0) {
    for (int i = 1; i <= 5; i++) {
      blSet(dipBl * i / 5);
      delay(10);
    }
    dipBl = -1;
  }
  needRender = false;
}
static void startHunt(const Contact& c) {
  memcpy(huntMac, c.mac, 6);
  hunting = true;
  huntEma = huntShown = c.ema;
  huntLastSeen = c.lastMs;
  huntSampleMs = millis();
  histN = 0;
  lockHuntChannel(c);
  bleHold(c.radio == RAD_WIFI);
  huntFrom = screen;
  huntSearching = false;
}
static void serviceHunt(uint32_t now, float dt) {
  if (!hunting)
    return;
  Contact c;
  if (!getContact(huntMac, &c))
    return;
  static uint32_t lastDecay = 0, lastHist = 0, lastCrumb = 0;
  // fresh sample -> smooth per sample, not per loop pass
  if (c.lastMs != huntLastSeen) {
    huntLastSeen = c.lastMs;
    huntEma += 0.35f * ((float)c.rssi - huntEma);
    huntSampleMs = now;
  } else if ((int32_t)(now - c.lastMs) > 2500 && now - lastDecay > 150) {
    lastDecay = now;
    huntEma -= 0.8f;
    if (huntEma < -100)
      huntEma = -100;
  }
  // keep the radio on the network's real channel, and find it again if it goes quiet
  if (c.radio == RAD_WIFI && radioOn && radioInit && !wifiFallback) {
    static uint32_t sweepAt = 0;
    static int sweepStep = 0, baseCh = 6;
    bool silent = (int32_t)(now - c.lastMs) > 1800;
    if (!silent) {
      huntSearching = false;
      // the beacon says which channel it really uses
      if (c.chOk && c.ch >= 1 && c.ch <= 13 && c.ch != huntLockedCh) {
        esp_wifi_set_channel(c.ch, WIFI_SECOND_CHAN_NONE);
        huntLockedCh = c.ch;
      }
      // wrong or drifting channel, or a quiet beacon: sweep outwards from where we were until it is heard again
    } else {
      if (!huntSearching) {
        huntSearching = true;
        baseCh = (huntLockedCh >= 1 && huntLockedCh <= 13) ? huntLockedCh : (c.ch >= 1 && c.ch <= 13 ? c.ch : 6);
        sweepStep = 0;
        sweepAt = now;
      }
      if ((int32_t)(now - sweepAt) >= 350) {
        sweepAt = now;
        sweepStep++;
        // base, +1, -1, +2, -2 ... wrapping over channels 1..13
        int off = ((sweepStep + 1) / 2) * ((sweepStep & 1) ? 1 : -1);
        int chn = (((baseCh - 1 + off) % 13) + 13) % 13 + 1;
        esp_wifi_set_channel(chn, WIFI_SECOND_CHAN_NONE);
        huntLockedCh = (uint8_t)chn;
      }
    }
  } else
    huntSearching = false;
  huntShown += (huntEma - huntShown) * (1 - expf(-dt * 9.0f));
  if (now - lastHist >= 250) {
    lastHist = now;
    if (histN >= (int)sizeof(hist)) {
      memmove(hist, hist + 1, sizeof(hist) - 1);
      histN = sizeof(hist) - 1;
    }
    hist[histN++] = (int8_t)constrain((int)lroundf(huntEma), -127, 0);
  }
  if (now - lastCrumb > 2500) {
    lastCrumb = now;
    logHuntBreadcrumb(c, huntEma);
  }
  huntPingTick(now);
}
// ---- Actions ---------------------------------------------------------------
static void armChanged(bool wasOff) {
  if (armedMask && wasOff && needNewSession)
    startNewSession();
  if (!armedMask)
    needNewSession = true;
  radioDirty = true;
  radioDirtyAt = millis();
}
static Screen backTarget() {
  switch (screen) {
    case SCR_HUNT:
      return huntFrom == SCR_SCAN ? SCR_SCAN : SCR_MAIN;
    case SCR_SCAN:
    case SCR_INFO:
    case SCR_FILES:
    case SCR_PORTAL:
    case SCR_REPORT:
    case SCR_TEST:
    case SCR_CAL:
    case SCR_TCAL:
    case SCR_IGN:
      return SCR_MAIN;
    default:
      return screen;
  }
}
static void goBack() {
#if HAS_TOUCH_CAL
  // the first touch set-up cannot be skipped: nothing works without it
  if (screen == SCR_TCAL && !tcal.ok)
    return;
#endif
  Screen t = backTarget();
  if (t == screen)
    return;
  if (screen == SCR_PORTAL)
    portalStop();
  sndPlay(SND_BACK, 2);
  navigate(t);
}
static void trackSelected(const Contact* list, int n) {
  if (!n)
    return;
  int si = selIndex();
  if (si < 0)
    si = 0;
  startHunt(list[si]);
  tapBlip();
  navigate(SCR_HUNT);
}
static void bannerTapped() {  // banner tap: track the device it is about
  uint8_t mac[6];
  memcpy(mac, banMac, 6);
  bannerHide();
  int i = indexOfMac(mac);
  if (i < 0) {
    showToast("No longer in range");
    return;
  }
  if (screen == SCR_HUNT)
    return;
  Contact c = contacts[i];
  memcpy(selMac, c.mac, 6);
  selValid = true;
  startHunt(c);
  tapBlip();
  navigate(SCR_HUNT);
}
static void setTerm(bool on) {
  applyThemeMode(on ? 1 : 0);
  savePrefs();
  if (!muted && soundOn && HAS_SOUND)
    sndPlay(SND_FIND, 3);
  showToast(on ? "ALIEN: tracker interface on" : "ALIEN off: normal interface");
  needRender = true;
}
static const Tone SND_ALS[] = {{2000, 40}, {0, 40}, {260, 30}, {160, 60}};  // the HUD ping, as a confirmation
static void setHud(bool on) {
  applyThemeMode(on ? 2 : 0);
  savePrefs();
  if (!muted && soundOn && HAS_SOUND)
    sndPlay(SND_ALS, 4);
  showToast(on ? "ALIENS: blue tracker on" : "ALIENS off: normal interface");
  needRender = true;
}
static void setRowTap(int id) {
  switch (id) {
    case 20:
      setTerm(!(termOn && !hudOn));
      return;
    case 21:
      setHud(!hudOn);
      return;
    case 0:
      raiseWake = !raiseWake;
      break;
    case 1:
      shakePause = !shakePause;
      break;
    case 2:
      powerMode = (powerMode + 1) % 3;
      saverOn = saverActive();
      applySaver();
      break;
    case 3:
      setDiscreet(!discreet);
      return;
    case 4:
      followMin = followMin == 0 ? 5 : (followMin == 5 ? 10 : (followMin == 10 ? 20 : 0));
      break;
    case 15:
      skimWatch = !skimWatch;
      showToast(skimWatch ? "Skimmer alert on (Bluetooth LE only)" : "Skimmer alert off");
      break;
    case 16:
      storMode = (storMode + 1) % 3;
      savePrefs();
      showToast(storMode == 1 ? "SD card only: restart to apply"
                              : (storMode == 2 ? "Flash only: restart to apply" : "SD first: restart to apply"));
      break;
    // the Ignored screen: see every hidden device and bring it back
    case 5:
      navigate(SCR_IGN);
      return;
    case 6:
      navigate(SCR_TEST);
      return;
    case 7:
      orient = (orient + 1) % (HAS_IMU_BOARD ? 5 : 4);
      applyRotation(orient < 4 ? orient : rot, true);
      savePrefs();
      showToast(orient == 4 ? "Auto: follows the sensor" : "Orientation changed");
      return;
    case 8:
      autoOff = (autoOff + 1) & 3;
      break;
    case 9:
      calStart();
      return;
    case 12:
      cycleListView();
      return;
    case 22: {  // Normal -> Swap R/B -> Invert -> Swap + invert; shows on the screen at once
      static const char* const CT[4] = {"Screen colors: normal", "Screen colors: red and blue swapped",
                                        "Screen colors: inverted", "Screen colors: swapped and inverted"};
      colorMode = (colorMode + 1) & 3;
      applyColorMode();
      savePrefs();
      showToast(CT[colorMode]);
      needRender = true;
      return;
    }
    case 14:
      bootLogoCycle();
      return;
#if HAS_BOOTSOUND
    case 13:
      bootSoundCycle();
      return;
#endif
    case 11:
      logMode = (logMode + 1) % 3;
      logFull = false;
      ledgerAutoAt = millis();
      showToast(logMode == 0
                    ? "Manual: tap SAVE LOG on the LOG page"
                    : (logMode == 1 ? "Auto: saved every 5 min and when you stop" : "Live: every new find is written"));
      break;
#if HAS_TOUCH_CAL
    case 10:
      tcalStart();
      return;
#endif
  }
  savePrefs();
  needRender = true;
}
// DELETE LOGS: asks once, deletes on the second tap within 3 s
static void filesDeleteTap() {
  static uint32_t armedAt = 0;
  int n = logFileCount();
  if (!n) {
    showToast("No log files");
    return;
  }
  char m[48];
  if (armedAt && millis() - armedAt < 3000) {
    int d = logDeleteAll();
    armedAt = 0;
    loadFiles();
    snprintf(m, sizeof(m), "Deleted %d log file%s", d, d == 1 ? "" : "s");
  } else {
    armedAt = millis();
    snprintf(m, sizeof(m), "Tap again to delete %d file%s", n, n == 1 ? "" : "s");
  }
  showToast(m);
  needRender = true;
}
static void cycleListView() {
  listView = (uint8_t)((listView + 1) & 3);
  ordN = 0;
  savePrefs();
  char m[40];
  snprintf(m, sizeof(m), "List: %s", LV_NAME[listView]);
  showToast(m);
  needRender = true;
}
static void onBtn(uint8_t id, int8_t arg) {
  if (id == B_BANNER) {
    bannerTapped();
    return;
  }
  if (id == B_IGNCHIP) {
    navigate(SCR_IGN);
    return;
  }
  // the count above the list: Strongest / Strong only / Newest / A-Z       // the "N hidden" chip / row on the FINDS page
  if (id == B_SORT) {
    tapBlip();
    cycleListView();
    return;
  }
  if (id == B_PILL) {
    if ((screen == SCR_MAIN || screen == SCR_SCAN) && armedMask) {
      scanning = !scanning;
      tapBlip();
      needRender = true;
    }
    return;
  }
  switch (screen) {
    case SCR_MAIN:
      if (id == B_TAB) {
        if (arg != pageTarget) {
          pageTarget = arg;
          tapBlip();
          needRender = true;
        }
      } else if (id == B_TILE) {
        // the other radio side is armed: refuse and say why
        if (tileBlocked(arg)) {
          snprintf(toastMsg, sizeof(toastMsg), arg == CAT_ZT ? "Turn off Wi-Fi groups first" : "Turn off ZB/TH first");
          toastUntil = millis() + 1900;
          shakeIdx = arg;
          shakeAt = millis();
          sndPlay(SND_BACK, 2);
          needRender = true;
          break;
        }
        bool wasOff = (armedMask == 0);
        catOn[arg] = !catOn[arg];
        syncArmed();
        armChanged(wasOff);
        if (catOn[arg])
          sndPlay(SND_ON, 2);
        else
          sndPlay(SND_OFF, 2);
      } else if (id == B_TRACK)
        trackSelected(vlist, vcount);
      else if (id == B_SHARE) {
        tapBlip();
        portalStart();
        navigate(SCR_PORTAL);
      } else if (id == B_DIM) {
        autoDim = !autoDim;
        savePrefs();
        tapBlip();
        needRender = true;
      } else if (id == B_SCENE)
        applyScene(arg);
      else if (id == B_REPORT) {
        navigate(SCR_REPORT);
      } else if (id == B_SAVE) {
        int r = ledgerSave();
        tapBlip();
        needRender = true;
        char m[56];
        if (r == SAVE_OK) {
          if (ledgerDropped)
            snprintf(m, sizeof(m), "Saved %u (%u left out)", (unsigned)ledgerSavedN, (unsigned)ledgerDropped);
          else
            snprintf(m, sizeof(m), "Saved %u device%s", (unsigned)ledgerSavedN, ledgerSavedN == 1 ? "" : "s");
        } else if (r == SAVE_NONE)
          snprintf(m, sizeof(m), "Nothing found yet to save");
        else if (r == SAVE_NOFS)
          snprintf(m, sizeof(m), "No storage: nothing was saved");
        else if (r == SAVE_FULL)
          snprintf(m, sizeof(m), "Storage full: delete logs in Storage");
        else
          snprintf(m, sizeof(m), "Could not write the log");
        showToast(m);
      } else if (id == B_SETROW)
        setRowTap(arg);
      else if (id == B_GROUP) {
        filterCat = arg;
        navigate(SCR_SCAN);
      }  // a group card opens that group's finds
      // plays the chime as a preview
      else if (id == B_ALERT) {
        alertMode = (alertMode + 1) % 3;
        savePrefs();
        if (alertMode)
          sndPlay(SND_FIND, 3);
        needRender = true;
      } else if (id == B_HELP) {
        tapBlip();
        navigate(SCR_INFO);
      } else if (id == B_STORE) {
        tapBlip();
        navigate(SCR_FILES);
      }
      break;
    case SCR_SCAN:
      if (id == B_NAV) {
        if (arg == 0)
          trackSelected(vlist, vcount);
        else if (arg == 1)
          goBack();
        else {
          scanning = !scanning;
          tapBlip();
          needRender = true;
        }
      }
      break;
    case SCR_HUNT:
      if (id == B_NAV) {
        if (arg == 0)
          goBack();
        else {
          muted = !muted;
          tapBlip();
          needRender = true;
        }
      }
      break;
    case SCR_INFO:
    case SCR_REPORT:
      if (id == B_NAV)
        goBack();
      break;
    case SCR_FILES:
      if (id == B_NAV) {
        if (arg == 0)
          goBack();
        else
          filesDeleteTap();
      }
      break;
    case SCR_TEST:
      if (id == B_NAV) {
        if (arg == 0)
          goBack();
        else
          sndPlay(SND_BOOT, 3);
      }
      break;
    case SCR_IGN:
      if (id == B_NAV) {
        if (arg == 0)
          goBack();
        else {
          ignoreClear();
          showToast("All restored");
          needRender = true;
        }
      } else if (id == B_IGN && arg >= 0 && arg < nIgnore) {
        char m[44];
        snprintf(m, sizeof(m), "Restored %s", ign[arg].name[0] ? ign[arg].name : "device");
        ignoreRemoveAt(arg);
        showToast(m);
        tapBlip();
        needRender = true;
      }
      break;
    case SCR_CAL:
      if (id == B_NAV) {
        if (arg == 0)
          goBack();
        else
          calRestart();
      }
      break;
#if HAS_TOUCH_CAL
    case SCR_TCAL:
      if (id == B_NAV && tcal.ok)
        goBack();
      else if (id == B_TCAL) {
        if (arg == 1)
          tcalFinish();
        else
          tcalStart();
      }
      break;
#else
    case SCR_TCAL:
      break;
#endif
    case SCR_PORTAL:
      if (id == B_NAV) {
        goBack();
      }
      break;
    default:
      break;
  }
}
static void onListTap(int x, int y) {
  bool onFinds = (screen == SCR_MAIN && curPage() == 1);
  if (!onFinds && screen != SCR_SCAN)
    return;
  int top = contentTop() + 18, bot = contentBot();
  if (y < top || y >= bot || x < 12 || x > W - 14)
    return;
  int idx = (y - top + (int)sc.y) / LIST_PITCH;
  int within = (y - top + (int)sc.y) % LIST_PITCH;
  if (idx < 0 || idx >= vcount || within >= LIST_CARD)
    return;
  if (selValid && macEq(selMac, vlist[idx].mac)) {  // second tap on the selected row = track it
    startHunt(vlist[idx]);
    tapBlip();
    navigate(SCR_HUNT);
    return;
  }
  memcpy(selMac, vlist[idx].mac, 6);
  selValid = true;
  tapBlip();
  needRender = true;
}
// ---- Touch state machine ---------------------------------------------------
static bool screenScrollable() {
  // (the Self-test is longer than the screen: it could not be scrolled before)
  if (screen == SCR_SCAN || screen == SCR_INFO || screen == SCR_FILES || screen == SCR_TEST)
    return true;
  if (screen == SCR_MAIN)
    return curPage() != 0;
  return false;
}
static bool wakeDisplay() {
  lastTouchMs = millis();
  if (screenOff) {
    panelSleep(false);
    dimmed = false;
    blSet(imax(12, bright / 6));
    blTarget = bright;
    needRender = true;
    return true;
  }
  if (dimmed) {
    dimmed = false;
    blTarget = bright;
    return true;
  }
  return false;
}
static bool tDown = false, tDrag = false, tSlider = false, tSwallow = false, tSwipe = false, tPager = false,
            tLong = false;
// rows hold still while a finger is on the list, for 3 s after, and while it is scrolled
static bool listFrozen() {
  if (tDown || (int32_t)(millis() - lastTouchMs) < 3000)
    return true;
  float sy = (screen == SCR_SCAN) ? scArr[N_PAGES].y : scArr[1].y;
  return sy > 3.0f;
}
static int tsx = 0, tsy = 0, tx = 0, ty = 0, tlx = 0, tly = 0, tMiss = 0;
static uint32_t tsMs = 0, tlMs = 0, tPoll = 0;
static float pageStartPos = 0;
static uint8_t sliderKind = 0;  // 0 = brightness, 1 = volume
static void setSliderFromX(int x) {
  float v = clamp01((float)(x - sliderX) / (float)sliderW);
  if (sliderKind == 0) {
    bright = 20 + (int)(v * 235);
    if (!dimmed)
      blTarget = bright;
  } else {
    int nv = (int)(v * 100.0f + 0.5f);
    // a small dead zone at the left end = silent
    if (nv < 4)
      nv = 0;
    if (nv != soundVol) {
      soundVol = nv;
      applyVolume();
    }
  }
  needRender = true;
}
static void touchDown(int x, int y, uint32_t now) {
  // amplifier ready before the blip (no pre-roll lag)
  audioWarm();
#if TOUCH_DEBUG
  Serial.printf("touch down %d,%d\n", x, y);
#endif
  tDown = true;
  tDrag = false;
  tSlider = false;
  tSwipe = false;
  tPager = false;
  tLong = false;
  tsx = tx = tlx = x;
  tsy = ty = tly = y;
  tsMs = tlMs = now;
  tMiss = 0;
  tSwallow = wakeDisplay();
  if (tSwallow)
    return;
  if (screen == SCR_MAIN)
    scp = &scArr[curPage()];
  sc.v = 0;
  sc.drag = false;
  int b = hitBtn(x, y);
  pressTicked = false;
  if (b >= 0) {
    pressId = btns[b].id;
    pressArg = btns[b].arg;
    // blip the instant a button is touched. Tiles keep their own arm/disarm tones on release, sliders sound on release.
    if (pressId != B_TILE && pressId != B_SLIDER && pressId != B_VOL)
      pressTick();
    if (pressId == B_SLIDER) {
      tSlider = true;
      sliderKind = 0;
      setSliderFromX(x);
    } else if (pressId == B_VOL) {
      tSlider = true;
      sliderKind = 1;
      setSliderFromX(x);
    }
  }
  needRender = true;
}
static void touchMove(int x, int y, uint32_t now) {
  tx = x;
  ty = y;
  tMiss = 0;
  lastTouchMs = now;
  if (tSwallow)
    return;
  if (tSlider) {
    setSliderFromX(x);
    return;
  }
  int dx = x - tsx, dy = y - tsy;
  if (!tDrag && !tSwipe && !tPager) {
    if (screenScrollable() && abs(dy) > 9 && abs(dy) > abs(dx)) {
      tDrag = true;
      sc.drag = true;
      pressId = B_NONE;
    } else if (screen == SCR_MAIN && abs(dx) > 10 && abs(dx) * 10 > abs(dy) * 12) {
      tPager = true;
      pageDragging = true;
      pageStartPos = pagePos;
      pageVel = 0;
      pressId = B_NONE;
      needRender = true;
    } else if (screen != SCR_MAIN && abs(dx) > 22 && abs(dx) > abs(dy) * 2) {
      tSwipe = true;
      pressId = B_NONE;
      needRender = true;
    } else if (abs(dx) > 14 || abs(dy) > 14) {
      if (pressId != B_NONE) {
        pressId = B_NONE;
        needRender = true;
      }
    }
  }
  float dtS = (int32_t)(now - tlMs) / 1000.0f;
  if (dtS < 0.004f)
    dtS = 0.004f;
  if (tPager) {
    float p = pageStartPos - (float)(x - tsx) / (float)W;
    if (p < 0)
      p *= 0.45f;  // rubber band at both ends
    if (p > N_PAGES - 1)
      p = (N_PAGES - 1) + (p - (N_PAGES - 1)) * 0.45f;
    pagePos = p;
    pageVel = 0.6f * pageVel + 0.4f * (-(float)(x - tlx) / (float)W / dtS);
    tlx = x;
    tlMs = now;
    needRender = true;
  }
  if (tDrag) {
    float d = (float)(y - tly);
    float resist = (sc.y < 0 || sc.y > sc.max) ? 0.4f : 1.0f;
    sc.y -= d * resist;
    sc.v = 0.65f * sc.v + 0.35f * (-d / dtS);
    tly = y;
    tlMs = now;
    needRender = true;
  }
}
static void touchUpImpl(uint32_t now) {
#if TOUCH_DEBUG
  Serial.printf("touchUp: swallow=%d pager=%d drag=%d swipe=%d slider=%d long=%d dimmed=%d\n", (int)tSwallow,
                (int)tPager, (int)tDrag, (int)tSwipe, (int)tSlider, (int)tLong, (int)dimmed);
#endif
  tDown = false;
  sc.drag = false;
  if (tSwallow) {
    tSwallow = false;
    pressId = B_NONE;
    return;
  }
  uint8_t pid = pressId;
  int8_t parg = pressArg;
  pressId = B_NONE;
  needRender = true;
  // volume: play a sample so you hear the level
  if (tSlider) {
    tSlider = false;
    savePrefs();
    if (sliderKind == 1)
      sndPlay(SND_ON, 2);
    else
      tapBlip();
    return;
  }
  if (tPager) {
    tPager = false;
    pageDragging = false;
    float proj = pagePos + constrain(pageVel * 0.2f, -0.8f, 0.8f);
    int tg = (int)lroundf(proj);
    pageTarget = constrain(tg, 0, N_PAGES - 1);
    return;
  }
  if (tDrag) {
    tDrag = false;
    if (fabsf(sc.v) > 2500)
      sc.v = sc.v > 0 ? 2500 : -2500;
    if ((int32_t)(now - tlMs) > 90)
      sc.v = 0;
    return;
  }
  int dx = tx - tsx, dy = ty - tsy;
  if (tSwipe) {
    tSwipe = false;
    if (dx > 55 && abs(dy) < 60 && now - tsMs < 700)
      goBack();
    return;
  }
  if (tLong) {
    tLong = false;
    return;
  }
  if (abs(dx) > 14 || abs(dy) > 14)
    return;
  if (now - tsMs < 25)
    return;
  int b = hitBtn(tsx, tsy);
#if TOUCH_DEBUG
  Serial.printf("touch up dx=%d dy=%d dur=%lu pid=%d hit=%d nBtn=%d\n", dx, dy, (unsigned long)(now - tsMs), (int)pid,
                b, nBtn);
#endif
  if (b >= 0 && pid != B_NONE && btns[b].id == pid && btns[b].arg == parg) {
    onBtn(btns[b].id, btns[b].arg);
    return;
  }
  onListTap(tsx, tsy);
}
static void touchUp(uint32_t now) {
  touchUpImpl(now);
  pressTicked = false;
}
static void pollTouch(uint32_t now) {
#if HAS_TOUCH_CAL
  if (screen == SCR_TCAL && tcalPolling()) {
    tcalPoll(now);
    return;
  }
#endif
  if (now - tPoll < 6)
    return;
  tPoll = now;
  int x, y;
  if (touched(&x, &y)) {
    if (!tDown)
      touchDown(x, y, now);
    else
      touchMove(x, y, now);
  } else if (tDown) {
    if (++tMiss >= TOUCH_MISS_N) {
      tMiss = 0;
      touchUp(now);
    }
  }
  // long-press a tile -> open that category's finds
  if (tDown && !tDrag && !tPager && !tSwipe && !tSlider && !tSwallow && !tLong && screen == SCR_MAIN &&
      pressId == B_TILE && now - tsMs > 480) {
    tLong = true;
    filterCat = pressArg;
    pressId = B_NONE;
    tapBlip();
    navigate(SCR_SCAN);
  }
  // long-press the status pill -> discreet mode (silent, dim, no banners)
  if (tDown && !tDrag && !tPager && !tSwipe && !tSlider && !tSwallow && !tLong && pressId == B_PILL &&
      now - tsMs > 700) {
    tLong = true;
    pressId = B_NONE;
    setDiscreet(!discreet);
  }
  // long-press a find row -> ignore that device (hidden from lists, counts and alerts until the ignore list is cleared)
  if (tDown && !tDrag && !tPager && !tSwipe && !tSlider && !tSwallow && !tLong && pressId == B_NONE &&
      ((screen == SCR_MAIN && curPage() == 1) || screen == SCR_SCAN) && now - tsMs > 750) {
    int top = contentTop() + 18, bot = contentBot();
    if (tsy >= top && tsy < bot && tsx >= 12 && tsx <= W - 14) {
      int idx = (tsy - top + (int)sc.y) / LIST_PITCH, within = (tsy - top + (int)sc.y) % LIST_PITCH;
      if (idx >= 0 && idx < vcount && within < LIST_CARD) {
        tLong = true;
        uint8_t mac[6];
        memcpy(mac, vlist[idx].mac, 6);
        char nm[24];
        snprintf(nm, sizeof(nm), "%s", vlist[idx].nick[0] ? vlist[idx].nick : kindLabel(vlist[idx]));
        uint8_t fm = (uint8_t)vlist[idx].fam;
        char m[44];
        snprintf(m, sizeof(m), "Hidden: %s", nm);
        ignoreAdd(mac, nm, fm);
        showToast(m);
        tapBlip();
        needRender = true;
      }
    }
  }
}
static bool tickScroll(float dt) {
  Scroller& s = *scp;
  if (s.drag)
    return false;
  bool moving = false;
  if (s.y < 0 || s.y > s.max) {
    float target = s.y < 0 ? 0.0f : s.max;
    s.y += (target - s.y) * (1 - expf(-dt * 14.0f));
    s.v = 0;
    moving = true;
    if (fabsf(target - s.y) < 0.4f)
      s.y = target;
  } else if (fabsf(s.v) > 8) {
    s.y += s.v * dt;
    s.v *= expf(-dt * 4.2f);
    moving = true;
    if (s.y < 0 || s.y > s.max)
      s.v *= 0.3f;
  } else
    s.v = 0;
  return moving;
}
static bool tickPager(float dt) {
  if (screen != SCR_MAIN || pageDragging)
    return false;
  float diff = (float)pageTarget - pagePos;
  // needRender: the settled frame registers the buttons and must not be dropped by the frame limiter
  if (fabsf(diff) < 0.003f) {
    if (pagePos != (float)pageTarget) {
      pagePos = (float)pageTarget;
      needRender = true;
      return true;
    }
    return false;
  }
  pagePos += diff * (1 - expf(-dt * 13.0f));
  return true;
}
static bool screenAnimating() {
  if (bannerAnimating() && screen != SCR_BOOT)
    return true;
  switch (screen) {
    case SCR_BOOT:
      return true;
    case SCR_HUNT:
      return true;
    case SCR_SCAN:
      return vcount == 0;
    case SCR_MAIN:
      return (curPage() == 1 && vcount == 0 && fabsf(pagePos - 1.0f) < 0.05f) || (int32_t)(toastUntil - millis()) > 0 ||
             (shakeIdx >= 0 && (int32_t)(millis() - shakeAt) < 400);
    default:
      return false;
  }
}
// =============================================================================
//  Setup / loop
// =============================================================================
void loop() {
  uint32_t now = millis();
  static uint32_t lastNow = 0, lastFrame = 0, lastPill = 0, lastList = 0;
  float dt = lastNow ? (now - lastNow) / 1000.0f : 0.016f;
  if (dt > 0.1f)
    dt = 0.1f;
  lastNow = now;
  {
    static uint32_t n = 0, t = 0;
    n++;
    if ((int32_t)(now - t) >= 1000) {
      dbgLps = n;
      n = 0;
      t = now;
    }
  }
  pollTouch(now);
#if CYD_TOUCH_DEBUG
  {
    static uint32_t dbgAt = 0;
    if ((int32_t)(now - dbgAt) >= 5000) {
      dbgAt = now;
      Serial.printf("touchdbg: screen=%d tcal.ok=%d  irq=%d z1=%d z2=%d z=%d raw=%d,%d down=%d reads=%lu\n",
                    (int)screen, (int)tcal.ok, ts.dirq, ts.dz1, ts.dz2, ts.dz, (int)ts.pt.x, (int)ts.pt.y, (int)ts.down,
                    (unsigned long)tpOkN);
    }
  }
#endif
  sndService();
  // clock, tracker follow, motion sensor, power saver, banner timeout
  plusService(now);
  // a pending chime waits out the gap, it is never dropped
  {
    static uint32_t lc = 0;
    if (findChime && (int32_t)(now - lc) > (alertMode == 2 ? 1500 : 4000)) {
      findChime = false;
      lc = now;
      onFindAlert();
    }
  }
  // radios are (re)configured after the tap has been drawn
  if (radioDirty && !portalOn && now - radioDirtyAt > 250) {
    radioDirty = false;
    if (armedMask)
      radioApply();
    else
      radioStop();
  }
#ifdef HAS_IEEE154
  drainZb();
#endif
  serviceGps();
  hopChannels();
  bleHousekeeping();
  drainLogQueue();
  ledgerService(now);
  if (logNotice) {
    showToast(logNotice);
    logNotice = nullptr;
  }
  servicePmu(now);
  touchRetry(now);
  touchDebugTick(now);
  radioDebugTick(now);
  wifiFallbackService(now);
  if (portalOn)
    web.handleClient();
  serviceHunt(now, dt);
  if (dimEnabled() && !dimmed && screen != SCR_BOOT && screen != SCR_HUNT && screen != SCR_PORTAL &&
      screen != SCR_CAL && screen != SCR_TCAL && (int32_t)(now - lastTouchMs) > (int32_t)dimAfterMs()) {
    dimmed = true;
    blTarget = imax(12, bright / 6);
  }
  if (dimEnabled() && dimmed && !screenOff && screen != SCR_HUNT && screen != SCR_PORTAL && screen != SCR_CAL &&
      screen != SCR_TCAL && (int32_t)(now - lastTouchMs) > (int32_t)sleepAfterMs()) {
    panelSleep(true);
  }  // display OFF: burn-in + battery
  // discreet mode / power saver cap the brightness
  int blGoal = imin(blTarget, brightCap());
  if (blLevel != blGoal) {
    int step = imax(1, (int)(dt * 420));
    blSet(blLevel < blGoal ? imin(blLevel + step, blGoal) : imax(blLevel - step, blGoal));
  }
  if (screen == SCR_BOOT && (now - bootAt > BOOT_MS + 200 + bootHoldMs || (tDown && now - bootAt > 300))) {
    if (tDown)
      audioStopFile();
    BOARD_BOOT_DONE
  }
  bool anim = tickTiles(dt);
  anim |= tickPager(dt);
  anim |= tickScroll(dt);
  anim |= screenAnimating();
  static uint32_t painted = 0;
  bool listVisible = (screen == SCR_MAIN && (curPage() <= 1)) || screen == SCR_SCAN;
  if (uiSerial != painted && listVisible) {
    painted = uiSerial;
    needRender = true;
  }
  if (((screen == SCR_MAIN && curPage() == 1) || screen == SCR_SCAN) && !sc.drag && now - lastList > 500) {
    lastList = now;
    needRender = true;
  }
  if (screen == SCR_MAIN && (curPage() == 2 || curPage() == 3) && now - lastList > 1000) {
    lastList = now;
    needRender = true;
  }
  if ((screen == SCR_REPORT && now - lastList > 1000) || (screen == SCR_TEST && now - lastList > 400) ||
      (screen == SCR_CAL && now - lastList > 120) || (screen == SCR_TCAL && now - lastList > 100)) {
    lastList = now;
    needRender = true;
  }
#if TOUCH_DEBUG
  {
    static uint32_t lastDbg = 0;
    if (screen == SCR_MAIN && (int32_t)(now - lastDbg) > 500) {
      lastDbg = now;
      markDirty(0, 40);
      present();
    }
  }
#endif
  if (screenOff) {
    needRender = false;
    dirtyY0 = dirtyY1 = 0;
    delay(4);
    return;
  }  // display is off: nothing to draw
  if ((needRender || anim) && now - lastFrame >= 20) {
    lastFrame = now;
    needRender = false;
#if PERF_LOG
    uint32_t t0 = micros();
#endif
    uint32_t f0 = millis();
    // custom picture is already on the panel: only the progress bar changes
    if (screen == SCR_BOOT && bootLogoMode == 1 && bootLogoDrawn)
      markDirty(H - 72, H);
    else
      render();
    present();
    dbgFrameMs = millis() - f0;
#if PERF_LOG
    Serial.printf("frame %lu us\n", (unsigned long)(micros() - t0));
#endif
  } else if (!needRender && !anim && screen != SCR_BOOT && pillLive() && now - lastPill >= 80) {
    lastPill = now;
    markDirty(0, hdrH());
    present();  // only the top strips are re-sent
  } else if (!needRender && !anim) {
    delay(2);
  }
}
void setup() {
  Serial.begin(115200);
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  Serial.printf("boot: free heap %u, largest block %u\n", (unsigned)ESP.getFreeHeap(),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, LOW);  // stay dark until the first frame is ready
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);
  pinMode(PIN_TOUCH_CS, OUTPUT);
  digitalWrite(PIN_TOUCH_CS, HIGH);
  pinMode(PIN_TFT_CS, OUTPUT);
  digitalWrite(PIN_TFT_CS, HIGH);
  pinMode(PIN_LED_R, OUTPUT);
  digitalWrite(PIN_LED_R, HIGH);
  pinMode(PIN_LED_G, OUTPUT);
  digitalWrite(PIN_LED_G, HIGH);
  pinMode(PIN_LED_B, OUTPUT);
  digitalWrite(PIN_LED_B, HIGH);
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SD_CS);
  panel->begin();
  loadPrefs();
  applyColorMode();
  panel->fillScreen(0);
  ledcAttach(PIN_TFT_BL, 5000, 10);
  blSet(0);
  // big enough for the wider (landscape) strip
  fb = (uint16_t*)heap_caps_malloc((size_t)PANEL_H * BAND_H * 2, MALLOC_CAP_8BIT);
  fbT = (uint16_t*)heap_caps_malloc((size_t)PANEL_H * BAND_H * 2, MALLOC_CAP_8BIT);
  // no RAM for the strip buffer: say so instead of crashing silently
  if (!fb || !fbT) {
    panel->fillScreen(0xF800);
    blSet(120);
    while (true)
      delay(1000);
  }
  // saved orientation (Auto starts upright until the sensor speaks)
  applyRotation(orient < 4 ? orient : 0, false);
  clipReset();
  touchWake();
  pmuInit();
  fsOk = mountFs();
  Serial.printf("free heap %u  storage %s\n", (unsigned)ESP.getFreeHeap(),
                fsOk ? (fsStore == (fs::FS*)&SD ? "SD" : "flash") : "FAIL");
  statReset();
  // ignore list + custom signatures from flash
  ignoreLoad();
  sigsLoad();
  // software clock (set from the phone); no motion sensor
  rtcInit();
  imuOk = false;
  saverOn = saverActive();
  if (fsOk)
    startNewSession();
  if (PIN_GPS_RX >= 0)
    GPS.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  audioReady = audioInit();  // optional buzzer
  syncArmed();
  bootAt = millis();
  screen = SCR_BOOT;
  renderRange(0, PH);
  for (int i = 1; i <= 14; i++) {
    blSet(bright * i / 14);
    delay(14);
  }  // fade in
  blTarget = bright;
  lastTouchMs = millis();
  sndPlay(SND_BOOT, 3);
}
