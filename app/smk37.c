/* SPDX-License-Identifier: GPL-3.0-only */
/* LOOM-8 on the M-VAVE SMK-37 (fm1-base BOARD=smk37, plat.h PLAT_SMK37): what the SMK's panel adds to the FM-1's. Part
 * of app.c's build after ui.c (its helpers); ui.c calls lsmk_poll at the start of input(), takes lsmk_pad_keys into its
 * keys, plays KEY_VEL, draws lsmk_lights after leds(), and hands every frame to lsmk_daw_frame first (DAW mode). The
 * FM-1's controls keep their jobs (fm1-base's panel map: K1 the volume, K2 SELECT, K3 ALGORITHM, K4 PRESETS, K5..K8
 * KNOB 1..4; the middle 27 keys, with velocity).
 *
 *   pads        on Steps and Bar (SEQ), the bar's 16 cells, pads 1-8 the top row: the white keys' job there, every
 *               gesture of theirs (a tap, a hold, a later one for the length); lit in the track's colour: a step with
 *               notes dim, the step playing bright, a step held white. Elsewhere, or after KNOB BANK there: pads 1-8
 *               (the top row) mute T1-T8 (a tap; held: solo; two taps: a solo that stays), lit while the track plays; pads 9-16 (the bottom row) the drums: the kit's
 *               first eight sounds, whatever track is selected (the kit: the track selected if it is one, else the
 *               kit selected last, else the first), with velocity, recorded into the kit while recording
 *   NOTE REPEAT tapped: note repeat on / off (lit): the drum pads held sound again in time (seq.c lrp_*: on the
 *               grid while playing, and recorded while recording); held: pads 1-8 pick its rate (1/4 1/8 1/8T 1/16
 *               1/16T 1/32 1/32T 1/64, lit while held; a pick turns it on); held and let go alone: the LFO page (the
 *               FM-1's LFO button, where the SMK prints NOTE REPEAT)
 *   KNOB BANK   the pads: cells -> live pads, mutes and drums (lit) on Steps, again the cells; elsewhere opens Steps,
 *               the pads its cells
 *   < >         the bar before / after (any page: the one Steps shows and the pads write), when let go
 *   < + >       DAW mode (below)
 *   faders      the levels of T1-T4, or T5-T8 (FADER BANK, lit on T5-T8); a fader acts once it moves
 *   outer keys  C3..E3 and G#5..C6 carry on the keys: more notes, with velocity; on a kit, its keys
 *   pitch wheel bend of the track the keys play (+-7 semitones, a fifth)
 *   mod wheel   MOD WHEEL (Setup): FILTER, the filter of the track the keys play (its cutoff; DYNAMIC: its hi cut),
 *               once the wheel passes the value it has (a pick-up); or CONTROL ALL: cutoff, LFO amount, send I, send II
 *               or decay on every synth track at once, each moved from its own value; KNOB BANK within 5 s of the
 *               wheel's last move puts them back
 *
 * DAW mode (< + > together; ported from Snoop37 by way of the Loom FM-1 port; smk_daw.h has the map): the SMK is a
 * USB MIDI controller and LOOM-8 is left alone (the sequencer stops on the way in; MIDI IN still plays it, a sound
 * module for the DAW). The keys notes on CHANNEL (OCT- / OCT+ the octave, both: 0); K1..K8 (KNOB BANK: A / B), the
 * faders (FADER BANK: A / B), the pads (KNOB BANK + FADER BANK together: A / B), 13 buttons and the wheels what the map
 * says. K1 is a knob here, not the volume. < and > send when let go. PATCH tapped: LOCAL (the keys, NOTE pads and
 * the pitch wheel also play LOOM-8). PATCH held: K5 CHANNEL, K6 CC MAP (DEFAULT / USER / REAPER: Mackie Control on the
 * second USB port, below), K7 KNOBS (ABSOLUTE /
 * RELATIVE), K8 KEY VELOCITY (as Setup's in LOOM-8: the keys' touch, or fixed); + OCT+ the map editor: touch a control, K5 its TYPE, K6 its NUMBER, K7 its CHANNEL; OCT+ the next
 * control, OCT- or PATCH done. Kept in flash (OBJ_DAW): the mode, the settings, the USER map. */

static struct {
    uint32_t latch;                     /* Steps: pads pressed and not yet seen by input() (a tap within a frame) */
    uint8_t pad_trk[NPADS], pad_note[NPADS];   /* a drum pad's sound playing: its kit and key (0 none) */
    uint8_t pad_vel[NPADS], pad_rp[NPADS];   /* its velocity; repeating (seq_repeat) */
    uint8_t nr_on, nr_rate;             /* NOTE REPEAT: on, its rate (SEQ_RP_*) */
    uint32_t nr_pads;                   /* pads pressed as a rate while NOTE REPEAT was held (not cells) */
    uint8_t last_kit;                   /* the kit the drum pads play when a synth is selected (0xFF: none yet) */
    uint8_t pad_drums;                  /* KNOB BANK: the pads drum pads on Steps too */
    uint8_t mute_held;                  /* the top row's mute pads held (bit p) since mute_t[p] */
    uint32_t mute_t[8];
    uint8_t fbank;                      /* the faders: 0 T1-T4, 1 T5-T8 */
    uint8_t flast[4];                   /* (soft takeover: 0xFF waits for the fader to move) */
    uint8_t out_trk[NOUTER], out_note[NOUTER];   /* an outer key's note playing: track and note (0 none) */
    uint8_t bend_trk;                   /* the track the pitch wheel bends (0xFF none) */
    int32_t bend;
    uint8_t mw_trk, mw_par, mw_last, mw_armed;   /* the mod wheel: its track and parameter, as last seen, picked up */
    uint8_t ca_on, ca_mode, ca_w0;      /* CONTROL ALL: a move under way (revertible), its mode, the wheel at its start */
    uint8_t ca_snap[NTRK];              /* each track's value at its start (KNOB BANK puts them back) */
    uint8_t ca_trks;                    /* the tracks it moves (bit t) */
    uint32_t ca_t;                      /* the wheel's last move (ms) */
    uint8_t lr_both;                    /* < + > pressed together: neither steps a bar (until both are up) */
    uint32_t sbtn;                      /* the SMK buttons as last seen */
} lsmk = {.last_kit = 0xFF, .flast = {0xFF, 0xFF, 0xFF, 0xFF}, .bend_trk = 0xFF, .mw_trk = 0xFF, .nr_rate = SEQ_RP_16};
static const char *const LSMK_RATE[NSEQ_RP] = {"1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T", "1/64"};
#define LSMK_NR_TAP_MS 400u                     /* NOTE REPEAT let go sooner: a tap (on / off); later: the LFO page */

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

/* NOTE REPEAT (the FM-1's LFO button) held: the top row picks the rate */
static int lsmk_nr_held(void) { return (plat_buttons() >> B_LFO & 1u) && !daw_on && !ui.ask; }

