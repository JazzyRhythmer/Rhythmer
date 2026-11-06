/* SPDX-License-Identifier: GPL-3.0-only */
#include "tb3po.h"
#include "six303_bridge.h"
static void six303_generate_impl(uint8_t *notes,uint8_t *flags,unsigned len,const uint8_t cfg[8],uint32_t *seed,int mutate,unsigned root,uint16_t mask){
 bpart_t b={0};b.len=(uint8_t)(len<1?1:len>64?64:len);tb3po_defaults(&b.gen,*seed);
 b.gen.density=cfg[0];b.gen.accent=cfg[1];b.gen.slide=cfg[2];b.gen.oct_range=cfg[3];
 b.gen.root=(uint8_t)root;b.gen.scale=cfg[5];b.gen.base_oct=cfg[6];b.gen.mutate_bars=cfg[7];
 for(unsigned i=0;i<b.len;i++){b.step[i].note=notes[i];b.step[i].flags=flags[i];}
 if(mask)tb3po_scale_mask(&b,seed,mask,mutate);else if(mutate)tb3po_mutate(&b,seed);else tb3po_generate(&b);
 for(unsigned i=0;i<b.len;i++){notes[i]=b.step[i].note;flags[i]=b.step[i].flags;}
}

void six303_generate(uint8_t *n,uint8_t *f,unsigned l,const uint8_t c[8],uint32_t *s,int m){six303_generate_impl(n,f,l,c,s,m,c[4],0);}
void six303_generate_scl(uint8_t *n,uint8_t *f,unsigned l,const uint8_t c[8],uint32_t *s,int m,unsigned root,uint16_t mask){six303_generate_impl(n,f,l,c,s,m,root,mask);}
