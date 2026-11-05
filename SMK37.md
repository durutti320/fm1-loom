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

**On one sheet:** [the cheat sheet](docs/SMK37-QRG.pdf) (A4 landscape, two-sided: flip on the short edge; the reverse
is DAW mode's REAPER profile), also on the emulator's site as `qrg.html` (the emulator's **Cheat sheet** button).

| SMK-37 | LOOM-8 |
| --- | --- |
| Keys F3..G5 (the middle 27) | the FM-1's 27 keys, **with velocity** (the unit's own factory curves, at **KEY VELOCITY**: below). On Steps and Bar they play the track (the pads are the cells there); a step's pad held on a kit: the black keys its sounds |
| Keys C3..E3, G#5..C6 (the outer 10) | more notes on the track the keys play, with velocity; on a kit, its keys |
| 16 pads, on Steps and Bar (SEQ) | the bar's 16 cells, pads 1-8 the top row, 9-16 the bottom: everything the white keys do there (a tap, a hold, a later one for the length). Lit in the track's colour: a step with notes dim, the step playing bright, a step held white |
| 16 pads, every other page | pads 1-8 (top row): T1-T8, lit while the track plays: a tap mutes or unmutes, held solos while held (more held: more), two taps solo and it stays (a tap ends it). Pads 9-16 (bottom row): the drums, the kit's first eight sounds whatever track is selected (the track selected if it is a kit, else the kit selected last), with velocity, recorded into the kit while recording |
| NOTE REPEAT | tapped: note repeat on / off (lit): a drum pad held sounds again in time, recorded while recording. Held + pads 1-8: its rate (1/4 1/8 1/8T 1/16 1/16T 1/32 1/32T 1/64). Held and let go alone: the LFO page |
| KNOB BANK | the pads' job. On Steps: live pads there too (mutes and drums, lit), again the cells. On any other page: opens Steps, with the pads as its cells |
| < / > | the bar before / after (the bar Steps shows and the pads write), when let go. Both together: DAW mode (below) |
| Faders 1..4 | the levels of T1-T4, or T5-T8 after FADER BANK (lit on T5-T8). A fader acts only once it moves |
| Pitch wheel | bend of the track the keys play, up to a fifth (+-7 semitones) |
| Mod wheel | the filter of the track the keys play: its cutoff (a DYNAMIC filter: its hi cut). A pick-up: the wheel acts once it passes the value the track has, so nothing jumps; the screen says where to pick it up |
| K1 | the volume (the FM-1's MASTER); the screen shows the level as it turns |
| K2 / K3 / K4 | SELECT / ALGORITHM / PRESETS |
| K5..K8 | KNOB 1..4. The screen's hints and help cards use the SMK's names throughout (K2, SCALE, PATCH, PARA, CHORD, REPEAT, GLOBE, BT) |
| FX, SCALE, CHORD, PARA, GLOBE | FX, SEL, ENV, EDIT, GLO |
| PATCH / BT | HOME / SAVE |
| ARP / SEQ / SEQ PLAY / SEQ REC | ARP / SEQ / PLAY / REC |
| SEQ REC tapped twice | a counted-in take, latched. Stopped: a count-in (REC's COUNT-IN, 1 or 2 bars; off or ON NOTE: a bar), then from step 1, recording (the first tap's step record undone). Playing: recording from the next bar line, the rest of this bar the count-in (less than a beat of it left: the bar after; the screen counts the beats, REC blinks); a key half a sixteenth early lands on the bar's first step. The count-in clicks even with the metronome off. A REC press while it waits: off |
| OCTAVE- / OCTAVE+ | OCT- / OCT+ |
| OCTAVE- + OCTAVE+ held at power-on | HARDWARE CALIBRATION: teach the buttons and knobs again |

**KEY VELOCITY** (GLOBE, Setup, SEL list; in DAW mode PATCH held + K8): how hard all 37 keys play, in LOOM-8 and
DAW mode alike (the pads keep their own). The keys time each stroke from the first contact to the second and read it
through the unit's factory curves; KEY VELOCITY sets the time base they read it at: **heavy** (0.1 ms, the reading
before 4 November, where a hard stroke gave about 80 on the unit this was made on), firm, medium, **normal** (the
default, 0.4 ms: that stroke near 127), light, lighter, lightest (0.8 ms); or **fixed 100** / **fixed 127**. The line
above it shows the last key: its velocity and its stroke in ms (DAW mode: on the screen while PATCH is held). Kept with
DAW mode's settings (OBJ_DAW), not in a project.

Not on the SMK-37: TRS MIDI IN (the SMK's jack is a MIDI output; its pin is a key row there). MIDI in works over USB.
LOOM-8's Bluetooth (MIDI and audio) and its USB MIDI host are off in the SMK-37 build for now: the SMK's radio, and
where its flash could keep pairing records, are not yet checked on the device (`app/fm1.toml`, the `smk37_` keys).

## DAW mode (MIDI controller)

**< + >** (the second pressed while the first is held) switches **LOOM-8 ↔ DAW**. In DAW mode the keyboard is a USB
MIDI controller: LOOM-8's sequencer stops and the controls leave LOOM-8 alone. MIDI IN still plays LOOM-8 (a sound
module for the DAW). The mode is kept: the SMK starts in DAW mode if it was left there. Ported from
[Snoop37](https://github.com/genkigenki/sloop-fm1/tree/snoop37) by genkigenki, by way of the Loom FM-1 port.

- **Keys:** notes on CHANNEL (default 1), the lowest key C3 = 48 at octave 0, with velocity. OCTAVE− / OCTAVE+: the
  octave (±3; both: 0).
- **K1–K8** (as printed; K1 is a knob here, not the volume, which stays where it was), **faders 1–4**, the **16 pads**,
  **13 buttons** and the **wheels** send what the map says. KNOB BANK / FADER BANK: bank A / B of the knobs / faders
  (lit on B); both together: the pads' bank. Knobs ABSOLUTE (0–127, kept per knob) or RELATIVE (64 ± the detents).
  `<` `>` send when let go. A CC TOGGLE flips once a press.
- **PATCH** tapped: **LOCAL** on / off (PATCH lit: on). On: the keys also play the track selected, NOTE pads the kit,
  the pitch wheel bends the track. Off (the default): LOOM-8 plays only what comes in.
- **PATCH held:** K5 CHANNEL, K6 CC MAP (DEFAULT, USER, REAPER: a detent a step), K7 KNOBS (left ABSOLUTE, right RELATIVE),
  K8 KEY VELOCITY (as Setup's: the screen shows it and the last key's velocity while PATCH is held), OCTAVE+ the map
  editor.
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

### The REAPER profile (CC MAP: REAPER)

SPIDER shows up as **two USB MIDI ports**: **SPIDER SMK-37** (the music: keys, drum pads, wheels, clock, SPIDER as a
sound module) and a second one for the DAW's control surface (Windows: **MIDIIN2 / MIDIOUT2 (SPIDER SMK-37)**; macOS
and Linux name it **SPIDER DAW**). With CC MAP on **REAPER**, SPIDER speaks **Mackie Control** on the second port to
REAPER's built-in *Mackie Control Universal* surface, and shows what REAPER sends back: the tracks' names, their
pan, meters, select, mute, solo and arm, the transport's lights, the time. The keys, the drum pads, the wheels and
LOCAL work as in the DEFAULT map, on the first port. (`app/smk_mcu.h`; REAPER's side: its SDK's
`reaper_csurf/csurf_mcu.cpp`.) On one page: the [cheat sheet](docs/SMK37-QRG.pdf)'s reverse.

| Control | REAPER profile |
|---|---|
| Keys, wheels | notes on CHANNEL, pitch bend, CC 1 (first port) |
| K1–K8, KNOB BANK unlit (MIX) | the V-pots: strips 1–8's pan (volume with FX: flip) |
| K1, KNOB BANK lit (NAV) | the jog: the edit cursor (with PARA, scrub) |
| K2–K5 (NAV) | CC 102–105, channel 16, relative (64 ± the detents: REAPER's *Relative 2*): learn *View: Scroll horizontally*, *View: Zoom horizontally*, *View: Scroll vertically*, *View: Zoom vertically* (the *MIDI CC relative/mousewheel* ones) |
| K6 (NAV) | CC 106 / 107 (127 a detent): learn *Track: Go to previous track* / *next track* |
| K7 (NAV) | the marker before / after (Mackie's << >>: nothing to learn) |
| K8 (NAV) | anticlockwise undo (Mackie's UNDO), clockwise CC 111: learn *Edit: Redo* |
| FADER 1–4 | strips 1–4's volume, FADER BANK lit 5–8 (pan with flip). A fader waits until it crosses REAPER's value (the screen says up or down), then follows: nothing jumps |
| SEQ PLAY | play; while REAPER plays, stop |
| SEQ REC · NOTE REPEAT · ARP · FX · PARA | record · loop · metronome · flip · scrub |
| SCALE · CHORD · BT | a marker · undo · save |
| `<` `>` | the strips by 8 (GLOBE held: by 1), when let go |
| SEQ | the pads' page: DRUMS → MIXER → ACTIONS (SEQ lit off DRUMS) |
| Pads, DRUMS | notes 36–51 on channel 10, as DEFAULT (LOCAL: the kit too) |
| Pads, MIXER | top row: select strips 1–8 (white); bottom row: mute (red). GLOBE held: arm (red), solo (amber, blinking as REAPER sends it). A pad with no track: dark |
| Pads, ACTIONS | Mackie's F1–F8 (top) and its eight view buttons (bottom), which REAPER hands to its action list: learn each to any action |
| GLOBE | held: SHIFT (above) |
| PATCH | as in every map (held: the settings screen) |

The lights follow REAPER: SEQ PLAY playing, SEQ REC recording, NOTE REPEAT loop, ARP click, FX flip, PARA scrub,
CHORD undo (a flash), BT saved (a second). The screen: REAPER and its transport, the time (bars.beats.ticks, or
min:sec.ms), the eight strips in two rows of four (the faders' row framed, the selected strip filled; name, pan
mark, meter; a red dot armed, an amber one soloed, a muted name faint), the strips' tracks, the pads' page, the
knobs' bank, and the control touched last. Nothing from REAPER for 3 s: **Waiting for REAPER**, and every 3 s the
query (`F0 00 00 66 14 01 58 59 5A F7`) after which REAPER sends everything again (as it does for a real Mackie
when it powers up). What comes in on the second port never plays SPIDER.

**In REAPER** (once): Preferences → Audio → MIDI Devices: **SPIDER SMK-37** in on (and *for control messages*), out
on; leave the second port (MIDIIN2 / MIDIOUT2) off there. Preferences → Control/OSC/web → Add → **Mackie Control
Universal**: MIDI input MIDIIN2 (SPIDER SMK-37), output MIDIOUT2 (SPIDER SMK-37); leave *Map F1–F8 to markers* off (the
ACTIONS pads). Then learn the NAV CCs above in the action list (Add…, turn the knob; K2–K5: CC mode *Relative 2*). Set
recording tracks' inputs to SPIDER SMK-37 channel 1 (the keys) or 10 (the pads), not all channels, so NAV's channel
16 never lands in a take.

**Song Position Pointer** (SYNTH mode, CLOCK: receive): stopped, a DAW's F2 then Continue plays from its place: each
track at the step it would be on there, in its loop, and the bar count with it (a song in Arrange starts from its
first slot). In REAPER: Preferences → Audio → MIDI Devices, the SPIDER SMK-37 output: *Send clock to this device*
(and SPP where it is offered on its own).

## Storage

The SMK-37 has less flash to write than the FM-1, around its calibration and stock data: 120 KB of store and 160 KB
of USRFLASH. This build has **two project slots** (the FM-1 has three), a **144 KB user sample bank** (the FM-1:
216 KB), and DAW mode's settings and map (8 KB). Load a bank built for the SMK:
`tools/loom_samples.py load --board=smk37 ...` (the samples page picks the board from the keyboard's port name). An
FM-1 bank is refused by the SMK and the other way round.

The split is one constant (`app/app.h NPROJ`, with `APP_STORE_MAP` and `dsp/sample.h SMP_REG*`): one slot would leave
about 192 KB of samples, three about 88 KB.

## On GitHub, and in the browser

The emulator's **Pretend REAPER** button plays REAPER's part on the second port (twelve tracks, their names, pan,
volume, select, mute, solo, arm, the transport, the time in beats, the meters while playing): DAW mode, PATCH held,
K6 to REAPER, and the REAPER profile works as on the unit. What SPIDER sends there shows in the MIDI list as DAW.

Each push to the branch `loom8-smk37` builds the packages (Actions, the newest **LOOM-8 SMK-37 firmware** run,
**packages**: the three `.fwsc`, the installer, the backup tool and `START-HERE.txt`) and the emulator, onto the branch
`loom8-smk37-emu` (`app/web/emu_smk37`: the SMK's panel in a page, the firmware itself as WebAssembly; an SMK in
Snoop37's DAW mode plays it over Web MIDI, Chrome or Edge).

## Build

```sh
BOARD=smk37 ./build.sh                  # app/build/loom-smk37.fwsc (SMK-37 Pro)
BOARD=smk37 MODEL=elite ./build.sh      # app/build/loom-smk37-elite.fwsc
BOARD=smk37 MODEL=mke ./build.sh        # app/build/loom-smk37-mke.fwsc
BOARD=smk37 ./build.sh --host           # the simulator: app/build/host/loom-smk37_host
./build.sh                              # the FM-1 build again before tests/run_tests.sh
```

The simulator takes the SMK's controls in a script: `pad`, `tappad`, `outer`, `fader`, `bend`, `modw`, `k1`, `smkbtn`,
`smktap`, `keyvel`, and `pads` prints the pads' colours; `midi USB2 ..` and `sysex USB2 F0 .. F7` come in on the second
port, `expect_out USB2 ..` checks what went out on it (`tests/scenarios_smk37/`; `smk37_daw.xy` checks DAW mode's
MIDI out byte by byte; `smk37_reaper.xy` the REAPER profile both ways, playing REAPER's part; `smk37_spp.xy` Song
Position Pointer; `tests/smk_mcu_test.c` the Mackie messages; `smk37_recbar.xy` the double-tapped REC, its steps and its clicks; `smk37_keyvel.xy` KEY
VELOCITY).

## Install, and the way back

1. **Back up the whole flash first.** The SMK-37 keeps its factory key, pad and wheel calibration in flash
   (`trimdat` at 0xC1224), unique to the unit. This firmware never writes there, but have the backup:
   `tools/smk37_backup.py loom-smk37.fwsc` (read only: it holds no erase or write command; `tools/START-HERE.txt` has
   the Windows steps), or by hand: fm1-to-smk37 `docs/02-update-and-recovery.md`, section 3. Then switch the keyboard
   off and on.
2. `python3 base/tools/fm1_install.py app/build/loom-smk37.fwsc` (the package for your model).
3. The way back: `tools/smk37_rescue.py BACKUP.bin --stock "SMK-37 Pro_016.fwsc" [--write]`: through the chip's
   boot ROM, the backup with the package's factory firmware in its app area and the update area blank (the
   backup holds LOOM-8's staged update program there). M-VAVE's SMK package cannot go on over LOOM-8 with
   `fm1_install.py`: its update program is another kind (`usb_hid_ota.bin`, a 16-byte block before its head),
   which LOOM-8's update refuses rather than hand the keyboard to.
4. The last resort, a keyboard that no longer starts: `tools/smk37_rescue.py BACKUP.bin` (an administrator
   terminal, jl-uboot-tool beside it, as the backup) reads the flash through the chip's boot ROM and lists what
   differs from the backup; `--write` puts those sectors back, never the boot head [0, 0x4000), and refuses
   another keyboard's backup (its calibration differs). Tested against a simulated flash
   (`tests/smk37_rescue_test.py`).

The first start clears the stock firmware's settings in the store area. Before LOOM-8 first writes into USRFLASH it
erases the stock firmware's marker there, so a return to stock rebuilds its factory key mapping.
