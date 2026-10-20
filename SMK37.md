# LOOM-8 on the M-VAVE SMK-37 (beta)

LOOM-8 also builds for the M-VAVE SMK-37, a 37-key controller with the same chip as the FM-1. The FM-1 build is
unchanged; everything for the SMK-37 is behind `BOARD_SMK37`, in LOOM-8 (`app/smk37.c` and a few switches), in
fm1-base (its SMK-37 target) and in felucca-bsp (`boards/smk37`).

The board layer comes from genkigenki's SMK-37 work for Felucca (genkigenki/Felucca branch `smk37`; hardware notes:
[fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37)). Those files are GPL-3.0-only, so in this tree they sit
apart from felucca-bsp's and fm1-base's MPL-2.0 files: `base/bsp/boards/smk37/`, `base/bsp/chip/fm1_flash_smk37.h`
and `base/device/smk37_io.c`.

## Models

- **SMK-37 Pro**: the default build (identity `SMK-37 Pro_1000`; also for the Starrykey 37 Play).
- **SMK-37 Elite**: `MODEL=elite` (`SMK-37 Elite_1000`).
- **MKE-P37**: `MODEL=mke` (`MKE-P37_1000`).

One keyboard under three names. Each build carries its model's name, so M-VAVE's updater still takes that unit's own
factory package later. The installers take a package only for exactly the connected model.

## Controls

The manual still applies: the FM-1's controls are where the SMK-37 prints the same job.

| SMK-37 | LOOM-8 |
| --- | --- |
| Keys F3..G5 (the middle 27) | the FM-1's 27 keys, **with velocity** (the unit's own factory curves) |
| Keys C3..E3, G#5..C6 (the outer 10) | more notes on the track the keys play, with velocity; on a kit, its keys |
| 16 pads, on Steps and Bar (SEQ) | the bar's 16 cells, pads 1-8 the top row, 9-16 the bottom: everything the white keys do there (a tap, a hold, a later one for the length). Lit in the track's colour: a step with notes dim, the step playing bright, a step held white |
| 16 pads, every other page | drum pads: pads 1-11 are the 11 keys of a kit, with velocity (the track selected if it is a kit, else the kit selected last). Recorded while recording. Pads 12-16 dark |
| Faders 1..4 | the levels of T1-T4, or T5-T8 after FADER BANK (lit on T5-T8). A fader acts only once it moves |
| Pitch wheel | bend of the track the keys play, +-2 semitones (as a MIDI pitch bend) |
| K1 | the volume (the FM-1's MASTER) |
| K2 / K3 / K4 | SELECT / ALGORITHM / PRESETS |
| K5..K8 | KNOB 1..4 (the help cards say K5..K8) |
| FX, SCALE, CHORD, NOTE REPEAT, PARA, GLOBE | FX, SEL, ENV, LFO, EDIT, GLO |
| PATCH / BT | HOME / SAVE |
| ARP / SEQ / SEQ PLAY / SEQ REC | ARP / SEQ / PLAY / REC |
| OCTAVE- / OCTAVE+ | OCT- / OCT+ |
| OCTAVE- + OCTAVE+ held at power-on | HARDWARE CALIBRATION: teach the buttons and knobs again |
| Mod wheel, KNOB BANK, < and > | no job yet |

Not on the SMK-37: TRS MIDI IN (the SMK's jack is a MIDI output; its pin is a key row there). MIDI in works over USB.
No Bluetooth (LOOM-8 builds without it anyway).

## Storage (provisional)

The SMK-37 has less flash to write than the FM-1, around its calibration and stock data: 120 KB of store and 160 KB
of USRFLASH. This build has **two project slots** (the FM-1 has three) and a **152 KB user sample bank** (the FM-1:
216 KB). Load a bank built for the SMK: `tools/loom_samples.py load --board=smk37 ...` (the samples page picks the
board from the keyboard's port name). An FM-1 bank is refused by the SMK and the other way round.

The split is one constant (`app/app.h NPROJ`, with `APP_STORE_MAP` and `dsp/sample.h SMP_REG*`): one slot leaves
about 200 KB of samples, three about 96 KB.

## Build

```sh
BOARD=smk37 ./build.sh                  # app/build/loom-smk37.fwsc (SMK-37 Pro)
BOARD=smk37 MODEL=elite ./build.sh      # app/build/loom-smk37-elite.fwsc
BOARD=smk37 MODEL=mke ./build.sh        # app/build/loom-smk37-mke.fwsc
BOARD=smk37 ./build.sh --host           # the simulator: app/build/host/loom-smk37_host
./build.sh                              # the FM-1 build again before tests/run_tests.sh
```

The simulator takes the SMK's controls in a script: `pad`, `tappad`, `outer`, `fader`, `bend`, `modw`, `smkbtn`,
`smktap`, `keyvel`, and `pads` prints the pads' colours (`tests/scenarios_smk37/`).

## Install, and the way back

1. **Back up the whole flash first.** The SMK-37 keeps its factory key, pad and wheel calibration in flash
   (`trimdat` at 0xC1224), unique to the unit. This firmware never writes there, but have the backup:
   fm1-to-smk37 `docs/02-update-and-recovery.md`, section 3.
2. `python3 base/tools/fm1_install.py app/build/loom-smk37.fwsc` (the package for your model).
3. The way back: M-VAVE's updater with the stock package for your model.

The first start clears the stock firmware's settings in the store area. Before LOOM-8 first writes into USRFLASH it
erases the stock firmware's marker there, so a return to stock rebuilds its factory key mapping.
