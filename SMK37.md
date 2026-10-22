# SPIDER: LOOM-8 on the M-VAVE SMK-37 (beta)

LOOM-8 also builds for the M-VAVE SMK-37, a 37-key controller with the same chip as the FM-1. There it is called
**SPIDER**: the same groovebox with the SMK's pads, faders, wheels and DAW mode, a spider's web of a look and its own
boot. The FM-1 build is unchanged; everything for the SMK-37 is behind `BOARD_SMK37`, in LOOM-8 (`app/smk37.c`,
`app/spider_splash.c` and a few switches), in fm1-base (its SMK-37 target) and in felucca-bsp (`boards/smk37`).

## The look

- **Boot:** about 3.6 s (`app/spider_splash.c`, all procedural). A night: a moon behind dead branches, fog
  drifting. A spider drops on its line to the middle, turns and walks out as the moorings, the frame and the spokes
  shoot out; then it walks the capture spiral from the outside in, as orb weavers do, dew glinting on what it has
  laid, and goes back to the hub. The name is woven in row by row, the shuttle crossing back and forth, and at the
  foot, small, the repositories SPIDER is woven from. The pads light along: the line down the middle, a flash at
  each spoke, a light running round with the spider, a red pulse, then the UI's own pad colours woven in (the
  eight inks on the mutes, LOOM-8's eight threads; the drums in the kit's). Under it a deep note, spidery: a cluster that skitters, glides apart into a
  wide D minor as it swells, the web ringing as it is strung (a pluck at each spoke), dying away under the UI; at
  the volume's middle (where the SMK starts) it peaks about -21 dBFS. A key, pad or button ends it at once. As
  Snoop37's boot: the first still at power-on, the rest once audio and input are up (fm1-base `APP_SPLASH_LATE`);
  after a failed boot, the last picture only, no sound. To watch and hear it without a keyboard, after a build:
  `cc -O2 -Iapp/build/gen -o spider_preview tests/spider_preview.c -lm && ./spider_preview frames/ 40 4096` (PPM
  frames, 25 a second, with the pads under the screen, and `boot.wav` at the volume given).
- **Colour:** the default theme is **web** (Setup: THEME; tape deck and blueprint are still there): near-black and
  violet panels, bone text, blood red for recording, and the five tracks and the throws and returns in jewel tones (crimson, ember, amber,
  absinthe, verdigris, cold blue, amethyst, orchid). Each page button has its own colour, shown on its page's tag
  and in the footer hints. Each knob has its own colour too: K5–K8's names and bars, SELECT / ALGO / PRESET in the
  footer, and the same knobs in DAW mode. The Mix page's track numbers take their track's colour.

The board layer comes from genkigenki's SMK-37 work for Felucca (genkigenki/Felucca branch `smk37`; hardware notes:
[fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37)). Those files are GPL-3.0-only, so in this tree they sit
apart from felucca-bsp's and fm1-base's MPL-2.0 files: `base/bsp/boards/smk37/`, `base/bsp/chip/fm1_flash_smk37.h`
and `base/device/smk37_io.c`.

## Models

The USB name is **SPIDER SMK-37** (the sample tools find it by that name too).

- **SMK-37 Pro**: the default build (identity `SMK-37 Pro_1000`; also for the Starrykey 37 Play).
- **SMK-37 Elite**: `MODEL=elite` (`SMK-37 Elite_1000`).
- **MKE-P37**: `MODEL=mke` (`MKE-P37_1000`).

One keyboard under three names. Each build carries its model's name, so M-VAVE's updater still takes that unit's own
factory package later. The installers take a package only for exactly the connected model.

## Controls

The manual still applies: the FM-1's controls are where the SMK-37 prints the same job.

