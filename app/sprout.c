/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (SPROUT, SMALL AXE) */
/* SPROUT: Sound Garden on the FM-1, a first instrument for a young child, on LOOM-8's engine. Included by app.c after
 * ui.c (its statics: the project, the engine, the drawing); ui_frame hands each frame here first (sprout_frame) and
 * LOOM-8's own pages run only in SMALL AXE's workshop.
 *
 * The garden (level 1): the screen is a keyboard laid out as the FM-1's 27 keys (F3..G5); the key played lights, its
 * name shows big above it (letters, sol-fa or none: the grown-ups' choice) and low / middle / high under the name. The
 * keys play real notes. KNOB 1 turns between free (the keys alone) and garden: an endless, never-repeating bed to play
 * over, a low drone, a wandering bell line and a quiet pulse, all on C with no chord changes, so every white key fits
 * (the white keys glow while it plays; the key the bell is on glows brighter: watch it and copy it). Nothing else on
 * the panel does anything yet; the grown-ups open more as the child is ready (the levels, sprout-plan's ladder).
 *
 * The grown-ups' menu: GLO and HOME held together for a second and a half. The level (its idea, and the signs that
 * say the child is ready for the next), the note names, SMALL AXE, and the way back to the garden.
 *
 * SMALL AXE: a dub desk, BLOODCLOT's idea in synth voices alone. Eight channels (DRUM BASS SKNK ORGN HORN VOX SIRN
 * HAND) play a riddim; the desk dubs it live: black keys 1-8 drop a channel (a tap out or back in, a hold out while
 * held), white keys 1-8 throw a channel into the echo while held, white keys 9-16 into the spring, black keys 9-11
 * sound the siren. KNOBS 1-4: the echo's time, repeats and tone, the spring's size; SELECT the tempo; PLAY plays.
 * HOME opens the workshop, LOOM-8's own pages on the same project (build the riddim there); the grown-ups' menu
 * comes back to the desk or the garden.
 *
 * The garden keeps its project (its sounds, its bed, the grown-ups' settings in rsv[8], SP_MAGIC in rsv[9]) in a slot
 * of its own: the last slot, or the last one free; a slot holding anything else is never written. SMALL AXE opens a
 * slot that holds a SMALL AXE project, else starts a riddim in a free one (or unsaved, when none is free). */
#if SPROUT

#define SP_MAGIC 0x5Au                  /* proj.rsv[9]: the garden's project */
#define AXE_MAGIC 0xA5u                 /* proj.rsv[9]: a SMALL AXE project */
#define SP_HOLD_MS 1500u                /* GLO and HOME held this long: the grown-ups' menu */
#define THEME_AXE 3u                    /* pages.c THEMES: "small axe" */
#define SP_NOSLOT 0xFFu

enum { SPM_BOOT, SPM_GARDEN, SPM_MENU, SPM_DESK, SPM_SHOP };
enum { NM_LETTERS, NM_SOLFA, NM_HIDDEN, NNM };
enum { GT_PULSE, GT_KEYS, GT_BELL, GT_DRONE };            /* the garden's tracks */
enum { AX_DRUM, AX_BASS, AX_SKNK, AX_ORGN, AX_HORN, AX_VOX, AX_SIRN, AX_HAND };   /* SMALL AXE's channels */
enum { MR_LEVEL, MR_NAMES, MR_DESK, MR_SHOP, MR_GARDEN, NMR };

static const char *const SP_NM_NAME[NNM] = {"letters", "do re mi", "hidden"};
static const char *const AX_NAME[NTRK] = {"DRUM", "BASS", "SKNK", "ORGN", "HORN", "VOX", "SIRN", "HAND"};
/* the ladder (sprout-plan): each level's idea, and what says the child is ready for the next */
static const char *const SP_LEVEL_NAME[8] = {"keys", "beat", "echo", "loop", "loud & soft", "drums", "voices",
                                             "bass & space"};
static const char *const SP_LEVEL_IDEA[8] = {
    "high and low, the keyboard, notes by name",
    "a steady pulse, fast and slow",
    "listening, and copying a tune",
    "patterns that repeat",
    "soft and loud",
    "rhythm patterns, layers",
    "which sound suits which mood",
    "a low part under the tune; space"};
static const char *const SP_READY[8][2] = {
    {"plays high or low when asked;", "happy to play along with the garden"},
    {"taps or plays along with the pulse;", "notices when the speed changes"},
    {"echoes three-note tunes most times;", "starts answering with their own"},
    {"records something on purpose", "and plays over it"},
    {"plays soft and loud when asked,", "and chooses on purpose"},
    {"builds a simple beat", "that sits with the pulse"},
    {"picks a voice on purpose", ""},
    {"", ""}};
#define SP_LEVELS_BUILT 1               /* this build has level 1; the others are on the way */

static struct {
    uint8_t mode, from;                 /* SPM_*; where the menu came from */
    uint8_t level, names, garden;       /* the grown-ups' settings (level 1..8), free / garden */
    uint8_t gslot, aslot;               /* the garden's slot and SMALL AXE's (SP_NOSLOT: none) */
    uint8_t dirty;                      /* settings changed, not yet saved with the garden */
    uint8_t row;                        /* the menu's row */
    uint32_t keys, btn;                 /* the panel as sprout last saw it (its own edges) */
    uint32_t combo_t;                   /* GLO and HOME held together since (0 not) */
    uint8_t combo_done;
    int8_t shown;                       /* the key whose name shows (-1: none played yet) */
    uint8_t bar_seen;                   /* the garden's bar playing, as last seen (0xFF none) */
    int8_t walk;                        /* the bell's place on its scale */
    uint32_t rnd;
    uint32_t sig;                       /* the screen as drawn (0: draw it) */
    /* the desk */
    uint8_t drop_was, drop_held, thr[NTRK], siren;
    uint32_t drop_t[NTRK];
    uint8_t pend_slot;                  /* a new project to save once the engine has its tempo (project_save reads it
                                         * back from the sequencer: SP_NOSLOT none) */
    uint32_t pend_t;
} spr = {.mode = SPM_BOOT, .level = 1, .names = NM_LETTERS, .gslot = SP_NOSLOT, .aslot = SP_NOSLOT, .shown = -1,
       .bar_seen = 0xFF, .walk = 2, .rnd = 0x2545F491u, .pend_slot = SP_NOSLOT};

static uint32_t spr_rand(void)
{
    uint32_t r = spr.rnd;
    r ^= r << 13, r ^= r >> 17, r ^= r << 5;
    return spr.rnd = r;
}
static int spr_owns(void) { return spr.mode != SPM_SHOP; }

/* ---- sounds and patterns ------------------------------------------------------------------------------------------ */

