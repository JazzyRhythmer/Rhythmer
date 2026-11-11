/* SPDX-License-Identifier: GPL-3.0-only
 * Rhythmer: Amenity Break behind the common track/clock interface.
 * The native Acid engine is independent in eng_acid.c. */
#include "../six/jungle.h"
static int8_t six_break_owner=-1;
static const uint8_t *six_loops[2];
static uint8_t six_loop_mask, six_loop_now, six_break_run;
static uint32_t six_break_acc, six_break_ticks, six_break_keys;
static uint8_t six_roll_held[16],six_roll_count;
static int32_t six_break_out[64];
static int16_t six_break_old[8];
/* Break uses slot 0 only as control state (active stays 0): no synth voice
 * allocation, new sample buffer or project-format field. */
enum { BENV_CUT, BENV_OFF, BENV_SEQ, BENV_KEYS };
static void mod_note(track_t *t,uint32_t note,uint32_t vel);
static int six_break_select(unsigned slot,int immediate);
static void six_break_straight(track_t *t);
static void six_break_env_on(track_t *t,unsigned cmd,unsigned vel){
 voice_t *v=&t->v[0];
 if(v->s[BENV_OFF]){jungle_sequence_off((unsigned)v->s[BENV_OFF]-1u);v->s[BENV_OFF]=0;}
 if(!v->gate){
  if(t->p[P_LPHASE]>=0){t->lfo_ph=(uint32_t)t->p[P_LPHASE]<<25;t->lfo_fade=0;}
  if(t->p[P_L2PHASE]>=0){t->lfo2_ph=(uint32_t)t->p[P_L2PHASE]<<25;t->lfo2_fade=0;}
 }
 v->active=0;v->gate=1;v->stage=1;v->note=(uint8_t)cmd;v->vel=v->mvel=(uint8_t)clamp((int)vel,1,127);
 v->pitch_cur=v->pitch16=60*16;v->henv=t->hum_env;v->fin_d=0;
 mod_note(t,cmd,v->vel);v->mrnd=t->m_rnd;t->m_vi=0;
}
static void six_break_env_release(track_t *t){
 voice_t *v=&t->v[0];if(v->s[BENV_SEQ]||v->s[BENV_KEYS])return;
 v->gate=0;if(v->stage)v->stage=3;
}
static void six_break_note(track_t *t,unsigned cmd,unsigned vel){
 if(cmd<32){t->v[0].s[BENV_SEQ]=(int32_t)((uint32_t)t->v[0].s[BENV_SEQ]|(1u<<cmd));six_break_env_on(t,cmd,vel);jungle_sequence_note(cmd);}
 else if(cmd<34)six_break_select(cmd-32,1);else if(cmd==34)six_break_straight(t);
}
static void six_break_note_off(track_t *t,unsigned cmd){
 if(cmd>=32)return;voice_t *v=&t->v[0];
 v->s[BENV_SEQ]=(int32_t)((uint32_t)v->s[BENV_SEQ]&~(1u<<cmd));six_break_env_release(t);
 if(!v->gate&&v->stage&&t->p[P_REL]>0)v->s[BENV_OFF]=(int32_t)cmd+1;
 else jungle_sequence_off(cmd);
}
static void six_record_command(track_t *t,unsigned cmd);
static void six_break_straight(track_t *t){t->p[P_E1]=t->p[P_E2]=six_break_old[1]=six_break_old[2]=0;jungle_variation(0);jungle_warp(0);jungle_straight();}
/* UI publishes only a context bit; the audio task owns held keys and DSP controls. */
static volatile uint8_t six_perf_enabled;
static uint8_t six_perf_was, six_key_cmd[27];
static const uint8_t six_white[16]={0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
static int six_break_command(unsigned key){
 if(six_perf_enabled){for(unsigned i=0;i<16;i++)if(key==six_white[i])return 16+(int)i;
  static const uint8_t black[8]={1,3,5,8,10,13,15,17},shots[8]={0,2,9,7,10,15,3,12};
  for(unsigned i=0;i<8;i++)if(key==black[i])return shots[i];
  if(key==20)return 32;if(key==22)return 33;
  if(key==25)return 32+((trk[six_break_track()>=0?(unsigned)six_break_track():song.sel].p[P_E4]&1)^1);return -1;}
 return -2; /* ordinary key: caller maps the slice */
}
static uint32_t six_filter_age,six_filter_q_age;
static unsigned six_filter_q=8;
static void six_native_init(void){
 drum_md_reset();
 acid_reset_all();
 six_break_owner=(int8_t)six_break_track();
 if(six_break_owner>=0&&trk[(unsigned)six_break_owner].engine==ENGI_BREAK)memset(&trk[(unsigned)six_break_owner].v[0],0,sizeof(voice_t));
 jungle_init();jungle_play(0);six_break_run=0;six_roll_count=0;six_break_acc=six_break_ticks=six_break_keys=0;
 for(unsigned i=0;i<8;i++)six_break_old[i]=-1;
 six_perf_was=0;memset(six_key_cmd,0,sizeof six_key_cmd);
 six_filter_age=six_filter_q_age=0;six_filter_q=8;
 six_loop_now=six_loops[0]?0:1;
 if(six_loops[six_loop_now])jungle_bind(six_loops[six_loop_now],72000,48000);
}
void jungle_midi_out(unsigned status){(void)status; /* JIANT owns MIDI clock. */}
static int six_break_select(unsigned slot,int immediate){
 if(slot>1||!six_loops[slot])return 0;
 trk[six_break_track()>=0?(unsigned)six_break_track():song.sel].p[P_E4]=(int16_t)slot;
 if(immediate&&slot!=six_loop_now){jungle_swap(six_loops[slot]);six_loop_now=(uint8_t)slot;}
 return 1;
}
static void six_break_on(track_t *t,voice_t *v){unsigned cmd=v->note,vel=v->vel;six_break_note(t,cmd,vel);v->active=0;if(v!=&t->v[0])v->gate=0;}
static void six_no_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m){(void)t;(void)v;(void)out;(void)n;(void)m;}
/* The same physical-key map records slice triggers in JIANT's armed sequencer. */
static unsigned six_break_slice(unsigned key){
 static const uint8_t slice[27]={0,0,1,2,2,9,3,4,7,5,10,6,7,15,8,3,9,12,10,11,13,12,14,13,15,15,15};
 return key<27?slice[key]:0;
}
static void six_break_key(uint32_t key,int down){
 static const uint8_t white[16]={0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
 static const uint8_t black[8]={1,3,5,8,10,13,15,17},shots[8]={0,2,9,7,10,15,3,12};
 if(key>=27)return;
 if(!down){
  six_break_keys&=~(1u<<key);unsigned count=0;
  for(unsigned i=0;i<six_roll_count;i++)if(white[six_roll_held[i]]!=key)six_roll_held[count++]=six_roll_held[i];
  six_roll_count=(uint8_t)count;if(count)jungle_roll(six_roll_held[count-1]);else jungle_roll_off();return;
 }
 for(unsigned i=0;i<16;i++)if(key==white[i]){
  if(!(six_break_keys&(1u<<key))&&six_roll_count<16)six_roll_held[six_roll_count++]=(uint8_t)i;
  six_break_keys|=1u<<key;jungle_roll(i);return;
 }
 for(unsigned i=0;i<8;i++)if(key==black[i]){jungle_one_shot(shots[i]);return;}
}
static void six_break_clock(uint32_t advance,uint32_t frames,uint32_t beat){
 int owner=six_break_track();uint32_t mask=0;
 if(owner<0){if(six_break_run){jungle_sequence_off(32);jungle_play(0);}six_break_owner=-1;six_break_run=six_roll_count=0;six_break_keys=0;jungle_tick_mask(0);return;}
 track_t *t=&trk[owner];
 if(six_break_owner!=owner){jungle_init();if(six_loops[six_loop_now])jungle_bind(six_loops[six_loop_now],72000,48000);six_break_owner=(int8_t)owner;six_break_run=six_roll_count=0;six_break_keys=0;memset(six_key_cmd,0,sizeof six_key_cmd);for(unsigned j=0;j<8;j++)six_break_old[j]=-1;}
 if(!six_track_enabled((unsigned)owner)){
  if(six_break_run||six_roll_count){jungle_roll_off();jungle_sequence_off(32);jungle_play(0);}
  six_break_run=six_roll_count=0;six_break_keys=0;t->rh_n=0;jungle_tick_mask(0);return;
 }
 for(unsigned i=0;i<8;i++)if(t->p[P_E0+i]!=six_break_old[i]){
  unsigned v=(unsigned)t->p[P_E0+i];six_break_old[i]=(int16_t)v;
  switch(i){case 0:jungle_feedback(v);break;case 1:jungle_variation(v);break;case 2:jungle_warp(v);break;
  case 3:jungle_filter(v);six_filter_age=six_filter_q_age=0;six_filter_q=14;jungle_filter_resonance(14);break;case 7:jungle_auto_roll(v);break;case 5:jungle_freeze(v!=0);break;case 6:if(v){six_break_straight(t);six_record_command(t,34);t->p[P_E6]=six_break_old[6]=0;}break;}
 }
 jungle_sequence_mode(t->p[P_LN0]!=0);
 if((!six_perf_enabled&&six_roll_count)||(!song.playing&&six_break_run)){
  six_roll_count=0;six_break_keys=0;jungle_roll_off();t->rh_n=0;
  t->v[0].s[BENV_KEYS]=0;six_break_env_release(t);
 }
 six_perf_was=six_perf_enabled;
 if(!song.playing&&six_break_run)jungle_sequence_off(32);
 if(six_filter_q>8){six_filter_age+=frames;six_filter_q_age+=frames;
  if(six_filter_age>=22050&&six_filter_q_age>=1323){six_filter_q_age=0;jungle_filter_resonance(--six_filter_q);}}
 if(song.playing&&!six_break_run){six_break_acc=six_break_ticks=0;jungle_play(0);memset(six_break_out,0,sizeof six_break_out);jungle_tick_mask(0);jungle_render(six_break_out,32);
  uint64_t q=(uint64_t)ms_clock*192u;six_break_ticks=beat?(uint32_t)(q/beat):0;
  six_break_acc=beat?(uint32_t)(q%beat)*frames:0;jungle_start_phase(six_break_ticks);}
 six_break_run=song.playing;jungle_play(song.playing);jungle_tempo((unsigned)song.g[G_BPM]*10u);
 unsigned wanted=(unsigned)t->p[P_E4]&1u;
 if(!six_loops[wanted])t->p[P_E4]=(int16_t)(wanted=six_loop_now);
 if(wanted!=six_loop_now&&six_loops[wanted]&&(!song.playing||six_break_ticks%48u==0)){
  six_break_select(wanted,1);
 }
 if(song.playing&&beat&&frames)for(unsigned i=0;i<frames&&i<32;i++){
  six_break_acc+=advance*192u;
  if(six_break_acc>=beat*frames){six_break_acc-=beat*frames;mask|=1u<<i;six_break_ticks++;}
 }
 jungle_tick_mask(mask);
}
static uint32_t six_break_render(track_t *t,int32_t *out,uint32_t n){
 if((int)(t-trk)!=six_break_track()){memset(out,0,n*sizeof *out);return 0;}if(n>32)n=32;memset(six_break_out,0,sizeof six_break_out);jungle_render(six_break_out,n);
 uint32_t audible=0;
 for(unsigned i=0;i<n;i++){out[i]=six_break_out[2*i]/256;audible|=out[i]!=0;}
 return audible; /* loaded banks are not sounding voices */
}
static const preset_t SIX_BREAK_PRESETS[]={
 {"BREAK A",{0,0,0,64,0,0,0,0},{0,0,127,0},0,1|TAG(DRUM),{0},0,0},
 {"BREAK B",{0,0,0,64,1,0,0,0},{0,0,127,0},0,1|TAG(DRUM),{0},0,0}
};
#define SIX_PD(n,hi,d) {n,F_INT,0,hi,d,0,0}
static const engine_t ENG_BREAK={.name="BREAK",.page_title={"AMENITY","BREAK LOOP"},
 .edit={SIX_PD("FDBK",127,0),SIX_PD("VAR",127,0),SIX_PD("WARP",127,0),SIX_PD("FILTER",127,64),
 SIX_PD("LOOP A/B",1,0),SIX_PD("FREEZE",1,0),SIX_PD("STRAIGHT",1,0),SIX_PD("AUTO ROLL",1,0)},
 .presets=SIX_BREAK_PRESETS,.npresets=2,.knob={P_E0,P_E1,P_E2,P_E3},.poly=1,.sampled=1,.oneshot=1,
 .note_on=six_break_on,.render=six_no_render};
#undef SIX_PD
