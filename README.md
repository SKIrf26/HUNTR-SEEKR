HUNTR/SEEKR  C5 CYD (NM-CYD-C5)  PUBLIC BETA 1
==============================================
Folder HUNTR-SEEKR_C5_CYD_Public_Beta_1 : the sketch (.ino) + SkiFonts.h + SkiFontsAlien.h. Keep all three together,
                                          in a folder with the same name as the .ino.
                                          
Folder C5_TFT_eSPI_setup  : display-library setup (read its README.txt) + User_Setup.h.

HUNTR-SEEKR_C5_CYD_Public_Beta_1_Quick_Guide.pdf : what is new, with screenshots.

Install: Arduino IDE, board "ESP32C5 Dev Module", USB CDC On Boot = Enabled, Flash 8 MB, partition "8M with spiffs".

Libraries: TFT_eSPI with C5 support (see C5_TFT_eSPI_setup), XPT2046_Touchscreen, TinyGPSPlus. Serial Monitor 115200.
