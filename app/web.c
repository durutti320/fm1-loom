/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (SPIDER, the LOOM-8 SMK-37 port) */
/* SPIDER in the browser (fm1-base web/emu/emu_web.c includes this with app.h APP_WEB, the SMK-37 build only): the
 * page's own exports. web_spider_boot: SPIDER's boot, its sound and its pads, run from the next frame (spider_splash.c
 * spd_web_*), as the keyboard plays it once the platform is up; the page calls it as it switches on. */
__attribute__((used, visibility("default"))) void web_spider_boot(void) { spd_web_start(); }
/* SPIDER's samples in the browser: the page builds a user bank (app/web/samples/loom_samples.js, for the SMK-37's
 * windows) and hands it over as an upload would leave it in the flash: web_bank_buf() the room for it (SMP_BANK_SIZE
 * bytes), web_bank(n) takes its first n (0: the bank emptied), each sector erased and written (the rest of the bank
 * erased), the bank read again. Returns the user samples it now holds (0: none, or a bank refused). */
static uint8_t web_bank_img[SMP_BANK_SIZE];
__attribute__((used, visibility("default"))) uint8_t *web_bank_buf(void) { return web_bank_img; }
__attribute__((used, visibility("default"))) int web_bank(uint32_t n)
{
    uint32_t v, off;
    if (n > SMP_BANK_SIZE)
        return -1;
    for (v = n; v < SMP_BANK_SIZE; v++)
        web_bank_img[v] = 0xFF;
    smp_user_drop();                                 /* (nothing reads the bank meanwhile) */
    for (v = 0; v < SMP_BANK_SIZE; v += 0x1000u) {
        if (smp_reg_of(v, &off) < 0)
            return -1;
        plat_data_erase(off);
        if (v < n)
            plat_data_write(off, web_bank_img + v, 0x1000u);
    }
    smp_user_scan();
    return smp_user_slots();
}
