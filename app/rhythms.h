/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (SPIDER) */
/* SPIDER's rhythms: classic drum patterns for a kit track, on its Steps page's list (pages.c list_steps): RHYTHM (K3
 * picks one, SCALE writes it into the pattern: its steps, length and swing, the pattern's old locks cleared; PATCH +
 * REC undoes it) and RHYTHM + KIT (the same, and the factory kit it was written for). Each is programmed here from
 * scratch, step by step, for LOOM-8's kits: their keys go 1 kick, 2 snare, 3 closed hat, 4 open hat (3 and 4 choke
 * each other), 5 clap, 6 low tom, 7 high tom, 8 percussion, 9 shaker, 10 cymbal, 11 bell (tools/gen_kits.py).
 *
 * A row: a key's letter, a space, then a character a step (a sixteenth): '.' none, 'g' a ghost (45), 'o' soft (80),
 * 'x' a hit (104), 'X' an accent (127). A kit step has one velocity for all its sounds: the loudest of them.
 * Keys: K kick, S snare, H hat, O open hat, C clap, L low tom, T high tom, P percussion, Z shaker, Y cymbal, B bell.
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
    {"motorik", "Burlap", 16, 0, {
        "K x.....x.x.......",
        "S ....x.......x...",
        "H X.x.x.x.X.x.x.x.",
     }, 0},
    {"d-beat", "Burlap", 16, 0, {              /* the forbidden beat: kick, snare, kick-kick, snare (two in the back half) */
        "K x.......x.x.....",
        "S ....X.......X...",
        "H x.x.x.x.x.x.x.x.",
        "Y X...............",
     }, 0},
    {"backbeat", "Burlap", 16, 0, {            /* the money beat: the kick on 1 and 3 */
        "K x.......x.......",
        "S ....X.......X...",
        "H x.o.x.o.x.o.x.o.",
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
        "Y X...............",
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
        "P x..x..x...x..x..",                  /* the cross-stick's clave */
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
        "Y ..x.....x.....x.",                  /* (Sequin's cowbell) */
     }, 0},
    {"one drop", "Linen", 16, 30, {           /* reggae: nothing on 1, the kick and rim on 3 (Linen's key 8: a rim) */
        "K ........X.......",
        "P ........x.......",
        "H x.x.x.x.x.x.x.x.",
     }, 0},
    {"rockers", "Linen", 16, 24, {
        "K x.......x.......",
        "P ........x.......",
        "H x.xxx.xxx.xxx.xx",
     }, 0},
    {"steppers", "Linen", 16, 20, {
        "K x...x...x...x...",
        "P ........x.......",
        "H ..x...x...x...x.",
     }, 0},
};
#define NRHYTHM ((int)(sizeof RHYTHM / sizeof RHYTHM[0]))

static int rh_key(char c)                         /* a row's letter -> the key (0..10), -1 none */
{
    static const char KEYS[] = "KSHOCLTPZYB";
    for (int i = 0; KEYS[i]; i++)
        if (KEYS[i] == c)
            return i;
    return -1;
}
static int rh_vel(char c) { return c == 'X' ? 127 : c == 'x' ? 104 : c == 'o' ? 80 : c == 'g' ? 45 : 0; }
static int rh_nudge(char c) { return c >= '1' && c <= '9' ? c - '0' : c >= 'a' && c <= 'i' ? -(c - 'a' + 1) : 0; }

/* step i of a rhythm: its sounds (bit k: key k + 1) and velocity (0: none) */
static uint32_t rhythm_step(const rhythm_t *r, int i, int *vel)
{
    uint32_t mask = 0;
    *vel = 0;
    for (int j = 0; j < RH_ROWS && r->row[j]; j++) {
        int key = rh_key(r->row[j][0]), v;
        const char *s = r->row[j] + 2;
        int n = 0;
        while (s[n])
            n++;
        if (key < 0 || i >= n || !(v = rh_vel(s[i])))
            continue;
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
        if (s_eq(kit_name(k), r->kit))
            return k;
    return -1;
}
