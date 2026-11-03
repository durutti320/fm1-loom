#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Cam Durnsford (the LOOM-8 SMK-37 port)
"""The SMK-37's last resort: its flash put back as smk37_backup.py saved it, through the chip's boot ROM (UBOOT).

For a keyboard that no longer starts, or no longer answers its installer (fm1_install.py). Anything that still
answers fm1_install.py is better served by it: install the stock package or LOOM-8's again. This is for when that
cannot work.

    py -3.12 smk37_rescue.py smk37-backup-DATE.bin            check: reads the flash, says what differs, writes nothing
    py -3.12 smk37_rescue.py smk37-backup-DATE.bin --write    then puts the differing sectors back

Windows, in a terminal opened as administrator (as the backup); Linux: sudo. The same setup as the backup:
jl-uboot-tool unpacked beside this file, and smk37_backup.py, fm1_install.py and fm1ports.py beside it.

1. The keyboard as a UBOOT disk (WL82 UBOOT1.00). If it is one already, that is used. If LOOM-8 (or its update
   loader) still answers over USB MIDI, it is told to restart into UBOOT (the soft key). Otherwise this waits for
   the disk while you switch the keyboard off and on: the chip's boot ROM takes over when the firmware cannot start
   (on the FM-1, after two failed starts). If Windows offers to format a disk: Cancel.
2. The backup is checked: 1 MiB, the calibration mark ("trimdat" at 0xC1224), and its .sha256.txt if beside it.
3. jl-uboot-tool reads the whole flash. It is compared with the backup, 4 KiB sector by sector:
   - the boot head [0, 0x4000) (the chip's bootloader) is never written: if it differs, nothing is written;
   - the calibration sector: if the keyboard has its mark and the sector differs, the backup is another keyboard's
     and nothing is written.
4. Check (the default): the differing sectors are listed. Nothing is written.
   --write: they are written back (jl-uboot-tool's write: each range erased, then written), the whole flash read
   again and compared; any sector still different is tried once more. It ends with "restored" only when the whole
   flash equals the backup.
5. Switch the keyboard off and on. It starts as it was when the backup was made: the factory firmware, or LOOM-8's
   update mode if the backup was made on the way to the first install (then: fm1_install.py with the stock
   package, or with LOOM-8's).

Exit codes: 0 done (check: the flash read; write: restored), 1 cancelled or failed, 2 bad arguments or setup,
3 nothing found, 5 the UBOOT disk not found or not readable, 7 a read or the write failed its checks, 8 refused
(the boot head differs, or the backup is another keyboard's).
"""
import argparse
import hashlib
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import smk37_backup as SB  # noqa: E402

I = SB.I
FLASH, SECTOR, HEAD = SB.FLASH, 0x1000, 0x4000
CAL = SB.TRIM_AT & ~(SECTOR - 1)            # the calibration's sector (0xC1000)
DELAY = {"wait": 300.0}                     # how long the disk is waited for while the keyboard is switched off and on
EXIT = dict(SB.EXIT, refused=8, badwrite=7)


class Stop(SB.Stop):
    pass


def differing(cur, want):
    """the 4 KiB sectors (their offsets) where the flash read differs from the backup"""
    return [o for o in range(0, FLASH, SECTOR) if cur[o:o + SECTOR] != want[o:o + SECTOR]]


def ranges(secs):
    """sector offsets -> [(start, length)], neighbours joined"""
    out = []
    for o in secs:
        if out and out[-1][0] + out[-1][1] == o:
            out[-1] = (out[-1][0], out[-1][1] + SECTOR)
        else:
            out.append((o, SECTOR))
    return out


def plan(cur, want):
    """-> the sectors to write; Stop if writing would touch what is never written"""
    secs = differing(cur, want)
    head = [o for o in secs if o < HEAD]
    if head:
        raise Stop("refused", f"the keyboard's boot head differs from the backup's (at {head[0]:#x}): this tool never "
                              "writes it, so it writes nothing. Keep both files and ask for help")
    if CAL in secs and cur[SB.TRIM_AT:SB.TRIM_AT + len(SB.TRIM)] == SB.TRIM:
        raise Stop("refused", "the keyboard's calibration differs from the backup's: the backup is another keyboard's "
                              "(or another time's). Nothing is written. Use this keyboard's own backup")
    return secs


