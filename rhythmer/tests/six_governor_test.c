/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* The real audio ISR with a simulated DMA and clock: overload must free work
 * within one block without cutting another part's lead or bass. */
#define main hostsim_main
#include "hostsim.c"
#undef main

#define FM1_AUDIO_HALF 0x80u
#define FM1_TICKS_PER_US 1u
static uint32_t host_clock, host_us, host_half_reads, host_dma_advanced, host_acks, host_nest;
static uint8_t host_pending;
static volatile uint32_t t5_nested_ticks;              /* audio.c's: TIMER5 nested in the render (main.c) */
static uint32_t fm1_ticks(void) { t5_nested_ticks = host_nest; host_clock += host_us; return host_clock; }
static uint8_t fm1_audio_pending(void) { return host_pending; }
static void fm1_audio_ack_aux(uint8_t p) { (void)p; }
static uint32_t fm1_audio_free_half(void) { return host_dma_advanced && host_half_reads++ > 0u; }
static void fm1_audio_ack_half(void) { host_acks++; }
static void fm1_audio_init(int32_t *b, uint32_t n, void (*isr)(void), uint32_t p)
{ (void)b; (void)n; (void)isr; (void)p; }
void isr_alnk0(void) {}
#define FELUCCA_UAC 1                                  /* audio.c taps USB audio: here into host_tap */
static int32_t host_tap[2 * 64];
static uint32_t host_tap_n;
static void uac_tap(const int32_t *out, uint32_t n) { memcpy(host_tap, out, 8u * n); host_tap_n = n; }
static uint32_t host_acks_at_start;
static void uac_render_start(void) { host_acks_at_start = host_acks; }   /* (just before the blocks) */
static void uac_render_end(void) {}
#include "../firmware/src/audio.c"

int main(void){
 mc_host=1;mc_host_v12=1;mc_off=0;fm6_init();trk_live=NTRK;
 for(unsigned k=0;k<NTRK;k++){trk[k].engine=trk[k].eng_req=ENGI_ANALOG;}
 sys_load=LOAD_FULL;gov_cap=16;gov_lite=0;
 if(HALF_FRAMES!=512||voice_budget()!=12)return 1;
 /* Exact upstream governor: three soft halves remove one, hot removes one
  * immediately. LITE only follows repeated actual sheds or the hot floor. */
 for(unsigned i=0;i<GOV_HALVES(3);i++)gov_tick(1,0,0,0);
 if(gov_cap!=11||gov_lite)return 2;
 gov_tick(1,1,0,0);if(gov_cap!=10||gov_lite)return 3;
 for(unsigned i=0;i<GOV_HALVES(172);i++)gov_tick(0,0,1,0);
 if(gov_cap!=11||gov_lite)return 4;
 gov_tick(0,0,1,0);gov_tick(0,0,0,1);if(gov_cap!=9||gov_lite)return 5;
 gov_tick(0,0,0,1);if(gov_cap!=7||gov_lite!=LOAD_LITE)return 6;
 for(unsigned i=0;i<GOV_HALVES(1720);i++)gov_tick(0,0,1,0);
 if(gov_cap!=12||gov_lite)return 7;
 /* Fresh released and held hits survive budget and overload decisions for
  * the transient only. Old tails must become eligible afterward. */
 memset(trk[0].v,0,sizeof trk[0].v);memset(trk[1].v,0,sizeof trk[1].v);
 trk[0].engine=ENGI_DRUM;trk[1].engine=ENGI_ANALOG;trk[1].p[P_VOICE]=V_POLY;
 trk[0].v[0]=(voice_t){.active=1,.stage=2,.gate=0,.env=1,.age=1};
 trk[0].v[1]=(voice_t){.active=1,.stage=2,.gate=1,.note=80,.env=1,.age=2};
 trk[1].v[0]=(voice_t){.active=1,.stage=2,.gate=1,.note=40,.env=100,.age=3};
 trk[1].v[1]=(voice_t){.active=1,.stage=2,.gate=0,.note=60,.env=100,.age=4};
 voice_t drums[NVOICE];memcpy(drums,trk[0].v,sizeof drums);
 shed_voice();if(trk[1].v[1].stage!=4||memcmp(drums,trk[0].v,sizeof drums))return 10;
 trk[1].v[1].stage=2;trk[1].v[1].gate=1;shed_voice();
 if(trk[1].v[1].stage!=4||memcmp(drums,trk[0].v,sizeof drums))return 11;
 for(unsigned i=0;i<20;i++)shed_voice();
 if(trk[1].v[0].stage==4||memcmp(drums,trk[0].v,sizeof drums))return 12;
 track_t *victim=0;trk[1].v[1].active=0;
 if(quiet_tail(&victim)!=NVOICE)return 13;
 /* The drum transient window has elapsed: released and gated tails can
  * now be selected. There is no blanket engine exclusion or MD-only cap. */
 for(unsigned i=0;i<DRUM_ATTACK_BLOCKS;i++)for(unsigned v=0;v<2;v++)env_tick(&trk[0],&trk[0].v[v]);
 trk[0].v[0].gate=0;trk[0].v[0].env_out=1;
 victim=0;if(quiet_tail(&victim)!=0||victim!=&trk[0])return 14;
 victim=0;if(voice_victim(&trk[1],0,&victim)!=0||victim!=&trk[0])return 15;
 shed_voice();if(trk[0].v[0].stage!=4)return 16;
 shed_voice();if(trk[0].v[1].stage!=4)return 17;
 /* Budget work must never suppress a new hit; retriggers restart protection. */
 trk_all_off(&trk[0]);memset(trk[0].v,0,sizeof trk[0].v);gov_cap=4;
 voice_start(&trk[0],&trk[0].v[0],36,127,0);
 if(!drum_attack(&trk[0],&trk[0].v[0]))return 18;
 /* Measured output work must fit alongside core 0's assigned tracks. A
  * 150+100 pair, a 170 bus and an 80 output tail have a 270 optimum; ignoring
  * the tail puts both tracks on core 0 and takes 330. No voices are removed. */
 memset(mc_tk,0,sizeof mc_tk);six_loop_mask=0;
 for(unsigned k=0;k<NTRK;k++){trk[k].engine=ENGI_ANALOG;memset(trk[k].v,0,sizeof trk[k].v);}
 trk[0].v[0].active=trk[1].v[0].active=1;mc_tk[0]=146;mc_tk[1]=96;mc_tail_tk=80;
 uint32_t mask=mc_split(170),c0=80,c1=170;
 for(unsigned k=0;k<2;k++){uint32_t cost=mc_tk[k]+mc_cost(&trk[k])*MC_VW;if(mask&(1u<<k))c1+=cost;else c0+=cost;}
 if(!mask||c0>280||c1>280||!trk[0].v[0].active||!trk[1].v[0].active)return 9;
 puts("JIANT timed governor + transient-only drum protection + retained core split: PASS");return 0;
}
