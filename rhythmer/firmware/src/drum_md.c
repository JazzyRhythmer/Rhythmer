/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita, Hugelton Instruments
 * Drum machine fork: 2026 DEADACTIVE
 * Rhythmer adapter for Hortator's MD synthesis bank, 2026 DEADACTIVE.
 * Pinned source 44c9ad4aa57bebf63f412bd552e57143c30b4065.
 * Eight fixed MD lanes per track, matching ordinary DRUM capacity, with hat choke.
 * All models share one union per hit; no heap or sample ROM. */
#include "md_states.h"
typedef struct {md_state_t ms; uint32_t t; int32_t rng,last; dx_voice_t *owner; uint8_t active,vel,kind;} md_voice_t;
typedef struct {int16_t p[8];int8_t nt;} md_track_t;
/* DRUM and FM6 cannot render on the same part at once. Their first four
 * runtime slots share RAM (not patches): engine switches finish the old
 * engine's fade before calling the new engine. The engine tag clears stale
 * pointers/phases once at that handoff, on the audio thread. This recovers
 * the original 512-frame DMA margin without reducing eight MD lanes or stacks. */
typedef union { md_voice_t md; fm6_note_t fm6;
#if RHYTHMER_PROPHET
 p5_voice_t prophet;
#endif
 } md_fm6_runtime_t;
typedef union {md_fm6_runtime_t voice[4];jungle_state_t break_state;} md_track_runtime_t;
static md_track_runtime_t md_shared[NPART];
static md_voice_t md_pool[NPART][3] __attribute__((section(".pool")));
static md_voice_t md_aux[NPART] __attribute__((section(".lowbuf")));
static uint8_t md_shared_engine[NPART];
static void md_memory(unsigned track,unsigned engine){
 if(md_shared_engine[track]!=engine){
  memset(&md_shared[track],0,sizeof md_shared[track]);
  if(engine==ENGI_DRUM){memset(md_pool[track],0,sizeof md_pool[track]);memset(&md_aux[track],0,sizeof md_aux[track]);}
  md_shared_engine[track]=(uint8_t)engine;
 }
}
static md_voice_t *md_at(unsigned track,unsigned slot){return slot<4?&md_shared[track].voice[slot].md:slot<7?&md_pool[track][slot-4]:&md_aux[track];}
static void drum_md_reset(void){memset(md_pool,0,sizeof md_pool);memset(md_aux,0,sizeof md_aux);memset(md_shared,0,sizeof md_shared);memset(md_shared_engine,0,sizeof md_shared_engine);}
static uint32_t pow2_q16(int32_t d16) {
 uint32_t u=(uint32_t)(d16+192*16),oct=u/192u;
 uint32_t r=pitch_inc(1600+u%192u)/(pitch_inc(1600)>>16);
 return oct>=16u?r<<(oct-16u):r>>(16u-oct);
}
static uint32_t inc_tune(uint32_t inc,int32_t semis) {return (uint32_t)(((uint64_t)inc*pow2_q16(clamp(semis,-48,48)*16))>>16);}
static int32_t dm_tune(const md_track_t *t) {return t->p[0]+clamp(t->nt,-24,24);}
static int32_t vel_gain(const md_voice_t *v) {return v->vel*258;}
static void dm_end(md_voice_t *v) {v->active=0;v->last=0;}
/* Q24 filter coefficient helpers: Hortator / Mutable Instruments (MIT).
 * Only called on a new cymbal strike, never per sample. */
#define QONE (1<<24)
#define Q24(x) ((int32_t)((x)*16777216.0+((x)<0?-0.5:0.5)))
static inline int32_t qm(int32_t a,int32_t b) {return (int32_t)(((int64_t)a*b+(1<<23))>>24);}
static inline int32_t qtan_acc(int32_t f)
{
    int32_t x = qm(f, Q24(3.14159265358979)), x2 = qm(x, x), p;
    p = Q24(9.5168091e-03);
    p = Q24(2.900525e-03) + qm(p, x2);
    p = Q24(5.33740603e-02) + qm(p, x2);
    p = Q24(1.333923995e-01) + qm(p, x2);
    p = Q24(3.333314036e-01) + qm(p, x2);
    return qm(x, QONE + qm(p, x2));
}

