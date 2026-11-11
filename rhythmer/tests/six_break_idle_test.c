/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static uint8_t source[72000];
static int32_t peak(const int32_t *b){int32_t p=0;for(unsigned i=0;i<64;i++){int32_t a=b[i]<0?-b[i]:b[i];if(a>p)p=a;}return p;}
int main(void){
 int bad=0;int32_t b[64];for(unsigned i=0;i<sizeof source;i++)source[i]=(uint8_t)(73*i+19);
 ui_power_on();fm6_init();power_on_parts();trks_put(1);song.master_q12=4096;mc_host=1;mc_host_v12=1;mc_off=0;
 for(unsigned k=0;k<NTRK;k++){trk[k].engine=trk[k].eng_req=ENGI_EMPTY;memset(trk[k].v,0,sizeof trk[k].v);trk[k].xf_on=trk[k].xf=0;}
 track_t*t=&trk[1];set_engine_of(t,ENGI_BREAK);for(unsigned i=0;i<16;i++)mix_block(b,32);
 six_loops[0]=six_loops[1]=source;six_loop_mask=3;six_native_init();panic_req=0;
 t->p[P_ATK]=t->p[P_DEC]=t->p[P_REL]=0;t->p[P_SUS]=127;t->p[P_FCUT]=127;t->p[P_FRES]=t->p[P_FTYPE]=0;t->p[P_E3]=64;
 song.g[G_BPM]=124;song.playing=1;for(unsigned i=0;i<NSTEP;i++)step_clear(&t->step[i]);
 for(unsigned i=0;i<4000;i++)mix_block(b,32);
 bad+=check("empty Break advances the common transport without output",six_break_ticks>100&&peak(b)==0);
 seq_note_on(t,7,127);mix_block(b,32);
 bad+=check("a live chop after long silence sounds in its first control block",peak(b)>0&&t->v[0].gate);
 uint32_t tick=six_break_ticks;trk_all_off(t);
 for(unsigned i=0;i<4000;i++)mix_block(b,32);
 bad+=check("all-off closes the actual source gate and finishes ENV",!t->v[0].stage&&!t->v[0].s[BENV_OFF]&&peak(b)==0&&six_break_ticks>tick);
 tick=six_break_ticks;seq_note_on(t,3,127);mix_block(b,32);
 bad+=check("next live hit has no wake-up block or transport restart",peak(b)>0&&six_break_ticks>=tick);
 t->p[P_REL]=60;trk_all_off(t);song.playing=0;mix_msil=1;prf.rest_ph=6;
 bad+=check("stopped silence shortcut cannot freeze an unfinished Break release",!mix_hush());
 for(unsigned i=0;i<40000;i++)mix_block(b,32);
 bad+=check("stopped release completes the deferred source-off",!t->v[0].stage&&!t->v[0].s[BENV_OFF]);
 song.playing=1;seq_note_on(t,5,127);mix_block(b,32);
 bad+=check("PLAY plus a chop returns on the next block after stopped idle",peak(b)>0);
 printf("Break idle/wake integration: %s\n",bad?"FAIL":"PASS");return !!bad;
}
