/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (SPIDER) */
/* SPIDER's rhythms: classic drum patterns for a kit track, on its Steps page's list (pages.c list_steps): RHYTHM (K3
 * picks one, SCALE writes it into the pattern: its steps, length and swing, the pattern's old locks cleared; PATCH +
 * REC undoes it) and RHYTHM + KIT (the same, and the factory kit it was written for). Each is programmed here from
 * scratch, step by step, for the kits: their keys go 1 kick, 2 snare, 3 closed hat, 4 open hat (3 and 4 choke each
 * other), 5 clap, 6 low tom, 7 high tom, 8 percussion (a conga), 9 shaker; 10 and 11 each kit its own (SPIDER: a
 * string and an FM blip on LOOM-8's, the crash and the rim on the acoustic: tools/gen_kits.py).
 *
 * A row: a key's letter, a space, then a character a step (a sixteenth): '.' none, 'g' a ghost (45), 'o' soft (80),
 * 'x' a hit (104), 'X' an accent (127). Each sound keeps its own: the step plays at its loudest's, the others by
 * their own velocities (18 Nov 2026: a ghost under a hat stays a ghost, the hats under the kick stay soft).
 * Keys: K kick, S snare, H hat, O open hat, C clap, L low tom, T high tom, P percussion, Z shaker, B key 11 (a bell,
 * a cowbell; the acoustic's rim). Sounds found in the kit the rhythm lands on, by what they are: R the rim (else the
 * snare), Y a ride (else the closed hat), W a crash (else the open hat).
 * nudge (optional): a character a step, '.' on the grid, '1'..'9' that many ticks late, 'a'..'i' 1..9 early (24
 * ticks a step). groove: the pattern's own groove amount (the project's type, swing unless changed). */
#pragma once
#define RH_ROWS 6
typedef struct {
    const char *name, *kit;
    uint8_t len;
    int8_t groove;
    const char *row[RH_ROWS];
    const char *nudge;
} rhythm_t;

static const rhythm_t RHYTHM[] = {
    /* Keep Chopping's drum module (keepchopping.neocities.org), its punk and psych grooves step for step; its tempo
     * in each comment */
    {"workhorse", "Burlap", 16, 0, {           /* 85: the backbeat; 170 and up, the Ramones */
        "K x.......x.......",
        "S ....x.......x...",
        "H o.o.o.o.o.o.o.o.",
     }, 0},
    {"d-beat", "Burlap", 16, 0, {              /* 175: the Discharge tumble, the kick rolling through the snare */
        "K x.....x.x.....x.",
        "S ....x.......x...",
        "H o.o.o.o.o.o.o.o.",
     }, 0},
    {"skank", "Burlap", 16, 0, {               /* 190: hardcore's double time */
        "K x...x...x...x...",
        "S ..x...x...x...x.",
        "H o.o.o.o.o.o.o.o.",
     }, 0},
    {"motorik", "Burlap", 16, 0, {             /* 130: krautrock, straight sixteenths */
        "K x.......x.......",
        "S ....x.......x...",
        "H oooooooooooooooo",
     }, 0},
    {"drone two-step", "Burlap", 16, 0, {      /* 100: Spacemen 3, the hat off the beat */
        "K x.......x.......",
        "S ............x...",
        "H ..o...o...o...o.",
     }, 0},
    {"dembow", "Sequin", 16, 0, {
        "K x...x...x...x...",
        "S ...x..x....x..x.",
        "H ..o...o...o...o.",
     }, 0},
    {"four on the floor", "Denim", 16, 0, {    /* house */
        "K x...x...x...x...",
        "C ....x.......x...",
        "O ..x...x...x...x.",
        "Z oooooooooooooooo",
     }, 0},
    {"disco", "Denim", 16, 0, {
        "K x...x...x...x...",
        "S ....X.......X...",
        "H x...x...x...x...",
        "O ..x...x...x...x.",
        "P ......o.....o.o.",
     }, 0},
    {"breakbeat", "Burlap", 16, 0, {           /* a funk break, programmed (no record sampled) */
        "K x.x.......xx....",
        "S ....X..g.g..X..g",
        "H x.x.x.x.x.x.x.x.",
     }, 0},
    {"boom bap", "Velvet", 16, 28, {
        "K x......x..x.....",
        "S ....X.......X...",
        "H x.x.x.x.x.x.x.o.",
     }, 0},
    {"dilla", "Velvet", 16, 0, {               /* unquantised: the kick early, the snare late, the hats drunk */
        "K x......x..x...x.",
        "S ....X.......X...",
        "H x.oxx.oxx.oxx.ox",
     }, "..425.4c..526.4."},
    {"two-step", "Linen", 16, 46, {            /* UK garage */
        "K x.........x.....",
        "C ....x.......x...",
        "H ..x...x...x...x.",
        "Z oooooooooooooooo",
     }, 0},
    {"drum and bass", "Burlap", 16, 0, {
        "K x.........x.....",
        "S ....X..g.g..X..g",
        "H x.x.x.x.x.x.x.x.",
        "W X...............",
     }, 0},
    {"trap", "Sequin", 16, 0, {                /* half time: the clap on 3, hats rolling */
        "K x......x..x.....",
        "C ........X.......",
        "H x.x.x.x.xxxxx.x.",
        "O ..............o.",
     }, 0},
    {"shuffle", "Burlap", 16, 66, {            /* the blues' triplet feel: swing all the way */
        "K x.......x.......",
        "S ....X.......X...",
        "Y x.x.x.x.x.x.x.x.",
     }, 0},
    {"bossa nova", "Linen", 16, 0, {
        "K x..xx..xx..xx..x",
        "R x..x..x...x..x..",                  /* the cross-stick's clave */
        "H o.o.o.o.o.o.o.o.",
     }, 0},
    {"samba", "Loom", 16, 18, {
        "K ....X.......X...",                  /* the surdo's answer on 2 and 4 */
        "P x.x..x.x.x..x.x.",                  /* tamborim */
        "Z XoooXoooXoooXooo",
        "L x.......x.......",
     }, 0},
    {"cumbia", "Loom", 16, 0, {                /* the tambora on 1 and 3, the llamador off the beat, the guacharaca's
                                                * long-short-short */
        "K x.......x.......",
        "P ..x...x...x...x.",
        "Z X.ooX.ooX.ooX.oo",
        "L ..............o.",
     }, 0},
    {"afrobeat", "Loom", 16, 12, {
        "K x.....x...x.....",
        "S ....x..g.g..x..g",
        "H x.xxx.xxx.xxx.xx",
        "O ..............x.",
        "P ..x..x....x..x..",
     }, 0},
    {"electro", "Sequin", 16, 0, {
        "K x......x..x.....",
        "S ....X.......X...",
        "H xoxoxoxoxoxoxoxo",
        "B ..x.....x.....x.",                  /* (Sequin's cowbell: an FM blip; the acoustic's rim) */
     }, 0},
    {"techno", "Denim", 16, 0, {               /* 909: the kick on every beat, the open hat between, a choked tick */
        "K x...x...x...x...",
        "O ..x...x...x...x.",
        "H .o.o.o.o.o.o.o.o",
        "C ....x.......x...",
        "P ...o......o...o.",
     }, 0},
    /* Keep Chopping's reggae and dub grooves, step for step (Linen's key 8: a rim) */
    {"one drop", "Linen", 16, 0, {             /* 75: nothing on 1, the kick and rim together on 3 */
        "K ........x.......",
        "R ........x.......",
        "H o.o.o.o.o.o.o.o.",
     }, 0},
    {"steppers", "Linen", 16, 0, {             /* 76: the kick on all four, the rim on 3 */
        "K x...x...x...x...",
        "R ........x.......",
        "H o.o.o.o.o.o.o.o.",
     }, 0},
    {"rockers", "Linen", 16, 0, {              /* 78: the kick on 1 and 3 and a pickup into the bar, the snare on 3 */
        "K x.......x.....x.",
        "S ........x.......",
        "H o.o.o.o.o.o.o.o.",
     }, 0},
};
#define NRHYTHM ((int)(sizeof RHYTHM / sizeof RHYTHM[0]))

