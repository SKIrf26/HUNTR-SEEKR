HUNTR/SEEKR  -  ESP32-2432S028 (CYD 2.8)  -  PUBLIC BETA 1
==========================================================

Welcome. HUNTR/SEEKR turns your ESP32-2432S028 (CYD 2.8) board into a pocket radio scanner. It
listens for the Wi-Fi, Bluetooth and Drone Remote ID signals that cameras, trackers, smart glasses,
drones and similar devices give off, and shows you what it heard and how strong each signal is.

It only listens. It never transmits attacks and never joins anyone's network.
Results are hints, not proof.


WHAT YOU NEED
-------------
  * the ESP32-2432S028 (CYD 2.8) board
  * a USB data cable (some cables only charge: if the computer does not see the board, try another cable)
  * a computer (Windows, macOS or Linux)
  * optional: a small 8 ohm speaker (for sound), a microSD card (for saved logs), a GPS module, a phone


THE FASTEST WAY TO START
------------------------
  1. Flash the .bin file                  -> open help/2_FLASHING.txt
  2. Calibrate the touch screen (the board asks you on first start: press and hold each of the four crosses)
  3. Tap the group tiles to arm them, then watch the FINDS page
  4. Read the user guide (docs folder, PDF) for every screen and setting

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
  config/                 display library set-up for this board
  tools/                  script that makes the release .bin from a build
  licenses/, LICENSE, THIRD_PARTY_NOTICES.md   licence information
  CHANGELOG.md, RELEASE_NOTES.md               what is in this release


THE USER GUIDE IN THREE PRINT FORMATS (docs folder)
----------------------------------------------------
  HUNTR-SEEKR_CYD_2432S028_Public_Beta_1_User_Guide.pdf
      normal pages, US Letter
  HUNTR-SEEKR_CYD_2432S028_Public_Beta_1_User_Guide_BOOKLET_flip-long-edge.pdf
      booklet, print double-sided, "flip on long edge"
  HUNTR-SEEKR_CYD_2432S028_Public_Beta_1_User_Guide_BOOKLET_flip-short-edge.pdf
      booklet, print double-sided, "flip on short edge"
  For a booklet: print the file at 100% (actual size) on US Letter, landscape, double-sided with the setting in
  the file name, then fold the stack in half and staple along the fold.
  (If the back of every sheet comes out upside down, use the other booklet file.)


GOOD TO KNOW (THIS IS A BETA)
-----------------------------
  * This is the first public release. Expect rough edges.
  * Signal strength is a rough guide to distance. The board has one antenna and cannot tell which direction a
    signal comes from: walk toward the strongest reading and use the warmer / colder number.
  * The board has one radio shared by Wi-Fi and Bluetooth. If Bluetooth finds are slow while a Wi-Fi group is
    armed, turn on SETUP > Bluetooth share.
  * The board listens on 2.4 GHz only.
  * Sound needs a small 8 ohm speaker plugged into the board's speaker socket. Without one the board is silent.
  * The board is updated over USB (the phone update page is not supported on this board in this release).
  * Hardware status: This release has been run on the real board by the project and works, including Wi-Fi
    scanning and Bluetooth. A Bluetooth start-up fault found on an earlier build is fixed. Not every feature has
    been tried one by one on the board yet, so some may have had less testing on hardware. Please report what you
    see, good or bad, with the Bug report form.

USE IT RESPONSIBLY
------------------
  Use it to find out what is around you, within your local laws. Do not use it to identify, follow or harass
  people. A match is a clue about a device, not evidence about a person or place.

Problems or ideas: use the Issues tab of the project page and include what the Serial Monitor (115200 baud) printed.
