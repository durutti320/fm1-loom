/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (SPIDER, the LOOM-8 SMK-37 port) */
/* SPIDER's boot (LOOM-8 on the M-VAVE SMK-37: app.h APP_NAME), about 3.6 s drawn as the unit starts: a night with a
 * moon behind gnarled branches and fog drifting; a spider drops on its line to the hub, the frame and the spokes
 * shoot out, and the spider walks the capture spiral from the outside in (as orb weavers lay it), dew glinting on
 * what it has laid; then the name is woven in, row by row like a shuttle at the loom, and under it LOOM-8's own
 * splash in little: eight threads in the tracks' inks, over and under.
 * All of it procedural: every frame is drawn again from the time alone (the same seeds, nothing kept), in two bands
 * of 240 x 120 through fm1-base's canvas (gfx.c cv_*; LOOM-8's faces, gfx_loom.c): no RAM beyond the canvas.
 *
 * spd_frame(t) draws the screen at t ms; app_splash (app.c) runs it from the chip's free-running timer until SPD_MS,
 * feeding the watchdog (nothing else runs yet). tests/spider_preview.c draws the same frames on a PC. */

#define SPD_MS 3600u
#define SPD_PI 3.14159265f

/* ---- a little maths without libm */
static uint32_t spd_rs = 1u;
static uint32_t spd_rand(void)
{
    spd_rs ^= spd_rs << 13;
    spd_rs ^= spd_rs >> 17;
    spd_rs ^= spd_rs << 5;
    return spd_rs;
}
static float spd_rf(float a, float b) { return a + (b - a) * (float)(spd_rand() & 0xFFFFu) * (1.0f / 65535.0f); }
static uint32_t spd_hash(uint32_t a, uint32_t b)
{
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    return h ^ (h >> 12);
}
static float spd_sin(float x)
{
    float y;
    int32_t k = (int32_t)(x * (0.5f / SPD_PI) + (x >= 0.0f ? 0.5f : -0.5f));   /* into -pi .. pi */
    x -= (float)k * 2.0f * SPD_PI;
    y = 1.27323954f * x - 0.40528473f * x * (x < 0.0f ? -x : x);
    return 0.225f * (y * (y < 0.0f ? -y : y) - y) + y;
}
static float spd_cos(float x) { return spd_sin(x + 0.5f * SPD_PI); }
static float spd_sqrt(float v)
{
    union { float f; uint32_t u; } c;
    float r;
    if (v <= 0.0f)
        return 0.0f;
    c.f = v;
    c.u = 0x1FBD1DF5u + (c.u >> 1);            /* a first guess, then Newton */
    r = c.f;
    r = 0.5f * (r + v / r);
    return 0.5f * (r + v / r);
}
static float spd_clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

/* ---- colours, and drawing in screen rows on the band (cv_oy = -the band's top) */
static uint16_t spd_mix(uint16_t a, uint16_t b, uint32_t k)   /* a -> b by k / 256 */
{
    uint32_t r = ((a >> 11) * (256u - k) + (b >> 11) * k) >> 8, g = ((a >> 5 & 63u) * (256u - k) + (b >> 5 & 63u) * k) >> 8,
             bl = ((a & 31u) * (256u - k) + (b & 31u) * k) >> 8;
    return (uint16_t)(r << 11 | g << 5 | bl);
}
static void spd_blend(int32_t x, int32_t y, uint16_t c, uint32_t a)   /* a 0 .. 256 */
{
    uint32_t i;
    y += cv_oy;
    if ((uint32_t)x >= cv_w || y < cv_y0 || y >= cv_y1 || !a)
        return;
    i = (uint32_t)y * cv_w + (uint32_t)x;
    cv_px[i] = swap16(a >= 256u ? c : spd_mix(swap16(cv_px[i]), c, a));
}
static int spd_rows(float y0, float y1)       /* any of [y0, y1] in the band */
{
    return y1 >= (float)(-cv_oy - 2) && y0 <= (float)(-cv_oy + (int32_t)cv_h + 2);
}
/* an ellipse (rows from y_top to y_bot only), its edges smoothed */
static void spd_ellipse(float cx, float cy, float rx, float ry, float y_top, float y_bot, uint16_t c, uint32_t a)
{
    int32_t y, ya, yb, x;
    if (rx < 0.5f || ry < 0.5f)
        return;
    ya = (int32_t)(cy - ry > y_top ? cy - ry : y_top);
    yb = (int32_t)(cy + ry < y_bot ? cy + ry : y_bot);
    if (!spd_rows((float)ya, (float)yb))
        return;
    for (y = ya; y <= yb; y++) {
        float dy = ((float)y + 0.5f - cy) / ry, hw, l, r;
        if (dy <= -1.0f || dy >= 1.0f)
            continue;
        hw = rx * spd_sqrt(1.0f - dy * dy);
        l = cx - hw;
        r = cx + hw;
        for (x = (int32_t)l; x <= (int32_t)r; x++) {
            float cov = 1.0f;
            if ((float)x < l)
                cov = (float)x + 1.0f - l;
            if ((float)x + 1.0f > r)
                cov -= (float)x + 1.0f - r;
            if (cov > 0.0f)
                spd_blend(x, y, c, (uint32_t)(cov * (float)a));
        }
    }
}
static void spd_disc(float cx, float cy, float r, uint16_t c, uint32_t a)
{
    spd_ellipse(cx, cy, r, r, -1000.0f, 1000.0f, c, a);
}

