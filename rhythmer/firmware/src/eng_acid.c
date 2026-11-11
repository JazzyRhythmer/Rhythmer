/* SPDX-License-Identifier: GPL-3.0-only */
/* Native Hortator Acid synth. Pinned source and kernel notices are retained in
 * hortator_acid.c. The MIDI-only generator is a separate UI command module.
 * No X0X DSP, model union, parallel note/gate cache or audio control poll. */
#include "hortator_acid.c"
static ha_voice_t acid_voice[NTRK];
static int32_t acid_out[2][CTL];
static void acid_reset(track_t *t){memset(&acid_voice[t-trk],0,sizeof acid_voice[0]);}
static void acid_reset_all(void){memset(acid_voice,0,sizeof acid_voice);}
static void acid_context(track_t *t,const voice_t *v,ha_track_t *ctx){
 ctx->p[0]=0;ctx->p[1]=t->p[P_E3];ctx->p[2]=t->p[P_E0];ctx->p[3]=t->p[P_E1];
 ctx->p[4]=t->p[P_E2];ctx->p[5]=t->p[P_E4];ctx->p[6]=t->p[P_E5]&1;ctx->p[7]=56;
 ctx->pitch16=v->pitch16;ctx->wow=acid_voice[t-trk].wow;
 /* Constant glide coefficients avoid a software 64-bit divide in every block. */
 static const uint32_t glide[128]={1521742,1217394,1014495,869567,760871,676330,608697,553361,507247,468228,434783,405798,380435,358057,338165,320366,304348,289855,276680,264650,253623,243478,234114,225443,217391,209895,202899,196353,190217,184453,179028,173913,169082,164512,160183,156076,152174,148462,144927,141557,138340,135266,132325,129510,126811,124223,121739,119352,117057,114848,112721,110672,108695,106788,104947,103169,101449,99786,98176,96618,95108,93645,92226,90850,89514,88216,86956,85731,84541,83383,82256,81159,80091,79051,78038,77050,76087,75147,74231,73337,72463,71611,70778,69965,69170,68392,67633,66889,66162,65451,64755,64073,63405,62752,62111,61484,60869,60267,59676,59096,58528,57971,57424,56887,56360,55843,55336,54837,54347,53867,53394,52930,52473,52025,51584,51151,50724,50305,49893,49487,49088,48695,48309,47928,47554,47185,46822,46465};
 ctx->glide=(int32_t)glide[clamp(t->p[P_E6],0,127)];
}
/* The native synth owns its oscillator, ladder, envelope and glide. The
 * TB-3PO generator writes ordinary sequencer events and never runs here. */
static void acid_on(track_t *t,voice_t *v){
 unsigned k=(unsigned)(t-trk);ha_voice_t *h=&acid_voice[k];ha_track_t ctx;
 acid_context(t,v,&ctx);h->vel=v->vel;h->active=1;h->t=0;
 if(t->slide_glide&&(h->x[2]&1))ha_glide(&ctx,h);else ha_trigger(&ctx,h);
 h->wow=ctx.wow;
}
static int acid_done(track_t *t,voice_t *v){(void)v;return !acid_voice[t-trk].active;}
static void acid_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m){
 unsigned k=(unsigned)(t-trk);if(n>CTL)n=CTL;if(!n)return;
 ha_voice_t *h=&acid_voice[k];ha_track_t ctx;acid_context(t,v,&ctx);
 if(v->gate&&v->pitch16!=h->x[1]){h->vel=v->vel;ha_glide(&ctx,h);}
 if(!v->gate&&(h->x[2]&1))ha_gate(&ctx,h,0);
 h->wow=mulq16(ctx.wow,ACID_WOW);
 int32_t *y=acid_out[MC_CPU()&1u];memset(y,0,n*sizeof *y);
 if(h->active)ha_render(&ctx,h,y,n);
 for(unsigned i=0;i<n;i++){int32_t x=mulq15(y[i],amp_at(m,i));if(t->p[P_E7])x=softclip(mulq15(x,32767+t->p[P_E7]*256));out[i]+=x;}
}
static const preset_t ACID_PRESETS[]={
 /* Hortator's four ACID colours with Rhythmer's factory cutoff settings.
  * Tune and gate follow track pitch and SEQ; the last two controls are SLIDE and DRIVE. */
 {"HT SQUELCH",{80,64,64,50,64,0,21,0},{0,64,127,0},0,1|TAG(BASS),{39,1,1,14},1,0},
 {"HT DEEPBASS",{80,30,30,30,40,0,21,0},{0,64,127,0},0,1|TAG(BASS),{39,1,1,14},1,0},
 {"HT SCREAMER",{50,115,100,80,100,0,21,0},{0,64,127,0},0,1|TAG(BASS),{39,1,1,14},1,0},
 {"HT SQUAREAC",{80,80,70,60,64,1,21,0},{0,64,127,0},0,1|TAG(BASS),{39,1,1,14},1,0}
};
#define ACID_PD(n,hi,d) {n,F_INT,0,hi,d,0,0}
static const char *const ACID_WAVES[]={"SAW","SQR"};
static const engine_t ENG_ACID={.name="ACID 303",.page_title={"ACID FILTER","ACID VOICE"},
 .edit={ACID_PD("CUTOFF",127,64),ACID_PD("RESO",127,64),ACID_PD("ENV",127,64),ACID_PD("DECAY",127,64),
 ACID_PD("ACCENT",127,64),{"WAVE",F_ENUM,0,1,0,ACID_WAVES,0},ACID_PD("SLIDE",127,21),ACID_PD("DRIVE",127,0)},
 .presets=ACID_PRESETS,.npresets=NELEM(ACID_PRESETS),.knob={P_E0,P_E1,P_E2,P_E3},.poly=1,.ownenv=1,
 .note_on=acid_on,.render=acid_render,.done=acid_done};
#undef ACID_PD
