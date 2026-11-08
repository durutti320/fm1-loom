# SPDX-License-Identifier: GPL-3.0-only
"""poly's presets (SPIDER, the SMK-37 build: dsp/synth_spider.c), after the Juno-106: one oscillator a voice (saw,
pulse and its width, a sub an octave down: the knobs; noise on the second page), the Juno's high-pass and chorus,
a 24 dB filter. x = (noise, hpf 0 boost / 1 flat / 2 240 Hz / 3 720 Hz, chorus 0 off / 1 I / 2 II / 3 I+II, drift).
The LFO on the synth's third knob (width) is PWM. The FM-1 leaves these out (lib.P: no poly engine there)."""
from lib import P

# its sound in a new project: the Juno's saw with a little sub, chorus I, the filter opening on the note
P("poly", "keys", "poly", knobs=dict(saw=127, pulse=0, width=0, sub=30), volume=86,
  amp=(2, 84, 96, 66), fenv=(0, 74, 30, 64), filter=("lp24", 70, 10, 82, 64), sends=(0, 20), x=(0, 1, 1, 16),
  genres=("synthpop", "house"))
# the pad: saw and a pulse the LFO widens and narrows (PWM), chorus II, slow in and out
P("pwm pad", "pad", "poly", knobs=dict(saw=110, pulse=90, width=40, sub=0), volume=74,
  amp=(80, 90, 120, 96), fenv=(60, 90, 70, 90), filter=("lp24", 72, 5, 72, 64), sends=(0, 40),
  lfo=("wave", 34, 84, "synth free", 2, 1), x=(0, 2, 2, 20), genres=("synthpop", "lofi"))
# the string machine: the pulse alone, swept, chorus I+II's shimmer
P("juno string", "strings", "poly", knobs=dict(saw=0, pulse=127, width=50, sub=0), volume=78,
  amp=(70, 80, 118, 92), fenv=(0, 64, 0, 64), filter=("lp24", 84, 5, 64, 64), sends=(0, 35),
  lfo=("wave", 44, 80, "synth free", 2, 1), x=(0, 2, 3, 16), genres=("synthpop",))
# the brass: saw, the filter swelling open with the note, chorus I
P("juno brass", "brass", "poly", knobs=dict(saw=127, pulse=0, width=0, sub=20), volume=78,
  amp=(40, 80, 112, 64), fenv=(62, 82, 70, 70), filter=("lp24", 60, 16, 98, 64), sends=(0, 20), x=(0, 1, 1, 14),
  genres=("synthpop",))
# the skank: a dub chord off the beat, short and thin (the high-pass up), into the delay and the reverb
P("skank", "chord", "poly", knobs=dict(saw=90, pulse=110, width=20, sub=0), volume=80,
  amp=(0, 62, 0, 40), fenv=(0, 56, 0, 40), filter=("lp24", 92, 10, 84, 64), sends=(45, 40), x=(0, 3, 1, 10),
  genres=("dub", "reggae"))
# the house stab: saw and pulse, a snappy filter, chorus II, a delay throw
P("juno stab", "stab", "poly", knobs=dict(saw=127, pulse=80, width=30, sub=40), volume=74,
  amp=(0, 70, 0, 52), fenv=(0, 60, 0, 48), filter=("lp24", 74, 22, 104, 64), sends=(40, 22), x=(0, 2, 2, 12),
  genres=("house", "garage"))
# the keys: a square's hollow, decaying like a piano's, chorus II
P("poly keys", "keys", "poly", knobs=dict(saw=40, pulse=127, width=10, sub=0), volume=88,
  amp=(0, 88, 20, 70), fenv=(0, 80, 10, 70), filter=("lp24", 76, 8, 80, 80), sends=(0, 24), x=(0, 1, 2, 14),
  genres=("lofi", "synthpop"))
# the pluck: the filter's snap on a saw, the resonance just showing, chorus I
P("pluck", "pluck", "poly", knobs=dict(saw=127, pulse=0, width=0, sub=0), volume=104,
  amp=(0, 70, 0, 58), fenv=(0, 58, 0, 50), filter=("lp24", 54, 34, 108, 64), sends=(32, 24), x=(0, 1, 1, 10),
  genres=("synthpop", "house"))
# the bell: a narrow pulse's overtones rung by a resonant filter, long, chorus I+II
P("glass", "bell", "poly", knobs=dict(saw=0, pulse=127, width=96, sub=0), volume=84,
  amp=(0, 96, 0, 92), fenv=(0, 84, 0, 84), filter=("lp24", 84, 60, 84, 100), sends=(20, 40), x=(0, 2, 3, 10),
  genres=("synthpop", "lofi"))
