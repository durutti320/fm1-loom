#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Cam Durnsford (the LOOM-8 SMK-37 port)
"""LOOM-8's backup: a full copy of the SMK-37's flash (1 MiB), made before LOOM-8 goes on (or at any time after).

    py -3.12 smk37_backup.py loom-smk37.fwsc        Windows, in a terminal opened as administrator
    sudo python3 smk37_backup.py loom-smk37.fwsc    Linux
    smk37_backup.py --check FILE.bin [FILE2.bin]      check a copy you have

The keyboard keeps its factory calibration in flash (the keys, pads and wheels, measured for this unit: "trimdat"
at 0xC1224). LOOM-8 never writes there, but the copy is the only way back if anything ever did.

It needs mido and python-rtmidi (as fm1_install.py, which sits beside it), and kagaimiq's jl-uboot-tool
(https://github.com/kagaimiq/jl-uboot-tool, with pyyaml, tqdm and crcmod) unpacked in a folder next to it (or --jl
DIR). It checks all of that, and that it may read the disk, before it touches the keyboard.

1. Over USB MIDI. On the factory firmware: the first step of LOOM-8's update only (genkigenki's fm1-to-smk37,
   docs/02 section 3.1). The factory firmware checks the package and starts LOOM-8's update loader; the firmware
   area is not written. Then the loader (or LOOM-8, if it runs already) is told to restart into the chip's boot
   ROM (UBOOT: the soft key F0 22 24 35 7D F7).
2. The keyboard comes back as a USB disk, WL82 UBOOT1.00. If Windows offers to format it: Cancel.
3. jl-uboot-tool reads the whole flash, twice ("read" only: this tool never erases or writes the keyboard).
4. Both reads must be 1048576 bytes, the same, and hold "trimdat" at 0xC1224.
5. Saved as smk37-backup-DATE.bin, its SHA-256 in smk37-backup-DATE.sha256.txt (next to this file, or --out DIR).

Then switch the keyboard off and on with its power switch. Coming from the factory firmware it starts in LOOM-8's
update mode until an install finishes: fm1_install.py loom-smk37.fwsc puts LOOM-8 on. If the keyboard is a UBOOT
disk already (a run that stopped after step 1), the run starts at step 2.

Exit codes: 0 done, 1 cancelled or failed, 2 bad arguments, package or setup, 3 keyboard not found, 4 connection
lost or the keyboard stopped, 5 the UBOOT disk not found or not readable, 6 another model, 7 a read failed its checks.
"""
import argparse
import datetime
import hashlib
import importlib.util
import os
import re
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
for d in (HERE, HERE.parent / "base" / "tools"):         # beside it (a download), or in the source tree
    if (d / "fm1_install.py").exists():
        sys.path.insert(0, str(d))
        break
import fm1_install as I  # noqa: E402

UBOOT_KEY = bytes([0xF0, 0x22, 0x24, 0x35, 0x7D, 0xF7])
FLASH = 0x100000
TRIM_AT, TRIM = 0xC1224, b"trimdat"
CUSTOM = 900                         # identities from 900 up: Felucca-family firmware and loaders (they know the key)
SMK_MODELS = ("SMK-37 Pro", "SMK-37 Elite", "MKE-P37")
DELAY = {"gone": 15.0, "disk": 60.0, "poll": 1.0}
EXIT = dict(I.EXIT, setup=2, nodisk=5, badread=7, cancelled=1)
JL_MODULES = ("yaml", "tqdm", "crcmod")    # jl-uboot-tool's (requirements.txt: pyyaml, tqdm, crcmod)


class Stop(I.InstallError):
    pass


# ---------------------------------------------------------------- copies ---

def problems(data):
    """what is wrong with one copy of the flash ([] if nothing)"""
    out = []
    if len(data) != FLASH:
        out.append(f"{len(data)} bytes, not {FLASH}")
    elif data[TRIM_AT:TRIM_AT + len(TRIM)] != TRIM:
        out.append(f"no calibration mark at {TRIM_AT:#x} (\"trimdat\")")
    if data and data.count(0xFF) == len(data):
        out.append("all 0xFF (nothing read)")
    return out


