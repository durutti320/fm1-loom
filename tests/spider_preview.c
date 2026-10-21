/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (SPIDER, the LOOM-8 SMK-37 port) */
/* SPIDER's boot animation (app/spider_splash.c) drawn on a PC: frames as PPM, every STEP ms, for a look before a flash.
 * After a build (its fonts: app/build/gen):
 *   cc -O2 -Iapp/build/gen -o spider_preview tests/spider_preview.c -lm && ./spider_preview OUTDIR [STEP_MS] */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define APP_CANVAS_PX (240u * 120u)
#define APP_NAME "SPIDER"
#define SPD_PREVIEW 1
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

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    uint32_t step = argc > 2 ? (uint32_t)atoi(argv[2]) : 40u, t, n = 0;
    for (t = 0; t <= SPD_MS + 1000u; t += step, n++) {   /* (a second held at the end, as the boot goes on) */
        char p[512];
        FILE *f;
        spd_frame(t > SPD_MS ? SPD_MS : t);
        snprintf(p, sizeof p, "%s/f%04u.ppm", dir, n);
        f = fopen(p, "wb");
        if (!f)
            return 1;
        fprintf(f, "P6 240 240 255\n");
        for (uint32_t i = 0; i < 240u * 240u; i++) {
            uint16_t c = fb[i];
            uint8_t rgb[3] = {(uint8_t)((c >> 11) * 255 / 31), (uint8_t)((c >> 5 & 63u) * 255 / 63), (uint8_t)((c & 31u) * 255 / 31)};
            fwrite(rgb, 1, 3, f);
        }
        fclose(f);
    }
    printf("%u frames\n", n);
    return 0;
}
