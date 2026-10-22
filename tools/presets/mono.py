# SPDX-License-Identifier: GPL-3.0-only
"""mono's presets (SPIDER, the SMK-37 build: dsp/synth_spider.c), after the SH-101: one oscillator (saw, pulse and its
width, a sub: the knobs), its sub's kind and noise on the second page, a 24 dB filter, one voice, a glide.
x = (sub type 0 -1 oct / 1 -2 oct / 2 -2 oct pulse, noise, pw env (the filter envelope narrowing the pulse), drift).
The LFO on the synth's third knob (width) is PWM. The FM-1 leaves these out (lib.P: no mono engine there)."""
from lib import P

# its sound in a new project: the SH's bass, saw and sub, a plucked filter
P("mono", "bass", "mono", knobs=dict(saw=127, pulse=0, width=0, sub=80), mode="mono", volume=108,
  amp=(0, 80, 90, 36), fenv=(0, 64, 10, 40), filter=("lp24", 50, 24, 92, 50), x=(0, 0, 0, 10),
  genres=("house", "synthpop"))
# the 101 bass: a pulse and the sub two octaves down, punchy
P("101 bass", "bass", "mono", knobs=dict(saw=0, pulse=127, width=30, sub=90), mode="mono", volume=104,
  amp=(0, 74, 70, 32), fenv=(0, 58, 0, 36), filter=("lp24", 44, 30, 96, 50), x=(1, 0, 0, 8),
  genres=("house", "electro"))
# the dub bass: round and deep, the sub under a dark saw, legato
P("dub bass", "bass", "mono", knobs=dict(saw=90, pulse=0, width=0, sub=110), mode="legato", glide=10, volume=112,
  amp=(2, 80, 110, 44), fenv=(0, 64, 0, 44), filter=("lp24", 38, 8, 76, 30), x=(0, 0, 0, 12),
  genres=("dub", "reggae"))
# the lead: a saw, a glide, a vibrato coming in late
P("lead", "lead", "mono", knobs=dict(saw=127, pulse=0, width=0, sub=30), mode="legato", glide=26, volume=92,
  amp=(4, 80, 110, 56), fenv=(0, 70, 50, 56), filter=("lp24", 78, 20, 80, 70), sends=(26, 20),
  lfo=("wobble", 92, 76, 64, 20, 0), x=(0, 0, 0, 12), genres=("synthpop",))
# the PWM lead: the pulse, its width on the LFO
P("pwm lead", "lead", "mono", knobs=dict(saw=0, pulse=127, width=40, sub=20), mode="legato", glide=18, volume=90,
  amp=(2, 80, 110, 56), fenv=(0, 64, 0, 56), filter=("lp24", 82, 10, 64, 70), sends=(24, 18),
  lfo=("wave", 60, 86, "synth free", 2, 1), x=(0, 0, 0, 10), genres=("synthpop", "lofi"))
# squelch: the resonance up, the envelope sweeping it, the pulse narrowing with it
P("squelch", "bass", "mono", knobs=dict(saw=60, pulse=100, width=20, sub=50), mode="mono", volume=100,
  amp=(0, 72, 60, 34), fenv=(0, 60, 0, 38), filter=("lp24", 42, 88, 104, 50), x=(0, 0, 60, 6),
  genres=("electro", "house"))
# the zap: a fast sweep down, resonant, for percussive hits
P("zap", "fx", "mono", knobs=dict(saw=127, pulse=0, width=0, sub=0), mode="mono", volume=96,
  amp=(0, 60, 0, 40), fenv=(0, 48, 0, 40), filter=("lp24", 30, 100, 124, 80), sends=(30, 10), x=(0, 0, 0, 0),
  genres=("electro",))
# the arp pluck: short and bright, for the arpeggiator
P("arp pluck", "pluck", "mono", knobs=dict(saw=127, pulse=40, width=20, sub=30), mode="mono", volume=100,
  amp=(0, 62, 0, 50), fenv=(0, 56, 0, 46), filter=("lp24", 56, 30, 100, 70), sends=(36, 20), x=(0, 0, 30, 8),
  genres=("synthpop", "house"))
# the sub: the 25 % pulse two octaves down alone, low-passed: a floor under the track
P("sub", "bass", "mono", knobs=dict(saw=20, pulse=0, width=0, sub=127), mode="mono", volume=116,
  amp=(2, 80, 120, 40), fenv=(0, 64, 0, 40), filter=("lp24", 40, 0, 64, 20), x=(2, 0, 0, 6),
  genres=("dub", "dnb", "garage"))
# the noise sweep: noise through a resonant filter swept by the envelope, a riser's whoosh
P("noise sweep", "fx", "mono", knobs=dict(saw=0, pulse=0, width=0, sub=0), mode="mono", volume=90,
  amp=(70, 90, 100, 80), fenv=(80, 90, 60, 80), filter=("lp24", 50, 70, 110, 0), sends=(20, 40), x=(0, 127, 0, 0),
  genres=("house", "dnb"))
