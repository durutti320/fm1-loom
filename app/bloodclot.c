/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (BLOODCLOT) */
/* BLOODCLOT: a dub box on SPIDER's base (the SMK-37 build of this branch). Its eight tracks are a desk's channels
 * (DRUM BASS SKNK ORGN HORN VOX SIRN HAND); a riddim is built as on SPIDER, then dubbed live from the pads: the top row
 * drops a channel out, the bottom row throws it into the echo while held (PATCH held: into the spring). GLOBE's first
 * page, Mix, is the desk: a speaker and a level a channel, the dropped dark and red, the thrown gold.
 *
 * Milestone 1 (this file): the channels and their starter sounds, a starter riddim in a new project (one drop, a bass
 * line, the skank, the organ's bubble), the desk page, the throws (engine.c engine_throw) and the pads' desk (smk37.c).
 * To come: the siren synth and CHORD, the tape echo and the spring as their own effects, HAND's loop, the boot. */
#if BLOODCLOT

/* ---- a new project's riddim -------------------------------------------------------------------------------------- */

/* a melodic row: a step each (a sixteenth), a note of BC_SCALE by its letter, '.' none, '-' the note before held on */
static const uint8_t BC_BASS_NOTE[8] = {33, 36, 38, 40, 31, 28, 45, 43};   /* a: A1  c: C2  d: D2  e: E2  g: G1  E: E1
                                                                            * A: A2  G: G2 */
static int bc_bass_note(char c)
{
    switch (c) {
    case 'a': return BC_BASS_NOTE[0];
    case 'c': return BC_BASS_NOTE[1];
    case 'd': return BC_BASS_NOTE[2];
    case 'e': return BC_BASS_NOTE[3];
    case 'g': return BC_BASS_NOTE[4];
    case 'E': return BC_BASS_NOTE[5];
    case 'A': return BC_BASS_NOTE[6];
    case 'G': return BC_BASS_NOTE[7];
    default: return 0;
    }
}
/* the bass line, two bars in A minor: the root held, a walk up to the fifth, and home by the seventh below */
static const char BC_BASS[] = "a--a..c.e---d.c."
                              "a--a..g.a-----E.";
static void bc_melodic(tpat_t *tp, const char *row)
{
    int n = 0, i;
    while (row[n])
        n++;
    pattern_init(tp);
    tp->len = (uint8_t)(n > NSTEP ? NSTEP : n);
    for (i = 0; i < tp->len; i++) {
        int note = bc_bass_note(row[i]), l = 1;
        if (!note)
            continue;
        while (i + l < tp->len && row[i + l] == '-')
            l++;
        tp->st[i].note[0] = (uint8_t)note;
        tp->st[i].vel = 108;
        tp->st[i].len = (uint8_t)step_len_code(l * 4 - 1);    /* (a quarter step short: the next note is a new one) */
    }
}
/* a chord (three notes) on the steps of mask (bit s: step s of the bar), every bar alike */
static void bc_chords(tpat_t *tp, uint32_t mask, const uint8_t *chord, int vel, int len_q)
{
    pattern_init(tp);
    tp->len = 16;
    for (int i = 0; i < 16; i++) {
        if (!(mask >> i & 1u))
            continue;
        for (int k = 0; k < 3; k++)
            tp->st[i].note[k] = chord[k];
        tp->st[i].vel = (uint8_t)vel;
        tp->st[i].len = (uint8_t)len_q;
    }
}
/* the kit's one drop (rhythms.h's, on the acoustic kit): nothing on 1, the kick and the rim together on 3, the hat's
 * eighths */
