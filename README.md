<div align="center">

# HUNTR/SEEKR

**Counter-surveillance signal scanner for the following : 
- **NM-CYD-C5 ("cheap yellow display", ESP32-C5)**
- **Waveshare 2.06 Inch AMOLED Touch Screen Dev Board, ESP32-C6 Chip, 410×502 Resolution, Support W-F 6, BLE 5,Zig-bee, IEEE 802.15.4**
- **Waveshare ESP32-C6 1.47inch Touch Display Development Board, 172×320 Resolution, 262K Display Color, Supports Wi-Fi 6 / BLE 5, 160MHz RISC-V Processor**
- **ESP32-S3 Development Board Onboard 1.69inch Touch LCD Display 240×280**

</div>

## What it is

HUNTR/SEEKR turns the NM-CYD-C5 board (ESP32-C5, 2.8 inch touchscreen) into a pocket radio scanner. It **listens** for the wireless signals that certain kinds of devices give off, compares them with built-in lists of known makers and names, and shows you what it heard and how strong each signal is. A Track screen lets you walk toward a signal.

It is passive: it only receives. It does not jam, attack, or connect to anyone's network.

> **Results are hints, not proof.** Names and addresses can be changed or randomized, many devices are not in the lists, and a quiet device may simply be out of range. "Nothing found" never proves nothing is there.

## What it can find

# HUNTR/SEEKR: What it can find (Public Beta 1)

HUNTR/SEEKR checks every signal it hears against more than 300 built-in address, name and service rules, sorted into nine groups, plus drone Remote ID and several alert types.

> Results are hints, not proof. Names and addresses can be changed or randomised, many devices are not in the lists, and "nothing found" never proves nothing is there.

## By group

Each hit lands in one of nine groups you arm on the ARM page. A tag on the row says why it matched.

| Group | What it finds | Seen over |
| --- | --- | --- |
| ALPR | Flock, Penguin, Pigvision, Raven, Flock battery packs, Ubicquia and Genetec units | Wi-Fi, Bluetooth |
| AXON | Body cameras, docks, TASER, AB2/AB3/AB4/ABX/AF3 names | Wi-Fi, Bluetooth |
| CAMS | Wyze, Ring, Tapo, Reolink, Eufy, Nest, Arlo, Blink, Verkada, Avigilon, i-PRO, Digital Ally, WatchGuard, Coban, Motorola body cams, hidden-camera name hints | Wi-Fi, Bluetooth |
| GLASS | Ray-Ban Meta, Oakley Meta, Luxottica, Snap Spectacles, Vuzix, Epson Moverio, RayNeo | Wi-Fi, Bluetooth |
| DRONE | DJI, Autel, Skydio, Parrot, Anafi, FPV names, Remote ID (serial number and position) | Wi-Fi, Bluetooth |
| BT | Other Bluetooth devices by name or maker: police and first-responder radios, light bars, rugged laptops and scanners, routers. Type tags: beacon, wearable, light, Fast Pair, mesh, skimmer | Bluetooth |
| TAGS | AirTag and other Find My trackers, Tile, Samsung SmartTag, Google Find Hub, Chipolo, Pebblebee, TrackR, Jiobit, Cube, Nutale, Eufy SmartTrack, Orbit | Bluetooth |
| WIFI | Nearby access points, mobile routers, vehicle and rugged hardware, smart bulbs in set-up mode. Tags: probe, de-auth, twin, karma | Wi-Fi 2.4 GHz |
| ZB/TH | Zigbee, Thread and plain 802.15.4 devices on channels 11-26 | 802.15.4 |

## Wi-Fi

Wi-Fi matching uses the hardware address (123 maker prefixes, two longer ones), the network name (43 words and 28 anchored patterns) and a few channel and frame checks.

