/* SPDX-License-Identifier: GPL-3.0-only */
/* MONO2K machine kernels, adapted from Iñigo Hernández's GPL-3.0-only
 * m2k_dsp.c / m2k_core.c at ecb3b96c65d9fa92b258923df6317e5bb0ef6b4d.
 * Only FM+STAT, SWAV-SAW, SID-6581. No MONO2K sequencer, track chain or FX buffers.
 * The original oscillator/control laws run in 16-frame subblocks; Rhythmer owns
 * ADSR, sequencing, modulation and FX. Runtime state fits the existing voice payload.
 */
#include "../mono2k/tables.h"
#define CLAMP(v,lo,hi) clamp((v),(lo),(hi))
#define M2K_SUB 16
#define P_SYN 0
#define M_SID 0
#define M_FMS 1
#define M_SAW 2
#define M2K_TRACKS6 6
#define SID_A 24950
#define SID_MIX0 1024
#define SID_MIXA (SID_A * 15 / 32)
typedef struct { uint32_t ph[7], menv[1], lfsr, drng; int32_t ms[6], fine; uint8_t eff[8]; int32_t mod[8]; } m2k_voice_t;
static inline int32_t m2_P8(const m2k_voice_t *t,int id){return clamp((int32_t)t->eff[id]*256+t->mod[id],0,127*256);}
static inline int32_t m2_P7(const m2k_voice_t *t,int id){return m2_P8(t,id)>>8;}
static inline int32_t m2_PB8(const m2k_voice_t *t,int id){return m2_P8(t,id)-64*256;}
static inline int32_t m2_PK(const m2k_voice_t *t,int id,int k){return (m2_P8(t,id)*k)>>8;}
static inline uint32_t m2_tq_u32(const uint32_t *a,int v){int i=v>>8,f=v&255;return !f?a[i]:a[i+1]>=a[i]?a[i]+(((a[i+1]-a[i])*(unsigned)f)>>8):a[i]-(((a[i]-a[i+1])*(unsigned)f)>>8);}
static inline int32_t m2_tq_u16(const uint16_t *a,int v){int i=v>>8,f=v&255;return !f?a[i]:a[i]+(((int32_t)a[i+1]-a[i])*f>>8);}
static __attribute__((noinline)) int64_t m2_mul_sh(int32_t a,int32_t b,int sh){return ((int64_t)a*b)>>sh;}
/* Positive Q15 coefficient: split the signed sample at bit 15. Both products
 * fit in 32 bits, exactly matching the former wide multiply/shift. */
static inline int32_t m2_hp_q15(int32_t a,int32_t b){return (b>>15)*a+(((b&32767)*a)>>15);}
static __attribute__((noinline)) uint32_t m2_mulu_hi(uint32_t a,uint32_t b,int sh){return (uint32_t)(((uint64_t)a*b)>>sh);}
static __attribute__((noinline)) uint64_t m2_mul2_q15(int32_t s,int32_t a,int32_t b){return (uint64_t)(uint32_t)(((int64_t)s*b)>>15)<<32|(uint32_t)(((int64_t)s*a)>>15);}
static inline int32_t m2_sat16(int32_t x){return clamp(x,-32767,32767);}
static inline uint32_t m2_noise_step(uint32_t *s){uint32_t x=*s;x^=x<<13;x^=x>>17;x^=x<<5;*s=x;return x;}
static inline int m2_fr24(int v){return (v*24+8)>>7;}
static inline int32_t m2_sinp(uint32_t ph)
{
    uint32_t i = ph >> 22, f = (ph >> 6) & 0xFFFFu;
    int32_t a = m2_T_SIN[i], b = m2_T_SIN[i + 1];
    return a + (((b - a) * (int32_t)f) >> 16);
}