def check_copies(datas):
    """-> (sha256 hex, []) or (None, the problems)"""
    errs = [f"copy {i + 1}: {p}" for i, d in enumerate(datas) for p in problems(d)]
    digests = {hashlib.sha256(d).hexdigest() for d in datas}
    if len(digests) > 1:
        errs.append("the copies differ")
    return (None, errs) if errs else (digests.pop(), [])


# ------------------------------------------------------------------ disk ---

def scsi_paths():
    if sys.platform == "win32":
        return [rf"\\.\PhysicalDrive{i}" for i in range(32)]
    if sys.platform.startswith("linux"):
        return sorted(str(p) for p in Path("/dev").glob("sg*"))
    raise Stop("setup", "the flash copy runs on Windows or Linux (jl-uboot-tool reads the disk there)")


def is_admin():
    if sys.platform == "win32":
        import ctypes
        try:
            return bool(ctypes.windll.shell32.IsUserAnAdmin())
        except Exception:
            return False
    return os.geteuid() == 0


class Disks:
    """the UBOOT disk over jl-uboot-tool's own SCSI layer (scsiio), as its jldevfind.py looks for it"""

    def __init__(self, jl):
        sys.path.insert(0, str(jl))
        from scsiio import SCSIDev                       # noqa: E402 (jl-uboot-tool's)
        self.SCSIDev = SCSIDev

    def inquiry(self, path):
        with self.SCSIDev(path) as dev:
            buf = bytearray(36)
            dev.execute(b"\x12\x00\x00" + len(buf).to_bytes(2, "big") + b"\x00", None, buf)
        text = lambda b: bytes(b).decode("ascii", "replace").strip()   # noqa: E731
        return text(buf[8:16]), text(buf[16:32])

    def find(self):
        """-> (path, "WL82 UBOOT1.00") of the keyboard's UBOOT disk, or (None, denied: was a disk refused)"""
        denied = False
        for path in scsi_paths():
            try:
                vendor, product = self.inquiry(path)
            except PermissionError:
                denied = True
                continue
            except Exception:
                continue
            if vendor.lower() == "wl82" and product.startswith(("UBOOT", "UDISK", "DEVICE")):
                return path, f"{vendor} {product}"
        return None, denied

    def wait(self, secs):
        end = time.monotonic() + secs
        while True:
            path, info = self.find()
            if path or time.monotonic() > end:
                return path, info
            time.sleep(DELAY["poll"])


def find_jl(given):
    """jl-uboot-tool's folder: --jl, or a folder beside this file (a GitHub download unpacks as
    jl-uboot-tool-master/, sometimes inside another of that name)"""
    if given:
        cands = [Path(given)]
    else:
        cands = [p for p in sorted(HERE.glob("jl-uboot-tool*")) if p.is_dir()]
        cands += [q for p in cands for q in sorted(p.glob("jl-uboot-tool*")) if q.is_dir()]
    for c in cands:
        if (c / "jluboottool.py").exists():
            return c.resolve()
    raise Stop("setup", "jl-uboot-tool not found: download it (https://github.com/kagaimiq/jl-uboot-tool, "
                        "Code > Download ZIP) and unpack it into the folder this file is in, or give --jl FOLDER")


def read_flash(jl, device, name, out_dir, say):
    """jl-uboot-tool reads the whole flash into out_dir/name; -> the flash ID it printed (or None)"""
    cmd = [sys.executable, str(jl / "jluboottool.py"), "--device", device, "--chip", "wl82",
           f"read 0 {FLASH:#x} {name}"]
    flash_id = None
    p = subprocess.Popen(cmd, cwd=str(out_dir), stdout=subprocess.PIPE, text=True, errors="replace")
    for line in p.stdout:
        say("    " + line.rstrip())
        m = re.search(r"\bID:\s*0x([0-9a-f]+)", line, re.I)
        if m:
            flash_id = int(m[1], 16)
    p.wait()
    return flash_id


# ------------------------------------------------------------------ MIDI ---