def load_backup(path, out):
    try:
        data = Path(path).read_bytes()
    except OSError as e:
        raise Stop("usage", f"cannot read {path}: {e.strerror}")
    errs = SB.problems(data)
    if errs:
        raise Stop("badread", f"{path} is not a good backup: " + "; ".join(errs))
    digest = hashlib.sha256(data).hexdigest()
    note = Path(path).with_suffix(".sha256.txt")
    if note.exists():
        if digest not in note.read_text(errors="replace"):
            raise Stop("badread", f"{path} does not match {note.name}: the file has changed since the backup")
        print(f"backup: {path} (SHA-256 matches {note.name})", file=out)
    else:
        print(f"backup: {path} (SHA-256 {digest}; no .sha256.txt beside it to check against)", file=out)
    return data


def jl_run(jl, device, cmds, cwd, say):
    """jl-uboot-tool with these commands, one run (its loader started once); -> its output"""
    import subprocess
    lines = []
    p = subprocess.Popen([sys.executable, str(jl / "jluboottool.py"), "--device", device, "--chip", "wl82", *cmds],
                         cwd=str(cwd), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
    for line in p.stdout:
        lines.append(line.rstrip())
        say("    " + line.rstrip())
    p.wait()
    return lines


def to_disk(a, backend, disks, out, ask):
    """the keyboard as a UBOOT disk: already, by the soft key, or by switching it off and on"""
    path, info = disks.find()
    if path:
        print(f"the keyboard is a UBOOT disk already ({info} at {path})", file=out)
        return path
    dev = None
    if backend is not None:
        up = I.Updater(backend, a.port)
        try:
            dev = up.find()
        except I.InstallError:
            dev = None
    if dev and dev.id.version >= SB.CUSTOM and I.model_of(dev.id.text) in SB.SMK_MODELS:
        print(f"keyboard: {dev.id.text} ({dev.name}) answers: telling it to restart into UBOOT", file=out)
        dev.link.send(SB.UBOOT_KEY)
        dev.link.close()
    elif dev:
        dev.link.close()
        raise Stop("stopped", f"the keyboard answers as {dev.id.text}: it works, so use fm1_install.py (the stock "
                              "package, or LOOM-8's) instead of this")
    else:
        print("The keyboard does not answer over USB MIDI. Switch it off, wait 5 seconds, switch it on; if no disk\n"
              "appears, do it once more (the chip's boot ROM takes over when the firmware cannot start).\n"
              "If Windows offers to format a disk: Cancel.", file=out)
    print("waiting for the UBOOT disk (WL82 UBOOT1.00) ...", file=out, flush=True)
    path, info = disks.wait(DELAY["wait"] if not dev else SB.DELAY["disk"])
    if not path:
        raise Stop("nodisk", "the UBOOT disk did not appear" +
                   (" or was refused: run this as administrator" if info else ". Ask for help before trying more"))
    print(f"found {info} at {path}", file=out)
    return path


def read_all(jl, disks, path, work, name, out, run):
    again, _ = disks.wait(SB.DELAY["disk"])
    path = again or path
    run(jl, path, [f"read 0 {FLASH:#x} {name}"], work, lambda s: print(s, file=out, flush=True))
    f = work / name
    data = f.read_bytes() if f.exists() else b""
    if len(data) != FLASH or data.count(0xFF) == len(data):
        raise Stop("badread", f"the flash read was not good ({len(data)} bytes): nothing more was done")
    return path, data


def rescue(a, backend, out, ask, disks=None, run=jl_run):
    want = load_backup(a.backup, out)
    jl = SB.find_jl(a.jl)
    missing = [m for m in SB.JL_MODULES if SB.importlib.util.find_spec(m) is None]
    if missing:
        raise Stop("setup", "missing Python packages: pip install pyyaml tqdm crcmod")
    SB.scsi_paths()
    if not SB.is_admin():
        raise Stop("setup", "the keyboard's disk needs an administrator terminal (Windows: Run as administrator)")
    disks = disks or SB.Disks(jl)
    path = to_disk(a, backend, disks, out, ask)
    with tempfile.TemporaryDirectory(prefix="smk37-rescue-") as tmp:
        work = Path(tmp)
        print("reading the flash", file=out, flush=True)
        path, cur = read_all(jl, disks, path, work, "now.bin", out, run)
        secs = plan(cur, want)
        if not secs:
            print("\nthe keyboard's flash is the same as the backup: nothing to put back", file=out)
            return 0
        rs = ranges(secs)
        print(f"\n{len(secs)} sectors differ from the backup ({len(secs) * SECTOR // 1024} KB), in {len(rs)} "
              "range(s): " + ", ".join(f"{s:#x}..{s + n - 1:#x}" for s, n in rs[:12]) + (" ..." if len(rs) > 12 else ""),
              file=out)
        if not a.write:
            print("check only: nothing was written. To put them back: the same command with --write", file=out)
            return 0
        if not a.yes and not ask("Write them back now? Do not unplug or switch off until it says so. [y/N] "):
            raise Stop("cancelled", "cancelled: nothing was written")
        for attempt in (1, 2):
            cmds = []
            for s, n in rs:
                name = f"put-{s:06x}.bin"
                (work / name).write_bytes(want[s:s + n])
                cmds.append(f"write {s:#x} {name}")
            cmds.append(f"read 0 {FLASH:#x} after{attempt}.bin")
            print(f"writing {len(rs)} range(s), then reading the whole flash back (try {attempt} of 2)",
                  file=out, flush=True)
            again, _ = disks.wait(SB.DELAY["disk"])
            path = again or path
            run(jl, path, cmds, work, lambda s: print(s, file=out, flush=True))
            f = work / f"after{attempt}.bin"
            cur = f.read_bytes() if f.exists() else b""
            if len(cur) != FLASH:
                raise Stop("badread", "the read after the write was not good: switch nothing off; run this again "
                                      "(it starts at the disk) and ask for help")
            left = differing(cur, want)
            if not left:
                print("\nrestored: the whole flash equals the backup.\nSwitch the keyboard off and on with its "
                      "power switch.", file=out)
                return 0
            if any(o < HEAD for o in left):
                raise Stop("badwrite", "the boot head changed during the write: switch nothing off and ask for help")
            rs = ranges(left)
            print(f"{len(left)} sectors still differ", file=out)
        raise Stop("badread", f"{len(left)} sectors still differ after two tries: switch nothing off; run this again "
                              "and ask for help")


def main(argv=None, backend=None, out=sys.stdout, ask=I.ask_tty, disks=None, run=jl_run):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("backup", help="the keyboard's backup (smk37-backup-DATE.bin, from smk37_backup.py)")
    ap.add_argument("--write", action="store_true", help="put the differing sectors back (default: check only)")
    ap.add_argument("--jl", metavar="DIR", help="jl-uboot-tool's folder (default: one beside this file)")
    ap.add_argument("--port", metavar="NAME", help="MIDI port to use (part of its name)")
    ap.add_argument("--yes", action="store_true", help="do not ask")
    a = ap.parse_args(argv)
    try:
        if backend is None:
            try:
                backend = I.MidoBackend()
            except ImportError:
                backend = None                          # (no MIDI: the disk only)
        return rescue(a, backend, out, ask, disks, run)
    except I.InstallError as e:
        print(f"error: {e}", file=sys.stderr)
        return EXIT.get(e.code, 1)
    except KeyboardInterrupt:
        print("\ninterrupted: if it was writing, do not switch off: run it again (it starts at the disk)",
              file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
