/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (SPIDER, the LOOM-8 SMK-37 port) */
/* SPIDER's boot (LOOM-8 on the M-VAVE SMK-37: app.h APP_NAME), about 3.6 s: a night with a moon behind gnarled
 * branches and fog drifting; a spider drops on its line to the hub, turns and walks out as the frame and the spokes
 * shoot out, then walks the capture spiral from the outside in (as orb weavers lay it), dew glinting on what it has
 * laid, and goes back to the hub; the name is woven in, row by row like a shuttle at the loom, and at the foot, small,
 * the repositories it is woven from. The pads light along (the line, a flash at each spoke, a light running round
 * with the spider, a red pulse, then the eight tracks' inks woven in: LOOM-8's eight threads), and a deep note,
 * spidery, sounds under it (spd_snd_*: a cluster that skitters, glides into a wide D minor, the web ringing as it
 * is strung). A key, pad or button ends it (the sound fades out in 150 ms).
 * All of the picture procedural: every frame is drawn again from the time alone (the same seeds, nothing kept), in
 * two bands of 240 x 120 through fm1-base's canvas (gfx.c cv_*; LOOM-8's faces, gfx_loom.c).
 *
 * As Snoop37's boot (genkigenki/sloop-fm1 snoop37, snoop_splash.c, snoop_jingle.c): app_splash (app.c) draws the first
 * still at power-on, and app_splash_late (fm1-base APP_SPLASH_LATE) runs the rest once the platform is up, the audio
 * (the sound is mixed into the render: app.c app_render), the scan (the keys that end it) and the pads' lights with
 * it. Not after a failed boot (fm1-base's bootguard): then the last picture only, no sound.
 * spd_frame(t) draws the screen at t ms, spd_pads(t) the pads. tests/spider_preview.c draws the same frames, the pads
 * and the sound on a PC. */

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
static float spd_atan2(float y, float x)       /* (to about 1e-5 rad) */
{
    float ax = x < 0.0f ? -x : x, ay = y < 0.0f ? -y : y, a, s, r;
    if (ax < 1e-6f && ay < 1e-6f)
        return 0.0f;
    a = ax < ay ? ax / ay : ay / ax;
    s = a * a;
    r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax)
        r = 0.5f * SPD_PI - r;
    if (x < 0.0f)
        r = SPD_PI - r;
    return y < 0.0f ? -r : r;
}
static float spd_turn(float a, float b, float p)   /* from angle a to b by p (0..1), the short way round */
{
    float d = b - a;
    while (d > SPD_PI)
        d -= 2.0f * SPD_PI;
    while (d < -SPD_PI)
        d += 2.0f * SPD_PI;
    return a + d * p;
}

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
#define SPD_T_DROP 250u                         /* .. + 700: the spider drops to the hub */
#define SPD_T_FRAME 1050u                       /* the moorings and the frame */
#define SPD_T_SPOKES 1150u                      /* the spokes, one each 22 ms (spd_spoke_t) */
#define SPD_T_WALK 1150u                        /* .. SPD_T_SPIRAL: it turns and walks out along the first spoke */
#define SPD_T_SPIRAL 1750u                      /* .. SPD_T_DONE: the capture spiral */
#define SPD_T_DONE 3000u
#define SPD_T_HOME 3400u                        /* back at the hub, head up */
static float spd_spiral_s(uint32_t t) { return SPD_TURNS * spd_ease(spd_prog(t, SPD_T_SPIRAL, SPD_T_DONE - SPD_T_SPIRAL)); }
static uint32_t spd_spoke_t(int j) { return SPD_T_SPOKES + (uint32_t)((j * 5) % SPD_NSPOKE) * 22u; }   /* spoke j shot */
static float spd_spiral_h(float s)              /* which way the spiral runs at s, its corners rounded (spokes apart) */
{
    float a = s - 0.035f, b = s + 0.035f, x0, y0, x1, y1;
    spd_spiral_pt(a < 0.0f ? 0.0f : a, &x0, &y0);
    spd_spiral_pt(b > SPD_TURNS ? SPD_TURNS : b, &x1, &y1);
    return spd_atan2(y1 - y0, x1 - x0);
}
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
                   spd_ease(spd_prog(t, spd_spoke_t(j), 160u)), 200u);
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

