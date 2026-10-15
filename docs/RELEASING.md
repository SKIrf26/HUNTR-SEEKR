# Making a release

For the maintainer. One release covers all five boards. Do these in order.

1. For each board: build the sketch (see that board's `docs/BUILDING.md`), then run `python3 tools/make_release_bin.py <build folder> out` from the board's folder. Keep the `.bin` and `SHA256SUMS.txt` it writes.
2. Flash each image on a real board. Confirm the boot screen says **Public Beta 1** and the screen draws correctly. Walk the basics: the pages, Track a device, and SETUP > Self-test; check the Serial Monitor (115200) for errors.
3. Commit and push. On GitHub: **Releases > Draft a new release**.
   - Tag: `v0.1.0-beta.1` (create it on publish), target `main`
   - Title: `HUNTR/SEEKR - Public Beta 1`
   - Description: paste `RELEASE_NOTES.md`
   - Attach, for every board:
   - `HUNTR-SEEKR_Watch_Public_Beta_1.bin` and `_app-only.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_Watch_Public_Beta_1_User_Guide*.pdf` files
   - `HUNTR-SEEKR_S3_LCD169_Public_Beta_1.bin` and `_app-only.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_S3_LCD169_Public_Beta_1_User_Guide*.pdf` files
   - `HUNTR-SEEKR_C6_LCD147_Public_Beta_1.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_C6_LCD147_Public_Beta_1_User_Guide*.pdf` files
   - `HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_C5_CYD_Public_Beta_1_User_Guide*.pdf` files
   - `HUNTR-SEEKR_CYD_2432S028_Public_Beta_1.bin`, `SHA256SUMS.txt`, and the three `HUNTR-SEEKR_CYD_2432S028_Public_Beta_1_User_Guide*.pdf` files
   - Tick **Set as a pre-release**
4. Settings > General: add the description and topics, and upload `docs/images/social-preview.png` as the social preview.
5. Open the release page in a private window, download one `.bin` and check it against `SHA256SUMS.txt`.