/* pitch in 1/64 semitones (MIDI note * 64) to a phase increment per sample */
static inline uint32_t m2_raw_inc_of(int32_t pq6)
{
    if (pq6 < 0) pq6 = 0;
    if (pq6 > 140 * 64) pq6 = 140 * 64;
    int32_t oct = pq6 / 768, r = pq6 - oct * 768;
    int32_t sh = 10 - oct;
    if (sh >= 0) return m2_T_INC_TOP[r] >> sh;
    uint32_t v = m2_T_INC_TOP[r];
    return v > (0x7FFFFFFFu >> (-sh - 1)) ? 0x7FFFFFFFu : v << -sh;
}

static inline int32_t m2_blep_saw(uint32_t ph, uint32_t inc, uint32_t inv)
{
    int32_t s = (int32_t)(ph >> 16) - 32768;
    if (ph < inc) {
        uint32_t t = (ph * inv) >> 16;
        int32_t p = 2 * (int32_t)t - (int32_t)((t * t) >> 16) - 65536;
        s -= p >> 1;
    } else if (~ph < inc) {
        uint32_t u = (~ph * inv) >> 16, w = 65536 - u;
        s -= (int32_t)((w * w) >> 16) >> 1;
    }
    return s;
}


static inline uint32_t m2_inc_of_fine(int32_t q,int32_t f){uint32_t v=m2_raw_inc_of(q);return v+(uint32_t)((int32_t)(v>>12)*f);}
#define inc_of(q) m2_inc_of_fine((q),t->fine)
static void m2k_kernel(m2k_voice_t *t,int m,int32_t pitch,int ti,uint32_t previous_inc,int16_t *o,unsigned count){
 switch(m){
    case M_SID: {                      /* PW PWAD PWRS WAVE MOD MSRC MFRQ TUNE */
        /* after the Monomachine's SID-6581, measured on its OS (tools/ref/chain_sid36.spec): an oscillator on the note,
         * TRI SAW PULS MIX NOIS at one peak level, band-limited. The pulse's duty is |PW - 64| / 128 (a square at 0
         * and 127, silent at 64); PWAD sweeps PW on from the trig, 2.70 steps a second a step, round and round, PWRS
         * ON starting it again at each trig. A second oscillator at MFRQ (C3 at 0, an octave every 32 steps, the same
         * at every note) or on the previous track's note (MSRC): SYNC plays it in the wave, hard-synced to the note;
         * RING multiplies the two triangles, unipolar, whatever the wave; R+S rings with the second synced */
        uint32_t inc1 = inc_of(pitch + (m2_PB8(t, P_SYN + 7) >> 7));
        int mod = m2_P7(t, P_SYN + 4) >> 5, wave = (m2_P7(t, P_SYN + 3) * 5) >> 7;
        /* (the previous track round the first six, as the Monomachine's: T1's is T6 in any build; T7's T6) */
        uint32_t inc2 = m2_P7(t, P_SYN + 5) >= 64 ? previous_inc
                                                : inc_of(48 * 64 + m2_PK(t, P_SYN + 6, 24));
        t->ms[4] = (int32_t)(((uint32_t)t->ms[4] + (uint32_t)m2_PK(t, P_SYN + 1, 64)) & 0x7FFFFFu);   /* the sweep, Q16 */
        int32_t pw = ((m2_P8(t, P_SYN + 0) << 8) + t->ms[4]) & 0x7FFFFF;                             /* PW, Q16, 0..128 */
        pw -= 64 << 16;
        uint32_t duty = (uint32_t)(pw < 0 ? -pw : pw) << 9;                  /* of a cycle (2^32): |PW - 64| / 128 */
        int sync = mod >= 2, ring = mod & 1, two = mod == 2;                 /* SYNC: the second is what's heard */
        uint32_t inc = two ? inc2 : inc1, iv = 0xFFFFFFFFu / (inc | 1), iv1 = 0xFFFFFFFFu / (inc1 | 1);
        for (int i = 0; i < (int)count; i++) {
            uint32_t p1 = t->ph[0];
            t->ph[0] += inc1;
            t->ph[1] += inc2;
            if (sync && t->ph[0] < p1)  /* the second restarts half a cycle on with the note's cycle (its partials
                                         * say so), as far into it as the note is */
                t->ph[1] = (1u << 31) + m2_mulu_hi(t->ph[0] * iv1, inc2, 32);
            uint32_t ph = two ? t->ph[1] : t->ph[0];
            int32_t s;
            if (ring) {                /* two unipolar triangles multiplied, its mean taken off (the chain takes the rest) */
                int32_t u1 = (int32_t)((t->ph[0] ^ (uint32_t)((int32_t)t->ph[0] >> 31)) >> 16);    /* 0..32767 */
                int32_t u2 = (int32_t)((t->ph[1] ^ (uint32_t)((int32_t)t->ph[1] >> 31)) >> 16);
                s = (((u1 * u2) >> 14) - 16384) * SID_A >> 16;
            } else if (wave == 0) {
                s = ((int32_t)((ph ^ (uint32_t)((int32_t)ph >> 31)) >> 15) - 32768) * SID_A >> 15;
            } else if (wave == 1) {
                s = m2_blep_saw(ph, inc, iv) * SID_A >> 15;
            } else if (wave == 2) {    /* two saws a duty apart: a pulse between +-SID_A, its mean left (the chain's) */
                s = (m2_blep_saw(ph, inc, iv) - m2_blep_saw(ph - duty, inc, iv) + 32768 - (int32_t)(duty >> 16)) * SID_A >> 15;
            } else if (wave == 3) {    /* MIX: the triangle AND the saw, as the chip combines them, its mean taken off */
                uint32_t tri = (ph ^ (uint32_t)((int32_t)ph >> 31)) << 1;
                s = ((int32_t)((tri & ph) >> 20) - SID_MIX0) * SID_MIXA >> 11;
            } else {                   /* NOIS: the chip's 23-bit register, clocked by bit 19 of the cycle's 24, its 8 */
                if (((ph ^ (ph - inc)) & (1u << 27)) && (ph & (1u << 27))) {     /* output bits */
                    uint32_t x = t->lfsr & 0x7FFFFFu;
                    if (!x) x = 0x7FFFF8u;
                    x = ((x << 1) | (((x >> 22) ^ (x >> 17)) & 1u)) & 0x7FFFFFu;
                    t->lfsr = x;
                    t->ms[0] = (int32_t)(((x >> 13) & 0x80) | ((x >> 12) & 0x40) | ((x >> 9) & 0x20) | ((x >> 7) & 0x10) |
                                         ((x >> 6) & 0x08) | ((x >> 3) & 0x04) | ((x >> 1) & 0x02) | (x & 0x01));
                }
                s = (t->ms[0] * 2 - 255) * (SID_A * 33 / 32) >> 8;    /* 0.3 dB over: as measured */
            }
            o[i] = (int16_t)m2_sat16(s);
        }
        break;
    }
    case M_FMS: {                      /* 1FRQ 1FIN 1ENV 1FB 2FRQ 2VOL TONE TUNE */
        /* after the Monomachine's FM+STAT, measured on its OS (tools/ref/chain_fm25.spec): a sine carrier on the note,
         * half GND-SIN's level, phase-modulated by two sine operators side by side. Each operator's ratio on one 24-step
         * table (1/64 .. 4); the first's fine-tuned by 0.75 .. 1.25 and fed back on itself; 1ENV its depth up to 64
         * (index 0 .. 1.56), above 64 that depth decaying from the trig (10 s .. 5 ms); 2VOL the second's depth, the same
         * law to 64 and on to index 14 at 127; both through a lowpass, TONE its cutoff (30 Hz at 0, 1.37 kHz at 64, open at
         * 127), so a high modulator goes in shallower; TUNE +-2 semitones */
        uint32_t inc = inc_of(pitch + (m2_PB8(t, P_SYN + 7) >> 7));
        uint32_t r1 = m2_mulu_hi(m2_T_FMR24[m2_fr24(m2_P7(t, P_SYN + 0))], 192 * 256 + (uint32_t)m2_P8(t, P_SYN + 1), 16);
        uint32_t r2 = m2_T_FMR24[m2_fr24(m2_P7(t, P_SYN + 4))];
        uint32_t i1 = m2_mulu_hi(inc, r1, 16), i2 = m2_mulu_hi(inc, r2, 16);
        int32_t env = m2_P8(t, P_SYN + 2);
        uint32_t k1 = m2_tq_u32(m2_T_FMB, env < 64 * 256 ? env : 64 * 256);
        if (env >= 65 * 256) {         /* the depth from the trig, decaying (m2_T_FMB[64] 32837: a word) */
            k1 = (m2_T_FMB[64] * (t->menv[0] >> 15)) >> 15;
            t->menv[0] = m2_mulu_hi(t->menv[0], m2_tq_u32(m2_T_FMEC, env - 64 * 256), 31);
        }
        uint32_t k2 = m2_tq_u32(m2_T_FMB, m2_P8(t, P_SYN + 5));
        int32_t lp = m2_tq_u16(m2_T_FMLP, m2_P8(t, P_SYN + 6));
        uint32_t kf = m2_tq_u32(m2_T_FMFB, m2_P8(t, P_SYN + 3));
        for (int i = 0; i < (int)count; i++) {
            int32_t m1 = m2_sinp(t->ph[1] + (uint32_t)((t->ms[0] + t->ms[1]) >> 1) * kf);   /* the low word alone */
            t->ms[1] = t->ms[0];
            t->ms[0] = m1;
            /* each modulator through two one-poles at TONE's cutoff on its way to the carrier (at 64, 1.37 kHz: its
             * depth measured 0.88 at 523 Hz, 0.30 at 2 kHz, 0.10 at 4 kHz; the feedback taken before), states Q23 */
            uint64_t d = m2_mul2_q15(lp, m1 * 256 - t->ms[2], m2_sinp(t->ph[2]) * 256 - t->ms[4]);   /* the two firsts */
            t->ms[2] += (int32_t)d;
            t->ms[4] += (int32_t)(d >> 32);
            d = m2_mul2_q15(lp, t->ms[2] - t->ms[3], t->ms[4] - t->ms[5]);                       /* and seconds */
            t->ms[3] += (int32_t)d;
            t->ms[5] += (int32_t)(d >> 32);
            uint32_t pm = (uint32_t)m2_mul_sh(t->ms[3], (int32_t)k1, 8) + (uint32_t)m2_mul_sh(t->ms[5], (int32_t)k2, 8);
            o[i] = (int16_t)(m2_sinp(t->ph[0] + pm) >> 1);
            t->ph[0] += inc;
            t->ph[1] += i1;
            t->ph[2] += i2;
        }
        break;
    }
    case M_SAW: {                      /* UNIL UNIW UNIX DRFT SUBX SUB1 SUB2 TUNE */
        /* after the Monomachine's SWAVE-SAW, measured on its OS (tools/ref/swave_saw*.spec): a rising saw, free
         * running; the unison pair at -0.42 / +0.43 c a step of UNIW, each as loud as UNIL says; the extended pair at
         * half that distance, UNIL x UNIX; a square an octave down on its own curve; all through one pole of highpass
         * that follows the note (cutoff 1.0 .. 2.6 x f0: the saw's hollow fundamental); then sines an octave and two
         * down. Against GND-SIN's sine the saw stands at 0.158, SUB1 at 0.217, SUB2 at 0.082. DRFT is MONO2K's own. */
        int32_t unil = m2_P8(t, P_SYN + 0), uniw = m2_P8(t, P_SYN + 1), drift = m2_P8(t, P_SYN + 3);   /* Q8 */
        int32_t pq = pitch + (m2_PB8(t, P_SYN + 7) >> 7);                  /* TUNE: +-2 semitones, 3.125 c a step */
        if (drift) {
            if (!t->drng) t->drng = 0x6C8E9CF5u ^ ((uint32_t)ti + 1u) * 0x9E3779B9u;   /* its own: either core, any order */
            t->ms[4] += (((int32_t)(m2_noise_step(&t->drng) >> 20) - 2048) * drift) >> 17;   /* 2^11 by 2^15 */
            t->ms[4] = CLAMP(t->ms[4], -(drift >> 5), drift >> 5);
            pq += t->ms[4] >> 6;
        }
        uint32_t inc[5], inv[5];
        inc[0] = inc_of(pq);
        inc[1] = m2_mulu_hi(inc[0], m2_tq_u32(m2_T_SW1LO, uniw), 24);
        inc[2] = m2_mulu_hi(inc[0], m2_tq_u32(m2_T_SW1HI, uniw), 24);
        inc[3] = m2_mulu_hi(inc[0], m2_tq_u32(m2_T_SW2LO, uniw), 24);
        inc[4] = m2_mulu_hi(inc[0], m2_tq_u32(m2_T_SW2HI, uniw), 24);
        int nv = unil ? 5 : 1;
        for (int k = 0; k < nv; k++) inv[k] = 0xFFFFFFFFu / (inc[k] | 1);
        int32_t g1 = 5166 * unil / (127 * 256), g2 = (g1 * m2_P8(t, P_SYN + 2)) / (127 * 256);   /* (2^13 by 2^15) */
        int32_t gx = (3369 * m2_tq_u16(m2_T_SWSUBX, m2_P8(t, P_SYN + 4))) >> 15;
        int32_t gs1 = 7097 * m2_P8(t, P_SYN + 5) / (127 * 256), gs2 = 2701 * m2_P8(t, P_SYN + 6) / (127 * 256);
        uint32_t ih = inc[0] >> 1, invh = 0xFFFFFFFFu / (ih | 1);
        int32_t a = m2_T_SWHP[CLAMP(pq >> 6, 0, 127)];
        for (int i = 0; i < (int)count; i++) {
            int32_t s = m2_blep_saw(t->ph[0], inc[0], inv[0]) * 5166;
            if (unil) {
                s += (m2_blep_saw(t->ph[1], inc[1], inv[1]) + m2_blep_saw(t->ph[2], inc[2], inv[2])) * g1;
                if (g2) s += (m2_blep_saw(t->ph[3], inc[3], inv[3]) + m2_blep_saw(t->ph[4], inc[4], inv[4])) * g2;
            }
            s >>= 15;                                                     /* rising, a drop at the reset, as the original */
            if (gx) s += ((m2_blep_saw(t->ph[5], ih, invh) - m2_blep_saw(t->ph[5] + 0x80000000u, ih, invh)) * gx) >> 15;
            for (int k = 0; k < nv; k++) t->ph[k] += inc[k];
            /* the highpass: y = a (y + x - x') */
            t->ms[0] = m2_hp_q15(a, t->ms[0] + s - t->ms[1]);
            t->ms[1] = s;
            int32_t y = t->ms[0];
            if (gs1) y += (m2_sinp(t->ph[5]) * gs1) >> 15;
            if (gs2) y += (m2_sinp(t->ph[6]) * gs2) >> 15;
            t->ph[5] += ih;
            t->ph[6] += inc[0] >> 2;
            o[i] = (int16_t)m2_sat16(y);
        }
        break;
    }

 }
}

