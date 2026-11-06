#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""SPIDER's acoustic kit (the SMK-37 build: tools/gen_samples.py ACOUSTIC, tools/gen_kits.py "Acoustic"): eleven drum
recordings from Versilian Studios' VCSL, CC0 1.0 (github.com/sgossner/VCSL), as SLOOP keeps them
(github.com/isod89/sloop-fm1, assets/samples-cc0/KIT: mono, cut, normalised; its ATTRIBUTION.txt names each source
file). Here each is mixed to mono, resampled to the rate the firmware keeps it at, cut to its length with the last
part faded (a sample that ends while it still sounds clicks), its peak normalised, into app/samples/acoustic/NAME.wav;
app/samples/acoustic/ATTRIBUTION.txt says where each came from. Run once, by hand; the results are committed.

  git clone --depth 1 https://github.com/isod89/sloop-fm1
  tools/prep_acoustic.py sloop-fm1/assets/samples-cc0/KIT
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sampleio as sio  # noqa: E402

DST = Path(__file__).resolve().parents[1] / "app" / "samples" / "acoustic"
VCSL = "sgossner/VCSL"
# name (the file's, and the sample's on the screen with "ac "), SLOOP's file, its VCSL source, rate, length (s), fade (s)
PICK = [
    ("kick", "10_kick_BDrumNew_hit_v5.wav",
     "Membranophones/Struck Membranophones/Bass Drum 1/BDrumNew_hit_v5_rr1_Sum.wav", 16000, 0.45, 0.10),
    ("snare", "11_snare_Snare2_HitSN_v7.wav",
     "Membranophones/Struck Membranophones/Snare Drum, Modern 1/Snare2_HitSN_v7_rr1_Mid.wav", 22050, 0.38, 0.10),
    ("hat", "14_hihat_HitC_v3.wav",
     "Idiophones/Struck Idiophones/Hi-Hat Cymbal/HiHat_HitC_v3_rr1_Mid.wav", 22050, 0.22, 0.06),
    ("open", "15_ohat_HitO.wav",
     "Idiophones/Struck Idiophones/Hi-Hat Cymbal/HiHat_HitO_rr1_Mid.wav", 22050, 0.48, 0.18),
    ("clap", "13_clap_Clap_rr1.wav",
     "Idiophones/Struck Idiophones/Claps/Clap_rr1.wav", 22050, 0.32, 0.08),
    ("tom lo", "16_tomlo_TomL_HitS_v4.wav",
     "Membranophones/Struck Membranophones/Tom 2/Stick/TomL_HitS_v4_rr1_Mid.wav", 16000, 0.48, 0.15),
    ("tom hi", "17_tomhi_TomH_HitS_v4.wav",
     "Membranophones/Struck Membranophones/Tom 1/Stick/TomH_HitS_v4_rr1_Mid.wav", 16000, 0.42, 0.15),
    ("conga", "02_Conga_HitN_v2_rr1_Sum.wav",
     "Membranophones/Struck Membranophones/Conga/Conga_HitN_v2_rr1_Sum.wav", 16000, 0.34, 0.10),
    ("shaker", "01_Mid_ShakerDouble_Down_rr1.wav",
     "Idiophones/Struck Idiophones/Shaker, Small/Mid_ShakerDouble_Down_rr1.wav", 22050, 0.26, 0.05),
    ("crash", "18_crash_susCymb2_hit_f1.wav",
     "Idiophones/Struck Idiophones/Suspended Cymbal 2/susCymb2_hit_f1.wav", 16000, 0.70, 0.35),
    ("rim", "12_rim_sidestick.wav",
     "Membranophones/Struck Membranophones/Legacy Snares/OldSnare/snare_rim.wav", 22050, 0.20, 0.05),
]


def write(path, s, rate):
    b = struct.pack(f"<{len(s)}h", *s)
    head = b"fmt " + struct.pack("<IHHIIHH", 16, 1, 1, rate, rate * 2, 2, 16)
    body = head + b"data" + struct.pack("<I", len(b)) + b
    path.write_bytes(b"RIFF" + struct.pack("<I", 4 + len(body)) + b"WAVE" + body)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    src = Path(sys.argv[1])
    DST.mkdir(parents=True, exist_ok=True)
    lines = ["SPIDER's acoustic kit (tools/prep_acoustic.py): drum recordings from VCSL by Versilian Studios",
             "Licence: CC0 1.0 Universal (public domain dedication), https://creativecommons.org/publicdomain/zero/1.0/",
             f"Source: https://github.com/{VCSL}, by way of SLOOP (https://github.com/isod89/sloop-fm1,",
             "assets/samples-cc0/KIT: mono, cut, normalised). Here: mono, resampled, cut and faded, normalised.", ""]
    total = 0
    for name, f, vcsl, rate, length, fade in PICK:
        sr, x, _ = sio.read_any_wav(src / f)
        x = sio.resample(x, sr, rate) if sr != rate else list(x)
        n = min(len(x), int(round(length * rate)))
        nf = min(n, int(round(fade * rate)))
        x = [v * (1.0 if i < n - nf else (n - i) / nf) for i, v in enumerate(x[:n])]
        s = sio.to_int16(x)
        write(DST / f"{name.replace(' ', '_')}.wav", s, rate)
        total += len(s) // 2 + (len(s) + sio.BLK - 1) // sio.BLK * 4
        lines.append(f"{name.replace(' ', '_')}.wav  <-  {VCSL}: {vcsl}  ({rate} Hz, {n / rate:.2f} s)")
    (DST / "ATTRIBUTION.txt").write_text("\n".join(lines) + "\n")
    print(f"acoustic kit: {len(PICK)} samples, about {total / 1024:.1f} KB in the image")


if __name__ == "__main__":
    main()
