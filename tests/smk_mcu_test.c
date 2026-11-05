/* SPDX-License-Identifier: GPL-3.0-only */
/* SMK-37 DAW mode's REAPER profile (app/smk_mcu.h), no hardware: the Mackie Control messages SPIDER sends (buttons,
 * faders, V-pots, the jog, the query), and what REAPER sends back as REAPER's own surface sends it (csurf_mcu.cpp): the
 * lights, the rings, the faders' positions, the meters and their fall, the time display in beats and in time, the
 * bank, the LCD's SysEx split across packets as USB brings it, a SysEx for another device left alone, the names. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../app/smk_mcu.h"

static int fails;
static void check(int ok, const char *what) { printf("smk_mcu: %-84s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }
#define PKT(cin, a, b, c) ((uint32_t)(cin) | (uint32_t)(a) << 8 | (uint32_t)(b) << 16 | (uint32_t)(c) << 24)

/* a SysEx as USB-MIDI packets into m (3 bytes a packet; the last 1..3 with its end) */
static void sysex(mcu_t *m, const uint8_t *b, uint32_t n, uint32_t now)
{
    uint32_t i = 0;
    while (i < n) {
        uint32_t k = n - i >= 3u ? 3u : n - i, end = i + k == n;
        uint32_t cin = end ? 4u + k : 4u;
        if (end && k == 3u)
            cin = 7u;
        mcu_in(m, PKT(cin, b[i], k > 1u ? b[i + 1] : 0, k > 2u ? b[i + 2] : 0), now);
        i += k;
    }
}
/* REAPER's UpdateMackieDisplay: F0 00 00 66 14 12 pos text, padded with spaces to pad, F7 */
static void lcd(mcu_t *m, uint32_t pos, const char *s, uint32_t pad, uint32_t now)
{
    uint8_t b[200];
    uint32_t n = 0, l = (uint32_t)strlen(s);
    b[n++] = 0xF0, b[n++] = 0, b[n++] = 0, b[n++] = 0x66, b[n++] = 0x14, b[n++] = 0x12, b[n++] = (uint8_t)pos;
    for (uint32_t i = 0; i < pad; i++)
        b[n++] = (uint8_t)(i < l ? s[i] : ' ');
    b[n++] = 0xF7;
    sysex(m, b, n, now);
}
/* REAPER's time display: ten characters, leftmost first (0 blank), sent right to left as CC 64..73 */
static void tc(mcu_t *m, const char *ten, uint32_t now)
{
    for (uint32_t x = 0; x < 10u; x++) {
        char c = ten[9 - x];
        mcu_in(m, PKT(0x0B, 0xB0, 0x40 + x, c == '_' ? 0 : (uint8_t)c), now);
    }
}

