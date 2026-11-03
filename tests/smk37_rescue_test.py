#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Cam Durnsford (the LOOM-8 SMK-37 port)
"""tools/smk37_rescue.py against a simulated SMK-37 flash behind a simulated jl-uboot-tool (no hardware, no disk):
the same flash: nothing written; check: nothing written; --write: only the differing sectors, never the boot head,
the whole flash equal after; a write that misses a sector the first time is tried again; a boot head that differs,
another keyboard's calibration, a bad backup or one that does not match its .sha256.txt: refused, nothing written;
LOOM-8 answering over MIDI is sent the UBOOT key; firmware that works is sent to fm1_install.py instead.
Run from the repo root: python3 tests/smk37_rescue_test.py"""
import hashlib
import io
import random
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import smk37_rescue as R  # noqa: E402

SB, I = R.SB, R.I
SB.JL_MODULES = ()
SB.is_admin = lambda: True
SB.scsi_paths = lambda: []
SB.DELAY.update(disk=0.05, poll=0.01)
R.DELAY.update(wait=0.1)
failed = 0


def ok(cond, what):
    global failed
    print(f"{what:<84} {'ok' if cond else 'FAIL'}")
    failed += not cond


def image(seed):
    rnd = random.Random(seed)
    b = bytearray(rnd.randbytes(R.FLASH))
    b[SB.TRIM_AT:SB.TRIM_AT + len(SB.TRIM)] = SB.TRIM
    return b


class Disks:
    def __init__(self, there=True):
        self.there = there

    def find(self):
        return ("disk0", "WL82 UBOOT1.00") if self.there else (None, False)

    def wait(self, secs):
        return self.find()


class Chip:
    """the flash behind jl-uboot-tool: its read and write (erase then write) as the real tool does them"""

    def __init__(self, data, flaky=None):
        self.data, self.writes, self.runs, self.flaky = bytearray(data), [], 0, set(flaky or ())

    def run(self, jl, device, cmds, cwd, say):
        self.runs += 1
        for c in cmds:
            w = c.split()
            if w[0] == "read":
                a, n = int(w[1], 0), int(w[2], 0)
                (Path(cwd) / w[3]).write_bytes(bytes(self.data[a:a + n]))
            elif w[0] == "write":
                a = int(w[1], 0)
                d = (Path(cwd) / w[2]).read_bytes()
                self.writes.append((a, len(d)))
                for o in range(0, len(d), R.SECTOR):
                    if a + o in self.flaky:             # (this sector's write misses, once)
                        self.flaky.discard(a + o)
                        continue
                    self.data[a + o:a + o + R.SECTOR] = d[o:o + R.SECTOR]
            else:
                raise AssertionError(f"jl-uboot-tool asked for {c}")
        return []


def go(backup, chip, *args, disks=None, backend=None, note=True, yes=True):
    d = Path(tempfile.mkdtemp())
    f = d / "smk37-backup-test.bin"
    f.write_bytes(bytes(backup))
    if note:
        (d / "smk37-backup-test.sha256.txt").write_text(f"{hashlib.sha256(bytes(backup)).hexdigest()}  {f.name}\n")
    jl = d / "jl-uboot-tool"
    jl.mkdir()
    (jl / "jluboottool.py").write_text("")
    out = io.StringIO()
    argv = [str(f), "--jl", str(jl), *args] + (["--yes"] if yes else [])
    rc = R.main(argv, backend=backend, out=out, ask=lambda q: False, disks=disks or Disks(), run=chip.run)
    return rc, out.getvalue(), f


back = image(1)
broken = bytearray(back)
broken[0x20000:0x28000] = b"\xff" * 0x8000            # the app area half erased (an install cut off)
broken[0x9D000:0x9E000] = bytes(R.SECTOR)              # and the store

# the same flash: nothing written
chip = Chip(back)
rc, txt, _ = go(back, chip, "--write")
ok(rc == 0 and not chip.writes and "nothing to put back" in txt, "the flash the same as the backup: nothing written")

# check: the differing sectors listed, nothing written
chip = Chip(broken)
rc, txt, _ = go(back, chip)
ok(rc == 0 and not chip.writes and "9 sectors differ" in txt, "check: 9 sectors listed, nothing written")
ok("0x20000..0x27fff" in txt and "0x9d000..0x9dfff" in txt, "  as two ranges")

# --write: only those, never the head; the flash equal after, in one run of jl-uboot-tool
chip = Chip(broken)
rc, txt, _ = go(back, chip, "--write")
ok(rc == 0 and bytes(chip.data) == bytes(back) and "restored" in txt, "--write: the flash equals the backup")
ok(sorted(chip.writes) == [(0x20000, 0x8000), (0x9D000, 0x1000)], "  only the differing ranges written")
ok(all(a >= R.HEAD for a, _ in chip.writes), "  the boot head never written")
ok(chip.runs == 2, "  one read, then one run: the writes and the read back")

