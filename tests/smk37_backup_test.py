#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Cam Durnsford (the LOOM-8 SMK-37 port)
"""tools/smk37_backup.py against a simulated SMK-37 (no hardware, no mido, no disk): the factory firmware makes
the update's first step only and LOOM-8's loader takes the UBOOT key; LOOM-8 takes it itself; a keyboard that is a
UBOOT disk already is left alone over MIDI; another model, M-VAVE's own loader, a refusal or a missing setup stop
before anything is sent; two reads that differ or lack the calibration are no backup; jl-uboot-tool is only ever
asked to read. Run from the repo root: python3 tests/smk37_backup_test.py"""
import hashlib
import io
import queue
import shutil
import sys
import tempfile
import threading
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import smk37_backup as SB  # noqa: E402

I = SB.I
I.DELAY.update(open=0, start=0.01, reply=0, loader=0.01, reboot=0.01, retry=0.02, poll=0.05, hs=0.1,
               idle_check=0.4, idle_write=0.4, wait_loader=2, wait_reboot=2)
SB.DELAY.update(gone=1.0, disk=0.5, poll=0.05)
SB.JL_MODULES = ()                     # (jl-uboot-tool's modules need not be here)
SB.is_admin = lambda: True
failed = 0


def ok(cond, what):
    global failed
    print(f"{what:<84} {'ok' if cond else 'FAIL'}")
    failed += not cond


# ------------------------------------------------------- simulated SMK-37 ---

class FakeLink:
    def __init__(self, kb, gen):
        self.kb, self.gen, self.q, self.closed = kb, gen, queue.Queue(), False

    @property
    def lost(self):
        return self.gen != self.kb.gen or not self.kb.connected

    def send(self, pkt):
        if self.lost:
            return False
        self.kb.sent.append(bytes(pkt))
        self.kb.rx(bytes(pkt))
        return True

    def read(self, timeout):
        try:
            return self.q.get(timeout=max(timeout, 0))
        except queue.Empty:
            return None

    def drain(self):
        while not self.q.empty():
            self.q.get_nowait()

    def close(self):
        self.closed = True


class FakeSMK:
    """a MIDI backend with one SMK-37 on it: the update's check step, its loader, the UBOOT key"""

    def __init__(self, image, identity="SMK-37 Pro_016", loader="ota-SMK-37 Pro_900", knows_key=None):
        self.image, self.loader, self.knows_key = image, loader, knows_key
        self.sent, self.links, self.keys, self.gen = [], [], [], 0
        self.upgrades = self.served = self.bad = 0
        self.uboot = False
        self.boot(identity, "SMK-37 Pro")

    def boot(self, identity, name):
        self.gen += 1
        self.identity, self.name, self.connected, self.waiting, self.queue = identity, name, True, None, []

    def input_names(self):
        return [self.name] if self.connected else []

    output_names = input_names

    def open(self, in_name, out_name):
        if in_name != self.name or not self.connected:
            raise IOError("no such port")
        link = FakeLink(self, self.gen)
        self.links.append(link)
        return link

    def tx(self, pkt):
        for link in self.links:
            if link.gen == self.gen and not link.closed:
                link.q.put(bytes(pkt))

    def rx(self, d):
        if d == I.HS_QUERY:
            t = self.identity.encode()
            self.tx(b"\xF0" + I.pack7(bytes([0, 0x59, 0x11, 0, 0, 0]) + t + bytes(28 - len(t))) + b"\xF7")
        elif d == SB.UBOOT_KEY:
            self.keys.append(self.identity)
            knows = self.knows_key if self.knows_key is not None else int(self.identity.split("_")[1]) >= 900
            if knows:                                       # Felucca-family firmware and loaders: into UBOOT
                self.connected, self.uboot = False, True
        elif d == I.UPGRADE:
            self.upgrades += 1
            self.queue = ([(k * 512, 512) for k in range(4)] + [(I.FINISH_WRITE, 8)] if self.identity.startswith("ota-")
                          else [(0, 64), (0x40, 160), (0x1000, 512), (I.FINISH_CHECK, 8)])
            self.next()
        elif self.waiting:
            u = I.unpack7(d[1:-1])
            addr, n = self.waiting
            want = b"success\0" if addr >= I.FINISH_CHECK else self.image[addr:addr + n]
            self.bad += u[14:14 + len(want)] != want
            self.waiting = None
            self.served += 1
            if addr == I.FINISH_CHECK:
                threading.Timer(0.05, self.boot, (self.loader, "Felucca Update")).start()
            elif addr != I.FINISH_WRITE:
                self.next()

    def next(self):
        if self.queue:
            self.waiting = addr, n = self.queue.pop(0)
            u = bytearray([0, 0x59, 0x30, 0, 0, 0, 0]) + addr.to_bytes(4, "little") + bytes([n & 0xFF, n >> 8, 0])
            u.append(~sum(u[6:14]) & 0xFF)
            self.tx(b"\xF0" + I.pack7(u) + b"\xF7")


