#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""SPIDER's FM engine (app/dsp/fm_spider.c): msfa's start-up tables and the factory patches, as one C header.

  tools/gen_fm.py OUT.h          (run by the build: app/fm1.toml gen)
  tools/gen_fm.py --names        the patches' names, in their order (the presets and the docs use them)

The tables are SLOOP's (tools/gen_tables.py fm6_tables, by Leo Kuroshita, Hugelton Instruments: GPL-3.0), at
SPIDER's rate (44.1 kHz) and the core's 32-sample block, with the 1025-point Q15 sine fm6_core.c reads.

The patches are SPIDER's own, written here as operator settings in the classic 6-operator voice layout (the
helpers op / voice after SLOOP's tools/gen_fm6_patches.py). They are sounds in the manner of the classic FM
keyboards' best known voices (an electric piano, a bass, brass, bells, a marimba...), made for SPIDER: no
instrument's factory data is copied. Operators are listed OP1..OP6; the voice starts with OP6, as the format does.
An algorithm is 1..32 in the usual numbering: 5 three pairs (1<-2, 3<-4, 5<-6 with the feedback), 1 two stacks
(1<-2, 3<-4<-5<-6), 16 one carrier under the five, 22 one stack and three carriers sharing a modulator, 32 six
carriers (an organ)."""
import math
import sys
from pathlib import Path

FS = 44100
BLOCK = 32


def arr(name, ctype, vals, per=12):
    out = [f"static const {ctype} {name}[{len(vals)}] = {{"]
    for i in range(0, len(vals), per):
        out.append("    " + ", ".join(str(v) for v in vals[i:i + per]) + ",")
    out.append("};")
    return out


FM6_LFO_SOURCE = [   # msfa lfo.cc: the LFO rates 0..99 (Hz)
    0.062541, 0.125031, 0.312393, 0.437120, 0.624610, 0.750694, 0.936330, 1.125302, 1.249609, 1.436782,
    1.560915, 1.752081, 1.875117, 2.062494, 2.247191, 2.374451, 2.560492, 2.686728, 2.873976, 2.998950,
    3.188013, 3.369840, 3.500175, 3.682224, 3.812065, 4.000800, 4.186202, 4.310716, 4.501260, 4.623209,
    4.814636, 4.930480, 5.121901, 5.315191, 5.434783, 5.617346, 5.750431, 5.946717, 6.062811, 6.248438,
    6.431695, 6.564264, 6.749460, 6.868132, 7.052186, 7.250580, 7.375719, 7.556294, 7.687577, 7.877738,
    7.993605, 8.181967, 8.372405, 8.504848, 8.685079, 8.810573, 8.986341, 9.122423, 9.300595, 9.500285,
    9.607994, 9.798158, 9.950249, 10.117361, 11.251125, 11.384335, 12.562814, 13.676149, 13.904338, 15.092062,
    16.366612, 16.638935, 17.869907, 19.193858, 19.425019, 20.833333, 21.034918, 22.502250, 24.003841, 24.260068,
    25.746653, 27.173913, 27.578599, 29.052876, 30.693677, 31.191516, 32.658393, 34.317090, 34.674064, 36.416606,
    38.197097, 38.550501, 40.387722, 40.749796, 42.625746, 44.326241, 44.883303, 46.772685, 48.590865, 49.261084]


def tables():
    L = ["/* msfa's start-up tables, precomputed (fm6_core.c) */",
         f"#define FM6_LFO_UNIT {int(BLOCK * 25190424 / FS + 0.5)}",
         f"#define FM6_PENV_UNIT {int(BLOCK * (1 << 24) / (21.3 * FS) + 0.5)}",
         f"#define FM6_FREQ_R {int(round(2 ** 40 / FS))}u",
         f"#define FM6_AMS_C0 {int(round(12.2 * math.log2(math.e) * 2 ** 24))}",
         f"#define FM6_AMS_K {int(round(0.07 / 262144 * math.log2(math.e) * 2 ** 24 * 1024))}"]
    L += arr("FM6_SINE", "int16_t", [int(round(32767 * math.sin(2 * math.pi * i / 1024))) for i in range(1025)])
    L += arr("FM6_EXP2", "uint32_t", [int(round((1 << 30) * (2 ** (i / 1024) - 1))) for i in range(1025)], 8)
    lforatio = int(4437500000.0 * BLOCK / FS)
    L += arr("FM6_LFO_DELTA", "uint32_t", [int(s * lforatio) for s in FM6_LFO_SOURCE], 8)
    base, step = 50857777, (1 << 24) // 12
    det = [int(round(0.0209 * math.exp(-0.396 * ((base + step * mn) / (1 << 24))) / 7 * (base + step * mn)))
           for mn in range(128)]
    L += arr("FM6_DETUNE", "uint16_t", det, 16)
    L += arr("FM6_FINE", "int32_t", [int(math.floor(24204406.323123 * math.log(1 + 0.01 * f) + 0.5)) for f in range(100)], 10)
    return L


def op(r=(99, 99, 99, 99), l=(99, 99, 99, 0), ol=0, fc=1, ff=0, det=7, mode=0, kvs=0, ams=0, rs=0,
       bp=39, ld=0, rd=0, lc=0, rc=0):
    """an operator: rates and levels (0..99), output level, coarse (0 = x0.5) / fine ratio, detune (7 = none),
    velocity sensitivity (0..7), rate scaling (0..7), keyboard level scaling around break point bp"""
    return dict(r=r, l=l, ol=ol, fc=fc, ff=ff, det=det, mode=mode, kvs=kvs, ams=ams, rs=rs, bp=bp, ld=ld, rd=rd,
                lc=lc, rc=rc)


def voice(name, alg, ops, fb=0, oks=1, pr=(99, 99, 99, 99), pl=(50, 50, 50, 50), lfs=35, lfd=0, lpmd=0, lamd=0,
          lks=1, lfw=0, lpms=3, trnsp=24):
    """the 155-byte single-voice layout (OP6 first); alg 1..32"""
    assert len(ops) == 6 and 1 <= alg <= 32 and len(name) <= 10
    v = []
    for o in reversed(ops):
        v += list(o["r"]) + list(o["l"]) + [o["bp"], o["ld"], o["rd"], o["lc"], o["rc"], o["rs"], o["ams"],
                                            o["kvs"], o["ol"], o["mode"], o["fc"], o["ff"], o["det"]]
    v += list(pr) + list(pl) + [alg - 1, fb, oks, lfs, lfd, lpmd, lamd, lks, lfw, lpms, trnsp]
    v += [ord(c) for c in name.ljust(10)]
    assert len(v) == 155
    return v


OFF = op(ol=0)
# carriers' and modulators' common shapes
HIT = (99, 30, 25, 60)          # an instant attack, a fall, a slow fall to nothing, a medium release

PATCHES = [
    # an electric piano: a tine (a high ratio struck hard and gone at once) over a body (1:1, softly)
    voice("TINE EP", 5, [
        op(r=(96, 25, 25, 67), l=(99, 75, 0, 0), ol=99, kvs=2, rs=3),
        op(r=(95, 50, 35, 78), l=(99, 75, 0, 0), ol=60, kvs=6, rs=3),
        op(r=(95, 20, 20, 50), l=(99, 95, 0, 0), ol=93, kvs=2, det=8, rs=2),
        op(r=(95, 29, 20, 50), l=(99, 95, 0, 0), ol=48, kvs=6, fc=1, det=6),
        op(r=(95, 20, 20, 50), l=(99, 95, 0, 0), ol=80, kvs=2, det=6, rs=2),
        op(r=(98, 65, 25, 60), l=(99, 50, 0, 0), ol=72, kvs=7, fc=14, rs=4),
    ], fb=6),
    # brighter, glassier keys: the tine pair louder, the body thinner
    voice("GLASS EP", 5, [
        op(r=(97, 30, 28, 62), l=(99, 70, 0, 0), ol=99, kvs=3, rs=3),
        op(r=(97, 55, 40, 70), l=(99, 60, 0, 0), ol=70, kvs=6, fc=1, rs=3),
        op(r=(97, 25, 22, 55), l=(99, 90, 0, 0), ol=88, kvs=2, det=9, rs=2),
        op(r=(97, 40, 30, 60), l=(99, 80, 0, 0), ol=62, kvs=6, fc=5, det=5),
        OFF,
        OFF,
    ], fb=0),
    # a solid bass: one carrier, a 1:1 modulator falling fast (the pluck), a second an octave up for the edge
    voice("SOLID BASS", 1, [
        op(r=(99, 35, 30, 70), l=(99, 85, 0, 0), ol=99, kvs=2, rs=2),
        op(r=(99, 60, 30, 70), l=(99, 70, 40, 0), ol=82, kvs=5, fc=1),
        op(r=(99, 40, 30, 70), l=(99, 80, 0, 0), ol=80, kvs=2, fc=0, det=8),
        op(r=(99, 70, 40, 70), l=(99, 50, 0, 0), ol=70, kvs=5, fc=2),
        OFF,
        OFF,
    ], fb=0),
    # a slap: the modulator at 3 snaps shut, the carrier rings on a little
    voice("SLAP BASS", 1, [
        op(r=(99, 45, 30, 72), l=(99, 80, 0, 0), ol=99, kvs=3, rs=2),
        op(r=(99, 82, 40, 72), l=(99, 40, 0, 0), ol=86, kvs=7, fc=3),
        op(r=(99, 50, 30, 72), l=(99, 75, 0, 0), ol=84, kvs=3, fc=1, det=8),
        op(r=(99, 88, 50, 72), l=(99, 30, 0, 0), ol=78, kvs=7, fc=1),
        OFF,
        OFF,
    ], fb=0),
    # brass: the modulators swell with the carriers (the 'blat'), a little pitch rise at the start
    voice("FM BRASS", 22, [
        op(r=(72, 40, 30, 60), l=(99, 92, 90, 0), ol=99, kvs=2),
        op(r=(62, 45, 30, 60), l=(99, 88, 82, 0), ol=78, kvs=4, fc=1),
        op(r=(72, 40, 30, 60), l=(99, 92, 90, 0), ol=96, kvs=2, det=9),
        op(r=(72, 40, 30, 60), l=(99, 92, 90, 0), ol=96, kvs=2, det=5),
        op(r=(60, 45, 30, 60), l=(99, 85, 80, 0), ol=76, kvs=4, fc=1),
        OFF,
    ], fb=7, pr=(84, 95, 95, 60), pl=(46, 50, 50, 50)),
    # tubular bells: an inharmonic pair (1 : 3.5) ringing long, a second at 1 : 1.41 under it
    voice("TUBE BELLS", 5, [
        op(r=(99, 22, 20, 30), l=(99, 70, 0, 0), ol=99, kvs=2, rs=2),
        op(r=(99, 30, 22, 30), l=(99, 60, 0, 0), ol=76, kvs=3, fc=2, ff=75),
        op(r=(99, 24, 20, 30), l=(99, 70, 0, 0), ol=88, kvs=2, fc=2, det=9),
        op(r=(99, 35, 25, 30), l=(99, 55, 0, 0), ol=68, kvs=3, fc=1, ff=41),
        OFF,
        OFF,
    ], fb=0),
    # a marimba: a short, round bar (the modulator at 4 gone in a moment)
    voice("MARIMBA", 5, [
        op(r=(99, 55, 40, 60), l=(99, 40, 0, 0), ol=99, kvs=3, rs=4),
        op(r=(99, 85, 60, 70), l=(99, 0, 0, 0), ol=78, kvs=5, fc=4, rs=4),
        op(r=(99, 60, 45, 60), l=(99, 30, 0, 0), ol=70, kvs=3, fc=1, det=8, rs=4),
        op(r=(99, 90, 70, 70), l=(99, 0, 0, 0), ol=70, kvs=5, fc=10, rs=4),
        OFF,
        OFF,
    ]),
    # a harpsichord: bright, both pairs plucked, the brightness falling with the note
    voice("HARPSI", 5, [
        op(r=(99, 40, 30, 70), l=(99, 75, 0, 0), ol=99, kvs=1, rs=3),
        op(r=(99, 45, 35, 70), l=(99, 70, 0, 0), ol=82, kvs=2, fc=4, rs=3),
        op(r=(99, 40, 30, 70), l=(99, 75, 0, 0), ol=92, kvs=1, fc=2, det=8, rs=3),
        op(r=(99, 50, 35, 70), l=(99, 60, 0, 0), ol=80, kvs=2, fc=7, rs=3),
        OFF,
        OFF,
    ], fb=0),
    # a clavinet: a nasal, funky pluck (feedback on the top modulator for the bite)
    voice("FUNK CLAV", 5, [
        op(r=(99, 55, 40, 80), l=(99, 60, 0, 0), ol=99, kvs=3, rs=3),
        op(r=(99, 60, 45, 80), l=(99, 50, 0, 0), ol=80, kvs=5, fc=3, rs=3),
        op(r=(99, 55, 40, 80), l=(99, 60, 0, 0), ol=90, kvs=3, fc=1, det=9, rs=3),
        op(r=(99, 70, 50, 80), l=(99, 40, 0, 0), ol=76, kvs=5, fc=1, rs=3),
        op(r=(99, 55, 40, 80), l=(99, 60, 0, 0), ol=0),
        op(r=(99, 60, 50, 80), l=(99, 40, 0, 0), ol=72, kvs=5, fc=2),
    ], fb=5),
    # an organ: six carriers as drawbars (16', 8', 4', 2 2/3', 2', 1 3/5'), a key click at the attack
    voice("FM ORGAN", 32, [
        op(r=(99, 99, 99, 85), l=(99, 99, 99, 0), ol=92, fc=0),
        op(r=(99, 99, 99, 85), l=(99, 99, 99, 0), ol=94, fc=1),
        op(r=(99, 99, 99, 85), l=(99, 99, 99, 0), ol=88, fc=2),
        op(r=(99, 99, 99, 85), l=(99, 99, 99, 0), ol=82, fc=3),
        op(r=(99, 99, 99, 85), l=(99, 99, 99, 0), ol=80, fc=4),
        op(r=(99, 90, 99, 85), l=(99, 0, 0, 0), ol=70, fc=10),
    ], fb=0, lfs=30, lfd=0, lpmd=4, lfw=4, lpms=2),
    # strings: slow in and out, two detuned stacks, a soft vibrato after a while
    voice("FM STRINGS", 1, [
        op(r=(45, 30, 30, 45), l=(99, 92, 90, 0), ol=96, kvs=1, det=10),
        op(r=(42, 30, 30, 45), l=(99, 90, 86, 0), ol=72, kvs=1, fc=1),
        op(r=(45, 30, 30, 45), l=(99, 92, 90, 0), ol=96, kvs=1, det=4),
        op(r=(42, 30, 30, 45), l=(99, 90, 86, 0), ol=70, kvs=1, fc=1, det=9),
        op(r=(50, 30, 30, 45), l=(99, 85, 80, 0), ol=58, fc=2),
        op(r=(50, 30, 30, 45), l=(99, 80, 75, 0), ol=48, fc=1),
    ], fb=4, lfs=33, lfd=60, lpmd=8, lpms=3),
    # a flute: a sine with a breath of noise from the feedback, a slow vibrato
    voice("FM FLUTE", 16, [
        op(r=(60, 40, 30, 60), l=(99, 95, 92, 0), ol=99, kvs=2),
        op(r=(55, 40, 30, 60), l=(99, 90, 85, 0), ol=55, kvs=3, fc=1),
        op(r=(60, 40, 30, 60), l=(99, 80, 75, 0), ol=48, fc=2),
        OFF,
        op(r=(70, 40, 30, 60), l=(99, 70, 60, 0), ol=52, fc=1),
        op(r=(90, 60, 50, 60), l=(99, 40, 30, 0), ol=40, fc=3),
    ], fb=7, lfs=32, lfd=50, lpmd=6, lpms=3),
    # a synth lead: a saw from the feedback, a second detuned beside it
    voice("SAW LEAD", 5, [
        op(r=(99, 50, 40, 60), l=(99, 95, 92, 0), ol=99, kvs=1),
        op(r=(99, 50, 40, 60), l=(99, 95, 92, 0), ol=88, kvs=1, fc=1),
        op(r=(99, 50, 40, 60), l=(99, 95, 92, 0), ol=99, kvs=1, det=10),
        op(r=(99, 50, 40, 60), l=(99, 95, 92, 0), ol=86, kvs=1, fc=1, det=10),
        op(r=(99, 50, 40, 60), l=(99, 95, 92, 0), ol=0),
        op(r=(99, 50, 40, 60), l=(99, 95, 92, 0), ol=99, fc=1),
    ], fb=7),
    # a kalimba: a thumb piano (a short, woody high partial)
    voice("KALIMBA", 5, [
        op(r=(99, 40, 30, 55), l=(99, 50, 0, 0), ol=99, kvs=3, rs=3),
        op(r=(99, 80, 60, 70), l=(99, 0, 0, 0), ol=72, kvs=5, fc=7, rs=3),
        op(r=(99, 45, 30, 55), l=(99, 40, 0, 0), ol=72, kvs=3, fc=3, rs=3),
        op(r=(99, 85, 60, 70), l=(99, 0, 0, 0), ol=60, kvs=5, fc=1, rs=3),
        OFF,
        OFF,
    ]),
    # a soft pad: two slow 1:1 pairs, wide, with the carriers' detune
    voice("SOFT PAD", 5, [
        op(r=(40, 25, 25, 40), l=(99, 95, 92, 0), ol=96, kvs=1, det=10),
        op(r=(38, 25, 25, 40), l=(99, 85, 80, 0), ol=60, kvs=1, fc=1),
        op(r=(40, 25, 25, 40), l=(99, 95, 92, 0), ol=96, kvs=1, det=4),
        op(r=(38, 25, 25, 40), l=(99, 85, 80, 0), ol=58, kvs=1, fc=2),
        OFF,
        OFF,
    ], fb=0, lfs=25, lfd=40, lpmd=5, lpms=2),
    # an FM 'pluck' with a pitch drop: a dub plucked chord stab
    voice("DUB STAB", 5, [
        op(r=(99, 60, 45, 70), l=(99, 40, 0, 0), ol=99, kvs=2, rs=2),
        op(r=(99, 70, 45, 70), l=(99, 30, 0, 0), ol=80, kvs=5, fc=1),
        op(r=(99, 60, 45, 70), l=(99, 40, 0, 0), ol=94, kvs=2, fc=2, det=9),
        op(r=(99, 75, 45, 70), l=(99, 30, 0, 0), ol=76, kvs=5, fc=2),
        OFF,
        OFF,
    ], fb=0, pr=(99, 70, 99, 99), pl=(56, 50, 50, 50)),
]
INIT = voice("INIT VOICE", 1, [op(ol=99)] + [op() for _ in range(5)], lpms=3)


def main(argv):
    names = [bytes(p[145:155]).decode().rstrip() for p in PATCHES]
    if argv and argv[0] == "--names":
        print("\n".join(names))
        return
    L = ["/* generated by tools/gen_fm.py: edit that, not this */", "#pragma once", "#include <stdint.h>", ""]
    L += tables()
    L += [f"#define FM_NPATCH {len(PATCHES)}"]
    L += ["static const uint8_t FM_PATCH[FM_NPATCH][156] = {"]
    for p in PATCHES:
        L.append("    {" + ", ".join(str(x) for x in p) + ", 0},")
    L.append("};")
    L.append("static const char *const FM_PATCH_NAME[FM_NPATCH] = {" + ", ".join(f'"{n.lower()}"' for n in names) + "};")
    text = "\n".join(L) + "\n"
    out = Path(argv[0])
    if not out.exists() or out.read_text() != text:
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(text)


if __name__ == "__main__":
    main(sys.argv[1:])