/* ---- the spider: where it is and which way it faces at t; its body and eight legs. Every move starts and ends at
 * rest (eased), and it turns before it walks: from head down (the drop) round to the way out, along it, round to
 * the spiral's way; on the spiral it faces the way the silk runs; at the end round to the hub, then head up */
static void spd_spider_at(uint32_t t, float *x, float *y, float *h, int *moving)
{
    float p, q, x1, y1, hw, ht;
    *moving = 1;
    if (t < SPD_T_WALK) {                        /* the drop on its line, then a rest at the hub */
        p = spd_ease(spd_prog(t, SPD_T_DROP, 700u));
        *x = SPD_HX;
        *y = -24.0f + (SPD_HY + 24.0f) * p;
        *h = SPD_PI * 0.5f;                      /* (head down, as it drops) */
        *moving = p < 1.0f;
        return;
    }
    if (t < SPD_T_SPIRAL) {                      /* turn, out along the first spoke to the spiral's start, turn */
        spd_spiral_pt(0.0f, &x1, &y1);
        q = spd_prog(t, SPD_T_WALK, SPD_T_SPIRAL - SPD_T_WALK);
        p = spd_ease(q);
        *x = SPD_HX + (x1 - SPD_HX) * p;
        *y = SPD_HY + (y1 - SPD_HY) * p;
        hw = spd_atan2(y1 - SPD_HY, x1 - SPD_HX);
        ht = spd_spiral_h(0.0f);
        *h = q < 0.7f ? spd_turn(SPD_PI * 0.5f, hw, spd_ease(q / 0.35f)) : spd_turn(hw, ht, spd_ease((q - 0.7f) / 0.3f));
        return;
    }
    if (t < SPD_T_DONE) {                        /* weaving: on the spiral's end, facing along it */
        float s = spd_spiral_s(t);
        spd_spiral_pt(s, x, y);
        *h = spd_spiral_h(s);
        return;
    }
    spd_spiral_pt(SPD_TURNS, &x1, &y1);          /* done: round to the hub, back to it, head up, still */
    q = spd_prog(t, SPD_T_DONE, SPD_T_HOME - SPD_T_DONE);
    p = spd_ease(q);
    *x = x1 + (SPD_HX - x1) * p;
    *y = y1 + (SPD_HY - y1) * p;
    hw = spd_atan2(SPD_HY - y1, SPD_HX - x1);
    ht = spd_spiral_h(SPD_TURNS);
    *h = q < 0.6f ? spd_turn(ht, hw, spd_ease(q / 0.3f)) : spd_turn(hw, -SPD_PI * 0.5f, spd_ease((q - 0.6f) / 0.4f));
    *moving = q < 1.0f;
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
    spd_thread(SPD_HX, -4.0f, SPD_HX, t < SPD_T_WALK ? y : SPD_HY, 1.0f, 150u);   /* its line from above */
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
 * a blood shadow */
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
}

/* ---- at the foot, small, over a bank of fog: the repositories SPIDER is woven from (README.md's credits), a line at
 * a time */
static const char *const SPD_CREDIT[3] = {"charlesvestal/fm1-loom + fm1-base", "hugelton/felucca-bsp",
                                          "genkigenki/fm1-to-smk37 + sloop-fm1"};
#define SPD_T_CRED 2850u
static void spd_credits(uint32_t t)
{
    const loom_font_t *f = &LFONT_XS;
    float in = spd_ease(spd_prog(t, SPD_T_CRED, 600u));
    int32_t i, y, top = 240 - 3 * (int32_t)f->h - 3;
    if (in <= 0.0f || !spd_rows((float)(top - 14), 240.0f))
        return;
    for (y = top - 14; y < 240; y++) {           /* the fog, thickening to the foot */
        int32_t x;
        uint32_t a = (uint32_t)(in * (y < top ? (float)(y - top + 14) * 14.0f : 196.0f));
        if (spd_rows((float)y, (float)y))
            for (x = 0; x < 240; x++)
                spd_blend(x, y, SPD_SKY0, a);
    }
    for (i = 0; i < 3; i++) {
        float li = spd_ease(spd_prog(t, SPD_T_CRED + (uint32_t)i * 140u, 400u));
        int32_t w = loom_text_w(f, SPD_CREDIT[i]);
        if (li > 0.0f)
            loom_text((240 - w) / 2, top + i * (int32_t)f->h, f, SPD_CREDIT[i],
                      spd_mix(SPD_SKY0, RGB(168, 148, 198), (uint32_t)(li * 256.0f)));
    }
}

