/* SPDX-License-Identifier: GPL-3.0-only
 * Shared A/B source banks with independent per-track Amenity playback. */
#include "../six/jungle.h"
static const uint8_t *six_loops[2];
static uint8_t six_loop_mask;
static int32_t six_break_out[2][64];
static uint8_t six_break_comb_low[2][JUNGLE_COMB_BYTES];
static uint8_t six_break_comb_pool[NTRK-2][JUNGLE_COMB_BYTES] __attribute__((section(".pool")));
static uint8_t *six_break_line(unsigned k){return k<2?six_break_comb_low[k]:six_break_comb_pool[k-2u];}
_Static_assert(sizeof(jungle_state_t)<=sizeof(md_shared[0].voice),"Break state fits per-track DSP storage");
static volatile uint8_t six_perf_enabled;

static jungle_state_t *six_break_state(const track_t *t)
{
    return &md_shared[(unsigned)(t-trk)].break_state;
}
static void six_break_reset(track_t *t){md_shared_engine[(unsigned)(t-trk)]=0;}
static jungle_state_t *six_break_prepare(track_t *t)
{
    unsigned k=(unsigned)(t-trk);
    jungle_state_t *ctx=six_break_state(t);
    if (md_shared_engine[k]!=ENGI_BREAK) {
        memset(&md_shared[k],0,sizeof md_shared[k]);
        md_shared_engine[k]=ENGI_BREAK;
        ctx->comb=six_break_line(k);
        jungle_init(ctx);
        ctx->loop_now=(uint8_t)(t->p[P_E4]&1u);
        if (!six_loops[ctx->loop_now]) ctx->loop_now=(six_loop_mask&1u)?0:1;
        ctx->filter_q=8;
        for (unsigned i=0;i<8;i++) ctx->old[i]=-1;
        if (six_loops[ctx->loop_now]) jungle_bind(ctx,six_loops[ctx->loop_now],72000,48000);
    }
    return ctx;
}
enum { BENV_CUT, BENV_OFF, BENV_SEQ, BENV_KEYS };
static void mod_note(track_t *t,uint32_t note,uint32_t vel);
static int six_break_select(track_t *t,unsigned slot,int immediate);
static void six_break_straight(track_t *t);
static void six_break_env_on(track_t *t,unsigned cmd,unsigned vel){
 jungle_state_t *ctx=six_break_prepare(t);
 voice_t *v=&t->v[0];
 if(v->s[BENV_OFF]){jungle_sequence_off(ctx,(unsigned)v->s[BENV_OFF]-1u);v->s[BENV_OFF]=0;}
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
 jungle_state_t *ctx=six_break_prepare(t);
 if(cmd<32){t->v[0].s[BENV_SEQ]=(int32_t)((uint32_t)t->v[0].s[BENV_SEQ]|(1u<<cmd));six_break_env_on(t,cmd,vel);jungle_sequence_note(ctx,cmd);}
 else if(cmd<34)six_break_select(t,cmd-32,1);else if(cmd==34)six_break_straight(t);
}
static void six_break_note_off(track_t *t,unsigned cmd){
 jungle_state_t *ctx=six_break_prepare(t);
 if(cmd>=32)return;voice_t *v=&t->v[0];
 v->s[BENV_SEQ]=(int32_t)((uint32_t)v->s[BENV_SEQ]&~(1u<<cmd));six_break_env_release(t);
 if(!v->gate&&v->stage&&t->p[P_REL]>0)v->s[BENV_OFF]=(int32_t)cmd+1;
 else jungle_sequence_off(ctx,cmd);
}
static void six_record_command(track_t *t,unsigned cmd);
static void six_break_straight(track_t *t){
 jungle_state_t *ctx=six_break_prepare(t);t->p[P_E1]=t->p[P_E2]=ctx->old[1]=ctx->old[2]=0;jungle_variation(ctx,0);jungle_warp(ctx,0);jungle_straight(ctx);}
