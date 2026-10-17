HUNTR/SEEKR  -  WAVESHARE ESP32-S3-TOUCH-LCD-1.47  -  PUBLIC BETA 1
===================================================================

Welcome. HUNTR/SEEKR turns your Waveshare ESP32-S3-Touch-LCD-1.47 board into a pocket radio scanner. It
listens for the Wi-Fi, Bluetooth and Drone Remote ID signals that cameras, trackers, smart glasses,
drones and similar devices give off, and shows you what it heard and how strong each signal is.

It only listens. It never transmits attacks and never joins anyone's network.
Results are hints, not proof.


WHAT YOU NEED
-------------
  * the Waveshare ESP32-S3-Touch-LCD-1.47 board
  * a USB data cable (some cables only charge: if the computer does not see the board, try another cable)
  * a computer (Windows, macOS or Linux)
  * optional: a GPS module, a lithium battery of the type Waveshare specifies, a phone


THE FASTEST WAY TO START
------------------------
  1. Flash the .bin file                  -> open help/2_FLASHING.txt
  2. Tap the group tiles on the ARM page to arm them, then watch the FINDS page
  3. Read help/1_GETTING_STARTED.txt for the pages and settings

Something not working?  -> open help/3_TROUBLESHOOTING.txt
Quick answers           -> open help/4_FAQ.txt


WHAT IS IN THIS FOLDER
----------------------
  READ_ME_FIRST.txt       this file
  HELP.txt                a one-page "I have this problem" finder
  README.md               the project page (shown on GitHub)
  help/                   plain-text help files (getting started, flashing, troubleshooting, FAQ)
  docs/                   build notes and the code map
  firmware/               how to flash and check the release .bin
  src/                    the firmware source code
  config/                 board definitions (pins, screen) and build settings
  tools/                  script that makes the release .bin from a build
  licenses/, LICENSE, THIRD_PARTY_NOTICES.md   licence information
  CHANGELOG.md, RELEASE_NOTES.md               what is in this release


THE USER GUIDE
--------------
  There is no illustrated guide for this board yet. The guide for the Waveshare ESP32-S3-Touch-LCD-1.69 (folder
  devices/s3-lcd169/docs on the project page) covers the same screens, settings and groups.


GOOD TO KNOW (THIS IS A BETA)
-----------------------------
  * This is the first public release. Expect rough edges.
  * Signal strength is a rough guide to distance. AIM can estimate a direction, but never better than +/-15
    degrees, and only if the board has a motion sensor (this one normally does not).
  * The board has one radio shared by Wi-Fi and Bluetooth. If Bluetooth finds are slow while a Wi-Fi group is
    armed, turn on SETUP > Bluetooth share.
  * The board listens on 2.4 GHz only.
  * Hardware status: This release has been flashed and run on a real Waveshare ESP32-S3-Touch-LCD-1.47 by the project. Not every feature has been tried one by one on the board yet, so some may have had less testing on hardware. Please report what you see, good or bad, with the Bug report form.

USE IT RESPONSIBLY
------------------
  Use it to find out what is around you, within your local laws. Do not use it to identify, follow or harass
  people. A match is a clue about a device, not evidence about a person or place.

Problems or ideas: use the Issues tab of the project page and include what the Serial Monitor (115200 baud) printed.
