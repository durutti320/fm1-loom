# SPROUT: Sound Garden on the M-VAVE FM-1

SPROUT is this branch's FM-1 firmware: a first instrument for a young child, built on LOOM-8's engine, with SMALL AXE,
a dub desk, behind a grown-ups' menu. The code is `app/sprout.c`; LOOM-8's own pages run only in SMALL AXE's
workshop. The SMK-37 build of this branch is still plain LOOM-8.

## The garden (level 1)

- The screen is a keyboard laid out like the FM-1's 27 keys (F3 to G5). The key being played lights, its name
  shows big above it, and *low*, *middle* or *high* sits under the name.
- The keys play real notes (a gentle marimba).
- **KNOB 1** turns between **free** (the keys alone) and **garden**: an endless bed to play over, never quite the
  same twice. A low drone on C and G, a bell wandering over the C major pentatonic (questions, then answers that come
  home to C) and a quiet shaker pulse, at 84 BPM. It never changes chord, so every white key fits. While it plays,
  the white keys glow, and the key the bell is on glows brighter (on the screen too: a gold dot), so the child can
  watch it and copy it.
- Nothing else on the panel does anything at level 1. The volume knob always works.

## The grown-ups' menu

Hold **GLO** and **HOME** together for a second and a half. **SELECT** moves, **KNOB 1** changes, **SEL** opens,
**HOME** goes back.

- **level**: the level the child is on, its idea, and the signs that say they are ready for the next. This build
  has level 1; levels 2–8 (beat, echo, loop, loud & soft, drums, voices, bass & space) come in later builds.
- **note names**: letters (C, C#), do re mi (fixed do: do, di, re ...), or hidden.
- **small axe**: the dub desk. **workshop**: LOOM-8's own pages on the desk's riddim. **garden**: back to the child's.

The settings are kept with the garden's project.

## SMALL AXE

A synth-only version of BLOODCLOT's idea. Eight channels play a one-drop riddim in A minor at 75 BPM: DRUM (Velvet's
kit with Linen's rim), BASS, SKNK, ORGN (the bubble), HORN, VOX (dropped at the start), SIRN, HAND.

| Control | Does |
| --- | --- |
| black keys 1–8 | drop a channel: a tap takes it out or brings it back, a hold takes it out while held |
| white keys 1–8 | throw a channel into the echo while held |
| white keys 9–16 | throw a channel into the spring while held |
| black keys 9–11 | the siren, three pitches |
| KNOB 1–4 | the echo's time, repeats and tone; the spring's size |
| SELECT | the tempo |
| PLAY | play / stop |
| HOME | the workshop (LOOM-8's pages); the grown-ups' menu comes back |

## The slots

The garden keeps its project in a slot of its own: the last slot, or the last free one. A slot that holds anything
else (a LOOM-8 project, say) is never written. SMALL AXE opens a slot that holds a SMALL AXE project, else starts a
new riddim in a free slot (or unsaved, when none is free). In the workshop, SAVE TO and LOAD refuse the garden's slot.

## Building and testing

As LOOM-8 (`BUILDING.md`): `./build.sh` makes `app/build/sprout.fwsc`, `./build.sh --host` the simulator
`app/build/host/sprout_host`. SPROUT's scenarios are `tests/scenarios_sprout/*.xy`; LOOM-8's own run on the same build
with `SPROUT_SHOP=1` (the workshop open from the start). On GitHub, `.github/workflows/sprout.yml` builds the firmware
with the community's shared builder, the browser emulator onto the branch `sprout-emu`, and the logs onto
`sprout-ci-log`.
