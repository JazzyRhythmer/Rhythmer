/* SPDX-License-Identifier: GPL-3.0-only */
#include "tb3po.h"
#include "acid_generate.h"
void acid_generate(uint8_t *notes,uint8_t *flags,unsigned len,const uint8_t cfg[5],uint32_t *seed,int mutate,unsigned root,uint16_t mask){
 acid_gen_line_t line={0};line.len=(uint8_t)(len<1?1:len>64?64:len);
 line.gen.density=cfg[0];line.gen.accent=cfg[1];line.gen.slide=cfg[2];line.gen.oct_range=cfg[3];
 line.gen.root=(uint8_t)(root%12u);line.gen.base_oct=cfg[4];line.gen.seed=*seed?*seed:0xBEEF;
 for(unsigned i=0;i<line.len;i++){line.step[i].note=notes[i];line.step[i].flags=flags[i];}
 tb3po_scale_mask(&line,seed,mask?mask:0x0FFFu,mutate);
 for(unsigned i=0;i<line.len;i++){notes[i]=line.step[i].note;flags[i]=line.step[i].flags;}
}