/* a drum pad's sound and its repeat end */
static void lsmk_drum_off(int p)
{
    if (lsmk.pad_rp[p]) {
        engine_repeat(lsmk.pad_trk[p], lsmk.pad_note[p], 0, 0);
        lsmk.pad_rp[p] = 0;
    }
    if (lsmk.pad_note[p]) {
        engine_key(lsmk.pad_trk[p], lsmk.pad_note[p], 0, 0);
        lsmk.pad_note[p] = 0;
    }
}
/* note repeat on / off: the drum pads held start / stop repeating */
static void lsmk_nr_set(int on)
{
    int p;
    lsmk.nr_on = (uint8_t)(on != 0);
    for (p = 8; p < NPADS; p++)
        if (lsmk.pad_note[p] && lsmk.nr_on != lsmk.pad_rp[p]) {
            engine_repeat(lsmk.pad_trk[p], lsmk.pad_note[p], lsmk.nr_on, lsmk.pad_vel[p]);
            lsmk.pad_rp[p] = lsmk.nr_on;
        }
    say("NOTE REPEAT", lsmk.nr_on ? LSMK_RATE[lsmk.nr_rate] : "off");
}
/* NOTE REPEAT let go, unused (no rate picked): a tap turns it on / off (1: taken); a hold leaves it to ui.c (LFO) */
static int lsmk_nr_release(uint32_t held_ms)
{
    if (daw_on || held_ms >= LSMK_NR_TAP_MS)
        return 0;
    lsmk_nr_set(!lsmk.nr_on);
    return 1;
}
static int lsmk_nr_lit(void) { return lsmk.nr_on; }

/* a mute pad (the top row) as Mix's black keys: a tap mutes or unmutes, held soloes while held (more held: more
 * soloed), two taps solo and it stays (a tap then ends it) */
#define LSMK_SOLO_MS 400u                          /* a mute pad held this long: its track soloed while held */
static void lsmk_mute_up(int b, uint32_t t0)
{
    uint32_t now = plat_ms();
    if (now - t0 < LSMK_SOLO_MS && (!ui.solo || ui.solo_lat)) {   /* a tap (not while others are held) */
        int twice = ui.solo_tap_trk == b && t0 - ui.solo_tap_t < SOLO_TWICE_MS, muted = ui.solo_tap_mut;
        ui.solo_tap_trk = twice ? 0xFF : (uint8_t)b;
        ui.solo_tap_t = now;
        ui.solo_tap_mut = (uint8_t)(!twice && !ui.solo_lat);
        if (twice) {                               /* a double tap: soloed, and it stays (the first tap's mute undone) */
            if (muted)
                set_mute(b, !(seq.mute >> b & 1u));
            solo_end();
            solo_begin(b);
            ui.solo_lat = (uint16_t)(1u << b);
            say("SOLO", TRACK_NAME[b]);
        } else if (ui.solo_lat) {                  /* a tap while a solo stays: it ends, the mutes back */
            solo_end();
            say("SOLO OFF", 0);
        } else {
            set_mute(b, !(seq.mute >> b & 1u));
            say(seq.mute >> b & 1u ? "MUTED" : "UNMUTED", TRACK_NAME[b]);
        }
    } else if (ui.solo) {                          /* let go after a hold: that track leaves the solo; the last, the
                                                    * mutes back */
        ui.solo &= (uint16_t)~((1u << b) & ~ui.solo_lat);
        if (!ui.solo) {
            ui.solo = 1;
            solo_end();
        } else
            seq.mute = (uint16_t)(((1u << NSEQT) - 1u) & ~ui.solo);
    }
}
static void lsmk_mute_holds(void)                  /* every pass: a mute pad held long enough solos its track */
{
    uint32_t now = plat_ms();
    for (int p = 0; p < 8 && p < NSEQT; p++)
        if ((lsmk.mute_held >> p & 1u) && now - lsmk.mute_t[p] >= LSMK_SOLO_MS && !(ui.solo >> p & 1u)) {
            solo_begin(p);
            say("SOLO", TRACK_NAME[p]);
        }
}

static void lsmk_pads(void)
{
    int p, v;
    while (plat_pad(&p, &v)) {
        uint32_t m;
        if (p < 0 || p >= NPADS)
            continue;
        m = 1u << p;
        if (v) {
            if (p < 8 && lsmk_nr_held()) {         /* NOTE REPEAT held: a rate (and repeat on) */
                lsmk.nr_pads |= m;
                ui.btn_used |= 1u << B_LFO;         /* (no tap, no LFO page when it is let go) */
                ui.card = 0;
                lsmk.nr_rate = (uint8_t)p;
                engine_repeat_rate(p);
                lsmk_nr_set(1);
                continue;
            }
            if (lsmk_pads_steps()) {               /* a cell: its white key (lsmk_pad_keys) */
                lsmk.latch |= m;
                continue;
            }
            if (p < 8) {                           /* the top row: T1-T8's mutes (a tap), solos (held): at the let go */
                if (p < NTRK && p < NSEQT) {
                    lsmk.mute_held |= (uint8_t)m;
                    lsmk.mute_t[p] = plat_ms();
                }
            } else {                               /* the bottom row: the kit's sounds 1-8 (its keys), a pad each */
                int kit = lsmk_kit(), key = p - 7;
                lsmk_drum_off(p);
                if (kit < 0) {
                    say_no("NO KIT", "make a track a kit");
                    continue;
                }
                lsmk.pad_trk[p] = (uint8_t)kit;
                lsmk.pad_note[p] = (uint8_t)key;
                lsmk.pad_vel[p] = (uint8_t)v;
                engine_key(kit, key, 1, v);        /* (recorded while recording: seq.c key_in) */
                if (lsmk.nr_on) {
                    engine_repeat(kit, key, 1, v);
                    lsmk.pad_rp[p] = 1;
                }
                ui.dsound = (uint8_t)(kit == sel_trk() ? key - 1 : ui.dsound);   /* (the kit's pages: the key shown) */
            }
        } else {                                   /* let go: what it started ends, whatever the page is now */
            lsmk.nr_pads &= ~m;
            lsmk_drum_off(p);
            if (p < 8 && (lsmk.mute_held >> p & 1u)) {
                lsmk.mute_held &= (uint8_t)~m;
                lsmk_mute_up(p, lsmk.mute_t[p]);
            }
        }
    }
    lsmk_mute_holds();
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
        if (held >> c & 1u && !(lsmk.nr_pads >> c & 1u))
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
            if (lsmk.out_note[i]) {
                engine_key(lsmk.out_trk[i], lsmk.out_note[i], 0, 0);
                outer_note(lsmk.out_trk[i], lsmk.out_note[i], 0);
            }
            lsmk.out_trk[i] = (uint8_t)t;
            lsmk.out_note[i] = (uint8_t)n;
            engine_key(t, n, 1, (int)daw_kv_vel((uint32_t)v));
            outer_note(t, n, 1);                   /* (with the keyboard's: shown held, a step's, step record's) */
        } else if (lsmk.out_note[i]) {
            engine_key(lsmk.out_trk[i], lsmk.out_note[i], 0, 0);
            outer_note(lsmk.out_trk[i], lsmk.out_note[i], 0);
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
        say("PADS LIVE", "mutes and drums");
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
static int lsmk_ca_revert(void);
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
        if (!lsmk.lr_both)
            lsmk_bar(released >> SB_RIGHT & 1u ? 1 : -1);
        if (!(b & lr))
            lsmk.lr_both = 0;
    }
    if ((pressed >> SB_KNOB_BANK & 1u) && !lsmk_ca_revert())   /* (CONTROL ALL just moved: KNOB BANK reverts) */
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

/* CONTROL ALL's parameter for a mode */
static int lsmk_ca_par(int mode)
{
    static const uint8_t P[MW_N] = {TP_CUT, TP_CUT, TP_LAMT, TP_SEND1, TP_SEND2, TP_D};
    return P[mode % MW_N];
}
/* the mod wheel as CONTROL ALL: the move's start (the wheel was at w0) snapshots the parameter on every synth track
 * (a kit's sends and envelope are its keys'; the FX tracks have none); each track then moves from its own value by
 * the wheel's travel since (the whole range for the wheel's). A pause of MW_REVERT_MS ends the move: kept */