/* ---- the pads along (0..7 the top row, 8..15 the bottom, as plat_pad_rgb; 0xRRGGBB): the line coming down the
 * middle, a flash at each spoke's pad as it shoots (by its direction from the hub), a light running round the pads
 * with the spider as it weaves (round the two rows clockwise, as the spiral), a red pulse as it turns home, then the
 * eight tracks' inks woven in, left to right, at the levels the UI starts with (smk37.c lsmk_lights: the top row
 * 200 / 256 of each track's ink, the bottom row 44) */
#define SPD_SILK_PAD 0xDCD2FFu
static void spd_add(float *c, uint32_t rgb, float a)
{
    c[0] += (float)(rgb >> 16 & 255u) * a;
    c[1] += (float)(rgb >> 8 & 255u) * a;
    c[2] += (float)(rgb & 255u) * a;
}
static uint32_t spd_rgb565(uint16_t c)          /* (as smk37.c lsmk_rgb) */
{
    uint32_t r = (c >> 11) << 3 | (c >> 13), g = ((c >> 5) & 0x3Fu) << 2 | ((c >> 9) & 3u), b = (c & 0x1Fu) << 3 | ((c >> 2) & 7u);
    return r << 16 | g << 8 | b;
}
static int spd_pad_at(float a)                  /* the pad in direction a from the hub */
{
    int col = (int)(3.5f + spd_cos(a) * 4.2f + 0.5f);
    return (spd_sin(a) < 0.0f ? 0 : 8) + (col < 0 ? 0 : col > 7 ? 7 : col);
}
static void spd_pads(uint32_t t, uint32_t *out)
{
    float c[16][3] = {{0.0f}}, d, k;
    int p, j;
    if (t >= SPD_T_DROP && t < SPD_T_DONE + 400u) {   /* the line down the middle, the web's glow once it is strung */
        float q = spd_prog(t, SPD_T_DROP, 700u), fade = 1.0f - spd_prog(t, SPD_T_DONE, 400u);
        float top = 0.3f * spd_clamp01(q * 2.0f) * fade, bot = 0.3f * spd_clamp01(q * 2.0f - 1.0f) * fade;
        float web = 0.07f * spd_ease(spd_prog(t, SPD_T_SPOKES, 400u)) * fade;
        for (p = 0; p < 16; p++)
            spd_add(c[p], SPD_SILK_PAD, web + ((p & 7) == 3 || (p & 7) == 4 ? (p < 8 ? top : bot) : 0.0f));
    }
    for (j = 0; j < SPD_NSPOKE; j++)             /* a flash at each spoke as it shoots */
        if (t >= spd_spoke_t(j) && t < spd_spoke_t(j) + 600u) {
            d = (float)(t - spd_spoke_t(j)) / 70.0f;
            spd_add(c[spd_pad_at(spd_spoke_a(j))], SPD_SILK_PAD, 0.85f / (1.0f + d * d));
        }
    if (t >= SPD_T_SPIRAL && t < SPD_T_DONE + 250u) {   /* a light running round with the spider */
        float x, y, pos, fade = 1.0f - spd_prog(t, SPD_T_DONE, 250u);
        spd_spiral_pt(spd_spiral_s(t < SPD_T_DONE ? t : SPD_T_DONE), &x, &y);
        pos = 3.5f + (spd_atan2(y - SPD_HY, x - SPD_HX) + 0.5f * SPD_PI) * (16.0f / (2.0f * SPD_PI));
        for (j = 0; j < 16; j++) {               /* ring place j: 0..7 the top row, 8..15 the bottom right to left */
            d = pos - (float)j;
            while (d < 0.0f)
                d += 16.0f;
            while (d >= 16.0f)
                d -= 16.0f;
            if (d < 5.0f)
                spd_add(c[j < 8 ? j : 23 - j], SPD_SILK_PAD, 0.8f * (1.0f - d / 5.0f) * (1.0f - d / 5.0f) * fade);
        }
    }
    if (t >= SPD_T_DONE && t < SPD_T_DONE + 600u) {   /* a red pulse as it turns home */
        k = t < SPD_T_DONE + 100u ? (float)(t - SPD_T_DONE) / 100.0f : 1.0f - (float)(t - SPD_T_DONE - 100u) / 500.0f;
        for (p = 0; p < 16; p++)
            spd_add(c[p], spd_rgb565(SPD_BLOOD), 0.6f * k * k);
    }
    for (p = 0; p < 16; p++) {                   /* the eight threads: each track's ink, woven in left to right */
        float o[3] = {0.0f, 0.0f, 0.0f};
        int ch;
        k = spd_ease(spd_prog(t, 3150u + (uint32_t)(p & 7) * 45u, 220u));
        spd_add(o, spd_rgb565(SPD_INK[p & 7]), p < 8 ? 200.0f / 256.0f : 44.0f / 256.0f);
        out[p] = 0;
        for (ch = 0; ch < 3; ch++) {
            float v = c[p][ch] * (1.0f - k) + o[ch] * k;
            out[p] |= (uint32_t)(v > 255.0f ? 255.0f : v < 0.0f ? 0.0f : v) << (16 - 8 * ch);
        }
    }
}

