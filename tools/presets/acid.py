# SPDX-License-Identifier: GPL-3.0-only
"""acid's presets (SPIDER, the SMK-37 build: dsp/synth_spider.c), after the TB-303: a saw or square into its own
filter, the knobs the 303's: cutoff, reso, env mod, decay; x = (accent, wave 0 saw / 1 square). A note at velocity
110 or more is accented; one played into a held one slides (legato, at the glide). The Filter page is not acid's (its
filter is its own). The FM-1 leaves these out (lib.P: no acid engine there)."""
from lib import P

ACID = dict(mode="legato", glide=22, amp=(15, 96, 100, 28), filter=("lp24", 127, 0, 64, 64))   # (attack ~3 ms: no tick)
# its sound in a new project: the classic, half way on everything
P("acid", "bass", "acid", knobs=dict(cutoff=40, reso=90, **{"env mod": 80}, decay=50), volume=100, x=(90, 0, 0, 0),
  genres=("house", "acid"), **ACID)
# squelch: the resonance up, a wide sweep
P("squelchy", "bass", "acid", knobs=dict(cutoff=34, reso=112, **{"env mod": 100}, decay=40), volume=100,
  x=(100, 0, 0, 0), genres=("acid",), **ACID)
# the square: hollower, the 303's other wave
P("square acid", "bass", "acid", knobs=dict(cutoff=46, reso=96, **{"env mod": 70}, decay=56), volume=100,
  x=(90, 1, 0, 0), genres=("acid",), **ACID)
# dark: low and short, a bassline under the track
P("dark acid", "bass", "acid", knobs=dict(cutoff=22, reso=70, **{"env mod": 60}, decay=30), volume=108,
  x=(70, 0, 0, 0), genres=("acid", "techno"), **ACID)
# the screamer: driven hard after the filter, the resonance near the top
P("screamer", "lead", "acid", knobs=dict(cutoff=52, reso=118, **{"env mod": 96}, decay=60), volume=90,
  x=(110, 0, 0, 0), char=("drive", 80, 90, 127), genres=("acid",), **ACID)
# rubber: little resonance, a round bassline
P("rubber", "bass", "acid", knobs=dict(cutoff=30, reso=40, **{"env mod": 64}, decay=44), volume=110,
  x=(60, 0, 0, 0), genres=("house",), **ACID)
# the dub acid: thrown into the delay
P("dub acid", "bass", "acid", knobs=dict(cutoff=36, reso=100, **{"env mod": 84}, decay=64), volume=96,
  sends=(50, 20), x=(90, 1, 0, 0), genres=("dub", "acid"), **ACID)
# worn tape over it
P("tape acid", "bass", "acid", knobs=dict(cutoff=42, reso=96, **{"env mod": 80}, decay=54), volume=100,
  char=("tape", 70, 60, 127), x=(90, 0, 0, 0), genres=("acid", "lofi"), **ACID)