#define SPD_GMAX (48 * 44)
typedef struct { int32_t x, y, w, h; uint8_t c[SPD_GMAX]; } spd_glyph_t;
static void spd_glyph(const loom_font_t *f, uint32_t gi, spd_glyph_t *g)
{
    const uint8_t *gd = f->data + f->off[gi], *bx = f->box[gi];
    uint32_t i, n, nib = 0, run = 0, a = 0;
    g->x = bx[0];
    g->y = bx[1];
    g->w = bx[2];
    g->h = bx[3];
    n = (uint32_t)(g->w * g->h);
    if (n > SPD_GMAX)
        n = 0, g->w = g->h = 0;
    for (i = 0; i < n; i++) {
        if (!run) {
            a = (nib & 1u) ? gd[nib >> 1] & 15u : gd[nib >> 1] >> 4;
            nib++;
            run = 1;
            if (f->rle && (a == 0 || a == 15u)) {
                run += (nib & 1u) ? gd[nib >> 1] & 15u : gd[nib >> 1] >> 4;
                nib++;
            }
        }
        run--;
        g->c[i] = (uint8_t)a;
    }
}
static float spd_cov(const spd_glyph_t *g, int32_t x, int32_t y)   /* at a cell pixel (from the pen - pad, the top) */
{
    x -= g->x;
    y -= g->y;
    if (x < 0 || y < 0 || x >= g->w || y >= g->h)
        return 0.0f;
    return (float)g->c[y * g->w + x] * (1.0f / 15.0f);
}

/* ---- the scene's colours */
#define SPD_SKY0 RGB(8, 6, 16)                  /* the sky at the top, at the foot */
#define SPD_SKY1 RGB(26, 12, 34)
#define SPD_BARK RGB(14, 10, 18)                /* the branches, their moonlit edge */
#define SPD_RIM RGB(104, 80, 136)
#define SPD_MOON RGB(226, 218, 204)
#define SPD_SILK RGB(236, 230, 240)             /* the threads, their glow */
#define SPD_HALO RGB(150, 120, 200)
#define SPD_BONE RGB(236, 228, 214)
#define SPD_BLOOD RGB(176, 18, 44)
#define SPD_HX 120.0f                           /* the web's hub */
#define SPD_HY 156.0f
#define SPD_NSPOKE 14
#define SPD_TURNS 8.0f
#define SPD_RIN 0.2f                            /* the spiral ends at this share of a spoke (the hub's free zone) */
/* the eight tracks' inks (pages.c THEMES[0], "web", on the SMK-37) and the loom's under-thread */
static const uint16_t SPD_INK[8] = {RGB(214, 44, 64), RGB(232, 112, 52), RGB(228, 182, 82), RGB(142, 202, 92),
                                    RGB(72, 182, 162), RGB(104, 142, 232), RGB(164, 104, 232), RGB(222, 112, 192)};
#define SPD_UNDER RGB(40, 30, 50)

