/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (FLY, the LOOM-8 SMK-37 port) */
/* FLY's boot (LOOM-8 on the M-VAVE SMK-37: app.h APP_NAME): about 3.6 s drawn as the unit starts. Mycelium threads
 * creep in from the soil and the edges, branching and glowing as they go; mushrooms pop up out of the moss (a big fly
 * agaric and a small one, both smiling, a chanterelle, a porcini, an amethyst deceiver, a cluster of glowing mycena);
 * the name grows in, red with white spots; fireflies and spores drift. All of it procedural: every frame is drawn
 * again from the time alone (the same seeds, nothing kept), in two bands of 240 x 120 through fm1-base's canvas
 * (gfx.c cv_*; LOOM-8's faces, gfx_loom.c), so it costs no RAM beyond the canvas and a few KB of code.
 *
 * fly_frame(t) draws the screen at t ms; app_splash (app.c) runs it from the chip's free-running timer until FLY_MS,
 * feeding the watchdog (nothing else runs yet). tests/fly_preview.c draws the same frames on a PC. */

#define FLY_MS 3600u
#define FLY_GROUND 198                        /* the soil's top, screen y */
#define FLY_PI 3.14159265f

/* ---- a little maths without libm */
static uint32_t fly_rs = 1u;
static uint32_t fly_rand(void)
{
    fly_rs ^= fly_rs << 13;
    fly_rs ^= fly_rs >> 17;
    fly_rs ^= fly_rs << 5;
    return fly_rs;
}
static float fly_rf(float a, float b) { return a + (b - a) * (float)(fly_rand() & 0xFFFFu) * (1.0f / 65535.0f); }
static uint32_t fly_hash(uint32_t a, uint32_t b)
{
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    return h ^ (h >> 12);
}
static float fly_sin(float x)
{
    float y;
    int32_t k = (int32_t)(x * (0.5f / FLY_PI) + (x >= 0.0f ? 0.5f : -0.5f));   /* into -pi .. pi */
    x -= (float)k * 2.0f * FLY_PI;
    y = 1.27323954f * x - 0.40528473f * x * (x < 0.0f ? -x : x);
    return 0.225f * (y * (y < 0.0f ? -y : y) - y) + y;
}
static float fly_cos(float x) { return fly_sin(x + 0.5f * FLY_PI); }
static float fly_sqrt(float v)
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
static float fly_clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }
static float fly_pop(uint32_t t, uint32_t t0, uint32_t dur)   /* 0 .. 1 with a bounce past 1 (an ease out, back) */
{
    float p, q;
    if (t <= t0)
        return 0.0f;
    p = fly_clamp01((float)(t - t0) / (float)dur) - 1.0f;
    q = 1.0f + 2.70158f * p * p * p + 1.70158f * p * p;
    return q < 0.0f ? 0.0f : q;
}

