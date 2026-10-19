HUNTR/SEEKR  -  Waveshare ESP32-Touch-LCD-3.5  -  PORT TEST 1
===========================================================

THIS IS A PORT THAT HAS NOT YET BEEN TRIED ON THE REAL BOARD. It is the current HUNTR/SEEKR firmware
adapted to this board and it compiles, but no one has run it on the hardware yet. Expect to find things that
need adjusting (touch direction, screen corners, sound). Please report what you see, good or bad.

HUNTR/SEEKR turns your Waveshare ESP32-Touch-LCD-3.5 (320 x 480 rounded-corner touch screen) into a pocket radio scanner. It
listens for the Wi-Fi, Bluetooth and Drone Remote ID signals that cameras, trackers, smart glasses, drones and
similar devices give off, and shows you what it heard and how strong each signal is.

It only listens. It never transmits attacks and never joins anyone's network.
Results are hints, not proof.


WHAT YOU NEED
-------------
  * the Waveshare ESP32-Touch-LCD-3.5 board
  * a USB-C data cable (some cables only charge: if the computer does not see the board, try another cable)
  * a computer (Windows, macOS or Linux)
  * optional: a TF (microSD) card for saved logs, a GPS module, a phone


THE FASTEST WAY TO START
------------------------
  1. Flash the .bin file                  -> open help/2_FLASHING.txt
  2. Tap the group tiles to arm them, then watch the FINDS page
  3. Open SETUP > Self-test: every dot should be green (Motion says "no sensor on this board")

Something not working?  -> open help/3_TROUBLESHOOTING.txt
Quick answers           -> open help/4_FAQ.txt


WHAT IS IN THIS FOLDER
----------------------
  READ_ME_FIRST.txt       this file
  HELP.txt                a one-page "I have this problem" finder
  README.md               the project page (shown on GitHub)
  help/                   plain-text help files (getting started, flashing, troubleshooting, FAQ)
  docs/                   build notes
  firmware/               how to flash and check the release .bin
  src/                    the firmware source code
  config/                 pin map and the few settings you may want to change
  tools/                  script that makes the release .bin from a build
  licenses/, LICENSE, THIRD_PARTY_NOTICES.md   licence information
  CHANGELOG.md, RELEASE_NOTES.md               what is in this release


GOOD TO KNOW
------------
  * The board listens on 2.4 GHz only (the chip has no 5 GHz radio, and no 802.15.4 radio, so there is no
    Zigbee/Thread group on this board: eight groups).
  * There is no motion sensor, so the AIM direction feature of the other boards is not available.
  * The board has one radio shared by Wi-Fi and Bluetooth. If Bluetooth finds are slow while a Wi-Fi group is
    armed, turn on SETUP > Bluetooth share.
  * The board's 2 MB of extra memory (PSRAM) is used for the large tables. A build made without it still starts,
    but may not have enough memory left for Bluetooth.
  * Sound plays through the board's built-in speaker.
  * The 16 MB image supports the phone update page; the 4 MB image is updated over USB.

USE IT RESPONSIBLY
------------------
  Use it to find out what is around you, within your local laws. Do not use it to identify, follow or harass
  people. A match is a clue about a device, not evidence about a person or place.

Problems or ideas: use the Issues tab of the project page and include what the Serial Monitor (115200 baud) printed.