static uint16_t spd_sky(int32_t y) { return spd_mix(SPD_SKY0, SPD_SKY1, (uint32_t)(y < 0 ? 0 : y > 239 ? 255 : y * 256 / 240)); }
static float spd_ease(float p) { p = spd_clamp01(p); return p * p * (3.0f - 2.0f * p); }   /* smooth in and out */
static float spd_prog(uint32_t t, uint32_t t0, uint32_t dur) { return t <= t0 ? 0.0f : spd_clamp01((float)(t - t0) / (float)dur); }

/* a thread from a to b, its first share p drawn: a soft halo, then the silk; a = its strength (0..256) */
static void spd_thread(float x0, float y0, float x1, float y1, float p, uint32_t a)
{
    float dx = (x1 - x0) * p, dy = (y1 - y0) * p, n = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
    int32_t i, steps = (int32_t)n + 1;
    if (p <= 0.0f || !spd_rows(y0 < y0 + dy ? y0 : y0 + dy, y0 < y0 + dy ? y0 + dy : y0))
        return;
    for (i = 0; i <= steps; i++) {
        float fx = x0 + dx * (float)i / (float)steps, fy = y0 + dy * (float)i / (float)steps;
        int32_t px = (int32_t)fx, py = (int32_t)fy;
        spd_blend(px, py - 1, SPD_HALO, a / 8u);
        spd_blend(px, py + 1, SPD_HALO, a / 8u);
        spd_blend(px - 1, py, SPD_HALO, a / 8u);
        spd_blend(px + 1, py, SPD_HALO, a / 8u);
        spd_blend(px, py, SPD_SILK, a);
    }
}

/* ---- the night: the sky, a moon, fog, motes */
static void spd_backdrop(uint32_t t)
{
    int32_t y, i, y0 = -cv_oy;
    float in = spd_ease(spd_prog(t, 0u, 500u));
    for (y = y0; y < y0 + (int32_t)cv_h; y++) {  /* the sky, dithered (a dark gradient has few steps in RGB565) */
        static const uint8_t BAYER[4] = {0, 128, 192, 64};
        uint32_t k = (uint32_t)(y < 0 ? 0 : y > 239 ? 239 : y) * 256u / 240u, x;
        uint32_t r = ((SPD_SKY0 >> 11) << 8) + (((SPD_SKY1 >> 11) - (SPD_SKY0 >> 11)) * k);
        uint32_t g = ((SPD_SKY0 >> 5 & 63u) << 8) + (((SPD_SKY1 >> 5 & 63u) - (SPD_SKY0 >> 5 & 63u)) * k);
        uint32_t b = ((SPD_SKY0 & 31u) << 8) + (((SPD_SKY1 & 31u) - (SPD_SKY0 & 31u)) * k);
        for (x = 0; x < 240u; x++) {
            uint32_t d = BAYER[(x & 1u) | ((uint32_t)y & 1u) << 1];
            cv_pset((int32_t)x, y, (uint16_t)(((r + d) >> 8) << 11 | ((g + d) >> 8) << 5 | ((b + d) >> 8)));
        }
    }
    if (spd_rows(70.0f, 150.0f)) {               /* the moon, low on the right, its halo and two old craters */
        spd_disc(204.0f, 110.0f, 40.0f, SPD_HALO, (uint32_t)(18.0f * in));
        spd_disc(204.0f, 110.0f, 30.0f, SPD_HALO, (uint32_t)(26.0f * in));
        spd_disc(204.0f, 110.0f, 21.0f, SPD_MOON, (uint32_t)(236.0f * in));
        spd_disc(197.0f, 104.0f, 4.5f, RGB(196, 186, 176), (uint32_t)(150.0f * in));
        spd_disc(210.0f, 117.0f, 3.0f, RGB(196, 186, 176), (uint32_t)(130.0f * in));
        spd_disc(212.0f, 103.0f, 2.0f, RGB(204, 194, 184), (uint32_t)(120.0f * in));
    }
    for (i = 0; i < 3; i++) {                    /* fog, drifting */
        float fy = 120.0f + (float)i * 42.0f, fx = 120.0f + spd_sin((float)t * 0.0006f + (float)i * 2.1f) * 50.0f;
        spd_ellipse(fx, fy, 150.0f, 9.0f + (float)i * 2.0f, -1000.0f, 1000.0f, RGB(90, 70, 120), (uint32_t)(16.0f * in));
    }
    for (i = 0; i < 16; i++) {                   /* motes, rising */
        uint32_t h = spd_hash(41u, (uint32_t)i);
        float age = (float)t * 0.001f + (float)(h % 997u) * 0.004f, mx, my;
        mx = (float)(h % 240u) + spd_sin(age * 1.7f + (float)i) * 8.0f;
        my = 240.0f - (float)((uint32_t)(age * (14.0f + (float)(h >> 12 & 15u)) + (float)(h >> 8 & 255u)) % 250u);
        if (spd_rows(my - 2.0f, my + 2.0f))
            spd_disc(mx, my, 1.0f, RGB(190, 170, 230), (uint32_t)(70.0f * in));
    }
}