/* ---- colours, and drawing in screen rows on the band (cv_oy = -the band's top) */
static uint16_t fly_mix(uint16_t a, uint16_t b, uint32_t k)   /* a -> b by k / 256 */
{
    uint32_t r = ((a >> 11) * (256u - k) + (b >> 11) * k) >> 8, g = ((a >> 5 & 63u) * (256u - k) + (b >> 5 & 63u) * k) >> 8,
             bl = ((a & 31u) * (256u - k) + (b & 31u) * k) >> 8;
    return (uint16_t)(r << 11 | g << 5 | bl);
}
static void fly_blend(int32_t x, int32_t y, uint16_t c, uint32_t a)   /* a 0 .. 256 */
{
    uint32_t i;
    y += cv_oy;
    if ((uint32_t)x >= cv_w || y < cv_y0 || y >= cv_y1 || !a)
        return;
    i = (uint32_t)y * cv_w + (uint32_t)x;
    cv_px[i] = swap16(a >= 256u ? c : fly_mix(swap16(cv_px[i]), c, a));
}
static int fly_rows(float y0, float y1)       /* any of [y0, y1] in the band */
{
    return y1 >= (float)(-cv_oy - 2) && y0 <= (float)(-cv_oy + (int32_t)cv_h + 2);
}
/* an ellipse (rows from y_top to y_bot only), its edges smoothed */
static void fly_ellipse(float cx, float cy, float rx, float ry, float y_top, float y_bot, uint16_t c, uint32_t a)
{
    int32_t y, ya, yb, x;
    if (rx < 0.5f || ry < 0.5f)
        return;
    ya = (int32_t)(cy - ry > y_top ? cy - ry : y_top);
    yb = (int32_t)(cy + ry < y_bot ? cy + ry : y_bot);
    if (!fly_rows((float)ya, (float)yb))
        return;
    for (y = ya; y <= yb; y++) {
        float dy = ((float)y + 0.5f - cy) / ry, hw, l, r;
        if (dy <= -1.0f || dy >= 1.0f)
            continue;
        hw = rx * fly_sqrt(1.0f - dy * dy);
        l = cx - hw;
        r = cx + hw;
        for (x = (int32_t)l; x <= (int32_t)r; x++) {
            float cov = 1.0f;
            if ((float)x < l)
                cov = (float)x + 1.0f - l;
            if ((float)x + 1.0f > r)
                cov -= (float)x + 1.0f - r;
            if (cov > 0.0f)
                fly_blend(x, y, c, (uint32_t)(cov * (float)a));
        }
    }
}
static void fly_disc(float cx, float cy, float r, uint16_t c, uint32_t a)
{
    fly_ellipse(cx, cy, r, r, -1000.0f, 1000.0f, c, a);
}

/* ---- the scene's colours */
#define FLY_SKY0 RGB(14, 12, 34)                /* the sky at the top, at the trees' line, the soil */
#define FLY_SKY1 RGB(24, 40, 52)
#define FLY_SOIL0 RGB(52, 34, 40)
#define FLY_SOIL1 RGB(28, 18, 26)
#define FLY_MOSS RGB(96, 170, 78)
#define FLY_THREAD RGB(244, 238, 214)           /* the mycelium's core, and its glow */
#define FLY_GLOW RGB(96, 200, 190)
#define FLY_RED RGB(230, 44, 38)
#define FLY_CREAM RGB(250, 244, 228)
static const uint16_t FLY_PASTEL[6] = {RGB(255, 150, 190), RGB(130, 230, 255), RGB(255, 226, 110), RGB(170, 250, 150),
                                       RGB(200, 160, 255), RGB(255, 180, 120)};

static uint16_t fly_sky(int32_t y)
{
    if (y >= FLY_GROUND)
        return fly_mix(FLY_SOIL0, FLY_SOIL1, (uint32_t)((y - FLY_GROUND) * 256 / (240 - FLY_GROUND)));
    return fly_mix(FLY_SKY0, FLY_SKY1, (uint32_t)(y * 256 / FLY_GROUND));
}
static void fly_backdrop(uint32_t t)
{
    int32_t y, i, y0 = -cv_oy;
    for (y = y0; y < y0 + (int32_t)cv_h; y++)
        cv_rect(0, y, 240, 1, fly_sky(y));
    for (i = 0; i < 26; i++) {                  /* stars, twinkling */
        uint32_t h = fly_hash(7u, (uint32_t)i);
        int32_t sx = (int32_t)(h % 240u), sy = (int32_t)(h >> 8 & 127u);
        float tw = 0.5f + 0.5f * fly_sin((float)t * 0.004f + (float)(h >> 16 & 63u));
        fly_blend(sx, sy, FLY_CREAM, (uint32_t)(40.0f + 110.0f * tw));
    }
    if (fly_rows(FLY_GROUND - 8, FLY_GROUND + 2)) {   /* the moss along the soil, tufts of it */
        cv_rect(0, FLY_GROUND, 240, 2, FLY_MOSS);
        for (i = 0; i < 40; i++) {
            uint32_t h = fly_hash(11u, (uint32_t)i);
            int32_t tx = (int32_t)(h % 240u), th = 2 + (int32_t)(h >> 9 & 5u), k;
            for (k = 0; k < th; k++)
                fly_blend(tx + (k * (int32_t)(h >> 14 & 1u ? 1 : -1)) / 3, FLY_GROUND - k, FLY_MOSS, 220u);
        }
    }
}