static int spr_preset(const char *name)  /* a factory preset by its whole name, -1 none */
{
    for (int k = 0; k < NPRESETS; k++)
        if (name_is(PRESETS[k].name, name))
            return k;
    return -1;
}
static void spr_sound(int t, const char *name)   /* a track's sound: the factory preset of that name */
{
    int k = spr_preset(name);
    if (k >= 0)
        apply_preset(&proj.trk[t], k);
}
static int spr_kit_ix(const char *name)
{
    for (int k = 1; k < NFKITS; k++)
        if (name_is(kit_name(k), name))
            return k;
    return 0;
}
static void spr_kit_par(int k, uint8_t *par)     /* a factory kit's sound and keys into par (kit_factory's) */
{
    sound_default(par, EG_DRUMLINE);
    if (k >= 1)
        for (int key = 0; key < DL_NSOUND; key++) {
            const uint8_t *g = GKIT[k - 1][key];
            par[TP_DRUM(key, DP_MODEL)] = g[0];
            for (int j = 0; j < 4; j++) {
                par[TP_DRUM(key, DP_K1 + j)] = g[1 + j];
                par[TP_DRUMX(key, j)] = g[5 + j];
            }
            par[TP_DRUM(key, DP_VOLUME)] = g[9];
            par[TP_DRUM(key, DP_PAN)] = g[10];
            par[TP_DRUM(key, DP_MODE)] = g[11];
            par[TP_DRUM(key, DP_DECAY)] = g[12];
        }
}
static void spr_kit_put(int t, const uint8_t *par, const char *name)   /* the kit onto track t (its mix stays) */
{
    uint8_t *p = proj.trk[t].par;
    for (int i = 0; i < NTP_SOUND; i++)
        p[i] = par[i];
    for (int i = TP_DRUM0; i < TP_O2FINE; i++)
        p[i] = par[i];
    p[TP_ENGINE] = EG_DRUMLINE;
    set_name(&proj.trk[t], name);
}
static tpat_t *spr_pat(int t) { return &pats.pat[t][0]; }   /* a new project: scene 1 plays pattern 1 everywhere */
static void spr_fresh(int t, int len)
{
    tpat_t *tp = spr_pat(t);
    pattern_init(tp);
    tp->len = (uint8_t)len;
}
static void spr_notes(int t, int s, int n0, int n1, int n2, int vel, int len_q)   /* notes on a step (0: none) */
{
    step_t *st = &spr_pat(t)->st[s];
    st->note[0] = (uint8_t)n0;
    st->note[1] = (uint8_t)n1;
    st->note[2] = (uint8_t)n2;
    st->note[3] = 0;
    st->vel = (uint8_t)vel;
    st->len = (uint8_t)step_len_code(len_q);
}
static void spr_hits(int t, int s, uint32_t sounds, int vel)   /* a kit's sounds on a step (bit k: sound k + 1) */
{
    step_t *st = &spr_pat(t)->st[s];
    kit_set(st, sounds | kit_mask(st));
    st->vel = (uint8_t)vel;
}
static void spr_quiet_tracks(void)       /* T5-T8 of a new project: no sound of their own (the garden uses four) */
{
    for (int t = 0; t < NTRK; t++) {
        proj.trk[t].par[TP_BRAIN] = 0;  /* nothing follows the conductor: the notes are as written and played */
        proj.trk[t].par[TP_CHAN] = 1;
    }
}

/* ---- the garden ------------------------------------------------------------------------------------------------- */

/* the bell's scale: C major pentatonic, C4 to G5 (the keyboard's top), so its note is always a key to copy */
static const uint8_t SP_PENTA[] = {60, 62, 64, 67, 69, 72, 74, 76, 79};
#define SP_NPENTA ((int)(sizeof SP_PENTA))
/* the bell's rhythms, a bar each ('x' a note): mostly on the beats, room to breathe */
static const char *const SP_RHY[] = {"x...x...x...x...", "x.....x.x.......", "x...x.x.x.......", "x.x.x...x.......",
                                     "x.......x...x...", "x...x...x.x.x...", "..x.x...x...x...", "x...x.......x..."};
#define SP_NRHY ((int)(sizeof SP_RHY / sizeof SP_RHY[0]))

static void spr_bell_bar(int bar)        /* bar (0..3) of the bell's line, new: a question (even bars) or an answer that
                                         * comes home to C (odd) */
{
    tpat_t *tp = spr_pat(GT_BELL);
    const char *r = SP_RHY[spr_rand() % SP_NRHY];
    int at[16], n = 0;
    for (int i = 0; i < 16; i++) {
        step_t *st = &tp->st[bar * 16 + i];
        st->note[0] = st->note[1] = st->note[2] = st->note[3] = 0;
        st->vel = 0;
        st->len = 0;
        if (r[i] == 'x')
            at[n++] = i;
    }
    for (int j = 0; j < n; j++) {
        static const int8_t MOVE[9] = {-2, -1, -1, -1, 0, 1, 1, 1, 2};   /* mostly a step, now and then a leap */
        int w = spr.walk + MOVE[spr_rand() % 9u], gap = (j + 1 < n ? at[j + 1] : 16) - at[j];
        if (w < 0)
            w = 1;
        if (w >= SP_NPENTA)
            w = SP_NPENTA - 2;
        if ((bar & 1) && j == n - 1)    /* the answer's last note: home, the nearest C */
            w = w < 3 ? 0 : 5;
        spr.walk = (int8_t)w;
        spr_notes(GT_BELL, bar * 16 + at[j], SP_PENTA[w], 0, 0, (j == 0 ? 74 : 52) + (int)(spr_rand() % 16u),
                 gap * 4 - 1);
    }
}
static void spr_garden_patterns(void)    /* the bed: the pulse, the drone, the bell (four bars each) */
{
    spr_fresh(GT_PULSE, 64);
    for (int b = 0; b < 4; b++)
        for (int i = 0; i < 16; i += 4)
            spr_hits(GT_PULSE, b * 16 + i, (1u << 8) | (i == 0 ? 1u : 0u), i == 0 ? 50 : 36);   /* the shaker, the kick on 1 */
    spr_fresh(GT_DRONE, 64);
    spr_notes(GT_DRONE, 0, 48, 55, 0, 70, NSTEP * 4);     /* C3 and G3, the whole four bars */
    spr_fresh(GT_BELL, 64);
    for (int b = 0; b < 4; b++)
        spr_bell_bar(b);
    spr_fresh(GT_KEYS, 16);
}
static void spr_garden_project(void)     /* a new garden: its sounds, its bed, the grown-ups' settings */
{
    uint8_t kit[NTP];
    project_defaults();
    spr_quiet_tracks();
    proj.tempo_x10 = 840;
    proj.groove = GR_SWING;
    spr_kit_par(spr_kit_ix("Velvet"), kit);
    spr_kit_put(GT_PULSE, kit, "pulse");
    spr_sound(GT_KEYS, "marimba");
    spr_sound(GT_BELL, "glass bell");
    spr_sound(GT_DRONE, "glass pad");
    proj.trk[GT_PULSE].par[TP_LEVEL] = 70;
    proj.trk[GT_KEYS].par[TP_LEVEL] = 124;
    proj.trk[GT_BELL].par[TP_LEVEL] = 84;
    proj.trk[GT_DRONE].par[TP_LEVEL] = 68;
    proj.trk[GT_KEYS].par[TP_SEND2] = 26;               /* a little of the reverb on the keys; more on the bed */
    proj.trk[GT_BELL].par[TP_SEND1] = 22;
    proj.trk[GT_BELL].par[TP_SEND2] = 40;
    proj.trk[GT_DRONE].par[TP_SEND2] = 44;
    proj.trk[GT_KEYS].par[TP_VMODE] = VM_POLY;
    set_name(&proj.trk[GT_KEYS], "keys");
    set_name(&proj.trk[GT_BELL], "bell");
    set_name(&proj.trk[GT_DRONE], "drone");
    spr_garden_patterns();
    proj.rsv[8] = (uint8_t)((spr.level - 1) | spr.names << 4);
    proj.rsv[9] = SP_MAGIC;
}

/* ---- SMALL AXE's riddim (BLOODCLOT's milestone 1, in synth voices) ---------------------------------------------- */