/* ---- dead branches from the edges (the web's moorings), and brambles along the foot */
typedef struct {
    float x, y, a, len, w;
    uint32_t seed;
} spd_br_t;
static void spd_branch(float x, float y, float a, float len, float w, uint32_t seed, uint32_t alpha)
{
    spd_br_t st[24];
    int n = 0, guard = 0;
    st[n++] = (spd_br_t){x, y, a, len, w, seed};
    while (n && guard < 400) {
        spd_br_t b = st[--n];
        spd_rs = b.seed;
        while (b.len > 0.0f && guard < 400) {
            float nx, ny;
            int32_t r;
            b.a += spd_rf(-0.28f, 0.28f);
            nx = b.x + spd_cos(b.a) * 3.0f;
            ny = b.y + spd_sin(b.a) * 3.0f;
            r = (int32_t)(b.w + 0.5f);
            if (spd_rows(ny - b.w - 2.0f, ny + b.w + 2.0f)) {
                spd_disc(nx, ny - 0.8f, b.w * 0.5f + 0.5f, SPD_RIM, alpha * 3u / 4u);   /* the moon's light on top */
                spd_disc(nx, ny + 0.4f, b.w * 0.5f + 0.3f, SPD_BARK, alpha);
            }
            (void)r;
            if ((spd_rand() & 255u) < 22u && n < 24 && b.w > 1.2f) {   /* a twig */
                spd_br_t *c = &st[n++];
                *c = b;
                c->x = nx;
                c->y = ny;
                c->a = b.a + ((spd_rand() & 1u) ? 0.7f : -0.7f) + spd_rf(-0.2f, 0.2f);
                c->len = b.len * spd_rf(0.3f, 0.5f);
                c->w = b.w * 0.6f;
                c->seed = spd_rand() | 1u;
            }
            b.x = nx;
            b.y = ny;
            b.len -= 3.0f;
            b.w = b.w > 0.8f ? b.w * 0.985f : 0.8f;
            guard++;
        }
    }
}
static void spd_trees(uint32_t t)
{
    uint32_t a = (uint32_t)(256.0f * spd_ease(spd_prog(t, 0u, 400u)));
    spd_branch(-6.0f, 104.0f, -0.25f, 64.0f, 6.0f, 0xA11u, a);    /* left, reaching up and in */
    spd_branch(246.0f, 186.0f, 3.4f, 58.0f, 6.5f, 0xB27u, a);     /* right, under the moon */
    if (spd_rows(214.0f, 240.0f)) {
        int i;
        for (i = 0; i < 9; i++) {                /* brambles along the foot */
            uint32_t h = spd_hash(53u, (uint32_t)i);
            spd_branch((float)(h % 240u), 244.0f, -SPD_PI * 0.5f + spd_rf(-0.6f, 0.6f), 16.0f + (float)(h >> 9 & 15u),
                       3.0f, h | 1u, a);
        }
    }
}

