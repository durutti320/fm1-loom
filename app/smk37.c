/* SPDX-License-Identifier: GPL-3.0-only */
/* LOOM-8 on the M-VAVE SMK-37 (fm1-base BOARD=smk37, plat.h PLAT_SMK37): what the SMK's panel adds to the FM-1's. Part
 * of app.c's build after ui.c (its helpers); ui.c calls lsmk_poll at the start of input(), takes lsmk_pad_keys into its
 * keys, plays KEY_VEL and draws lsmk_lights after leds(). The FM-1's controls keep their jobs (fm1-base's panel map:
 * K1 the volume, K2 SELECT, K3 ALGORITHM, K4 PRESETS, K5..K8 KNOB 1..4; the middle 27 keys, with velocity).
 *
 *   pads        on Steps and Bar (SEQ), the bar's 16 cells, pads 1-8 the top row: the white keys' job there, every
 *               gesture of theirs (a tap, a hold, a later one for the length); lit in the track's colour: a step with
 *               notes dim, the step playing bright, a step held white. On every other page drum pads: pads 1-11 the
 *               11 keys of a kit (the track selected, if it is a kit, else the kit selected last), with velocity,
 *               recorded while recording; 12-16 dark
 *   faders      the levels of T1-T4, or T5-T8 (FADER BANK, lit on T5-T8); a fader acts once it moves
 *   outer keys  C3..E3 and G#5..C6 carry on the keys: more notes, with velocity; on a kit, its keys
 *   pitch wheel bend of the track the keys play (+-2 semitones, as a MIDI bend)
 *   mod wheel, KNOB BANK, < >: not given a job yet (smk37-port.yaml open_questions) */

static struct {
    uint32_t latch;                     /* Steps: pads pressed and not yet seen by input() (a tap within a frame) */
    uint32_t steps_held;                /* Steps: the pads that are cells now (their keys) */
    uint8_t pad_trk[NPADS], pad_note[NPADS];   /* a drum pad's sound playing: its track and key + 1 (0 none) */
    uint8_t last_kit;                   /* the kit the drum pads play when a synth is selected (0xFF: none yet) */
    uint8_t fbank;                      /* the faders: 0 T1-T4, 1 T5-T8 */
    uint8_t flast[4];                   /* (soft takeover: 0xFF waits for the fader to move) */
    uint8_t out_trk[NOUTER], out_note[NOUTER];   /* an outer key's note playing: track and note (0 none) */
    uint8_t bend_trk;                   /* the track the pitch wheel bends (0xFF none) */
    int32_t bend;
    uint32_t sbtn;                      /* the SMK buttons as last seen */
} lsmk = {.last_kit = 0xFF, .flast = {0xFF, 0xFF, 0xFF, 0xFF}, .bend_trk = 0xFF};

static int lsmk_pads_steps(void) { return steps_page(ui.page) && is_seqt(sel_trk()) && !ui.ask; }
/* the kit the drum pads play: the track selected if it is a kit, else the last kit selected, else the first kit */
static int lsmk_kit(void)
{
    int t = sel_trk();
    if (is_drum(t))
        return lsmk.last_kit = (uint8_t)t, t;
    if (lsmk.last_kit < NTRK && is_drum(lsmk.last_kit))
        return lsmk.last_kit;
    for (t = 0; t < NTRK; t++)
        if (is_drum(t))
            return t;
    return -1;
}
/* the track the keys play (FX tracks: the instrument selected last) */
static int lsmk_play_trk(void) { return is_aux(sel_trk()) ? inst_trk() : sel_trk(); }

static void lsmk_pads(void)
{
    int p, v;
    while (plat_pad(&p, &v)) {
        uint32_t m = 1u << p;
        if (v) {
            if (lsmk_pads_steps()) {               /* a cell: its white key (lsmk_pad_keys) */
                lsmk.latch |= m;
                continue;
            }
            if (p < DL_NSOUND) {                  /* a drum pad: the kit's key p + 1 */
                int kit = lsmk_kit();
                if (kit < 0) {
                    say_no("NO KIT", "select a kit's track");
                    continue;
                }
                if (lsmk.pad_note[p])
                    engine_key(lsmk.pad_trk[p], lsmk.pad_note[p], 0, 0);
                lsmk.pad_trk[p] = (uint8_t)kit;
                lsmk.pad_note[p] = (uint8_t)(p + 1);
                engine_key(kit, p + 1, 1, v);
                ui.dsound = (uint8_t)(kit == sel_trk() ? p : ui.dsound);   /* (on the kit's own pages: the key shown) */
            }
        } else if (lsmk.pad_note[p]) {             /* a drum pad let go, whatever the page is now */
            engine_key(lsmk.pad_trk[p], lsmk.pad_note[p], 0, 0);
            lsmk.pad_note[p] = 0;
        }
    }
}
/* the cells held (and pressed since the last pass) as their white keys: input() takes them with the keys */
static uint32_t lsmk_pad_keys(void)
{
    uint32_t held = plat_pads_held() | lsmk.latch, k = 0;
    int c;
    lsmk.latch = 0;
    if (!lsmk_pads_steps())
        return 0;
    for (c = 0; c < 16; c++)
        if (held >> c & 1u)
            k |= 1u << WHITE_K[c];
    return k;
}