#undef inc_of
#undef M2K_SUB
#undef P_SYN
#undef M2K_TRACKS6
#undef CLAMP

/* Persistent payload maps into ph[3] + s[8] (44 bytes), unchanged voice_t.
 * Every kernel runs on a local snapshot; no shared synth scratch between cores. */
static void m2k_load(m2k_voice_t *a,const voice_t *v,unsigned e){
 for(unsigned i=0;i<3;i++)a->ph[i]=v->ph[i];
 if(e==ENGI_M2SAW){for(unsigned i=0;i<4;i++)a->ph[i+3]=(uint32_t)v->s[i];a->ms[0]=v->s[4];a->ms[1]=v->s[5];a->ms[4]=v->s[6];a->drng=(uint32_t)v->s[7];}
 else if(e==ENGI_M2STAT){for(unsigned i=0;i<6;i++)a->ms[i]=v->s[i];a->menv[0]=(uint32_t)v->s[6];}
 else {a->ms[0]=v->s[0];a->ms[4]=v->s[1];a->lfsr=(uint32_t)v->s[2];}
}
static void m2k_store(const m2k_voice_t *a,voice_t *v,unsigned e){
 for(unsigned i=0;i<3;i++)v->ph[i]=a->ph[i];
 if(e==ENGI_M2SAW){for(unsigned i=0;i<4;i++)v->s[i]=(int32_t)a->ph[i+3];v->s[4]=a->ms[0];v->s[5]=a->ms[1];v->s[6]=a->ms[4];v->s[7]=(int32_t)a->drng;}
 else if(e==ENGI_M2STAT){for(unsigned i=0;i<6;i++)v->s[i]=a->ms[i];v->s[6]=(int32_t)a->menv[0];}
 else {v->s[0]=a->ms[0];v->s[1]=a->ms[4];v->s[2]=(int32_t)a->lfsr;}
}
static void m2k_note_on(track_t *t,voice_t *v){
 for(unsigned i=0;i<8;i++)v->s[i]=0;
 v->ph[0]=v->ph[1]=v->ph[2]=0;
 if(t->engine==ENGI_M2STAT){v->ph[0]=0x80000000u;v->s[6]=1u<<30;}
 if(t->engine==ENGI_M2SID)v->s[2]=0x7FFFF8;
}
static void m2k_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m){
 m2k_voice_t a={0};int16_t b[16];unsigned e=t->engine,kind=e==ENGI_M2STAT?M_FMS:e==ENGI_M2SAW?M_SAW:M_SID;
 m2k_load(&a,v,e);a.fine=m->fine;
 for(unsigned i=0;i<8;i++)a.eff[i]=(uint8_t)clamp(t->p[P_E0+i],0,127);
 /* Engine SHAPE / CUTOFF destinations stay available to both LFOs and slots. */
 a.mod[e==ENGI_M2STAT?2:e==ENGI_M2SAW?1:0]=m->shape-(64<<8);
 if(e==ENGI_M2STAT)a.mod[6]=m->cutoff;
 unsigned ti=(unsigned)(t-trk);uint32_t previous=m2_raw_inc_of(48*64);
 /* Only the previous track's stored note is read, never its DSP voice state. */
 if(e==ENGI_M2SID&&t->p[P_E5]>=64){const track_t *p=&trk[ti?ti-1:5];unsigned at=p->seq_idx%NSTEP;const step_t *s=&p->step[at];if(s->time==ST_NOTE&&s->n)previous=m2_raw_inc_of((int32_t)s->note[0]*64);}
 /* The measured SWAV-SAW kernel has much lower output than the other
  * machines. Match its level after the envelope and keep its waveform. */
 int32_t gain=e==ENGI_M2SAW?2:1;
 for(unsigned off=0;off<n;off+=16){unsigned count=n-off<16?n-off:16;
  m2k_kernel(&a,kind,m->pitch16*4,(int)ti,previous,b,count);
  for(unsigned i=0;i<count;i++)out[off+i]+=voice_amp(b[i],m,off+i)*gain;
 }
 m2k_store(&a,v,e);
}
/* Adapted musical starting sounds: source SYN values retained; Rhythmer ADSR/FX
 * replace MONO2K's independent amp/filter/delay chain. Not bit-identical patches. */
