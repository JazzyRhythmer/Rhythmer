/* SPDX-License-Identifier: GPL-3.0-only */
/* Break-only sequence authoring. Main loop, bounded to 64 steps; no extra
 * voice, sample buffer, project format or audio-ISR work. Uses tl_src's
 * existing scratch and JIANT's bar-synchronous ORBIT/DRIFT scheduler. */
#include "break_sequence.c"
static const char *const BS_NAMES[]={"STRAIGHT","MUTATE","RANDOM","FILL","SHUFFLE","REVERSE",
 "SHIFT <","SHIFT >","A/B SEQ","LOOP A","LOOP B","CLEAR","REVERT","CLR MOTION"};
static const char *const BS_BLACK[]={"OFF","2 BARS","4 BARS","8 BARS","16 BARS",
 "MUTATE","FILL","RANDOM","ORBIT","DRIFT"};
static const char *bs_do(track_t *t,unsigned action,unsigned amount){
 unsigned len=tl_len(t);uint64_t was=tl_used(t,len);
 if(action==BS_A||action==BS_B){unsigned slot=action-BS_A;if(!six_loops[slot])return "BANK NOT LOADED";t->p[P_E4]=(int16_t)slot;return slot?"SOURCE B":"SOURCE A";}
 if(action==BS_LOCKS){motion_clear(t);return "AUTOMATION CLEARED";}
 if(action==BS_STRAIGHT){t->p[P_SLEN]=32;t->p[P_SDIV]=2;t->p[P_SSWING]=0;t->p[P_SGATE]=127;bs_straight(t,32);return "STRAIGHT / TWO BARS";}
 if(action==BS_CLEAR){for(unsigned i=0;i<len;i++)step_clear(&t->step[i]);latch_drop_req|=(uint8_t)(1u<<trk_index(t));tl_unlock(t,was,len);return "BREAK SEQ CLEARED";}
 if(action==BS_MUTATE||action==BS_RANDOM||action==BS_FILL){
  if(!bs_nonempty(t,len))bs_straight(t,len);
  bs_vary(t,action,amount);return action==BS_MUTATE?"BREAK MUTATED":action==BS_RANDOM?"NEW BREAK CHOPS":"BREAK FILL";
 }
 if(action==BS_AB){
  if(len<2)return "LEN MUST BE 2+";
  if(!bs_nonempty(t,len))bs_straight(t,len);
  /* Check capacity before editing, then replace only A/B source commands. */
  for(unsigned z=0;z<2;z++){const step_t *s=&t->step[z?len/2:0];unsigned n=0;for(unsigned j=0;s->time==ST_NOTE&&j<s->n&&j<4;j++)n+=s->note[j]!=32&&s->note[j]!=33;if(n==4)return "SOURCE STEP FULL";}
  for(unsigned i=0;i<len;i++){step_t *s=&t->step[i];if(s->time!=ST_NOTE)continue;unsigned n=0;for(unsigned j=0;j<s->n&&j<4;j++)if(s->note[j]!=32&&s->note[j]!=33)s->note[n++]=s->note[j];s->n=(uint8_t)n;if(!n)s->time=ST_TIE;}
  for(unsigned z=0;z<2;z++){step_t *s=&t->step[z?len/2:0];if(s->time!=ST_NOTE){s->n=0;s->time=ST_NOTE;s->vel=100;}s->note[s->n++]=(uint8_t)(32+z);}
  return "A/B HALF-PATTERN";
 }
 tl_take(t);
 if(action==BS_REVERSE)tl_reverse(t,len);
 else if(action==BS_LEFT||action==BS_RIGHT)tl_shift(t,len,action==BS_LEFT?len-1:1);
 else if(action==BS_SHUFFLE){
  uint8_t order[NSTEP],beat[16];unsigned nb=len/4;for(unsigned i=0;i<len;i++)order[i]=(uint8_t)i;
  for(unsigned i=0;i<nb;i++)beat[i]=(uint8_t)i;
  for(unsigned i=nb;i>2;i--){unsigned j=1+tl_rnd(i-1);uint8_t x=beat[i-1];beat[i-1]=beat[j];beat[j]=x;}
  if(nb>2){unsigned same=1;for(unsigned i=1;i<nb;i++)same&=beat[i]==i;if(same){uint8_t x=beat[1];beat[1]=beat[2];beat[2]=x;}}
  for(unsigned i=0;i<nb*4;i++)order[i]=(uint8_t)(beat[i/4]*4+i%4);tl_order(t,len,order);
 }
 return action==BS_SHUFFLE?"BEATS SHUFFLED":action==BS_REVERSE?"CHOPS REVERSED":action==BS_LEFT?"SHIFTED LEFT":"SHIFTED RIGHT";
}
