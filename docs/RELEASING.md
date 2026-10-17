# Making a release

For the maintainer. Each board has its own release, because each board has its own `SHA256SUMS.txt` and its own instructions. Do these in order for every board.

1. Build the sketch (see that board's `docs/BUILDING.md`), then run `python3 tools/make_release_bin.py <build folder> out` from the board's folder. Keep the files it writes.
2. Flash the image on a real board. Confirm the boot screen says **Public Beta 1** and the screen draws correctly. Walk the basics: the pages, Track a device, and SETUP > Self-test; check the Serial Monitor (115200) for errors.
3. Commit and push. On GitHub: **Releases > Draft a new release**.
   - Tag: `<board>-v0.1.0-beta.1` from the table below (create it on publish), target `main`
   - Title: `HUNTR/SEEKR for <board name> - Public Beta 1`
   - Description: paste the board's `RELEASE_NOTES.md`
   - Attach the files listed below
   - Tick **Set as a pre-release**

| Board | Tag | Files to attach |
|---|---|---|
| Waveshare ESP32-C6-Touch-AMOLED-2.06 | `watch-amoled206-v0.1.0-beta.1` | `HUNTR-SEEKR_Watch_Public_Beta_1.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_Watch_Public_Beta_1_User_Guide*.pdf` files |
| Waveshare ESP32-S3-Touch-LCD-1.69 | `s3-lcd169-v0.1.0-beta.1` | `HUNTR-SEEKR_S3_LCD169_Public_Beta_1.bin`, `HUNTR-SEEKR_S3_LCD169_Public_Beta_1_app-only.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_S3_LCD169_Public_Beta_1_User_Guide*.pdf` files |
| Waveshare ESP32-S3-Touch-LCD-1.47 | `s3-lcd147-v0.1.0-beta.1` | `HUNTR-SEEKR_S3_LCD147_Public_Beta_1.bin`, `HUNTR-SEEKR_S3_LCD147_Public_Beta_1_app-only.bin` and `SHA256SUMS.txt` (there is no user guide PDF for this board yet) |
| Waveshare ESP32-C6-Touch-LCD-1.47 | `c6-lcd147-v0.1.0-beta.1` | `HUNTR-SEEKR_C6_LCD147_Public_Beta_1.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_C6_LCD147_Public_Beta_1_User_Guide*.pdf` files |
| Waveshare ESP32-C6-LCD-1.47 (no touch, one button) | `c6-lcd147-button-v0.1.0-beta.1` | `HUNTR-SEEKR_C6_LCD147_BUTTON_Public_Beta_1.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_C6_LCD147_BUTTON_Public_Beta_1_User_Guide*.pdf` files |
| NM-CYD-C5 | `c5-cyd-v0.1.0-beta.1` | `HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_C5_CYD_Public_Beta_1_User_Guide*.pdf` files |
| ESP32-2432S028 (CYD 2.8) | `cyd-2432s028-v0.1.0-beta.1` | `HUNTR-SEEKR_CYD_2432S028_Public_Beta_1.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_CYD_2432S028_Public_Beta_1_User_Guide*.pdf` files |

4. Settings > General: add the description and topics, and upload `docs/images/social-preview.png` as the social preview.
5. Open each release page in a private window, download the `.bin` once and check it against `SHA256SUMS.txt`.
