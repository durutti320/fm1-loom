/* SPDX-License-Identifier: GPL-3.0-only */
/* SMK-37 DAW mode's REAPER profile (PATCH held + K6 CC MAP: REAPER): SPIDER as a small Mackie Control surface on the
 * second USB MIDI port (fm1-base plat_midi2_in, TO_USB2; fm1.toml smk37_usb_midi_ports = 2), for REAPER's built-in
 * "Mackie Control Universal" control surface (REAPER SDK reaper_csurf/csurf_mcu.cpp: what it does with each message,
 * what it sends back). This header is the protocol, no hardware: the messages SPIDER sends, and what REAPER sends back
 * read into mcu_t (the lights, the strips' names, their pan or volume rings, the faders' positions, the meters, the
 * time display, the bank); app/smk37.c reads the controls, draws the screen and lights the pads from it.
 *
 * Out (cable 1, channel 1): buttons as notes, 127 pressed, 0 let go (note-on, velocity 0: REAPER reads 0x90 only);
 * the faders pitch bend on channels 1..8 (14 bits); the V-pots CC 16..23 and the jog CC 60, relative (1..15 clockwise,
 * 65..79 anticlockwise: the detents); the query F0 00 00 66 14 01 58 59 5A F7, after which REAPER sends everything
 * again (csurf_mcu.cpp OnMCUReset).
 * In: notes 0..0x75 the lights (127 on, 1 blink, 0 off); CC 48..55 the rings (the low nibble 1..11 the position, 0 off);
 * CC 64..73 the time display's ten digits right to left, CC 74..75 the two-digit bank; channel pressure the meters
 * (the high nibble the strip, the low the level 0..13; 14, 15 the clip light); pitch bend channels 1..9 the faders'
 * positions; the LCD SysEx F0 00 00 66 14 12 offset text F7 (two lines of 56; REAPER writes six characters a strip at
 * 0, 7 .. 49 of the top line). REAPER sends the time every 2 s at least: 3 s of nothing is "not connected". */
#pragma once
#include <stdint.h>

/* the Mackie Control notes (buttons and their lights) */
enum {
    MCU_REC0 = 0x00, MCU_SOLO0 = 0x08, MCU_MUTE0 = 0x10, MCU_SEL0 = 0x18, MCU_VSEL0 = 0x20,
    MCU_BANK_L = 0x2E, MCU_BANK_R = 0x2F, MCU_CH_L = 0x30, MCU_CH_R = 0x31, MCU_FLIP = 0x32, MCU_GLOBAL = 0x33,
    MCU_F1 = 0x36, MCU_VIEW0 = 0x3E, MCU_SAVE = 0x50, MCU_UNDO = 0x51, MCU_MARKER = 0x54, MCU_CYCLE = 0x56,
    MCU_CLICK = 0x59, MCU_REW = 0x5B, MCU_FF = 0x5C, MCU_STOP = 0x5D, MCU_PLAY = 0x5E, MCU_RECORD = 0x5F,
    MCU_ZOOM = 0x64, MCU_SCRUB = 0x65, MCU_SMPTE = 0x71, MCU_BEATS = 0x72, MCU_NLED = 0x76
};
/* the CCs: out the V-pots and the jog; in the rings, the time digits, the bank's two */
enum { MCU_VPOT0 = 0x10, MCU_JOG = 0x3C, MCU_RING0 = 0x30, MCU_TC0 = 0x40, MCU_ASSIGN0 = 0x4A };
#define MCU_LCD_N 112u                     /* two lines of 56 */
#define MCU_SILENT_MS 3000u                /* nothing from REAPER this long: not connected */
#define MCU_METER_MS 100u                  /* a meter falls a step this often (as REAPER's own model: 70 dB in 1.4 s) */
#define MCU_NOFADER 0xFFFFu                /* a fader's position not known yet */

#define MCU_PKT(cin, a, b, c) ((uint32_t)(cin) | (uint32_t)(a) << 8 | (uint32_t)(b) << 16 | (uint32_t)(c) << 24)
static uint32_t mcu_note_pkt(uint32_t n, uint32_t on) { return MCU_PKT(0x09, 0x90, n & 127u, on ? 127u : 0u); }
static uint32_t mcu_fader_pkt(uint32_t strip, uint32_t v14)
{
    v14 = v14 > 16383u ? 16383u : v14;
    return MCU_PKT(0x0E, 0xE0u | (strip & 15u), v14 & 127u, v14 >> 7);
}
/* a V-pot (cc MCU_VPOT0 + k) or the jog (MCU_JOG) turned s detents: 1..15 clockwise, 0x40 + 1..15 anticlockwise */
static uint32_t mcu_rel_pkt(uint32_t cc, int32_t s)
{
    uint32_t n = (uint32_t)(s < 0 ? -s : s);
    n = n > 15u ? 15u : n;
    return MCU_PKT(0x0B, 0xB0, cc & 127u, (s < 0 ? 0x40u : 0u) | n);
}
/* the faders' 0..127 as 14 bits (127: the top, 16383) */
static uint32_t mcu_v14(uint32_t v) { return v >= 127u ? 16383u : v << 7 | v; }
/* the query that makes REAPER send everything again: its four packets */
static void mcu_query_pkts(uint32_t *p)
{
    p[0] = MCU_PKT(0x04, 0xF0, 0x00, 0x00);
    p[1] = MCU_PKT(0x04, 0x66, 0x14, 0x01);
    p[2] = MCU_PKT(0x04, 0x58, 0x59, 0x5A);
    p[3] = MCU_PKT(0x05, 0xF7, 0, 0);
}

