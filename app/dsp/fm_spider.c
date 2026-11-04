/* SPDX-License-Identifier: GPL-3.0-only */
/* SPIDER's fm engine (EG_FM; dsp/synth_spider.c includes this file, in two parts): six operators in the manner of the
 * classic FM keyboards, by msfa (Dexed's core) as SLOOP ported it to integer C (dsp/fm6_core.c, Apache-2.0).
 *
 * The sound is a patch (tools/gen_fm.py: SPIDER's own sixteen, an electric piano, basses, brass, bells...): the four
 * knobs on top of it, the simple way in:
 *   patch   which of the sixteen
 *   bright  the modulators' level, +-36 dB about the middle: duller to brighter (an LFO or a lock moves it too)
 *   decay   every operator's decay and release rates, slower (up) or faster (down) about the middle
 *   detune  the carriers spread apart in pitch, up to about +-36 cents between the outer ones
 * The operators' own envelopes shape the note and end it: the voice's amp envelope stays open (its presets hold it
 * at full, a long release) and the voice stops when every carrier has gone quiet (fm_done). The filter, the
 * character stage, the LFO and the sends work on it as on any engine.
 *
 * Each voice keeps its note (fm6_note_t) and LFO in the synth (fm_vs: the pool), and renders 32 samples at a time
 * into its own buffer, read out over the sub-blocks: a render split anywhere sounds the same. The patch through
 * the patch and decay knobs is the track's (fm_trk_t, made in synth_prepare: one core, before the voices); bright
 * and detune are the voice's, a chunk at a time. The modulator buses are a core's own (fm_bus). */

#ifndef FM_SPIDER_PART2
/* ------------------------------------------------------------- part 1: types --- */
#include "fm.h"                                /* (the build's gen: tools/gen_fm.py) */
#include "fm6_core.c"

typedef struct {
    fm6_note_t n;
    fm6_lfo_t lfo;
    int32_t buf[FM6_N];                        /* the 32 samples rendered, Q24 */
    uint8_t pos;                               /* the next to read (FM6_N: none left) */
    uint8_t start, live, rsv;                  /* start: a note to set up at the next render (voice_start) */
    int32_t logf;                              /* the note's pitch, log2 Hz Q24 (eng_prep) */
} fm_voice_t;

typedef struct {
    uint8_t p[FP_SIZE + 1u];                   /* the patch through the decay knob */
    uint8_t patch, decay, ok, alg;             /* what it was made from; the algorithm */
} fm_trk_t;

#else
/* ---------------------------------------------------------- part 2: the engine --- */
static int32_t fm_bus[2][2][FM6_N];            /* the modulator buses, a core's (voice_t cpu) */

static int fm_patch_of(int k) { return k < 0 ? 0 : k >= FM_NPATCH ? FM_NPATCH - 1 : k; }   /* (the patch knob: 0..15) */

/* the track's patch through its decay knob (synth_prepare: before the voices render) */
static void fm_track(synth_t *s, int trk)
{
    fm_trk_t *f = &s->fm_t[trk];
    int patch = fm_patch_of(P(s, trk, TP_E1)), decay = P(s, trk, TP_E3), k, j;
    if (f->ok && f->patch == patch && f->decay == decay)
        return;
    {
        const uint8_t *src = FM_PATCH[patch];
        int d = (decay - 64) * 30 / 64;        /* rate steps: up slower, down faster */
        for (j = 0; j < (int)FP_SIZE; j++)
            f->p[j] = src[j];
        f->p[FP_SIZE] = 0;
        for (k = 0; k < 6; k++)
            for (j = 1; j < 4; j++) {          /* (the attack kept: R1) */
                int r = f->p[k * FP_OP + FP_R1 + j] - d;
                f->p[k * FP_OP + FP_R1 + j] = (uint8_t)(r < 0 ? 0 : r > 99 ? 99 : r);
            }
    }
    f->alg = (uint8_t)(f->p[FP_ALG] & 31u);
    f->patch = (uint8_t)patch;
    f->decay = (uint8_t)decay;
    f->ok = 1;
}

static void fm_init_voice(synth_t *s, voice_t *v)
{
    fm_voice_t *f = &s->fm_v[v->vi % NVOICE];
    f->live = 0;
    f->start = 0;
    f->pos = FM6_N;
}

/* voice_start: the note sets up at its first render (its core) */
static void fm_strike(synth_t *s, voice_t *v) { s->fm_v[v->vi % NVOICE].start = 1; }