class FakeDisks:
    """the UBOOT disk appears once the keyboard took the key (or from the start: already)"""

    def __init__(self, kb=None, already=False):
        self.kb, self.already, self.finds = kb, already, 0

    def find(self):
        self.finds += 1
        up = self.already or (self.kb is not None and self.kb.uboot)
        return (r"\\.\PhysicalDrive3", "WL82 UBOOT1.00") if up else (None, False)

    def wait(self, secs):
        return self.find()


def flash(seed=0, trim=True):
    data = bytearray(hashlib.sha256(bytes([seed]) * 64).digest() * (SB.FLASH // 32))
    if trim:
        data[SB.TRIM_AT:SB.TRIM_AT + 7] = SB.TRIM
    return bytes(data)


class FakeReader:
    """jl-uboot-tool's reads: the same flash each time, or another for read 2 (differ), or none"""

    def __init__(self, second=None, trim=True):
        self.second, self.trim, self.calls = second, trim, []

    def __call__(self, jl, device, name, out_dir, say):
        self.calls.append((device, name))
        k = len(self.calls)
        data = flash(0, self.trim) if k == 1 or self.second is None else self.second
        if data is not None:
            (Path(out_dir) / name).write_bytes(data)
        return 0xEB6014


# ----------------------------------------------------------------- helpers ---

TMP = Path(tempfile.mkdtemp(prefix="smk37-backup-"))
JL = TMP / "jl-uboot-tool-master"
JL.mkdir()
(JL / "jluboottool.py").write_text("""import sys, pathlib
pathlib.Path(sys.argv[-1].split()[-1]).write_bytes(b'\\xAA' * 0x100000)
pathlib.Path('argv.txt').write_text('\\n'.join(sys.argv[1:]))
print('================ Quick info ==================')
print('     ID: 0xeb6014')
""")


def package(product="SMK-37 Pro_1000", size=0x2000):
    raw = bytearray((i * 7) & 0xFF for i in range(size + I.BLOCKS))
    for i in range(I.BLOCKS):
        raw[i * I.BLK + I.KEEP] = (ord(product[i]) + i + 1) & 0xFF if i < len(product) else 0x7D
    raw[0x1800:0x1800 + 16] = I.LOADER_MARK
    return bytes(raw)


PRO = package()
PKG = TMP / "loom-smk37.fwsc"
PKG.write_bytes(PRO)


def run(args, kb, disks=None, reader=None, answer=True, out_dir=None):
    out_dir = out_dir or Path(tempfile.mkdtemp(dir=TMP))
    out, err = io.StringIO(), io.StringIO()
    old, sys.stderr = sys.stderr, err
    try:
        rc = SB.main([*args, "--jl", str(JL), "--out", str(out_dir)], backend=kb, out=out, ask=lambda _p: answer,
                     disks=disks if disks is not None else FakeDisks(kb), reader=reader or FakeReader())
    finally:
        sys.stderr = old
    return rc, out.getvalue() + err.getvalue(), out_dir


def files(d):
    return sorted(p.name for p in Path(d).iterdir())


# ------------------------------------------------------------------- tests ---

def from_factory():
    kb = FakeSMK(I.logical_image(PRO))
    rc, text, d = run([str(PKG)], kb)
    names = files(d)
    bins = [n for n in names if n.endswith(".bin")]
    ok(rc == 0 and kb.upgrades == 1 and kb.bad == 0 and kb.served == 4,
       "factory firmware: the update's check step only (its reads served, no write)")
    ok(kb.keys == ["ota-SMK-37 Pro_900"], "the UBOOT key went to LOOM-8's loader, and only to it")
    ok(len(bins) == 1 and bins[0].startswith("smk37-backup-") and not any("read" in n for n in names),
       "one backup file kept, the two reads folded into it")
    want = hashlib.sha256(flash(0)).hexdigest()
    ok(bins and hashlib.sha256((d / bins[0]).read_bytes()).hexdigest() == want, "the backup is the flash read")
    sums = [n for n in names if n.endswith(".sha256.txt")]
    ok(sums and (d / sums[0]).read_text().startswith(f"{want}  {bins[0]}") and "0xeb6014" in (d / sums[0]).read_text(),
       "its SHA-256 written beside it, with the flash ID")
    ok("backup OK" in text and "power switch" in text, "says it is done, and to switch the keyboard off and on")


def from_loom():
    kb = FakeSMK(I.logical_image(PRO), identity="SMK-37 Pro_1000")
    rc, text, d = run([str(PKG)], kb)
    ok(rc == 0 and kb.upgrades == 0 and kb.keys == ["SMK-37 Pro_1000"],
       "LOOM-8 running: the key straight to it, no update step")
    kb = FakeSMK(I.logical_image(PRO), identity="ota-SMK-37 Pro_900")
    rc, text, d = run([str(PKG)], kb)
    ok(rc == 0 and kb.upgrades == 0 and kb.keys == ["ota-SMK-37 Pro_900"],
       "already in LOOM-8's update mode: the key to the loader")


def already_disk():
    kb = FakeSMK(I.logical_image(PRO))
    reader = FakeReader()
    rc, text, d = run([str(PKG)], kb, disks=FakeDisks(already=True), reader=reader)
    ok(rc == 0 and kb.sent == [] and len(reader.calls) == 2, "a UBOOT disk already: nothing over MIDI, two reads")


def refusals():
    for what, ident in (("an SMK-37 Elite with the Pro package", "SMK-37 Elite_016"),
                        ("an FM-1 with the SMK package", "FM-1_015")):
        kb = FakeSMK(I.logical_image(PRO), identity=ident)
        rc, text, d = run([str(PKG)], kb)
        ok(rc == 6 and kb.upgrades == 0 and kb.keys == [] and files(d) == [], f"refused, nothing sent: {what}")
    kb = FakeSMK(I.logical_image(PRO), identity="ota-SMK-37 Pro_016")
    rc, text, d = run([str(PKG)], kb)
    ok(rc == 4 and kb.upgrades == 0 and kb.keys == [], "M-VAVE's own update mode: refused, nothing sent")
    kb = FakeSMK(I.logical_image(PRO))
    rc, text, d = run([str(PKG)], kb, answer=False)
    ok(rc == 1 and kb.upgrades == 0 and kb.keys == [], "answered no: nothing sent")
    kb = FakeSMK(I.logical_image(PRO), loader="ota-SMK-37 Pro_016")
    rc, text, d = run([str(PKG)], kb)
    ok(rc == 4 and kb.keys == [], "another loader after the check step: no key, stops")
    kb = FakeSMK(I.logical_image(PRO), identity="SMK-37 Pro_1000", knows_key=False)
    rc, text, d = run([str(PKG)], kb)
    ok(rc == 4 and "did not restart" in text, "the key not taken (still on MIDI): stops, no reads")


def setup_first():
    """the setup is checked before the keyboard is touched"""
    kb = FakeSMK(I.logical_image(PRO))
    SB.is_admin = lambda: False
    rc, text, d = run([str(PKG)], kb)
    SB.is_admin = lambda: True
    ok(rc == 2 and kb.sent == [] and "administrator" in text, "not an administrator: stops before MIDI")
    fm1 = TMP / "loom.fwsc"
    fm1.write_bytes(package("FM-1_906"))
    rc, text, d = run([str(fm1)], kb)
    ok(rc == 2 and kb.sent == [], "an FM-1 package: stops before MIDI")
    out, err = io.StringIO(), io.StringIO()
    old, sys.stderr = sys.stderr, err
    try:
        rc = SB.main([str(PKG), "--jl", str(TMP / "nowhere")], backend=kb, out=out, ask=lambda _p: True,
                     disks=FakeDisks(kb), reader=FakeReader())
    finally:
        sys.stderr = old
    ok(rc == 2 and kb.sent == [] and "jl-uboot-tool not found" in err.getvalue(), "no jl-uboot-tool: stops before MIDI")


def bad_reads():
    kb = FakeSMK(I.logical_image(PRO))
    rc, text, d = run([str(PKG)], kb, reader=FakeReader(second=flash(1)))
    ok(rc == 7 and "differ" in text and len([n for n in files(d) if "read" in n]) == 2 and
       not any(n.startswith("smk37-backup") for n in files(d)), "two reads that differ: no backup, both left to see")
    kb = FakeSMK(I.logical_image(PRO))
    rc, text, d = run([str(PKG)], kb, reader=FakeReader(trim=False))
    ok(rc == 7 and "trimdat" in text, "no calibration mark at 0xC1224: no backup")
    kb = FakeSMK(I.logical_image(PRO))
    rc, text, d = run([str(PKG)], kb, reader=FakeReader(second=b""))
    ok(rc == 7 and "copy 2" in text, "a read that wrote nothing: no backup")


def check_mode():
    a, b = TMP / "a.bin", TMP / "b.bin"
    a.write_bytes(flash(0))
    b.write_bytes(flash(0))
    out = io.StringIO()
    ok(SB.main(["--check", str(a), str(b)], out=out) == 0 and "good" in out.getvalue(), "--check: two good copies")
    b.write_bytes(flash(0)[:-1])
    out = io.StringIO()
    ok(SB.main(["--check", str(a), str(b)], out=out) == 7, "--check: one short copy")


def reads_only():
    """the real read: jl-uboot-tool run with one command, read, of the whole flash, into the folder given"""
    d = Path(tempfile.mkdtemp(dir=TMP))
    said = []
    fid = SB.read_flash(JL, r"\\.\PhysicalDrive3", "r1.bin", d, said.append)
    argv = (d / "argv.txt").read_text().split("\n")
    ok(argv == ["--device", r"\\.\PhysicalDrive3", "--chip", "wl82", "read 0 0x100000 r1.bin"],
       "jl-uboot-tool asked for one thing: read 0 0x100000, the WL82, that disk")
    ok(fid == 0xEB6014 and (d / "r1.bin").stat().st_size == SB.FLASH, "its flash ID taken from what it printed")
    src = (ROOT / "tools" / "smk37_backup.py").read_text()
    ok(not any(w in src for w in ('"erase', '"write ', "'erase", "'write ", "erasechip")),
       "the tool holds no erase or write command for the keyboard")


def disk_search():
    """the real search, over a stand-in for jl-uboot-tool's scsiio: the WL82's UBOOT disk among others, a refused
    one noted (not an administrator)"""
    jl = TMP / "jl2"
    (jl / "scsiio").mkdir(parents=True)
    (jl / "scsiio" / "__init__.py").write_text("""
DISKS = {'/dev/sg1': (b'ATA     ', b'Samsung SSD 870 '), '/dev/sg2': (b'WL82    ', b'UBOOT1.00       ')}
DENIED = {'/dev/sg0'}
class SCSIDev:
    def __init__(self, path):
        if path in DENIED: raise PermissionError(path)
        if path not in DISKS: raise OSError(2, 'no such device')
        self.path = path
    def __enter__(self): return self
    def __exit__(self, *a): pass
    def execute(self, cdb, out, data):
        assert cdb == b'\\x12\\x00\\x00\\x00\\x24\\x00' and out is None and len(data) == 36
        v, p = DISKS[self.path]
        data[8:16], data[16:32] = v, p
""")
    paths = SB.scsi_paths
    SB.scsi_paths = lambda: ["/dev/sg0", "/dev/sg1", "/dev/sg2", "/dev/sg3"]
    try:
        disks = SB.Disks(jl)
        ok(disks.find() == ("/dev/sg2", "WL82 UBOOT1.00"), "the disk search: the WL82 UBOOT disk, by an inquiry only")
        sys.modules["scsiio"].DISKS.pop("/dev/sg2")
        ok(disks.find() == (None, True), "no UBOOT disk, one refused: noted (run as administrator)")
    finally:
        SB.scsi_paths = paths
        sys.path.remove(str(jl))


from_factory()
from_loom()
disk_search()
already_disk()
refusals()
setup_first()
bad_reads()
check_mode()
reads_only()
shutil.rmtree(TMP, ignore_errors=True)
print(f"SMK37 BACKUP TESTS FAILED ({failed})" if failed else "smk37 backup tests passed")
sys.exit(1 if failed else 0)
