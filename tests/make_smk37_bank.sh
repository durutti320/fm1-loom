#!/bin/sh
# SPIDER's full user bank for tests/scenarios_smk37/smk37_bank.xy (tests/run_tests.sh, the CI's test step): eight
# samples at 32 kHz, 171 of its 180 KB, the last (the sweep) past the 144 KB of earlier builds, in the bank's third
# window (0xEB000.., the gap the stock layout leaves after USRFLASH: dsp/sample.h SMP_REG2); from the factory WAVs
set -e
PY=${PY:-python3}
W=build/gen/wav
mkdir -p build/gen
"$PY" tools/gen_samples.py build/gen/samples.h "$W" >/dev/null
"$PY" tools/loom_samples.py build --board=smk37 build/gen/user_bank_smk37_full.bin \
    "x1=$W/piano.wav:60%32k" "x2=$W/strings.wav:60%32k" "x3=$W/brass.wav:60%32k" "x4=$W/flute.wav:60%32k" \
    "x5=$W/upright.wav:60%32k" "x6=$W/piano.wav:60%32k" "x7=$W/strings.wav:60%32k" "last=$W/sweep.wav:57%32k"
# a bank as the 144 KB builds made it (the size it was made for, 147,456 B: the same bytes; the header's size is not in
# its CRC), still taken
"$PY" tools/loom_samples.py build --board=smk37 build/gen/user_bank_smk37_v144.bin "x1=$W/piano.wav:60" \
    "x2=$W/flute.wav:72%16k"
"$PY" -c "import sys, struct; b = bytearray(open(sys.argv[1], 'rb').read()); b[16:20] = struct.pack('<I', 147456); \
open(sys.argv[1], 'wb').write(b)" build/gen/user_bank_smk37_v144.bin