static void lsmk_outer(void)
{
    int i, v;
    while (plat_outer(&i, &v)) {
        if (i < 0 || i >= NOUTER)
            continue;
        if (v) {
            int t = lsmk_play_trk(), n;
            if (t < 0 || t >= NTRK || ui.ask)
                continue;
            n = is_drum(t) ? i % DL_NSOUND + 1 : clampi(KEY_BASE + OUTER_NOTE_OFS(i) + ui.oct * 12, 1, 127);
            if (lsmk.out_note[i])
                engine_key(lsmk.out_trk[i], lsmk.out_note[i], 0, 0);
            lsmk.out_trk[i] = (uint8_t)t;
            lsmk.out_note[i] = (uint8_t)n;
            engine_key(t, n, 1, v);
        } else if (lsmk.out_note[i]) {
            engine_key(lsmk.out_trk[i], lsmk.out_note[i], 0, 0);
            lsmk.out_note[i] = 0;
        }
    }
}

static void lsmk_faders(void)
{
    uint32_t b = plat_smk_buttons(), pressed = b & ~lsmk.sbtn;
    int f;
    lsmk.sbtn = b;
    if (pressed >> SB_FADER_BANK & 1u) {
        lsmk.fbank ^= 1u;
        for (f = 0; f < 4; f++)
            lsmk.flast[f] = 0xFF;                  /* (the other bank: each fader waits to move) */
        say("FADERS", lsmk.fbank ? "T5 to T8" : "T1 to T4");
    }
    for (f = 0; f < 4; f++) {
        uint32_t v = plat_fader(f);
        int t = lsmk.fbank * 4 + f;
        if (lsmk.flast[f] == 0xFF) {
            lsmk.flast[f] = (uint8_t)v;
            continue;
        }
        if (v == lsmk.flast[f])
            continue;
        lsmk.flast[f] = (uint8_t)v;
        {
            const tp_info_t *in = tp_info_par(TP_LEVEL, proj.trk[t].par);
            char s[8];
            set_tp(t, TP_LEVEL, in->lo + (int)((in->hi - in->lo) * v + 63u) / 127);
            num_s(v * 100u / 127u, s);
            say(TRACK_NAME[t], s_cat(s, "% level"));
        }
    }
}

static void lsmk_wheels(void)
{
    int t = lsmk_play_trk();
    int32_t b = plat_bend();
    if (t < 0 || t >= NTRK)
        t = 0xFF;
    if (t != lsmk.bend_trk) {                      /* another track: the one before unbent */
        if (lsmk.bend_trk < NTRK)
            engine_bend(lsmk.bend_trk, 0);
        lsmk.bend_trk = (uint8_t)t;
        lsmk.bend = 0x7FFFFFFF;
    }
    if (t < NTRK && b != lsmk.bend) {
        lsmk.bend = b;
        engine_bend(t, b);
    }
}

/* every input pass, first */
static void lsmk_poll(void)
{
    lsmk_kit();                                    /* (the kit selected last, kept) */
    lsmk_pads();
    lsmk_outer();
    lsmk_faders();
    lsmk_wheels();
}

/* the pads' and the SMK buttons' lights, after leds() */
static uint32_t lsmk_rgb(uint16_t c, uint32_t lv)  /* an RGB565 ink at lv / 256 */
{
    uint32_t r = (c >> 11) << 3 | (c >> 13), g = ((c >> 5) & 0x3Fu) << 2 | ((c >> 9) & 3u), b = (c & 0x1Fu) << 3 | ((c >> 2) & 7u);
    return (r * lv >> 8) << 16 | (g * lv >> 8) << 8 | (b * lv >> 8);
}
static void lsmk_lights(void)
{
    int c, t = sel_trk();
    if (lsmk_pads_steps()) {
        const tpat_t *tp = tpat(t);
        uint16_t ink = trk_ink(t);
        int ph = playing() ? eng_step[t] : -1;
        for (c = 0; c < 16; c++) {
            int s = ui.bar * 16 + c;
            uint32_t lv = 0;
            if (s < tp->len) {
                if (ui.held >> c & 1u)
                    lv = 0x1000000u;              /* (white: the step held) */
                else if (s == ph)
                    lv = 256u;
                else if (t != AUX_WEAVE && step_on(&tp->st[s], t))
                    lv = 56u;
            }
            plat_pad_rgb(c, lv == 0x1000000u ? 0xC8C8C8u : lsmk_rgb(ink, lv));
        }
    } else {
        int kit = lsmk_kit();
        uint32_t held = plat_pads_held();
        for (c = 0; c < NPADS; c++)
            plat_pad_rgb(c, kit < 0 || c >= DL_NSOUND ? 0u : lsmk_rgb(trk_ink(kit), held >> c & 1u ? 256u : 40u));
    }
    plat_smk_leds(lsmk.fbank ? 1u << SB_FADER_BANK : 0u);
}
