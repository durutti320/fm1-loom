/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (SPIDER, the LOOM-8 SMK-37 port) */
/* LOOM-8 on the M-VAVE SMK-37 (fm1-base BOARD=smk37, plat.h PLAT_SMK37): what the SMK's panel adds to the FM-1's. Part
 * of app.c's build after ui.c (its helpers); ui.c calls lsmk_poll at the start of input(), takes lsmk_pad_keys into its
 * keys, plays KEY_VEL, draws lsmk_lights after leds(), and hands every frame to lsmk_daw_frame first (DAW mode). The
 * FM-1's controls keep their jobs (fm1-base's panel map: K1 the volume, K2 SELECT, K3 ALGORITHM, K4 PRESETS, K5..K8
 * KNOB 1..4; the middle 27 keys, with velocity).
 *
 *   pads        pads 1-8 (the top row) mute and unmute T1-T8, lit in the track's ink while it plays, dark when
 *               muted; pads 9-16 (the bottom row) play T1-T8, one pad a track, with velocity (a synth its C, a kit its
 *               first key), recorded while recording. On Steps and Bar (SEQ) the bar's 16 cells instead, pads 1-8 the
 *               top row: the white keys' job there, every gesture of theirs (a tap, a hold, a later one for the
 *               length); lit in the track's colour: a step with notes dim, the step playing bright, a step held white
 *   > held      the pads fire the throws, pads 1-12 the 12 of them on the whole mix, while held (13-16 dark)
 *   KNOB BANK   on Steps: the cells <-> mutes and tracks (lit); elsewhere opens Steps, the pads its cells
 *   < >         the bar before / after (any page: the one Steps shows and the pads write), when let go (> not if
 *               it fired a throw)
 *   < + >       DAW mode (below)
 *   faders      the levels of T1-T4, or T5-T8 (FADER BANK, lit on T5-T8); a fader acts once it moves
 *   outer keys  C3..E3 and G#5..C6 carry on the keys: more notes, with velocity; on a kit, its keys
 *   pitch wheel bend of the track the keys play (+-2 semitones, as a MIDI bend)
 *   mod wheel   the filter of the track the keys play (its cutoff; DYNAMIC: its hi cut), once the wheel passes the
 *               value it has (a pick-up: nothing jumps)
 *
 * DAW mode (< + > together; ported from Snoop37 by way of the Loom FM-1 port; smk_daw.h has the map): the SMK is a
 * USB MIDI controller and LOOM-8 is left alone (the sequencer stops on the way in; MIDI IN still plays it, a sound
 * module for the DAW). The keys notes on CHANNEL (OCT- / OCT+ the octave, both: 0); K1..K8 (KNOB BANK: A / B), the
 * faders (FADER BANK: A / B), the pads (KNOB BANK + FADER BANK together: A / B), 13 buttons and the wheels what the map
 * says. K1 is a knob here, not the volume. < and > send when let go. PATCH tapped: LOCAL (the keys, NOTE pads and
 * the pitch wheel also play LOOM-8). PATCH held: K5 CHANNEL, K6 CC MAP (DEFAULT / USER), K7 KNOBS (ABSOLUTE /
 * RELATIVE); + OCT+ the map editor: touch a control, K5 its TYPE, K6 its NUMBER, K7 its CHANNEL; OCT+ the next
 * control, OCT- or PATCH done. Kept in flash (OBJ_DAW): the mode, the settings, the USER map. */

static struct {
    uint32_t latch;                     /* Steps: pads pressed and not yet seen by input() (a tap within a frame) */
    uint8_t pad_trk[NPADS], pad_note[NPADS];   /* a track pad's note playing: its track and note (0 none) */
    uint8_t pad_fx[NPADS];              /* a throw pad's throw firing: its throw + 1 (0 none) */
    uint8_t last_kit;                   /* the kit DAW mode's LOCAL pads play when a synth is selected (0xFF: none) */
    uint8_t pad_drums;                  /* KNOB BANK: the pads mutes and tracks on Steps too */
    uint8_t gt_used;                    /* > held fired a throw (its let-go steps no bar) */
    uint8_t fbank;                      /* the faders: 0 T1-T4, 1 T5-T8 */
    uint8_t flast[4];                   /* (soft takeover: 0xFF waits for the fader to move) */
    uint8_t out_trk[NOUTER], out_note[NOUTER];   /* an outer key's note playing: track and note (0 none) */
    uint8_t bend_trk;                   /* the track the pitch wheel bends (0xFF none) */
    int32_t bend;
    uint8_t mw_trk, mw_par, mw_last, mw_armed;   /* the mod wheel: its track and parameter, as last seen, picked up */
    uint8_t lr_both;                    /* < + > pressed together: neither steps a bar (until both are up) */
    uint32_t sbtn;                      /* the SMK buttons as last seen */
} lsmk = {.last_kit = 0xFF, .flast = {0xFF, 0xFF, 0xFF, 0xFF}, .bend_trk = 0xFF, .mw_trk = 0xFF};

static void lsmk_daw_set(int on);

static int lsmk_pads_steps(void)
{
    return steps_page(ui.page) && is_seqt(sel_trk()) && !ui.ask && !lsmk.pad_drums;
}
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

