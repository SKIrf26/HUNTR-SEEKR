# Third-party notices

HUNTR/SEEKR itself is released under the **GNU General Public License v3.0** (see [LICENSE](LICENSE)). Copyright (C) 2026 SKIrf Labs.

Each board is built with a slightly different set of third-party libraries, so the full notice for each board - what is used, under which licence, and the licence text - sits in that board's folder:

- [Waveshare ESP32-C6-Touch-AMOLED-2.06](devices/watch-amoled206/THIRD_PARTY_NOTICES.md): licence texts in [devices/watch-amoled206/licenses/](devices/watch-amoled206/licenses/)
- [Waveshare ESP32-S3-Touch-LCD-1.69](devices/s3-lcd169/THIRD_PARTY_NOTICES.md): licence texts in [devices/s3-lcd169/licenses/](devices/s3-lcd169/licenses/)
- [Waveshare ESP32-C6-Touch-LCD-1.47](devices/c6-lcd147/THIRD_PARTY_NOTICES.md): licence texts in [devices/c6-lcd147/licenses/](devices/c6-lcd147/licenses/)
- [NM-CYD-C5](devices/c5-cyd/THIRD_PARTY_NOTICES.md): licence texts in [devices/c5-cyd/licenses/](devices/c5-cyd/licenses/)
- [ESP32-2432S028 (CYD 2.8)](devices/cyd-2432s028/THIRD_PARTY_NOTICES.md): licence texts in [devices/cyd-2432s028/licenses/](devices/cyd-2432s028/licenses/)

In summary, the projects use:

- Arduino-ESP32 core and ESP-IDF (LGPL 2.1 and Apache 2.0)
- GFX Library for Arduino (BSD) on the Waveshare boards; TFT_eSPI (BSD) on the two "cheap yellow display" boards
- TinyGPSPlus (LGPL 2.1); XPT2046_Touchscreen (MIT) on the NM-CYD-C5; NimBLE-Arduino (Apache 2.0) on the ESP32-2432S028
- Bluetooth company names from Nordic Semiconductor's bluetooth-numbers-database (BSD 3-clause)
- The Inter, Share Tech Mono and Oxanium fonts (SIL Open Font Licence 1.1), converted to glyph data in `Fonts_Standard.h` and `Fonts_Terminal.h`