/* UI publishes only a context bit; the audio task owns held keys and DSP controls. */


static const uint8_t six_white[16]={0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
static int six_break_command(track_t *t,unsigned key){
 if(six_perf_enabled){for(unsigned i=0;i<16;i++)if(key==six_white[i])return 16+(int)i;
  static const uint8_t black[8]={1,3,5,8,10,13,15,17},shots[8]={0,2,9,7,10,15,3,12};
  for(unsigned i=0;i<8;i++)if(key==black[i])return shots[i];
  if(key==20)return 32;if(key==22)return 33;
  if(key==25)return 32+((t->p[P_E4]&1)^1);return -1;}
 return -2; /* ordinary key: caller maps the slice */
}
static void six_native_init(void)
{
    drum_md_reset();
    acid_reset_all();
    six_perf_enabled=0;
    for (unsigned k=0;k<NTRK;k++) {
        if (trk[k].engine!=ENGI_BREAK) continue;
        memset(&trk[k].v[0],0,sizeof(voice_t));
        six_break_prepare(&trk[k]);
    }
}

void jungle_midi_out(unsigned status){(void)status; /* JIANT owns MIDI clock. */}
static int six_break_select(track_t *t,unsigned slot,int immediate)
{
    if(slot>1||!six_loops[slot])return 0;
    jungle_state_t *ctx=six_break_prepare(t);
    t->p[P_E4]=(int16_t)slot;
    if(immediate&&slot!=ctx->loop_now){
        jungle_swap(ctx,six_loops[slot]);
        ctx->loop_now=(uint8_t)slot;
    }
    return 1;
}

static void six_break_on(track_t *t,voice_t *v){unsigned cmd=v->note,vel=v->vel;six_break_note(t,cmd,vel);v->active=0;if(v!=&t->v[0])v->gate=0;}
static void six_no_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m){(void)t;(void)v;(void)out;(void)n;(void)m;}
/* The same physical-key map records slice triggers in JIANT's armed sequencer. */
static unsigned six_break_slice(unsigned key){
 static const uint8_t slice[27]={0,0,1,2,2,9,3,4,7,5,10,6,7,15,8,3,9,12,10,11,13,12,14,13,15,15,15};
 return key<27?slice[key]:0;
}
static void six_break_key(track_t *t,uint32_t key,int down){
 jungle_state_t *ctx=six_break_prepare(t);
 static const uint8_t white[16]={0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
 static const uint8_t black[8]={1,3,5,8,10,13,15,17},shots[8]={0,2,9,7,10,15,3,12};
 if(key>=27)return;
 if(!down){
  ctx->keys&=~(1u<<key);unsigned count=0;
  for(unsigned i=0;i<ctx->held_count;i++)if(white[ctx->roll_held[i]]!=key)ctx->roll_held[count++]=ctx->roll_held[i];
  ctx->held_count=(uint8_t)count;if(count)jungle_roll(ctx,ctx->roll_held[count-1]);else jungle_roll_off(ctx);return;
 }
 for(unsigned i=0;i<16;i++)if(key==white[i]){
  if(!(ctx->keys&(1u<<key))&&ctx->held_count<16)ctx->roll_held[ctx->held_count++]=(uint8_t)i;
  ctx->keys|=1u<<key;jungle_roll(ctx,i);return;
 }
 for(unsigned i=0;i<8;i++)if(key==black[i]){jungle_one_shot(ctx,shots[i]);return;}
}
static void six_break_clock_track(track_t *t,uint32_t advance,uint32_t frames,uint32_t beat){
 unsigned owner=(unsigned)(t-trk);uint32_t mask=0;
 jungle_state_t *ctx=six_break_prepare(t);
 if(!six_track_enabled((unsigned)owner)){
  if(ctx->run||ctx->held_count){jungle_roll_off(ctx);jungle_sequence_off(ctx,32);jungle_play(ctx,0);}
  ctx->run=ctx->held_count=0;ctx->keys=0;t->rh_n=0;jungle_tick_mask(ctx,0);return;
 }
 for(unsigned i=0;i<8;i++)if(t->p[P_E0+i]!=ctx->old[i]){
  unsigned v=(unsigned)t->p[P_E0+i];ctx->old[i]=(int16_t)v;
  switch(i){case 0:jungle_feedback(ctx,v);break;case 1:jungle_variation(ctx,v);break;case 2:jungle_warp(ctx,v);break;
  case 3:jungle_filter(ctx,v);ctx->filter_age=ctx->filter_q_age=0;ctx->filter_q=14;jungle_filter_resonance(ctx,14);break;case 7:jungle_auto_roll(ctx,v);break;case 5:jungle_freeze(ctx,v!=0);break;case 6:if(v){six_break_straight(t);six_record_command(t,34);t->p[P_E6]=ctx->old[6]=0;}break;}
 }
 jungle_sequence_mode(ctx,t->p[P_LN0]!=0);
 if((!(six_perf_enabled&&t==TSEL)&&ctx->held_count)||(!song.playing&&ctx->run)){
  ctx->held_count=0;ctx->keys=0;jungle_roll_off(ctx);t->rh_n=0;
  t->v[0].s[BENV_KEYS]=0;six_break_env_release(t);
 }

 if(!song.playing&&ctx->run)jungle_sequence_off(ctx,32);
 if(ctx->filter_q>8){ctx->filter_age+=frames;ctx->filter_q_age+=frames;
  if(ctx->filter_age>=22050&&ctx->filter_q_age>=1323){ctx->filter_q_age=0;jungle_filter_resonance(ctx,--ctx->filter_q);}}
 if(song.playing&&!ctx->run){ctx->clock_acc=ctx->clock_ticks=0;jungle_play(ctx,0);memset(six_break_out[0],0,sizeof six_break_out[0]);jungle_tick_mask(ctx,0);jungle_render(ctx,six_break_out[0],32);
  uint64_t q=(uint64_t)ms_clock*192u;ctx->clock_ticks=beat?(uint32_t)(q/beat):0;
  ctx->clock_acc=beat?(uint32_t)(q%beat)*frames:0;jungle_start_phase(ctx,ctx->clock_ticks);}
 ctx->run=song.playing;jungle_play(ctx,song.playing);jungle_tempo(ctx,(unsigned)song.g[G_BPM]*10u);
 unsigned wanted=(unsigned)t->p[P_E4]&1u;
 if(!six_loops[wanted])t->p[P_E4]=(int16_t)(wanted=ctx->loop_now);
 if(wanted!=ctx->loop_now&&six_loops[wanted]&&(!song.playing||ctx->clock_ticks%48u==0)){
  six_break_select(t,wanted,1);
 }
 if(song.playing&&beat&&frames)for(unsigned i=0;i<frames&&i<32;i++){
  ctx->clock_acc+=advance*192u;
  if(ctx->clock_acc>=beat*frames){ctx->clock_acc-=beat*frames;mask|=1u<<i;ctx->clock_ticks++;}
 }
 jungle_tick_mask(ctx,mask);
}
static void six_break_clock(uint32_t advance,uint32_t frames,uint32_t beat)
{
    for(unsigned k=0;k<NTRK;k++)
        if(trk[k].engine==ENGI_BREAK)
            six_break_clock_track(&trk[k],advance,frames,beat);
}

static uint32_t six_break_render(track_t *t,int32_t *out,uint32_t n)
{
    jungle_state_t *ctx=six_break_prepare(t);
    int32_t *stereo=six_break_out[MC_CPU()&1u];
    if(n>32)n=32;
    memset(stereo,0,n*2u*sizeof *stereo);
    jungle_render(ctx,stereo,n);
    uint32_t audible=0;
    for(unsigned i=0;i<n;i++){out[i]=stereo[2*i]/256;audible|=out[i]!=0;}
    return audible;
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