/* ---- the mycelium: threads from the soil and the edges, each step 5 px a while after the last, forking now and
 * then; a branch not yet grown stops there (and its forks with it). Each branch draws from its own seed, so a frame
 * draws exactly the threads of the one before, and more */
typedef struct {
    float x, y, a, aim, left;
    uint32_t t, seed;
    uint8_t depth;
} fly_br_t;
static void fly_seg(float x0, float y0, float x1, float y1, int32_t th, uint32_t a)
{
    float dx = x1 - x0, dy = y1 - y0, n = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
    int32_t i, steps = (int32_t)n + 1, j, k;
    if (!fly_rows(y0 < y1 ? y0 : y1, y0 < y1 ? y1 : y0))
        return;
    for (i = 0; i <= steps; i++) {
        int32_t px = (int32_t)(x0 + dx * (float)i / (float)steps), py = (int32_t)(y0 + dy * (float)i / (float)steps);
        for (j = -1; j <= th; j++)               /* the glow around, then the thread */
            for (k = -1; k <= th; k++)
                fly_blend(px + j, py + k, FLY_GLOW, a * 3u / 16u);
        for (j = 0; j < th; j++)
            for (k = 0; k < th; k++)
                fly_blend(px + j, py + k, FLY_THREAD, a);
    }
}
static void fly_mycelium(uint32_t t)
{
    fly_br_t st[64];
    int n = 0, segs = 0, i;
    for (i = 0; i < 10; i++) {                  /* the roots: six from the soil, two from each side */
        fly_br_t *b = &st[n++];
        fly_rs = fly_hash(0xF1u, (uint32_t)i) | 1u;
        if (i < 6) {
            b->x = 18.0f + (float)i * 41.0f + fly_rf(-8.0f, 8.0f);
            b->y = 242.0f;
            b->aim = -0.5f * FLY_PI + fly_rf(-0.45f, 0.45f);
            b->left = fly_rf(190.0f, 240.0f);
            b->t = (uint32_t)i * 70u;
        } else {
            int right = i >= 8;
            b->x = right ? 242.0f : -2.0f;
            b->y = (i & 1) ? 70.0f + fly_rf(0.0f, 30.0f) : 140.0f + fly_rf(0.0f, 30.0f);
            b->aim = (right ? FLY_PI : 0.0f) + fly_rf(-0.3f, 0.3f);
            b->left = fly_rf(120.0f, 160.0f);
            b->t = 180u + (uint32_t)(i - 6) * 90u;
        }
        b->a = b->aim;
        b->depth = 0;
        b->seed = fly_rand() | 1u;
    }
    while (n && segs < 1800) {
        fly_br_t b = st[--n];
        uint32_t dt = b.depth ? 26u : 32u;
        int32_t th = b.depth ? 1 : 2;
        fly_rs = b.seed;
        while (b.left > 0.0f && segs < 1800) {
            float nx, ny;
            uint32_t age, a;
            if (b.t > t)
                break;                           /* not grown yet: this branch ends here, this frame */
            b.a += fly_rf(-0.34f, 0.34f) + (b.aim - b.a) * 0.12f;   /* wandering, around its way */
            nx = b.x + fly_cos(b.a) * 5.0f;
            ny = b.y + fly_sin(b.a) * 5.0f;
            age = t - b.t;
            a = age < 160u ? 256u : age > 1100u ? 92u : 256u - (age - 160u) * 164u / 940u;   /* bright new, then calm */
            if (t > 1900u)                       /* (and calmer still once the name grows in) */
                a = a * (256u - (t - 1900u > 700u ? 700u : t - 1900u) * 110u / 700u) >> 8;
            fly_seg(b.x, b.y, nx, ny, th, a);
            if (age < dt * 2u && fly_rows(ny - 3.0f, ny + 3.0f)) {   /* the growing tip: a spark */
                uint16_t c = FLY_PASTEL[b.seed % 6u];
                fly_disc(nx + 0.5f, ny + 0.5f, 2.6f, c, 120u);
                fly_disc(nx + 0.5f, ny + 0.5f, 1.2f, FLY_CREAM, 256u);
            }
            segs++;
            if (b.depth < 3 && (fly_rand() & 255u) < 26u && n < 64) {   /* a fork */
                fly_br_t *c = &st[n++];
                float side = (fly_rand() & 1u) ? 1.0f : -1.0f;
                *c = b;
                c->x = nx;
                c->y = ny;
                c->a = c->aim = b.a + side * fly_rf(0.5f, 1.1f);
                c->left = b.left * fly_rf(0.35f, 0.6f);
                c->t = b.t + dt;
                c->depth = (uint8_t)(b.depth + 1);
                c->seed = fly_rand() | 1u;
            }
            b.x = nx;
            b.y = ny;
            b.left -= 5.0f;
            b.t += dt;
            if (nx < -12.0f || nx > 252.0f || ny < -12.0f || ny > 252.0f)
                break;
        }
    }
}