/* ---- the web: spokes from the hub to a frame, moorings to the branches, the capture spiral laid outside in */
static float spd_spoke_a(int j) { return (float)j * 2.0f * SPD_PI / SPD_NSPOKE + (float)((int32_t)(spd_hash(61u, (uint32_t)j) % 21u) - 10) * 0.008f; }
static float spd_spoke_r(int j)               /* its length: a little uneven, and kept on the screen */
{
    float a = spd_spoke_a(j), dx = spd_cos(a), dy = spd_sin(a), r = 80.0f + (float)(spd_hash(67u, (uint32_t)j) % 17u), m = 1000.0f;
    if (dx > 0.01f) m = (232.0f - SPD_HX) / dx < m ? (232.0f - SPD_HX) / dx : m;
    if (dx < -0.01f) m = (8.0f - SPD_HX) / dx < m ? (8.0f - SPD_HX) / dx : m;
    if (dy > 0.01f) m = (232.0f - SPD_HY) / dy < m ? (232.0f - SPD_HY) / dy : m;
    if (dy < -0.01f) m = (60.0f - SPD_HY) / dy < m ? (60.0f - SPD_HY) / dy : m;
    return r < m ? r : m;
}
static void spd_spiral_pt(float s, float *x, float *y)   /* s turns from the outside: the point there */
{
    int j = (int)(s * SPD_NSPOKE) % SPD_NSPOKE, j1 = (j + 1) % SPD_NSPOKE;
    float u = s * SPD_NSPOKE - (float)(int)(s * SPD_NSPOKE), f0, f1, x0, y0, x1, y1;
    float sj = (float)(int)(s * SPD_NSPOKE) / SPD_NSPOKE, sj1 = sj + 1.0f / SPD_NSPOKE;
    f0 = 0.94f - (0.94f - SPD_RIN) * sj / SPD_TURNS;
    f1 = 0.94f - (0.94f - SPD_RIN) * sj1 / SPD_TURNS;
    x0 = SPD_HX + spd_cos(spd_spoke_a(j)) * spd_spoke_r(j) * f0;
    y0 = SPD_HY + spd_sin(spd_spoke_a(j)) * spd_spoke_r(j) * f0;
    x1 = SPD_HX + spd_cos(spd_spoke_a(j1)) * spd_spoke_r(j1) * f1;
    y1 = SPD_HY + spd_sin(spd_spoke_a(j1)) * spd_spoke_r(j1) * f1;
    *x = x0 + (x1 - x0) * u;                    /* (straight from spoke to spoke, as the silk lies) */
    *y = y0 + (y1 - y0) * u;
}
#define SPD_T_DROP 250u                         /* the spider drops to the hub */
#define SPD_T_FRAME 1050u                       /* the moorings and the frame */
#define SPD_T_SPOKES 1250u
#define SPD_T_SPIRAL 1650u                      /* .. SPD_T_DONE: the capture spiral */
#define SPD_T_DONE 2950u
static float spd_spiral_s(uint32_t t) { return SPD_TURNS * spd_ease(spd_prog(t, SPD_T_SPIRAL, SPD_T_DONE - SPD_T_SPIRAL)); }
static void spd_web(uint32_t t)
{
    static const float MOOR[3][3] = {{-6.0f, 92.0f, 7.0f}, {246.0f, 176.0f, 2.0f}, {60.0f, 244.0f, 10.0f}};   /* to, from spoke */
    float s = spd_spiral_s(t), x0, y0, x1, y1, q;
    int j, k;
    uint32_t calm = t > SPD_T_DONE ? 190u : 230u;
    if (t < SPD_T_FRAME)
        return;
    for (k = 0; k < 3; k++) {                    /* moorings, out to the branches and the brambles */
        j = (int)MOOR[k][2];
        x0 = SPD_HX + spd_cos(spd_spoke_a(j)) * spd_spoke_r(j);
        y0 = SPD_HY + spd_sin(spd_spoke_a(j)) * spd_spoke_r(j);
        spd_thread(x0, y0, MOOR[k][0], MOOR[k][1], spd_ease(spd_prog(t, SPD_T_FRAME + (uint32_t)k * 60u, 260u)), 150u);
    }
    for (j = 0; j < SPD_NSPOKE; j++) {           /* the frame */
        int j1 = (j + 1) % SPD_NSPOKE;
        x0 = SPD_HX + spd_cos(spd_spoke_a(j)) * spd_spoke_r(j);
        y0 = SPD_HY + spd_sin(spd_spoke_a(j)) * spd_spoke_r(j);
        x1 = SPD_HX + spd_cos(spd_spoke_a(j1)) * spd_spoke_r(j1);
        y1 = SPD_HY + spd_sin(spd_spoke_a(j1)) * spd_spoke_r(j1);
        spd_thread(x0, y0, x1, y1, spd_ease(spd_prog(t, SPD_T_FRAME + 100u + (uint32_t)j * 14u, 200u)), 170u);
    }
    for (j = 0; j < SPD_NSPOKE; j++) {           /* the spokes, shot out from the hub */
        float a = spd_spoke_a(j), r = spd_spoke_r(j);
        spd_thread(SPD_HX, SPD_HY, SPD_HX + spd_cos(a) * r, SPD_HY + spd_sin(a) * r,
                   spd_ease(spd_prog(t, SPD_T_SPOKES + (uint32_t)((j * 5) % SPD_NSPOKE) * 22u, 160u)), 200u);
    }
    for (k = 0; (float)k / SPD_NSPOKE < s; k++) {   /* the capture spiral, as far as it is laid */
        float s0 = (float)k / SPD_NSPOKE, s1 = s0 + 1.0f / SPD_NSPOKE;
        spd_spiral_pt(s0, &x0, &y0);
        spd_spiral_pt(s1 < s ? s1 : s, &x1, &y1);
        spd_thread(x0, y0, x1, y1, 1.0f, s - s0 < 0.6f ? 256u : calm);
        q = s - s0;                              /* dew, a while after it is laid, glinting */
        if (q > 0.5f && (spd_hash(71u, (uint32_t)k) & 3u) == 0u) {
            float dx = (x0 + x1) * 0.5f, dy = (y0 + y1) * 0.5f, tw = 0.5f + 0.5f * spd_sin((float)t * 0.008f + (float)k);
            if (spd_rows(dy - 2.0f, dy + 2.0f)) {
                spd_disc(dx, dy, 1.6f, RGB(200, 220, 255), (uint32_t)(90.0f + 120.0f * tw));
                spd_blend((int32_t)dx, (int32_t)dy - 1, RGB(255, 255, 255), (uint32_t)(120.0f + 130.0f * tw));
            }
        }
    }
}

