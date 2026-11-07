/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita, Hugelton Instruments
 * Drum machine fork: 2026 DEADACTIVE
 * Hortator MD state and kernels: 2026 DEADACTIVE.
 * Pinned source: 44c9ad4aa57bebf63f412bd552e57143c30b4065.
 * Rhythmer: union-local common scratch, fixed hit pool and macro adapters. */
typedef struct {int32_t a1,a2,a3,k;} dsvf_t;
typedef struct {                        /* KTRX (dm_ktrx.c) */
    uint32_t inc0, inc_d, inc_f, inc_r, ph_h;
    int32_t e_d, e_t, e_a, e_s, e_h, e_n;
    int32_t punch, sub, th, cgain, nlev;
    uint32_t k_n, d_amp, d_ramp, e_amp, e_r;
    uint8_t first;
} ktrx_t;
typedef struct {                        /* SDTRX (dm_sdtrx.c) */
    int32_t ga, gb, bk, g, nl, xl, x1, x2, hy, hx;
    uint32_t e_amp, d_amp, bu, du, be, dbe, drive, e_n, e_x;
} sdtrx_t;
typedef struct {                        /* B2TRX (dm_b2trx.c) */
    uint32_t e_r, d_r, e_amp, d_amp, hold, e_n;
    int32_t tick, nl, qb, g8, hk, lp, z1, z2;
} b2trx_t;
typedef struct {                        /* XTTRX, XCTRX (dm_xttrx.c) */
    uint32_t inc0, inc_r, e, d_e, e_r, d_r, sag, sl;
    int32_t th, cl, cz, px, hp1, hp1p, hp2;
    uint8_t mc;
} xttrx_t;
typedef struct {                        /* CLTRX (dm_cltrx.c) */
    uint32_t e, d_e, sx;
    int32_t ax, ay, ce;
} cltrx_t;
typedef struct {                        /* RSTRX (dm_rstrx.c) */
    uint32_t e, d_e, tj, d0, kk, mm;
    int32_t amp, g;
} rstrx_t;
typedef struct { int32_t b0, a1, a2; } tnbq_t;          /* CPTRX, MATRX (dm_trxn.c): a 2-pole section, Q30 */
typedef struct {                        /* CPTRX (dm_trxn.c) */
    tnbq_t hp, lp, rq;
    int32_t s[8], r[6];
    uint32_t e, d, ac, a4, re, rd;
    uint32_t on[5], cend, rend;
    uint8_t nb;
} cptrx_t;
typedef struct {                        /* MATRX (dm_trxn.c) */
    tnbq_t hp;
    int32_t s[6], lp, g, rk, gq, et, gv;
    uint32_t poff, ia, idv, dv;
    uint32_t t1, t2, t2v, t3, tend;
} matrx_t;
typedef struct { int32_t a1, a2, g, k, s1, s2; } trxmf_t;   /* the TRX metal ports' SVF (dm_trxm.c): a1 a2 k Q30, g Q28 */
typedef struct {                        /* CHTRX OHTRX CYTRX (dm_trxm.c) */
    uint32_t ph[6], inc[6];
    int32_t w[7], wsum, gb, gn;
    trxmf_t hp, lp, pk, tp;
    int32_t pkg, e, et, ek, ds, eend;
    uint32_t d, dt, nh;
    uint8_t ish, osh, top;
} trxm_t;
typedef struct { int32_t pn[7], hx, hy, hl, el; } trxcbp_t;   /* CBTRX: its pink noise, the floor, the grit */
typedef struct {                        /* CBTRX (dm_trxcb.c) */
    uint32_t pb, pa, ib, ia, e, e0, ea, eg, eb, d, nh;
    int32_t gb, ga, gae, m, erk, bump, glvl;
    trxcbp_t pk;
    trxmf_t db, da, enh;
} trxcb_t;
typedef struct {                        /* BDEFM, XTEFM (dm_bdefm.c) */
    uint32_t inc0, e_a, d_a, e_r, d_r, e_i, d_i;
    int32_t rk, i0, mf, fb, m, x1, x2, y1, y2, q1, q2, hx, hy;
    uint8_t xt;
} bdefm_t;
typedef struct { int32_t x1, x2, y1, y2, q1, q2; } efmn_hp2_t;   /* the EFM noise voices' track filter (dm_efmn.c) */
typedef struct {                        /* SDEFM, CPEFM's noise-FM carrier (dm_efmn.c) */
    uint32_t pc, pm, inc, incm, dev, ei, di;
    int32_t fz, nz;
    uint8_t hc, hold;
} efmn_fm_t;
typedef struct {                        /* SDEFM, CPEFM (dm_efmn.c) */
    efmn_fm_t fm;
    efmn_hp2_t hp;
    uint32_t ea, da, en, dn, ec, dc, tc, per;
    int32_t nlp, nhp, nxp, hh, hx, hk;
    uint8_t nb;
} efmn_t;
typedef struct {                        /* RSEFM (dm_efmn.c) */
    efmn_hp2_t hp;
    uint32_t pc[2], inc[2], dev[2], e[2], d[2];
    int32_t n[2], z[2], g, hs;
} efmrs_t;
typedef struct {                        /* CBEFM (dm_efmm.c) */
    uint32_t pc[2], pm[2], inc[2], minc[2], ea, es, ei, da, ds, di;
    int32_t y[2], bq, sn, sk, lvl;
} cbefm_t;
typedef struct {                        /* HHEFM (dm_efmm.c) */
    uint32_t pc, pm, inc, minc, ea, ei, da, di, tp, tinc;
    int32_t bq, dep, h, up, l, g, gs, lvl;
} hhefm_t;
typedef struct {                        /* CYEFM (dm_efmm.c) */
    uint32_t pc[4], pm[4], wc[4], wm[4], ea, da, ih, di;
    int32_t yp[4], bq, gk, s1, s2, lvl;
} cyefm_t;

typedef union {
 struct {ktrx_t s; uint32_t ph[2]; int32_t env[3]; uint32_t k[3]; int32_t f[2]; dsvf_t c[1];} kt;
 struct {sdtrx_t s; uint32_t ph[3]; uint32_t inc[3]; int32_t f[4];} sd;
 struct {xttrx_t s; uint32_t ph[1];} xt;
 struct {rstrx_t s; uint32_t ph[1]; uint32_t inc[1];} rs;
 struct {cltrx_t s; uint32_t ph[3]; uint32_t inc[3];} cl;
 struct {b2trx_t s; uint32_t ph[1]; uint32_t inc[2];} b2;
 struct {bdefm_t s; uint32_t ph[2];} bdefm;
 struct {cptrx_t s; } cp;
 struct {matrx_t s; } ma;
 struct {trxm_t s; } tm;
 struct {trxcb_t s; } cb;
 struct {efmn_t s; } en;
 struct {efmrs_t s; } er;
 struct {cbefm_t s; } cbe;
 struct {hhefm_t s; } hhe;
 struct {cyefm_t s; } cye;
} md_state_t;
