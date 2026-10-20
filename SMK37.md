# FLY: LOOM-8 on the M-VAVE SMK-37 (beta)

LOOM-8 also builds for the M-VAVE SMK-37, a 37-key controller with the same chip as the FM-1. There it is called
**FLY**: the same groovebox with the SMK's pads, faders, wheels and DAW mode, its own boot and a more colourful
screen. The FM-1 build is unchanged; everything for the SMK-37 is behind `BOARD_SMK37`, in LOOM-8 (`app/smk37.c`,
`app/fly_splash.c` and a few switches), in fm1-base (its SMK-37 target) and in felucca-bsp (`boards/smk37`).

## The look

- **Boot:** about 3.6 s, drawn as the unit starts (`app/fly_splash.c`, all procedural). Glowing mycelium creeps in
  from the soil and the edges and branches across the screen. Then mushrooms pop up out of the moss, with a little
  bounce: a big fly agaric and a small one (both smiling, one blinks), a chanterelle, a porcini, an amethyst
  deceiver, and a cluster of glowing mycena. FLY grows in, red with white spots, over "it grows on you", with
  fireflies and spores drifting. To watch it without a keyboard, after a build:
  `cc -O2 -Iapp/build/gen -o fly_preview tests/fly_preview.c -lm && ./fly_preview frames/` (PPM frames, 25 a
  second).
- **Colour:** the default theme is **forest** (Setup: THEME; tape deck and blueprint are still there). It has
  night-forest panels, and the eight tracks are the mushrooms' colours: amanita red, chanterelle orange, sulphur
  yellow, moss, teal, mycena blue, amethyst and pink. Each page button has its own colour, shown on its page's tag
  and in the footer hints. Each knob has its own colour too: K5–K8's names and bars, SELECT / ALGO / PRESET in the
  footer, and the same knobs in DAW mode. The Mix page's track numbers take their track's colour.

The board layer comes from genkigenki's SMK-37 work for Felucca (genkigenki/Felucca branch `smk37`; hardware notes:
[fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37)). Those files are GPL-3.0-only, so in this tree they sit
apart from felucca-bsp's and fm1-base's MPL-2.0 files: `base/bsp/boards/smk37/`, `base/bsp/chip/fm1_flash_smk37.h`
and `base/device/smk37_io.c`.

## Models

The USB name is **FLY SMK-37** (the sample tools find it by that name too).

- **SMK-37 Pro**: the default build (identity `SMK-37 Pro_1000`; also for the Starrykey 37 Play).
- **SMK-37 Elite**: `MODEL=elite` (`SMK-37 Elite_1000`).
- **MKE-P37**: `MODEL=mke` (`MKE-P37_1000`).

One keyboard under three names. Each build carries its model's name, so M-VAVE's updater still takes that unit's own
factory package later. The installers take a package only for exactly the connected model.

## Controls

The manual still applies: the FM-1's controls are where the SMK-37 prints the same job.