/* ---- the sound (mixed into the render: app.c app_render, spd_snd_mix): a deep note, spidery. Eight voices, each two
 * saws a few cents apart (left, right), start as a quiet cluster around G3..D4 that drifts and skitters (each its
 * own wander, and a quick flutter of its level: legs on silk), then glide apart (1.9 s, eased) into a wide D minor
 * with its ninth (D1 D2 A2 D3 F3 A3 D4 E4) as they swell and the filter opens. The line twangs as the spider lands,
 * the web rings as it is strung (a glassy pluck at each spoke, D minor pentatonic upwards, from the spoke's side),
 * and two small bells when it is done. Peak about -20 dBFS at the volume's middle (the volume follows MASTER, as the
 * render's); it dies away under the UI (SPD_SND_MS). Silent and free when not running; spd_snd_stop() fades out in
 * 150 ms. Its clock is its own sample count. */
#define SPD_FS 44100.0f
#define SPD_NV 8
#define SPD_NP 6                                 /* plucks at once (the quietest is taken for the next) */
#define SPD_NEV (SPD_NSPOKE + 3)
#define SPD_SND_MS 5600u
typedef struct { float ph, inc, mph, amp, peak, dec, idx, gl, gr; } spd_pluck_t;
typedef struct { uint16_t ms; uint8_t note, lvl, pan, len; } spd_ev_t;
static struct {
    volatile uint8_t on, stop;
    uint32_t s, ev;                              /* samples since the start; the next event */
    float fade, ph[SPD_NV][2], lpl, lpr, flut[SPD_NV];
    spd_pluck_t pk[SPD_NP];
    spd_ev_t evs[SPD_NEV];
} spd_snd;
static const int16_t SPD_FROM[SPD_NV] = {5820, 5560, 6050, 5710, 6140, 5900, 5650, 6000};   /* cents: the cluster */
static const int16_t SPD_TO[SPD_NV] = {2600, 3800, 4500, 5000, 5300, 5700, 6200, 6400};     /* D1 D2 A2 D3 F3 A3 D4 E4 */
static const uint8_t SPD_VPAN[SPD_NV] = {128, 92, 168, 60, 196, 110, 150, 214};             /* 0 left .. 255 right */
static const uint8_t SPD_VLVL[SPD_NV] = {170, 210, 190, 220, 200, 190, 180, 150};
static const uint8_t SPD_PENTA[SPD_NSPOKE] = {62, 65, 67, 69, 72, 74, 77, 79, 81, 84, 86, 89, 91, 93};   /* D4 .. A6 */

