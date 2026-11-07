/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared sequence generator for device UI, automatic mutation and browser data engine. */
#pragma once
enum { BS_STRAIGHT,BS_MUTATE,BS_RANDOM,BS_FILL,BS_SHUFFLE,BS_REVERSE,
       BS_LEFT,BS_RIGHT,BS_AB,BS_A,BS_B,BS_CLEAR,BS_REVERT,BS_LOCKS };
static unsigned bs_len(const track_t *t){return (unsigned)clamp(t->p[P_SLEN],1,NSTEP);}
static unsigned bs_rnd(unsigned n){return n?rng()%n:0;}
/* A/B and return commands survive a new chop/repeat on their step. */
static void bs_hit(step_t *s,unsigned command){
 uint8_t keep[3];unsigned n=0;
 if(s->time==ST_NOTE)for(unsigned j=0;j<s->n&&j<4;j++)if(s->note[j]>=32&&s->note[j]<=34&&n<3)keep[n++]=s->note[j];
 step_clear(s);s->time=ST_NOTE;s->n=(uint8_t)(1+n);s->note[0]=(uint8_t)command;s->vel=100;
 for(unsigned j=0;j<n;j++)s->note[j+1]=keep[j];
}
static unsigned bs_has_audio(const step_t *s){
 if(s->time==ST_NOTE)for(unsigned j=0;j<s->n&&j<4;j++)if(s->note[j]<32)return 1;
 return 0;
}
static unsigned bs_nonempty(const track_t *t,unsigned len){unsigned n=0;for(unsigned i=0;i<len;i++)n+=bs_has_audio(&t->step[i]);return n;}
static void bs_straight(track_t *t,unsigned len){
 for(unsigned i=0;i<len;i++){step_clear(&t->step[i]);if(i&1)t->step[i].time=ST_TIE;else bs_hit(&t->step[i],(i/2)&15);}
 t->p[P_LN0]=1;
}
static void bs_vary(track_t *t,unsigned kind,unsigned amount){
 unsigned len=bs_len(t),changed=0;amount=(unsigned)clamp((int32_t)amount,0,127);
 if(!amount||!bs_nonempty(t,len))return;
 if(kind==BS_FILL){
  unsigned count=amount>95?8:4,start=len>count?len-count:0;
  /* Retrigger an existing phrase's tail; choose safe 1/16 and 1/32 rolls.
   * A real slice precedes every roll so a cold start is deterministic. */
  unsigned slice=(start/2)&15;bs_hit(&t->step[start],slice);
  for(unsigned i=start+1;i<len;i++)bs_hit(&t->step[i],24u+bs_rnd(8));
 }else for(unsigned i=0;i<len;i++){
  step_t *s=&t->step[i];
  if(kind==BS_MUTATE){
   /* Keep quarter-note anchors, rests and source-only automation intact. */
   if(!(i&3)||(!bs_has_audio(s)&&s->time!=ST_TIE)||bs_rnd(256)>=amount)continue;
   unsigned old=s->time==ST_NOTE&&s->n?s->note[0]:((i/2)&15);
   bs_hit(s,(old+1u+bs_rnd(15))&15);changed++;
  }else{
   /* New slice order, with the opening downbeat anchored. Amount adds
    * sixteenth retriggers; low amounts keep eighth-note continuity. */
   if((i&1)&&bs_rnd(128)>=amount){
    unsigned mods=0;for(unsigned j=0;s->time==ST_NOTE&&j<s->n&&j<4;j++)mods|=s->note[j]>=32;
    if(mods){bs_hit(s,bs_rnd(16));}else{step_clear(s);s->time=ST_TIE;}
   }else bs_hit(s,!i?0:bs_rnd(16));
  }
 }
 if(kind==BS_MUTATE&&!changed)for(unsigned i=1;i<len;i++)if((i&3)&&(bs_has_audio(&t->step[i])||t->step[i].time==ST_TIE)){
  unsigned old=t->step[i].time==ST_NOTE?t->step[i].note[0]:((i/2)&15);bs_hit(&t->step[i],(old+1u)&15);break;
 }
 t->p[P_LN0]=1;
}