# --write asks first; no: nothing written
chip = Chip(broken)
rc, txt, _ = go(back, chip, "--write", yes=False)
ok(rc == R.EXIT["cancelled"] and not chip.writes, "--write without yes, answered no: nothing written")

# a sector whose write misses: tried again
chip = Chip(broken, flaky={0x22000})
rc, txt, _ = go(back, chip, "--write")
ok(rc == 0 and bytes(chip.data) == bytes(back) and (0x22000, 0x1000) in chip.writes,
   "a sector that missed the first time: written again, then equal")

# the boot head differs: refused
other_head = bytearray(broken)
other_head[0x1000:0x1010] = b"\x00" * 16
chip = Chip(other_head)
rc, txt, _ = go(back, chip, "--write")
ok(rc == R.EXIT["refused"] and not chip.writes, "the boot head differs: refused, nothing written")

# another keyboard's calibration: refused
other_cal = bytearray(broken)
other_cal[SB.TRIM_AT + 16:SB.TRIM_AT + 32] = b"\x55" * 16
chip = Chip(other_cal)
rc, txt, _ = go(back, chip, "--write")
ok(rc == R.EXIT["refused"] and not chip.writes, "another keyboard's calibration: refused, nothing written")

# calibration erased on the keyboard (no mark): put back from the backup
no_cal = bytearray(broken)
no_cal[R.CAL:R.CAL + R.SECTOR] = b"\xff" * R.SECTOR
chip = Chip(no_cal)
rc, txt, _ = go(back, chip, "--write")
ok(rc == 0 and bytes(chip.data) == bytes(back), "calibration erased on the keyboard: put back from the backup")

# a bad backup, or one that does not match its note
bad = bytearray(back)
bad[SB.TRIM_AT:SB.TRIM_AT + 7] = b"xxxxxxx"
chip = Chip(broken)
rc, txt, _ = go(bad, chip, "--write")
ok(rc == R.EXIT["badread"] and not chip.writes, "a backup without the calibration mark: refused")
chip = Chip(broken)
d = Path(tempfile.mkdtemp())
f = d / "b.bin"
f.write_bytes(bytes(back))
(d / "b.sha256.txt").write_text("0" * 64 + "  b.bin\n")
(d / "jl").mkdir()
(d / "jl" / "jluboottool.py").write_text("")
rc = R.main([str(f), "--jl", str(d / "jl"), "--write", "--yes"], backend=None, out=io.StringIO(),
            disks=Disks(), run=chip.run)
ok(rc == R.EXIT["badread"] and not chip.writes, "a backup that does not match its .sha256.txt: refused")


# over MIDI: LOOM-8 answering gets the UBOOT key; firmware that works is sent to fm1_install.py
class Link:
    def __init__(self):
        self.sent, self.closed = [], False

    def send(self, b):
        self.sent.append(bytes(b))

    def close(self):
        self.closed = True


class Dev:
    def __init__(self, text, version):
        self.id = type("Id", (), {"text": text, "version": version})()
        self.name, self.link = "LOOM-8 SMK-37", Link()


class DisksAfter(Disks):
    """no disk until the key was sent"""

    def __init__(self, dev):
        super().__init__(False)
        self.dev = dev

    def find(self):
        return ("disk0", "WL82 UBOOT1.00") if self.dev.link.sent else (None, False)


real_updater = I.Updater
for text, ver, want in (("SMK-37 Pro_905", 905, "key"), ("SMK-37 Pro_016", 16, "install")):
    dev = Dev(text, ver)
    I.Updater = lambda backend, port=None, _d=dev: type("U", (), {"find": lambda self: _d})()
    chip = Chip(broken)
    rc, txt, _ = go(back, chip, "--write", disks=DisksAfter(dev), backend=object())
    if want == "key":
        ok(dev.link.sent == [SB.UBOOT_KEY] and rc == 0 and bytes(chip.data) == bytes(back),
           "LOOM-8 answering: told to restart into UBOOT, then restored")
    else:
        ok(not dev.link.sent and rc == R.EXIT["stopped"] and not chip.writes,
           "the factory firmware answering: nothing sent, fm1_install.py instead")
I.Updater = real_updater

# no keyboard at all, no disk: it waits, then stops without writing
chip = Chip(broken)
rc, txt, _ = go(back, chip, "--write", disks=Disks(there=False))
ok(rc == R.EXIT["nodisk"] and not chip.writes, "no disk appearing: stops, nothing written")

print("smk37 rescue tests " + ("FAILED" if failed else "passed"))
sys.exit(1 if failed else 0)