static const preset_t M2STAT_PRESETS[]={
 {"INIT STAT",{61,64,80,32,82,64,64,64},{0,85,96,65},0,1|TAG(KEYS)},
 {"FM KEYS",{98,63,105,8,109,19,55,64},{0,80,0,82},0,1|TAG(KEYS)},
 {"DX BASS",{80,64,98,24,101,24,80,64},{0,68,28,26},0,1|TAG(BASS)},
 {"FM BELL",{117,64,90,0,101,50,127,64},{0,86,0,70},0,1|TAG(BELL)},
 {"FM PAD",{101,66,30,30,112,40,110,64},{64,100,110,80},0,1|TAG(PAD)}
};
static const preset_t M2SAW_PRESETS[]={
 {"INIT SAW",{0,0,0,0,0,0,0,64},{0,90,110,45},0,1|TAG(LEAD)},
 {"REESE",{127,46,0,20,0,40,0,64},{1,90,127,30},0,1|TAG(BASS)},
 {"MOOG",{0,0,0,0,70,90,0,64},{0,70,0,28},0,1|TAG(BASS)},
 {"SUPER LEAD",{90,40,40,10,0,0,0,64},{2,90,127,40},0,1|TAG(LEAD)},
 {"BRASS",{70,20,0,0,0,0,0,64},{30,90,127,14},0,1|TAG(LEAD)},
 {"STRINGS",{100,50,70,20,0,0,0,64},{72,100,127,92},0,1|TAG(PAD)}
};
static const preset_t M2SID_PRESETS[]={
 {"INIT SID",{0,0,127,0,0,0,64,64},{0,90,120,50},0,1|TAG(LEAD)},
 {"CHIP SQR",{16,0,127,51,0,0,64,64},{0,90,127,14},0,1|TAG(BASS)},
 {"DUB TRI",{0,0,127,0,0,0,64,64},{4,100,127,50},0,1|TAG(BASS)},
 {"SID LEAD",{20,30,127,51,0,0,64,64},{2,90,127,30},0,1|TAG(LEAD)},
 {"SYNC BASS",{0,0,127,26,64,0,30,64},{0,100,127,20},0,1|TAG(BASS)},
 {"RING HIT",{0,0,127,0,32,0,64,64},{0,66,0,35},0,1|TAG(BELL)}
};
#define M2_PD(n,d) {n,F_INT,0,127,d,0,0}
static const engine_t ENG_M2STAT={.name="FM+STAT",.page_title={"FM 1","FM 2"},
 .edit={M2_PD("1FRQ",61),M2_PD("1FIN",64),M2_PD("1ENV",80),M2_PD("1FB",32),M2_PD("2FRQ",82),M2_PD("2VOL",64),M2_PD("TONE",64),M2_PD("TUNE",64)},
 .presets=M2STAT_PRESETS,.npresets=NELEM(M2STAT_PRESETS),.knob={P_E0,P_E2,P_E3,P_E6},.poly=1,.note_on=m2k_note_on,.render=m2k_render};