static float spd_exp2(float x)                  /* 2^x (x -16..16; to about 1e-5) */
{
    union { float f; uint32_t u; } r;
    int32_t i = (int32_t)(x + 16.0f) - 16;
    float f = x - (float)i;
    r.u = (uint32_t)(i + 127) << 23;
    return r.f * (1.0f + f * (0.6931472f + f * (0.2402265f + f * (0.0555041f + f * 0.0096181f))));
}
static float spd_hz(float cents) { return 8.1757989f * spd_exp2(cents * (1.0f / 1200.0f)); }
static float spd_sinp(float ph)                 /* sin(2 pi ph), ph 0..1 (the parabola, refined: about 0.1 %) */
{
    float x = 1.0f - 2.0f * ph, y = 4.0f * x * (1.0f - (x < 0.0f ? -x : x));
    return 0.225f * (y * (y < 0.0f ? -y : y) - y) + y;
}
static float spd_wrap(float ph) { return ph - (float)(int32_t)(ph < 0.0f ? ph - 1.0f : ph); }

static void spd_snd_start(void)
{
    uint32_t i, k = 1;
    spd_snd.on = 0;
    memset((void *)&spd_snd, 0, sizeof spd_snd);
    for (i = 0; i < SPD_NV; i++) {               /* the saws start apart */
        spd_snd.ph[i][0] = (float)(spd_hash(83u, i) & 1023u) / 1024.0f;
        spd_snd.ph[i][1] = (float)(spd_hash(89u, i) & 1023u) / 1024.0f;
    }
    spd_snd.fade = 1.0f;
    spd_snd.evs[0] = (spd_ev_t){SPD_T_DROP + 700u, 50, 150, 128, 60};   /* the line twangs: D3, long */
    for (i = 0; i < SPD_NSPOKE; i++) {           /* a pluck at each spoke, in the order they shoot */
        int j;
        for (j = 0; j < SPD_NSPOKE && (j * 5) % SPD_NSPOKE != (int)i; j++)
            ;
        spd_snd.evs[k++] = (spd_ev_t){(uint16_t)spd_spoke_t(j), SPD_PENTA[i], (uint8_t)(120u - i * 4u),
                                      (uint8_t)(128.0f + spd_cos(spd_spoke_a(j)) * 100.0f), 22};
    }
    spd_snd.evs[k++] = (spd_ev_t){SPD_T_DONE, 86, 80, 96, 40};          /* the web done: D6, A6 */
    spd_snd.evs[k++] = (spd_ev_t){SPD_T_DONE + 70u, 93, 60, 168, 40};
    spd_snd.on = 1;
}
static void spd_snd_stop(void) { spd_snd.stop = 1; }

static void spd_pluck(const spd_ev_t *e)
{
    spd_pluck_t *p = &spd_snd.pk[0];
    uint32_t i;
    for (i = 1; i < SPD_NP; i++)                 /* the quietest */
        if (spd_snd.pk[i].amp + spd_snd.pk[i].peak < p->amp + p->peak)
            p = &spd_snd.pk[i];
    p->inc = spd_hz((float)e->note * 100.0f) * (1.0f / SPD_FS);
    p->peak = (float)e->lvl * (0.2f / 255.0f);
    p->dec = 1.0f - 1.0f / ((float)e->len * 0.01f * SPD_FS);     /* len: the decay's time, 10 ms */
    p->idx = 0.28f;                              /* the FM index (in turns), closing fast */
    p->gl = (float)(255u - e->pan) * (1.0f / 255.0f);
    p->gr = (float)e->pan * (1.0f / 255.0f);
}

static float spd_env(float ms)                  /* the voices' level */
{
    if (ms < 400.0f)
        return 0.2f * spd_ease(ms / 400.0f);
    if (ms < 2600.0f) {
        float q = (ms - 400.0f) / 2200.0f;
        return 0.2f + 0.8f * q * q;
    }
    if (ms < 3400.0f)
        return 1.0f;
    ms = 1.0f - spd_clamp01((ms - 3400.0f) / (float)(SPD_SND_MS - 3400u));
    return ms * ms;
}

