# NuPhy Air60 V2 custom firmware

Custom QMK firmware for the **NuPhy Air60 V2 (ANSI)** that adds two things the stock firmware does not have:

- 🔋 **Battery level over the 2.4G dongle.** The keyboard reports its battery percentage and charging state to the computer, so a desktop indicator can show it. Stock firmware only shows it on the side LEDs.
- 🇪🇸 **`Fn` + `N` types `ñ`** (`Ñ` with `Shift` or `Caps Lock`) on Linux with a US layout.

Everything else (typing, lighting, Bluetooth, sleep, VIA) behaves like the stock firmware. A ready-to-flash stock build is included so you can always go back.

> Pairs with [ubuntu-battery-tray](https://github.com/padronjosef/ubuntu-battery-tray), which shows the battery of this keyboard, a Logitech mouse and a HyperX headset in the GNOME top bar.

---

## Contents

| Path | What it is |
|---|---|
| `firmware/nuphy_air60_v2_ansi_via-custom.bin` | Custom firmware, ready to flash |
| `firmware/nuphy_air60_v2_ansi_via-stock.bin` | Unmodified firmware built from the same source, to restore |
| `firmware/SHA256SUMS` | Checksums of both binaries |
| `air60_v2.patch` | The whole change, as a patch over NuPhy's QMK tree |
| `src/` | The new source files, for easy reading |
| `build.sh` | Rebuilds either binary from source with Docker |
| `tools/nuphy_battery_listen.py` | Prints the battery reports the keyboard sends (for testing) |

Source base: [`nuphy-src/qmk_firmware`](https://github.com/nuphy-src/qmk_firmware), branch `nuphy-keyboards`, commit `f1856912` (2025-10-13).

---

## ⚡ Flash it (step by step)

You need a Linux computer and the USB-C cable. It takes about a minute.

**1. Install `dfu-util`**

```bash
sudo apt install dfu-util
```

**2. Download this repo**

```bash
git clone https://github.com/padronjosef/nuphy-air60-v2-firmware.git
cd nuphy-air60-v2-firmware
sha256sum -c firmware/SHA256SUMS
```

**3. Put the keyboard in flashing mode (DFU)**

1. Unplug the keyboard.
2. Hold `Esc`.
3. Plug in the USB-C cable while still holding `Esc`, then release it. The lights stay off.
4. Check that it is in DFU mode. This must print a line with `0483:df11`:

   ```bash
   lsusb | grep 0483:df11
   ```

**4. Flash**

```bash
sudo dfu-util -d 0483:DF11 -a 0 -s 0x08000000:leave -D firmware/nuphy_air60_v2_ansi_via-custom.bin
```

Wait for `File downloaded successfully`. Do not unplug the cable before that. A `Device's firmware is corrupt` line at the start is normal and clears itself on the next line.

**5. Use it**

Unplug the cable, set the switch to **2.4G** and type normally.

### ↩️ Go back to stock

Same steps, with the stock file in step 4:

```bash
sudo dfu-util -d 0483:DF11 -a 0 -s 0x08000000:leave -D firmware/nuphy_air60_v2_ansi_via-stock.bin
```

You can also flash NuPhy's official release (v2.1.5) from [nuphy.com/pages/qmk-firmwares](https://nuphy.com/pages/qmk-firmwares) the same way. DFU mode is entered before the firmware runs, so you can always reflash.

---

## 🔋 Battery report

### Check that it works

```bash
python3 tools/nuphy_battery_listen.py
```

Then press `Fn` + `\` (the battery key). You should see:

```
listening on /dev/hidraw5
2026-10-27T20:58:14-0500 battery=100% discharging
```

### When the keyboard sends it

- 3 seconds after the 2.4G link connects or reconnects.
- 1 second after the charger is plugged in or unplugged.
- When the level changes, at most once a minute.
- When you press `Fn` + `\`, at most once every 2 seconds.

It never wakes the keyboard to send, and it does not reset the idle timer, so sleep and battery life are unchanged. If the keyboard is asleep, the next update goes out when it wakes. Only 2.4G mode is affected; USB and Bluetooth are stock.

### How it works

The stock 2.4G link never carries the battery level: the wireless module knows it but only passes it to the keyboard chip for the side LEDs. This firmware sends it inside a media key report (report ID 3 on the dongle) with a value no real key uses:

| 16-bit value | Meaning |
|---|---|
| `0x0300` + percent | discharging |
| `0x0400` + percent | charger connected |

The value is above the highest media key code the dongle declares (`0x2A0`), so the operating system ignores it as a key press, while programs reading the raw HID device (`/dev/hidrawN`) still receive it. Each report is followed by a release (`0x0000`).

While charging, the wireless module reports a fixed 100%, so the real level is only known again after unplugging.

---

## 🇪🇸 `Fn` + `N` for `ñ`

| Keys | Types |
|---|---|
| `Fn` + `N` | `ñ` |
| `Shift` + `Fn` + `N` or `Caps Lock` on | `Ñ` |

A keyboard can only send key positions, never letters, and the US layout has no `ñ`. So the keyboard types the Linux Unicode input sequence (`Ctrl` + `Shift` + `U`, `f1`, `Space`) and IBus, GNOME's input system, turns it into the letter. The digits and the closing space go out in a single report, so the intermediate `u00f1` is never drawn on screen. Normal typing keeps its stock timing.

**Limits:** it needs IBus (default on Ubuntu/GNOME). Apps that do not use an input method, Windows and Android get the raw sequence instead of `ñ`.

---

## 🛠️ Build from source

Needs `git` and Docker. No ARM toolchain on your machine.

```bash
./build.sh           # custom firmware  -> firmware/nuphy_air60_v2_ansi_via-custom.bin
./build.sh --stock   # stock firmware   -> firmware/nuphy_air60_v2_ansi_via-stock.bin
```

The script clones NuPhy's QMK tree into `.qmk/`, pins the commit above, applies `air60_v2.patch` and builds inside the official `ghcr.io/qmk/qmk_cli` image. Binaries embed the build date, so your checksum will differ from the one in `SHA256SUMS`.

Both features can be turned off at build time in `keyboards/nuphy/air60_v2/ansi/rules.mk`: `BATTERY_REPORT_ENABLE = no`, `ENYE_ENABLE = no`.

---

## ⚠️ Notes

- Only for the **Air60 V2 ANSI**. Do not flash it to an Air75 V2, Air96 V2, Halo or any other model.
- NuPhy's public QMK tree is not byte-identical to their official v2.1.5 release, which may include extra 2.4G and Bluetooth fixes. It has worked fine here, but keep the stock binary at hand.
- VIA keeps your keymap in the keyboard's memory and only resets it to the compiled one when the build date changes.

## License

GPL-2.0, like QMK. See [LICENSE](LICENSE).