static void lsmk_ca(uint32_t m)
{
    int p = lsmk_ca_par(daw_mw), t, span, v;
    uint32_t now = plat_ms();
    const tp_info_t *in;
    char s[24];
    if (!lsmk.ca_on || lsmk.ca_mode != daw_mw || now - lsmk.ca_t > MW_REVERT_MS) {
        lsmk.ca_on = 1;
        lsmk.ca_mode = daw_mw;
        lsmk.ca_w0 = lsmk.mw_last;
        lsmk.ca_trks = 0;
        for (t = 0; t < NTRK && t < 8; t++)
            if (!is_aux(t) && !is_drum(t)) {
                lsmk.ca_snap[t] = proj.trk[t].par[p];
                lsmk.ca_trks |= (uint8_t)(1u << t);
            }
    }
    lsmk.ca_t = now;
    lsmk.mw_last = (uint8_t)m;
    for (t = 0; t < NTRK && t < 8; t++)
        if (lsmk.ca_trks >> t & 1u) {
            in = tp_info_par(p, proj.trk[t].par);
            span = in->hi - in->lo;
            v = clampi((int)lsmk.ca_snap[t] + ((int)m - (int)lsmk.ca_w0) * span / 127, in->lo, in->hi);
            if (v != proj.trk[t].par[p])
                set_tp(t, p, v);
        }
    {
        int d = ((int)m - (int)lsmk.ca_w0) * 100 / 127;
        char *e = s_cat(s, MW_NAME[daw_mw % MW_N]);
        e = s_cat(e, d < 0 ? " -" : " +");
        s_cat(num_s((uint32_t)(d < 0 ? -d : d), e), "%");
    }
    say("CONTROL ALL", s);
}
/* KNOB BANK within MW_REVERT_MS of CONTROL ALL's last move: every track's value back as the move found it (1: done,
 * KNOB BANK's own job skipped) */
static int lsmk_ca_revert(void)
{
    int p, t;
    if (!lsmk.ca_on || plat_ms() - lsmk.ca_t > MW_REVERT_MS)
        return 0;
    p = lsmk_ca_par(lsmk.ca_mode);
    for (t = 0; t < NTRK && t < 8; t++)
        if (lsmk.ca_trks >> t & 1u)
            set_tp(t, p, lsmk.ca_snap[t]);
    lsmk.ca_on = 0;
    say("REVERTED", MW_NAME[lsmk.ca_mode % MW_N]);
    return 1;
}
/* the mod wheel: CONTROL ALL (above), or FILTER: the track the keys play, its cutoff (DYNAMIC: its hi cut). A pick-up:
 * the wheel acts once it passes the value the track has (another track, or another filter type: picked up again) */
static void lsmk_modw(void)
{
    int t = lsmk_play_trk(), p, span;
    uint32_t m = plat_modw(), cur;
    const tp_info_t *in;
    char s[24];
    if (daw_mw != MW_FILTER) {
        if (m != lsmk.mw_last)
            lsmk_ca(m);
        lsmk.mw_trk = 0xFF;                       /* (FILTER again: picked up afresh) */
        return;
    }
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

/* K1 is the volume (fm1-base's MASTER): its level on the screen as it turns */
static void lsmk_volume(void)
{
    static uint32_t last = 0xFFFFFFFFu;
    uint32_t v = plat_master();
    char s[16];
    if (last == 0xFFFFFFFFu || plat_ms() < 3000u) {   /* (the level at boot is not a turn) */
        last = v;
        return;
    }
    if (v == last)
        return;
    last = v;
    s_cat(num_s((v * 100u + 2048u) / 4096u, s), "%");
    say("VOLUME", s);
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
    lsmk_volume();
}

/* the pads' and the SMK buttons' lights, after leds() */
/* an RGB565 ink as the pads' LEDs show it, at lv / 256. The screen's values are gamma-coded and an LED's are linear, so
 * as they were a pale ink's every channel lit and the colour washed out to white: each channel cubed (the gamma
 * undone: 2.8 is the usual for such LEDs), then scaled back so the brightest keeps its value (amber 228,182,82 ->
 * 228,116,10; cold blue 104,142,232 -> 20,53,232; a grey stays a grey) */
static uint32_t lsmk_rgb(uint16_t c, uint32_t lv)
{
    uint32_t r = (c >> 11) << 3 | (c >> 13), g = ((c >> 5) & 0x3Fu) << 2 | ((c >> 9) & 3u), b = (c & 0x1Fu) << 3 | ((c >> 2) & 7u);
    uint32_t mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    if (mx) {
        uint32_t m2 = mx * mx;
        r = r * r * r / m2, g = g * g * g / m2, b = b * b * b / m2;   /* (x^3 / 255^2, the brightest back to mx) */
    }
    return (r * lv >> 8) << 16 | (g * lv >> 8) << 8 | (b * lv >> 8);
}
/* the pads at rest away from Steps (nothing held): the mutes lit while their track plays, the drums dim in the kit's
 * ink (dark with no kit) */
static void lsmk_pad_rest(uint32_t *col)
{
    int c, kit = lsmk_kit();
    for (c = 0; c < 8; c++)
        col[c] = c >= NTRK ? 0u : lsmk_rgb(trk_ink(c), seq.mute >> c & 1u ? 10u : 200u);
    for (c = 8; c < NPADS; c++)
        col[c] = kit < 0 ? 0u : lsmk_rgb(trk_ink(kit), 44u);
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
    } else {                                   /* mutes (lit while the track plays) and the drums */
        int kit = lsmk_kit();
        uint32_t held = plat_pads_held(), col[NPADS];
        lsmk_pad_rest(col);
        for (c = 0; c < NPADS; c++)
            plat_pad_rgb(c, c >= 8 && held >> c & 1u && kit >= 0 ? lsmk_rgb(trk_ink(kit), 256u) : col[c]);
    }
    if (lsmk_nr_held())                        /* NOTE REPEAT held: the rates, the one in use bright */
        for (c = 0; c < 8; c++)
            plat_pad_rgb(c, lsmk_rgb(theme->ink[2], c == lsmk.nr_rate ? 240u : 40u));
    plat_smk_leds((lsmk.fbank ? 1u << SB_FADER_BANK : 0u) | (lsmk.pad_drums ? 1u << SB_KNOB_BANK : 0u));
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

static int lrp_active(void);                     /* (the REAPER profile, below) */
static void lrp_say(const char *what, const char *v);
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

/* ---- KEY VELOCITY (smk_daw.h daw_kv): Setup's row in LOOM-8, PATCH held + K8 in DAW mode; kept with DAW mode's
 * settings (OBJ_DAW), the touch handed to the keys at start and on each change (fm1-base plat_key_touch) */
static void lsmk_kv_apply(void) { plat_key_touch(KV_TOUCH[daw_kv % KV_N]); }
static void lsmk_kv_step(int d)
{
    int v = clampi((int)daw_kv + d, 0, KV_N - 1);
    if (v == daw_kv)
        return;
    daw_kv = (uint8_t)v;
    lsmk_kv_apply();
    lsmk_daw_dirty();
}
/* a middle key's velocity (ui.c KEY_VEL): at KEY VELOCITY; a pad on Steps (a cell, as its white key): its own */
static int lsmk_key_vel(int k)
{
    uint32_t v = plat_key_vel(k);
    return (int)(k >= 0 && k < NKEYS && (lsmk_padk >> k & 1u) ? v : daw_kv_vel(v));
}
/* the last key, "velocity 112 at 14.4 ms" (as KEY VELOCITY plays it; the stroke contact to contact), written at t: to
 * set KEY VELOCITY by ear and by number. 0: no key yet (nothing written) */
static int lsmk_kv_last(char *t)
{
    int vel, dt;
    char *e;
    if (plat_key_last(&vel, &dt) < 0)
        return 0;
    e = num_s(daw_kv_vel((uint32_t)vel), s_cat(t, "velocity "));
    if (dt > 0 && daw_kv < KV_FIX100) {
        e = num_s((uint32_t)dt / 10u, s_cat(e, " at "));
        s_cat(num_s((uint32_t)dt % 10u, s_cat(e, ".")), " ms");
    }
    return 1;
}
static void lsmk_mw_step(int d)                 /* MOD WHEEL: the next mode (a move under way ends, kept) */
{
    int v = clampi((int)daw_mw + d, 0, MW_N - 1);
    if (v == daw_mw)
        return;
    daw_mw = (uint8_t)v;
    lsmk.ca_on = 0;
    lsmk.mw_last = (uint8_t)plat_modw();
    lsmk_daw_dirty();
}
static void lsmk_list_keys(void)                /* Setup (pages.c list_setup): the last key, then KEY VELOCITY (the line
                                                 * above the row: in sight while the row is picked) */
{
    static char last[48];
    list_head("SMK-37 keys");
    if (!lsmk_kv_last(s_cat(last, "Last key: ")))
        s_cat(last, "Play a key to see its velocity");
    list_add(last, 0, LK_NOTE, 0);
    list_add(ACTION_NAME[A_KEY_VEL], KV_NAME[daw_kv % KV_N], LK_PICK, A_KEY_VEL);
    list_add(ACTION_NAME[A_MOD_WHEEL], MW_NAME[daw_mw % MW_N], LK_PICK, A_MOD_WHEEL);
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
        if (lrp_active()) {
            char u[20];
            num_s(daw_ch, s_cat(num_s((uint32_t)n, s_cat(u, "note ")), " ch "));
            lrp_say("KEYS", u);
        }
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
            lsmk_daw_key(k + 5, keys >> k & 1u ? (int)daw_kv_vel(plat_key_vel(k)) : 0);
    while (plat_outer(&i, &v))
        if (i >= 0 && i < NOUTER)
            lsmk_daw_key(i < 5 ? i : NKEYS + i, (int)daw_kv_vel((uint32_t)v));
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
        if (v && daw_local && lsmk_daw_ok() && daw_ent(i).type == DT_NOTE && p >= 8) {   /* LOCAL: the kit's sounds
                                                                                       * 1-8, as the drum pads */
            int kit = lsmk_kit();
            if (kit >= 0) {
                ld.pad_loc_trk[p] = (uint8_t)kit;
                ld.pad_loc[p] = (uint8_t)(p - 7);
                engine_key(kit, p - 7, 1, v);
            }
        }
    }
}