/* ---- the spider: where it is and which way it faces at t; its body and eight legs */
static void spd_spider_at(uint32_t t, float *x, float *y, float *h, int *moving)
{
    float s = spd_spiral_s(t), p, x1, y1, x0, y0;
    *moving = 1;
    if (t < SPD_T_SPIRAL - 300u) {               /* the drop on its line, then a rest at the hub */
        p = spd_ease(spd_prog(t, SPD_T_DROP, 700u));
        *x = SPD_HX;
        *y = -24.0f + (SPD_HY + 24.0f) * p;
        *h = SPD_PI * 0.5f;                      /* (head down, as it drops) */
        *moving = p < 1.0f;
        return;
    }
    if (t < SPD_T_SPIRAL) {                      /* out along the first spoke to the spiral's start */
        spd_spiral_pt(0.0f, &x1, &y1);
        p = spd_ease(spd_prog(t, SPD_T_SPIRAL - 300u, 300u));
        *x = SPD_HX + (x1 - SPD_HX) * p;
        *y = SPD_HY + (y1 - SPD_HY) * p;
        *h = spd_spoke_a(0);
        return;
    }
    if (t < SPD_T_DONE) {                        /* weaving: on the spiral's end, facing along it */
        spd_spiral_pt(s, x, y);
        spd_spiral_pt(s + 0.02f, &x1, &y1);
        x0 = x1 - *x;
        y0 = y1 - *y;
        *h = (x0 * x0 + y0 * y0 > 0.0001f) ? spd_spoke_a((int)(s * SPD_NSPOKE) % SPD_NSPOKE) + SPD_PI * 0.5f : 0.0f;
        return;
    }
    spd_spiral_pt(SPD_TURNS, &x1, &y1);          /* done: back to the hub, and still, head up */
    p = spd_ease(spd_prog(t, SPD_T_DONE, 300u));
    *x = x1 + (SPD_HX - x1) * p;
    *y = y1 + (SPD_HY - y1) * p;
    *h = -SPD_PI * 0.5f;
    *moving = p < 1.0f;
}
static void spd_line(float x0, float y0, float x1, float y1, float w, uint16_t c, uint32_t a)
{
    float dx = x1 - x0, dy = y1 - y0, n = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
    int32_t i, steps = (int32_t)n + 1;
    if (!spd_rows((y0 < y1 ? y0 : y1) - w, (y0 < y1 ? y1 : y0) + w))
        return;
    for (i = 0; i <= steps; i++)
        spd_disc(x0 + dx * (float)i / (float)steps, y0 + dy * (float)i / (float)steps, w, c, a);
}
static void spd_spider_body(float x, float y, float h, uint32_t t, int moving, float grow, uint16_t col, uint32_t al)
{
    static const float LEG[4] = {0.55f, 1.05f, 1.75f, 2.35f};   /* each pair's angle from the head, either side */
    const float S = 1.5f;                       /* its size */
    float ch = spd_cos(h), sh = spd_sin(h);
    int i, side;
    for (side = -1; side <= 1; side += 2)        /* the legs: up and out to a knee, then down and out to the foot */
        for (i = 0; i < 4; i++) {
            float wig = moving ? spd_sin((float)t * 0.035f + (float)i * 1.6f + (side > 0 ? SPD_PI : 0.0f)) * 0.22f : 0.0f;
            float a1 = h + (float)side * (LEG[i] + wig), a2 = h + (float)side * (LEG[i] + 0.55f + wig);
            float bx = x + ch * 2.0f * S, by = y + sh * 2.0f * S;
            float kx = bx + spd_cos(a1) * 8.0f * S, ky = by + spd_sin(a1) * 8.0f * S - 2.5f * S;
            float fx = kx + spd_cos(a2) * 9.0f * S, fy = ky + spd_sin(a2) * 9.0f * S + 2.5f * S;
            spd_line(bx, by, kx, ky, 1.0f + grow, col, al);
            spd_line(kx, ky, fx, fy, 0.75f + grow, col, al);
        }
    spd_disc(x - ch * 6.5f * S, y - sh * 6.5f * S, 6.2f * S + grow, col, al);   /* the abdomen */
    spd_disc(x + ch * 1.8f * S, y + sh * 1.8f * S, 3.8f * S + grow, col, al);   /* the head */
}
static void spd_spider(uint32_t t)
{
    const float S = 1.5f;
    float x, y, h, ch, sh;
    int moving;
    if (t < SPD_T_DROP)
        return;
    spd_spider_at(t, &x, &y, &h, &moving);
    spd_thread(SPD_HX, -4.0f, SPD_HX, t < SPD_T_SPIRAL - 300u ? y : SPD_HY, 1.0f, 150u);   /* its line from above */
    if (!spd_rows(y - 30.0f, y + 30.0f))
        return;
    ch = spd_cos(h);
    sh = spd_sin(h);
    spd_spider_body(x, y, h, t, moving, 1.2f, SPD_HALO, 110u);   /* the moon's light round its edge */
    spd_spider_body(x, y, h, t, moving, 0.0f, RGB(16, 10, 20), 256u);
    spd_disc(x - ch * 7.8f * S - sh * 2.4f, y - sh * 7.8f * S + ch * 2.4f, 3.4f, RGB(120, 88, 168), 100u);   /* a sheen */
    spd_disc(x - ch * 5.0f * S, y - sh * 5.0f * S, 1.7f, SPD_BLOOD, 256u);   /* the hourglass */
    spd_disc(x - ch * 7.8f * S, y - sh * 7.8f * S, 1.7f, SPD_BLOOD, 256u);
    spd_disc(x + ch * 4.2f * S - sh * 1.5f, y + sh * 4.2f * S + ch * 1.5f, 0.9f, RGB(255, 60, 70), 256u);   /* eyes */
    spd_disc(x + ch * 4.2f * S + sh * 1.5f, y + sh * 4.2f * S - ch * 1.5f, 0.9f, RGB(255, 60, 70), 256u);
}

