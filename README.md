# LOOM-8 for the M-VAVE FM-1

> **This is SPIDER's home, a fork.** SPIDER is LOOM-8 ported to the M-VAVE SMK-37 (branch `smk37`): the same
> groovebox with the SMK's pads, faders and wheels, a DAW mode, a spider's-web look and its own boot. It is not the
> official LOOM-8, and its authors don't support it: please report SPIDER problems here, not upstream. See
> [SMK37.md](SMK37.md).
>
> **Credits.** LOOM-8 is by Charles Vestal ([charlesvestal/fm1-loom](https://github.com/charlesvestal/fm1-loom)),
> on his [fm1-base](https://github.com/charlesvestal/fm1-base) and on [felucca-bsp](https://github.com/hugelton/felucca-bsp)
> by Leo Kuroshita (Hügelton Instruments), from [Felucca](https://github.com/hugelton/Felucca). The SMK-37 hardware
> support and the DAW mode are genkigenki's ([fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37), Felucca's
> `smk37` branch, and Snoop37 in [sloop-fm1](https://github.com/genkigenki/sloop-fm1/tree/snoop37)). The SPIDER
> port is by Cam Durnsford, written with Claude. Not affiliated with or endorsed by M-VAVE.
>
> **No warranty.** Installing custom firmware is at your own risk. Back up your keyboard's flash first (SMK37.md),
> and M-VAVE's own updater puts the factory firmware back. Licences as below (LICENSING.md): GPL-3.0-only, with
> MPL-2.0 platform files.

LOOM-8 turns the M-VAVE FM-1 into a pocket groovebox: eight instrument tracks, each in its own colour, on
nine engines (six synths: knit, rayon, jacquard, bellows, cane and voile; a sample player, a break slicer and a drum kit), 85 factory presets; a 64-step sequencer with step tricks and parameter locks; players (arp,
strum, hold); weave, which crosses two patterns (bar by bar, step by step, or one's rhythm with the other's
notes); throws (momentary effects); a conductor track that moves the song through a progression; two
send-effect tracks (the reverb with the PlayStation's reverbs among its modes), a mixer with EQ
and a compressor on the master, scenes and a song. Built on
[fm1-base](https://github.com/charlesvestal/fm1-base) and [felucca-bsp](https://github.com/hugelton/felucca-bsp),
from [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments), with X0X's sequencer
discipline and Polly's menu paradigm.

- **Read the manual**: [docs/MANUAL.md](docs/MANUAL.md), with a ten-minute quick start, every page and gesture, and screenshots
- **Try it in your browser** (no FM-1 needed): <https://charlesvestal.github.io/fm1-loom/emu/>
- **Install it** (Chrome or Edge, with the FM-1 plugged in): <https://charlesvestal.github.io/fm1-loom/install/>
- **Download the firmware file**: [releases](https://github.com/charlesvestal/fm1-loom/releases)

Installing is at your own risk, though it's hard to get stuck: the web installer can put M-VAVE's
firmware back, and an FM-1 that keeps crashing starts in a safe mode.

It also builds for the M-VAVE SMK-37 (Pro, Elite, MKE-P37) as **SPIDER**, with its pads, faders, wheels, a DAW mode
and a spider's-web look: see [SMK37.md](SMK37.md) (beta).

The design is in `docs/superpowers/specs/2026-10-15-xy1-design.md`.

Your projects and presets can be backed up to a computer and put back (the site's Backup page); your own
samples load from their WAV files with `tools/loom_samples.py` (see the manual).

## Building and testing

LOOM-8 is an app (`app/`) on fm1-base, a git submodule (`base/`): clone with `--recursive`. See `BUILDING.md`.
`tests/run_tests.sh` runs everything on the host, silently (the audio goes to WAV files). `LICENSING.md` says
where the code comes from.

## Credits

The platform: fm1-base and felucca-bsp (MPL-2.0), from Felucca by Leo Kuroshita (Hügelton Instruments), with his
permission (felucca-bsp#4). cane and voile: Mutable Instruments Braids by Emilie Gillet (MIT). The reverb's PlayStation modes: PSXVerb (MIT). The factory breaks: free loops by u/FCTRYBEATS,
with their permission; the 909 drums: renders of 9W9 (GPL-3.0); the recorded instruments: Versilian Studios Chamber
Orchestra 2 Community Edition (CC0). The fonts: Barlow and Terminus (SIL OFL 1.1). The second core's lessons: Melodee (keremimo) and Jangada (zednaked).
The idea of a browser emulator: Groove OS.

LOOM-8's way of working owes a great deal to teenage engineering's OP-XY; LOOM-8 is not affiliated with
teenage engineering.
