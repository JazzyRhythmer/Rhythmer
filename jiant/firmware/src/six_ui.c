/* SPDX-License-Identifier: GPL-3.0-only */
#include "../six/six303_bridge.h"
static uint32_t six_gen_fx_at,six_song_fx_at;
static uint8_t six_gen_fx_kind;
static uint32_t six_gen_seed[NTRK],six_gen_bar[NTRK];
static int six_gen_page(void){return !ui.home&&!ui.menu&&!ui.confirm&&!ui.layer&&!ui.ly&&cur_page()->graph==GR_SIXGEN&&trk_eng(TSEL)==ENGI_ACID;}
static int six_perf_page(void){return !ui.home&&!ui.menu&&!ui.confirm&&!ui.layer&&!ui.ly&&trk_eng(TSEL)==ENGI_BREAK&&cur_page()->graph==GR_SIXBRK;}
/* Amenity HOME, explicitly scoped to a BREAK action page. DSP reset is
 * consumed by the audio-side control poll, never called from the UI core. */
static int six_return_page(void){return !ui.home&&!ui.menu&&!ui.confirm&&!ui.layer&&!ui.ly&&trk_eng(TSEL)==ENGI_BREAK&&cur_page()->graph==GR_SIXRETURN;}
static void six_return_apply(int apply){
 if(apply){undo_mark();TSEL->p[P_E6]=1;}
 for(unsigned i=0;i<nav_ch(SEC_EDIT)->n;i++){unsigned p=nv_page(SEC_EDIT,i);if(p<NPAGES&&PAGES[p].graph==GR_SIXBRK){nav_go(SEC_EDIT,i,0);break;}}
 ui.force=1;if(apply)ui_message("STRAIGHT BREAK");
}
static void six_gen_apply_to(track_t *t,int mutate,int automatic){
 unsigned ti=(unsigned)(t-trk);uint8_t notes[64],flags[64],cfg[8];unsigned len=(unsigned)clamp(t->p[P_SLEN],1,64);
 if(chain_busy()){if(!automatic)ui_message("STOP SONG TO GENERATE");return;}
 if(!automatic)undo_mark();
 /* An untouched legacy C/CHR is not a chosen scale. Explicit CHR stays CHR. */
 if(t->p[P_LN5]<64&&!t->p[P_ROOT]&&!t->p[P_SCALE])t->p[P_SCALE]=2;
 for(unsigned i=0;i<8;i++)cfg[i]=(uint8_t)param_fit(&SIX_GEN[i],t->p[P_LN0+i]);
 for(unsigned i=0;i<len;i++){step_t *s=&t->step[i];int32_t snapped=scale_snap(t,s->note[0]);if(snapped<0)snapped+=12;notes[i]=(uint8_t)clamp(snapped,0,127);flags[i]=(s->time==ST_NOTE?1:s->time==ST_TIE?2:0)|((s->flags&SF_ACCENT)?4:0)|((s->flags&SF_SLIDE)?8:0);}
 if(!mutate){six_gen_seed[ti]=(fm1_ms^ui.frame*2654435761u^six_gen_seed[ti])*1664525u+1013904223u;if(!six_gen_seed[ti])six_gen_seed[ti]=1;}
 six303_generate_scl(notes,flags,len,cfg,&six_gen_seed[ti],mutate,(unsigned)t->p[P_ROOT],(uint16_t)scale_mask(t));
 load_begin(t,LOAD_PAT);
 for(unsigned i=0;i<len;i++){step_t *s=&t->step[i];step_clear(s);unsigned g=flags[i]&3;s->time=g==1?ST_NOTE:g==2?ST_TIE:ST_REST;s->n=g==1?1:0;s->note[0]=notes[i];s->vel=(flags[i]&4)?127:96;s->flags=((flags[i]&4)?SF_ACCENT:0)|((flags[i]&8)?SF_SLIDE:0);}
 six_gen_fx_at=fm1_ms;six_gen_fx_kind=mutate?1:2;
 t->p[P_VOICE]=V_MONO;load_end(t);ui.force=1;if(!automatic)ui_message(mutate?"303 MUTATED":"NEW 303 LINE");
}
static void six_gen_apply(int mutate,int automatic){six_gen_apply_to(TSEL,mutate,automatic);}
static void six_wave_cache(unsigned slot);
static void six_gen_poll(void){
 if(!song.playing&&six_loop_mask){six_wave_cache(0);six_wave_cache(1);}
 six_perf_enabled=(uint8_t)(six_perf_page()&&six_track_enabled(song.sel));
 uint32_t b=bar_n;
 for(unsigned k=0;k<NTRK;k++){
  track_t *t=&trk[k];if(t->eng_req!=ENGI_ACID)continue;unsigned n=(unsigned)t->p[P_LN7];
  if(!song.playing){six_gen_bar[k]=b;continue;}if(b==six_gen_bar[k])continue;six_gen_bar[k]=b;
  if(six_track_enabled(k)&&n&&b&&b%n==0)six_gen_apply_to(t,1,1);
 }
}