/* ---- the REAPER profile (CC MAP: REAPER; smk_mcu.h): a Mackie Control surface on the second USB port for REAPER's own
 * "Mackie Control Universal" surface, the music on the first. The keys, the wheels and LOCAL as DEFAULT's. KNOB BANK:
 * MIX (the V-pots: the strips' pan, FLIP their volume) or NAV (lit: K1 the jog, K2..K5 scroll and zoom as relative CCs
 * 102..105 on channel 16, K6 the track before / after (CC 106 / 107), K7 the marker before / after (Mackie's << >>),
 * K8 undo / redo (Mackie's UNDO / CC 111)). The faders the volumes of strips 1-4, FADER BANK 5-8, each once it
 * crosses REAPER's value (a pick-up). SEQ the pads' page: DRUMS (notes 36..51 on channel 10, as DEFAULT's), MIXER
 * (the top row selects strips 1-8, the bottom mutes them; GLOBE held: arm, solo), ACTIONS (F1..F8 and the eight view
 * buttons: REAPER passes them to its action list, to learn). SEQ PLAY play (stop while REAPER plays), SEQ REC record,
 * NOTE REPEAT loop, ARP the click, FX flip, PARA scrub, SCALE a marker, CHORD undo, BT save, < > the strips by 8
 * (GLOBE held: by 1). The lights, the pads and the screen show what REAPER sends back; 3 s of nothing from it: waiting,
 * and the query (F0 00 00 66 14 01 ..) every 3 s, which makes REAPER send everything again. */
enum { RP_DRUMS, RP_MIXER, RP_ACTIONS, RP_NPAGE };
static const char *const RP_PAGE_NAME[RP_NPAGE] = {"DRUMS", "MIXER", "ACTIONS"};
#define RP_NAV_CH 15u                           /* NAV's CCs: channel 16, on the first port */
#define RP_NAV_CC 102u                          /* K2..K5: 102..105 (relative, 64 +- the detents); K6 106 / 107; K8 111 */
static mcu_t lrp_m;                              /* what REAPER sent */
static struct {
    uint8_t on;                                 /* the profile in use, last frame (its start: a fresh state, the query) */
    uint8_t page;                               /* the pads: RP_* */
    uint8_t pad_note[NPADS];                    /* the note a pad pressed sent (0xFF none), and where: 1 the DAW port */
    uint8_t pad_daw[NPADS];
    uint8_t play_note;                          /* SEQ PLAY pressed: PLAY or STOP (its let-go the same) */
    uint8_t pick;                               /* the strips' faders that took REAPER's value (bit s: they send) */
    uint8_t fb_seen;                            /* the fader bank the pick-ups are for */
    uint16_t fsent[8];                          /* what a strip's fader sent last (MCU_NOFADER none), and when */
    uint32_t fsent_t[8];
    int32_t vacc[8], nacc[8];                   /* detents not sent yet: the V-pots; NAV's knobs (K1 the jog) */
    uint32_t query_t, link;                     /* the query sent last (ms | 1); the USB connection it went on */
    char last[20], lastv[32];                   /* the control touched last, what it did (the screen) */
    uint32_t last_n;                            /* (counts the touches: the screen redraws on it) */
    uint8_t drew;                               /* the screen as drawn: 1 REAPER's, 0 DAW mode's */
} lrp;
static int lrp_active(void) { return daw_on && daw_user == DAW_MAP_REAPER && !ld.edit; }
static void lrp_tx(uint32_t pkt) { plat_midi_send(pkt, TO_USB2); }
static void lrp_btn(uint32_t note, int on) { lrp_tx(mcu_note_pkt(note, (uint32_t)on)); }
static void lrp_tap(uint32_t note)
{
    lrp_btn(note, 1);
    lrp_btn(note, 0);
}
static void lrp_say(const char *what, const char *v)
{
    s_cat(lrp.last, what);
    s_cat(lrp.lastv, v);
    lrp.last_n++;
}
static void lrp_query(void)
{
    uint32_t p[4], k;
    lrp.query_t = plat_ms() | 1u;
    lrp.link = plat_midi_out_link();
    if (plat_midi_out_room(TO_USB2) < 8u)
        return;
    mcu_query_pkts(p);
    for (k = 0; k < 4u; k++)
        lrp_tx(p[k]);
}
/* the second port's packets, every frame: REAPER's when the profile is in use, else dropped */
static void lrp_input(void)
{
    uint32_t pkt, n = 0, now = plat_ms(), use = lrp_active() || (daw_on && daw_user == DAW_MAP_REAPER);
    while (n++ < 1024u && plat_midi2_in(&pkt))
        if (use)
            mcu_in(&lrp_m, pkt, now);
}
/* a pad let go: its note off (where it went), LOCAL's sound off */
static void lrp_pad_up(int p)
{
    if (lrp.pad_note[p] != 0xFFu) {
        if (lrp.pad_daw[p])
            lrp_btn(lrp.pad_note[p], 0);
        else
            plat_midi_out(daw_note_pkt(9u, lrp.pad_note[p], 0));
        lrp.pad_note[p] = 0xFF;
    }
    if (ld.pad_loc[p]) {
        engine_key(ld.pad_loc_trk[p], ld.pad_loc[p], 0, 0);
        ld.pad_loc[p] = 0;
    }
}
static void lrp_pads_up(void)
{
    int p;
    for (p = 0; p < NPADS; p++)
        lrp_pad_up(p);
}
static void lrp_unpick(void) { lrp.pick = 0; }
/* every frame in the profile: its start, the query while waiting (and on a new USB connection), the meters' fall,
 * the faders let go of when REAPER moved a strip on its own (automation, the mouse) */
