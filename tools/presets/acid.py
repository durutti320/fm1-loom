# SPDX-License-Identifier: MPL-2.0
"""acid's presets (2026-10-30): X0X's TB-303 (app/dsp/bass303.c). knobs: wave (0 saw, 1 square), accent, slide (time:
2 .. 360 ms), tune (64 = A440, +-1.5 semitones). Its pages are the 303's: amp=(attack, decay, acc dec, length) is the
Envelope page (attack: the filter's rise on a plain note, 0.3 ms .. 3 s, 32 the stock 3 ms; decay 200 .. 2000 ms; acc
dec, the accented note's, 30 ms .. 3 s; length, a held note's fade, 30 ms .. 3 s, 51 the stock 1.2 s); filter=(type,
cutoff 60 .. 2394 Hz: 64 up the stock 303's 874 .. 2394 Hz, below it stretched down from its 314 Hz (bass303_cutoff_hz;
the values here before 2026-11-01 converted to keep their sound), reso, env mod, low cut: the resonance's feedback high-pass, 350 .. 100 Hz, 86 the stock
150 Hz; the type unused); x = (drive, its type: 0 off, 1 soft, 2 rat; shaper: how square the square, 27 stock). The
filter envelope and the mode are unused (one voice). A step of velocity 112 or more is accented, a note held into the
next slides."""
from lib import P

SAW, SQUARE = 0, 1
OFF, SOFT, RAT = 0, 1, 2

# its sound in a new project: the 303 as it comes, the filter half open, a resonance that sings, a little soft drive
P("acid line", "bass", "acid", knobs=dict(wave=SAW, accent=80, slide=21, tune=64), volume=100,
  amp=(32, 50, 7, 51), filter=("lp24", 55, 96, 70, 86), x=(30, SOFT, 27, 0), genres=("house",))
# squelch: the square, high resonance, the envelope wide open and short
P("squelch", "bass", "acid", knobs=dict(wave=SQUARE, accent=100, slide=21, tune=64), volume=100,
  amp=(32, 30, 7, 51), filter=("lp24", 51, 115, 100, 86), x=(40, SOFT, 27, 0), genres=("house",))
# rubber: a round, short saw bass, little resonance, more low end in it, for under house chords and garage
P("rubber", "bass", "acid", knobs=dict(wave=SAW, accent=50, slide=21, tune=64), volume=88,
  amp=(32, 15, 7, 30), filter=("lp24", 47, 30, 40, 110), x=(0, SOFT, 27, 0), genres=("house", "garage"))
# rat acid: driven hard through the RAT, the resonance biting
P("rat acid", "bass", "acid", knobs=dict(wave=SAW, accent=90, slide=21, tune=64), volume=84,
  amp=(32, 45, 7, 51), filter=("lp24", 57, 105, 80, 86), x=(70, RAT, 27, 0), genres=("house", "dnb"))
# devilfish: long slides, a soft attack (the filter swells in), the accent's filter ringing long, notes held longer
P("devilfish", "bass", "acid", knobs=dict(wave=SAW, accent=100, slide=60, tune=64), volume=100,
  amp=(60, 70, 60, 80), filter=("lp24", 53, 110, 85, 100), x=(35, SOFT, 40, 0), genres=("house",))
# a garage bass: the square, dark and short, rounder (a softer shaper), no squelch
P("garage sq", "bass", "acid", knobs=dict(wave=SQUARE, accent=40, slide=15, tune=64), volume=110,
  amp=(32, 20, 7, 25), filter=("lp24", 45, 20, 35, 86), x=(0, SOFT, 10, 0), genres=("garage",))
# an acid lead: an octave up, a delay throw
P("acid lead", "lead", "acid", knobs=dict(wave=SAW, accent=80, slide=30, tune=64), volume=90,
  amp=(32, 60, 20, 70), filter=("lp24", 59, 100, 75, 86), sends=(40, 10), x=(20, SOFT, 27, 0), octave=-1,
  genres=("house", "lofi"))
# dub acid: slow, open, a soft attack, into the delay and the room
P("dub acid", "bass", "acid", knobs=dict(wave=SQUARE, accent=70, slide=40, tune=64), volume=100,
  amp=(50, 100, 30, 90), filter=("lp24", 55, 90, 60, 86), sends=(50, 30), x=(25, SOFT, 27, 0), genres=("lofi", "house"))

# SPIDER's (8 Nov): more 303 lines, so the acid track has a range of its own
# hoover-ish: the square through the RAT, slides long, the resonance near the edge
P("acid rave", "bass", "acid", knobs=dict(wave=SQUARE, accent=110, slide=50, tune=64), volume=84,
  amp=(32, 55, 20, 60), filter=("lp24", 58, 118, 95, 86), x=(80, RAT, 27, 0), genres=("house", "dnb"))
# a plucky saw, tiny decay: sequencer ticks for a fast pattern
P("acid tick", "bass", "acid", knobs=dict(wave=SAW, accent=70, slide=10, tune=64), volume=98,
  amp=(32, 8, 7, 20), filter=("lp24", 52, 85, 90, 86), x=(20, SOFT, 27, 0), genres=("house", "electro"))
# deep: the filter low, the resonance gentle, a long decay: a sub-ish 303 under dub and dnb
P("deep acid", "bass", "acid", knobs=dict(wave=SAW, accent=40, slide=30, tune=64), volume=112,
  amp=(40, 90, 30, 80), filter=("lp24", 40, 50, 45, 110), x=(15, SOFT, 27, 0), genres=("dub", "dnb"))
# the classic squelch on the saw: wide envelope, short decay, accents popping
P("phuture", "bass", "acid", knobs=dict(wave=SAW, accent=120, slide=21, tune=64), volume=96,
  amp=(32, 25, 10, 51), filter=("lp24", 50, 112, 110, 86), x=(25, SOFT, 27, 0), genres=("house",))
# a slow, wet acid: soft attack, open, into the delay
P("wet acid", "lead", "acid", knobs=dict(wave=SAW, accent=60, slide=45, tune=64), volume=90,
  amp=(70, 80, 40, 80), filter=("lp24", 60, 95, 55, 86), sends=(60, 30), x=(10, SOFT, 27, 0), octave=-1,
  genres=("lofi", "ambient"))
# the square, dark and fat with soft drive: a bassline that sits rather than screams
P("fat square", "bass", "acid", knobs=dict(wave=SQUARE, accent=50, slide=21, tune=64), volume=106,
  amp=(32, 40, 15, 45), filter=("lp24", 46, 60, 55, 100), x=(45, SOFT, 50, 0), genres=("electro", "garage"))
# the resonance screaming, the envelope small: a whistling line
P("whistle", "lead", "acid", knobs=dict(wave=SAW, accent=90, slide=30, tune=64), volume=80,
  amp=(32, 50, 20, 60), filter=("lp24", 62, 125, 40, 86), x=(10, SOFT, 27, 0), octave=-1, genres=("house",))
# the RAT on full, short: a distorted stab
P("acid stab", "stab", "acid", knobs=dict(wave=SAW, accent=100, slide=10, tune=64), volume=82,
  amp=(32, 20, 10, 30), filter=("lp24", 56, 100, 100, 86), sends=(30, 20), x=(100, RAT, 27, 0), genres=("house", "dnb"))