/* the bass line, two bars in A minor (a step each, '-' the note before held on): the root held, a walk up to the
 * fifth, and home by the seventh below */
static int ax_bass_note(char c)
{
    switch (c) {
    case 'a': return 33;
    case 'c': return 36;
    case 'd': return 38;
    case 'e': return 40;
    case 'g': return 31;
    case 'E': return 28;
    default: return 0;
    }
}
static void ax_line(int t, const char *row, int (*note)(char), int vel)
{
    int n = 0;
    while (row[n])
        n++;
    spr_fresh(t, n > NSTEP ? NSTEP : n);
    for (int i = 0; i < n && i < NSTEP; i++) {
        int nt = note(row[i]), l = 1;
        if (!nt)
            continue;
        while (i + l < n && row[i + l] == '-')
            l++;
        spr_notes(t, i, nt, 0, 0, vel, l * 4 - 1);
    }
}
static int ax_horn_note(char c)
{
    switch (c) {
    case 'a': return 69;
    case 'c': return 72;
    case 'd': return 74;
    case 'e': return 76;
    case 'g': return 67;
    default: return 0;
    }
}
static void ax_chords(int t, uint32_t mask, const uint8_t *ch, int vel, int len_q)   /* every bar alike */
{
    spr_fresh(t, 16);
    for (int i = 0; i < 16; i++)
        if (mask >> i & 1u)
            spr_notes(t, i, ch[0], ch[1], ch[2], vel, len_q);
}
static void ax_riddim(void)
{
    static const uint8_t AM_LOW[3] = {57, 60, 64}, AM_HIGH[3] = {69, 72, 76};
    spr_fresh(AX_DRUM, 16);                              /* the one drop: nothing on 1, the kick and the rim on 3 */
    for (int i = 0; i < 16; i += 2)
        spr_hits(AX_DRUM, i, (1u << 2) | (i == 8 ? (1u << 0) | (1u << 7) : 0u), i == 8 ? 112 : i % 4 ? 96 : 72);
    ax_line(AX_BASS, "a--a..c.e---d.c.a--a..g.a-----E.", ax_bass_note, 108);
    ax_chords(AX_SKNK, 0x4444u, AM_LOW, 104, 2);        /* the skank: every "and", short */
    ax_chords(AX_ORGN, 0xAAAAu, AM_HIGH, 84, 1);        /* the bubble: the sixteenths between, soft */
    ax_line(AX_HORN, "................a...c.e-----d...", ax_horn_note, 96);   /* an answer in the second bar */
    spr_fresh(AX_VOX, 32);
    spr_notes(AX_VOX, 0, 57, 64, 0, 80, 32 * 4);         /* an "ooh" over the two bars (dropped at the start) */
    spr_fresh(AX_SIRN, 16);                              /* (the siren: the desk's black keys 9-11) */
    spr_fresh(AX_HAND, 16);                              /* the hand drums: the heartbeat, the repeater, the shaker */
    for (int i = 0; i < 16; i += 2)
        spr_hits(AX_HAND, i, 1u << 8, 40);
    spr_hits(AX_HAND, 0, 1u << 5, 92);
    spr_hits(AX_HAND, 6, 1u << 7, 84);
    spr_hits(AX_HAND, 7, 1u << 7, 64);
    spr_hits(AX_HAND, 14, 1u << 7, 76);
}
static void ax_project(void)            /* a new SMALL AXE project: the channels, the echo and the spring, the riddim */
{
    uint8_t kit[NTP], lin[NTP];
    project_defaults();
    spr_quiet_tracks();
    spr_kit_par(spr_kit_ix("Velvet"), kit);               /* the drums: Velvet's round kick, its hats; Linen's rim on key 8 */
    spr_kit_par(spr_kit_ix("Linen"), lin);
    for (int j = 0; j < DP_N; j++)
        kit[TP_DRUM(7, j)] = lin[TP_DRUM(7, j)];
    for (int j = 0; j < 4; j++)
        kit[TP_DRUMX(7, j)] = lin[TP_DRUMX(7, j)];
    spr_kit_put(AX_DRUM, kit, "drum");
    spr_kit_par(spr_kit_ix("Velvet"), kit);               /* the hand drums: its toms, conga and shaker */
    spr_kit_put(AX_HAND, kit, "hand");
    spr_sound(AX_BASS, "round sub");
    spr_sound(AX_SKNK, "mellow ep");
    spr_sound(AX_ORGN, "combo");
    spr_sound(AX_HORN, "brass");
    spr_sound(AX_VOX, "ooh");
    spr_sound(AX_SIRN, "soft lead");
    {   /* the siren: its pitch swept up by the LFO, again and again, while a key is held; mostly echo */
        uint8_t *p = proj.trk[AX_SIRN].par;
        p[TP_LTYPE] = LT_WAVE;
        p[TP_LSPEED] = 72;                              /* (free: about 2.5 Hz) */
        p[TP_LAMT] = 104;                               /* about +-7 semitones */
        p[TP_LDEST] = LD_AMP;
        p[TP_LX] = 1;                                   /* (the voice's pitch) */
        p[TP_LX2] = 2;                                  /* (a ramp: a whoop each time round) */
        p[TP_VMODE] = VM_MONO;
        p[TP_SEND1] = 96;
        p[TP_SEND2] = 30;
    }
    for (int t = 0; t < NTRK; t++)
        if (t != AX_SIRN && t != AX_DRUM && t != AX_HAND) {
            proj.trk[t].par[TP_SEND1] = (uint8_t)(t == AX_SKNK ? 24 : 0);
            proj.trk[t].par[TP_SEND2] = (uint8_t)(t == AX_SKNK || t == AX_ORGN ? 20 : t == AX_HORN || t == AX_VOX ? 30 : 0);
        }
    for (int t = 0; t < NTRK; t++)
        set_name(&proj.trk[t], AX_NAME[t]);
    proj.trk[AX_BASS].par[TP_LEVEL] = 96;
    proj.trk[AX_SKNK].par[TP_LEVEL] = 108;
    proj.trk[AX_ORGN].par[TP_LEVEL] = 88;
    proj.trk[AX_HAND].par[TP_LEVEL] = 84;
    proj.trk[AX_VOX].par[TP_LEVEL] = 84;
    set_name(&proj.trk[AUX_FX1], "echo");
    set_name(&proj.trk[AUX_FX2], "spring");
    proj.trk[AUX_FX1].par[TP_ENGINE] = FXT_DELAY;       /* a dotted eighth, long repeats, dark, nothing below 100 Hz */
    proj.trk[AUX_FX1].par[TP_E1] = 88;
    proj.trk[AUX_FX1].par[TP_E1 + 1] = 64;
    proj.trk[AUX_FX1].par[TP_E1 + 2] = 98;
    proj.trk[AUX_FX1].par[TP_E1 + 3] = 58;
    proj.trk[AUX_FX1].par[TP_FENV] = 34;
    proj.trk[AUX_FX1].par[TP_LEVEL] = 114;
    proj.trk[AUX_FX2].par[TP_ENGINE] = FXT_REVERB + RV_PLATE;   /* the spring: the plate, small and bright */
    proj.trk[AUX_FX2].par[TP_E1] = 58;
    proj.trk[AUX_FX2].par[TP_E1 + 1] = 30;
    proj.trk[AUX_FX2].par[TP_E1 + 2] = 70;
    proj.trk[AUX_FX2].par[TP_E1 + 3] = 56;
    proj.tempo_x10 = 750;
    ax_riddim();
    pats.scene[0].mute = (uint16_t)(1u << AX_VOX);      /* the voices wait to be brought in */
    proj.rsv[1] = (uint8_t)(THEME_SET | THEME_AXE);
    proj.rsv[9] = AXE_MAGIC;
}