static void lrp_service(void)
{
    uint32_t now = plat_ms(), s;
    if (!lrp.on) {
        mcu_reset(&lrp_m);
        for (s = 0; s < NPADS; s++)
            lrp.pad_note[s] = 0xFF;
        for (s = 0; s < 8u; s++)
            lrp.fsent[s] = MCU_NOFADER, lrp.vacc[s] = lrp.nacc[s] = 0;
        lrp.pick = 0;
        lrp.fb_seen = ld.fb;
        lrp.on = 1;
        lrp.drew = 0xFF;
        lrp_say("REAPER", "Mackie Control on SPIDER DAW");
        lrp_query();
    } else if ((!mcu_connected(&lrp_m, now) && now - (lrp.query_t & ~1u) >= MCU_SILENT_MS) ||
               plat_midi_out_link() != lrp.link)
        lrp_query();
    mcu_tick(&lrp_m, now);
    if (ld.fb != lrp.fb_seen) {                     /* (the other strips: each fader picks up again) */
        lrp.fb_seen = ld.fb;
        lrp.pick &= (uint8_t)~(15u << (4u * ld.fb));
    }
    for (s = 0; s < 8u; s++) {
        uint32_t at = lrp_m.fpos[s], sent = lrp.fsent[s];
        if (lrp.pick >> s & 1u && at != MCU_NOFADER && sent != MCU_NOFADER && now - lrp.fsent_t[s] > 500u &&
            (at > sent + 512u || at + 512u < sent))
            lrp.pick &= (uint8_t)~(1u << s);
    }
}

/* a fader moved from was to v (0..127): its strip's volume, once it has crossed REAPER's value (or REAPER has not
 * said one: at once) */