def to_uboot(up, product, image, out, ask, yes):
    """the keyboard over USB MIDI -> the UBOOT disk (step 1)"""
    dev = up.find()
    if not dev:
        names = up.backend.input_names()
        raise Stop("notfound", "the keyboard was not found over USB MIDI (switched on? a data cable? another app "
                               f"using it?)\n  MIDI inputs: {', '.join(names) if names else '(none)'}")
    print(f"keyboard: {dev.id.text}  ({dev.name})", file=out)
    if dev.id.open or I.model_of(dev.id.text) != I.model_of(product):
        dev.link.close()
        raise Stop("model", f"the keyboard is {dev.id.text}, the package is for {product}: use your model's package")
    custom = dev.id.version >= CUSTOM
    if dev.id.loader and not custom:
        dev.link.close()
        raise Stop("stopped", "the keyboard is in M-VAVE's own update mode: finish that update first")
    if not custom:
        print("It runs its factory firmware. To read its flash, LOOM-8's update makes its first step only: the\n"
              "factory firmware checks LOOM-8's package and starts LOOM-8's update loader. The firmware itself is\n"
              "not written. Afterwards the keyboard starts in LOOM-8's update mode until an install finishes\n"
              "(fm1_install.py with the same package).", file=out)
    else:
        print("It goes into its boot ROM (UBOOT) for the copy; switch it off and on after.", file=out)
    if not yes and not ask("Go on? Do not unplug the keyboard. [y/N] "):
        dev.link.close()
        raise Stop("cancelled", "cancelled: nothing was changed")
    target = dev
    if not dev.id.loader and not custom:
        prog = I.Progress(out)
        try:
            target = up.check(dev, image, prog)
        finally:
            prog.end()
        if target.id.version < CUSTOM or I.model_of(target.id.text) != I.model_of(product):
            target.link.close()
            raise Stop("stopped", f"another update loader came up ({target.id.text}), not LOOM-8's: "
                                  "power-cycle the keyboard and tell whoever helps you")
    print(f"telling {target.id.text} to restart into UBOOT", file=out)
    target.link.send(UBOOT_KEY)
    end = time.monotonic() + DELAY["gone"]
    while not target.link.lost and time.monotonic() < end:
        time.sleep(0.2)
    gone = target.link.lost
    target.link.close()
    if not gone:
        raise Stop("stopped", f"{target.id.text} did not restart into UBOOT (still on USB MIDI)")


# ------------------------------------------------------------------- run ---

def preflight(a, out, midi=True):
    """everything checked before the keyboard is touched; -> (product, image, jl, out_dir)"""
    product, image, _ = I.load_package(a.package, False)
    if I.model_of(product) not in SMK_MODELS:
        raise Stop("badpkg", f"{a.package} is for {product}: this backup is for the SMK-37 (loom-smk37*.fwsc)")
    jl = find_jl(a.jl)
    missing = [m for m in (*(("mido", "rtmidi") if midi else ()), *JL_MODULES) if importlib.util.find_spec(m) is None]
    if missing:
        names = {"rtmidi": "python-rtmidi", "yaml": "pyyaml"}
        raise Stop("setup", "missing Python packages: " + " ".join(names.get(m, m) for m in missing) +
                            "\n  (pip install mido python-rtmidi pyyaml tqdm crcmod; on Windows with Python 3.12, "
                            "for which python-rtmidi installs ready-made)")
    scsi_paths()
    if not is_admin():
        raise Stop("setup", "reading the keyboard's disk needs an administrator terminal (Windows: right-click "
                            "Terminal or PowerShell > Run as administrator; Linux: sudo)")
    out_dir = Path(a.out).resolve() if a.out else HERE
    try:
        out_dir.mkdir(parents=True, exist_ok=True)
        probe = out_dir / ".smk37-backup-probe"
        probe.write_bytes(b"x")
        probe.unlink()
    except OSError as e:
        raise Stop("setup", f"cannot write to {out_dir}: {e.strerror}")
    print(f"package: {product}\njl-uboot-tool: {jl}\nsaving to: {out_dir}", file=out)
    return product, image, jl, out_dir