static void bc_drums(tpat_t *tp)
{
    pattern_init(tp);
    tp->len = 16;
    for (int i = 0; i < 16; i += 2) {
        kit_set(&tp->st[i], 1u << 2 | (i == 8 ? 1u << 0 | 1u << 7 : 0u));   /* key 3 the hat, 1 the kick, 8 the rim */
        tp->st[i].vel = (uint8_t)(i == 8 ? 112 : i % 4 ? 96 : 72);          /* (the hat's offbeats the louder) */
    }
}
static void bc_riddim(void)
{
    static const uint8_t AM_LOW[3] = {57, 60, 64}, AM_HIGH[3] = {69, 72, 76};   /* A minor: the skank's, the organ's */
    bc_drums(&pats.pat[0][0]);
    bc_melodic(&pats.pat[1][0], BC_BASS);
    bc_chords(&pats.pat[2][0], 0x4444u, AM_LOW, 104, 2);      /* the skank: every "and", half a step long */
    bc_chords(&pats.pat[3][0], 0xAAAAu, AM_HIGH, 84, 1);      /* the bubble: the sixteenths between, soft and short */
}

/* a new project's channels: their sounds by name (project.c track_sound_new: the kits the acoustic kit), their sends
 * as a dub mix has them at rest (dry: the throws do the sending), the echo and the spring, the riddim, 75 BPM */
static const char *const BC_SOUND[NTRK] = {0, "dub bass", "skank", "bubble", "horn stab", "orch hit", "laser", 0};
static void bc_project(void)
{
    for (int t = 0; t < NTRK; t++) {
        trk_t *tr = &proj.trk[t];
        if (BC_SOUND[t]) {
            int k = preset_named(tr->par[TP_ENGINE], BC_SOUND[t]);
            if (k >= 0) {
                apply_preset(tr, k);
                tr->par[TP_BRAIN] = 1;
                tr->par[TP_CHAN] = 1;
            }
        }
        tr->par[TP_SEND1] = (uint8_t)(t == 2 ? 24 : 0);       /* (the skank alone leans on the echo) */
        tr->par[TP_SEND2] = (uint8_t)(t == 2 || t == 3 ? 20 : t == 4 || t == 5 ? 30 : 0);
    }
    proj.trk[1].par[TP_LEVEL] = 92;                           /* the bass a little under the kit */
    proj.trk[2].par[TP_LEVEL] = 112;                          /* the skank up front */
    proj.trk[3].par[TP_LEVEL] = 96;                           /* the bubble under it */
    set_name(&proj.trk[AUX_FX1], "echo");
    set_name(&proj.trk[AUX_FX2], "spring");
    /* the echo: a dotted eighth (three sixteenths), long repeats, dark, nothing below about 100 Hz; the spring: the plate,
     * small and bright (the spring tank itself is to come) */
    proj.trk[AUX_FX1].par[TP_ENGINE] = FXT_DELAY;
    proj.trk[AUX_FX1].par[TP_E1] = 88;
    proj.trk[AUX_FX1].par[TP_E1 + 1] = 64;
    proj.trk[AUX_FX1].par[TP_E1 + 2] = 98;
    proj.trk[AUX_FX1].par[TP_E1 + 3] = 58;
    proj.trk[AUX_FX1].par[TP_FENV] = 34;
    proj.trk[AUX_FX1].par[TP_LEVEL] = 114;
    proj.trk[AUX_FX2].par[TP_ENGINE] = FXT_REVERB + RV_PLATE;
    proj.trk[AUX_FX2].par[TP_E1] = 58;
    proj.trk[AUX_FX2].par[TP_E1 + 1] = 30;
    proj.trk[AUX_FX2].par[TP_E1 + 2] = 70;
    proj.trk[AUX_FX2].par[TP_E1 + 3] = 56;
    proj.tempo_x10 = 750;
    proj.name[0] = 'D', proj.name[1] = 'U', proj.name[2] = 'B', proj.name[3] = 0;
    bc_riddim();
}

/* ---- the desk (GLOBE's Mix page) ---------------------------------------------------------------------------------- */