static void lrp_fader(uint32_t f, uint32_t was, uint32_t v)
{
    uint32_t s = 4u * ld.fb + f, v14 = mcu_v14(v), at = lrp_m.fpos[s], w14 = mcu_v14(was);
    char t[12];
    num_s(s + 1u, s_cat(t, "STRIP "));
    if (!(lrp.pick >> s & 1u)) {
        if (at == MCU_NOFADER || !mcu_connected(&lrp_m, plat_ms()) || (v14 >= at) != (w14 >= at) ||
            (v14 + 300u > at && v14 < at + 300u))
            lrp.pick |= (uint8_t)(1u << s);
        else {
            lrp_say(t, v14 < at ? "fader: up to pick up" : "fader: down to pick up");
            return;
        }
    }
    lrp_tx(mcu_fader_pkt(s, v14));
    lrp.fsent[s] = (uint16_t)v14;
    lrp.fsent_t[s] = plat_ms();
    lrp_m.fpos[s] = (uint16_t)v14;                  /* (REAPER follows; it may not echo it) */
    {
        char u[16];
        s_cat(num_s(v * 100u / 127u, s_cat(u, "volume ")), "%");
        lrp_say(t, u);
    }
}
static void lrp_knob(uint32_t k, int32_t s)
{
    char t[4];
    static const char *const NAV[8] = {"jog", "scroll", "zoom", "tracks scroll", "tracks zoom", "track", "marker",
                                       "undo / redo"};
    num_s(k + 1u, s_cat(t, "K"));
    if (ld.kb)
        lrp.nacc[k] += s, lrp_say(t, NAV[k]);
    else {
        char u[12];
        lrp.vacc[k] += s;
        num_s(k + 1u, s_cat(u, mcu_lit(&lrp_m, MCU_FLIP) ? "volume " : "pan "));
        lrp_say(t, u);
    }
}
/* the knobs' detents out, each frame: the V-pots and the jog relative, NAV's CCs, the steps a few a frame */
static void lrp_flush(void)
{
    uint32_t k;
    if (plat_midi_out_room(TO_USB2) < 16u || plat_midi_out_room(TO_USB) < 16u)
        return;
    for (k = 0; k < 8u; k++) {
        int32_t a = lrp.vacc[k], n = a > 15 ? 15 : a < -15 ? -15 : a;
        if (n) {
            lrp_tx(mcu_rel_pkt(MCU_VPOT0 + k, n));
            lrp.vacc[k] -= n;
        }
    }
    for (k = 0; k < 8u; k++) {
        int32_t a = lrp.nacc[k], n;
        if (!a)
            continue;
        if (k == 0u) {                                 /* K1: the jog */
            n = a > 15 ? 15 : a < -15 ? -15 : a;
            lrp_tx(mcu_rel_pkt(MCU_JOG, n));
        } else if (k <= 4u) {                          /* K2..K5: relative CCs, 64 +- n */
            n = a > 63 ? 63 : a < -63 ? -63 : a;
            plat_midi_out(daw_cc_pkt(RP_NAV_CH, RP_NAV_CC + k - 1u, (uint32_t)(64 + n)));
        } else {                                       /* K6..K8: one step a message, four a frame at most */
            n = a > 4 ? 4 : a < -4 ? -4 : a;
            for (int32_t i = 0; i < (n < 0 ? -n : n); i++) {
                if (k == 5u)
                    plat_midi_out(daw_cc_pkt(RP_NAV_CH, n < 0 ? 106u : 107u, 127u));
                else if (k == 6u)
                    lrp_tap(n < 0 ? MCU_REW : MCU_FF);
                else if (n < 0)
                    lrp_tap(MCU_UNDO);
                else
                    plat_midi_out(daw_cc_pkt(RP_NAV_CH, 111u, 127u));
            }
        }
        lrp.nacc[k] -= n;
    }
}
/* the pads: DRUMS notes on channel 10 (and LOCAL's kit), MIXER and ACTIONS Mackie's buttons */
static void lrp_pads(void)
{
    int p, v;
    while (plat_pad(&p, &v)) {
        char t[8];
        if (p < 0 || p >= NPADS)
            continue;
        lrp_pad_up(p);                                  /* (a press after a press: the first let go) */
        if (!v)
            continue;
        num_s((uint32_t)p + 1u, s_cat(t, "PAD "));
        if (lrp.page == RP_DRUMS) {
            uint32_t n = p >= 8 ? 36u + (uint32_t)p - 8u : 44u + (uint32_t)p;
            char u[16];
            plat_midi_out(daw_note_pkt(9u, n, (uint32_t)v));
            lrp.pad_note[p] = (uint8_t)n;
            lrp.pad_daw[p] = 0;
            s_cat(num_s(n, s_cat(u, "note ")), " ch 10");
            lrp_say(t, u);
            if (daw_local && p >= 8) {                 /* LOCAL: the kit's sounds 1-8, as the drum pads */
                int kit = lsmk_kit();
                if (kit >= 0) {
                    ld.pad_loc_trk[p] = (uint8_t)kit;
                    ld.pad_loc[p] = (uint8_t)(p - 7);
                    engine_key(kit, p - 7, 1, v);
                }
            }
        } else {
            uint32_t shift = plat_buttons() >> B_GLO & 1u, s = (uint32_t)p & 7u, n;
            static const char *const WHAT[2][2][2] = {{{"select", "mute"}, {"arm", "solo"}}, {{"F", "view "}, {"F", "view "}}};
            char u[16];
            if (lrp.page == RP_MIXER)
                n = p < 8 ? (shift ? MCU_REC0 : MCU_SEL0) + s : (shift ? MCU_SOLO0 : MCU_MUTE0) + s;
            else
                n = p < 8 ? MCU_F1 + s : MCU_VIEW0 + s;
            lrp_btn(n, 1);
            lrp.pad_note[p] = (uint8_t)n;
            lrp.pad_daw[p] = 1;
            if (lrp.page == RP_MIXER)
                num_s(s + 1u, s_cat(s_cat(u, WHAT[0][shift][p >= 8]), " strip "));
            else
                num_s(s + 1u, s_cat(u, WHAT[1][0][p >= 8]));
            lrp_say(t, u);
        }
    }
}
/* the buttons and knobs (after DAW mode's own: < + >, PATCH, the banks, OCT) */
static void lrp_controls(uint32_t btn, uint32_t pr, uint32_t rl, uint32_t sb, uint32_t srl, const int32_t *s)
{
    static const uint8_t BTN[8][2] = {{B_REC, MCU_RECORD}, {B_LFO, MCU_CYCLE}, {B_ARP, MCU_CLICK}, {B_FX, MCU_FLIP},
                                      {B_EDIT, MCU_SCRUB}, {B_SEL, MCU_MARKER}, {B_ENV, MCU_UNDO}, {B_SAVE, MCU_SAVE}};
    static const char *const SAY[8][2] = {{"SEQ REC", "record"}, {"NOTE REPEAT", "loop"}, {"ARP", "click"},
                                          {"FX", "flip: faders on pan"}, {"PARA", "scrub"}, {"SCALE", "marker"},
                                          {"CHORD", "undo"}, {"BT", "save"}};
    uint32_t shift = btn >> B_GLO & 1u, k;
    if (pr >> B_PLAY & 1u) {                       /* SEQ PLAY: play, or stop while REAPER plays */
        lrp.play_note = (uint8_t)(mcu_lit(&lrp_m, MCU_PLAY) ? MCU_STOP : MCU_PLAY);
        lrp_btn(lrp.play_note, 1);
        lrp_say("SEQ PLAY", lrp.play_note == MCU_STOP ? "stop" : "play");
    } else if (rl >> B_PLAY & 1u && lrp.play_note)
        lrp_btn(lrp.play_note, 0);
    for (k = 0; k < 8u; k++)
        if (pr >> BTN[k][0] & 1u) {
            lrp_btn(BTN[k][1], 1);
            lrp_say(SAY[k][0], k == 3u && mcu_lit(&lrp_m, MCU_FLIP) ? "flip: faders on volume" : SAY[k][1]);
        } else if (rl >> BTN[k][0] & 1u)
            lrp_btn(BTN[k][1], 0);
    if (pr >> B_SEQ & 1u) {                        /* SEQ: the pads' page */
        lrp_pads_up();
        lrp.page = (uint8_t)((lrp.page + 1u) % RP_NPAGE);
        lrp_say("SEQ", RP_PAGE_NAME[lrp.page]);
    }
    for (k = 0; k < 2u; k++)                       /* < >: the strips by 8, GLOBE held by 1 (when let go) */
        if (srl >> (k ? SB_RIGHT : SB_LEFT) & 1u && !ld.lr_both) {
            lrp_tap(shift ? (k ? MCU_CH_R : MCU_CH_L) : (k ? MCU_BANK_R : MCU_BANK_L));
            lrp_say(k ? ">" : "<", shift ? (k ? "strips +1" : "strips -1") : (k ? "strips +8" : "strips -8"));
            lrp_unpick();
        }
    if (!(sb & (1u << SB_LEFT | 1u << SB_RIGHT)))
        ld.lr_both = 0;
    for (k = 0; k < 8u; k++)
        if (s[k])
            lrp_knob(k, s[k]);
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
        uint32_t v = plat_fader((int)f), i = DAW_FADER0 + 4u * ld.fb + f, was = ld.fseen[f];
        if (v == ld.fseen[f])
            continue;
        ld.fseen[f] = (uint8_t)v;
        if (lrp_active())
            lrp_fader(f, was, v);                   /* (REAPER: a strip's volume, picked up) */
        else if (!ld.edit)
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
    /* PATCH: tapped LOCAL; held, the settings (K5 CHANNEL, K6 CC MAP, K7 KNOBS, K8 KEY VELOCITY) and OCT+ the map
     * editor */
    if (pr >> B_HOME & 1u)
        ld.patch_used = 0;
    if (patch) {
        if (s[4] || s[5] || s[6] || s[7] || (pr & oct))
            ld.patch_used = 1;
        if (s[7] && !ld.edit)
            lsmk_kv_step(s[7]);
        if (s[4])
            daw_ch = (uint8_t)clampi(daw_ch + s[4], 1, 16);
        if (s[5])
            daw_user = (uint8_t)clampi((int)daw_user + (s[5] > 0 ? 1 : -1), 0, (int)DAW_MAP_REAPER);
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
    if (lrp_active()) {                             /* the REAPER profile: its own jobs for the buttons and knobs */
        lrp_controls(btn, pr, rl, sb, srl, s);
        return;
    }
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
        text_r(228, 4, &FONT_S, daw_user == DAW_MAP_REAPER ? "REAPER" : daw_user ? "USER MAP" : "DEFAULT MAP", K_TEXT);
        text_r(228, 21, &FONT_XS, daw_rel ? "knobs relative" : "knobs absolute", K_DIM);
        cv_rect(0, 39, 240, 1, K_LINE);
        cv_blit(0, 0);
    }
    /* band 1, 40..119: the control last touched, or the editor's; PATCH held: KEY VELOCITY and the last key */
    int pv = (ld.btn >> B_HOME & 1u) && !ld.edit;
    char kl[40];
    kl[0] = 0;
    if (pv) {
        if (!lsmk_kv_last(kl))
            kl[0] = 0;
        h = hash_s(hash_u(0x56u, daw_kv), kl);
    } else if (ld.edit) {
        daw_ent_t e = daw_umap[ld.esel % DAW_N];
        h = hash_u(hash_u(hash_u(hash_u(0x45u, ld.edit), ld.esel), e.type | e.num << 8 | e.ch << 16), daw_ch);
    } else
        h = hash_u(hash_u(hash_u(hash_u(0x4Cu, ld.sel), ld.sel_v), ld.sel_bank), daw_user | daw_ch << 4);
    if (h != ld.sig[1]) {
        ld.sig[1] = h;
        cv_begin(240, 80, K_BG);
        if (pv) {
            cv_text(12, 4, &FONT_B, "Key velocity", K_TEXT);
            text_r(228, 4, &FONT_B, KV_NAME[daw_kv % KV_N], K_TEXT);
            cv_text(12, 28, &FONT_XS, "K8: heavy to lightest, fixed 100, 127", K_DIM);
            if (kl[0])
                cv_text(12, 50, &FONT_S, kl, K_TEXT);
            else
                cv_text(12, 50, &FONT_S, "play a key to see its velocity", K_DIM);
        } else if (ld.edit == 1u) {
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
            text_c(120, 40, &FONT_XS, "LOOM-8 plays what comes in (MIDI IN)", K_DIM);
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
            cv_text((int32_t)x, (int32_t)y, &FONT_XS, t, K_TEXT);
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
        text_c(120, 7, &FONT_XS, ld.btn >> B_HOME & 1u ? "K5 ch  K6 map  K7 knobs  K8 vel  OCT+ edit"
                                 : ld.edit ? "OCT+ next control    OCT- done"
                                           : "< + >  LOOM-8        PATCH  local", K_DIM);
        cv_blit(0, 212);
    }
}

/* ---- the REAPER profile's screen and lights (PATCH held: DAW mode's settings, as in every map) */
static void lsmk_daw_redraw(uint32_t which)     /* the screen about to be REAPER's (1) or DAW mode's (0): new, all */
{
    uint32_t k;
    if (lrp.drew == which)
        return;
    lrp.drew = (uint8_t)which;
    for (k = 0; k < 5u; k++)
        ld.sig[k] = 0;
    lcd_fill(0, 0, 240, 240, K_BG);
}
static uint16_t lrp_565(uint32_t rgb)
{
    return (uint16_t)((rgb >> 19 & 31u) << 11 | (rgb >> 10 & 63u) << 5 | (rgb >> 3 & 31u));
}
static uint32_t lrp_hash(uint32_t h, const void *p, uint32_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    while (n--)
        h = hash_u(h, *b++);
    return h;
}
static void lrp_draw(void)
{
    uint32_t now = plat_ms(), conn = (uint32_t)mcu_connected(&lrp_m, now), h, s, first = 1;
    char t[40], u[40];
    if (ld.btn >> B_HOME & 1u) {                   /* PATCH held: the settings */
        lsmk_daw_redraw(0);
        lsmk_daw_draw();
        return;
    }
    lsmk_daw_redraw(1);
    if (lrp_m.assign[0] >= '0' && lrp_m.assign[0] <= '9' && lrp_m.assign[1] >= '0' && lrp_m.assign[1] <= '9')
        first = (uint32_t)(lrp_m.assign[0] - '0') * 10u + (uint32_t)(lrp_m.assign[1] - '0');
    /* band 0, 0..39: REAPER, its transport, the time */
    h = lrp_hash(hash_u(0x52u, conn | (uint32_t)lrp_m.led[MCU_PLAY] << 1 | (uint32_t)lrp_m.led[MCU_RECORD] << 9 |
                                   (uint32_t)lrp_m.led[MCU_CYCLE] << 17 | (uint32_t)!!lrp_m.led[MCU_CLICK] << 25 |
                                   (uint32_t)!!lrp_m.led[MCU_BEATS] << 26), lrp_m.tc, sizeof lrp_m.tc);
    if (h != ld.sig[0]) {
        ld.sig[0] = h;
        cv_begin(240, 40, K_BG);
        cv_text(12, 4, &FONT_S, "REAPER", K_TEXT);
        s_cat(s_cat(s_cat(t, !conn ? "waiting" : mcu_lit(&lrp_m, MCU_RECORD) ? "recording"
                                 : mcu_lit(&lrp_m, MCU_PLAY) ? "playing" : "stopped"),
                    conn && mcu_lit(&lrp_m, MCU_CYCLE) ? "  loop" : ""), conn && mcu_lit(&lrp_m, MCU_CLICK) ? "  click" : "");
        cv_text(12, 21, &FONT_XS, t, conn && mcu_lit(&lrp_m, MCU_RECORD) ? K_REC : K_DIM);
        if (conn && mcu_time(&lrp_m, u))
            text_r(228, cap_y(0, 40, &FONT_B), &FONT_B, u, K_TEXT);
        cv_rect(0, 39, 240, 1, K_LINE);
        cv_blit(0, 0);
    }
    /* band 1, 40..119: the eight strips, two rows of four (the faders' row framed): name, ring, meter */
    h = lrp_hash(lrp_hash(lrp_hash(hash_u(0x53u, conn | ld.fb << 1 | (uint32_t)lrp_m.clip << 2), lrp_m.lcd, 56), lrp_m.ring,
                        8), lrp_m.meter, 8);
    h = lrp_hash(h, lrp_m.led, 0x20);
    if (h != ld.sig[1]) {
        ld.sig[1] = h;
        cv_begin(240, 80, K_BG);
        if (!conn) {
            text_c(120, 6, &FONT_B, "Waiting for REAPER", K_TEXT);
            text_c(120, 30, &FONT_XS, "Control surface: Mackie Control", K_DIM);
            text_c(120, 44, &FONT_XS, "Universal, MIDI in and out", K_DIM);
            text_c(120, 58, &FONT_XS, "MIDIIN2 / MIDIOUT2 (SPIDER SMK-37)", K_DIM);
        } else {
            cv_frame(1, (int32_t)ld.fb * 40, 238, 40, 5, K_LINE, K_BG);
            for (s = 0; s < 8u; s++) {
                int32_t x = 4 + (int32_t)(s % 4u) * 58, y = (int32_t)(s / 4u) * 40;
                uint32_t named = (uint32_t)mcu_name(&lrp_m, s, t), ring = lrp_m.ring[s] & 15u, m = lrp_m.meter[s];
                uint16_t ink = trk_ink((int)s);
                if (mcu_lit(&lrp_m, MCU_SEL0 + s))
                    cv_round(x, y + 3, 56, 34, 4, K_PANEL);
                text_fit(x + 4, y + 4, &FONT_XS, named ? t : "-", 33,
                         !named ? K_FAINT : mcu_lit(&lrp_m, MCU_MUTE0 + s) ? K_FAINT : K_TEXT);
                if (mcu_lit(&lrp_m, MCU_REC0 + s))
                    cv_round(x + 46, y + 7, 6, 6, 3, K_REC);
                if (mcu_lit(&lrp_m, MCU_SOLO0 + s))
                    cv_round(x + 38, y + 7, 6, 6, 3, lrp_565(0xFFC000u));
                cv_rect(x + 4, y + 23, 48, 1, K_LINE);
                if (ring)
                    cv_rect(x + 4 + (int32_t)((ring > 11u ? 10u : ring - 1u) * 44u / 10u), y + 20, 4, 7, ink);
                cv_rect(x + 4, y + 30, 48, 3, K_PANEL2);
                if (m)
                    cv_rect(x + 4, y + 30, (int32_t)(m * 48u / 13u), 3, (lrp_m.clip >> s & 1u) || m >= 12u ? K_REC
                                                                          : lrp_565(m >= 10u ? 0xFFB000u : 0x30C060u));
            }
        }
        cv_blit(0, 40);
    }
    /* band 2, 120..151: the strips' tracks, the pads' page, the knobs' bank, the octave */
    h = hash_u(hash_u(hash_u(0x54u, first | conn << 8), lrp.page | ld.kb << 4), (uint32_t)(ld.oct + 8));
    if (h != ld.sig[2]) {
        ld.sig[2] = h;
        cv_begin(240, 32, K_BG);
        if (conn)
            num_s(first + 7u, s_cat(num_s(first, s_cat(t, "TRACKS ")), "-"));
        else
            s_cat(t, "TRACKS -");
        cv_text(12, 8, &FONT_XS, t, K_TEXT);
        s_cat(s_cat(u, "PADS "), RP_PAGE_NAME[lrp.page % RP_NPAGE]);
        cv_text(100, 8, &FONT_XS, u, K_TEXT);
        num_i(ld.oct, s_cat(s_cat(t, ld.kb ? "NAV" : "MIX"), ld.oct > 0 ? "  OCT +" : "  OCT "));
        text_r(228, 8, &FONT_XS, t, K_TEXT);
        cv_blit(0, 120);
    }
    /* band 3, 152..211: the control touched last, what it did */
    h = hash_u(0x55u, lrp.last_n);
    if (h != ld.sig[3]) {
        ld.sig[3] = h;
        cv_begin(240, 60, K_BG);
        cv_rect(0, 0, 240, 1, K_LINE);
        text_fit(12, 10, &FONT_B, lrp.last, 216, K_TEXT);
        text_fit(12, 34, &FONT_S, lrp.lastv, 216, K_DIM);
        cv_blit(0, 152);
    }
    /* band 4, 212..239: what the page's pads and GLOBE do */
    h = hash_u(0x56u, lrp.page | (ld.btn >> B_GLO & 1u) << 4);
    if (h != ld.sig[4]) {
        ld.sig[4] = h;
        cv_begin(240, 28, K_BG);
        cv_rect(0, 0, 240, 1, K_LINE);
        text_c(120, 7, &FONT_XS, ld.btn >> B_GLO & 1u ? "GLOBE:  < > by 1   pads arm, solo"
                                 : lrp.page == RP_MIXER ? "top: select   bottom: mute   GLOBE"
                                 : lrp.page == RP_ACTIONS ? "learn each pad in the action list"
                                                         : "SEQ pads   KNOB BANK nav   PATCH", K_DIM);
        cv_blit(0, 212);
    }
}
static void lrp_lights(void)
{
    static const uint8_t L[8][2] = {{B_PLAY, MCU_PLAY}, {B_REC, MCU_RECORD}, {B_LFO, MCU_CYCLE}, {B_ARP, MCU_CLICK},
                                    {B_FX, MCU_FLIP}, {B_EDIT, MCU_SCRUB}, {B_ENV, MCU_UNDO}, {B_SAVE, MCU_SAVE}};
    uint32_t now = plat_ms(), held = plat_pads_held(), blink = now >> 8 & 1u, shift = ld.btn >> B_GLO & 1u, p, k;
    uint32_t conn = (uint32_t)mcu_connected(&lrp_m, now), lit = 0;
    char nm[8];
    for (p = 0; p < NPADS; p++) {
        uint32_t c, lv = 48u, s = p & 7u;
        if (lrp.page == RP_DRUMS)
            c = 0x2060FFu;
        else if (lrp.page == RP_ACTIONS)
            c = 0x8040FFu;
        else {                                     /* MIXER: REAPER's selects, mutes (GLOBE: arms, solos) */
            uint32_t n = p < 8u ? (shift ? MCU_REC0 : MCU_SEL0) + s : (shift ? MCU_SOLO0 : MCU_MUTE0) + s,
                     v = lrp_m.led[n];
            c = p < 8u ? (shift ? 0xFF1800u : 0xFFFFFFu) : (shift ? 0xFFB000u : 0xFF1800u);
            if (!conn || !mcu_name(&lrp_m, s, nm))
                c = 0x101010u, v = 0;              /* (no track there) */
            lv = v == 1u ? (blink ? 256u : 64u) : v ? 256u : 40u;
        }
        if (held >> p & 1u)
            lv = 256u;
        plat_pad_rgb((int)p, (((c >> 16) & 255u) * lv >> 8) << 16 | (((c >> 8) & 255u) * lv >> 8) << 8 |
                                 ((c & 255u) * lv >> 8));
    }
    for (k = 0; k < 8u; k++) {
        uint32_t v = conn ? lrp_m.led[L[k][1]] : 0u;
        if (v == 127u || (v && blink))
            lit |= 1u << L[k][0];
    }
    lit |= ld.btn & (1u << B_SEL | 1u << B_GLO);   /* (held: SCALE, GLOBE) */
    if (lrp.page != RP_DRUMS)
        lit |= 1u << B_SEQ;
    if (daw_local)
        lit |= 1u << B_HOME;
    lit |= (ld.oct < 0 ? 1u << B_OCTDN : 0u) | (ld.oct > 0 ? 1u << B_OCTUP : 0u);
    plat_lights(lit, 0, 0, 0, 0);
    plat_smk_leds((ld.kb ? 1u << SB_KNOB_BANK : 0u) | (ld.fb ? 1u << SB_FADER_BANK : 0u));
}

/* ---- in and out */
static void lsmk_synth_quiet(void)              /* LOOM-8's side: the pads', outer keys' notes off, the bend 0 */
{
    int i;
    for (i = 0; i < NPADS; i++)
        lsmk_drum_off(i);
    lsmk.nr_pads = 0;
    for (i = 0; i < NOUTER; i++)
        if (lsmk.out_note[i]) {
            engine_key(lsmk.out_trk[i], lsmk.out_note[i], 0, 0);
            chord_del(lsmk.out_note[i]);
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
    if (lrp.on)
        lrp_pads_up();                              /* (the REAPER profile's pads: their own notes) */
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
    lrp.drew = 0xFF;
    for (k = 0; k < 5; k++)
        ld.sig[k] = 0;
    lcd_fill(0, 0, 240, 240, K_BG);
    ui_redraw();
    lsmk_daw_dirty();
    if (!on)
        say("LOOM-8", "< + > for DAW mode");
}

/* every frame, before LOOM-8's: 1 = DAW mode had it */
static int lsmk_daw_frame(void)
{
    if (!ld.loaded) {                              /* the first frame: the settings, the map; the mode kept */
        ld.loaded = 1;
        lsmk_daw_load();
        lsmk_kv_apply();                           /* (KEY VELOCITY's touch: the keys read with it from now) */
        if (daw_on) {
            daw_on = 0;
            lsmk_daw_set(1);
            ld.dirty = 0;
        }
    }
    lrp_input();                                    /* (the second USB port: read always, kept for REAPER) */
    if (!daw_on) {
        lrp.on = 0;
        lsmk_daw_save();
        return 0;
    }
    lsmk_daw_controls();
    if (!daw_on)                                   /* (< + >: LOOM-8 again, from the next frame) */
        return 1;
    if (lrp_active())
        lrp_service();
    else if (lrp.on) {                              /* (the profile left: its pads let go) */
        lrp_pads_up();
        lrp.on = 0;
    }
    lsmk_daw_keys();
    if (lrp_active())
        lrp_pads();
    else
        lsmk_daw_pads();
    lsmk_daw_analog();
    lsmk_daw_flush();
    if (lrp_active()) {
        lrp_flush();
        lrp_draw();
        lrp_lights();
    } else {
        lsmk_daw_redraw(0);
        lsmk_daw_draw();
        lsmk_daw_lights();
    }
    lsmk_daw_save();
    ui.frame++;
    return 1;
}