/* ---- the slots ------------------------------------------------------------------------------------------------- */

static int spr_slot_holds(int s)         /* what slot s holds: 0 nothing, SP_MAGIC the garden, AXE_MAGIC SMALL AXE, 1 else */
{
    int r;
    if (!project_exists(s))
        return 0;
    project_device_get();
    if (project_load(s) != 0) {
        project_device_put();
        return 1;
    }
    project_device_put();
    r = proj.rsv[9] == SP_MAGIC ? SP_MAGIC : proj.rsv[9] == AXE_MAGIC ? AXE_MAGIC : 1;
    return r;
}
static void spr_find_slots(void)         /* the garden's slot (the last free or its own) and SMALL AXE's */
{
    int hold[NPROJ];
    spr.gslot = spr.aslot = SP_NOSLOT;
    for (int s = 0; s < NPROJ; s++)
        hold[s] = spr_slot_holds(s);
    for (int s = NPROJ - 1; s >= 0 && spr.gslot == SP_NOSLOT; s--)
        if (hold[s] == SP_MAGIC)
            spr.gslot = (uint8_t)s;
    for (int s = NPROJ - 1; s >= 0 && spr.gslot == SP_NOSLOT; s--)
        if (hold[s] == 0)
            spr.gslot = (uint8_t)s;
    for (int s = 0; s < NPROJ && spr.aslot == SP_NOSLOT; s++)
        if (hold[s] == AXE_MAGIC)
            spr.aslot = (uint8_t)s;
}
static void spr_settings_take(void)      /* the grown-ups' settings from the garden's project */
{
    spr.level = (uint8_t)((proj.rsv[8] & 7u) + 1u);
    spr.names = (uint8_t)((proj.rsv[8] >> 4 & 3u) % NNM);
    if (spr.level > SP_LEVELS_BUILT)
        spr.level = SP_LEVELS_BUILT;
}
static void spr_stop_all(void)
{
    engine_transport(0);
    engine_panic();
    for (int t = 0; t < NTRK; t++)
        engine_keys_off(t);
}
static void spr_save_soon(int slot)    /* a new project saved a moment from now, once the engine runs at its tempo */
{
    spr.pend_slot = (uint8_t)slot;
    spr.pend_t = plat_ms();
}
static void spr_pending(void)
{
    if (spr.pend_slot == SP_NOSLOT || plat_ms() - spr.pend_t < 80u)
        return;
    if (proj_slot == spr.pend_slot && !playing())
        project_save(spr.pend_slot);
    spr.pend_slot = SP_NOSLOT;
}
static void spr_garden_load(void)        /* the garden's project into RAM (made, and kept in its slot, the first time) */
{
    spr_stop_all();
    spr.garden = 0;
    project_device_get();
    if (spr.gslot == SP_NOSLOT || project_load(spr.gslot) != 0 || proj.rsv[9] != SP_MAGIC) {
        project_device_put();
        spr_garden_project();
        proj_slot = spr.gslot != SP_NOSLOT ? spr.gslot : PROJ_NOSLOT;
        if (spr.gslot != SP_NOSLOT)
            spr_save_soon(spr.gslot);
        spr.dirty = 0;
    } else {
        project_device_put();
        proj_slot = spr.gslot;
        if (spr.dirty) {                 /* the grown-ups changed something while SMALL AXE had the RAM */
            proj.rsv[8] = (uint8_t)((spr.level - 1) | spr.names << 4);
            project_save(spr.gslot);
            spr.dirty = 0;
        } else
            spr_settings_take();
    }
    after_load();
    ui.dirty = 0;
    spr.bar_seen = 0xFF;
}
static void spr_settings_save(void)      /* the grown-ups' settings kept with the garden (it is in RAM) */
{
    int was = spr.garden;
    if (!spr.dirty || spr.gslot == SP_NOSLOT || proj.rsv[9] != SP_MAGIC)
        return;
    proj.rsv[8] = (uint8_t)((spr.level - 1) | spr.names << 4);
    if (was)
        engine_transport(0);           /* (a flash write stops the sound for a moment anyway) */
    if (project_save(spr.gslot) == 0)
        spr.dirty = 0;
    if (was)
        engine_transport(3);
}
static void spr_axe_load(void)           /* SMALL AXE's project into RAM (a new riddim the first time) */
{
    spr_stop_all();
    spr.garden = 0;
    project_device_get();
    if (spr.aslot != SP_NOSLOT && project_load(spr.aslot) == 0 && proj.rsv[9] == AXE_MAGIC) {
        project_device_put();
        proj_slot = spr.aslot;
    } else {
        project_device_put();
        ax_project();
        proj_slot = PROJ_NOSLOT;
        for (int s = 0; s < NPROJ && proj_slot == PROJ_NOSLOT; s++)
            if (s != spr.gslot && !project_exists(s))
                proj_slot = (uint8_t)s;
        if (proj_slot < NPROJ) {
            spr.aslot = proj_slot;
            spr_save_soon(proj_slot);
        }
    }
    proj.rsv[1] = (uint8_t)(THEME_SET | THEME_AXE);
    after_load();
    seq.mute = pats.scene[cur_scene()].mute;
    ui.dirty = 0;
    spr.drop_held = 0;
    for (int t = 0; t < NTRK; t++)
        spr.thr[t] = 0;
    spr.siren = 0;
}
static int spr_slot_guard_quiet(int slot) { return slot != spr.gslot; }   /* (NEW PROJECT's free slot) */
static int spr_slot_guard(int slot)      /* LOOM-8's SAVE TO / LOAD (the workshop): 1 = not the garden's slot */
{
    if (slot == spr.gslot) {
        say_no("THE GARDEN'S", "pick another slot");
        return 0;
    }
    return 1;
}

/* ---- handing the panel over ------------------------------------------------------------------------------------- */

static void spr_panel_take(void)         /* from LOOM-8's pages to sprout's: nothing held counts as new */
{
    spr.btn = plat_buttons();
    spr.keys = plat_keys();
    for (int i = 0; i < NE; i++)
        (void)plat_enc(i);
    spr.sig = 0;
}
static void spr_panel_give(void)         /* to LOOM-8's pages: what is held stays held, and does nothing */
{
    ui.btn = plat_buttons();
    ui.btn_used = ui.btn;
    ui.keys = plat_keys();
    for (int k = 0; k < NKEYS; k++)
        if (ui.keys >> k & 1u)
            ui.kmode[k] = KM_IDLE;
    for (int i = 0; i < NE; i++)
        (void)plat_enc(i);
    ui.card = 0;
    ui_redraw();
    k_th = 0;                           /* (ink_follow: the whole screen again) */
}
static void spr_mode(int m)
{
    int was = spr.mode;
    spr.mode = (uint8_t)m;
    if (m == SPM_SHOP)
        spr_panel_give();
    else if (was == SPM_SHOP)
        spr_panel_take();
    spr.sig = 0;
}
static int spr_combo(void)               /* 1: GLO and HOME have been held together long enough (once a hold) */
{
    uint32_t b = plat_buttons(), m = 1u << B_GLO | 1u << B_HOME;
    if ((b & m) != m) {
        spr.combo_t = 0;
        spr.combo_done = 0;
        return 0;
    }
    if (!spr.combo_t)
        spr.combo_t = plat_ms() | 1u;
    if (!spr.combo_done && plat_ms() - spr.combo_t >= SP_HOLD_MS) {
        spr.combo_done = 1;
        return 1;
    }
    return 0;
}
static void spr_menu_open(void)
{
    spr.from = spr.mode;
    spr.row = spr.from == SPM_GARDEN ? MR_LEVEL : MR_DESK;
    spr_mode(SPM_MENU);
}