#define BC_GREEN RGB(56, 200, 96)                 /* playing */
#define BC_GOLD RGB(250, 196, 44)                 /* the echo, the sends, a throw */
#define BC_RED RGB(226, 44, 44)                   /* dropped, peaking */
static const char *const BC_NAME[NTRK] = {"DRUM", "BASS", "SKNK", "ORGN", "HORN", "VOX", "SIRN", "HAND"};

static struct {
    uint8_t drop_was;                   /* a drop pad held: the channel as it was before (bit: muted) */
    uint8_t drop_held;
    uint32_t drop_t[8];
    uint8_t thr[8];                     /* a throw pad held: 1 the echo, 2 the spring (0 none) */
} bc;

static void draw_desk(void)
{
    static float m_db[NALLTRK];
    static uint32_t m_last;
    uint32_t now = plat_ms(), dt = now - m_last;
    int sel = sel_trk(), beat = playing() ? (eng_step[0] / 4) % 4 : -1;
    uint32_t h = hash_u(hash_u(2166136261u, 0xDE5Cu), (uint32_t)sel | (uint32_t)seq.mute << 4 | (uint32_t)(beat + 1) << 20);
    float fall = (float)(dt > 1000u ? 1000u : dt) * 0.03f;
    m_last = now;
    for (int t = 0; t < NALLTRK; t++) {
        float db = lin_db((float)ui_peak[t] * (1.0f / 16383.5f));
        if (dt > 250u)
            m_db[t] = MIX_DB_LO - 1.0f;
        m_db[t] = fm_maxf(db, m_db[t] - fall);
        h = hash_u(h, (uint32_t)mix_y(m_db[t]) | (uint32_t)lev_now(t) << 8 |
                      (uint32_t)(t < NTRK ? engine_thrown(t, 0) | engine_thrown(t, 1) << 8 : 0) << 16);
    }
    if (!begin_main(h))
        return;
    for (int t = 0; t < NTRK; t++) {
        int32_t x = 4 + t * 29, cx = x + 14, cy = 30, lv = mix_y(m_db[t]);
        int muted = seq.mute >> t & 1u, thrown = engine_thrown(t, 0) || engine_thrown(t, 1), v = lev_now(t);
        uint16_t ink = muted ? BC_RED : thrown ? BC_GOLD : BC_GREEN;
        float cone = 3.0f + (float)lv * (7.0f / (float)MIX_H);            /* the cone moves with the level */
        if (thrown)                                                       /* thrown: a gold ring round the cabinet */
            cv_frame(x, 3, 28, 54, 4, BC_GOLD, K_BG);
        else if (t == sel)
            cv_frame(x, 3, 28, 54, 4, K_SEL, K_BG);
        cv_round(x + 3, 6, 22, 48, 3, K_PANEL);                           /* the cabinet */
        cv_dotf((float)cx, (float)cy, 10.0f, muted ? K_PANEL2 : K_LINE);  /* the speaker's ring */
        cv_dotf((float)cx, (float)cy, muted ? 8.0f : 8.5f, K_BG);
        if (muted) {
            cv_dotf((float)cx, (float)cy, 3.0f, BC_RED);                  /* dropped: dark, a red eye */
        } else {
            cv_dotf((float)cx, (float)cy, cone, ink);
            cv_dotf((float)cx, (float)cy, 2.0f, K_BG);
        }
        text_c(cx, 60, &FONT_XS, BC_NAME[t], muted ? BC_RED : t == sel ? K_TEXT : K_DIM);
        /* the level: a bar under the name, the fader's mark on it */
        cv_round(x + 3, 76, 22, 6, 2, K_PANEL);
        if (lv > 0 && !muted)
            cv_round(x + 3, 76, 1 + lv * 21 / MIX_H, 6, 2, lv > mix_y(0.0f) ? BC_RED : ink);
        cv_rect(x + 3 + (v ? mix_y(lin_db(tp_level_gain(v))) * 21 / MIX_H : 0), 74, 1, 10, t == sel ? K_SEL : K_DIM);
        /* its sends at rest: the echo's and the spring's, gold ticks */
        cv_rect(x + 4, 88, 1 + proj.trk[t].par[TP_SEND1] * 8 / 127, 2, proj.trk[t].par[TP_SEND1] ? BC_GOLD : K_PANEL2);
        cv_rect(x + 15, 88, 1 + proj.trk[t].par[TP_SEND2] * 8 / 127, 2, proj.trk[t].par[TP_SEND2] ? BC_GOLD : K_PANEL2);
    }
    for (int k = 0; k < 2; k++) {                                         /* the echo's and the spring's returns: a bar */
        int32_t x = k ? 176 : 36, lv = mix_y(m_db[AUX_FX1 + k]) * 36 / MIX_H;   /* each, either side of the beats */
        text_r(x - 3, 99, &FONT_XS, k ? "SPR" : "ECHO", sel == AUX_FX1 + k ? K_TEXT : K_DIM);
        cv_round(x, 102, 38, 6, 2, K_PANEL);
        if (lv > 0)
            cv_round(x + 1, 102, lv, 6, 2, BC_GOLD);
    }
    for (int b = 0; b < 4; b++)                                           /* the bar's beats */
        cv_round(84 + b * 15, 102, 11, 6, 2, b == beat ? (b ? BC_GREEN : BC_GOLD) : K_PANEL);
    cv_blit(0, MAIN_Y);
}