/* eng_prep: the note's pitch (its glide, bend, vibrato), log2 Hz Q24, with the patch's transpose */
static void fm_prep(synth_t *s, voice_t *v)
{
    fm_voice_t *f = &s->fm_v[v->vi % NVOICE];
    const fm_trk_t *t = &s->fm_t[v->trk < NTRK ? v->trk : 0];
    float semi = v->pitch_cur + v->c.pitch_off * 0.01f + (float)((int)t->p[FP_TRNSP] - 24);
    f->logf = FM6_LOGF0 + (int32_t)(semi * (16777216.0f / 12.0f));
    v->c.stereo = 0;
}

/* every carrier quiet for good: the voice can end */
static int fm_done(synth_t *s, const voice_t *v)
{
    const fm_voice_t *f = &s->fm_v[v->vi % NVOICE];
    const fm_trk_t *t = &s->fm_t[v->trk < NTRK ? v->trk : 0];
    return !f->start && (!f->live || fm6_note_done(&f->n, t->p, t->alg));
}

static void fm_block(synth_t *s, voice_t *v, fm_voice_t *f)
{
    const fm_trk_t *t = &s->fm_t[v->trk < NTRK ? v->trk : 0];
    const uint8_t *p = t->p;
    int32_t dt[6], step, modlvl, lv, ld;
    uint32_t car = fm6_carriers(t->alg), k, n = 0;
    if (f->start) {                            /* the note: from silence, or over the voice's last (a retrigger) */
        int note = (int)v->note;
        fm6_note_init(&f->n, p, note + (int)p[FP_TRNSP] - 24, (int32_t)(v->vel * 127.0f + 0.5f), !f->live);
        fm6_lfo_reset(&f->lfo, p);
        fm6_lfo_key(&f->lfo);
        f->start = 0;
        f->live = 1;
    }
    if (!f->live) {
        for (k = 0; k < FM6_N; k++)
            f->buf[k] = 0;
        return;
    }
    if (!v->held && f->n.down)
        fm6_note_key(&f->n, p, 0);
    step = (int32_t)(v->c.e[3] * (12.0f * 13981.0f));   /* detune: up to 12 cents a step (a cent: 13981) */
    for (k = 0; k < 6u; k++) {
        static const int8_t SPREAD[6] = {0, 1, -1, 2, -2, 3};
        uint32_t ki = 5u - k;                  /* OP1 first: the carriers in their order */
        dt[ki] = (car >> ki & 1u) ? SPREAD[n++ % 6u] * step : 0;
    }
    modlvl = (int32_t)((v->c.e[1] - 0.5f) * 2.0f * 1536.0f) << 16;   /* bright: +-36 dB on the modulators */
    lv = fm6_lfo_sample(&f->lfo);
    ld = fm6_lfo_delay(&f->lfo);
    if (!fm6_note_compute(&f->n, p, f->buf, fm_bus[v->cpu & 1u], lv, ld, f->logf, t->alg, p[FP_FB] & 7u, dt, modlvl))
        for (k = 0; k < FM6_N; k++)
            f->buf[k] = 0;
}

/* the Engine page's picture (pages.c pic_fm): a patch's six output levels (OP1 first), its algorithm (1..32); returns
 * its carriers (bit k: OP k + 1) */
int synth_fm_shape(int patch, uint8_t *ol, uint8_t *alg)
{
    const uint8_t *p = FM_PATCH[fm_patch_of(patch)];
    uint32_t car = fm6_carriers(p[FP_ALG] & 31u), out = 0, k;
    for (k = 0; k < 6u; k++) {                 /* (the voice lists OP6 first) */
        ol[k] = p[(5u - k) * FP_OP + FP_OL];
        out |= (car >> (5u - k) & 1u) << k;
    }
    *alg = (uint8_t)((p[FP_ALG] & 31u) + 1u);
    return (int)out;
}

/* the samples: from the buffer, 32 rendered whenever it runs out. Q24 -> about 1 for a carrier at full; the
 * output's DC (FM puts sidebands at 0 Hz) taken out by a one-pole high-pass, ~7 Hz */
static void fm_run(synth_t *s, voice_t *v, float *xl, float *xr, int m)
{
    fm_voice_t *f = &s->fm_v[v->vi % NVOICE];
    int j;
    for (j = 0; j < m; j++) {
        int32_t x;
        if (f->pos >= FM6_N) {
            fm_block(s, v, f);
            f->pos = 0;
        }
        x = f->buf[f->pos++] - f->n.dc;
        f->n.dc += x >> 10;
        xl[j] = xr[j] = (float)x * (0.5f / 16777216.0f);
    }
    if (f->live && !f->start && !v->held && fm_done(s, v))
        v->amp.stage = ENV_OFF;                /* (every carrier done after the key: the voice is free) */
}
#endif
