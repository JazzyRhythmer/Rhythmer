/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Jazzy Rhythmer
 * UI adapter for Elixir's drum generators; source notices in drum_generate.h.
 * No new audio voice, background generator, clock, engine ID or save format.
 */
#include "../six/drum_generate.h"
static const char *const DG_NAMES[]={"EUCLID","MUTATE","TURING","CELLULAR","GRIDS"};
static drum_gen_t dg_cfg[NTRK];
static uint8_t dg_seen;
static drum_gen_t *dg_current(void) {
    unsigned k=song.sel%NTRK;
    if(!(dg_seen&(1u<<k))) {
        dg_cfg[k]=(drum_gen_t){DG_GRIDS,0,72,0,64,64,1,31,0,0,0,8};
        dg_seen|=(uint8_t)(1u<<k);
    }
    if(!drum_track(TSEL)&&dg_cfg[k].lane>3)dg_cfg[k].lane=0;
    return &dg_cfg[k];
}
static int drumgen_page(void) {
    return !ui.home&&!ui.menu&&!ui.confirm&&!ui.layer&&!ui.ly&&
           cur_page()->graph==GR_DGEN;
}
static void drumgen_knob(const page_t *pg,unsigned k,int32_t s) {
    drum_gen_t *g=dg_current();
    unsigned sub=pg->id[0];
    uint8_t *v=0;int max=127;
    if(sub==0) {
        if(k==0) {g->type=(uint8_t)clamp((int)g->type+(s>0?1:-1),0,DG_COUNT-1);
            g->variation=g->type==DG_CELLULAR?30:g->type==DG_TURING?96:0;return;}
        if(k==1){v=&g->lane;max=drum_track(TSEL)?7:3;}
        else if(k==2)v=&g->density;
        else {v=&g->variation;max=g->type==DG_CELLULAR?255:g->type==DG_EUCLID?63:127;}
    } else if(sub==1) {
        v=k==0?&g->x:k==1?&g->y:k==2?&g->accent:&g->seed;
        if(k==2)max=1;
    } else {
        v=k==0?&g->hits2:k==1?&g->rotate2:k==2?&g->join:&g->reglen;
        max=k<2?64:k==2?3:16;
    }
    int lo=sub==2&&k==3?2:0;
    *v=(uint8_t)clamp((int)*v+accel(EN_K1+k,s,max),lo,max);
}
/* Main-loop scratch shared across all tracks; generated MIDI is stored in the
 * existing step format. No additional per-track pattern or audio-state pool. */