/* the kit's key (0..10) whose sound is a rim (want 0), a ride (1), a crash (2), by its model or its sample; -1 none */
static int rh_find(const uint8_t *par, int want)
{
    static const char *const AC[3] = {"ac rim", "ac ride", "ac crash"};   /* (the acoustic kit's samples) */
    static const uint8_t DM[3][2] = {{DM_XRIM, DM_909RIM}, {DM_909RIDE, DM_909RIDE}, {DM_909CRASH, DM_909CRASH}};
    for (int k = NDRUM - 1; k >= 0; k--) {
        int m = par[TP_DRUM(k, DP_MODEL)];
        if (m == DM[want][0] || m == DM[want][1] || (m == DM_SAMPLE && s_eq(smp_name(par[TP_DRUM(k, DP_K1)]), AC[want])))
            return k;
    }
    return -1;
}
static int rh_key(char c, const uint8_t *par)     /* a row's letter -> the key (0..10) in the kit par, -1 none */
{
    static const char KEYS[] = "KSHOCLTPZ?B";     /* (10: no letter, each kit its own) */
    int k;
    if (c == 'R' || c == 'Y' || c == 'W')         /* by what the kit has: the rim, else the snare; a ride, else the
                                                   * closed hat; a crash, else the open hat */
        return (k = rh_find(par, c == 'R' ? 0 : c == 'Y' ? 1 : 2)) >= 0 ? k : c == 'R' ? 1 : c == 'Y' ? 2 : 3;
    for (int i = 0; KEYS[i]; i++)
        if (KEYS[i] == c && c != '?')
            return i;
    return -1;
}
static int rh_vel(char c) { return c == 'X' ? 127 : c == 'x' ? 104 : c == 'o' ? 80 : c == 'g' ? 45 : 0; }
static int rh_nudge(char c) { return c >= '1' && c <= '9' ? c - '0' : c >= 'a' && c <= 'i' ? -(c - 'a' + 1) : 0; }

/* step i of a rhythm on the kit par: its sounds (bit k: key k + 1), each one's velocity (vk[k]), the loudest's (*vel;
 * 0: none) */
static uint32_t rhythm_step(const rhythm_t *r, int i, const uint8_t *par, uint8_t *vk, int *vel)
{
    uint32_t mask = 0;
    *vel = 0;
    for (int j = 0; j < RH_ROWS && r->row[j]; j++) {
        int key = rh_key(r->row[j][0], par), v;
        const char *s = r->row[j] + 2;
        int n = 0;
        while (s[n])
            n++;
        if (key < 0 || i >= n || !(v = rh_vel(s[i])))
            continue;
        if (!(mask >> key & 1u) || v > vk[key])    /* (two rows on one key: the louder) */
            vk[key] = (uint8_t)v;
        mask |= 1u << key;
        if (v > *vel)
            *vel = v;
    }
    return mask;
}
/* the factory kit the rhythm was written for (pages.c's kit numbers), -1 none */
static int rhythm_kit(const rhythm_t *r)
{
    for (int k = 0; k < NFKITS; k++)
        if (s_eq(kit_id(k), r->kit))                 /* (by LOOM-8's name: SPIDER shows another) */
            return k;
    return -1;
}
