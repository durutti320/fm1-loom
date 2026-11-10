// SPDX-License-Identifier: GPL-3.0-only
// SPIDER's projects page (app/web/projects/index.html: the block between "projects: begin" and "end", on fm1-base's
// backup protocol as tools/make_spider_projects.py inlines it) against SPIDER's simulator (BOARD=smk37):
//   1. a backup the simulator made of two saved projects reads as two slots: their names, tempos, sizes
//   2. rename, swap, clear, a project file and the library's text: each round trip keeps every byte
//   3. the image the page builds is one fm1-base's parse takes, and the simulator restores it (restore_rc 0): after
//      a restart it opens the swapped project, and its own next backup holds the slots as the page sent them
//   4. damaged and foreign files are refused in plain words, before anything is sent
//   node tests/host/projects_web.mjs app/build/host/spider-smk37_host [PYTHON]
import { execFileSync } from "node:child_process";
import { mkdtempSync, readFileSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

const HOST = process.argv[2], PY = process.argv[3] || process.env.PYTHON || "python3";
if (!HOST) { console.error("usage: node tests/host/projects_web.mjs HOST [PYTHON]"); process.exit(2); }
const page = readFileSync("app/web/projects/index.html", "utf8");
const proto = execFileSync(PY, ["tools/make_spider_projects.py", "--protocol"], { encoding: "utf8", maxBuffer: 1 << 24 });
const logic = page.slice(page.indexOf("// --- projects: begin"), page.indexOf("// --- projects: end"));
const { PB, SP } = new Function(proto + "\n" + logic + "\nreturn { PB, SP };")();

let bad = 0;
const check = (what, ok) => { console.log(`  ${ok ? "ok  " : "FAIL"} ${what}`); if (!ok) bad++; };
const same = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
const throws = (fn) => { try { fn(); return null; } catch (e) { return e; } };
const dir = mkdtempSync(join(tmpdir(), "spider-proj-"));
const SAVE = ["wait 300", "tap HOME", "spin K1 20", "wait 40", "expect tempo 140", "projsave 1 ALPHA", "spin K1 -45", "wait 40",
              "expect tempo 95", "projsave 2 BETA_TWO", "wait 100"];
function run(name, lines) {
  writeFileSync(join(dir, name), lines.join("\n") + "\n");
  let out = "";
  try { out = execFileSync(HOST, [join(dir, name), dir], { encoding: "utf8", maxBuffer: 1 << 24 }); }
  catch (e) { out = String(e.stdout || "") + String(e.stderr || "") + "\nFAIL: the simulator's exit"; }
  return out;
}

console.log("projects page:");
// 1. the simulator's backup as the page reads it
let out = run("a.xy", [...SAVE, "backup out:a.bkp"]);
check("the simulator saved two projects and backed up", !/FAIL/.test(out));
const imgA = new Uint8Array(readFileSync(join(dir, "a.bkp")));
const st = SP.fromBackup(PB.parse(imgA).objs);
const i0 = SP.info(st.slots[0]), i1 = SP.info(st.slots[1]);
check("slot 1: ALPHA, 140 BPM", i0.ok && i0.name === "ALPHA" && i0.bpm === 140 && i0.bytes > 58);
check("slot 2: BETA TWO, 95 BPM, saved last", i1.ok && i1.name === "BETA TWO" && i1.bpm === 95 && i1.count > i0.count);
check("the image built from it again is the same file", same(SP.toBackup(st, PB.MAGIC, PB.VERSION, PB.crc32), imgA));

// 2. the page's own changes
const file = SP.toFile(st.slots[0], PB.crc32);
check("a project file round trip keeps every byte", SP.sameSlot(SP.fromFile(file, PB.crc32), st.slots[0]));
check("the library's text round trip", same(SP.fromB64(SP.toB64(file)), file));
const ren = SP.rename(st.slots[0], "  Dub Sirën 12345678  ");
check("a name: printable ASCII, 11 characters", SP.info(ren).name === "Dub Sirn 12" && SP.info(st.slots[0]).name === "ALPHA");
check("a rename changes the name's 12 bytes only", ren[0].every((v, i) => (i >= 8 && i < 20) || v === st.slots[0][0][i]) &&
      ren.slice(1).every((d, k) => same(d, st.slots[0][k + 1])));
check("an empty slot: nothing saved, no name to give", SP.info(SP.emptySlot()).empty && SP.isEmpty(SP.rename(SP.emptySlot(), "x")));
check("file names: safe characters only", SP.fileName("a/b:c*", "x") === "abc.spiderproj" && SP.fileName("", "slot") === "slot.spiderproj");

// 4. what is refused
const cut = file.slice(0, file.length - 9), flip = Uint8Array.from(file);
flip[20] ^= 1;
check("a cut project file: refused", throws(() => SP.fromFile(cut, PB.crc32))?.code === "file");
check("a changed byte: refused", throws(() => SP.fromFile(flip, PB.crc32))?.code === "file");
check("a backup file is not a project file", throws(() => SP.fromFile(imgA, PB.crc32))?.code === "file");
check("an empty project's file: refused", throws(() => SP.fromFile(SP.toFile(SP.emptySlot(), PB.crc32), PB.crc32))?.code === "file");
check("another layout's backup: refused", throws(() => SP.fromBackup(PB.parse(imgA).objs.slice(0, 8)))?.code === "file");
check("an object too large for SPIDER: refused", throws(() => SP.toBackup({ presets: st.presets,
      slots: [[new Uint8Array(3841), ...st.slots[0].slice(1)], st.slots[1]] }, PB.MAGIC, PB.VERSION, PB.crc32))?.code === "file");

// 3. swapped and renamed, restored by the simulator
const want = { presets: st.presets, slots: [SP.rename(st.slots[1], "Night Bus"), SP.fromFile(file, PB.crc32)] };
const imgB = SP.toBackup(want, PB.MAGIC, PB.VERSION, PB.crc32);
check("the page's image is one fm1-base's parse takes", !throws(() => PB.parse(imgB)));
writeFileSync(join(dir, "b.bkp"), imgB);
out = run("b.xy", [...SAVE, "restore out:b.bkp", "expect restore_rc 0", "wait 100", "reboot", "wait 300", "expect tempo 95",
                   "backup out:c.bkp"]);
check("the simulator restores it and opens the swapped project (95 BPM) after a restart", !/FAIL/.test(out) && /ok tempo == 95/.test(out));
const got = SP.fromBackup(PB.parse(new Uint8Array(readFileSync(join(dir, "c.bkp")))).objs);
const g0 = SP.info(got.slots[0]), g1 = SP.info(got.slots[1]);
check("its next backup: slot 1 Night Bus 95 BPM, slot 2 ALPHA 140 BPM", g0.name === "Night Bus" && g0.bpm === 95 && g1.name === "ALPHA" && g1.bpm === 140);
check("the patterns' objects as sent, byte for byte", [0, 1].every((s) => got.slots[s].slice(1).every((d, k) => same(d, want.slots[s][k + 1]))));
check("the presets untouched", same(got.presets, st.presets));
// a cleared slot
const imgD = SP.toBackup({ presets: st.presets, slots: [SP.emptySlot(), st.slots[0]] }, PB.MAGIC, PB.VERSION, PB.crc32);
writeFileSync(join(dir, "d.bkp"), imgD);
out = run("d.xy", [...SAVE, "restore out:d.bkp", "expect restore_rc 0", "wait 100", "backup out:e.bkp"]);
const gd = SP.fromBackup(PB.parse(new Uint8Array(readFileSync(join(dir, "e.bkp")))).objs);
check("a cleared slot restores as empty, the other as sent", !/FAIL/.test(out) && SP.isEmpty(gd.slots[0]) && SP.info(gd.slots[1]).name === "ALPHA");

console.log(bad ? `projects page: ${bad} FAILED` : "projects page: all passed");
process.exit(bad ? 1 : 0);
