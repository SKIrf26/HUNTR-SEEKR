# HUNTR-SEEKR
Multi-device rf signal finder &amp; foxhunter. HUNT the device and get the info, SEEK the physical device itself.
TEST BUILD - HUNTR/SEEKR Watch Public Beta 1
Built in a sandbox from the Public Beta 1 sketch with Arduino-ESP32 3.3.9, GFX Library 1.6.9, TinyGPSPlus 1.1.0,
board ESP32C6 Dev Module, 16MB, partition 16M Flash (3MB APP/9.9MB FATFS), USB CDC on boot enabled.
It COMPILES and the image layout is checked, but it has NOT been run on a watch.

1. Flash:  esptool --chip esp32c6 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_Watch_Public_Beta_1.bin
2. Test it on the watch (see firmware/RELEASE_CHECKLIST.md in the repo).
3. If it is good, upload THIS SAME FILE (and SHA256SUMS.txt) to the GitHub Release, so what people download is what you tested.
HUNTR-SEEKR_Watch_Public_Beta_1_app-only.bin is for the phone update page (192.168.4.1/update).