/* ---- the drawing -------------------------------------------------------------------------------------------------- */

#define SP_PAPER RGB(239, 232, 216)
#define SP_CARD RGB(246, 241, 228)
#define SP_CELL RGB(228, 219, 198)
#define SP_INK RGB(74, 64, 52)
#define SP_SOFT RGB(133, 119, 98)
#define SP_FAINT RGB(196, 186, 164)
#define SP_SAGE RGB(147, 169, 126)
#define SP_SAGE_D RGB(108, 132, 92)
#define SP_OCHRE RGB(210, 160, 92)
#define SP_WKEY RGB(250, 247, 240)
#define AX_BG RGB(14, 12, 10)
#define AX_PANEL RGB(36, 28, 22)
#define AX_PANEL2 RGB(60, 46, 34)
#define AX_DIM RGB(150, 128, 96)
#define AX_TEXT RGB(246, 232, 200)
#define AX_GREEN RGB(56, 200, 96)
#define AX_GOLD RGB(250, 196, 44)
#define AX_RED RGB(226, 44, 44)

static const char *const SP_LET[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *const SP_SOL[12] = {"do", "di", "re", "ri", "mi", "fa", "fi", "sol", "si", "la", "li", "ti"};
static int spr_black(int note) { int c = note % 12; return c == 1 || c == 3 || c == 6 || c == 8 || c == 10; }

/* a leaf from (x0, y0) to its tip (x1, y1): round dots along it, widest at the middle (no trigonometry) */
static void spr_leaf(float x0, float y0, float x1, float y1, float w, uint16_t c)
{
    for (int i = 0; i <= 12; i++) {
        float t = (float)i / 12.0f, r = 4.0f * t * (1.0f - t) * w + 0.6f;
        cv_dotf(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, r, c);
    }
}
/* a sprout standing in a little mound of earth at (cx, by): a curved stem, a pair of seed leaves, and a leaf more
 * for each level past the first (leaves: 1.. ); s its scale */
static void spr_seedling(int32_t cx, int32_t by, float s, int leaves)
{
    float x = (float)cx, y = (float)by, top = y - 22.0f * s;
    cv_linef(x, y - 2.0f * s, x + 1.5f * s, y - 12.0f * s, SP_SAGE_D, 2.2f * s);       /* the stem, a little bent */
    cv_linef(x + 1.5f * s, y - 12.0f * s, x, top, SP_SAGE_D, 2.2f * s);
    spr_leaf(x, top, x - 11.0f * s, top - 6.0f * s, 4.2f * s, SP_SAGE);                /* the seed leaves */
    spr_leaf(x, top, x + 11.0f * s, top - 7.0f * s, 4.2f * s, SP_SAGE);
    for (int l = 1; l < leaves && l < 8; l++) {          /* a leaf more, alternately, down the stem */
        float ly = y - (4.0f + 3.0f * (float)l) * s, d = l & 1 ? 1.0f : -1.0f;
        spr_leaf(x + d * 1.0f * s, ly, x + d * 9.0f * s, ly - 5.0f * s, 3.2f * s, SP_SAGE_D);
    }
    for (int i = -3; i <= 3; i++)                         /* the earth: a low mound */
        cv_dotf(x + (float)i * 3.2f * s, y + 1.5f * s + (float)(i * i) * 0.25f * s, 3.0f * s, SP_OCHRE);
}
/* the keyboard, F3..G5 as the FM-1's keys, in a band: whites 0..15 from x0, the held lit, the bell's note marked */
static void spr_keyboard(int32_t y, int32_t h, uint32_t held, int bell_key)
{
    const int32_t x0 = 6, ww = 14, bw = 9, bh = h * 6 / 10;
    int wi = 0;
    cv_round(x0 - 4, y - 4, 16 * ww + 8, h + 8, 6, SP_CARD);
    for (int k = 0; k < NKEYS; k++) {
        if (spr_black(KEY_BASE + k))
            continue;
        int32_t x = x0 + wi * ww;
        cv_round(x + 1, y, ww - 2, h, 2, held >> k & 1u ? SP_SAGE : SP_WKEY);
        if (k == bell_key && !(held >> k & 1u))
            cv_dotf((float)x + (float)ww / 2.0f, (float)(y + h - 9), 3.2f, SP_OCHRE);
        wi++;
    }
    wi = 0;
    for (int k = 0; k < NKEYS; k++) {
        if (!spr_black(KEY_BASE + k)) {
            wi++;
            continue;
        }
        int32_t x = x0 + wi * ww - bw / 2;
        cv_round(x, y, bw, bh, 2, held >> k & 1u ? SP_SAGE : SP_INK);
        if (k == bell_key && !(held >> k & 1u))
            cv_dotf((float)x + (float)bw / 2.0f, (float)(y + bh - 7), 2.6f, SP_OCHRE);
    }
}
static void spr_chip(int32_t cx, int32_t y, const char *s, int on)
{
    int32_t w = text_w(&FONT_B, s);
    if (on)
        cv_round(cx - w / 2 - 9, y, w + 18, 20, 10, SP_INK);
    text_c(cx, y + 2, &FONT_B, s, on ? SP_CARD : SP_SOFT);
}
static int spr_bell_key(void)            /* the key the garden's bell is on now (-1 none) */
{
    const tpat_t *tp = spr_pat(GT_BELL);
    int s = eng_step[GT_BELL] % NSTEP;
    if (!spr.garden)
        return -1;
    for (int i = 0; i < 3; i++) {
        int n = tp->st[(s - i + NSTEP) % NSTEP].note[0];
        if (n)
            return n >= KEY_BASE && n < KEY_BASE + NKEYS ? n - KEY_BASE : -1;
    }
    return -1;
}
static void spr_draw_garden(void)
{
    uint32_t held = plat_keys() & ((1u << NKEYS) - 1u);
    int bell = spr_bell_key();
    uint32_t h = hash_u(hash_u(hash_u(2166136261u, 0x6A4Du), held), (uint32_t)(spr.shown + 1) | (uint32_t)(bell + 1) << 8 |
                        (uint32_t)spr.garden << 16 | (uint32_t)spr.names << 20 | (uint32_t)spr.level << 24);
    if (h == spr.sig)
        return;
    spr.sig = h;
    cv_begin(240, 120, SP_PAPER);                       /* the top: the note's name, or the sprout */
    if (spr.shown < 0 || spr.names == NM_HIDDEN) {
        spr_seedling(120, 96, 3.0f, spr.level);
    } else {
        int note = KEY_BASE + spr.shown;
        const char *nm = spr.names == NM_SOLFA ? SP_SOL[note % 12] : SP_LET[note % 12];
        uint16_t c = held >> spr.shown & 1u ? SP_INK : SP_SOFT;
        spr_seedling(18, 30, 0.9f, spr.level);
        text_c(120, 8, &LFONT_G, nm, c);
        text_c(120, 96, &FONT_B, note < 60 ? "low" : note < 72 ? "middle" : "high", SP_SOFT);
    }
    cv_blit(0, 0);
    cv_begin(240, 120, SP_PAPER);                       /* the bottom: the keyboard, and KNOB 1's two places */
    spr_keyboard(18, 64, held, bell);
    spr_chip(84, 94, "free", !spr.garden);
    spr_chip(156, 94, "garden", spr.garden);
    cv_blit(0, 120);
}

static const char *spr_row_value(int r, char *buf)
{
    switch (r) {
    case MR_LEVEL: {
        char *e = num_s(spr.level, buf);
        s_cat(s_cat(e, " "), SP_LEVEL_NAME[spr.level - 1]);
        return buf;
    }
    case MR_NAMES: return SP_NM_NAME[spr.names];
    case MR_DESK: return spr.from == SPM_DESK ? "back to it" : "open";
    case MR_SHOP: return "LOOM-8's pages";
    default: return spr.from == SPM_GARDEN ? "back to play" : "the child's";
    }
}
static void spr_draw_menu(void)
{
    static const char *const ROW[NMR] = {"level", "note names", "small axe", "workshop", "garden"};
    char buf[32];
    uint32_t h = hash_u(hash_u(2166136261u, 0x3E2Bu), (uint32_t)spr.row | (uint32_t)spr.level << 4 |
                        (uint32_t)spr.names << 8 | (uint32_t)spr.from << 12);
    if (h == spr.sig)
        return;
    spr.sig = h;
    cv_begin(240, 120, SP_PAPER);
    spr_seedling(16, 28, 0.85f, 1);
    cv_text(34, 6, &FONT_B, "grown-ups", SP_INK);
    text_r(234, 6, &FONT_XS, "SPROUT", SP_SOFT);
    for (int r = 0; r < NMR; r++) {
        int32_t y = 30 + r * 18, on = r == spr.row;
        if (on)
            cv_round(4, y - 1, 232, 18, 6, SP_CELL);
        cv_text(12, y, &FONT_S, ROW[r], on ? SP_INK : SP_SOFT);
        text_r(228, y, &FONT_B, spr_row_value(r, buf), on ? SP_INK : SP_SOFT);
    }
    cv_blit(0, 0);
    cv_begin(240, 120, SP_PAPER);
    if (spr.row == MR_LEVEL || spr.row == MR_NAMES) {     /* the level's idea, and the signs for the next */
        int nx = spr.level < 8 ? spr.level : 0;
        cv_text(12, 2, &FONT_XS, SP_LEVEL_IDEA[spr.level - 1], SP_INK);
        if (nx) {
            cv_text(12, 22, &FONT_XS, "ready for the next level when the child", SP_SOFT);
            cv_text(12, 37, &FONT_XS, SP_READY[spr.level - 1][0], SP_SOFT);
            cv_text(12, 52, &FONT_XS, SP_READY[spr.level - 1][1], SP_SOFT);
            num_s((uint32_t)nx + 1u, s_cat(buf, "next: "));
            s_cat(s_cat(buf + s_len(buf), " "), SP_LEVEL_NAME[nx]);
            cv_text(12, 70, &FONT_XS, buf, SP_SAGE_D);
            text_r(228, 70, &FONT_XS, spr.level < SP_LEVELS_BUILT + 1 && nx + 1 > SP_LEVELS_BUILT ? "in a later build" : "",
                   SP_SOFT);
        }
    } else if (spr.row == MR_DESK || spr.row == MR_SHOP) {
        cv_text(12, 2, &FONT_XS, "a dub desk on the grown-ups' riddim:", SP_INK);
        cv_text(12, 18, &FONT_XS, "black keys drop, white keys throw,", SP_SOFT);
        cv_text(12, 33, &FONT_XS, "black keys 9-11 the siren. HOME there:", SP_SOFT);
        cv_text(12, 48, &FONT_XS, "the workshop, LOOM-8's own pages", SP_SOFT);
    } else {
        cv_text(12, 2, &FONT_XS, "the garden: the keys, KNOB 1 free / garden", SP_INK);
    }
    cv_rect(12, 92, 216, 1, SP_FAINT);
    text_c(120, 98, &FONT_XS, "SELECT moves   KNOB 1 changes   SEL opens", SP_SOFT);
    cv_blit(0, 120);
}

static void spr_draw_desk(void)
{
    static float m_db[NALLTRK];
    static uint32_t m_last;
    uint32_t now = plat_ms(), dt = now - m_last;
    int beat = playing() ? (eng_step[AX_DRUM] / 4) % 4 : -1;
    float fall = (float)(dt > 1000u ? 1000u : dt) * 0.03f;
    char buf[16];
    uint32_t h = hash_u(hash_u(2166136261u, 0xDE5Cu), (uint32_t)seq.mute | (uint32_t)(beat + 1) << 16 |
                        (uint32_t)playing() << 20 | (uint32_t)spr.siren << 21);
    m_last = now;
    meters_take();
    for (int t = 0; t < NALLTRK; t++) {
        float db = lin_db((float)ui_peak[t] * (1.0f / 16383.5f));
        if (dt > 250u)
            m_db[t] = MIX_DB_LO - 1.0f;
        m_db[t] = fm_maxf(db, m_db[t] - fall);
        h = hash_u(h, (uint32_t)mix_y(m_db[t]) | (uint32_t)(t < NTRK ? spr.thr[t] : 0) << 8);
    }
    h = hash_u(h, proj.tempo_x10);
    for (int k = 0; k < 4; k++)
        h = hash_u(h, k < 3 ? proj.trk[AUX_FX1].par[TP_E1 + (k == 0 ? 0 : k == 1 ? 2 : 3)] : proj.trk[AUX_FX2].par[TP_E1]);
    if (h == spr.sig)
        return;
    spr.sig = h;
    cv_begin(240, 120, AX_BG);                          /* the top: the name, the tempo, the beats; the cabinets */
    cv_text(6, 2, &FONT_B, "SMALL AXE", AX_GOLD);
    tempo_text(buf);
    text_r(234, 2, &FONT_B, buf, playing() ? AX_GREEN : AX_DIM);
    for (int b = 0; b < 4; b++)
        cv_round(98 + b * 12, 7, 9, 6, 2, b == beat ? (b ? AX_GREEN : AX_GOLD) : AX_PANEL);
    for (int t = 0; t < NTRK; t++) {
        int32_t x = 4 + t * 29, cx = x + 14, cy = 50, lv = mix_y(m_db[t]);
        int muted = seq.mute >> t & 1u, thrown = spr.thr[t] != 0;
        uint16_t ink = muted ? AX_RED : thrown ? AX_GOLD : AX_GREEN;
        float cone = 3.0f + (float)lv * (7.0f / (float)MIX_H);
        if (thrown)
            cv_frame(x, 22, 28, 66, 4, AX_GOLD, AX_BG);
        cv_round(x + 3, 25, 22, 60, 3, AX_PANEL);
        cv_dotf((float)cx, (float)cy, 10.0f, muted ? AX_PANEL2 : RGB(66, 52, 38));
        cv_dotf((float)cx, (float)cy, muted ? 8.0f : 8.5f, AX_BG);
        if (muted) {
            cv_dotf((float)cx, (float)cy, 3.0f, AX_RED);
        } else {
            cv_dotf((float)cx, (float)cy, cone, ink);
            cv_dotf((float)cx, (float)cy, 2.0f, AX_BG);
        }
        cv_dotf((float)cx, 74.0f, 3.0f, muted ? AX_PANEL2 : RGB(66, 52, 38));   /* the tweeter */
        text_c(cx, 92, &FONT_XS, proj.rsv[9] == AXE_MAGIC ? AX_NAME[t] : TRACK_NAME[t], muted ? AX_RED : AX_TEXT);
    }
    cv_blit(0, 0);
    cv_begin(240, 120, AX_BG);                          /* the bottom: the returns, the four knobs, the keys' jobs */
    for (int k = 0; k < 2; k++) {
        int32_t x = k ? 128 : 8, lv = mix_y(m_db[AUX_FX1 + k]) * 104 / MIX_H;
        cv_text(x, 0, &FONT_XS, k ? "SPRING" : "ECHO", AX_DIM);
        cv_round(x, 16, 104, 6, 2, AX_PANEL);
        if (lv > 0)
            cv_round(x, 16, lv, 6, 2, AX_GOLD);
    }
    {
        static const char *const KN[4] = {"time", "repeats", "tone", "spring"};
        for (int k = 0; k < 4; k++) {
            int32_t x = 8 + k * 58;
            int v = k < 3 ? proj.trk[AUX_FX1].par[TP_E1 + (k == 0 ? 0 : k == 1 ? 2 : 3)] : proj.trk[AUX_FX2].par[TP_E1];
            cv_round(x, 34, 50, 34, 5, AX_PANEL);
            text_c(x + 25, 37, &FONT_XS, KN[k], AX_DIM);
            num_s((uint32_t)v, buf);
            text_c(x + 25, 50, &FONT_B, buf, AX_TEXT);
        }
    }
    text_c(120, 76, &FONT_XS, "black 1-8 drop   white 1-8 echo   9-16 spring", AX_DIM);
    text_c(120, 92, &FONT_XS, "black 9-11 siren   PLAY   HOME the workshop", AX_DIM);
    cv_blit(0, 120);
}

/* ---- the garden's frame ------------------------------------------------------------------------------------------ */

static void spr_garden_set(int on)
{
    if (on == spr.garden)
        return;
    spr.garden = (uint8_t)on;
    if (on) {
        spr.bar_seen = 0xFF;
        for (int b = 0; b < 4; b++)
            spr_bell_bar(b);
        engine_transport(3);
    } else {
        engine_transport(0);
        engine_keys_off(GT_BELL);
        engine_keys_off(GT_DRONE);
    }
}
static void spr_garden_frame(void)
{
    uint32_t keys = plat_keys(), ch = keys ^ spr.keys;
    int32_t e = plat_enc(EN_K1);
    for (int i = 0; i < NE; i++)
        if (i != EN_K1)
            (void)plat_enc(i);         /* (the other knobs: nothing yet) */
    spr.keys = keys;
    for (int k = 0; k < NKEYS; k++)
        if (ch >> k & 1u) {
            if (keys >> k & 1u) {
                engine_key(GT_KEYS, KEY_BASE + k, 1, 118);
                spr.shown = (int8_t)k;
            } else
                engine_key(GT_KEYS, KEY_BASE + k, 0, 0);
        }
    if (e)
        spr_garden_set(e > 0);
    if (spr.garden) {                    /* the bed never repeats: two bars ahead of the playhead, a new bar */
        int bar = (eng_step[GT_BELL] % NSTEP) / 16;
        if (bar != spr.bar_seen) {
            if (spr.bar_seen != 0xFF)
                spr_bell_bar((bar + 2) % 4);
            spr.bar_seen = (uint8_t)bar;
        }
    }
    {   /* the lights: the keys held; while the garden plays every white key glows, the bell's brighter */
        static const uint8_t W[16] = PLAT_WHITE_KEYS;
        uint32_t whites = 0;
        int bell = spr_bell_key();
        for (int i = 0; i < 16; i++)
            whites |= 1u << W[i];
        plat_lights(0, keys, 0, spr.garden ? whites : 0u, spr.garden ? 1 : 0);
        plat_lights_mid(0, bell >= 0 ? 1u << bell : 0u);
    }
    spr_draw_garden();
}

/* ---- the menu's frame ---------------------------------------------------------------------------------------------- */

static void spr_menu_change(int d)
{
    switch (spr.row) {
    case MR_LEVEL:                      /* (this build: level 1 only) */
        if (spr.level + d >= 1 && spr.level + d <= SP_LEVELS_BUILT && spr.level + d != spr.level) {
            spr.level = (uint8_t)(spr.level + d);
            spr.dirty = 1;
        }
        break;
    case MR_NAMES:
        spr.names = (uint8_t)((spr.names + (d > 0 ? 1 : NNM - 1)) % NNM);
        spr.dirty = 1;
        break;
    default:
        break;
    }
    spr.sig = 0;
}
static void spr_go_garden(void)
{
    if (spr.from != SPM_GARDEN || proj.rsv[9] != SP_MAGIC) {
        if (proj.rsv[9] == AXE_MAGIC && proj_slot < NPROJ && ui.dirty)
            save_project(proj_slot);   /* (the riddim as left: the workshop's autosave may not have come round) */
        spr_garden_load();
    } else
        spr_settings_save();
    spr_mode(SPM_GARDEN);
}
static void spr_menu_open_row(void)
{
    switch (spr.row) {
    case MR_DESK:
    case MR_SHOP:
        if (proj.rsv[9] == SP_MAGIC) {
            spr_settings_save();
            spr_axe_load();
        }
        spr_mode(spr.row == MR_DESK ? SPM_DESK : SPM_SHOP);
        break;
    case MR_GARDEN:
        spr_go_garden();
        break;
    default:
        break;
    }
}
static void spr_menu_frame(void)
{
    uint32_t b = plat_buttons(), dn = b & ~spr.btn;
    int32_t e;
    spr.btn = b;
    spr.keys = plat_keys();
    if ((e = plat_enc(EN_SELECT)) != 0) {
        spr.row = (uint8_t)((spr.row + (e > 0 ? 1 : NMR - 1)) % NMR);
        spr.sig = 0;
    }
    for (int i = EN_ALGO; i < NE; i++)
        if ((e = plat_enc(i)) != 0) {
            if (spr.row == MR_LEVEL || spr.row == MR_NAMES)
                spr_menu_change(e > 0 ? 1 : -1);
        }
    if (dn >> B_SEL & 1u || dn >> B_PLAY & 1u)
        spr_menu_open_row();
    else if ((dn >> B_HOME & 1u) && !(b >> B_GLO & 1u)) {   /* HOME alone: back where it came from */
        if (spr.from == SPM_GARDEN)
            spr_go_garden();
        else
            spr_mode(spr.from);
    }
    if (spr.mode == SPM_MENU) {
        plat_lights(1u << B_SEL | 1u << B_HOME, 0, 0, 0, 0);
        plat_lights_mid(0, 0);
        spr_draw_menu();
    }
}

/* ---- the desk's frame --------------------------------------------------------------------------------------------- */

#define AX_TAP_MS 300u                  /* a drop key let go sooner: a tap, the drop stays (later: back as it was) */
static void ax_drop(int ch, int down)   /* a channel out at once; let go: a tap toggles it, a hold was a cut */
{
    uint32_t m = 1u << ch;
    if (down) {
        spr.drop_was = (uint8_t)((spr.drop_was & ~m) | (seq.mute >> ch & 1u ? m : 0u));
        spr.drop_held |= (uint8_t)m;
        spr.drop_t[ch] = plat_ms();
        if (!(seq.mute >> ch & 1u))
            set_mute(ch, 1);
        return;
    }
    if (!(spr.drop_held & m))
        return;
    spr.drop_held &= (uint8_t)~m;
    if (plat_ms() - spr.drop_t[ch] < AX_TAP_MS)
        set_mute(ch, !(spr.drop_was >> ch & 1u));
    else
        set_mute(ch, spr.drop_was >> ch & 1u);
}
static void ax_throw(int ch, int slot, int on)   /* a channel's send to the echo (0) or the spring (1) full while held */
{
    if (is_drum(ch)) {                  /* a kit: each key's own send */
        for (int k = 0; k < DL_NSOUND; k++) {
            int p = TP_DRUM(k, slot ? DP_SEND2 : DP_SEND1);
            engine_set(ch, p, on ? 120 : proj.trk[ch].par[p]);
        }
    } else {
        int p = slot ? TP_SEND2 : TP_SEND1;
        engine_set(ch, p, on ? 127 : proj.trk[ch].par[p]);
    }
    spr.thr[ch] = (uint8_t)(on ? slot + 1 : 0);
}
static void ax_knob(int k, int32_t e)   /* KNOBS 1-4: the echo's time, repeats, tone; the spring's size */
{
    int t = k < 3 ? AUX_FX1 : AUX_FX2, p = TP_E1 + (k == 0 ? 0 : k == 1 ? 2 : k == 2 ? 3 : 0);
    int v = clampi((int)proj.trk[t].par[p] + (int)e * 2, 0, 127);
    proj.trk[t].par[p] = (uint8_t)v;
    apply_aux(t);
    ui.dirty = 1;
}
static void spr_desk_frame(void)
{
    static const uint8_t W[16] = PLAT_WHITE_KEYS, BK[11] = PLAT_BLACK_KEYS;
    static const uint8_t SIREN[3] = {62, 69, 74};
    uint32_t b = plat_buttons(), dn = b & ~spr.btn, keys = plat_keys(), ch = keys ^ spr.keys, lit = 0;
    int32_t e;
    spr.btn = b;
    spr.keys = keys;
    if (dn >> B_PLAY & 1u)
        engine_transport(playing() ? 0 : 1);
    if ((dn >> B_HOME & 1u) && !(b >> B_GLO & 1u)) {
        for (int t = 0; t < NTRK; t++)
            if (spr.thr[t])
                ax_throw(t, spr.thr[t] - 1, 0);
        spr_mode(SPM_SHOP);
        return;
    }
    for (int i = 0; i < 16; i++)
        if (ch >> W[i] & 1u)
            ax_throw(i % 8, i / 8, keys >> W[i] & 1u);
    for (int i = 0; i < 11; i++)
        if (ch >> BK[i] & 1u) {
            int down = keys >> BK[i] & 1u;
            if (i < 8)
                ax_drop(i, down);
            else {
                engine_key(AX_SIRN, SIREN[i - 8], down, down ? 112 : 0);
                spr.siren = (uint8_t)(down ? spr.siren | 1u << (i - 8) : spr.siren & ~(1u << (i - 8)));
            }
        }
    if ((e = plat_enc(EN_SELECT)) != 0) {
        set_tempo_x10((int)proj.tempo_x10 + accel(EN_SELECT, e) * 5);
        ui.dirty = 1;
    }
    (void)plat_enc(EN_ALGO);
    (void)plat_enc(EN_PRESET);
    for (int k = 0; k < 4; k++)
        if ((e = plat_enc(EN_K1 + k)) != 0)
            ax_knob(k, e);
    for (int t = 0; t < NTRK; t++) {    /* the lights: a channel in, its black key; thrown, its white key */
        if (!(seq.mute >> t & 1u))
            lit |= 1u << BK[t];
        if (spr.thr[t])
            lit |= 1u << W[t + (spr.thr[t] - 1) * 8];
    }
    for (int i = 0; i < 3; i++)
        if (spr.siren >> i & 1u)
            lit |= 1u << BK[8 + i];
    plat_lights(playing() ? LED_PLAY_GREEN : 0u, lit, 1u << B_HOME | 1u << B_PLAY, 0, 1);
    plat_lights_mid(0, 0);
    if (ui.dirty && proj_slot < NPROJ && !playing() && !(keys | b) && eng_voices == 0 && eng_quiet >= AUTOSAVE_QUIET &&
        plat_ms() - ui.act_t >= AUTOSAVE_MS && !(proj.rsv[2] & AUTOSAVE_OFF)) {
        if (project_save(proj_slot) == 0)      /* the desk's knobs and drops kept, as the workshop's autosave would */
            ui.dirty = 0;
    }
    if (b || keys || ch)
        ui.act_t = plat_ms();
    spr_draw_desk();
}

/* ---- the frame (ui_frame's first call) ---------------------------------------------------------------------------- */

static void spr_reset(void)            /* (the simulator's reboot: what RAM forgets) */
{
    memset(&spr, 0, sizeof spr);
    spr.mode = SPM_BOOT;
    spr.level = 1;
    spr.gslot = spr.aslot = SP_NOSLOT;
    spr.shown = -1;
    spr.bar_seen = 0xFF;
    spr.walk = 2;
    spr.rnd = 0x2545F491u;
    spr.pend_slot = SP_NOSLOT;
}
static void spr_boot(void)
{
    spr_find_slots();
    spr_garden_load();
    spr_panel_take();
    spr.mode = SPM_GARDEN;
    lcd_fill(0, 0, 240, 240, SP_PAPER);
}
/* 1: the frame was sprout's (LOOM-8's pages stay away); 0: the workshop, LOOM-8's */
static int sprout_frame(void)
{
    if (spr.mode == SPM_BOOT) {
#if defined(PLAT_HOST) && !defined(PLAT_WEB)
        if (getenv("SPROUT_SHOP")) {    /* (the simulator: LOOM-8's own scenarios, on SMALL AXE's workshop) */
            spr.mode = SPM_SHOP;
            return 0;
        }
#endif
        spr_boot();
    }
    spr_pending();
    if (spr_combo() && spr.mode != SPM_MENU) {
        if (spr.mode == SPM_SHOP)
            spr_panel_take();
        spr_menu_open();
    }
    switch (spr.mode) {
    case SPM_GARDEN:
        spr_garden_frame();
        return 1;
    case SPM_MENU:
        spr_menu_frame();
        return spr.mode != SPM_SHOP;
    case SPM_DESK:
        spr_desk_frame();
        return spr.mode != SPM_SHOP;
    default:
        return 0;
    }
}

/* the boot's picture: a seed sprouting, the name under it */
static void sprout_splash(void)
{
    lcd_fill(0, 0, 240, 240, SP_PAPER);
    cv_begin(240, 120, SP_PAPER);
    spr_seedling(120, 94, 3.4f, 1);
    cv_blit(0, 40);
    cv_begin(240, 60, SP_PAPER);
    {
        int32_t w = text_w(&FONT_L, "sprout");
        cv_text((240 - w) / 2, 0, &FONT_L, "sprout", SP_INK);
        w = text_w(&FONT_S, "a sound garden");
        cv_text((240 - w) / 2, 36, &FONT_S, "a sound garden", SP_SOFT);
    }
    cv_blit(0, 166);
}

#endif /* SPROUT */
