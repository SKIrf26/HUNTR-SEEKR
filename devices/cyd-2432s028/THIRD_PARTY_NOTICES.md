# Third-party notices

HUNTR/SEEKR itself is released under the **GNU General Public License v3.0** (see [LICENSE](LICENSE)). Copyright (C) 2026 SKIrf Labs.

It is built with, and links against, the open-source components below. Their licences are all compatible with distributing a combined GPL v3 firmware. Copies of the licence texts are in the `licenses/` folder.

| Component | Used for | Licence | Copy of licence |
|---|---|---|---|
| Arduino-ESP32 core (Espressif Systems), incl. ESP-IDF components | Runtime, Wi-Fi, Bluetooth, file system | LGPL 2.1 or later, with Apache 2.0 and other permissive parts for the underlying ESP-IDF | [licenses/LGPL-2.1.txt](licenses/LGPL-2.1.txt), [licenses/Apache-2.0.txt](licenses/Apache-2.0.txt); see the core's own repository for the full per-component list |
| TFT_eSPI (Bodmer) | Display driver and drawing | FreeBSD (BSD 2-clause); parts derived from Adafruit libraries (BSD / MIT) | [licenses/BSD-TFT_eSPI.txt](licenses/BSD-TFT_eSPI.txt) |
| NimBLE-Arduino (h2zero), built on Apache Mynewt NimBLE | Bluetooth LE scanning | Apache 2.0 | [licenses/Apache-2.0.txt](licenses/Apache-2.0.txt) |
| TinyGPSPlus (Mikal Hart) | NMEA parsing (only used if a GPS module is wired) | LGPL 2.1 or later | [licenses/LGPL-2.1.txt](licenses/LGPL-2.1.txt) |
| bluetooth-numbers-database (Nordic Semiconductor) | Bluetooth company names in the built-in lists (shortened) | BSD 3-Clause | [licenses/BSD-3-Clause-Nordic-bluetooth-numbers-database.txt](licenses/BSD-3-Clause-Nordic-bluetooth-numbers-database.txt) |
| Inter typeface (The Inter Project Authors) | Glyph shapes drawn on screen | SIL Open Font License 1.1 | [licenses/OFL-1.1-Inter.txt](licenses/OFL-1.1-Inter.txt) |
| Share Tech Mono (Carrois Type Design) | Glyph shapes drawn on screen | SIL Open Font License 1.1 | [licenses/OFL-1.1-Share-Tech-Mono.txt](licenses/OFL-1.1-Share-Tech-Mono.txt) |
| Oxanium (The Oxanium Project Authors) | Glyph shapes drawn on screen | SIL Open Font License 1.1 | [licenses/OFL-1.1-Oxanium.txt](licenses/OFL-1.1-Oxanium.txt) |

## About the font data
The firmware contains pre-rendered, anti-aliased glyph bitmaps generated from the typefaces above (SIL OFL 1.1). The bitmaps are derivatives of the font software and remain under the OFL; they are not sold on their own. The copyright lines are in the licence files in `licenses/`. They are in `src/HUNTR-SEEKR_CYD_2432S028/Fonts_Standard.h` and `Fonts_Terminal.h`.

## About the Bluetooth company names
The Bluetooth company names in the built-in lists come from the Bluetooth SIG assigned numbers, as published by Nordic Semiconductor in the bluetooth-numbers-database (BSD 3-Clause, Copyright (c) 2019 - 2020, Nordic Semiconductor ASA). The names are shortened for the display.

## About the hardware-address prefixes
Many of the maker address prefixes in the built-in lists (the first 3, 7 or 9 characters of a device's hardware address) were checked against, or taken from, the public address-block listings of the IEEE Registration Authority (MA-L, MA-M and MA-S). The firmware keeps only the prefix and a short maker tag. Which maker a block was assigned to says nothing about who owns or uses a particular device.

## Trademarks
Names such as Espressif, Arduino, AirTag, Tile, SmartTag, Wyze, Axon, Flock, and the other brand and product names that appear in the detection lists or screenshots belong to their respective owners. They are used only to describe what the scanner recognises. This project is not affiliated with, endorsed by, or sponsored by any of them.

## Binary releases
A pre-built `.bin` attached to a GitHub Release contains compiled code from the components above. The library sources are available from the projects named in the table.