/* what REAPER said last */
typedef struct {
    uint8_t led[128];                      /* the lights: 0 off, 1 blink, 127 on */
    uint8_t ring[8];                       /* the V-pots' rings as sent (the low nibble 1..11, 0 off) */
    uint8_t meter[8];                      /* the meters 0..13, falling a step every MCU_METER_MS */
    uint8_t clip;                          /* the strips' clip lights (bit s) */
    uint16_t fpos[9];                      /* the faders' positions (14 bits; the master 8; MCU_NOFADER: none yet) */
    char lcd[MCU_LCD_N];                   /* the LCD: the strips' names on the top line */
    uint8_t tc[10];                        /* the time display as sent, leftmost first (bit 6: its dot) */
    uint8_t assign[2];                     /* the two-digit display (REAPER: the first strip's track number) */
    uint32_t last_ms;                      /* when REAPER sent anything last (ms | 1; 0 never) */
    uint32_t meter_t;                      /* the meters' last fall */
    uint32_t gen;                          /* counts every change (the screen redraws on it) */
    uint32_t seen;                         /* messages read */
    uint8_t sx_on, sx_n, sx_pos, sx_hdr[5];   /* a SysEx as it comes in */
} mcu_t;

static void mcu_reset(mcu_t *m)
{
    uint32_t i;
    for (i = 0; i < sizeof *m; i++)
        ((uint8_t *)m)[i] = 0;
    for (i = 0; i < 9u; i++)
        m->fpos[i] = MCU_NOFADER;
    for (i = 0; i < MCU_LCD_N; i++)
        m->lcd[i] = ' ';
}
static int mcu_connected(const mcu_t *m, uint32_t now) { return m->last_ms && now - (m->last_ms & ~1u) < MCU_SILENT_MS; }
static int mcu_lit(const mcu_t *m, uint32_t n) { return m->led[n & 127u] != 0; }

