# SPDX-License-Identifier: GPL-3.0-only
"""fm's presets (SPIDER, the SMK-37 build: dsp/fm_spider.c): six operators in the manner of the classic FM keyboards,
one preset for each of its sixteen patches (tools/gen_fm.py, SPIDER's own), in the patches' order: the patch knob is
the patch's number. bright and decay sit at the middle (the patch as written), detune spreads the carriers. The
operators' envelopes shape the note: the amp envelope held open (full at once, a long release the operators end
first), the filter open. The FM-1 leaves these out (lib.P: no fm engine there)."""
from lib import P

OPEN = ("lp24", 127, 0, 64, 64)
AMP = (0, 64, 127, 84)


def fm(name, role, patch, bright=64, decay=64, detune=0, volume=96, sends=(0, 20), lfo=None, octave=None,
       genres=(), mode="poly", glide=0):
    P(name, role, "fm", knobs=dict(patch=patch, bright=bright, decay=decay, detune=detune), volume=volume, amp=AMP,
      filter=OPEN, sends=sends, lfo=lfo, octave=octave, genres=genres, mode=mode, glide=glide)


# its sound in a new project: the electric piano, a little detune for the shimmer
fm("tine ep", "keys", 0, detune=12, sends=(0, 24), genres=("soul", "lofi", "synthpop"))
fm("glass ep", "keys", 1, detune=10, sends=(10, 24), genres=("synthpop",))
fm("solid bass", "bass", 2, volume=104, sends=(0, 0), genres=("synthpop", "house"))
fm("slap bass", "bass", 3, volume=104, sends=(0, 0), genres=("synthpop", "funk"))
fm("fm brass", "brass", 4, detune=16, sends=(0, 24), genres=("synthpop",))
fm("tube bells", "bell", 5, sends=(20, 40), genres=("synthpop", "ambient"))
fm("marimba", "pluck", 6, sends=(0, 20), genres=("house", "dub"))
fm("harpsi", "keys", 7, sends=(0, 20), genres=("synthpop",))
fm("funk clav", "keys", 8, sends=(0, 10), genres=("funk", "dub"))
fm("fm organ", "keys", 9, sends=(0, 16), genres=("reggae", "house"))
fm("fm strings", "strings", 10, detune=24, sends=(0, 40), genres=("synthpop",))
fm("fm flute", "lead", 11, sends=(0, 30), genres=("ambient",))
fm("saw lead", "lead", 12, detune=20, sends=(20, 20), mode="mono", glide=20, genres=("synthpop",))
fm("kalimba", "pluck", 13, sends=(20, 30), genres=("ambient", "lofi"))
fm("soft pad", "pad", 14, detune=30, sends=(0, 50), genres=("ambient",))
fm("dub stab", "stab", 15, sends=(50, 40), genres=("dub", "reggae"))