/* ---- the mushrooms */
enum { FM_FLY, FM_CHANT, FM_PORCINI, FM_AMETHYST, FM_MYCENA };
typedef struct {
    uint8_t kind, face;
    int16_t x, h, r;
    uint16_t t0;
} fly_shroom_t;
static const fly_shroom_t FLY_SHROOMS[] = {         /* back to front */
    {FM_PORCINI, 0, 26, 26, 17, 1250}, {FM_AMETHYST, 0, 150, 20, 9, 1650}, {FM_CHANT, 0, 174, 30, 15, 1150},
    {FM_MYCENA, 0, 196, 30, 4, 1600},  {FM_MYCENA, 0, 214, 36, 5, 1500},   {FM_MYCENA, 0, 204, 46, 6, 1350},
    {FM_FLY, 1, 122, 40, 18, 1450},    {FM_FLY, 1, 72, 70, 33, 950},
};
#define FLY_NSHROOM (sizeof FLY_SHROOMS / sizeof FLY_SHROOMS[0])
static const float FLY_SPOTS[7][3] = {{-0.55f, -0.35f, 0.13f}, {-0.1f, -0.74f, 0.12f}, {0.42f, -0.5f, 0.15f},
                                      {0.74f, -0.18f, 0.09f}, {-0.82f, -0.12f, 0.08f}, {0.14f, -0.28f, 0.09f},
                                      {-0.36f, -0.66f, 0.07f}};

