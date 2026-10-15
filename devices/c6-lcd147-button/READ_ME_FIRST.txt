HUNTR/SEEKR  -  WAVESHARE ESP32-C6-LCD-1.47 (NO TOUCH), ONE-BUTTON EDITION  -  PUBLIC BETA 1
=================================================================================

Welcome. HUNTR/SEEKR turns your Waveshare ESP32-C6-LCD-1.47 board into a pocket radio scanner. It listens for
the Wi-Fi, Bluetooth, Zigbee/Thread and Drone Remote ID signals that cameras, trackers, smart glasses, drones and
similar devices give off, and shows you what it heard and how strong each signal is.

This edition is for the PLAIN board, the one with no touch panel. Everything is done with the BOOT button.
If your board is the "Touch" version (it has a touch screen), use the touch edition (folder c6-lcd147) instead.

It only listens. It never transmits attacks and never joins anyone's network.
Results are hints, not proof.


THE ONE BUTTON
--------------
  TAP   (under half a second)     NEXT     move to the next item
  HOLD  (half a second to 1.5 s)  SELECT   open / start / apply the highlighted item
  LONG  (over 1.5 seconds)        BACK     go back (on the home screen: screen off, any press wakes it)

  The three boxes along the bottom of every screen show what each gesture does right now, and the one you are
  making lights up while the button is down. Let go while the box you want is lit.
  The button is BOOT. The RESET button only restarts the chip.


WHAT YOU NEED
-------------
  * the Waveshare ESP32-C6-LCD-1.47 board (the plain one, without touch)
  * a USB data cable (some cables only charge: if the computer does not see the board, try another cable)
  * a computer (Windows, macOS or Linux)
  * optional: a small passive buzzer (for sound), up to two extra push buttons, a GPS module, a phone


THE FASTEST WAY TO START
------------------------
  1. Flash the .bin file                  -> open help/2_FLASHING.txt
  2. On the Home screen: TAP to SCENE, HOLD to open it, TAP to a scene, HOLD to arm it. It starts listening.
  3. Read the user guide (docs folder, PDF) for every screen

Something not working?  -> open help/3_TROUBLESHOOTING.txt
Quick answers           -> open help/4_FAQ.txt


WHAT IS IN THIS FOLDER
----------------------
  READ_ME_FIRST.txt       this file
  HELP.txt                a one-page "I have this problem" finder
  README.md               the project page (shown on GitHub)
  help/                   plain-text help files (getting started, flashing, troubleshooting, FAQ)
  docs/                   the illustrated user guide (PDF) in three print formats, build notes, pictures
  firmware/               how to flash and check the release .bin
  src/                    the firmware source code
  config/                 board definitions (pins, screen) and build settings
  tools/                  script that makes the release .bin from a build
  licenses/, LICENSE, THIRD_PARTY_NOTICES.md   licence information
  CHANGELOG.md, RELEASE_NOTES.md               what is in this release


THE USER GUIDE IN THREE PRINT FORMATS (docs folder)
----------------------------------------------------
  HUNTR-SEEKR_C6_LCD147_BUTTON_Public_Beta_1_User_Guide.pdf
      normal pages, US Letter
  HUNTR-SEEKR_C6_LCD147_BUTTON_Public_Beta_1_User_Guide_BOOKLET_flip-long-edge.pdf
      booklet, print double-sided, "flip on long edge"
  HUNTR-SEEKR_C6_LCD147_BUTTON_Public_Beta_1_User_Guide_BOOKLET_flip-short-edge.pdf
      booklet, print double-sided, "flip on short edge"
  For a booklet: print the file at 100% (actual size) on US Letter, landscape, double-sided with the setting in
  the file name, then fold the stack in half and staple along the fold.
  (If the back of every sheet comes out upside down, use the other booklet file.)


GOOD TO KNOW (THIS IS A BETA)
-----------------------------
  * This is the first public release of this edition. Expect rough edges.
  * Signal strength is a rough guide to distance. This edition has no direction finder (AIM).
  * No settings pages, ignore-list editing, session report or self-test: those need a touch screen.
  * Findings are saved automatically (every 5 minutes and when scanning stops). Download them from the phone page
    (More > Phone page, join the Wi-Fi shown, open http://192.168.4.1).
  * The board has one radio shared by Wi-Fi and Bluetooth, and it listens on 2.4 GHz only.
  * There is no speaker on the board. Sound needs a small passive buzzer (see the user guide, section 10).
  * The board is updated over USB (the phone update page is not supported on this board in this release).
  * Hardware status: This release has been flashed and run on a real plain ESP32-C6-LCD-1.47 by the project: it starts, shows the interface and finds Wi-Fi networks. An earlier one-button build of the same interface (version 3.6-btn) also ran on that board, including the display settings, the LED and reading BOOT as the button. Not every feature has been tried one by one on this release yet (Bluetooth, Zigbee/Thread, the phone page and the saved logs in particular).
    Please report what you see, good or bad, with the Bug report form.

USE IT RESPONSIBLY
------------------
  Use it to find out what is around you, within your local laws. Do not use it to identify, follow or harass
  people. A match is a clue about a device, not evidence about a person or place.

Problems or ideas: use the Issues tab of the project page and include what the Serial Monitor (115200 baud) printed.