static void mcu_sx_byte(mcu_t *m, uint32_t b)
{
    if (b == 0xF0u) {
        m->sx_on = 1;
        m->sx_n = 0;
        return;
    }
    if (!m->sx_on)
        return;
    if (b & 0x80u) {                       /* F7, or anything else ends it */
        m->sx_on = 0;
        return;
    }
    if (m->sx_n < 5u) {                    /* 00 00 66 14 and the command */
        m->sx_hdr[m->sx_n++] = (uint8_t)b;
        if (m->sx_n == 5u && !(m->sx_hdr[0] == 0 && m->sx_hdr[1] == 0 && m->sx_hdr[2] == 0x66 && m->sx_hdr[3] == 0x14 &&
                               m->sx_hdr[4] == 0x12))
            m->sx_on = 0;                  /* (not the LCD: the meter modes, another device's) */
        return;
    }
    if (m->sx_n == 5u) {                   /* the LCD: its offset, then the text from there */
        m->sx_pos = (uint8_t)b;
        m->sx_n = 6;
        return;
    }
    if (m->sx_pos < MCU_LCD_N) {
        char c = (char)(b >= 0x20u && b < 0x7Fu ? b : ' ');
        if (m->lcd[m->sx_pos] != c) {
            m->lcd[m->sx_pos] = c;
            m->gen++;
        }
        m->sx_pos++;
    }
}
/* one packet from the second port (the cable cleared), at now (ms) */
static void mcu_in(mcu_t *m, uint32_t pkt, uint32_t now)
{
    uint32_t cin = pkt & 15u, st = pkt >> 8 & 0xFFu, d1 = pkt >> 16 & 0xFFu, d2 = pkt >> 24 & 0xFFu, k;
    m->last_ms = now | 1u;
    m->seen++;
    if (cin >= 4u && cin <= 7u) {          /* SysEx: 3 bytes going on; 1, 2, 3 ending */
        uint32_t nb = cin == 4u || cin == 7u ? 3u : cin - 4u;
        for (k = 0; k < nb; k++)
            mcu_sx_byte(m, pkt >> (8u * (k + 1u)) & 0xFFu);
        return;
    }
    switch (st & 0xF0u) {
    case 0x90:
    case 0x80: {
        uint8_t v = (uint8_t)((st & 0xF0u) == 0x80u ? 0u : d2);
        if (m->led[d1 & 127u] != v)
            m->led[d1 & 127u] = v, m->gen++;
        break;
    }
    case 0xB0:
        if (d1 >= MCU_RING0 && d1 < MCU_RING0 + 8u) {
            if (m->ring[d1 - MCU_RING0] != d2)
                m->ring[d1 - MCU_RING0] = (uint8_t)d2, m->gen++;
        } else if (d1 >= MCU_TC0 && d1 < MCU_TC0 + 10u) {
            k = 9u - (d1 - MCU_TC0);       /* (right to left) */
            if (m->tc[k] != d2)
                m->tc[k] = (uint8_t)d2, m->gen++;
        } else if (d1 == MCU_ASSIGN0 || d1 == MCU_ASSIGN0 + 1u) {
            k = d1 == MCU_ASSIGN0 ? 1u : 0u;   /* (74 the right digit, 75 the left) */
            if (m->assign[k] != d2)
                m->assign[k] = (uint8_t)d2, m->gen++;
        }
        break;
    case 0xD0:
        k = d1 >> 4 & 7u;
        if ((d1 & 15u) <= 13u) {
            if ((d1 & 15u) >= m->meter[k] || (d1 & 15u) == 0u) {   /* up at once; down as it falls (0: cleared) */
                if (m->meter[k] != (d1 & 15u))
                    m->meter[k] = (uint8_t)(d1 & 15u), m->gen++;
            }
        } else {
            uint8_t c = (uint8_t)((d1 & 15u) == 14u ? m->clip | 1u << k : m->clip & ~(1u << k));
            if (c != m->clip)
                m->clip = c, m->gen++;
        }
        break;
    case 0xE0:
        k = st & 15u;
        if (k < 9u) {
            uint16_t v = (uint16_t)(d1 | d2 << 7);
            if (m->fpos[k] != v)
                m->fpos[k] = v, m->gen++;
        }
        break;
    default:
        break;
    }
}
/* the meters fall (call often; now in ms) */
static void mcu_tick(mcu_t *m, uint32_t now)
{
    uint32_t s, any = 0;
    if (now - m->meter_t < MCU_METER_MS)
        return;
    m->meter_t = now;
    for (s = 0; s < 8u; s++)
        if (m->meter[s])
            m->meter[s]--, any = 1;
    if (any)
        m->gen++;
}

/* a strip's name (the LCD's top line, six characters at 7 x strip), trimmed, into b (7 bytes); 0: none (blank) */
static int mcu_name(const mcu_t *m, uint32_t strip, char *b)
{
    uint32_t i, n = 0, lo = 6u;
    const char *p = m->lcd + 7u * (strip & 7u);
    while (lo && p[lo - 1u] == ' ')
        lo--;
    for (i = 0; i < lo && p[i] == ' '; i++)
        ;
    for (; i < lo; i++)
        b[n++] = p[i];
    b[n] = 0;
    return n != 0;
}
/* one time digit as text: a digit, a letter, or a space (0: blank) */
static char mcu_tc_char(uint32_t v)
{
    v &= 0x3Fu;
    return (char)(v == 0u ? ' ' : v < 0x20u ? v + 0x40u : v);
}
/* the time display as text into b (16 bytes): REAPER's groups 3 2 2 3 (bars, beats, -, ticks / hours, minutes,
 * seconds, ms), the beats light "12.3.50", else "00:12.345"; leading blanks dropped. 0: nothing yet */
static int mcu_time(const mcu_t *m, char *b)
{
    static const uint8_t G0[4] = {0, 3, 5, 7}, GN[4] = {3, 2, 2, 3};
    int beats = mcu_lit(m, MCU_BEATS);
    uint32_t g, i, n = 0, any = 0;
    for (g = 0; g < 4u; g++) {
        char t[3];
        uint32_t k = 0, blank = 1;
        if (beats && g == 2u)
            continue;                      /* (beats: the third group unused) */
        for (i = 0; i < GN[g]; i++) {
            t[k] = mcu_tc_char(m->tc[G0[g] + i]);
            blank &= t[k] == ' ';
            k++;
        }
        if (beats && g == 3u)
            k = 2;                         /* (the ticks: two of the three) */
        if (blank && !any)
            continue;
        if (any)
            b[n++] = beats || g == 3u ? '.' : ':';
        for (i = 0, blank = 1; i < k; i++) {
            if (t[i] == ' ' && i + 1u < k && blank)
                continue;                  /* (a group's leading blanks; a blank one: 0) */
            blank = 0;
            b[n++] = t[i] == ' ' ? '0' : t[i];
        }
        any = 1;
    }
    b[n] = 0;
    return any;
}
