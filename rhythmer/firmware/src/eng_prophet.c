/* SPDX-License-Identifier: GPL-3.0-only
 * Melodee DSP: Copyright (C) 2026 Kerem Kilic (Ellic Studio).
 * Existing Jazzy macro adapter; Rhythmer runtime ownership added 2026.
 * Audio kernel copied unchanged from the local Jazzy integration archive.
 */
#include "../ports/prophet_programs.h"
static voice_t *prophet_owner[NPART][4];
_Static_assert(sizeof(p5_voice_t)<=sizeof(md_voice_t),"Prophet fits existing mutually-exclusive MD/FM6 runtime");
static p5_voice_t *prophet_state(track_t *t,voice_t *v){
 unsigned ti=(unsigned)(t-trk);if(md_shared_engine[ti]!=ENGI_PROPHET){memset(prophet_owner[ti],0,sizeof prophet_owner[ti]);md_memory(ti,ENGI_PROPHET);}
 unsigned free=4;
 for(unsigned i=0;i<4;i++){if(prophet_owner[ti][i]==v)return &md_shared[ti][i].prophet;if(!prophet_owner[ti][i]||!prophet_owner[ti][i]->active)free=i;}
 if(free==4)free=0;prophet_owner[ti][free]=v;return &md_shared[ti][free].prophet;
}
static void prophet_on(track_t *t,voice_t *v){p5_voice_t *s=prophet_state(t,v);memset(s,0,sizeof *s);s->noise=0x13579bdu+(unsigned)(t-trk)*177u+(unsigned)(v-t->v)*77u;}
static void jp_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m){
 const int16_t *p=t->p+P_E0;p5_voice_t *s=prophet_state(t,v);uint8_t patch[56]={0};
 patch[P5_SAW_A]=p[3]==0||p[3]==3||p[3]==5||p[3]==7;patch[P5_SAW_B]=p[3]==0||p[3]==3;
 patch[P5_PULSE_A]=p[3]==1||p[3]==3||p[3]==6;patch[P5_PULSE_B]=p[3]==1||p[3]==3;patch[P5_TRI_B]=p[3]==2||p[3]==7;
 patch[P5_FILTER_REV]=(uint8_t)p[2];patch[P5_SYNC]=(uint8_t)p[7];patch[P5_POLY_FILTER]=p[6]>0;patch[P5_POLY_PW]=p[6]>0;patch[P5_POLY_FREQ]=p[6]>0;
 static const float zero[CTL]={0};
 float env=(float)m->envq15/32768.0f;
 p5_render_params_t c={0};c.f0=c.f1=env;c.a0=c.a1=1;c.cut=(float)p[0]+(float)m->cutoff/256.0f;c.k=p[1]*(p[2]?19200.0f:18800.0f)/(127.0f*4096.0f);c.filter_gain=1-p[1]*(p[2]?100.0f:45.0f)/32768.0f;
 c.filter_velocity=c.amp_velocity=1;c.env_amount=p[5]*.3f;c.pf_b=p[6]*.35f;c.pa_b=p[6]/512.0f;c.pa_env=p[5]/512.0f;c.pw_b=p[6]/256.0f;c.pwm_a=c.pwm_b=.5f+(m->shape-16384)/65536.0f;c.la=c.lb=.5f;c.ln=p[3]==4?1.0f:0;
 c.ia=m->inc;c.ib=cents_inc(clamp(m->pitch16+p[4]*16,0,2047),6,m->fine);c.wheel=zero;c.wheel_pitch=zero;
 p5_samples(s,patch,out,n,m,&c);
}
#define PC(n,l,h,d) {n,F_INT,l,h,d,0,0}
static const engine_t ENG_PROPHET={.name="PROPHET",.page_title={"FILTER / OSC","POLY-MOD"},.edit={PC("CUTOFF",0,127,70),PC("RESO",0,127,25),PC("REV",0,1,1),PC("OSC",0,7,0),PC("SEMI B",-48,48,0),PC("ENV",0,127,50),PC("POLYMOD",0,127,0),PC("SYNC",0,1,0)},.presets=PROPHET_PROGRAMS,.npresets=NELEM(PROPHET_PROGRAMS),.poly=3,.note_on=prophet_on,.render=jp_render,.knob={P_E0,P_E1,P_E5,P_E6}};
#undef PC