/* ---- the name, woven in: rows of 3 px, each crossed by the shuttle (left to right, then right to left), bone with
 * a blood shadow; and under it, LOOM-8's own splash in little: eight threads in the tracks' inks, over and under */
#define SPD_T_NAME 2300u
static void spd_title(uint32_t t)
{
    const loom_font_t *f = &LFONT_L;
    const char *s0 = APP_NAME, *s;
    float sc = 200.0f / (float)loom_text_w(f, s0), x, xl;
    int32_t y = 10, qh, w;
    spd_glyph_t g;
    if (sc > 2.4f)
        sc = 2.4f;
    w = (int32_t)((float)loom_text_w(f, s0) * sc);
    xl = (240.0f - (float)w) * 0.5f;
    qh = (int32_t)((float)f->h * sc);
    if (t >= SPD_T_NAME && spd_rows((float)y, (float)(y + qh + 4))) {
        for (x = xl, s = s0; *s; s++) {
            uint32_t gi = loom_glyph(f, (uint8_t)*s);
            int32_t qw, qx, qy, x0 = (int32_t)(x - (float)f->pad * sc);
            spd_glyph(f, gi, &g);
            qw = (int32_t)((float)(g.x + g.w) * sc) + 2;
            for (qy = (int32_t)((float)g.y * sc) - 1; qy < qh; qy++) {
                int32_t py = y + qy, row = qy / 3;
                if (!spd_rows((float)py - 4.0f, (float)py))
                    continue;
                for (qx = 0; qx < qw; qx++) {
                    float u = ((float)qx + 0.5f) / sc - 0.5f, v = ((float)qy + 0.5f) / sc - 0.5f, c, xn;
                    int32_t iu = (int32_t)(u + 1.0f) - 1, iv = (int32_t)(v + 1.0f) - 1, px = x0 + qx;
                    float fu = u - (float)iu, fv = v - (float)iv;
                    c = (spd_cov(&g, iu, iv) * (1.0f - fu) + spd_cov(&g, iu + 1, iv) * fu) * (1.0f - fv) +
                        (spd_cov(&g, iu, iv + 1) * (1.0f - fu) + spd_cov(&g, iu + 1, iv + 1) * fu) * fv;
                    if (c < 0.04f)
                        continue;
                    xn = ((float)px - xl) / (float)(w > 1 ? w : 1);
                    if (t < SPD_T_NAME + (uint32_t)row * 24u + (uint32_t)(((row & 1) ? 1.0f - xn : xn) * 280.0f))
                        continue;                /* the shuttle has not crossed here yet */
                    spd_blend(px + 2, py + 2, SPD_BLOOD, (uint32_t)(c * 200.0f));
                    spd_blend(px, py, SPD_BONE, (uint32_t)(c * 256.0f));
                }
            }
            x += (float)f->adv[gi] * sc;
        }
        {                                        /* the shuttle's thread across the row being woven */
            int32_t row = (int32_t)(t - SPD_T_NAME) / 24, ry = y + row * 3 + 1;
            uint32_t r0 = SPD_T_NAME + (uint32_t)row * 24u;
            if (ry < y + qh && t < r0 + 280u && spd_rows((float)ry, (float)ry)) {
                float p = (float)(t - r0) / 280.0f, a = (row & 1) ? xl + (float)w * (1.0f - p) : xl, b = (row & 1) ? xl + (float)w : xl + (float)w * p;
                spd_thread(a, (float)ry, b, (float)ry, 1.0f, 120u);
            }
        }
    }
    if (t >= 2900u && spd_rows(66.0f, 84.0f)) {   /* LOOM-8's eight threads, woven in row by row */
        int i, j, rows = (int)((t - 2900u) / 45u);
        cv_rect(80, 66, 80, 18, SPD_SKY0);       /* (on a dark ground, clear of the web) */
        for (i = 0; i < 8; i++)
            for (j = 0; j < 7 && j < rows; j++)
                cv_rect(84 + i * 9, 69 + j * 2, 6, 2, (i + j) & 1 ? SPD_INK[i] : SPD_UNDER);
    }
}