/* > held (alone): the pads are the throws */
static int lsmk_throws(void) { return (plat_smk_buttons() >> SB_RIGHT & 1u) && !lsmk.lr_both; }
static void lsmk_pads(void)
{
    int p, v;
    while (plat_pad(&p, &v)) {
        uint32_t m;
        if (p < 0 || p >= NPADS)
            continue;
        m = 1u << p;
        if (v) {
            if (lsmk_throws()) {                   /* a throw: on the whole mix (both its groups), while held */
                lsmk.gt_used = 1;
                if (p < NPUNCH && !lsmk.pad_fx[p]) {
                    lsmk.pad_fx[p] = (uint8_t)(p + 1);
                    engine_punch(p, 1, -1);
                    engine_punch(p + PX_MEL, 1, -1);
                    say("THROW", PUNCH_NAME[p]);
                }
                continue;
            }
            if (lsmk_pads_steps()) {               /* a cell: its white key (lsmk_pad_keys) */
                lsmk.latch |= m;
                continue;
            }
            if (p < 8) {                           /* the top row: T1-T8's mutes */
                if (p < NSEQT) {
                    set_mute(p, !(seq.mute >> p & 1u));
                    say(seq.mute >> p & 1u ? "MUTED" : "UNMUTED", TRACK_NAME[p]);
                }
            } else {                               /* the bottom row: T1-T8 played, a pad each */
                int t = p - 8, n = is_drum(t) ? 1 : clampi(KEY_BASE + 7 + ui.oct * 12, 1, 127);
                if (lsmk.pad_note[p])
                    engine_key(lsmk.pad_trk[p], lsmk.pad_note[p], 0, 0);
                lsmk.pad_trk[p] = (uint8_t)t;
                lsmk.pad_note[p] = (uint8_t)n;
                engine_key(t, n, 1, v);
            }
        } else {                                   /* let go: what it started ends, whatever the layer is now */
            if (lsmk.pad_fx[p]) {
                engine_punch(lsmk.pad_fx[p] - 1, 0, -1);
                engine_punch(lsmk.pad_fx[p] - 1 + PX_MEL, 0, -1);
                lsmk.pad_fx[p] = 0;
            }
            if (lsmk.pad_note[p]) {
                engine_key(lsmk.pad_trk[p], lsmk.pad_note[p], 0, 0);
                lsmk.pad_note[p] = 0;
            }
        }
    }
}
/* the cells held (and pressed since the last pass) as their white keys: input() takes them with the keys */
static uint32_t lsmk_pad_keys(void)
{
    uint32_t held = plat_pads_held() | lsmk.latch, k = 0;
    int c;
    lsmk.latch = 0;
    if (!lsmk_pads_steps() || lsmk_throws())
        return 0;                                  /* (> held: the pads are throws, on Steps too) */
    for (c = 0; c < 16; c++)
        if (held >> c & 1u && !lsmk.pad_fx[c])
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

/* < >: the bar before / after, of the track selected (the one Steps shows and the pads write) */
static void lsmk_bar(int d)
{
    char s[8];
    if (!is_seqt(sel_trk())) {
        say_no("NO BARS", "this track has no steps");
        return;
    }
    ui.bar = (uint8_t)clampi(ui.bar + d, 0, NBAR - 1);   /* (as OCT- / OCT+ on Steps) */
    num_s((uint32_t)ui.bar + 1u, s);
    say("BAR", s);
}
/* KNOB BANK: the pads' job. On Steps, cells <-> drums; elsewhere the Steps page, its cells on the pads */
static void lsmk_pad_mode(void)
{
    lsmk.latch = 0;
    if (ui.ask || lst.open)
        return;
    if (lsmk_pads_steps()) {
        lsmk.pad_drums = 1;
        say("PADS", "mutes and tracks");
        return;
    }
    lsmk.pad_drums = 0;
    if (!(steps_page(ui.page) && is_seqt(sel_trk())))
        page_button(B_SEQ);
    if (lsmk_pads_steps())
        say("PADS", "the bar's steps");
    else
        say_no("NO STEPS", "this track has none");
}
/* the SMK's own four buttons: FADER BANK, KNOB BANK when pressed; < > when let go (unless both: DAW mode) */
static void lsmk_buttons(void)
{
    uint32_t b = plat_smk_buttons(), pressed = b & ~lsmk.sbtn, released = lsmk.sbtn & ~b, lr = 1u << SB_LEFT | 1u << SB_RIGHT;
    int f;
    lsmk.sbtn = b;
    if ((pressed & lr) && (b & lr) == lr) {        /* < + >: DAW mode */
        lsmk.lr_both = 1;
        lsmk_daw_set(1);
        return;
    }
    if (released & lr) {
        if (!lsmk.lr_both && !((released >> SB_RIGHT & 1u) && lsmk.gt_used))
            lsmk_bar(released >> SB_RIGHT & 1u ? 1 : -1);
        if (released >> SB_RIGHT & 1u)
            lsmk.gt_used = 0;
        if (!(b & lr))
            lsmk.lr_both = 0;
    }
    if (pressed >> SB_KNOB_BANK & 1u)
        lsmk_pad_mode();
    if (pressed >> SB_FADER_BANK & 1u) {
        lsmk.fbank ^= 1u;
        for (f = 0; f < 4; f++)
            lsmk.flast[f] = 0xFF;                  /* (the other bank: each fader waits to move) */
        say("FADERS", lsmk.fbank ? "T5 to T8" : "T1 to T4");
    }
}

static void lsmk_faders(void)
{
    int f;
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
            char s[16];
            set_tp(t, TP_LEVEL, in->lo + (int)((in->hi - in->lo) * v + 63u) / 127);
            s_cat(num_s(v * 100u / 127u, s), "% level");
            say(TRACK_NAME[t], s);
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

/* the mod wheel: the filter of the track the keys play, its cutoff (DYNAMIC: its hi cut). A pick-up: the wheel acts
 * once it passes the value the track has (another track, or another filter type: picked up again) */
static void lsmk_modw(void)
{
    int t = lsmk_play_trk(), p, span;
    uint32_t m = plat_modw(), cur;
    const tp_info_t *in;
    char s[24];
    if (t < 0 || t >= NTRK)
        return;
    p = proj.trk[t].par[TP_FTYPE] % NFT == FT_DYNAMIC ? TP_KTRK : TP_CUT;
    if (t != lsmk.mw_trk || p != lsmk.mw_par) {
        lsmk.mw_trk = (uint8_t)t;
        lsmk.mw_par = (uint8_t)p;
        lsmk.mw_last = (uint8_t)m;
        lsmk.mw_armed = 0;
        return;
    }
    if (m == lsmk.mw_last)
        return;
    in = tp_info_par(p, proj.trk[t].par);
    span = in->hi - in->lo;
    if (span <= 0)
        return;
    cur = (uint32_t)(clampi(proj.trk[t].par[p], in->lo, in->hi) - in->lo) * 127u / (uint32_t)span;
    if (!lsmk.mw_armed && ((lsmk.mw_last <= cur && m >= cur) || (lsmk.mw_last >= cur && m <= cur)))
        lsmk.mw_armed = 1;
    lsmk.mw_last = (uint8_t)m;
    if (!lsmk.mw_armed) {
        s_cat(num_s(cur * 100u / 127u, s_cat(s, "pick up at ")), "%");
        say("FILTER", s);
        return;
    }
    set_tp(t, p, in->lo + (int)((uint32_t)span * m + 63u) / 127);
    s_cat(num_s(m * 100u / 127u, s_cat(s, p == TP_KTRK ? "hi cut " : "cutoff ")), "%");
    say(TRACK_NAME[t], s);
}

/* every input pass, first */
static void lsmk_poll(void)
{
    lsmk_kit();                                    /* (the kit selected last, kept) */
    lsmk_pads();
    lsmk_outer();
    lsmk_buttons();
    if (daw_on)                                    /* (< + >: DAW mode, from the next frame) */
        return;
    lsmk_faders();
    lsmk_wheels();
    lsmk_modw();
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
    if (lsmk_throws()) {                       /* the throws (on every page): dim violet, bright while firing */
        for (c = 0; c < NPADS; c++)
            plat_pad_rgb(c, c >= NPUNCH ? 0u : lsmk.pad_fx[c] ? 0xF0E0FFu : lsmk_rgb(theme->ink[6], 60u));
    } else if (lsmk_pads_steps()) {
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
    } else {                                   /* mutes (lit while the track plays) and the tracks' pads */
        uint32_t held = plat_pads_held();
        for (c = 0; c < 8; c++)
            plat_pad_rgb(c, lsmk_rgb(trk_ink(c), seq.mute >> c & 1u ? 10u : 200u));
        for (c = 8; c < NPADS; c++)
            plat_pad_rgb(c, lsmk_rgb(trk_ink(c - 8), held >> c & 1u ? 256u : 44u));
    }
    plat_smk_leds((lsmk.fbank ? 1u << SB_FADER_BANK : 0u) | (lsmk.pad_drums ? 1u << SB_KNOB_BANK : 0u) |
                  (lsmk_throws() ? 1u << SB_RIGHT : 0u));
}

/* ======================================================================================================= DAW mode */

/* the 13 buttons in the map's order (smk_daw.h DAW_BTN_NAME): < > the SMK's own, the rest the FM-1's as printed */
#define LSMK_LEFT (-1)
#define LSMK_RIGHT (-2)
static const int8_t LSMK_DAW_BTN[DAW_NBTN] = {LSMK_LEFT, LSMK_RIGHT, B_PLAY, B_REC, B_SEQ, B_FX, B_EDIT, B_ARP, B_LFO,
                                               B_SEL, B_ENV, B_GLO, B_SAVE};
#define LSMK_NKEY (NOUTER + NKEYS)             /* the keybed's 37: key 0 C3 .. 36 C6 */
#define LSMK_CV_PITCH 24u                      /* the continuous controls: the knobs' and faders' entries, the wheels */
#define LSMK_CV_MOD 25u
#define LSMK_NCV 26u

static struct {
    uint8_t loaded, dirty;
    uint32_t dirty_t;                   /* the last change (the settings are saved a while after it) */
    uint8_t kb, fb, pb;                 /* the banks: knobs, faders, pads (0 A, 1 B) */
    uint8_t bank_both;                  /* KNOB BANK + FADER BANK pressed together: the pads' bank, neither other */
    uint8_t lr_both;                    /* < + > pressed together: neither sends (until both are up) */
    int8_t oct;                         /* the keys' octave, -3..3 */
    uint8_t edit;                       /* the map editor: 0 off, 1 picking a control, 2 editing it */
    uint8_t esel;                       /* the editor's control */
    uint8_t sel;                        /* the control last touched: an entry, DAW_KEYS, 0xFF none */
    uint8_t sel_bank;
    uint16_t sel_v;                     /* its value (the pitch wheel: 0..16383; the keys: the note) */
    uint8_t patch_used;                 /* PATCH held: something else done meanwhile (no LOCAL toggle) */
    uint32_t btn, sbtn, keys;           /* as last seen */
    uint8_t key_note[LSMK_NKEY];        /* the note a key sent (0xFF none) */
    uint8_t loc_trk[LSMK_NKEY], loc_note[LSMK_NKEY];   /* LOCAL: the note a key plays on LOOM-8 (0 none) */
    uint8_t pad_ent[NPADS];             /* the entry a pad pressed sent (its release: the same) */
    uint8_t pad_loc[NPADS], pad_loc_trk[NPADS];   /* LOCAL: a NOTE pad's kit key + 1 (0 none) */
    uint8_t kv[16];                     /* ABSOLUTE: each knob's value, per bank */
    uint8_t fseen[4], mseen;
    int32_t bseen;
    daw_cv_t cv[LSMK_NCV];
    uint8_t tog[DAW_N];                 /* CC TOGGLE: on */
    uint32_t tog_t[DAW_N];
    uint32_t sig[5];                    /* the screen's bands as drawn (0: draw) */
} ld;

static int lsmk_daw_ok(void) { return ld.edit == 0; }   /* the controls send (the editor: they pick) */
static void lsmk_daw_tx(uint32_t i, uint32_t v)        /* entry i's message at v */
{
    daw_ent_t e = daw_ent(i);
    uint32_t p[2], n = daw_msg(&e, v, p), k;
    for (k = 0; k < n; k++)
        plat_midi_out(p[k]);
}
static void lsmk_daw_touch(uint32_t i, uint32_t v)
{
    ld.sel = (uint8_t)i;
    ld.sel_v = (uint16_t)v;
    ld.sel_bank = (uint8_t)(i < DAW_N ? daw_bank_of(i) : 0u);
    if (ld.edit == 1u && i < DAW_N) {              /* the editor: this control */
        ld.esel = (uint8_t)i;
        ld.edit = 2;
        daw_user = 1;                              /* (the USER map, in use) */
        ld.dirty = 1;
    }
}
static void lsmk_daw_dirty(void)
{
    ld.dirty = 1;
    ld.dirty_t = plat_ms();
}

/* ---- flash: OBJ_DAW */
static daw_store_t lsmk_daw_obj;                /* (loading and saving: one buffer) */
static void lsmk_daw_load(void)
{
    daw_store_t *s = &lsmk_daw_obj;
    int n;
    daw_reset_user();
    n = plat_store_load(OBJ_DAW, s, sizeof *s);
    if (n > 0)
        (void)daw_store_take((const uint8_t *)s, (uint32_t)n);
}
static void lsmk_daw_save(void)                /* a while after the last change, with nothing sounding or held (a
                                                 * flash write cuts the audio: as autosave, stopped and quiet) */
{
    if (!ld.dirty || ld.edit || plat_ms() - ld.dirty_t < 1500u || playing() || store_held || eng_voices ||
        eng_quiet < AUTOSAVE_QUIET || plat_keys() || plat_pads_held())
        return;
    daw_store_fill(&lsmk_daw_obj);
    if (plat_store_save(OBJ_DAW, &lsmk_daw_obj, sizeof lsmk_daw_obj) == 0)
        ld.dirty = 0;
    else
        ld.dirty_t = plat_ms();                    /* (a failed or held write: later) */
}

/* ---- the keys: notes on CHANNEL; LOCAL: the track the keys play too */
static void lsmk_daw_key(int k, int vel)        /* k 0..36 (C3..C6), vel 0: let go */
{
    if (k < 0 || k >= LSMK_NKEY)
        return;
    if (ld.key_note[k] != 0xFFu) {                 /* (its note before: off) */
        plat_midi_out(daw_note_pkt(daw_ch - 1u, ld.key_note[k], 0));
        ld.key_note[k] = 0xFF;
    }
    if (ld.loc_note[k]) {
        engine_key(ld.loc_trk[k], ld.loc_note[k], 0, 0);
        ld.loc_note[k] = 0;
    }
    if (!vel || !lsmk_daw_ok())
        return;
    {
        int n = (int)DAW_NOTE0 + k + ld.oct * 12, t = lsmk_play_trk();
        if (n < 0 || n > 127)
            return;
        ld.key_note[k] = (uint8_t)n;
        plat_midi_out(daw_note_pkt(daw_ch - 1u, (uint32_t)n, (uint32_t)vel));
        lsmk_daw_touch(DAW_KEYS, (uint32_t)n);
        if (daw_local && t >= 0 && t < NTRK) {
            int ln = is_drum(t) ? k % DL_NSOUND + 1 : n;
            ld.loc_trk[k] = (uint8_t)t;
            ld.loc_note[k] = (uint8_t)ln;
            engine_key(t, ln, 1, vel);
        }
    }
}
static void lsmk_daw_keys(void)
{
    uint32_t keys = plat_keys(), ch = keys ^ ld.keys;
    int k, i, v;
    ld.keys = keys;
    for (k = 0; k < NKEYS; k++)
        if (ch >> k & 1u)
            lsmk_daw_key(k + 5, keys >> k & 1u ? (int)plat_key_vel(k) : 0);
    while (plat_outer(&i, &v))
        if (i >= 0 && i < NOUTER)
            lsmk_daw_key(i < 5 ? i : NKEYS + i, v);
}

/* ---- the pads and buttons: a press (vel) and a let-go (0) */
static void lsmk_daw_hit(uint32_t i, uint32_t vel)
{
    daw_ent_t e = daw_ent(i);
    if (vel)
        lsmk_daw_touch(i, vel);
    if (!lsmk_daw_ok())
        return;
    switch (e.type) {
    case DT_TOG:
        if (vel && daw_toggle(&ld.tog[i], &ld.tog_t[i], plat_ms()))
            lsmk_daw_tx(i, ld.tog[i] ? 127u : 0u);
        break;
    case DT_MMC:
        if (vel)
            lsmk_daw_tx(i, 127u);
        break;
    case DT_CC:
        lsmk_daw_tx(i, vel ? 127u : 0u);
        break;
    case DT_NOTE:
        lsmk_daw_tx(i, vel);
        break;
    default:
        break;
    }
}
static void lsmk_daw_pads(void)
{
    int p, v;
    while (plat_pad(&p, &v)) {
        uint32_t i;
        if (p < 0 || p >= NPADS)
            continue;
        i = v ? DAW_PAD0 + 16u * ld.pb + (uint32_t)p : ld.pad_ent[p];
        if (v)
            ld.pad_ent[p] = (uint8_t)i;
        lsmk_daw_hit(i, (uint32_t)v);
        if (ld.pad_loc[p]) {                       /* LOCAL: its kit key off */
            engine_key(ld.pad_loc_trk[p], ld.pad_loc[p], 0, 0);
            ld.pad_loc[p] = 0;
        }
        if (v && daw_local && lsmk_daw_ok() && daw_ent(i).type == DT_NOTE && p < DL_NSOUND) {   /* LOCAL: the kit */
            int kit = lsmk_kit();
            if (kit >= 0) {
                ld.pad_loc_trk[p] = (uint8_t)kit;
                ld.pad_loc[p] = (uint8_t)(p + 1);
                engine_key(kit, p + 1, 1, v);
            }
        }
    }
}

/* ---- the continuous controls: the value to send, out at most every DAW_RATE_MS (the last one always) */
static void lsmk_daw_cv(uint32_t c, uint32_t i, int32_t v)
{
    ld.cv[c].v = v;
    lsmk_daw_touch(i, (uint32_t)v);
}
static void lsmk_daw_knob(uint32_t k, int32_t s)  /* knob k (0..7: K1..K8) turned s detents */
{
    uint32_t i = DAW_KNOB0 + 8u * ld.kb + k;
    if (!s)
        return;
    if (ld.edit == 1u) {
        lsmk_daw_touch(i, ld.kv[i]);
        return;
    }
    if (ld.edit)
        return;
    if (daw_rel) {
        ld.cv[i].acc += s;
        lsmk_daw_touch(i, (uint32_t)(64 + clampi(ld.cv[i].acc, -63, 63)));
    } else
        lsmk_daw_cv(i, i, (int32_t)daw_knob_val(&ld.kv[i], s, 0));
}
static void lsmk_daw_flush(void)
{
    uint32_t c, now = plat_ms() | 1u;
    for (c = 0; c < LSMK_NCV; c++) {
        uint32_t i = c == LSMK_CV_PITCH ? DAW_PITCH : c == LSMK_CV_MOD ? DAW_MOD : c, rel = c < 16u && daw_rel;
        daw_cv_t *v = &ld.cv[c];
        if (!daw_cv_due(v, now, rel) || plat_midi_out_room(TO_USB) < 4u)
            continue;
        if (rel) {
            lsmk_daw_tx(i, (uint32_t)(64 + clampi(v->acc, -63, 63)));
            v->acc = 0;
        } else {
            lsmk_daw_tx(i, (uint32_t)v->v);
            v->sent = v->v;
        }
        v->t = now;
    }
}
static void lsmk_daw_analog(void)               /* the faders and the wheels */
{
    uint32_t f, m = plat_modw();
    int32_t b = plat_bend();
    for (f = 0; f < 4u; f++) {
        uint32_t v = plat_fader((int)f), i = DAW_FADER0 + 4u * ld.fb + f;
        if (v == ld.fseen[f])
            continue;
        ld.fseen[f] = (uint8_t)v;
        if (!ld.edit)
            lsmk_daw_cv(i, i, (int32_t)v);
        else if (ld.edit == 1u)
            lsmk_daw_touch(i, v);                  /* (the editor: picked, nothing sent) */
    }
    if (b != ld.bseen) {
        uint32_t v14 = (uint32_t)clampi(b + 8192, 0, 16383);
        daw_ent_t e = daw_ent(DAW_PITCH);
        int moved = b > 2048 || b < -2048 || ld.bseen > 2048 || ld.bseen < -2048;
        ld.bseen = b;
        if (!ld.edit)
            lsmk_daw_cv(LSMK_CV_PITCH, DAW_PITCH, (int32_t)(e.type == DT_BEND ? v14 : v14 >> 7));
        else if (ld.edit == 1u && moved)
            lsmk_daw_touch(DAW_PITCH, e.type == DT_BEND ? v14 : v14 >> 7);   /* (the editor: moved from rest) */
        if (daw_local && lsmk_daw_ok()) {
            int t = lsmk_play_trk();
            if (t >= 0 && t < NTRK)
                engine_bend(t, b);
        }
    }
    if (m != ld.mseen) {
        int moved = m > 24u || ld.mseen > 24u;
        ld.mseen = (uint8_t)m;
        if (!ld.edit)
            lsmk_daw_cv(LSMK_CV_MOD, DAW_MOD, (int32_t)m);
        else if (ld.edit == 1u && moved)
            lsmk_daw_touch(DAW_MOD, m);
    }
}

/* the map editor's knobs: K5 TYPE, K6 NUMBER, K7 CHANNEL (0, 1, 2) */
static void lsmk_daw_edit_knob(uint32_t r, int32_t s)
{
    daw_ent_t *e = &daw_umap[ld.esel % DAW_N];
    if (r == 0u)
        daw_set_type(ld.esel, e, daw_type_step(ld.esel, e->type, s));
    else if (r == 1u) {
        int32_t lo = e->type == DT_MMC ? 1 : 0, hi = e->type == DT_MMC ? (int32_t)DAW_MMC_N - 1 : 127;
        e->num = (uint8_t)clampi((int32_t)e->num + s, lo, hi);
    } else
        e->ch = (uint8_t)clampi((int32_t)e->ch + s, 0, 16);
    lsmk_daw_dirty();
}

/* ---- the buttons, the knobs */
static void lsmk_daw_controls(void)
{
    uint32_t btn = plat_buttons(), pr = btn & ~ld.btn, rl = ld.btn & ~btn;
    uint32_t sb = plat_smk_buttons(), spr = sb & ~ld.sbtn, srl = ld.sbtn & ~sb;
    uint32_t lr = 1u << SB_LEFT | 1u << SB_RIGHT, banks = 1u << SB_KNOB_BANK | 1u << SB_FADER_BANK;
    uint32_t oct = 1u << B_OCTDN | 1u << B_OCTUP, patch = btn >> B_HOME & 1u, b;
    int32_t s[8];
    int k;
    ld.btn = btn;
    ld.sbtn = sb;
    /* < + >: back to LOOM-8 */
    if ((spr & lr) && (sb & lr) == lr) {
        ld.lr_both = 1;
        lsmk_daw_set(0);
        return;
    }
    /* the knobs: K1 its own (plat_k1_raw), K2..K4 SELECT ALGORITHM PRESETS, K5..K8 KNOB 1..4 */
    s[0] = plat_k1_enc();
    for (k = 1; k < 8; k++)
        s[k] = plat_enc(k - 1);
    /* PATCH: tapped LOCAL; held, the settings (K5 CHANNEL, K6 CC MAP, K7 KNOBS) and OCT+ the map editor */
    if (pr >> B_HOME & 1u)
        ld.patch_used = 0;
    if (patch) {
        if (s[4] || s[5] || s[6] || (pr & oct))
            ld.patch_used = 1;
        if (s[4])
            daw_ch = (uint8_t)clampi(daw_ch + s[4], 1, 16);
        if (s[5])
            daw_user = (uint8_t)(s[5] > 0);
        if (s[6])
            daw_rel = (uint8_t)(s[6] > 0);
        if (s[4] || s[5] || s[6])
            lsmk_daw_dirty();
        if (pr >> B_OCTUP & 1u) {
            ld.edit = 1;
            lsmk_daw_dirty();
        }
        return;                                    /* (the rest: not while PATCH is held) */
    }
    if (rl >> B_HOME & 1u && !ld.patch_used) {
        if (ld.edit)
            ld.edit = 0;                           /* (the editor: done) */
        else
            daw_local = (uint8_t)!daw_local;
        lsmk_daw_dirty();
    }
    /* the banks: KNOB BANK, FADER BANK when let go; both together the pads' */
    if ((spr & banks) && (sb & banks) == banks) {
        ld.bank_both = 1;
        ld.pb ^= 1u;
    }
    if (srl & banks) {
        if (!ld.bank_both) {
            if (srl >> SB_KNOB_BANK & 1u)
                ld.kb ^= 1u;
            if (srl >> SB_FADER_BANK & 1u) {
                ld.fb ^= 1u;
                for (k = 0; k < 4; k++)
                    ld.fseen[k] = (uint8_t)plat_fader(k);   /* (the other bank: a fader sends once it moves) */
            }
        }
        if (!(sb & banks))
            ld.bank_both = 0;
    }
    /* the map editor: OCT+ the next control, OCT- done; the knobs K5..K7 edit, every other control picks */
    if (ld.edit) {
        if (pr >> B_OCTDN & 1u) {
            ld.edit = 0;
            lsmk_daw_dirty();
            return;
        }
        if (pr >> B_OCTUP & 1u)
            ld.edit = 1;
        for (b = 0; b < DAW_NBTN; b++) {
            int id = LSMK_DAW_BTN[b];
            if (id >= 0 ? pr >> id & 1u : spr >> (id == LSMK_LEFT ? SB_LEFT : SB_RIGHT) & 1u)
                lsmk_daw_touch(DAW_BTN0 + b, 127u);
        }
        for (k = 0; k < 8; k++)
            if (s[k] && ld.edit == 2u && k >= 4 && k <= 6)
                lsmk_daw_edit_knob((uint32_t)(k - 4), s[k]);
            else
                lsmk_daw_knob((uint32_t)k, s[k]);
        return;
    }
    /* OCT- / OCT+: the keys' octave (both: 0) */
    if (pr & oct)
        ld.oct = (btn & oct) == oct ? 0 : (int8_t)clampi(ld.oct + (pr >> B_OCTUP & 1u ? 1 : -1), -3, 3);
    /* the 13 buttons; < and > when let go, unless both were down */
    for (b = 0; b < DAW_NBTN; b++) {
        int id = LSMK_DAW_BTN[b];
        uint32_t i = DAW_BTN0 + b;
        if (id >= 0) {
            if (pr >> id & 1u)
                lsmk_daw_hit(i, 127u);
            else if (rl >> id & 1u)
                lsmk_daw_hit(i, 0);
        } else if (srl >> (id == LSMK_LEFT ? SB_LEFT : SB_RIGHT) & 1u && !ld.lr_both) {
            lsmk_daw_hit(i, 127u);
            lsmk_daw_hit(i, 0);
        }
    }
    if (!(sb & lr))
        ld.lr_both = 0;
    for (k = 0; k < 8; k++)
        lsmk_daw_knob((uint32_t)k, s[k]);
}

/* ---- the lights: the pads by kind (NOTE bank A blue, B green; CC yellow; a CC TOGGLE bright while on; OFF faint),
 * bright while held; the buttons held, LOCAL on PATCH, a toggle's while on, the octave on OCT- / OCT+, bank B on
 * KNOB BANK / FADER BANK; the editor: its control */
static void lsmk_daw_lights(void)
{
    uint32_t held = plat_pads_held(), lit = ld.btn, b, p;
    for (p = 0; p < NPADS; p++) {
        uint32_t i = DAW_PAD0 + 16u * ld.pb + p, c, lv = held >> p & 1u ? 256u : 48u;
        daw_ent_t e = daw_ent(i);
        c = e.type == DT_NOTE ? (ld.pb ? 0x00E060u : 0x2060FFu) : e.type == DT_OFF ? 0x101010u : 0xE0B000u;
        if (e.type == DT_TOG && ld.tog[i])
            lv = 256u;
        if (ld.edit == 2u && ld.esel == i)
            c = 0xFFFFFFu, lv = 256u;
        plat_pad_rgb((int)p, (((c >> 16) & 255u) * lv >> 8) << 16 | (((c >> 8) & 255u) * lv >> 8) << 8 |
                                 ((c & 255u) * lv >> 8));
    }
    if (daw_local || (ld.edit && (plat_ms() >> 8 & 1u)))
        lit |= 1u << B_HOME;                       /* (the editor: PATCH blinks) */
    lit &= ~(1u << B_OCTDN | 1u << B_OCTUP);
    lit |= (ld.oct < 0 ? 1u << B_OCTDN : 0u) | (ld.oct > 0 ? 1u << B_OCTUP : 0u);
    for (b = 2; b < DAW_NBTN; b++)
        if (ld.tog[DAW_BTN0 + b] && daw_ent(DAW_BTN0 + b).type == DT_TOG)
            lit |= 1u << LSMK_DAW_BTN[b];
    plat_lights(lit, 0, 0, 0, 0);
    plat_smk_leds((ld.kb ? 1u << SB_KNOB_BANK : 0u) | (ld.fb ? 1u << SB_FADER_BANK : 0u));
}

/* ---- the screen: DAW, CHANNEL, LOCAL, the map; the last control (its message, value); the banks and octave; the
 * eight knobs as bars; what PATCH does. The editor: its control, TYPE, NUMBER, CHANNEL */
static char *lsmk_daw_msg_s(uint32_t i, char *b)   /* "CC 70  CH 1" */
{
    daw_ent_t e = daw_ent(i);
    char *p = b;
    if (i == DAW_KEYS)
        p = num_s(ld.sel_v, s_cat(b, "NOTE "));
    else
        switch (e.type) {
        case DT_CC: p = num_s(e.num, s_cat(b, "CC ")); break;
        case DT_TOG: p = num_s(e.num, s_cat(b, "CC TOGGLE ")); break;
        case DT_NOTE: p = num_s(e.num, s_cat(b, "NOTE ")); break;
        case DT_MMC: p = s_cat(s_cat(b, "MMC "), DAW_MMC_NAME[e.num % DAW_MMC_N]); break;
        case DT_BEND: p = s_cat(b, "PITCH BEND"); break;
        default: return s_cat(b, "OFF"), b;
        }
    num_s(i == DAW_KEYS ? daw_ch : daw_chan(&e) + 1u, s_cat(p, "  CH "));
    return b;
}
static void lsmk_daw_name(uint32_t i, char *b)
{
    if (i == DAW_KEYS) {
        s_cat(b, "KEYS");
        return;
    }
    daw_name(i, b);
    while (*b)
        b++;
    if (daw_class(i) <= DC_PAD)
        s_cat(b, daw_bank_of(i) ? "  B" : "  A");
}
static void lsmk_daw_draw(void)
{
    char t[40], u[40];
    uint32_t h, k;
    /* band 0, 0..39: the mode */
    h = hash_u(hash_u(hash_u(hash_u(2166136261u, daw_ch), daw_local), daw_user | daw_rel << 1), ld.edit);
    if (h != ld.sig[0]) {
        ld.sig[0] = h;
        cv_begin(240, 40, K_BG);
        cv_text(12, cap_y(0, 40, &FONT_M), &FONT_M, ld.edit ? "MAP" : "DAW", K_TEXT);
        num_s(daw_ch, s_cat(t, "CH "));
        cv_text(84, 4, &FONT_S, t, K_TEXT);
        cv_text(84, 21, &FONT_XS, daw_local ? "LOCAL ON" : "LOCAL OFF", daw_local ? K_TEXT : K_DIM);
        text_r(228, 4, &FONT_S, daw_user ? "USER MAP" : "DEFAULT MAP", K_TEXT);
        text_r(228, 21, &FONT_XS, daw_rel ? "knobs relative" : "knobs absolute", K_DIM);
        cv_rect(0, 39, 240, 1, K_LINE);
        cv_blit(0, 0);
    }
    /* band 1, 40..119: the control last touched, or the editor's */
    if (ld.edit) {
        daw_ent_t e = daw_umap[ld.esel % DAW_N];
        h = hash_u(hash_u(hash_u(hash_u(0x45u, ld.edit), ld.esel), e.type | e.num << 8 | e.ch << 16), daw_ch);
    } else
        h = hash_u(hash_u(hash_u(hash_u(0x4Cu, ld.sel), ld.sel_v), ld.sel_bank), daw_user | daw_ch << 4);
    if (h != ld.sig[1]) {
        ld.sig[1] = h;
        cv_begin(240, 80, K_BG);
        if (ld.edit == 1u) {
            text_c(120, 14, &FONT_B, "Touch a control", K_TEXT);
            text_c(120, 38, &FONT_XS, "a knob, fader, wheel, pad or button", K_DIM);
            text_c(120, 56, &FONT_XS, "OCT- or PATCH: done", K_DIM);
        } else if (ld.edit == 2u) {
            daw_ent_t e = daw_umap[ld.esel % DAW_N];
            lsmk_daw_name(ld.esel, t);
            cv_text(12, 4, &FONT_B, t, K_TEXT);
            s_cat(s_cat(u, "K5 TYPE  "), DT_NAME[e.type % DT_N]);
            cv_text(12, 24, &FONT_S, u, K_TEXT);
            if (e.type == DT_MMC)
                s_cat(s_cat(u, "K6 COMMAND  "), DAW_MMC_NAME[e.num % DAW_MMC_N]);
            else
                num_s(e.num, s_cat(u, "K6 NUMBER  "));
            cv_text(12, 42, &FONT_S, e.type == DT_OFF || e.type == DT_BEND ? "K6 NUMBER  -" : u, K_TEXT);
            if (e.ch)
                num_s(e.ch, s_cat(u, "K7 CHANNEL  "));
            else
                s_cat(num_s(daw_ch, s_cat(u, "K7 CHANNEL  MAIN (")), ")");
            cv_text(12, 60, &FONT_S, u, K_TEXT);
        } else if (ld.sel == 0xFFu) {
            text_c(120, 16, &FONT_B, "A USB MIDI controller", K_TEXT);
            text_c(120, 40, &FONT_XS, APP_NAME " plays what comes in (MIDI IN)", K_DIM);
        } else {
            uint32_t full = ld.sel == DAW_PITCH && daw_ent(DAW_PITCH).type == DT_BEND ? 16383u : 127u, v = ld.sel_v;
            lsmk_daw_name(ld.sel, t);
            cv_text(12, 4, &FONT_B, t, K_TEXT);
            cv_text(12, 26, &FONT_S, lsmk_daw_msg_s(ld.sel, u), K_TEXT);
            num_s(v, t);
            text_r(228, 4, &FONT_B, t, K_TEXT);
            cv_rect(12, 56, 216, 10, K_PANEL);
            cv_rect(12, 56, (int32_t)(216u * (v > full ? full : v) / full), 10, K_TEXT);
        }
        cv_rect(0, 79, 240, 1, K_LINE);
        cv_blit(0, 40);
    }
    /* band 2, 120..151: the banks and the octave */
    h = hash_u(hash_u(0x42u, ld.kb | ld.fb << 1 | ld.pb << 2), (uint32_t)(ld.oct + 8));
    if (h != ld.sig[2]) {
        ld.sig[2] = h;
        cv_begin(240, 32, K_BG);
        cv_text(12, 8, &FONT_XS, ld.kb ? "KNOBS B" : "KNOBS A", K_TEXT);
        cv_text(72, 8, &FONT_XS, ld.fb ? "FADERS B" : "FADERS A", K_TEXT);
        cv_text(138, 8, &FONT_XS, ld.pb ? "PADS B" : "PADS A", K_TEXT);
        num_i(ld.oct, s_cat(t, ld.oct > 0 ? "OCT +" : "OCT "));
        text_r(228, 8, &FONT_XS, t, K_TEXT);
        cv_blit(0, 120);
    }
    /* band 3, 152..211: the eight knobs (this bank), two rows of four: K and the CC, a bar of the value */
    h = hash_u(0x4Bu, ld.kb | daw_user << 1 | daw_rel << 2 | daw_ch << 4);
    for (k = 0; k < 8u; k++) {
        daw_ent_t e = daw_ent(DAW_KNOB0 + 8u * ld.kb + k);
        h = hash_u(h, ld.kv[8u * ld.kb + k] | (uint32_t)e.type << 8 | (uint32_t)e.num << 16);
    }
    if (h != ld.sig[3]) {
        ld.sig[3] = h;
        cv_begin(240, 60, K_BG);
        for (k = 0; k < 8u; k++) {
            uint32_t i = DAW_KNOB0 + 8u * ld.kb + k, x = 12u + (k % 4u) * 56u, y = (k / 4u) * 30u;
            daw_ent_t e = daw_ent(i);
            char *p = num_s(k + 1u, s_cat(t, "K"));
            if (e.type == DT_CC)
                num_s(e.num, s_cat(p, "  "));
            else
                s_cat(p, e.type == DT_OFF ? "  off" : "");
            cv_text((int32_t)x, (int32_t)y, &FONT_XS, t, trk_ink((int)k));   /* (each knob its colour, as on the rest of SPIDER) */
            cv_rect((int32_t)x, (int32_t)y + 18, 48, 5, K_PANEL);
            if (!daw_rel)
                cv_rect((int32_t)x, (int32_t)y + 18, (int32_t)(48u * ld.kv[i] / 127u), 5, trk_ink((int)k));
            else
                cv_rect((int32_t)x + 23, (int32_t)y + 18, 2, 5, trk_ink((int)k));
        }
        cv_blit(0, 152);
    }
    /* band 4, 212..239: what PATCH does */
    h = hash_u(0x50u, (ld.btn >> B_HOME & 1u) | (uint32_t)ld.edit << 1);
    if (h != ld.sig[4]) {
        ld.sig[4] = h;
        cv_begin(240, 28, K_BG);
        cv_rect(0, 0, 240, 1, K_LINE);
        text_c(120, 7, &FONT_XS, ld.btn >> B_HOME & 1u ? "K5 channel  K6 map  K7 knobs  OCT+ edit"
                                 : ld.edit ? "OCT+ next control    OCT- done"
                                           : "< + >  " APP_NAME "        PATCH  local", K_DIM);
        cv_blit(0, 212);
    }
}

/* ---- in and out */
static void lsmk_synth_quiet(void)              /* LOOM-8's side: the pads' notes and throws, outer keys' notes
                                                 * off, the bend 0 */
{
    int i;
    for (i = 0; i < NPADS; i++) {
        if (lsmk.pad_fx[i]) {
            engine_punch(lsmk.pad_fx[i] - 1, 0, -1);
            engine_punch(lsmk.pad_fx[i] - 1 + PX_MEL, 0, -1);
            lsmk.pad_fx[i] = 0;
        }
        if (lsmk.pad_note[i]) {
            engine_key(lsmk.pad_trk[i], lsmk.pad_note[i], 0, 0);
            lsmk.pad_note[i] = 0;
        }
    }
    lsmk.gt_used = 0;
    for (i = 0; i < NOUTER; i++)
        if (lsmk.out_note[i]) {
            engine_key(lsmk.out_trk[i], lsmk.out_note[i], 0, 0);
            lsmk.out_note[i] = 0;
        }
    if (lsmk.bend_trk < NTRK)
        engine_bend(lsmk.bend_trk, 0);
    lsmk.bend_trk = 0xFF;
}
static void lsmk_daw_quiet(void)                /* DAW mode's notes off (keys, pads; LOCAL's too), the bend 0 */
{
    int k, p;
    for (k = 0; k < LSMK_NKEY; k++)
        lsmk_daw_key(k, 0);
    for (p = 0; p < NPADS; p++) {
        if (plat_pads_held() >> p & 1u && daw_ent(ld.pad_ent[p]).type == DT_NOTE)
            lsmk_daw_tx(ld.pad_ent[p], 0);
        if (ld.pad_loc[p]) {
            engine_key(ld.pad_loc_trk[p], ld.pad_loc[p], 0, 0);
            ld.pad_loc[p] = 0;
        }
    }
    for (k = 0; k < NTRK; k++)
        engine_bend(k, 0);
}
static void lsmk_daw_set(int on)
{
    int k, i, v;
    on = on != 0;
    if (on == daw_on)
        return;
    if (on) {
        lsmk_synth_quiet();
        if (playing()) {
            engine_transport(0);
            rec_off();
        }
        ui.ask = 0;
        lst.open = 0;
        for (k = 0; k < LSMK_NKEY; k++)
            ld.key_note[k] = 0xFF;
        ld.btn = plat_buttons();                   /* (held now: not pressed in DAW mode) */
        ld.sbtn = plat_smk_buttons();
        ld.keys = plat_keys();
        for (k = 0; k < 4; k++)
            ld.fseen[k] = (uint8_t)plat_fader(k);
        ld.bseen = plat_bend();
        ld.mseen = (uint8_t)plat_modw();
        ld.lr_both = 1;                            /* (the < + > that got here: neither sends when let go) */
        ld.edit = 0;
        ld.sel = 0xFF;
        while (plat_pad(&i, &v))
            ;
        while (plat_outer(&i, &v))
            ;
        for (k = 0; k < NE; k++)
            (void)plat_enc(k);
        plat_k1_raw(1);
    } else {
        lsmk_daw_quiet();
        ld.edit = 0;
        plat_k1_raw(0);
        for (k = 0; k < NE; k++)
            (void)plat_enc(k);
        while (plat_pad(&i, &v))
            ;
        while (plat_outer(&i, &v))
            ;
        ui.btn = plat_buttons();                   /* (held now: not pressed in LOOM-8) */
        ui.keys = plat_keys();
        lsmk.sbtn = plat_smk_buttons();
        lsmk.lr_both = 1;
        for (k = 0; k < 4; k++)
            lsmk.flast[k] = 0xFF;
        lsmk.mw_trk = 0xFF;
        lsmk.latch = 0;
    }
    daw_on = (uint8_t)on;
    for (k = 0; k < 5; k++)
        ld.sig[k] = 0;
    lcd_fill(0, 0, 240, 240, K_BG);
    ui_redraw();
    lsmk_daw_dirty();
    if (!on)
        say(APP_NAME, "< + > for DAW mode");
}

/* every frame, before LOOM-8's: 1 = DAW mode had it */
static int lsmk_daw_frame(void)
{
    if (!ld.loaded) {                              /* the first frame: the settings, the map; the mode kept */
        ld.loaded = 1;
        lsmk_daw_load();
        if (daw_on) {
            daw_on = 0;
            lsmk_daw_set(1);
            ld.dirty = 0;
        }
    }
    if (!daw_on) {
        lsmk_daw_save();
        return 0;
    }
    lsmk_daw_controls();
    if (!daw_on)                                   /* (< + >: LOOM-8 again, from the next frame) */
        return 1;
    lsmk_daw_keys();
    lsmk_daw_pads();
    lsmk_daw_analog();
    lsmk_daw_flush();
    lsmk_daw_draw();
    lsmk_daw_lights();
    lsmk_daw_save();
    ui.frame++;
    return 1;
}