# the bass: saw and sub, one voice, the filter low and plucked
P("poly bass", "bass", "poly", knobs=dict(saw=127, pulse=0, width=0, sub=90), mode="mono", volume=104,
  amp=(0, 78, 90, 34), fenv=(0, 62, 10, 40), filter=("lp24", 48, 14, 86, 40), x=(0, 0, 0, 8),
  genres=("house", "synthpop"))
# the warm pad: saw and sub, the HPF's bass boost, slow, the filter breathing on a drift, chorus I
P("warm pad", "pad", "poly", knobs=dict(saw=127, pulse=40, width=30, sub=50), volume=74,
  amp=(90, 90, 120, 100), fenv=(0, 64, 0, 64), filter=("lp24", 62, 5, 64, 64), sends=(0, 45),
  lfo=("drift", 22, 60, "filter free", 0, 64), x=(0, 0, 1, 24), genres=("lofi", "dub"))
# dusty keys: worn tape on the keys
P("dusty juno", "keys", "poly", knobs=dict(saw=80, pulse=100, width=20, sub=10), volume=86,
  amp=(0, 86, 30, 70), fenv=(0, 76, 10, 70), filter=("lp24", 70, 5, 76, 70), sends=(0, 28),
  char=("tape", 60, 50, 127), x=(10, 1, 2, 30), genres=("lofi",))

# SPIDER's (8 Nov): more of the Juno's range
# the house organ-ish chord: square pulse, short, chorus off, for chord stabs on the offbeat
P("house chord", "chord", "poly", knobs=dict(saw=60, pulse=127, width=0, sub=40), volume=82,
  amp=(0, 60, 0, 40), fenv=(0, 56, 0, 40), filter=("lp24", 66, 20, 80, 64), sends=(20, 20), x=(0, 1, 0, 8),
  genres=("house", "garage"))
# hoover-ish: saws, sub, chorus I+II, a glide: the rave classic, played low
P("hoover", "lead", "poly", knobs=dict(saw=127, pulse=100, width=60, sub=100), mode="mono", glide=40, volume=82,
  amp=(4, 80, 120, 70), fenv=(0, 64, 0, 64), filter=("lp24", 80, 10, 64, 64), sends=(10, 30),
  lfo=("wave", 50, 90, "synth free", 2, 1), x=(0, 1, 3, 24), genres=("house", "dnb"))
# the dreamy pad: slow, dark, chorus II, a long release into the room
P("dream pad", "pad", "poly", knobs=dict(saw=100, pulse=60, width=50, sub=20), volume=72,
  amp=(100, 100, 120, 110), fenv=(90, 100, 80, 100), filter=("lp24", 58, 10, 70, 64), sends=(10, 60),
  lfo=("wave", 20, 70, "synth free", 2, 1), x=(10, 2, 2, 28), genres=("ambient", "lofi"))
# the bell-ish: a narrow pulse, high-passed, short with a long tail, chorus I
P("juno bell", "bell", "poly", knobs=dict(saw=0, pulse=127, width=110, sub=0), volume=78,
  amp=(0, 70, 0, 80), fenv=(0, 60, 0, 60), filter=("lp24", 90, 30, 64, 80), sends=(30, 40), x=(0, 3, 1, 10),
  genres=("synthpop", "ambient"))
# the sync-less "sweep pad": the filter rising slowly, resonant, PWM
P("sweep pad", "pad", "poly", knobs=dict(saw=127, pulse=80, width=50, sub=0), volume=74,
  amp=(60, 100, 120, 100), fenv=(127, 110, 100, 100), filter=("lp24", 40, 70, 100, 64), sends=(0, 50),
  lfo=("wave", 30, 80, "synth free", 2, 1), x=(0, 1, 2, 20), genres=("synthpop", "ambient"))
# the reggae bubble: short, dark, chorus off, the organ's shuffle on the keys
P("bubble", "keys", "poly", knobs=dict(saw=40, pulse=127, width=20, sub=60), volume=84,
  amp=(0, 50, 0, 30), fenv=(0, 48, 0, 30), filter=("lp24", 52, 10, 70, 64), sends=(10, 10), x=(0, 1, 0, 6),
  genres=("reggae", "dub"))
# the lush keys: saw and pulse, a soft attack, chorus I+II
P("lush keys", "keys", "poly", knobs=dict(saw=110, pulse=70, width=30, sub=20), volume=80,
  amp=(10, 84, 90, 80), fenv=(0, 70, 30, 70), filter=("lp24", 74, 10, 70, 64), sends=(10, 30), x=(0, 1, 3, 18),
  genres=("synthpop", "lofi", "soul"))
# the noise wash: noise and a little saw, very slow, high-passed: a texture under a track
P("noise wash", "fx", "poly", knobs=dict(saw=30, pulse=0, width=0, sub=0), volume=70,
  amp=(110, 100, 120, 110), fenv=(100, 100, 100, 100), filter=("lp24", 70, 40, 60, 64), sends=(20, 70),
  x=(127, 3, 2, 30), genres=("ambient",))
