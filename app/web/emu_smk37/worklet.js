// SPDX-License-Identifier: GPL-3.0-only
// SPIDER (LOOM-8 on the M-VAVE SMK-37) in an AudioWorklet: app.wasm is the whole firmware against fm1-base's
// simulated SMK-37 (web/emu/emu_web.c, BOARD=smk37). Each render quantum asks it for 128 frames, which runs the
// device's clock forward; between quanta the page's input goes in and, about 30 times a second, the screen, the
// lights, the pads' colours, the MIDI it sent and any saved objects go out. From LOOM-8's worklet.js.

const clock = globalThis.performance ? () => globalThis.performance.now() : () => Date.now();

class Spider extends AudioWorkletProcessor {
  constructor() {
    super();
    this.ex = null;
    this.sentBlits = -1;
    this.sentWrites = -1;
    this.lastFrame = 0;
    this.busy = 0;
    this.frames = 0;
    this.port.onmessage = (e) => this.onMessage(e.data);
  }

  async onMessage(m) {
    if (m.type === "load") {
      try {
        const now = () => BigInt(Math.round(currentTime * 1e9));
        const wasi = {
          clock_time_get: (id, prec, out) => {
            new DataView(this.mem.buffer).setBigUint64(out, now(), true);
            return 0;
          },
          fd_write: (fd, iov, n, out) => { new DataView(this.mem.buffer).setUint32(out, 0, true); return 0; },
          proc_exit: () => {},
        };
        const stub = new Proxy(wasi, { get: (t, k) => (k in t ? t[k] : () => 0) });
        const env = new Proxy({}, { get: () => () => 0 });
        const { instance } = await WebAssembly.instantiate(m.wasm, { wasi_snapshot_preview1: stub, env });
        this.ex = instance.exports;
        this.mem = this.ex.memory;
        if (this.ex._initialize) this.ex._initialize();
        const max = this.ex.web_store_max();
        for (const [obj, bytes] of Object.entries(m.store || {})) {
          const b = new Uint8Array(bytes).subarray(0, max);
          new Uint8Array(this.mem.buffer, this.ex.web_store(+obj), b.length).set(b);
          this.ex.web_store_set_len(+obj, b.length);
        }
        this.sentWrites = this.ex.web_store_writes();
        this.ex.web_master(2048);                       // the SMK starts with its volume at the middle
        this.ex.web_boot();
        if (m.bootShow !== false && this.ex.web_spider_boot) this.ex.web_spider_boot();
        this.port.postMessage({ type: "ready" });
      } catch (err) {
        this.port.postMessage({ type: "error", message: String(err && err.message || err) });
      }
      return;
    }
    const ex = this.ex;
    if (!ex) return;
    switch (m.type) {
      case "buttons": ex.web_buttons(m.mask >>> 0); break;
      case "smk": ex.web_smk_buttons(m.mask >>> 0); break;
      case "key": ex.web_key_vel(m.k, m.vel || 100); ex.web_key(m.k, m.vel, 0); break;
      case "outer": ex.web_outer(m.k, m.vel); break;
      case "pad": ex.web_pad(m.p, m.vel); break;
      case "enc": ex.web_enc(m.role, m.n | 0); break;
      case "k1": ex.web_k1(m.n | 0); break;
      case "fader": ex.web_fader(m.f, m.v); break;
      case "bend": ex.web_bend(m.v | 0); break;
      case "modw": ex.web_modw(m.v); break;
      case "boot": if (ex.web_spider_boot) ex.web_spider_boot(); break;
    }
  }

  publish() {
    const ex = this.ex;
    const blits = ex.web_blits();
    const msg = {
      type: "frame", buttons: ex.web_lit_buttons() >>> 0, smk: ex.web_smk_lit() >>> 0,
      pads: new Uint32Array(this.mem.buffer, ex.web_pad_rgb(), 16).slice(), volume: ex.web_master_pos(),
    };
    if (this.frames >= 44100) {                       // the share of real time spent running the device
      msg.load = Math.round(100 * this.busy / (this.frames / 44.1));
      this.busy = this.frames = 0;
    }
    const transfer = [];
    if (blits !== this.sentBlits) {
      this.sentBlits = blits;
      msg.fb = new Uint16Array(this.mem.buffer, ex.web_fb(), 240 * 240).slice();
      transfer.push(msg.fb.buffer);
    }
    const n = ex.web_out_n();
    if (n) {                                          // the MIDI it sent (USB-MIDI packets)
      msg.midi = new Uint32Array(this.mem.buffer, ex.web_out_pkts(), n).slice();
      ex.web_out_clear();
    }
    const writes = ex.web_store_writes();
    if (writes !== this.sentWrites) {
      this.sentWrites = writes;
      msg.store = {};
      for (let o = 0; o < ex.web_nobj(); o++) {
        const len = ex.web_store_len(o);
        if (len >= 0) msg.store[o] = new Uint8Array(this.mem.buffer, ex.web_store(o), len).slice();
      }
    }
    this.port.postMessage(msg, transfer);
  }

  process(inputs, outputs) {
    if (!this.ex) return true;
    const out = outputs[0];
    const n = out[0].length;
    const t0 = clock();
    this.ex.web_render(n);
    this.busy += clock() - t0;
    this.frames += n;
    out[0].set(new Float32Array(this.mem.buffer, this.ex.web_out_l(), n));
    if (out[1]) out[1].set(new Float32Array(this.mem.buffer, this.ex.web_out_r(), n));
    if (currentTime - this.lastFrame >= 1 / 30) {
      this.lastFrame = currentTime;
      this.publish();
    }
    return true;
  }
}

registerProcessor("spider", Spider);
