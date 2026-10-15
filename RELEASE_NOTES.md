# HUNTR/SEEKR - Public Beta 1

First public release, for five boards. Pick the files that match your board; every file name starts with the board name.

- **Waveshare ESP32-C6-Touch-AMOLED-2.06**: `HUNTR-SEEKR_Watch_Public_Beta_1.bin`, user guide in three print formats
- **Waveshare ESP32-S3-Touch-LCD-1.69**: `HUNTR-SEEKR_S3_LCD169_Public_Beta_1.bin`, user guide in three print formats (builds; not yet run on the board)
- **Waveshare ESP32-C6-Touch-LCD-1.47**: `HUNTR-SEEKR_C6_LCD147_Public_Beta_1.bin`, user guide in three print formats (builds; not yet run on the board)
- **NM-CYD-C5**: `HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin`, user guide in three print formats
- **ESP32-2432S028 (CYD 2.8)**: `HUNTR-SEEKR_CYD_2432S028_Public_Beta_1.bin`, user guide in three print formats (builds; not yet run on the board)

Also attached: `SHA256SUMS.txt` for each board. Check the `.bin` against it before flashing.

## Before you flash
- This is a **beta**. Results are hints, not proof.
- The Watch and NM-CYD-C5 builds have been run on real hardware. The S3 LCD 1.69, C6 LCD 1.47 and ESP32-2432S028 builds compile cleanly and their screens were checked in a simulator, but they have not yet been run on the real boards by the project. Please report what you see.
- Flashing steps and the flash address are in each board's folder: `devices/<board>/help/2_FLASHING.txt`.

Please report problems with the **Bug report** form on the Issues tab, with the Serial Monitor output (115200 baud) if you can.