static void fly_face(float cx, float y, float w, uint32_t t, uint32_t t0)
{
    uint16_t ink = RGB(70, 40, 44);
    float ex = w * 0.24f;
    int blink = (t > 2950u && t < 3060u) || (t > t0 + 900u && t < t0 + 980u);
    if (blink) {
        fly_ellipse(cx - ex, y, 1.8f, 0.7f, -1000.0f, 1000.0f, ink, 256u);
        fly_ellipse(cx + ex, y, 1.8f, 0.7f, -1000.0f, 1000.0f, ink, 256u);
    } else {
        fly_ellipse(cx - ex, y, 1.6f, 2.4f, -1000.0f, 1000.0f, ink, 256u);
        fly_ellipse(cx + ex, y, 1.6f, 2.4f, -1000.0f, 1000.0f, ink, 256u);
        fly_disc(cx - ex + 0.5f, y - 1.0f, 0.6f, FLY_CREAM, 256u);   /* a glint */
        fly_disc(cx + ex + 0.5f, y - 1.0f, 0.6f, FLY_CREAM, 256u);
    }
    fly_disc(cx - ex * 1.7f, y + 3.5f, 1.8f, RGB(255, 140, 150), 140u);   /* cheeks */
    fly_disc(cx + ex * 1.7f, y + 3.5f, 1.8f, RGB(255, 140, 150), 140u);
    fly_ellipse(cx, y + 3.6f, ex * 0.55f, 2.0f, y + 3.6f, 1000.0f, ink, 256u);   /* the smile: a half ellipse */
    fly_ellipse(cx, y + 3.4f, ex * 0.42f, 1.2f, y + 3.4f, 1000.0f, RGB(244, 236, 214), 256u);
}
static void fly_shroom(const fly_shroom_t *m, uint32_t t)
{
    float g = fly_pop(t, m->t0, 620u), cg, hs, ws, top, cx = (float)m->x, gy = (float)FLY_GROUND + 1.0f, r = (float)m->r;
    if (g <= 0.0f)
        return;
    cg = fly_clamp01((g - 0.25f) / 0.75f) * (g > 1.0f ? g : 1.0f);
    hs = (float)m->h * (g > 1.0f ? 1.0f + (g - 1.0f) * 0.5f : g);
    top = gy - hs;
    if (!fly_rows(top - r * 1.6f, gy + 4.0f))
        return;
    switch (m->kind) {
    case FM_FLY:
        ws = r * 0.44f;
        fly_ellipse(cx, gy - 2.0f, ws * 0.8f, ws * 0.42f, -1000.0f, 1000.0f, RGB(222, 210, 186), 256u);   /* its bulb */
        fly_ellipse(cx, top + hs * 0.5f, ws * 0.5f, hs * 0.5f, -1000.0f, 1000.0f, RGB(244, 236, 214), 256u);
        fly_ellipse(cx + ws * 0.22f, top + hs * 0.5f, ws * 0.24f, hs * 0.48f, -1000.0f, 1000.0f, RGB(222, 210, 186), 140u);
        if (hs > 10.0f)                          /* the ring */
            fly_ellipse(cx, top + hs * 0.22f, ws * 0.72f, 2.2f, -1000.0f, 1000.0f, RGB(250, 246, 236), 256u);
        if (m->face && g > 0.85f)
            fly_face(cx, top + hs * 0.55f, ws, t, m->t0);
        if (cg > 0.0f) {
            float rx = r * cg, ry = r * 0.72f * cg, cy = top + ry * 0.12f;
            int i;
            fly_ellipse(cx, cy, rx * 0.9f, ry * 0.2f, -1000.0f, 1000.0f, RGB(236, 220, 196), 256u);   /* the gills */
            fly_ellipse(cx, cy, rx, ry, -1000.0f, cy, FLY_RED, 256u);
            fly_ellipse(cx - rx * 0.22f, cy - ry * 0.18f, rx * 0.6f, ry * 0.62f, -1000.0f, cy - ry * 0.25f,
                        RGB(255, 98, 76), 110u);    /* a shine */
            for (i = 0; i < 7; i++) {
                float sx = cx + FLY_SPOTS[i][0] * rx, sy = cy + FLY_SPOTS[i][1] * ry, sr = FLY_SPOTS[i][2] * r * cg;
                fly_disc(sx, sy, sr, FLY_CREAM, 256u);
            }
        }
        break;
    case FM_PORCINI:
        ws = r * 0.78f;
        fly_ellipse(cx, top + hs * 0.55f, ws * 0.55f, hs * 0.55f, -1000.0f, 1000.0f, RGB(228, 206, 168), 256u);
        if (cg > 0.0f) {
            float rx = r * cg, ry = r * 0.78f * cg, cy = top + ry * 0.2f;
            fly_ellipse(cx, cy, rx * 0.92f, ry * 0.22f, -1000.0f, 1000.0f, RGB(214, 196, 120), 256u);
            fly_ellipse(cx, cy, rx, ry, -1000.0f, cy, RGB(150, 88, 50), 256u);
            fly_ellipse(cx - rx * 0.2f, cy - ry * 0.3f, rx * 0.55f, ry * 0.45f, -1000.0f, cy, RGB(184, 120, 72), 150u);
        }
        break;
    case FM_AMETHYST:
        ws = r * 0.36f;
        fly_ellipse(cx, top + hs * 0.5f, ws, hs * 0.5f, -1000.0f, 1000.0f, RGB(184, 140, 236), 256u);
        if (cg > 0.0f) {
            float rx = r * cg, ry = r * 0.6f * cg, cy = top + ry * 0.2f;
            fly_ellipse(cx, cy, rx, ry, -1000.0f, cy + ry * 0.1f, RGB(150, 92, 220), 256u);
            fly_ellipse(cx - rx * 0.25f, cy - ry * 0.3f, rx * 0.45f, ry * 0.4f, -1000.0f, cy, RGB(196, 150, 255), 140u);
        }
        break;
    case FM_CHANT: {
        int32_t y;
        for (y = (int32_t)top; y <= (int32_t)gy; y++) {   /* a funnel: narrow at the foot, wide at the rim */
            float p = fly_clamp01((gy - (float)y) / (hs > 1.0f ? hs : 1.0f)), w = r * (0.28f + 0.72f * p * p * cg);
            float x0 = cx - w;
            int32_t x;
            if (!fly_rows((float)y, (float)y))
                continue;
            for (x = (int32_t)x0; x <= (int32_t)(cx + w); x++)
                fly_blend(x, y, fly_mix(RGB(250, 176, 44), RGB(214, 120, 24), (uint32_t)(p * 120.0f)), 256u);
        }
        if (cg > 0.0f) {
            fly_ellipse(cx, top, r * cg, r * 0.26f * cg, -1000.0f, 1000.0f, RGB(255, 200, 80), 256u);
            fly_ellipse(cx, top + 0.5f, r * 0.72f * cg, r * 0.14f * cg, -1000.0f, 1000.0f, RGB(224, 136, 30), 256u);
        }
        break;
    }
    default: {                                   /* mycena: a thin stem, a bell, glowing (and breathing) */
        float glow = 0.75f + 0.25f * fly_sin((float)t * 0.006f + (float)m->x);
        int32_t y;
        for (y = (int32_t)top; y <= (int32_t)gy; y++) {
            float p = (gy - (float)y) / (hs > 1.0f ? hs : 1.0f);
            if (fly_rows((float)y, (float)y)) {
                fly_blend((int32_t)(cx + fly_sin(p * 1.6f) * 3.0f), y, RGB(214, 236, 244), 256u);
                fly_blend((int32_t)(cx + fly_sin(p * 1.6f) * 3.0f) + 1, y, RGB(170, 206, 222), 200u);
            }
        }
        if (cg > 0.0f) {
            float bx = cx + fly_sin(1.6f) * 3.0f + 0.5f;
            fly_disc(bx, top, r * 3.2f * cg, RGB(70, 200, 255), (uint32_t)(46.0f * glow));
            fly_disc(bx, top, r * 1.9f * cg, RGB(90, 210, 255), (uint32_t)(70.0f * glow));
            fly_ellipse(bx, top + r * 0.4f * cg, r * cg, r * 1.2f * cg, -1000.0f, top + r * 0.4f * cg,
                        RGB(130, 230, 255), 256u);
            fly_ellipse(bx - r * 0.3f * cg, top - r * 0.2f * cg, r * 0.35f * cg, r * 0.5f * cg, -1000.0f, 1000.0f,
                        RGB(220, 250, 255), 200u);
        }
        break;
    }
    }
}

