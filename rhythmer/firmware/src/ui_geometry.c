/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Rhythmer: integer curve, line and dot primitives (adapted from the credited sources). */
/* Everything is drawn into the graph canvas (gfx.c cv_px) with integer maths: no stored artwork. Positions in
 * 1/16 px (Q4) so curves move smoothly between pixels; lines antialiased (two pixels across, blended over what
 * is there). Angles are 16-bit turns (0x10000 = 360 degrees), through dsp.c's sine table.
 *   og_line     an antialiased line (Q4 ends)
 *   og_quad, og_cubic   Bezier curves, in short lines
 *   og_path     a closed outline of cubic segments from a small table of points (int8), morphed between two such
 *               tables (A and B: the same shape count), scaled about an anchor (inner copies: the plates' vessels)
 *               and mirrored (one table draws both sides of a symmetric organ)
 *   og_ring, og_node    a circle; a dot with a soft rim
 *   og_dash     a dashed line (the plates' axes and leaders); og_label a letter in a circle (their references)
 *   og_stipple  dots scattered in an ellipse (their stippled shading), the same dots every frame
 *   og_col      the five JIANT colours: cream, coral, teal, mustard, mint (on another palette: blends of its own)
 * The look is the anatomical wall chart (1960s, white line on black): thin cream outlines, colour only in details
 * (red tips, yellow and cyan vessels), cyan reference letters, dashed axes. */
#define OG_Q 16                                          /* Q4 */
enum { OG_CREAM, OG_CORAL, OG_TEAL, OG_MUSTARD, OG_MINT };

