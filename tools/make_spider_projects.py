#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""SPIDER's projects page, beside its emulator and its samples page (.github/workflows/loom8-smk37.yml: the spider2-emu
site): app/web/projects/index.html with fm1-base's backup protocol (base/web/backup/index.html, the block between
"backup protocol: begin" and "end", its values filled in as base/tools/make_site.py fills them) inlined at
/*PROTOCOL*/, in SPIDER's words (the SMK-37, its buttons).

  tools/make_spider_projects.py OUT     OUT/projects/index.html
  tools/make_spider_projects.py --protocol     the protocol block alone, to stdout (tests/host/projects_web.mjs)
"""
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "base" / "tools"))
import make_site  # noqa: E402

BEGIN, END = "// --- backup protocol: begin", "// --- backup protocol: end"
# the block's own words for SPIDER on the SMK-37 (each must be there)
SAY = [("HOME was pressed on the FM-1", "PATCH was pressed on the SMK-37"),
       ("Plug it in and press Back up again.", "Plug it in and press Read from SPIDER again."),
       ("Press Back up again.", "Press Read from SPIDER again."),
       ("update it on the Install page.", "install the newest SPIDER build."),
       ("The FM-1", "The SMK-37"), ("the FM-1", "the SMK-37")]


def protocol():
    page = (SRC / "base" / "web" / "backup" / "index.html").read_text(encoding="utf-8")
    if page.count(BEGIN) != 1 or page.count(END) != 1:
        raise SystemExit("make_spider_projects: base/web/backup/index.html has no backup protocol block")
    block = page[page.index(BEGIN):page.index(END) + len(END)]
    conf = make_site.read_conf(SRC / "app" / "fm1.toml")
    vals = make_site.backup_values(SRC / "app", conf)
    # SPIDER on any SMK-37 model: its identity is the model's name and the prefix's digits ("SMK-37 Pro_10..")
    vals.update(NAME="SPIDER", ID="SMK-37", EXT="spider", PORTS_JS=make_site.ports_js())
    for key in ("NAME", "ID", "MAGIC", "BACKUP_VERSION", "EXT", "PORTS_JS"):
        block = block.replace("{{" + key + "}}", vals[key])
    if "{{" in block.replace("${{", ""):
        raise SystemExit("make_spider_projects: a value of the protocol block was not filled in")
    for old, new in SAY:
        if old not in block:
            raise SystemExit(f"make_spider_projects: not in the protocol block: {old!r}")
        block = block.replace(old, new)
    return block


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    if sys.argv[1] == "--protocol":
        sys.stdout.write(protocol())
        return
    page = (SRC / "app" / "web" / "projects" / "index.html").read_text(encoding="utf-8")
    if page.count("/*PROTOCOL*/") != 1:
        raise SystemExit("app/web/projects/index.html must contain /*PROTOCOL*/ once")
    out = Path(sys.argv[1]) / "projects"
    out.mkdir(parents=True, exist_ok=True)
    (out / "index.html").write_text(page.replace("/*PROTOCOL*/", protocol()), encoding="utf-8")
    print(f"spider projects page: {out / 'index.html'}")


if __name__ == "__main__":
    main()