static void spd_snd_mix(int32_t *lr, uint32_t n, uint32_t gain_q12)
{
    float t0, t1, a0, a1, da, g, bright, kf, vol, inc[SPD_NV][2], fl0[SPD_NV], dfl[SPD_NV], wl[SPD_NV], wr[SPD_NV];
    uint32_t i, v;
    if (!spd_snd.on || !n)
        return;
    t0 = (float)spd_snd.s * (1000.0f / SPD_FS);
    t1 = (float)(spd_snd.s + n) * (1000.0f / SPD_FS);
    while (spd_snd.ev < SPD_NEV && (float)spd_snd.evs[spd_snd.ev].ms < t1)
        spd_pluck(&spd_snd.evs[spd_snd.ev++]);
    a0 = spd_env(t0);
    a1 = spd_env(t1);
    da = (a1 - a0) / (float)n;
    g = spd_ease(spd_clamp01((t0 - 700.0f) / 1900.0f));   /* the glide, cluster -> chord */
    bright = 350.0f + 5200.0f * a0 * spd_sqrt(a0);
    kf = bright * (2.0f * SPD_PI / SPD_FS);
    if (kf > 0.9f)
        kf = 0.9f;
    for (v = 0; v < SPD_NV; v++) {
        float sec = t0 * 0.001f, c = (float)SPD_FROM[v] + ((float)SPD_TO[v] - (float)SPD_FROM[v]) * g, hz, f1;
        c += (1.0f - g) * (30.0f * spd_sin(sec * 5.7f + (float)v * 1.3f) + 14.0f * spd_sin(sec * 14.1f + (float)v * 2.9f));
        hz = spd_hz(c);
        inc[v][0] = hz * spd_exp2(-5.0f / 1200.0f) * (1.0f / SPD_FS);   /* left a little flat, right a little sharp */
        inc[v][1] = hz * spd_exp2(5.0f / 1200.0f) * (1.0f / SPD_FS);
        f1 = 1.0f - 0.55f * (1.0f - spd_ease(spd_clamp01((t1 - 700.0f) / 1900.0f))) *
                     (0.5f + 0.5f * spd_sin((t1 * 0.001f) * 2.0f * SPD_PI * (9.0f + (float)v) + (float)v * 1.7f));
        fl0[v] = spd_snd.s ? spd_snd.flut[v] : f1;
        dfl[v] = (f1 - fl0[v]) / (float)n;
        spd_snd.flut[v] = f1;
        wl[v] = (float)SPD_VLVL[v] * (0.16f / 255.0f) * (float)(255u - SPD_VPAN[v]) * (2.0f / 255.0f);
        wr[v] = (float)SPD_VLVL[v] * (0.16f / 255.0f) * (float)SPD_VPAN[v] * (2.0f / 255.0f);
    }
    vol = (float)gain_q12 * (4194303.0f / 4096.0f);
    for (i = 0; i < n; i++) {
        float l = 0.0f, r = 0.0f, pl = 0.0f, pr = 0.0f, amp = (a0 + da * (float)i) * spd_snd.fade, x;
        for (v = 0; v < SPD_NV; v++) {
            float m = fl0[v] + dfl[v] * (float)i, s[2];
            int ch;
            for (ch = 0; ch < 2; ch++) {         /* a saw, its jump smoothed (polyBLEP) */
                float ph = spd_snd.ph[v][ch] + inc[v][ch], dt = inc[v][ch], q;
                if (ph >= 1.0f)
                    ph -= 1.0f;
                spd_snd.ph[v][ch] = ph;
                s[ch] = 2.0f * ph - 1.0f;
                if (ph < dt) {
                    q = ph / dt;
                    s[ch] -= q + q - q * q - 1.0f;
                } else if (ph > 1.0f - dt) {
                    q = (ph - 1.0f) / dt;
                    s[ch] -= q * q + q + q + 1.0f;
                }
            }
            l += s[0] * wl[v] * m;
            r += s[1] * wr[v] * m;
        }
        spd_snd.lpl += kf * (l - spd_snd.lpl);
        spd_snd.lpr += kf * (r - spd_snd.lpr);
        for (v = 0; v < SPD_NP; v++) {
            spd_pluck_t *p = &spd_snd.pk[v];
            if (p->peak > 0.0f) {                /* a 1 ms rise, then the decay */
                p->amp += p->peak * 0.023f;
                if (p->amp >= p->peak) {
                    p->amp = p->peak;
                    p->peak = 0.0f;
                }
            } else if (p->amp < 1e-5f) {
                continue;
            } else {
                p->amp *= p->dec;
            }
            p->ph = spd_wrap(p->ph + p->inc);
            p->mph = spd_wrap(p->mph + p->inc * 3.5f);   /* 3.5 : 1, glassy */
            p->idx *= 0.99985f;
            x = spd_sinp(spd_wrap(p->ph + p->idx * spd_sinp(p->mph))) * p->amp;
            pl += x * p->gl;
            pr += x * p->gr;
        }
        x = (spd_snd.lpl * amp + pl * spd_snd.fade) * vol + (float)lr[2u * i];
        lr[2u * i] = (int32_t)(x > 8388606.0f ? 8388606.0f : x < -8388606.0f ? -8388606.0f : x);
        x = (spd_snd.lpr * amp + pr * spd_snd.fade) * vol + (float)lr[2u * i + 1u];
        lr[2u * i + 1u] = (int32_t)(x > 8388606.0f ? 8388606.0f : x < -8388606.0f ? -8388606.0f : x);
    }
    spd_snd.s += n;
    if (spd_snd.stop) {                          /* a key: out in 150 ms */
        spd_snd.fade -= (float)n / (0.15f * SPD_FS);
        if (spd_snd.fade <= 0.0f)
            spd_snd.on = 0;
    }
    if (t1 > (float)SPD_SND_MS)
        spd_snd.on = 0;
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
        spd_credits(t);
        cv_oy = 0;
        cv_blit(0, (uint32_t)y0);
    }
}

