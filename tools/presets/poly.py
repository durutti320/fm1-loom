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
P("strings", "strings", "poly", knobs=dict(saw=0, pulse=127, width=50, sub=0), volume=78,
  amp=(70, 80, 118, 92), fenv=(0, 64, 0, 64), filter=("lp24", 84, 5, 64, 64), sends=(0, 35),
  lfo=("wave", 44, 80, "synth free", 2, 1), x=(0, 2, 3, 16), genres=("synthpop",))
# the brass: saw, the filter swelling open with the note, chorus I
P("brass", "brass", "poly", knobs=dict(saw=127, pulse=0, width=0, sub=20), volume=78,
  amp=(40, 80, 112, 64), fenv=(62, 82, 70, 70), filter=("lp24", 60, 16, 98, 64), sends=(0, 20), x=(0, 1, 1, 14),
  genres=("synthpop",))
# the skank: a dub chord off the beat, short and thin (the high-pass up), into the delay and the reverb
P("skank", "chord", "poly", knobs=dict(saw=90, pulse=110, width=20, sub=0), volume=80,
  amp=(0, 62, 0, 40), fenv=(0, 56, 0, 40), filter=("lp24", 92, 10, 84, 64), sends=(45, 40), x=(0, 3, 1, 10),
  genres=("dub", "reggae"))
# the house stab: saw and pulse, a snappy filter, chorus II, a delay throw
P("house stab", "stab", "poly", knobs=dict(saw=127, pulse=80, width=30, sub=40), volume=74,
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
P("dusty keys", "keys", "poly", knobs=dict(saw=80, pulse=100, width=20, sub=10), volume=86,
  amp=(0, 86, 30, 70), fenv=(0, 76, 10, 70), filter=("lp24", 70, 5, 76, 70), sends=(0, 28),
  char=("tape", 60, 50, 127), x=(10, 1, 2, 30), genres=("lofi",))
