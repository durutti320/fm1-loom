/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Cam Durnsford (SPIDER, the LOOM-8 SMK-37 port) */
/* SPIDER in the browser (fm1-base web/emu/emu_web.c includes this with app.h APP_WEB, the SMK-37 build only): the
 * page's own exports. web_spider_boot: SPIDER's boot, its sound and its pads, run from the next frame (spider_splash.c
 * spd_web_*), as the keyboard plays it once the platform is up; the page calls it as it switches on. */
__attribute__((used, visibility("default"))) void web_spider_boot(void) { spd_web_start(); }