/* ---- the name, FLY_TS times FONT_L (smoothly scaled), red with white spots, grown in from the left with a ragged
 * edge; and its line under it, faded in */
#define FLY_TS 2.4f
static int fly_spot(int32_t x, int32_t y)       /* a spot in x, y's cell of 13 x 13 (each wholly in its cell) */
{
    int32_t cx = x / 13, cy = y / 13;
    uint32_t h = fly_hash((uint32_t)cx * 131u, (uint32_t)cy);
    int32_t r = 3 + (int32_t)(h >> 16 & 1u), sx = cx * 13 + 4 + (int32_t)(h % 5u), sy = cy * 13 + 4 + (int32_t)(h >> 8 & 7u) % 5;
    return (h >> 20 & 3u) != 0u && (x - sx) * (x - sx) + (y - sy) * (y - sy) <= r * r;
}
/* a glyph of LOOM-8's faces (gfx_loom.c: its ink box only, maybe run-length coded) into g: its box, coverage 0..15 */
#define FLY_GMAX (48 * 44)
typedef struct { int32_t x, y, w, h; uint8_t c[FLY_GMAX]; } fly_glyph_t;
static void fly_glyph(const loom_font_t *f, uint32_t gi, fly_glyph_t *g)
{
    const uint8_t *gd = f->data + f->off[gi], *bx = f->box[gi];
    uint32_t i, n, nib = 0, run = 0, a = 0;
    g->x = bx[0];
    g->y = bx[1];
    g->w = bx[2];
    g->h = bx[3];
    n = (uint32_t)(g->w * g->h);
    if (n > FLY_GMAX)
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
static float fly_cov(const fly_glyph_t *g, int32_t x, int32_t y)   /* at a cell pixel (from the pen - pad, the top) */
{
    x -= g->x;
    y -= g->y;
    if (x < 0 || y < 0 || x >= g->w || y >= g->h)
        return 0.0f;
    return (float)g->c[y * g->w + x] * (1.0f / 15.0f);
}
static void fly_title(uint32_t t)
{
    const loom_font_t *f = &LFONT_L;
    const char *s = APP_NAME;
    const uint32_t T0 = 1850u;
    float x = (240.0f - (float)loom_text_w(f, s) * FLY_TS) * 0.5f;
    int32_t y = 4, qh = (int32_t)((float)f->h * FLY_TS);
    fly_glyph_t g;
    if (t < T0 || !fly_rows((float)y, (float)(y + qh + 4)))
        goto line;
    for (; *s; s++) {
        uint32_t gi = loom_glyph(f, (uint8_t)*s);
        int32_t qw, qx, qy, x0 = (int32_t)(x - (float)f->pad * FLY_TS);
        fly_glyph(f, gi, &g);
        qw = (int32_t)((float)(g.x + g.w) * FLY_TS) + 2;
        for (qy = (int32_t)((float)g.y * FLY_TS) - 1; qy < qh; qy++) {
            int32_t py = y + qy;
            if (!fly_rows((float)py - 4.0f, (float)py))
                continue;
            for (qx = 0; qx < qw; qx++) {
                float u = ((float)qx + 0.5f) / FLY_TS - 0.5f, v = ((float)qy + 0.5f) / FLY_TS - 0.5f, c;
                int32_t iu = (int32_t)(u + 1.0f) - 1, iv = (int32_t)(v + 1.0f) - 1, px = x0 + qx;
                float fu = u - (float)iu, fv = v - (float)iv;
                c = (fly_cov(&g, iu, iv) * (1.0f - fu) + fly_cov(&g, iu + 1, iv) * fu) * (1.0f - fv) +
                    (fly_cov(&g, iu, iv + 1) * (1.0f - fu) + fly_cov(&g, iu + 1, iv + 1) * fu) * fv;
                if (c < 0.04f)
                    continue;
                if (t < T0 + 860u && t < T0 + (uint32_t)(px * 3) + fly_hash((uint32_t)px >> 2, (uint32_t)py >> 2) % 140u)
                    continue;                    /* not grown in yet */
                fly_blend(px + 3, py + 3, RGB(6, 4, 16), (uint32_t)(c * 170.0f));   /* its shadow */
                fly_blend(px, py, fly_spot(px, py) ? FLY_CREAM : FLY_RED, (uint32_t)(c * 256.0f));
            }
        }
        x += (float)f->adv[gi] * FLY_TS;
    }
line:
    if (t > 2500u && fly_rows(84.0f, 102.0f)) {
        const char *l = "it grows on you";
        uint32_t k = (uint32_t)(fly_clamp01((float)(t - 2500u) / 500.0f) * 256.0f);
        int32_t lx = (240 - loom_text_w(&LFONT_S, l)) / 2;
        loom_text(lx + 1, 87, &LFONT_S, l, fly_mix(fly_sky(94), RGB(6, 4, 16), k));
        loom_text(lx, 86, &LFONT_S, l, fly_mix(fly_sky(94), FLY_CREAM, k));
    }
}

/* ---- the little lights: fireflies over it all, spores rising from the caps */
static void fly_lights(uint32_t t)
{
    int i;
    for (i = 0; i < 9; i++) {                   /* fireflies, from 1.6 s */
        uint32_t h = fly_hash(23u, (uint32_t)i);
        float ft = (float)t * 0.001f, bx = (float)(h % 220u) + 10.0f, by = 100.0f + (float)(h >> 8 & 63u);
        float fx = bx + fly_sin(ft * (0.8f + (float)(h >> 16 & 7u) * 0.1f) + (float)i) * 14.0f;
        float fy = by + fly_cos(ft * 1.1f + (float)i * 2.0f) * 9.0f;
        float on = fly_clamp01((float)((int32_t)t - 1600 - i * 90) / 400.0f) * (0.6f + 0.4f * fly_sin(ft * 5.0f + (float)i));
        if (on <= 0.0f || !fly_rows(fy - 6.0f, fy + 6.0f))
            continue;
        fly_disc(fx, fy, 5.0f, RGB(200, 255, 110), (uint32_t)(50.0f * on));
        fly_disc(fx, fy, 1.6f, RGB(250, 255, 200), (uint32_t)(256.0f * on));
    }
    for (i = 0; i < 18; i++) {                  /* spores: up from the caps once they are open */
        const fly_shroom_t *m = &FLY_SHROOMS[i % FLY_NSHROOM];
        uint32_t h = fly_hash(31u, (uint32_t)i), born = m->t0 + 500u + (h % 900u);
        float age, sx, sy;
        if (t < born)
            continue;
        age = (float)(t - born) * 0.001f;
        sx = (float)m->x + fly_sin(age * 3.0f + (float)i) * 6.0f + (float)((int32_t)(h >> 8 & 15u) - 8);
        sy = (float)(FLY_GROUND - m->h - m->r / 2) - age * (18.0f + (float)(h >> 12 & 15u));
        if (!fly_rows(sy - 4.0f, sy + 4.0f))
            continue;
        fly_disc(sx, sy, 2.4f, FLY_PASTEL[h % 6u], (uint32_t)(130.0f * (1.0f - fly_clamp01(age / 2.0f))));
        fly_disc(sx, sy, 0.9f, FLY_CREAM, (uint32_t)(256.0f * (1.0f - fly_clamp01(age / 2.0f))));
    }
}

/* the screen at t ms, in two bands */
static void fly_frame(uint32_t t)
{
    int32_t y0;
    uint32_t i;
    for (y0 = 0; y0 < 240; y0 += 120) {
        cv_begin(240, 120, FLY_SKY0);
        cv_oy = -y0;                             /* (screen rows y0 .. y0 + 119 on the canvas) */
        cv_y0 = 0;
        cv_y1 = 120;
        fly_backdrop(t);
        fly_mycelium(t);
        for (i = 0; i < FLY_NSHROOM; i++)
            fly_shroom(&FLY_SHROOMS[i], t);
        fly_lights(t);
        fly_title(t);
        cv_oy = 0;
        cv_blit(0, (uint32_t)y0);
    }
}

#if !defined(PLAT_HOST) && !defined(FLY_PREVIEW)
/* the boot: nothing else runs yet (no tick, no input), so the chip's own timer paces it and the watchdog is fed */
static void fly_splash(void)
{
    uint32_t t0 = fm1_micros(), t;
    do {
        t = (fm1_micros() - t0) / 1000u;
        if (t > FLY_MS)
            t = FLY_MS;
        fly_frame(t);
        fm1_wdt_feed();
    } while (t < FLY_MS);
}
#else
static void fly_splash(void) { fly_frame(FLY_MS); }
#endif
