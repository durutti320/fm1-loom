#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The cutting chain is SLOOP's (github.com/genkigenki/sloop-fm1, tools/gen_samples.py gm_kit_entry, GPL-3.0-only).
"""SPIDER's ACOUSTIC kit (the SMK-37 build): SLOOP's sampled studio kit, CC0 recordings from Versilian Studios' VCSL
(github.com/sgossner/VCSL), cut as SLOOP cuts them: from just before the onset, at most KEEP seconds, the kick's and
the toms' bodies shortened (a tuned-up, damped head: the level falls as exp(-t / tau) after a hold), the silent tail
dropped, the end faded. Lighter than SLOOP's (73 KB there, about 50 KB here: room for a second kit beside it in
SPIDER's app slot): the metal at 24 kHz (SLOOP: 32), the kick and the toms at 16 kHz, the rest at 22.05 kHz, the
hats and the cymbals a little shorter. Into app/samples/ac_*.wav, mono, 16-bit, each at the rate the bank keeps it
at. tools/gen_samples.py reads them for BOARD=smk37 only; the kit's levels are its keys' volumes (drumline.c DL_KITS),
as SLOOP's GM_GAIN has them. Run once, by hand; the results are committed.

  git clone --depth 1 https://github.com/genkigenki/sloop-fm1
  tools/prep_acoustic.py sloop-fm1/assets/samples-cc0/KIT
"""
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sampleio as sio  # noqa: E402

DST = Path(__file__).resolve().parents[1] / "app" / "samples"
# name (app/samples/ac_NAME.wav), SLOOP's file in assets/samples-cc0/KIT, rate, seconds kept, (hold, tau) or None,
# the end's fade (s)
PICK = [("kick", "10_kick_BDrumNew_hit_v5.wav", 16000, 0.45, (0.03, 0.13), 0.03),
        ("snare", "11_snare_Snare2_HitSN_v7.wav", 22050, 0.40, None, 0.03),
        ("hat", "14_hihat_HitC_v3.wav", 24000, 0.30, None, 0.03),
        ("open", "15_ohat_HitO.wav", 24000, 0.45, None, 0.10),
        ("rim", "12_rim_sidestick.wav", 22050, 0.20, None, 0.03),
        ("ltom", "16_tomlo_TomL_HitS_v4.wav", 16000, 0.42, (0.04, 0.22), 0.03),
        ("htom", "17_tomhi_TomH_HitS_v4.wav", 16000, 0.40, (0.04, 0.2), 0.03),
        ("crash", "18_crash_susCymb2_hit_f1.wav", 24000, 0.55, None, 0.16),
        ("ride", "19_ride_susCymb2_stick_mf1.wav", 24000, 0.50, None, 0.16),
        ("clap", "13_clap_Clap_rr1.wav", 22050, 0.30, None, 0.03),
        ("tamb", "00_Tamb1_Hit_v2_rr1_Mid.wav", 22050, 0.35, None, 0.06)]


def cut(x, sr, rate, keep, shape, fade):
    pk = max(map(abs, x))
    on = next((i for i, v in enumerate(x) if abs(v) > 0.02 * pk), 0)
    x = sio.resample(x[max(0, on - 16):], sr, rate)[:int(keep * rate)]
    if shape:
        hold, tau = shape
        h = int(hold * rate)
        x = [v * (1.0 if i < h else math.exp(-(i - h) / (tau * rate))) for i, v in enumerate(x)]
    pk = max(map(abs, x))
    end = len(x)                                        # the silent tail dropped
    while end > 64 and abs(x[end - 1]) < 0.004 * pk:
        end -= 1
    x = x[:end]
    f = min(len(x) // 4, int(fade * rate))
    for i in range(f):
        x[len(x) - f + i] *= 1 - i / f
    return x


def write(path, x, rate):
    pk = max(1e-9, max(map(abs, x)))
    s = struct.pack("<%dh" % len(x), *(max(-32768, min(32767, int(round(v / pk * 30000)))) for v in x))
    head = b"fmt " + struct.pack("<IHHIIHH", 16, 1, 1, rate, rate * 2, 2, 16)
    body = head + b"data" + struct.pack("<I", len(s)) + s
    path.write_bytes(b"RIFF" + struct.pack("<I", 4 + len(body)) + b"WAVE" + body)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    src, total = Path(sys.argv[1]), 0
    for name, f, rate, keep, shape, fade in PICK:
        sr, x, _ = sio.read_any_wav(src / f)
        x = cut(x, sr, rate, keep, shape, fade)
        write(DST / f"ac_{name}.wav", x, rate)
        total += sio.sample_bytes(len(x))
        print(f"{name}: {len(x)} samples at {rate} Hz ({len(x) / rate:.2f} s, {sio.sample_bytes(len(x))} B in the bank)")
    print(f"the kit: {total} B ({total / 1024:.1f} KB)")


if __name__ == "__main__":
    main()