/* the screen at t ms, in two bands */
static void spd_frame(uint32_t t)
{
    int32_t y0;
    for (y0 = 0; y0 < 240; y0 += 120) {
        cv_begin(240, 120, SPD_SKY0);
        cv_oy = -y0;                             /* (screen rows y0 .. y0 + 119 on the canvas) */
        cv_y0 = 0;
        cv_y1 = 120;
        spd_backdrop(t);
        spd_trees(t);
        spd_web(t);
        spd_spider(t);
        spd_title(t);
        cv_oy = 0;
        cv_blit(0, (uint32_t)y0);
    }
}

#if !defined(PLAT_HOST) && !defined(SPD_PREVIEW)
/* the boot: nothing else runs yet (no tick, no input), so the chip's own timer paces it and the watchdog is fed */
static void spd_splash(void)
{
    uint32_t t0 = fm1_micros(), t, n = 0;
    do {                                         /* (at most 400 frames, whatever the clock says: never a hang) */
        t = (fm1_micros() - t0) / 1000u;
        if (t > SPD_MS || ++n >= 400u)
            t = SPD_MS;
        spd_frame(t);
        fm1_wdt_feed();
    } while (t < SPD_MS);
}
#else
static void spd_splash(void) { spd_frame(SPD_MS); }
#endif
