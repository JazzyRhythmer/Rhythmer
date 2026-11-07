/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita and Juan Ignacio Antonucci; adapted for Rhythmer. */
static uint16_t bg_col(int32_t v) { return heat_col(v); }
static int32_t bg_sin(uint32_t a) { return sine_i(a << 16); }
static int32_t bg_energy(void)
{
    static int32_t e;
    static uint32_t fr = 0xFFFFFFFFu;
    uint32_t w = scope_w, i;
    int32_t pk = 0, v;
    if (fr == ui.frame)                                 /* (once a frame: a redraw in the same frame draws the same) */
        return e;
    fr = ui.frame;
    for (i = 1; i <= 256u; i += 2u) {
        v = scope_buf[(w - i) & (SCOPE_N - 1u)];
        if (v < 0) v = -v;
        if (v > pk) pk = v;
    }
    v = pk * 256 / 12000;
    if (v > 256) v = 256;
    e += v > e ? (v - e) * 3 / 4 : (v - e) / 6;
    return e;
}

static void lz_arc(int32_t cx, int32_t cy, int32_t r, int32_t w, int32_t f, uint16_t tc, uint16_t vc)
{
    int32_t k, i;
    for (k = 0; k < w; k++) {
        int32_t rr = (r - k) * 16, px = 0, py = 0;
        for (i = 0; i <= 18; i++) {                     /* (18 segments over 270 degrees) */
            uint32_t a = 0x6000u + (uint32_t)i * 0xC000u / 18u;   /* (from 135 degrees, y down) */
            int32_t x = cx * 16 + (og_cos(a) * rr >> 15), y = cy * 16 + (og_sin(a) * rr >> 15);
            if (i)
                og_line(px, py, x, y, i * 1000 <= f * 18 ? vc : tc);
            px = x;
            py = y;
        }
    }
}