def backup(a, backend, out, ask, disks=None, reader=read_flash):
    product, image, jl, out_dir = preflight(a, out, midi=isinstance(backend, I.MidoBackend))
    disks = disks or Disks(jl)
    path, info = disks.find()
    if path:
        print(f"the keyboard is a UBOOT disk already ({info} at {path})", file=out)
    else:
        to_uboot(I.Updater(backend, a.port), product, image, out, ask, a.yes)
        print("waiting for the UBOOT disk (WL82 UBOOT1.00). If Windows offers to format a disk: Cancel.", file=out)
        path, info = disks.wait(DELAY["disk"])
        if not path:
            raise Stop("nodisk", "the UBOOT disk did not appear" +
                       (" or was refused: run this as administrator" if info else
                        ". Unplug and replug the USB cable and run this again (it starts at the disk)"))
        print(f"found {info} at {path}", file=out)
    stamp = datetime.datetime.now().strftime("%Y-%m-%d-%H%M")
    reads = [f"smk37-read-{stamp}-{k}.bin" for k in (1, 2)]
    ids = []
    for k, name in enumerate(reads, 1):
        print(f"reading the flash, copy {k} of 2", file=out, flush=True)
        if k == 2:
            again, _ = disks.wait(DELAY["disk"])
            path = again or path
        ids.append(reader(jl, path, name, out_dir, lambda s: print(s, file=out, flush=True)))
    datas = [(out_dir / n).read_bytes() if (out_dir / n).exists() else b"" for n in reads]
    digest, errs = check_copies(datas)
    if errs:
        raise Stop("badread", "the copies are not good: " + "; ".join(errs) +
                              f"\n  (left in {out_dir}: {', '.join(reads)}). Run this again: it starts at the disk")
    final = out_dir / f"smk37-backup-{stamp}.bin"
    (out_dir / reads[0]).replace(final)
    (out_dir / reads[1]).unlink()
    fid = next((f"{i:#08x}" for i in ids if i), "not shown")
    (out_dir / f"smk37-backup-{stamp}.sha256.txt").write_text(
        f"{digest}  {final.name}\n# {product} package, flash ID {fid}, {FLASH} bytes, two reads the same, "
        f"\"trimdat\" at {TRIM_AT:#x}\n")
    print(f"\nbackup OK: {final}\n  SHA-256 {digest}  (flash ID {fid})\n"
          "Keep it in two places (cloud storage and a USB stick, say).\n"
          "Now switch the keyboard off and on with its power switch.", file=out)
    return 0


def check_files(files, out):
    datas = []
    for f in files:
        try:
            datas.append(Path(f).read_bytes())
        except OSError as e:
            raise Stop("usage", f"cannot read {f}: {e.strerror}")
    digest, errs = check_copies(datas)
    if errs:
        print("NOT GOOD: " + "; ".join(errs), file=out)
        return EXIT["badread"]
    print(f"good: {len(files)} cop{'y' if len(files) == 1 else 'ies'}, SHA-256 {digest}", file=out)
    return 0


def main(argv=None, backend=None, out=sys.stdout, ask=I.ask_tty, disks=None, reader=read_flash):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("package", nargs="?", help="LOOM-8's package for your model (loom-smk37*.fwsc)")
    ap.add_argument("--check", nargs="+", metavar="FILE", help="check one or two copies you have")
    ap.add_argument("--jl", metavar="DIR", help="jl-uboot-tool's folder (default: one beside this file)")
    ap.add_argument("--out", metavar="DIR", help="where the copy goes (default: this file's folder)")
    ap.add_argument("--port", metavar="NAME", help="MIDI port to use (part of its name)")
    ap.add_argument("--yes", action="store_true", help="do not ask")
    a = ap.parse_args(argv)
    if bool(a.check) == bool(a.package):
        ap.error("give LOOM-8's package (loom-smk37.fwsc), or --check FILE")
    try:
        if a.check:
            return check_files(a.check, out)
        if backend is None:
            try:
                backend = I.MidoBackend()
            except ImportError:
                raise Stop("setup", "needs mido and python-rtmidi (pip install mido python-rtmidi)")
        return backup(a, backend, out, ask, disks, reader)
    except I.InstallError as e:
        print(f"error: {e}", file=sys.stderr)
        return EXIT.get(e.code, 1)
    except KeyboardInterrupt:
        print("\ninterrupted: switch the keyboard off and on, and run this again", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