#define ENV1 (1<<24)
#define ENV_END (1<<10)
static inline uint32_t md_decay_k(int32_t i){return DECAY_K[clamp(i,0,127)];}
static inline int32_t env_q15(int32_t e){return e>>9;}
static inline int32_t dnoise(md_voice_t *v){return (int32_t)noise32(&v->rng)>>16;}
static inline void env_step(md_voice_t *v,uint32_t i){v->ms.kt.env[i]=mulq16(v->ms.kt.env[i],v->ms.kt.k[i]);}
static void dsvf_coef(dsvf_t *c, int32_t cut, int32_t reso)       /* cut 0..127 << 8, reso 0..127 */
{
    tsvf_t t;
    tsvf_coef(&t, cut, reso);
    c->a1 = t.a1;
    c->a2 = t.a2;
    c->a3 = t.a3;
    c->k = 8192 - reso * 7600 / 127;          /* damping, Q12, as tsvf_coef */
}

static inline int32_t dsvf_tick(const dsvf_t *c, int32_t in, int32_t *ic1, int32_t *ic2, int32_t *bp, int32_t *hp)
{
    int32_t v3 = in - *ic2;
    int32_t v1 = (c->a1 * *ic1 + c->a2 * v3) >> 13;
    int32_t v2 = *ic2 + ((c->a2 * *ic1 + c->a3 * v3) >> 13);
    *ic1 = clamp(2 * v1 - *ic1, -150000, 150000);
    *ic2 = clamp(2 * v2 - *ic2, -150000, 150000);
    *bp = v1;
    *hp = in - ((c->k * v1) >> 12) - v2;
    return v2;
}


static inline void dm_put(md_voice_t *v,int32_t *out,uint32_t i,int32_t x){x=mulq15(mulq15(x,vel_gain(v)),VOICE_FS);out[i]+=x;v->last=x;}
#define HZ(f) ((uint32_t)((f)*97391.548))
#define LIFE_A ((uint32_t)FS*11u/2u)
#define dk md_decay_k
#define track_t md_track_t
#define dvoice_t md_voice_t
#define P_E0 0
#include "md_ktrx_kernels.h"
#include "md_sdtrx_kernels.h"
#include "md_xttrx_kernels.h"
#include "md_trxn_kernels.h"
#include "md_rstrx_kernels.h"
#include "md_trxm_kernels.h"
#include "md_trxcb_kernels.h"
#include "md_cltrx_kernels.h"
#include "md_b2trx_kernels.h"
#include "md_bdefm_kernels.h"
#include "md_efmn_kernels.h"
#include "md_efmm_kernels.h"
#undef dk
#undef P_E0
#undef dvoice_t
#undef track_t
#undef LIFE_A
#undef HZ
#include "md_models.h"

/* The section's kit byte stores the extra model bit on MD lanes.
 * On native lanes the same bit flips the default kick role. */
static unsigned md_kind(const dx_lane_t *p){uintptr_t a=(uintptr_t)p,b=(uintptr_t)dx_kit;unsigned h=a>=b&&a<b+sizeof dx_kit?((dx_md_hi>>((a-b)/sizeof *p))&1u):0u;return dx_md_id(p->mode)|(h<<5);}
static void md_set_hi(dx_lane_t *p,unsigned kind){uintptr_t a=(uintptr_t)p,b=(uintptr_t)dx_kit;if(a>=b&&a<b+sizeof dx_kit){unsigned bit=1u<<((a-b)/sizeof *p);dx_md_hi=(uint8_t)((dx_md_hi&~bit)|((kind>>5)?bit:0));}}
static int md_ext_ok(const dx_lane_t *p,unsigned hi){for(unsigned i=0;i<8;i++)if(dx_is_md(p[i].mode)&&(dx_md_id(p[i].mode)|(((hi>>i)&1u)<<5))>=MD_NMODEL)return 0;return 1;}
static void md_tail(dx_voice_t *x,int32_t *y,uint32_t n){
 for(uint32_t i=0;i<n;i++)if(x->b2>0){y[i]=(int32_t)(((int64_t)x->b1*x->b2+(int64_t)y[i]*(32-x->b2))>>5);x->b2--;}
 x->hl=n?y[n-1]:x->hl;
}
static md_voice_t *md_find(unsigned track,dx_voice_t *x){
 for(unsigned j=0;j<NVOICE;j++){md_voice_t *v=md_at(track,j);if(v->owner==x)return v;}return 0;
}
static md_voice_t *md_claim(unsigned track,dx_voice_t *x,unsigned kind){
 md_memory(track,ENGI_DRUM);
 md_voice_t *best=0,*same=0,*free=0;
 for(unsigned j=0;j<NVOICE;j++){
  md_voice_t *v=md_at(track,j);
  if(v->owner&&(!v->owner->live||!dx_is_md(v->owner->model)))v->active=0;
  if((md_base(kind)==0||md_base(kind)==10||md_base(kind)==11)&&v->active&&(md_base(v->kind)==0||md_base(v->kind)==10||md_base(v->kind)==11))return v;
  if(v->owner==x)same=v;
  if(!v->active&&!free)free=v;
  if(!best||v->t>best->t)best=v;
 }return same?same:free?free:best;
}