static step_t tl_src[NSTEP] __attribute__((section(".pool")));
#define dg_src tl_src /* SEQ tools and GEN are mutually exclusive main-loop actions. */
static int seqgen_gate(const track_t *t,const step_t *s){
 if(s->time!=ST_NOTE||!s->n)return 0;
 if(trk_eng(t)!=ENGI_BREAK)return 1;
 for(unsigned j=0;j<s->n&&j<4;j++)if(s->note[j]<32)return 1;return 0;
}
static void seqgen_notes(track_t *t,const drum_gen_t *g,unsigned len,drum_gen_row_t r) {
    unsigned src[NSTEP],nsrc=0,next=0,first=1;int brk=trk_eng(t)==ENGI_BREAK;
    memcpy(dg_src,t->step,sizeof dg_src);
    for(unsigned i=0;i<len;i++)if(seqgen_gate(t,&dg_src[i]))src[nsrc++]=i;
    uint32_t rnd=0x9e3779b9u^((uint32_t)g->seed+1)*977u;
    for(unsigned i=0;i<len;i++) {
        step_t *st=&t->step[i];const step_t *old=&dg_src[i];
        unsigned hit=(unsigned)(r.hits>>i)&1,n=0;uint8_t notes[4]={0};
        if(g->type==DG_MUTATE&&hit==(unsigned)seqgen_gate(t,old)){
            if(hit)st->flags=(uint8_t)((st->flags&~SF_ACCENT)|((r.accents>>i&1)?SF_ACCENT:0));
            continue;
        }
        if(hit) {
            const step_t *from=nsrc?&dg_src[src[next++%nsrc]]:0;
            if(g->type==DG_MUTATE){
                for(unsigned dist=1;dist<=len;dist++){
                    unsigned j=(i+len-dist)%len;
                    if(seqgen_gate(t,&dg_src[j])&&!(r.hits>>j&1)){from=&dg_src[j];break;}
                }
            }
            if(brk) {
                unsigned command=(i/2)&15;
                if((g->lane==1||g->type==DG_MUTATE)&&from)for(unsigned j=0;j<from->n&&j<4;j++)if(from->note[j]<32){command=from->note[j];break;}
                notes[n++]=(uint8_t)command;
            }else if((g->lane==0||g->type==DG_MUTATE)&&from) {
                n=from->n>4?4:from->n;memcpy(notes,from->note,n);
            }else {
                int root=60+t->p[P_ROOT];
                if(g->lane==2)root=scale_steps(t,root,(int)(dg_random(&rnd)%8));
                root=clamp(scale_snap(t,root),0,127);
                n=g->lane==3?chord_build(t,(unsigned)root,notes):1;
                if(g->lane!=3||!n){n=1;notes[0]=(uint8_t)root;}
            }
            if(trk_eng(t)==ENGI_ACID||trk_eng(t)==ENGI_M2STAT||trk_eng(t)==ENGI_M2SAW||trk_eng(t)==ENGI_M2SID)n=1;
        }
        if(brk) {
            /* Source changes and RETURN are automation, not gate events. */
            for(unsigned j=0;old->time==ST_NOTE&&j<old->n&&j<4&&n<4;j++)
                if(old->note[j]>=32&&old->note[j]<=34)notes[n++]=old->note[j];
            if(hit&&first&&g->lane>=2&&n<4)notes[n++]=(uint8_t)(g->lane==2?32:33);
        }
        st->n=(uint8_t)n;memcpy(st->note,notes,4);st->time=n?ST_NOTE:ST_REST;st->hit=st->acc=0;
        st->flags=(uint8_t)(st->flags&~(SF_ACCENT|SF_SLIDE|SF_RATCH));
        if(hit){if(r.accents>>i&1)st->flags|=SF_ACCENT;if(!st->vel)st->vel=100;first=0;}
    }
    if(g->type==DG_MUTATE)for(unsigned i=0;i<len;i++)if(t->step[i].time==ST_TIE){
        unsigned j=(i+len-1)%len,n=0;while(n++<len&&t->step[j].time==ST_TIE)j=(j+len-1)%len;
        if(t->step[j].time!=ST_NOTE)t->step[i].time=ST_REST;
    }
    if(brk)t->p[P_LN0]=1; /* the generated gates drive chops, not a free loop */
}
static void drumgen_apply(int undo) {
    if(chain_busy()){ui_message("STOP SONG TO EDIT");return;}
    if(undo){ui_message(undo_do()?"NOTHING TO UNDO":"SEQ GEN UNDONE");ui.force=1;return;}
    track_t *t=TSEL;if(trk_eng(t)==ENGI_EMPTY){ui_message("SELECT AN ENGINE FIRST");return;}drum_gen_t *g=dg_current();
    if(g->type==DG_MUTATE&&!g->density){ui_message("MUTATE AMOUNT IS ZERO");return;}
    unsigned len=(unsigned)clamp(t->p[P_SLEN],1,NSTEP),lane=g->lane&7;
    uint64_t before=0,acc=0,late,flam;
    for(unsigned i=0;i<len;i++)if(t->step[i].time==ST_NOTE&&(drum_track(t)?(step_lanes(&t->step[i])>>lane&1):seqgen_gate(t,&t->step[i]))) {
        before|=(uint64_t)1<<i;
        if(drum_track(t)?(t->step[i].acc>>lane&1):(t->step[i].flags&SF_ACCENT))acc|=(uint64_t)1<<i;
    }
    drum_gen_row_t r=drum_generate(g,len,before,acc,&late,&flam);
    undo_mark();load_begin(t,LOAD_PAT);
    if(!drum_track(t))seqgen_notes(t,g,len,r);
    else for(unsigned i=0;i<len;i++) {
        unsigned on=(unsigned)(r.hits>>i)&1;
        grid_hit(t,i,lane,on);
        if(on){step_t *st=&t->step[i];unsigned bit=1u<<lane;
            st->acc=(uint8_t)((st->acc&~bit)|((r.accents>>i&1)?bit:0));
            /* Per-lane FLAM avoids shifting the other seven drum lanes.
             * A lone Grids half-step is quantized to the current sixteenth. */
            if(g->type!=DG_MUTATE||!(before>>i&1))step_set_lmode(st,lane,(flam>>i&1)?LM_FLAM:LM_NONE);
        }
    }
    load_end(t);ui.lane=(uint8_t)lane;cursor_fix();ui.force=1;
    ui_say("SEQ GEN: ",DG_NAMES[g->type]);
}