| SMK-37 | SPIDER |
| --- | --- |
| Keys F3..G5 (the middle 27) | the FM-1's 27 keys, **with velocity** (the unit's own factory curves) |
| Keys C3..E3, G#5..C6 (the outer 10) | more notes on the track the keys play, with velocity; on a kit, its keys |
| 16 pads, on Steps and Bar (SEQ) | the bar's 16 cells, pads 1-8 the top row, 9-16 the bottom: everything the white keys do there (a tap, a hold, a later one for the length). Lit in the track's colour: a step with notes dim, the step playing bright, a step held white |
| Pads 1-8 (top row), every other page | the mutes: pads 1-5 T1-T5; pad 6 the throws (held, let go after ~0.4 s: the conductor); pad 7 FX I's return (the delay), pad 8 FX II's (the reverb), kept in the scene. Lit in the ink while it plays, dark when muted (pad 6 red while the conductor is muted) |
| Pads 9-16 (bottom row), every other page | the drums: the kit's first eight sounds (in a new kit: kick, snare, hat, open hat, clap, low tom, high tom, conga), whatever track is selected, with velocity. The kit: the track selected if it is one, else the kit selected last, else the first kit. Recorded into the kit while recording (REC + PLAY, as the keys: quantised, overdub or replace). Dim in the kit's ink, bright while hit |
| NOTE REPEAT tapped | note repeat on / off (lit): a drum pad held sounds again in time, on the grid while playing (the first repeat at the grid line after the hit), from the hit when stopped; recorded while recording, for fills |
| NOTE REPEAT held + pads 1-8 | its rate: 1/4, 1/8, 1/8T, 1/16 (the default), 1/16T, 1/32, 1/32T, 1/64 (lit while held, the one in use bright); a pick turns repeat on |
| NOTE REPEAT held, let go alone | the LFO page (the FM-1's LFO button, where the SMK prints NOTE REPEAT) |
| > held, then the pads | the throws: pads 1-12 fire the 12 momentary effects (mute, stutter, repeat, echo, build, floor, jumble, warble, pan, octave, chop, ring) on the whole mix, while held, on every page (Steps too: no step is touched). Dim violet, bright while firing; > lights |
| KNOB BANK | the pads' job. On Steps: live pads there too, mutes and drums (lit; REC + PLAY records the drums you play), again the cells. On any other page: opens Steps, with the pads as its cells |
| < / > | the bar before / after (the bar Steps shows and the pads write), when let go; > not after it fired a throw. Both together: DAW mode (below) |
| Faders 1..4 | the levels of T1-T4; after FADER BANK (lit) T5, FX I's and FX II's returns, the master. A fader acts only once it moves |
| Pitch wheel | bend of the track the keys play, +-2 semitones (as a MIDI pitch bend) |
| Mod wheel | the filter of the track the keys play: its cutoff (a DYNAMIC filter: its hi cut). A pick-up: the wheel acts once it passes the value the track has, so nothing jumps; the screen says where to pick it up |
| K1 | the volume (the FM-1's MASTER) |
| K2 / K3 / K4 | SELECT / ALGORITHM / PRESETS |
| K5..K8 | KNOB 1..4 (the help cards say K5..K8) |
| FX, SCALE, CHORD, PARA, GLOBE | FX, SEL, ENV, EDIT, GLO (NOTE REPEAT: above) |
| PATCH / BT | HOME / SAVE |
| ARP / SEQ / SEQ PLAY / SEQ REC | ARP / SEQ / PLAY / REC |
| OCTAVE- / OCTAVE+ | OCT- / OCT+ |
| OCTAVE- + OCTAVE+ held at power-on | HARDWARE CALIBRATION: teach the buttons and knobs again |

Not on the SMK-37: TRS MIDI IN (the SMK's jack is a MIDI output; its pin is a key row there). MIDI in works over USB.
No Bluetooth (LOOM-8 builds without it anyway).

## Sounds

SPIDER is a poly, a mono and an acid box with drums and breaks. It has **five tracks** (the FM-1's LOOM-8 eight): a new
project's T1 is the kit, T2 **poly**, T3 **mono**, T4 **acid**, T5 the samples (slice: a break; or the sample engine);
then the conductor, the throws, FX I and FX II as on the FM-1. Any of T1-T5 can be any engine (ENGINE, SEL's list).
SPIDER's projects are its own format: an FM-1 project does not load.

- **Three engines in place of LOOM-8's six** (`app/dsp/synth_spider.c`, which `synth.c` takes on the SMK-37; knit, rayon,
  jacquard, bellows, cane and voile, their tables and the Braids lines, stay out of SPIDER's image):
  - **poly**, after the Juno-106: a voice's one oscillator, its mixer on the knobs: **saw**, **pulse**, **width** (50 %
    .. 5 %: the LFO on it is PWM), **sub** (a square an octave down); the second page (Sub): **noise**, **hpf** (the
    Juno's: boost, flat, 240 Hz, 720 Hz), **chorus** (off, I, II, I+II: after the filter, two taps a voice swinging
    apart), **drift** (the pitch wandering a few cents). Then the Filter page's 24 dB filter, its envelopes, as LOOM-8.
  - **mono**, after the SH-101: the same oscillator, one voice and a glide (mono / legato); the second page: **sub
    type** (-1 oct, -2 oct, a 25 % pulse -2 oct), **noise**, **pw env** (the filter envelope narrowing the pulse),
    **drift**.
  - **acid**, after the TB-303: a saw or a square into its own filter, the 303's knobs on the first page: **cutoff**,
    **reso**, **env mod**, **decay**; the second page: **accent**, **wave**. A note at velocity 110 or more is accented
    (louder, brighter, a shorter sweep, and a charge that builds over accents in a row); a note played into a held one
    **slides** (legato, at the Play page's glide) without a new sweep, any other note jumps. Its Filter page does nothing
    (the filter is its own). On Steps: a step's velocity is its accent, a note longer than its step slides into the next.
  - Factory presets: `tools/presets/poly.py`, `mono.py`, `acid.py` (12, 10 and 8; the FM-1's left out).

- **Three kits.** A drum track takes one from SEL's list on its Engine page: **KIT**, the list's first action (ALGORITHM
  picks, SEL puts it on; the track takes the kit's name, its mix, sends and patterns stay). **ACOUSTIC**: SLOOP's
  sampled studio kit, CC0 recordings from Versilian Studios' VCSL, cut as SLOOP cuts them (`tools/prep_acoustic.py`:
  the metal at 32 kHz, the rest at 22.05 kHz, about 72 KB), levelled to LOOM-8's kit. **909**: the factory 909 samples on kick, snare, hat and clap, LOOM-8's models on the rest. **SYNTH**: LOOM-8's
  own kit. A new project starts on ACOUSTIC. The kits keep the keys' places (kick, snare, closed hat, open hat, clap,
  low tom, high tom), so a pattern plays its like on another kit; ACOUSTIC's eighth key, the drum pads' last, is its
  crash, then the ride, the rim and the tambourine. A sample key is named after its sample (the key grid, the
  messages). Six sample keys sound at once (the FM-1: four), so a crash rings on under a fill.
- **No recorded instruments.** The piano, upright bass, strings, brass, flute and orchestra hit, and their 15 presets,
  are the FM-1's only: their room holds the kits (`tools/gen_samples.py SMK_DROP`). The 909 samples, the sweep, the
  vinyl and the four breaks stay; the sample engine's plain sound is the 909 kick, to play tuned. Samples of your own
  go into the user bank (below).

## DAW mode (MIDI controller)

**< + >** (the second pressed while the first is held) switches **SPIDER ↔ DAW**. In DAW mode the keyboard is a USB
MIDI controller: SPIDER's sequencer stops and the controls leave SPIDER alone. MIDI IN still plays SPIDER (a sound
module for the DAW). The mode is kept: the SMK starts in DAW mode if it was left there. Ported from
[Snoop37](https://github.com/genkigenki/sloop-fm1/tree/snoop37) by genkigenki, by way of the Loom FM-1 port.

- **Keys:** notes on CHANNEL (default 1), the lowest key C3 = 48 at octave 0, with velocity. OCTAVE− / OCTAVE+: the
  octave (±3; both: 0).
- **K1–K8** (as printed; K1 is a knob here, not the volume, which stays where it was), **faders 1–4**, the **16 pads**,
  **13 buttons** and the **wheels** send what the map says. KNOB BANK / FADER BANK: bank A / B of the knobs / faders
  (lit on B); both together: the pads' bank. Knobs ABSOLUTE (0–127, kept per knob) or RELATIVE (64 ± the detents).
  `<` `>` send when let go. A CC TOGGLE flips once a press.
- **PATCH** tapped: **LOCAL** on / off (PATCH lit: on). On: the keys also play the track selected, the bottom row's
  NOTE pads the kit's sounds 1-8 (as SPIDER's drum pads, which the DEFAULT map sends as notes 36-43: a drum rack's
  first eight slots in the same order), the pitch wheel bends the track. Off (the default): SPIDER plays only what
  comes in. (Note repeat is SPIDER's; in DAW mode NOTE REPEAT sends its CC, for the DAW's own.)
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
BOARD=smk37 ./build.sh                  # app/build/spider-smk37.fwsc (SMK-37 Pro)
BOARD=smk37 MODEL=elite ./build.sh      # app/build/spider-smk37-elite.fwsc
BOARD=smk37 MODEL=mke ./build.sh        # app/build/spider-smk37-mke.fwsc
BOARD=smk37 ./build.sh --host           # the simulator: app/build/host/spider-smk37_host
./build.sh                              # the FM-1 build again before tests/run_tests.sh
```

SPIDER lives in [durutti320/fm1-loom](https://github.com/durutti320/fm1-loom/tree/smk37), branch `smk37`, with its
nested projects in [durutti320/fm1-base](https://github.com/durutti320/fm1-base/tree/smk37) and
[durutti320/felucca-bsp](https://github.com/durutti320/felucca-bsp/tree/smk37) (`.gitmodules` names them):
`git clone --recursive --branch smk37 https://github.com/durutti320/fm1-loom`.

On GitHub, `.github/workflows/spider.yml` builds all three packages with the FM-1 community's shared builder
([zvenson/fm1-build](https://github.com/zvenson/fm1-build), the one the [Firmware Hub](https://thefirmwarehub.com)
trusts): a push to `smk37` (or "Run workflow" on the Actions tab) gives them as the run's `packages` download, with
a signed build attestation; a tag `vX.Y` also makes a release. It runs the SMK simulator scenarios and the DAW map
test first.

The simulator takes the SMK's controls in a script: `pad`, `tappad`, `outer`, `fader`, `bend`, `modw`, `k1`, `smkbtn`,
`smktap`, `keyvel`, and `pads` prints the pads' colours (`tests/scenarios_smk37/`; `smk37_daw.xy` checks DAW mode's
MIDI out byte by byte).

## Install, and the way back

1. **Back up the whole flash first.** The SMK-37 keeps its factory key, pad and wheel calibration in flash
   (`trimdat` at 0xC1224), unique to the unit. This firmware never writes there, but have the backup.
   `tools/spider_backup.py` makes it (fm1-to-smk37 `docs/02-update-and-recovery.md`, section 3.1, in one command):
   SPIDER's update makes its first step only (the factory firmware checks the package and starts SPIDER's loader;
   the firmware area is not written), the loader restarts into the boot ROM (UBOOT), and
   [jl-uboot-tool](https://github.com/kagaimiq/jl-uboot-tool) reads the 1 MiB twice. Both reads must match and hold
   `trimdat`; the copy is kept with its SHA-256. It needs an administrator terminal (Windows) or root (Linux), and
   checks that, the tool and the package before it touches the keyboard. It only ever reads.

   ```sh
   python3 tools/spider_backup.py app/build/spider-smk37.fwsc   # jl-uboot-tool in a folder beside it, or --jl DIR
   python3 tools/spider_backup.py --check smk37-backup-*.bin    # check a copy later
   ```

   Then switch the keyboard off and on: it starts in SPIDER's update mode until the install below finishes.
2. `python3 base/tools/fm1_install.py app/build/spider-smk37.fwsc` (the package for your model).
3. The way back: M-VAVE's updater with the stock package for your model.

The GitHub build's `packages` download holds all three packages, `fm1_install.py`, `spider_backup.py` and
`START-HERE.txt` (the Windows steps: Python 3.12, whose python-rtmidi installs ready-made).

The first start clears the stock firmware's settings in the store area. Before SPIDER first writes into USRFLASH it
erases the stock firmware's marker there, so a return to stock rebuilds its factory key mapping.
