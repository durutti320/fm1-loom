#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Cam Durnsford (the LOOM-8 SMK-37 port)
"""An ELF32's functions by address (its symbol table, no tools for its CPU needed): one line each, "SYM ADDR SIZE NAME",
sorted. For looking a crash's pc and rets up (fm1_remote.py log) against the build that crashed.
    elf_funcs.py FILE.elf"""
import struct
import sys


def funcs(data):
    if data[:4] != b"\x7fELF" or data[4] != 1:
        raise SystemExit("not an ELF32 file")
    e = "<" if data[5] == 1 else ">"
    shoff, = struct.unpack_from(e + "I", data, 0x20)
    shentsize, shnum = struct.unpack_from(e + "HH", data, 0x2E)
    secs = [struct.unpack_from(e + "IIIIIIIIII", data, shoff + i * shentsize) for i in range(shnum)]
    out = []
    for s in secs:
        if s[1] != 2:                                   # SHT_SYMTAB
            continue
        strtab = secs[s[6]]
        for k in range(s[5] // 16):
            name, value, size, info, _, _ = struct.unpack_from(e + "IIIBBH", data, s[4] + 16 * k)
            if info & 15 != 2:                          # STT_FUNC
                continue
            o = strtab[4] + name
            n = data[o:data.index(b"\0", o)].decode("latin-1")
            out.append((value, size, n))
    return sorted(out)


if __name__ == "__main__":
    for v, sz, n in funcs(open(sys.argv[1], "rb").read()):
        print(f"SYM {v:08x} {sz:6d} {n}")