| Kind | Covers |
| --- | --- |
| Maker prefixes | Flock (29), Ring (13), DJI (8), Parrot (5), Wyze (5), Meta (4), Axon, Reolink, Eufy, Nest, Skydio, Vuzix, Luxottica, Digital Ally, WatchGuard, i-PRO, Verkada, Avigilon, plus vehicle and rugged gear: Zebra (17), Sierra Wireless (7), Peplink (4), Toughbook, Datalogic, Cradlepoint, Havis, SoundThink |
| Network names | Flock, Penguin, Pigvision, WYZE, Blink-, Arlo, AB2-/AB3-/AB4-/AXON-, Ring-, Tapo_, Reolink, eufy, Nest, Ray-Ban, DJI-, MAVIC, MINI-, AIR2, AVATA, Autel, Skydio, Parrot, ANAFI, Phantom, Inspire, FPV-, FIRSTNET, LEnet |
| Name patterns | Cradlepoint, Sierra and Pepwave router names, Axon and Motorola body-camera IDs, WatchGuard, Digital Ally, Coban, and hidden-camera names such as IPC-, IPCAM, HDWiFiCam, WiFiCam, YsxLite, MV, A9, P2P and CAM- followed by an ID |
| Smart bulbs | Bulbs in set-up mode (WiZ, LIFX, Kasa, Yeelight, Smart Life, Magic Home, LEDnet, Nanoleaf, Govee, Hue), tagged LIGHT |
| Behaviour | A device asking for a named network (PROBE), de-auth floods, open copies of protected networks and answer-everything access points: see [Alerts](#alerts) |

## Bluetooth

Bluetooth matching reads the address prefix (72 makers), the advertised name (31 names and 10 anchored patterns), the company ID (11), service IDs and a few payload markers.

| Kind | Covers |
| --- | --- |
| Trackers | Apple Find My (offline-finding frames, not ordinary iPhone chatter), Tile (service FEED/FEEC and name), Samsung SmartTag (FD5A) and Galaxy SmartTag, Google Find Hub (FEAA), Chipolo (FE65), Pebblebee, TrackR, Jiobit, Cube, Nutale, Eufy SmartTrack, Orbit |
| Body cams and ALPR | Axon (name, TASER, BWCDEVICE marker, service FC81, company ID), Flock and Raven (services 3100-3500, company ID), FS Ext Battery, Penguin |
| Glasses | Meta, Luxottica, Snap, Vuzix, RayNeo company IDs; Ray-Ban, Oakley Meta, Wayfarer, Spectacles, Moverio names; service FD5F |
| Drones | DJI names and service FFE0, Bluetooth Remote ID (service FFFA) with serial number and position |
| Radios and lights | Motorola (APX), L3Harris (XL200), Tait, Hytera, Feniex, SoundOff, Whelen; rugged scanners and laptops; Cradlepoint, Sierra and Peplink routers |
| Type tags | IBEACON, EDDYSTONE, ALTBEACON (beacons); WEARABLE (watches, bands, rings from Fitbit, Garmin, Amazfit, Suunto, Coros, Whoop, Oura, Polar and more); LIGHT (Govee, Hue, Nanoleaf, LIFX, LED strips and bulbs); FASTPAIR (Google Fast Pair accessories); MESH (Bluetooth Mesh nodes); SKIMMER |
| Company names | The maker's name from the Bluetooth company ID is shown on the row when it is known |

## Alerts

These are behaviours rather than device lists. The tracker, skimmer and rogue Wi-Fi alerts each have a switch under SETUP.

| Alert | Tag | Triggers when |
| --- | --- | --- |
| Tracker following you | FOLLOWING | A tracker has been heard continuously for the set number of minutes (10 by default) |
| Card skimmer | SKIMMER | A Bluetooth module still shows a factory name: HC-03, HC-05, HC-06, HC-08, Linvor or RNBT-xxxx |
| De-auth attack | DEAUTH | One transmitter sends 15 or more de-authentication frames within a second |
| Evil twin | TWIN | A network name is heard open on one access point and protected on another from a different maker |
| Karma | KARMA | One transmitter answers probes for 5 or more different network names within 30 s |
| Probe | PROBE | A device is asking for a named network |
| Drone Remote ID | RID | A drone broadcasts its serial number and position, read and shown on the row |

## Your own rules

Add up to 128 rules of your own: a Wi-Fi or Bluetooth address prefix (`oui`), a Wi-Fi name (`ssid`), a Bluetooth name (`ble`), a company ID (`cid`) or a service ID (`uuid`), each assigned to a group and a confidence. Edit them on the phone page (`sigs.txt`), or drop a `sigs_update.txt` file on the storage and it is added after yours.

```
# kind   pattern      group   [confidence]
oui      B4:1E:52     ALPR    high
ssid     MyCamNet     CAMS
```

## Board differences

| Board | Wi-Fi bands | 802.15.4 (ZB/TH group) |
| --- | --- | --- |
| C5 CYD | 2.4 GHz and 5 GHz | Yes |
| S3 1.69 | 2.4 GHz | No, so eight groups |
| C6 1.47 | 2.4 GHz | Yes |
| Watch (C6 AMOLED 2.06) | 2.4 GHz | Yes |

## Install (replace your device in place of the CYD if one of the others)

You need an **NM-CYD-C5** board and a USB-C **data** cable. Not sure where to start? Read [READ_ME_FIRST.txt](READ_ME_FIRST.txt) and the [help folder](help/).

### Flash online

Flash the .bin at address 0x0 using any popular online flasher (ESP Terminator, SpaceHuhn, etc)

### Flash the firmware file

1. Download `HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin` and `SHA256SUMS.txt` from the [Releases page](../../releases).
2. Install Python 3, then `pip install esptool`.
3. Plug in the board and run:

   ```
   esptool --chip esp32c5 --port YOUR_PORT --baud 460800 write-flash 0x0 HUNTR-SEEKR_C5_CYD_Public_Beta_1.bin
   ```
4. The board restarts. The boot screen shows **Public Beta 1** at the bottom. The first start asks you to calibrate the touch screen: press and hold each of the four crosses.

Writing the image resets saved settings; logs on the microSD card stay. Step-by-step help, including checking the download, is in [help/2_FLASHING.txt](help/2_FLASHING.txt).

## Documentation

The illustrated user guide covers set-up, every screen, the settings, troubleshooting and care:

- [User guide (PDF)](docs/HUNTR-SEEKR_C5_CYD_Public_Beta_1_User_Guide.pdf)
- Booklet versions for printing two-sided: [flip on long edge](docs/HUNTR-SEEKR_C5_CYD_Public_Beta_1_User_Guide_BOOKLET_flip-long-edge.pdf) / [flip on short edge](docs/HUNTR-SEEKR_C5_CYD_Public_Beta_1_User_Guide_BOOKLET_flip-short-edge.pdf)
- Plain-text help: [help/](help/) (getting started, flashing, troubleshooting, FAQ)

## Beta status - what to expect

This is the **first public release**. It is a beta: expect rough edges, and expect some behaviour to change.

- Signal strength is only a rough guide to distance, and one antenna cannot tell which direction a signal comes from. Use the warmer / colder reading and walk around.
- Sound needs a small **passive** buzzer or piezo fitted to the board (see the user guide).
- Update over USB: the phone update page is not supported on this board in this release.
- The detection lists are not exhaustive.

Found a problem? Open an issue with the **Bug report** form and include what the Serial Monitor (115200 baud) printed.

## Responsible use

This project is meant for defensive, privacy-protecting use: finding out what is around you. Receiving radio signals that are openly broadcast is lawful in many places, but laws differ, and some places restrict particular uses of scanners or of the information you collect. You are responsible for following the laws where you are. Do not use it to identify, follow or harass individual people. A match is a clue about a device, not evidence about a person or a place.

## Licence

HUNTR/SEEKR is free software released under the **GNU General Public License v3.0** - see [LICENSE](LICENSE). It comes with **no warranty**.

It is built with third-party components under their own licences (LGPL, BSD, MIT, SIL OFL): see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

RockBase, NMTech, Espressif, Arduino and all brand and product names that appear in the detection lists or screenshots belong to their owners. This project is not affiliated with, or endorsed by, any of them.

## Project files

| Path | Contents |
|---|---|
| `help/` | Plain-text help: getting started, flashing, troubleshooting, FAQ |
| `firmware/` | How the release `.bin` is flashed and checked |
| `docs/` | User guide PDFs, images, optional browser installer page |
| `licenses/` | Licence texts of the third-party components |
| `CHANGELOG.md`, `RELEASE_NOTES.md` | What is in each release |
