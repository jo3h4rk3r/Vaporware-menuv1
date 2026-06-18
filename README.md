# Vaporware - MenuV1
<img width="3229" height="2451" alt="image" src="https://github.com/user-attachments/assets/8107a78f-6bb3-49ea-ba97-c2bc24bec41c" />
<img width="2160" height="2171" alt="vapeio" src="https://github.com/user-attachments/assets/2b7b1250-0093-4b2f-94e1-1af762395348" />

This is a fork of Vaporware that adds
multi-button support, beeper support
and a interactive menu.
Beeper uses existing vape coil pins and is disabled by default. 

I could not find the exact keypad that i used here but i found one very similar that will work: https://www.caddxfpv.com/products/caddxfpv-camera-accessories-osd-menu-board?variant=19340307759193&country=US&currency=USD&utm_medium=product_sync&utm_source=google&utm_content=sag_organic&utm_campaign=sag_organic&srsltid=AfmBOooicjKupR_XhQf7rAnX03UfVZbYXv-UX_9yr33v4fmzhtaw_Hm3AII

>THIS PROGRAMS STRUCTURE AND README HAS BEEN ALTED FROM ORIGNAL VERSION! PLEASE VISIT https://github.com/ImoverEngineering/Vaporware IF YOU WANT THE ORIGNAL PROJECT.

>CREDITS TO ImoverEngineering.
>Vaperware fork "MenuV1" made by jo3h4rk3r.

A minimal C firmware SDK for building games and apps on the **Raz DC25000** disposable vape — repurposed as a pocket game console.

The device runs a Nations Tech **N32G031K8Q7-1** (ARM Cortex-M0) driving a 128×160 GC9107 IPS display, with a single button, battery ADC, and a coil MOSFET you can optionally fire. All examples exclude the Coil MOSFET and pressure sensor.

---

## ⚠️ Disclaimer

> **This software is provided for educational and research purposes only.**
>
> By using this project you acknowledge that:
>
> - **You assume all risk.** Modifying consumer electronics — especially lithium battery-powered devices — carries real hazards including fire, explosion, electric shock, and permanent hardware damage. Proceed only if you understand what you are doing.
> - **The coil is dangerous.** The heating coil draws significant current from the LiPo cell. Custom firmware that fires the coil incorrectly (wrong duty cycle, no thermal cutoff, wrong timing) can cause the battery to overheat, vent, or catch fire. The examples in this repo do **not** fire the coil. If you choose to, you do so entirely at your own risk.
> - **Vaping carries health risks.** This project does not encourage or endorse vaping. The hardware is used purely as a convenient, cheap embedded platform.
> - **Flashing may brick your device.** Incorrect firmware can permanently damage the hardware.
> - **Exposure to chemicals.** Opening or modifying a vaping device may expose you to harmful substances.
> - **Respect local laws.** Possession and modification of vaping devices may be regulated or prohibited in your jurisdiction.
> - **No warranty. No liability.** THE AUTHOR(S) ARE NOT LIABLE FOR ANY DAMAGES, INJURIES, OR LEGAL CONSEQUENCES ARISING FROM THE USE OR MISUSE OF THIS SOFTWARE.

---

## Hardware at a Glance

| | |
|---|---|
| **MCU** | N32G031K8Q7-1, Cortex-M0 @ 8 MHz, 64 KB flash, 8 KB SRAM |
| **Display** | 128×160 IPS TFT, GC9107, RGB565, SPI @ 4 MHz |
| **Button** | PA7, active-LOW |
| **Battery** | 3.7 V LiPo, ~4.2 V full, measured via PA6 ADC (channel 6) |
| **Coil** | MOSFET gate — HIGH = fire. **Pin varies by board variant** (PB0, PB8, PA5 all observed); scan before use |
| **Debug** | SWD (PA13/PA14) via ST-Link V2 |

Full pin table and peripheral map: [`docs/README.md`](docs/README.md)

---

### Hardware

| Item | Notes |
|---|---|
| [ST-Link V2](https://www.amazon.com/s?k=st-link+v2) | SWD programmer (~$8) — keep plugged in for streaming too |
| Raz DC25000 vape | Or any device with an N32G031 + GC9107 |

### Software

**1 — Arm GNU Toolchain 14.2** (cross-compiler)

Download from [developer.arm.com](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).
Install to the default path or update the `GCC` / `OBJCOPY` / `SIZE` lines at the top of each `build_*.bat`:
```
C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\
```

**2 — WSL 2** (Windows Subsystem for Linux)

```powershell
wsl --install          # installs Ubuntu by default — reboot if prompted
```

**3 — OpenOCD in WSL** (flash and SWD debug server)

```bash
sudo apt update && sudo apt install openocd
```

**4 — usbipd-win** (share ST-Link USB with WSL)

Download the installer from [github.com/dorssel/usbipd-win/releases](https://github.com/dorssel/usbipd-win/releases).

After installing, find your ST-Link bus ID (run in PowerShell with ST-Link plugged in):
```powershell
usbipd list
```
Look for a line like `1-2   0483:3748  STMicroelectronics ST-Link`. The `build.bat` and `flash.bat` scripts hardcode `--busid 1-2` — if yours differs, edit that line in the batch file.


## Quick Start

### 1 — Wire the ST-Link

| ST-Link pin | Vape test pad |
|---|---|
| SWDIO | PA13 |
| SWCLK | PA14 |
| GND | GND |
| 3.3 V | — (vape is self-powered, leave disconnected) |

### 2 — Edit flash.bat

Edit flash.bat to match your ST-Link bus ID.

usbipd attach --wsl --busid <busID>

### 3 — Flash

Run start_flash.bat directly and it will build and flash the firmware for you.

The vape boots into the new firmware immediately after flashing.

## Library Overview

The `src/` library provides everything needed to run apps. You never call `main()` — the framework does that. Just implement two functions:

```c
void app_init(void) {
    // called once at startup — set up your initial state and draw first frame
    display_fill(COL_BLACK);
}

void app_update(uint32_t frame) {
    // called ~30 times per second — update state, redraw changed regions
    if (button_just_pressed()) { /* ... */ }
}
```

| Module | What it does |
|---|---|
| `system` | 8 MHz HSI clock, TIM3 delay, TIM1 wall clock, IWDG feed |
| `display` | GC9107 init, fill, set window, draw pixel, draw image (RGB565) |
| `button` | PA7 debounce — pressed / just_pressed / just_released / held_ms |
| `battery` | PA6 ADC read (channel 6), raw-to-voltage conversion, charge-level thresholds |
| `nv` | Write-forward NV storage in top 4 KB of flash — read / write / reset |
| `vape` | Coil safety init — drives coil gate LOW at reset to prevent accidental fire |
| `app` | `main()`, frame timer, sleep timeout, hold-to-reset, hardware init order |

Full API documentation: [`docs/README.md`](docs/README.md)

---

## Flashing Tools

`tools/` contains host-side Python scripts for tasks beyond the normal build/flash flow:

| Script | Purpose |
|---|---|
| `flash_charge.py` | Flash any `.bin` via OpenOCD telnet (edit `BIN_PATH` at top) |
| `check_voltage.py` | Read live battery voltage via SWD without flashing |
| `spi_sniff.py` | Passive SPI transaction capture via SWD memory reads |

All scripts connect to an already-running OpenOCD telnet server on port 6666.