#if !defined(PLAT_HOST) && !defined(SPD_PREVIEW)
/* at power-on (app_splash): the first still, the night sky, while the platform comes up */
static void spd_splash(void) { spd_frame(0u); }

static void spd_lights(uint32_t t)
{
    uint32_t c[16];
    int p;
    spd_pads(t, c);
    for (p = 0; p < 16; p++)
        plat_pad_rgb(p, c[p]);
    plat_pad_flush();                            /* (the main loop's job, which has not started) */
}
static int spd_pressed(uint32_t *was)            /* a key, a button or a pad pressed since the last look */
{
    uint32_t now[4] = {plat_keys(), plat_buttons(), plat_smk_buttons(), plat_pads_held()};
    int i, hit = 0;
    for (i = 0; i < 4; i++) {
        hit |= (now[i] & ~was[i]) != 0u;
        was[i] = now[i];
    }
    return hit;
}

/* the platform up (app_splash_late, before the UI): the animation, the pads and the sound, from the chip's timer,
 * the watchdog fed; a press ends it (the sound fades, and the UI waits for the hand to let go so the press plays
 * nothing). show 0 (a failed boot before this one): the last picture and the pads only, no sound */
static void spd_splash_late(int show)
{
    uint32_t t0, t = 0, n = 0, held[4] = {plat_keys(), plat_buttons(), plat_smk_buttons(), plat_pads_held()};
    int pad, vel, skip = 0;
    if (show) {
        spd_snd_start();
        t0 = fm1_micros();
        do {                                     /* (at most 600 frames, whatever the clock says: never a hang) */
            t = (fm1_micros() - t0) / 1000u;
            if (t > SPD_MS || ++n >= 600u)
                t = SPD_MS;
            spd_frame(t);
            spd_lights(t);
            fm1_wdt_feed();
            skip = spd_pressed(held);
        } while (t < SPD_MS && !skip);
    }
    if (skip || !show) {
        spd_snd_stop();
        spd_frame(SPD_MS);
        spd_lights(SPD_MS);
        t0 = fm1_micros();
        while ((plat_keys() | plat_buttons() | plat_smk_buttons() | plat_pads_held()) &&
               fm1_micros() - t0 < 1500000u)
            fm1_wdt_feed();
    }
    while (plat_pad(&pad, &vel))                 /* what was hit meanwhile is not the UI's */
        ;
    while (plat_outer(&pad, &vel))
        ;
}
#else
static void spd_splash(void) { spd_frame(SPD_MS); }
#endif
