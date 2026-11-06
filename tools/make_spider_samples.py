#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""SPIDER's samples page, beside its emulator (.github/workflows/loom8-smk37.yml: the spider2-emu site): LOOM-8's
samples page (app/web/samples/index.html, its bank code loom_samples.js inlined as tools/make_pages.py does) in
SPIDER's words: the SMK-37 and its 180 KB, no "LOOM-8's sounds" (SPIDER has them built in), the breaks its slice
presets play; and loom_samples.js itself beside the emulator, whose own Samples section builds banks with it.

  tools/make_spider_samples.py OUT     OUT/samples/index.html, OUT/samples/breaks/*.wav, OUT/loom_samples.js
"""
import shutil
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "base" / "tools"))
sys.path.insert(0, str(SRC / "tools"))
from make_site import strip_module  # noqa: E402
import gen_samples  # noqa: E402

REPO = "https://github.com/durutti320/fm1-loom/tree/spider2"
# (old, new): each must be on the page once
SAY = [
    ("<title>Samples · LOOM-8</title>", "<title>Samples · SPIDER</title>"),
    ("""  <h1>Samples</h1>
  <p>Your own samples on the FM-1, up to 32 of them. On a track they come after LOOM-8's own: turn the sample
    engine's sample knob (or a drum key's, on the sample model) past the factory ones to user 1, user 2 and so on.</p>
  <p class="small">Connect the FM-1 by USB with LOOM-8 running, and use Chrome or Edge. WAV, AIFF, MP3 and FLAC files work.</p>""",
     """  <a class="home" href="../">← SPIDER in the browser</a>
  <h1>Samples</h1>
  <p>Your own samples on SPIDER, up to 32 of them in 180 KB. On a track they come after SPIDER's own: turn the sample
    engine's sample knob (PARA, then K5), or a drum key's on the sample model, past the factory ones to user 1, user 2
    and so on.</p>
  <p class="small">Connect the SMK-37 by USB with SPIDER running, close REAPER and anything else using its MIDI ports,
    and use Chrome or Edge on a computer. WAV, AIFF, MP3 and FLAC files work.</p>"""),
    ("""  <div class="bar">
    <button class="quiet" id="factory">""", """  <div class="bar" style="display: none">   <!-- (SPIDER has LOOM-8's sounds built in) -->
    <button class="quiet" id="factory">"""),
    ("""  <p class="small">Since LOOM-8 0.7 its four breaks are no longer built into the firmware, to make room for new sounds.
    "Add the breaks" puts them in your bank, then "Send to the FM-1". The slice engine's presets and your projects find
    them there by name, whatever their place in the list. Without them, those presets are silent and the screen says
    "install the breaks". The breaks are free loops posted by u/FCTRYBEATS, with permission to use them in any project.</p>""",
     """  <p class="small">SPIDER's slice presets (breaks, heat, choke, ecstasy and the rest) play four breaks that live in
    your bank, not in the firmware. "Add the breaks" puts them there, then "Send to SPIDER". The presets and your
    projects find them by name, whatever their place in the list. Without them, those presets are silent and the screen
    says "install the breaks". The breaks are free loops posted by u/FCTRYBEATS, with permission to use them in any
    project.</p>"""),
    ("The bank holds about 18 seconds at 22 kHz, 35 at 11 kHz.",
     "The bank holds about 16 seconds at 22 kHz, 31 at 11 kHz."),
    ("""  <p class="small">Also from the command line: <code>tools/loom_samples.py</code>, in
    <a href="https://github.com/charlesvestal/fm1-loom">the source</a>. LOOM-8 is MPL-2.0, built on fm1-base and
    felucca-bsp, from Felucca by Leo Kuroshita (Hügelton Instruments). <a href="../manual/#samples">The manual's Samples section</a>.</p>""",
     f"""  <p class="small">Also from the command line: <code>tools/loom_samples.py load --board=smk37</code>, in
    <a href="{REPO}">the source</a>. SPIDER is GPL-3.0, built on LOOM-8 by Charles Vestal (MPL-2.0), fm1-base and
    felucca-bsp, from Felucca by Leo Kuroshita (Hügelton Instruments).</p>"""),
]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = Path(sys.argv[1])
    page = (SRC / "app" / "web" / "samples" / "index.html").read_text(encoding="utf-8")
    for old, new in SAY:
        if page.count(old) != 1:
            raise SystemExit(f"make_spider_samples: not once on the samples page: {old[:60]!r}")
        page = page.replace(old, new)
    # the device in the page's own words, after the paragraphs above: "the FM-1" is SPIDER, an FM-1 the SMK-37
    page = page.replace("Send to the FM-1", "Send to SPIDER").replace("No FM-1 running LOOM-8 found",
                                                                       "No SMK-37 running SPIDER found")
    page = page.replace("the FM-1", "SPIDER").replace("The FM-1", "SPIDER").replace("FM-1", "SMK-37")
    if page.count("/*LIB*/") != 1:
        raise SystemExit("app/web/samples/index.html must contain /*LIB*/ once")
    lib = (SRC / "app" / "web" / "samples" / "loom_samples.js").read_text(encoding="utf-8")
    (out / "samples" / "breaks").mkdir(parents=True, exist_ok=True)
    lib = strip_module(lib)
    for a, b in (("The FM-1 ", "SPIDER "), ("This LOOM-8 is too old", "This SPIDER is too old"),
                 ("This LOOM-8 can't", "This SPIDER can't")):         # (the library's messages, as the page shows them)
        lib = lib.replace(a, b)
    (out / "samples" / "index.html").write_text(page.replace("/*LIB*/", lib), encoding="utf-8")
    for brk, f, _, _ in gen_samples.RECORDINGS:     # ("Add the breaks" fetches them beside the page)
        shutil.copy(gen_samples.SAMPLE_DIR / f, out / "samples" / "breaks" / f"{brk}.wav")
    shutil.copy(SRC / "app" / "web" / "samples" / "loom_samples.js", out / "loom_samples.js")
    print(f"spider samples page: {out / 'samples' / 'index.html'}")


if __name__ == "__main__":
    main()