#ifdef RHYTHMER_MD_TEST
static unsigned md_test_triggers,md_test_renders;
#endif
static void md_lane_run(track_t *t,drum_lane_t *lane,const dx_lane_t *patch,int32_t morph,int32_t tune16,int32_t dofs,int32_t cofs,int32_t nofs,int32_t *y,uint32_t n){
 unsigned track=(unsigned)(t-trk);dx_voice_t *x=&lane->x;unsigned kind=x->trig?md_kind(patch):0u;
 memset(y,0,n*sizeof *y);if(track>=NPART||kind>=MD_NMODEL)return;
 md_voice_t *v=md_find(track,x);
 if(!x->live&&!x->trig){if(v)v->active=0;return;}
 if(x->trig){
  v=md_claim(track,x,kind);
  if(v->owner&&v->owner!=x&&v->owner->live&&dx_is_md(v->owner->model)){v->owner->b1=v->owner->hl;v->owner->b2=32;}
  int32_t last=x->hl;memset(v,0,sizeof *v);v->owner=x;v->active=1;v->vel=127;v->kind=(uint8_t)kind;v->rng=x->rng?x->rng:(int32_t)(0x2545f491u+track*977u+kind*313u);
  x->model=patch->mode;x->b1=last;x->b2=32;x->trig=0;x->live=1;x->choke=0;
  int q[5];morph=clamp(morph,0,127);for(unsigned k=0;k<5;k++)q[k]=patch->a[k]+(patch->b[k]-patch->a[k])*morph/127;
  const md_model_t *model=&MD_MODELS[kind];md_track_t ctx;memcpy(ctx.p,model->def,sizeof ctx.p);ctx.nt=0;
  ctx.p[0]=(int16_t)clamp((q[0]-64)*3/4+tune16/16,-48,48);ctx.p[1]=(int16_t)clamp(q[2]+dofs,0,127);
  ctx.p[model->ctl[0]]=(int16_t)q[1];ctx.p[model->ctl[1]]=(int16_t)clamp(q[3]+nofs,0,127);ctx.p[model->ctl[2]]=(int16_t)clamp(q[4]+cofs,0,127);
#ifdef RHYTHMER_MD_TEST
  md_test_triggers++;
#endif
  model->trigger(&ctx,v);
 }
 if(v&&v->owner==x&&v->active){
#ifdef RHYTHMER_MD_TEST
  md_test_renders++;
#endif
  MD_MODELS[v->kind].render(0,v,y,n);x->rng=v->rng;
  if(x->choke){for(uint32_t i=0;i<n;i++)y[i]=(y[i]*(int32_t)(n-i))/(int32_t)n;v->active=0;x->choke=0;}
  if(patch->mode&DX_DRIVE)for(uint32_t i=0;i<n;i++)y[i]=softclip(y[i]*3);
 }
 md_tail(x,y,n);x->live=(v&&v->owner==x&&v->active)||x->b2>0;
}

static void md_patch_default(dx_lane_t *p,unsigned kind){
 const md_model_t *m=&MD_MODELS[kind%MD_NMODEL];md_set_hi(p,kind);p->mode=DX_MD(kind);p->a[0]=(uint8_t)clamp(64+(m->def[0]*4+(m->def[0]<0?-2:2))/3,0,127);p->a[2]=(uint8_t)m->def[1];
 p->a[1]=(uint8_t)m->def[m->ctl[0]];p->a[3]=(uint8_t)m->def[m->ctl[1]];p->a[4]=(uint8_t)m->def[m->ctl[2]];memcpy(p->b,p->a,5);
}