/* ---- the pads' desk (smk37.c lsmk_pads, lsmk_lights) -------------------------------------------------------------- */

#define BC_TAP_MS 300u                             /* a drop pad let go sooner: a tap, the drop stays (later: back as it was) */
/* a drop pad (the top row) pressed: the channel out at once; let go: a tap toggles it, a hold was a cut while held */
static void bc_drop(int ch, int down)
{
    uint32_t m = 1u << ch;
    if (ch >= NTRK || ch >= NSEQT)
        return;
    if (down) {
        bc.drop_was = (uint8_t)((bc.drop_was & ~m) | (seq.mute >> ch & 1u ? m : 0u));
        bc.drop_held |= (uint8_t)m;
        bc.drop_t[ch] = plat_ms();
        if (!(seq.mute >> ch & 1u))
            set_mute(ch, 1);
        return;
    }
    if (!(bc.drop_held & m))
        return;
    bc.drop_held &= (uint8_t)~m;
    if (plat_ms() - bc.drop_t[ch] < BC_TAP_MS) {   /* a tap: the other way from how it was */
        int was = bc.drop_was >> ch & 1u;
        set_mute(ch, !was);
        say(was ? "IN" : "DROP", BC_NAME[ch]);
    } else {
        set_mute(ch, bc.drop_was >> ch & 1u);      /* held: as it was */
    }
}
/* a throw pad (the bottom row): the channel into the echo while held, into the spring with PATCH held */
static void bc_throw(int ch, int down)
{
    if (ch >= NTRK)
        return;
    if (down) {
        int spring = plat_buttons() >> B_HOME & 1u;
        if (bc.thr[ch])
            engine_throw(ch, bc.thr[ch] - 1, 0);
        bc.thr[ch] = (uint8_t)(spring ? 2 : 1);
        engine_throw(ch, spring, 127);
        if (spring)
            ui.btn_used |= 1u << B_HOME;           /* (PATCH's own job not done when it is let go) */
    } else if (bc.thr[ch]) {
        engine_throw(ch, bc.thr[ch] - 1, 0);
        bc.thr[ch] = 0;
    }
}
static void bc_pads_quiet(void)                    /* the pads' job changes: nothing left held */
{
    for (int ch = 0; ch < 8; ch++) {
        bc_throw(ch, 0);
        if (bc.drop_held >> ch & 1u) {
            bc.drop_held &= (uint8_t)~(1u << ch);
            set_mute(ch, bc.drop_was >> ch & 1u);
        }
    }
}
#endif
