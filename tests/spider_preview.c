/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (SPIDER, the LOOM-8 SMK-37 port) */
/* SPIDER's boot (app/spider_splash.c) on a PC, for a look and a listen before a flash: the frames as PPM every STEP ms,
 * the 16 pads drawn under the screen (two rows of eight, as on the keyboard), and the sound as a WAV (boot.wav, 44.1
 * kHz stereo, at the volume given: 4096 the MASTER knob full, 1024 its middle, where the SMK starts). After a build
 * (its fonts: app/build/gen):
 *   cc -O2 -Iapp/build/gen -o spider_preview tests/spider_preview.c -lm && ./spider_preview OUTDIR [STEP_MS] [GAIN] */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define APP_CANVAS_PX (240u * 120u)
#define APP_NAME "SPIDER"
#define SPD_PREVIEW 1
#define PH 312                                   /* the picture: the screen, then the pads */
static uint16_t fb[240 * 240];
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)
{
    for (uint32_t j = 0; j < h && y + j < 240u; j++)
        for (uint32_t i = 0; i < w && x + i < 240u; i++)
            fb[(y + j) * 240u + x + i] = (uint16_t)(px[j * w + i] >> 8 | px[j * w + i] << 8);
}
#include "../base/device/gfx.c"
#include "../app/gfx_loom.c"
#include "../app/spider_splash.c"

static void put16(FILE *f, uint32_t v) { fputc((int)(v & 255u), f); fputc((int)(v >> 8 & 255u), f); }
static void put32(FILE *f, uint32_t v) { put16(f, v & 0xFFFFu); put16(f, v >> 16); }

static int wav(const char *dir, uint32_t gain)
{
    char p[512];
    static int32_t buf[512];
    uint32_t n = (SPD_SND_MS + 400u) * 441u / 10u / 256u * 256u, i, k;
    double peak = 0.0, sum = 0.0;
    FILE *f;
    snprintf(p, sizeof p, "%s/boot.wav", dir);
    if (!(f = fopen(p, "wb")))
        return 1;
    fwrite("RIFF", 1, 4, f);
    put32(f, 36u + n * 4u);
    fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16u);
    put16(f, 1u);
    put16(f, 2u);
    put32(f, 44100u);
    put32(f, 44100u * 4u);
    put16(f, 4u);
    put16(f, 16u);
    fwrite("data", 1, 4, f);
    put32(f, n * 4u);
    spd_snd_start();
    for (k = 0; k < n; k += 256u) {
        memset(buf, 0, sizeof buf);
        spd_snd_mix(buf, 256u, gain);
        for (i = 0; i < 512u; i++) {             /* (the codec's full scale: 2^23; the render's ceiling 2^22) */
            double v = (double)buf[i] / 8388608.0;
            int32_t s = (int32_t)(v * 32767.0);
            peak = v > peak ? v : -v > peak ? -v : peak;
            sum += v * v;
            put16(f, (uint32_t)(uint16_t)(int16_t)(s > 32767 ? 32767 : s < -32768 ? -32768 : s));
        }
    }
    fclose(f);
    printf("boot.wav: %.2f s, peak %.1f dBFS, rms %.1f dBFS (gain %u / 4096)\n", (double)n / 44100.0,
           20.0 * __builtin_log10(peak > 1e-9 ? peak : 1e-9), 10.0 * __builtin_log10(sum / (2.0 * n) > 1e-18 ? sum / (2.0 * n) : 1e-18), gain);
    return 0;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    uint32_t step = argc > 2 ? (uint32_t)atoi(argv[2]) : 40u, gain = argc > 3 ? (uint32_t)atoi(argv[3]) : 1024u, t, n = 0;
    for (int i = 0; i < 3; i++)
        printf("credit %d: %d px\n", i, (int)loom_text_w(&LFONT_XS, SPD_CREDIT[i]));
    for (t = 0; t <= SPD_SND_MS; t += step, n++) {   /* (the last picture held while the sound dies away) */
        char p[512];
        uint32_t pads[16];
        FILE *f;
        spd_frame(t > SPD_MS ? SPD_MS : t);
        spd_pads(t > SPD_MS ? SPD_MS : t, pads, 0);
        snprintf(p, sizeof p, "%s/f%04u.ppm", dir, n);
        f = fopen(p, "wb");
        if (!f)
            return 1;
        fprintf(f, "P6 240 %d 255\n", PH);
        for (uint32_t i = 0; i < 240u * 240u; i++) {
            uint16_t c = fb[i];
            uint8_t rgb[3] = {(uint8_t)((c >> 11) * 255 / 31), (uint8_t)((c >> 5 & 63u) * 255 / 63), (uint8_t)((c & 31u) * 255 / 31)};
            fwrite(rgb, 1, 3, f);
        }
        for (int y = 240; y < PH; y++)           /* the pads: 8 x 2, 24 px with 5 px between, on the case's black */
            for (int x = 0; x < 240; x++) {
                int col = (x - 5) / 29, row = (y - 248) / 30, px = (x - 5) % 29, py = (y - 248) % 30;
                uint32_t c = 0x141018u;
                if (x >= 5 && col < 8 && y >= 248 && row < 2 && px < 24 && py < 24) {
                    uint32_t q = pads[row * 8 + col], k;   /* (no darker than an unlit pad's grey) */
                    for (c = 0, k = 0; k < 24u; k += 8u)
                        c |= ((q >> k & 255u) > 0x1Au ? (q >> k & 255u) : 0x1Au) << k;
                }
                uint8_t rgb[3] = {(uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c};
                fwrite(rgb, 1, 3, f);
            }
        fclose(f);
    }
    printf("%u frames\n", n);
    return wav(dir, gain);
}