int main(void)
{
    static mcu_t m;
    uint32_t p[4], i, ok;
    char b[32];

    /* ---- out ---- */
    check(mcu_note_pkt(MCU_PLAY, 1) == PKT(0x09, 0x90, 0x5E, 127) && mcu_note_pkt(MCU_PLAY, 0) == PKT(0x09, 0x90, 0x5E, 0),
          "a button: note-on 127 pressed, note-on 0 let go (REAPER reads 0x90 only)");
    check(mcu_fader_pkt(0, 0) == PKT(0x0E, 0xE0, 0, 0) && mcu_fader_pkt(7, 16383) == PKT(0x0E, 0xE7, 127, 127) &&
          mcu_fader_pkt(2, 99999) == PKT(0x0E, 0xE2, 127, 127), "a fader: pitch bend on its strip's channel, 14 bits, capped");
    check(mcu_v14(0) == 0 && mcu_v14(127) == 16383 && mcu_v14(64) == (64u << 7 | 64u), "fader 0..127 -> 0..16383, the top the top");
    check(mcu_rel_pkt(MCU_VPOT0 + 2, 1) == PKT(0x0B, 0xB0, 0x12, 1) && mcu_rel_pkt(MCU_VPOT0, -3) == PKT(0x0B, 0xB0, 0x10, 0x43),
          "a V-pot: CC 16 + k, 1..15 clockwise, 0x40 + n anticlockwise");
    check(mcu_rel_pkt(MCU_JOG, 40) == PKT(0x0B, 0xB0, 0x3C, 15) && mcu_rel_pkt(MCU_JOG, -40) == PKT(0x0B, 0xB0, 0x3C, 0x4F),
          "the jog: CC 60, at most 15 detents a message");
    mcu_query_pkts(p);
    check(p[0] == PKT(4, 0xF0, 0, 0) && p[1] == PKT(4, 0x66, 0x14, 0x01) && p[2] == PKT(4, 0x58, 0x59, 0x5A) &&
          p[3] == PKT(5, 0xF7, 0, 0), "the query: F0 00 00 66 14 01 58 59 5A F7 (REAPER's OnMCUReset matches its 9 bytes)");

    /* ---- in ---- */
    mcu_reset(&m);
    check(!mcu_connected(&m, 1000) && m.fpos[0] == MCU_NOFADER && m.lcd[0] == ' ', "at rest: not connected, no fader known, LCD blank");
    mcu_in(&m, PKT(0x09, 0x90, MCU_PLAY, 127), 5000);
    mcu_in(&m, PKT(0x09, 0x90, MCU_SOLO0 + 2, 1), 5000);
    mcu_in(&m, PKT(0x09, 0x90, MCU_MUTE0 + 3, 127), 5000);
    check(mcu_lit(&m, MCU_PLAY) && m.led[MCU_SOLO0 + 2] == 1 && mcu_lit(&m, MCU_MUTE0 + 3) && !mcu_lit(&m, MCU_RECORD),
          "lights: 127 on, 1 blink (REAPER's solo), the rest off");
    mcu_in(&m, PKT(0x09, 0x90, MCU_MUTE0 + 3, 0), 5000);
    mcu_in(&m, PKT(0x08, 0x80, MCU_PLAY, 64), 5000);
    check(!mcu_lit(&m, MCU_MUTE0 + 3) && !mcu_lit(&m, MCU_PLAY), "off: velocity 0, or a note-off");
    check(mcu_connected(&m, 7999) && !mcu_connected(&m, 8000), "connected: something within 3 s");
    mcu_in(&m, PKT(0x0B, 0xB0, MCU_RING0 + 1, 6), 5000);
    mcu_in(&m, PKT(0x0B, 0xB0, MCU_RING0 + 7, 11), 5000);
    check(m.ring[1] == 6 && m.ring[7] == 11 && m.ring[0] == 0, "rings: CC 48..55 (REAPER: 1 + pan x 11 / 128: 6 the centre)");
    mcu_in(&m, PKT(0x0E, 0xE1, 0x00, 0x60), 5000);
    mcu_in(&m, PKT(0x0E, 0xE8, 0x7F, 0x7F), 5000);
    check(m.fpos[1] == 0x3000 && m.fpos[8] == 16383 && m.fpos[0] == MCU_NOFADER, "faders' positions: pitch bend channel 1..9");
    mcu_in(&m, PKT(0x0D, 0xD0, 0x2C, 0), 5000);
    mcu_in(&m, PKT(0x0D, 0xD0, 0x0E, 0), 5000);
    check(m.meter[2] == 12 && (m.clip & 1u) && m.meter[0] == 0, "meters: high nibble the strip, low the level; 14 the clip light");
    mcu_in(&m, PKT(0x0D, 0xD0, 0x0F, 0), 5000);
    mcu_in(&m, PKT(0x0D, 0xD0, 0x25, 0), 5000);
    check(!(m.clip & 1u) && m.meter[2] == 12, "15 clears the clip light; a lower level waits for the fall");
    m.meter_t = 5000;
    mcu_tick(&m, 5050);
    check(m.meter[2] == 12, "the fall: not before 100 ms");
    mcu_tick(&m, 5100);
    mcu_tick(&m, 5200);
    check(m.meter[2] == 10, "then a step every 100 ms");
    mcu_in(&m, PKT(0x0B, 0xB0, 0x4B, '0'), 5000);
    mcu_in(&m, PKT(0x0B, 0xB0, 0x4A, '9'), 5000);
    check(m.assign[0] == '0' && m.assign[1] == '9', "the bank: CC 75 the left digit, CC 74 the right (REAPER: 09)");

    /* the LCD: the splash, then names, in USB's 3-byte packets */
    lcd(&m, 0, "REAPER! Initializing... Please wait...", 112, 6000);
    check(!memcmp(m.lcd, "REAPER! Init", 12) && m.lcd[111] == ' ', "the LCD: the splash, 112 characters from 0");
    lcd(&m, 0, "Kick", 7, 6000);
    lcd(&m, 7, "Bass", 7, 6000);
    lcd(&m, 14, "  03  ", 7, 6000);
    lcd(&m, 49, "Vox lead", 7, 6000);
    check(mcu_name(&m, 0, b) && !strcmp(b, "Kick"), "a strip's name: its six characters, trimmed (Kick)");
    check(mcu_name(&m, 2, b) && !strcmp(b, "03"), "an unnamed track: REAPER's number, trimmed (03)");
    check(mcu_name(&m, 7, b) && !strcmp(b, "Vox le"), "a long name: six characters (Vox le)");
    lcd(&m, 21, "", 7, 6000);
    check(!mcu_name(&m, 3, b) && !b[0], "a blank strip (no track there): no name");
    {
        static const uint8_t meter_mode[9] = {0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, 0x01, 0x03, 0xF7};
        static const uint8_t other[12] = {0xF0, 0x00, 0x00, 0x66, 0x15, 0x12, 0x00, 'X', 'X', 'X', 'X', 0xF7};
        uint32_t g = m.gen;
        sysex(&m, meter_mode, 9, 6000);
        sysex(&m, other, 12, 6000);
        check(m.gen == g && mcu_name(&m, 0, b) && !strcmp(b, "Kick"),
              "SysEx for the meter modes, or an extender's LCD (0x15): left alone");
    }
    {
        static const uint8_t cut[10] = {0xF0, 0x00, 0x00, 0x66, 0x14, 0x12, 0x00, 'Z', 0x90, 'Q'};
        sysex(&m, cut, 10, 6000);
        check(m.lcd[0] == 'Z' && m.lcd[1] == 'i' && !m.sx_on, "a SysEx cut by a status byte: what came stays, the rest dropped");
    }

    /* the time display */
    mcu_in(&m, PKT(0x09, 0x90, MCU_BEATS, 127), 7000);
    tc(&m, "_12_3__050", 7000);
    check(mcu_time(&m, b) && !strcmp(b, "12.3.05"), "beats: bars.beats.ticks (REAPER's 3 2 - 3 groups) -> 12.3.05");
    tc(&m, "__1_1__000", 7000);
    check(mcu_time(&m, b) && !strcmp(b, "1.1.00"), "beats at the start: 1.1.00");
    mcu_in(&m, PKT(0x09, 0x90, MCU_BEATS, 0), 7000);
    tc(&m, "___0112345", 7000);
    check(mcu_time(&m, b) && !strcmp(b, "01:12.345"), "time: minutes, seconds, ms -> 01:12.345");
    tc(&m, "__10203004", 7000);
    check(mcu_time(&m, b) && !strcmp(b, "1:02:03.004"), "an hour on: 1:02:03.004");
    mcu_reset(&m);
    check(!mcu_time(&m, b) && !b[0], "nothing yet: no time");

    /* every packet REAPER could send, in any order, never runs out of the state */
    mcu_reset(&m);
    for (ok = 1, i = 0; i < 200000u; i++) {
        uint32_t x = i * 2654435761u;
        mcu_in(&m, x, i);
        ok &= m.sx_n <= 6u;
    }
    check(ok, "200,000 packets of noise: the state stays in bounds");
    printf(fails ? "smk_mcu: %d FAILED\n" : "smk_mcu: ok\n", fails);
    return fails != 0;
}
