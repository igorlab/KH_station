# KH station

Autosampling titrator that measures carbonate hardness (dKH) in a reef aquarium and
reports it to Telegram, a web dashboard and [labaqua.net](https://labaqua.net/).

![License](https://img.shields.io/badge/license-GPL3.0-green)
![Release stable](https://badgen.net/github/release/igorlab/KH_station/stable)

![All parts](Assembling/img/front1.jpg)
![All parts](Assembling/img/back1.jpg)

[![Build and demo video](https://markdown-videos.vercel.app/youtube/T8ol2PM2Kjg)](https://youtu.be/T8ol2PM2Kjg)

Questions, feedback, or just following along:
[Arduino Aquarium titrator group chat](https://t.me/+Ad4m-7L7tV1lNGNi).

---

## What it does

It performs a real acid–base titration, unattended, on a schedule.

One cycle:

1. **Home** — both syringes drive to their endstops, so position is known absolutely.
2. **Load reagent** — the reagent syringe draws 10 ml of HCl *in parallel* with the
   water work below, because that is dead time otherwise.
3. **Wash** — the reactor is rinsed with aquarium water and drained, so the previous
   sample cannot bias this one.
4. **Aliquot** — 25 ml of aquarium water is drawn and injected into the reactor.
5. **Settle** — the stirrer runs and the electrode is given time to stop moving.
6. **Dose** — HCl goes in on a ladder: 3 ml, then 1 ml, then 0.5, 0.25, 0.1, 0.05 and
   finally 0.01 ml steps as the pH approaches the endpoint. After every dose the
   firmware *waits for the reading to settle* rather than waiting a fixed time.
7. **Endpoint** — dosing stops at **pH 4.10**.
8. **Return** — both syringes go home, the reactor drains, the result is published.

KH is computed from the volume of acid consumed:

```
KH = (2804 × V_acid × c_HCl) / (V_sample / 1.026)
```

with `c_HCl` and `V_sample` configurable (0.01 M and 25 ml by default; 25 g of 1.025
density water is 24.39 ml, which is where the 1.026 comes from).

A result that differs from the previous one by more than `maxDeviation` (0.30 dKH by
default) is not accepted — the station retitrates, up to twice, and reports that it
could not converge rather than publishing a number it does not trust.

---

## Repository layout

| Path | What is in it |
|---|---|
| `firmware/` | `firmware.bin` and `bin_version.txt` — what the device downloads for OTA |
| `PCB/` | schematics (main board and pH front end) and the BOM |
| `STL/` | every printed part: syringe pump bodies, carriages, reactor holder, valve rotors, stirrer |
| `Assembling/` | build photos and assembly notes |
| `tools/kh_loader/` | one-off service sketch for stations coming from an old firmware |

---

## Hardware

- **ESP32** (DOIT DEVKIT V1 or equivalent, 4 MB flash)
- **Two syringe pumps** — reagent (10 ml) and water (25 ml), stepper driven, each with
  an optical endstop and a servo-driven rotary valve (fill / inject)
- **pH electrode** with an amplifier board feeding a plain ADC pin
- **Magnetic stirrer** (PWM)
- **Drain pump** for emptying the reactor

Pin map (`include/PIN_definition.h`):

| Function | GPIO | | Function | GPIO |
|---|---|---|---|---|
| Reagent EN / STEP / DIR | 13 / 12 / 23 | | Water EN / STEP / DIR | 27 / 26 / 19 |
| Reagent endstop / valve | 16 / 18 | | Water endstop / valve | 5 / 17 |
| pH input (ADC1_CH7) | 35 | | pH DAC loopback (bench) | 25 |
| Drain pump | 33 | | Stirrer | 32 |
| Buzzer | 4 | | | |

GPIO 34–39 are input-only on the ESP32, which is why the pH input lives on 35.

### Step rates

Three separate rates, and the split is deliberate:

| Move | Default | Why |
|---|---|---|
| Dose | 1000 Hz | A missed step here goes straight into the KH with nothing to detect it |
| Fill | 1500 Hz | Also open loop, but not in the measurement path |
| Home | 3000 Hz | Runs until an endstop fires, so it is free to be fast |

All three are settable at runtime (`setdosehz_`, `setfillhz_`, `sethomehz_`).

---

## Getting started

### 1. Provision the device

Connect over USB and open a serial terminal at **115200 baud**, then send:

```
wifipasw_YourSSID#YourPassword
ukey_<token emailed to you by labaqua.net>
bottoken_<token from @BotFather>
settlgrmid_<your numeric Telegram id>
restart
```

`settlgrmid_` matters: it is what stops anyone else driving your station. If all four
took, the bot greets you with "KH station started" after the restart.

To get the Telegram token, talk to [BotFather](https://telegram.me/botfather) and
follow the [documented steps](https://core.telegram.org/bots#botfather).

### 2. Calibrate the pumps

Volume accuracy is the largest term in the result, so do this before trusting a number.

1. Telegram → **Calibrations** → **Calibrate pumps**
2. Put the hose tip over a weighed vessel, run **Run \<pump\> pump test**
3. When it beeps three times, weigh what came out
4. If it is outside tolerance, send the real mass back: **Set real \<pump\> mass**, then
   e.g. `24.87`
5. Repeat until it lands

Tolerance: **reagent 10.00 ± 0.05 g, water 25.00 ± 0.05 g.**

### 3. Calibrate the pH electrode

The shipped calibration line is a placeholder. Two-point, using 4.01 and 6.86 buffers:

1. Rinse the electrode with deionised water, put it in the 4.01 buffer
2. Press **Start reading pH** (web) or `readph` (serial), wait for the value to settle
3. Send `calph_4.01`
4. Rinse, repeat in the 6.86 buffer, send `calph_6.86`

Do not skip step 2. Calibrating without the reading running stores a value the filter
has not converged to, and every KH afterwards is computed from a wrong line.

---

## Web dashboard

`http://<device-ip>/` — one page, no internet required, no CDN.

- **Station diagram** — both syringes with their live contents, the reactor, the tubes,
  the stirrer, and the numbers you actually watch: pH, KH and ml dosed
- **Titration curve** — pH against volume dosed, one point per step. Point at it and it
  names the nearest step; outside a run it plots pH against time for calibration
- **Console** — the machine's own log, streamed live, not a 30-second snapshot
- **Readings** — everything in NVS, including the correction index and reagent stock
- **Pumps / Settings** — collapsed by default; they are setup, not daily driving

Every button confirms before it moves anything.

> The dashboard has **no authentication**. Keep the station on a network you trust and
> do not forward its port.

---

## Command reference

Commands work over serial, Telegram and `http://<device-ip>/commands?param=<command>`.
Those marked ⚠ move hardware and are refused while a titration is running.

**Titration**

| Command | Effect |
|---|---|
| `titr_1` ⚠ | Run one titration now |
| `stoptitr` | Abort the running titration; both syringes return home |
| `washreactor` ⚠ | Rinse and drain the reactor |
| `refilreagent` ⚠ | Prime the reagent line |
| `allhome` ⚠ | Drive both syringes to their endstops |
| `lastkh`, `counttitr`, `lastlog` | Last result, cycle counters, last run's log |

**pH**

| Command | Effect |
|---|---|
| `readph` / `stopreadph` | Start / stop continuous reading |
| `calph_4.01`, `calph_6.86` | Store a calibration point |
| `phwait_<ms>_<eps>_<n>_<timeout>` | Settle detector: sample period, tolerance, consecutive samples, give-up time |
| `phfilter_<mea>_<est>_<q>` | Kalman constants — changing these changes the measured pH |
| `phsim_1` / `phsim_0` | Bench simulator instead of the electrode |
| `phloop_1` / `phloop_0` | DAC loopback (see below) |

**Settings**

| Command | Default |
|---|---|
| `maxdeviation_<dKH>` | 0.30 |
| `retitrhour_<1..24>` | 3 |
| `setwvolume_<ml>` | 25.00 |
| `setrvolume_<L>` | 5.0 (reagent stock) |
| `setdosehz_` / `setfillhz_` / `sethomehz_` | 1000 / 1500 / 3000 |
| `stirrerd_<0..255>` | 220 |
| `settings`, `getcalvalues` | Dump everything currently stored |

**System**

`restart`, `getip`, `freeheap`, `gettime`, `checkupdate`, `updatedevice`, `sendlogs`

---

## Firmware updates

The station checks `firmware/bin_version.txt` in this repository and, if it differs
from what it is running, offers the update in Telegram under **Update**. `updatedevice`
downloads `firmware/firmware.bin` over HTTPS and flashes it.

Certificates are verified against the ESP32 root CA bundle, so nothing has to be
re-pinned when GitHub rotates its issuing CA.

The partition table (`minimal_SPIFFS.csv`) gives each OTA slot **1,966,080 bytes**.
Check a new build fits before publishing it.

---

## Updating a station that has been running for years

**Read this before OTA-ing a device that is still on 2.2.x.**

Old firmware stored some of its settings under different NVS key names — the keys
carried a `state.` prefix, and `state.r_usd_vol` is exactly 15 characters, which is the
longest key name NVS accepts. The current firmware reads the short names:

| Old key | Current key |
|---|---|
| `vol_w_ml` | `w_volume` |
| `vol_r_ml` | `r_volume` |
| `state.lastkh` | `lastkh` |
| `state.r_usd_vol` | `r_usd_vol` |
| `state.countTitr` | `countTitr` |

Update straight over the air and the new firmware will not find those, and will quietly
fall back to factory defaults — your sample volume, your last result and your reagent
usage counter, gone, with nothing on screen to say so.

`tools/kh_loader/kh_loader.ino` exists for exactly this. Flash it once from the Arduino
IDE and it will:

1. **Print everything** in NVS first, so you have a written record before anything
   happens
2. **Copy** each old key to its current name
3. **Connect to Wi-Fi and install the current firmware** over the air

Pump and pH calibration are not affected — those key names never changed, so there is no
need to recalibrate.

The one rule the sketch follows, and the reason it is safe to run twice:

> **It never overwrites a value that already exists.** A write happens in exactly one
> case — the old key is present and the new one is not. If both exist, the values are
> compared, shown to you, and left alone.

It does not erase NVS, format flash, or reset anything. Set `DRY_RUN` to `1` at the top
and it writes not a single byte while still doing the full dump and comparison.

Three Arduino IDE settings are required, and the second one is the one people miss:

- **Board** → ESP32 Dev Module
- **Partition Scheme** → *Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)* — not the
  default. The firmware is ~1.6 MB and the default scheme gives only 1.31 MB per slot,
  so the download would succeed and the install would fail at the very end
- **Upload Speed** → 115200

Stations already on 2.3.x can update normally from Telegram; they are past the rename.

---

## Building from source

```bash
pio run                 # build
pio run -t upload       # flash over USB
```

The platform is pinned in `platformio.ini`:

```ini
platform = https://github.com/pioarduino/platform-espressif32.git#55.03.311
```

That is **pioarduino**, not `platformio/espressif32`. The official platform never left
Arduino core 2.0.17 — 7.1.3 still pins it — so it is the only route to core 3.x.
55.03.311 is Arduino core 3.3.11 / ESP-IDF 5.5.5 / GCC 14.2.

The tag is exact on purpose. A floating ref would move the compiler under firmware that
drives syringes from a timer ISR.

**Upload speed is 115200.** Higher rates fail here: the esptool pioarduino ships loads
its stub, switches baud, and then gets no answer to the next command.

Nothing secret belongs in this repository. Wi-Fi credentials, the Telegram token, the
chat id and the labaqua key all live in the device's NVS and are set with the commands
above.

---

## Testing without a pH board

`phloop_1` turns the ESP32's DAC1 into a stand-in electrode. Jumper **GPIO25 → GPIO35**
and the firmware synthesises the voltage it expects to read, then reads it back through
the real ADC with the simulator switched *off* — so `analogRead`, the oversampling, the
Kalman filter and the stored calibration line are all genuinely exercised.

It sweeps all 256 DAC codes once and keeps the mapping the ADC actually returned,
rather than assuming the DAC is linear against it. If the sweep reports a tiny span, the
jumper is missing and it says so.

Deliberately not saved to NVS: a bench rig that survived a reboot into production would
be worse than retyping the command.

---

## What the station refuses to do

- **Dose into an implausible sample.** If the settled starting pH is below the endpoint,
  or the ADC sits on a rail, the cycle aborts with *"pH input does not look like an
  electrode"* instead of dosing nothing and reporting KH 0.00.
- **Accept a run that dosed nothing.** Zero volume is never a valid result.
- **Publish a result it does not trust.** Outside `maxDeviation`, it retitrates; twice
  in a row without converging and it reports the failure.
- **Keep moving after a stop.** `stoptitr` kills the current move and walks both
  syringes home, so nothing is left loaded.

---

## License

GPL-3.0. See [LICENSE](LICENSE).