| SMK-37 | FLY |
| --- | --- |
| Keys F3..G5 (the middle 27) | the FM-1's 27 keys, **with velocity** (the unit's own factory curves) |
| Keys C3..E3, G#5..C6 (the outer 10) | more notes on the track the keys play, with velocity; on a kit, its keys |
| 16 pads, on Steps and Bar (SEQ) | the bar's 16 cells, pads 1-8 the top row, 9-16 the bottom: everything the white keys do there (a tap, a hold, a later one for the length). Lit in the track's colour: a step with notes dim, the step playing bright, a step held white |
| 16 pads, every other page | drum pads: pads 1-11 are the 11 keys of a kit, with velocity (the track selected if it is a kit, else the kit selected last). Recorded while recording. Pads 12-16 dark |
| KNOB BANK | the pads' job. On Steps: drum pads there too (lit), again the cells. On any other page: opens Steps, with the pads as its cells |
| < / > | the bar before / after (the bar Steps shows and the pads write), when let go. Both together: DAW mode (below) |
| Faders 1..4 | the levels of T1-T4, or T5-T8 after FADER BANK (lit on T5-T8). A fader acts only once it moves |
| Pitch wheel | bend of the track the keys play, +-2 semitones (as a MIDI pitch bend) |
| Mod wheel | the filter of the track the keys play: its cutoff (a DYNAMIC filter: its hi cut). A pick-up: the wheel acts once it passes the value the track has, so nothing jumps; the screen says where to pick it up |
| K1 | the volume (the FM-1's MASTER) |
| K2 / K3 / K4 | SELECT / ALGORITHM / PRESETS |
| K5..K8 | KNOB 1..4 (the help cards say K5..K8) |
| FX, SCALE, CHORD, NOTE REPEAT, PARA, GLOBE | FX, SEL, ENV, LFO, EDIT, GLO |
| PATCH / BT | HOME / SAVE |
| ARP / SEQ / SEQ PLAY / SEQ REC | ARP / SEQ / PLAY / REC |
| OCTAVE- / OCTAVE+ | OCT- / OCT+ |
| OCTAVE- + OCTAVE+ held at power-on | HARDWARE CALIBRATION: teach the buttons and knobs again |

Not on the SMK-37: TRS MIDI IN (the SMK's jack is a MIDI output; its pin is a key row there). MIDI in works over USB.
No Bluetooth (LOOM-8 builds without it anyway).

## DAW mode (MIDI controller)

**< + >** (the second pressed while the first is held) switches **FLY ↔ DAW**. In DAW mode the keyboard is a USB
MIDI controller: FLY's sequencer stops and the controls leave FLY alone. MIDI IN still plays FLY (a sound
module for the DAW). The mode is kept: the SMK starts in DAW mode if it was left there. Ported from
[Snoop37](https://github.com/genkigenki/sloop-fm1/tree/snoop37) by genkigenki, by way of the Loom FM-1 port.

- **Keys:** notes on CHANNEL (default 1), the lowest key C3 = 48 at octave 0, with velocity. OCTAVE− / OCTAVE+: the
  octave (±3; both: 0).
- **K1–K8** (as printed; K1 is a knob here, not the volume, which stays where it was), **faders 1–4**, the **16 pads**,
  **13 buttons** and the **wheels** send what the map says. KNOB BANK / FADER BANK: bank A / B of the knobs / faders
  (lit on B); both together: the pads' bank. Knobs ABSOLUTE (0–127, kept per knob) or RELATIVE (64 ± the detents).
  `<` `>` send when let go. A CC TOGGLE flips once a press.
- **PATCH** tapped: **LOCAL** on / off (PATCH lit: on). On: the keys also play the track selected, NOTE pads the kit,
  the pitch wheel bends the track. Off (the default): FLY plays only what comes in.
- **PATCH held:** K5 CHANNEL, K6 CC MAP (left DEFAULT, right USER), K7 KNOBS (left ABSOLUTE, right RELATIVE),
  OCTAVE+ the map editor.
- **The map editor** (your own map, USER): touch a control (turn a knob, move a fader or a wheel, hit a pad, press a
  button), then K5 its TYPE (OFF, CC, CC TOGGLE, NOTE, MMC, PITCH BEND, as the control has them), K6 its NUMBER (the
  CC, note or MMC command), K7 its CHANNEL (MAIN, the one PATCH + K5 sets, or 1–16). OCTAVE+ the next control,
  OCTAVE− or PATCH done. Editing puts the USER map in use.
- **Screen:** DAW, CHANNEL, LOCAL, the map; the last control touched (its message and value); the banks and the
  octave; the eight knobs with their CCs.
- **Pads:** NOTE bank A blue, bank B green; CC yellow; OFF faint. Bright while held (a CC TOGGLE: while on).

The DEFAULT map uses free CCs only (no bank select, data entry, volume, pan, pedals or channel mode). A pad or button
CC is 127 while held, 0 when let go.

| Control | Bank A | Bank B | Channel |
|---|---|---|---|
| K1–K8 | CC 70–77 | CC 102–109 | MAIN |
| FADER 1–4 | CC 20–23 | CC 24–27 | MAIN |
| PAD 9–16 (bottom row) | notes 36–43 | notes 52–59 | 10 |
| PAD 1–8 (top row) | notes 44–51 | notes 60–67 | 10 |
| `<` `>` | CC 110, 111 | | MAIN |
| SEQ PLAY, SEQ REC | CC 115, 117 | | MAIN |
| SEQ, FX, PARA | CC 112, 113, 114 | | MAIN |
| ARP, NOTE REPEAT | CC 118, 119 | | MAIN |
| SCALE, CHORD, GLOBE, BT | CC 85, 86, 87, 88 | | MAIN |
| Pitch wheel | pitch bend | | MAIN |
| Mod wheel | CC 1 | | MAIN |
| Keys | notes | | MAIN |

Not here (Snoop37 has them): TRS MIDI out, the web editor's DAW map tab, RESET MAP (CC MAP DEFAULT is the map as
shipped; the USER map stays as you left it).

## Storage

The SMK-37 has less flash to write than the FM-1, around its calibration and stock data: 120 KB of store and 160 KB
of USRFLASH. This build has **two project slots** (the FM-1 has three), a **144 KB user sample bank** (the FM-1:
216 KB), and DAW mode's settings and map (8 KB). Load a bank built for the SMK:
`tools/loom_samples.py load --board=smk37 ...` (the samples page picks the board from the keyboard's port name). An
FM-1 bank is refused by the SMK and the other way round.

The split is one constant (`app/app.h NPROJ`, with `APP_STORE_MAP` and `dsp/sample.h SMP_REG*`): one slot would leave
about 192 KB of samples, three about 88 KB.

## Build

```sh
BOARD=smk37 ./build.sh                  # app/build/fly-smk37.fwsc (SMK-37 Pro)
BOARD=smk37 MODEL=elite ./build.sh      # app/build/fly-smk37-elite.fwsc
BOARD=smk37 MODEL=mke ./build.sh        # app/build/fly-smk37-mke.fwsc
BOARD=smk37 ./build.sh --host           # the simulator: app/build/host/fly-smk37_host
./build.sh                              # the FM-1 build again before tests/run_tests.sh
```

The simulator takes the SMK's controls in a script: `pad`, `tappad`, `outer`, `fader`, `bend`, `modw`, `k1`, `smkbtn`,
`smktap`, `keyvel`, and `pads` prints the pads' colours (`tests/scenarios_smk37/`; `smk37_daw.xy` checks DAW mode's
MIDI out byte by byte).

## Install, and the way back

1. **Back up the whole flash first.** The SMK-37 keeps its factory key, pad and wheel calibration in flash
   (`trimdat` at 0xC1224), unique to the unit. This firmware never writes there, but have the backup:
   fm1-to-smk37 `docs/02-update-and-recovery.md`, section 3.
2. `python3 base/tools/fm1_install.py app/build/fly-smk37.fwsc` (the package for your model).
3. The way back: M-VAVE's updater with the stock package for your model.

The first start clears the stock firmware's settings in the store area. Before FLY first writes into USRFLASH it
erases the stock firmware's marker there, so a return to stock rebuilds its factory key mapping.