static const engine_t ENG_M2SAW={.name="SWAV-SAW",.page_title={"UNISON","SUB"},
 .edit={M2_PD("UNIL",0),M2_PD("UNIW",0),M2_PD("UNIX",0),M2_PD("DRFT",0),M2_PD("SUBX",0),M2_PD("SUB1",0),M2_PD("SUB2",0),M2_PD("TUNE",64)},
 .presets=M2SAW_PRESETS,.npresets=NELEM(M2SAW_PRESETS),.knob={P_E0,P_E1,P_E4,P_E5},.poly=1,.note_on=m2k_note_on,.render=m2k_render};
static const engine_t ENG_M2SID={.name="SID-6581",.page_title={"WAVE","MOD"},
 .edit={M2_PD("PW",0),M2_PD("PWAD",0),M2_PD("PWRS",127),M2_PD("WAVE",0),M2_PD("MOD",0),M2_PD("MSRC",0),M2_PD("MFRQ",64),M2_PD("TUNE",64)},
 .presets=M2SID_PRESETS,.npresets=NELEM(M2SID_PRESETS),.knob={P_E0,P_E3,P_E4,P_E6},.poly=1,.note_on=m2k_note_on,.render=m2k_render};
#undef M2_PD
#undef M_SID
#undef M_FMS
#undef M_SAW
#undef SID_A
#undef SID_MIX0
#undef SID_MIXA
