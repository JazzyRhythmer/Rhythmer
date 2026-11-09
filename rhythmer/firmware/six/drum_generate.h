/* SPDX-License-Identifier: GPL-3.0-only
 * Rhythmer one-shot drum generators, adapted from dmajka/elixir
 * d35a1cc45248e2c3d68014aaa1d3a34d6787a6f4, firmware/src/seq.c.
 * LXR sequencer: Copyright (C) 2012-2026 Erica Synths.
 * Grids map/interpolation: Copyright (C) 2011-2012 Emilie Gillet,
 * GPL-3.0-or-later, used here under GPL-3.0-only.
 * These functions run on an explicit UI action, never in the audio callback.
 * They materialize a normal editable sequence, not Elixir's live gpat engine.
 */
#pragma once
#include <stdint.h>
#include "drum_grids_map.h"
enum { DG_EUCLID, DG_MUTATE, DG_TURING, DG_CELLULAR, DG_GRIDS, DG_COUNT };
typedef struct {
    uint8_t type, lane, density, variation;
    uint8_t x, y, accent, seed;
    uint8_t hits2, rotate2, join, reglen;
} drum_gen_t;
typedef struct { uint64_t hits, accents; } drum_gen_row_t;
static uint32_t dg_random(uint32_t *s) {
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5; return *s;
}
static uint64_t dg_mask(unsigned len) { return len >= 64 ? ~(uint64_t)0 : ((uint64_t)1 << len) - 1; }
static uint64_t dg_euclid(unsigned len, unsigned hits, unsigned rotate) {
    uint64_t bits = 0;
    if (!len || len > 64) return 0;
    if (hits > len) hits = len;
    for (unsigned k = 0; k < len; k++) if (k * hits % len < hits) bits |= (uint64_t)1 << k;
    rotate %= len;
    if (rotate) bits = ((bits << rotate) | (bits >> (len - rotate))) & dg_mask(len);
    return bits;
}
static unsigned dg_mix(unsigned a, unsigned b, unsigned x) { return (a * (255 - x) + b * x) >> 8; }
static unsigned dg_grid(unsigned part, unsigned step, unsigned x, unsigned y) {
    unsigned i=x>>6,j=y>>6,o=part*32+(step&31);
    return dg_mix(dg_mix(GRIDS_NODE[GRIDS_AT[i][j]][o],GRIDS_NODE[GRIDS_AT[i+1][j]][o],(x<<2)&255),
                  dg_mix(GRIDS_NODE[GRIDS_AT[i][j+1]][o],GRIDS_NODE[GRIDS_AT[i+1][j+1]][o],(x<<2)&255),(y<<2)&255);
}
/* A Rhythmer step is a sixteenth. Elixir/Grids has two map ticks per step;
 * preserve its second tick as a FLAM when both fire, or as a late hit below.
 * late/flam are optional output masks, not persistent generator state. */
static drum_gen_row_t drum_generate(const drum_gen_t *g, unsigned len, uint64_t before,
                                    uint64_t before_acc, uint64_t *late, uint64_t *flam) {
    drum_gen_row_t r={0,0};
    uint32_t rng=0x2545f491u ^ ((uint32_t)g->seed+1)*0x9e3779b9u ^ ((uint32_t)g->lane+1)*977u;
    unsigned d=g->density>127?127:g->density, v=g->variation;
    if (late) *late=0;
    if (flam) *flam=0;
    if (!len || len>64 || g->type>=DG_COUNT) return r;
    before &= dg_mask(len); before_acc &= before;
    if (g->type==DG_EUCLID) {
        r.hits=dg_euclid(len,(d*len+63)/127,v);
        if(g->hits2) {
            uint64_t b=dg_euclid(len,g->hits2,g->rotate2);
            r.hits=g->join==1?r.hits^b:g->join==2?r.hits&b:g->join==3?r.hits&~b:r.hits|b;
        }
    } else if (g->type==DG_MUTATE) {
        r.hits=before; r.accents=before_acc;
        for(unsigned k=0;k<2;k++) {
            unsigned s=dg_random(&rng)%len,op=dg_random(&rng)%3;
            if(dg_random(&rng)%128 >= d) continue;
            uint64_t b=(uint64_t)1<<s;
            if(!op) {r.hits^=b;r.accents&=r.hits;}
            else {
                for(unsigned j=0;j<len&&!(r.hits&b);j++){s=(s+1)%len;b=(uint64_t)1<<s;}
                if(!(r.hits&b))continue;
                if(op==1) {
                    unsigned to=(s+(dg_random(&rng)&1?1:len-1))%len;
                    uint64_t nb=(uint64_t)1<<to;
                    if(r.hits&nb)continue;
                    r.hits=(r.hits&~b)|nb;
                    r.accents=(r.accents&~b)|((r.accents&b)?nb:0);
                } else r.accents^=b;
            }
        }
    } else if (g->type==DG_TURING) {
        unsigned n=g->reglen<2?2:g->reglen>16?16:g->reglen,mask=(1u<<n)-1;
        unsigned reg=(unsigned)before&mask,lock=v>127?127:v;
        if(!reg)reg=(dg_random(&rng)|1)&mask;
        for(unsigned i=0;i<len;i++) {
            unsigned bit=reg>>(n-1)&1;
            if(dg_random(&rng)%254<127-lock)bit^=1;
            reg=((reg<<1)|bit)&mask;
            unsigned level=reg*127/mask;
            if(level<d){r.hits|=(uint64_t)1<<i;if(level>d*3/4)r.accents|=(uint64_t)1<<i;}
        }
    } else if(g->type==DG_CELLULAR) {
        if(!before)before=(uint64_t)1<<(len/2);
        for(unsigned k=0;k<len;k++) {
            unsigned l=(unsigned)(before>>((k+len-1)%len))&1,c=(unsigned)(before>>k)&1;
            unsigned rr=(unsigned)(before>>((k+1)%len))&1;
            if((v>>(l*4+c*2+rr))&1)r.hits|=(uint64_t)1<<k;
        }
        if(!r.hits)r.hits=before; /* Elixir's non-empty restart */
    } else {
        unsigned x=(g->x&127)*2+(g->x>>6), y=(g->y&127)*2+(g->y>>6);
        unsigned part=(g->lane==1||g->lane==2)?1:(g->lane==3||g->lane==4)?2:0;
        unsigned threshold=255-(d*2+(d>>6)),pert=0;
        for(unsigned i=0;i<len;i++) {
            if(!(i&15))pert=(dg_random(&rng)%256*((v>127?127:v)/2))>>8;
            unsigned a=dg_grid(part,i*2,x,y),b=dg_grid(part,i*2+1,x,y);
            a=a+pert>255?255:a+pert;b=b+pert>255?255:b+pert;
            int ha=a>threshold&&a,hb=b>threshold&&b;
            uint64_t bit=(uint64_t)1<<i;
            if(ha||hb)r.hits|=bit;
            if((ha?a:b)>192)r.accents|=bit;
            if(hb&&!ha&&late)*late|=bit;
            if(ha&&hb&&flam)*flam|=bit;
        }
    }
    if(g->type==DG_EUCLID||g->type==DG_CELLULAR)
        for(unsigned i=0;i<len;i++)if((r.hits>>i&1)&&!(i&3))r.accents|=(uint64_t)1<<i;
    r.hits&=dg_mask(len);r.accents=g->accent?r.accents&r.hits:0;
    return r;
}