static int og_neutral(uint16_t c) { return (c >> 11) == (c & 31u) && ((c >> 5) & 63u) >> 1 == (c >> 11); }
static uint16_t og_col(uint32_t k) { return k == OG_TEAL ? T_THEME : k == OG_MINT ? T_MID : T_ACCENT; }
/* c over the canvas pixel at (x, y) with coverage a (0..256) */
static inline void og_plot(int32_t x, int32_t y, uint16_t c, int32_t a)
{
    uint16_t *p, o;
    uint32_t r, g, b;
    y += cv_oy;
    if (a <= 0 || (uint32_t)x >= cv_w || (uint32_t)y >= cv_h)
        return;
    p = &cv_px[(uint32_t)y * cv_w + (uint32_t)x];
    if (a >= 256) {
        *p = swap16(c);
        return;
    }
    o = swap16(*p);
    r = (o >> 11) + ((((int32_t)(c >> 11) - (int32_t)(o >> 11)) * a) >> 8);
    if (og_neutral(o) && og_neutral(c)) {                /* grey over grey stays grey */
        *p = swap16((uint16_t)(r << 11 | r << 6 | r));
        return;
    }
    g = ((o >> 5) & 63u) + ((((int32_t)((c >> 5) & 63u) - (int32_t)((o >> 5) & 63u)) * a) >> 8);
    b = (o & 31u) + ((((int32_t)(c & 31u) - (int32_t)(o & 31u)) * a) >> 8);
    *p = swap16((uint16_t)(r << 11 | g << 5 | b));
}
/* an antialiased line between Q4 points, at strength a (0..256) */
static void og_line_a(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c, int32_t a)
{
    int32_t dx = x1 - x0, dy = y1 - y0, adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy, t, g, p, e, f;
    if (adx >= ady) {                                    /* along x: y in Q8, two pixels per column */
        if (x0 > x1) { t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
        g = adx ? ((y1 - y0) << 8) / (x1 - x0) : 0;      /* dy / dx, Q8 */
        e = x1 >> 4;
        for (p = x0 >> 4; p <= e && p - (x0 >> 4) < 480; p++) {
            int32_t y = (y0 << 4) + g * (p - (x0 >> 4)) - 128;   /* (pixel centres) */
            f = y & 255;
            og_plot(p, y >> 8, c, ((256 - f) * a) >> 8);
            og_plot(p, (y >> 8) + 1, c, (f * a) >> 8);
        }
    } else {
        if (y0 > y1) { t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
        g = ((x1 - x0) << 8) / (y1 - y0);
        e = y1 >> 4;
        for (p = y0 >> 4; p <= e && p - (y0 >> 4) < 480; p++) {
            int32_t x = (x0 << 4) + g * (p - (y0 >> 4)) - 128;
            f = x & 255;
            og_plot(x >> 8, p, c, ((256 - f) * a) >> 8);
            og_plot((x >> 8) + 1, p, c, (f * a) >> 8);
        }
    }
}
static void og_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c) { og_line_a(x0, y0, x1, y1, c, 256); }

/* a quadratic Bezier (Q4 points) in n short lines */
static void og_quad_a(int32_t x0, int32_t y0, int32_t cx, int32_t cy, int32_t x1, int32_t y1, uint16_t c, uint32_t n,
                      int32_t a)
{
    int32_t px = x0, py = y0;
    uint32_t i;
    for (i = 1; i <= n; i++) {
        int32_t t = (int32_t)((i << 8) / n), u = 256 - t;   /* Q8 */
        int32_t x = (u * u * x0 + 2 * u * t * cx + t * t * x1) >> 16, y = (u * u * y0 + 2 * u * t * cy + t * t * y1) >> 16;
        og_line_a(px, py, x, y, c, a);
        px = x;
        py = y;
    }
}
static void og_quad(int32_t x0, int32_t y0, int32_t cx, int32_t cy, int32_t x1, int32_t y1, uint16_t c, uint32_t n)
{
    og_quad_a(x0, y0, cx, cy, x1, y1, c, n, 256);
}

/* cos / sin of a 16-bit angle, Q15 */
static int32_t og_cos(uint32_t ang) { return sine_i((ang + 0x4000u) << 16); }
static int32_t og_sin(uint32_t ang) { return sine_i(ang << 16); }

static void og_cubic_a(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3,
                       uint16_t c, uint32_t n, int32_t a)
{
    int32_t px = x0, py = y0;
    uint32_t i;
    for (i = 1; i <= n; i++) {                           /* t in Q6: products stay in 32 bits */
        int32_t t = (int32_t)((i << 6) / n), u = 64 - t, uu = u * u, tt = t * t;
        int32_t x = (uu * u * x0 + 3 * uu * t * x1 + 3 * u * tt * x2 + tt * t * x3) >> 18;
        int32_t y = (uu * u * y0 + 3 * uu * t * y1 + 3 * u * tt * y2 + tt * t * y3) >> 18;
        og_line_a(px, py, x, y, c, a);
        px = x;
        py = y;
    }
}

/* A shape: n cubic segments, a start point then three points a segment, x y as int8 (units, y down), closed by its
 * last point = its start. og_path draws it morphed m (0 A .. 256 B), each unit u / 16 px (Q4 per unit: u), about
 * the anchor (ax, ay) (units) scaled by k (Q8: 256 the shape itself) and mirrored (mx -1) at (cx, cy) (Q4) */
typedef struct {
    int16_t cx, cy, u, mx;                               /* where, units -> Q4, -1 mirrors */
    int16_t ax, ay, k;                                   /* scaled about (ax, ay) by k / 256 */
} og_xf_t;
static void og_pt(const og_xf_t *f, const int8_t *a, const int8_t *b, int32_t m, uint32_t i, int32_t *x, int32_t *y)
{
    int32_t px = a[2 * i] * 256 + (b[2 * i] - a[2 * i]) * m, py = a[2 * i + 1] * 256 + (b[2 * i + 1] - a[2 * i + 1]) * m;
    px = f->ax * 256 + (((px - f->ax * 256) >> 4) * f->k >> 4);   /* (units Q8) */
    py = f->ay * 256 + (((py - f->ay * 256) >> 4) * f->k >> 4);
    *x = f->cx + f->mx * ((px * f->u) >> 8);
    *y = f->cy + ((py * f->u) >> 8);
}
static void og_path(const og_xf_t *f, const int8_t *a, const int8_t *b, uint32_t nseg, int32_t m, uint16_t c, int32_t al)
{
    uint32_t s;
    for (s = 0; s < nseg; s++) {
        int32_t x0, y0, x1, y1, x2, y2, x3, y3;
        og_pt(f, a, b, m, 3 * s, &x0, &y0);
        og_pt(f, a, b, m, 3 * s + 1, &x1, &y1);
        og_pt(f, a, b, m, 3 * s + 2, &x2, &y2);
        og_pt(f, a, b, m, 3 * s + 3, &x3, &y3);
        og_cubic_a(x0, y0, x1, y1, x2, y2, x3, y3, c, 10, al);
    }
}
/* a dashed line (Q4 ends): on / off in px */
static void og_dash(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c, int32_t on, int32_t off)
{
    int32_t dx = x1 - x0, dy = y1 - y0, len = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
    int32_t p, step = (on + off) * OG_Q;
    for (p = 0; p < len && len; p += step) {
        int32_t q = p + on * OG_Q < len ? p + on * OG_Q : len;
        og_line(x0 + dx * p / len, y0 + dy * p / len, x0 + dx * q / len, y0 + dy * q / len, c);
    }
}
/* n dots inside the ellipse (cx, cy) rx ry (Q4), from seed: the same dots every frame */
static void og_stipple(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint32_t n, uint32_t seed, uint16_t c)
{
    uint32_t i, h = seed | 1u;
    for (i = 0; i < n; i++) {
        int32_t x, y;
        h ^= h << 13; h ^= h >> 17; h ^= h << 5;
        x = (int32_t)(h & 255u) - 128;
        y = (int32_t)((h >> 8) & 255u) - 128;
        if (x * x + y * y > 128 * 128)
            continue;
        og_plot((cx + x * rx / 128) >> 4, (cy + y * ry / 128) >> 4, c, 160 + (int32_t)((h >> 16) & 95u));
    }
}

/* a circle of radius r (Q4) in 20 lines */
static void og_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t px = cx + r, py = cy;
    uint32_t i;
    for (i = 1; i <= 20u; i++) {
        uint32_t ang = i * (1u << 16) / 20u;
        int32_t x = cx + ((og_cos(ang) * r) >> 15), y = cy + ((og_sin(ang) * r) >> 15);
        og_line(px, py, x, y, c);
        px = x;
        py = y;
    }
}
/* a filled dot of radius r px (pixel centre (cx, cy) in Q4) with an antialiased rim */
static void og_node(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t x0 = cx >> 4, y0 = cy >> 4, i, j;
    for (j = -r; j <= r; j++)
        for (i = -r; i <= r; i++) {
            int32_t d = i * i + j * j - r * r;          /* inside: d <= 0; the rim: 0 .. 2r */
            if (d <= -r)
                og_plot(x0 + i, y0 + j, c, 256);
            else if (d < r)
                og_plot(x0 + i, y0 + j, c, 128 - d * 128 / r);
        }
}
/* a reference letter in a circle (the plates' cyan A B C): centre (cx, cy) in px */
static void og_label(int32_t cx, int32_t cy, const char *s, uint16_t c)
{
    og_ring(cx * OG_Q, cy * OG_Q, 7 * OG_Q, c);
    cv_text_in(cx - 7, cy - 7 + CAP_IN(S, 15), 15, &AF_S, s, c, cv_bg);
}

