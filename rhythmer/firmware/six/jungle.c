/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception */
/* JUNGLE engine. See jungle.h.
 *
 * Clock: an integer accumulator gains bpm_x10 * 4 * JUNGLE_TICKS per sample
 * and emits a tick at JUNGLE_RATE * 600, so tempo is exact with no drift.
 * Sequencer, rolls and one-shots act on ticks; voices render per sample.
 *
 * The break is SEA5 in the memory-mapped sample bank. Each voice decodes
 * 20-frame groups on demand and keeps the last two, so playback forward
 * costs one group decode per 20 source frames; a new start decodes from its
 * 320-frame chunk's seed.
 *
 * STRETCH hits: the voice's timeline moves at half speed while two grains
 * read the slice at full speed. Every `hop` samples a new grain starts at
 * the timeline and fades in over `fade` samples while the previous one
 * fades out (the two always sum to one), so each piece sounds twice. At a
 * trigger both grains start at the slice, which keeps the attack whole.
 * The defaults, 38 ms pieces and a 10 ms splice, are the user's pick: the
 * closest to an Akaizer-style 200% stretch. The splices bubble a little in
 * the lows; that is part of the sound (a band split did not help).
 *
 * No division by variables in audio code: everything it calls must stay
 * safe while Flash is being written. */
#include "jungle.h"
#define FM1_SEA_GROUP 20
#define FM1_SEA_FRAMES 320
#include "six_memory.h"

#ifndef FM1_AUDIO_DATA
#define FM1_AUDIO_DATA
#endif

/* Rhythmer omits the upstream EQ, saturation, compressor and bitcrusher. */

#ifndef JUNGLE_FADE_IN
#define JUNGLE_FADE_IN 48   /* samples, ~1.1 ms; equal fades keep a crossfade flat */
#endif
#ifndef JUNGLE_FADE_OUT
#define JUNGLE_FADE_OUT 48
#endif

#define TICK_THRESHOLD ((uint32_t)JUNGLE_RATE * 600U)
#define BAR_TICKS (JUNGLE_STEPS * JUNGLE_TICKS)
#define BEAT_TICKS (4 * JUNGLE_TICKS)
#define FULL 32767
#define SHOT_TICKS (JUNGLE_TICKS / 2)
#define SEA_BYTES 228U
#define NO_GROUP 0xffffffffU
#define ONE_Q24 (1U << 24)

/* 2^(n/12) in Q16 for n = -24..+24. */
static FM1_AUDIO_DATA const uint32_t pitch_q16[49] = {
    16384, 17358, 18390, 19484, 20643, 21870, 23170, 24548,
    26008, 27554, 29193, 30929, 32768, 34716, 36781, 38968,
    41285, 43740, 46341, 49097, 52016, 55109, 58386, 61858,
    65536, 69433, 73562, 77936, 82570, 87480, 92682, 98193,
    104032, 110218, 116772, 123715, 131072, 138866, 147123, 155872,
    165140, 174960, 185364, 196386, 208064, 220436, 233544, 247431,
    262144,
};
/* DJ filter: Chamberlin state-variable, run twice per sample (88.2 kHz)
 * to stay stable up to its 12 kHz top. Q15 frequency coefficients and the
 * cutoff in Hz per knob value; 64 is off. Q 1.4 gives a sweep some edge
 * (the level moves at most ~1 dB above the dry break); the panel uses it
 * only while the knob turns and rests at a flat Q 0.8. */
static FM1_AUDIO_DATA const uint16_t filter_f[128] = {
    350, 375, 402, 430, 460, 493, 528, 565, 606, 648, 694, 744,
    796, 853, 913, 978, 1047, 1121, 1201, 1286, 1377, 1475, 1579, 1691,
    1811, 1939, 2076, 2223, 2381, 2550, 2730, 2924, 3131, 3352, 3590, 3844,
    4116, 4407, 4719, 5053, 5410, 5792, 6202, 6640, 7108, 7610, 8147, 8721,
    9334, 9991, 10693, 11443, 12245, 13101, 14016, 14992, 16033, 17144, 18327, 19587,
    20928, 22353, 23865, 25469, 0, 70, 76, 82, 89, 96, 104, 112,
    122, 132, 142, 154, 167, 181, 195, 211, 229, 248, 268, 290,
    314, 339, 367, 397, 430, 465, 504, 545, 590, 638, 691, 747,
    809, 875, 947, 1025, 1109, 1200, 1298, 1405, 1520, 1645, 1780, 1926,
    2084, 2255, 2440, 2641, 2857, 3092, 3346, 3620, 3917, 4238, 4586, 4962,
    5368, 5808, 6283, 6797, 7353, 7954, 8604, 9306,
};
static const uint16_t filter_hz[128] = {
    150, 161, 172, 184, 197, 211, 226, 242, 259, 278, 297, 319,
    341, 365, 391, 419, 449, 480, 514, 551, 590, 632, 677, 724,
    776, 831, 890, 953, 1020, 1093, 1170, 1253, 1342, 1437, 1539, 1648,
    1764, 1889, 2023, 2167, 2320, 2485, 2661, 2849, 3051, 3267, 3499, 3747,
    4012, 4297, 4601, 4927, 5277, 5651, 6051, 6480, 6939, 7431, 7957, 8521,
    9125, 9772, 10464, 11206, 0, 30, 32, 35, 38, 41, 45, 48,
    52, 56, 61, 66, 71, 77, 84, 91, 98, 106, 115, 124,
    134, 145, 157, 170, 184, 199, 216, 233, 253, 273, 296, 320,
    346, 375, 406, 439, 475, 514, 556, 602, 651, 705, 763, 825,
    893, 966, 1046, 1132, 1225, 1325, 1434, 1552, 1679, 1817, 1966, 2128,
    2302, 2491, 2696, 2917, 3157, 3416, 3696, 4000,
};
#define FILTER_OPEN 64
/* Damping 1/Q in Q14; jungle_filter_resonance sets it (host listening tests). */
 /* Q 0.8: flat; the panel raises it while sweeping */

enum { REPEAT, DOWN, UP, SHRINK, BUILD, ECHO, STRETCH_ALL };
enum { STRETCH_KEY = 2 };
#define BEAT (4 * JUNGLE_TICKS)
/* Per roll key: ticks between hits and what each hit does. */
static FM1_AUDIO_DATA const struct { uint8_t ticks, kind; } rolls[JUNGLE_ROLL_KEYS] = {
    { BEAT, REPEAT }, { BEAT, BUILD }, { 0, STRETCH_ALL }, { BEAT * 3 / 4, ECHO },
    { BEAT / 2, REPEAT }, { BEAT / 2, DOWN }, { BEAT / 2, UP }, { BEAT / 2, SHRINK },
    { BEAT / 4, REPEAT }, { BEAT / 4, DOWN }, { BEAT / 4, UP }, { BEAT / 4, SHRINK },
    { BEAT / 8, REPEAT }, { BEAT / 8, DOWN }, { BEAT / 8, UP }, { BEAT / 8, SHRINK },
};

/* Reads the SEA5 source, keeping the last two decoded 20-frame groups. */
/* Control task -> audio: single-word writes only. */



/* SEQ owns the playback gate in both LOOP and CHOP. Manual punch-ins remain an overlay. */



     /* 0, or 1 + rate * 4 + variant */
 /* request counter */
    /* request counter << 4 | slice */




 /* timeline / grain speed, Q16: 1 / slow; 4x, the user's pick */
/* Slower holds each slice's loud attack longer: -0.8, -1.4, -2.0 dB for
 * 2x, 3x, 4x keep stretch hits level with the rest (measured at 38/10 ms). */

/* Piece and splice in output samples; crossfade rise per sample, Q24. */
 /* 38 ms, 10 ms */

/* Sample binding: pointer last, so audio never sees new data with old sizes. */


 /* source frames per output sample at the tempo, Q24 */


/* Audio -> control task. */


/* Audio state. */


     /* SEQ fill this phrase: 0, or 1 + white key */
  /* its first tick in the phrase */
  /* the STRETCH key holds: every hit stretched */
 /* tick_late: Q16 samples since the tick */


/* MIDI clock in: counters from the routing task (one writer each). */
 /* clk_mark: pulses at the last Start/Continue */

/* Following it: the n-th pulse since pulse_base marks tick 8(n-1). A DAW
 * may send its pulses in bursts, once per audio buffer, so the break does not
 * jump to each pulse: a phase-locked loop runs it on its own clock, rate
 * lock_rate (the tempo it has found, as a tick increment), steered towards
 * the pulses by the phase error (1/256 tick, smoothed in lock_err), which
 * also slowly corrects lock_rate. Only when it is more than LOCK_FAR ticks
 * out does it jump ahead or wait. */
enum { CLOCK_LOST = JUNGLE_RATE }; /* a second without pulses */
enum { LOCK_FAR = 48 };            /* six pulses */
#ifndef LOCK_I
#define LOCK_I 23                  /* tempo correction: the error x 2^-LOCK_I per pulse */
#endif
#ifndef LOCK_I_FAST
#define LOCK_I_FAST 20
#endif
#ifndef LOCK_P
#define LOCK_P 16
#endif
#ifndef ROUGH_PULSES
#define ROUGH_PULSES 24U           /* the rough tempo over this many pulses */
#endif
#ifndef LOCK_MOST
#define LOCK_MOST 2
#endif





/* A first rough tempo, from the frames of the first two beats after Start
 * or Continue (rough_frame: the first pulse), for the control task to work
 * out and hand back as ctl_rough_rate, so the loop need not pull in from far
 * away. */

 /* rough_state: 0 first pulse, 1 two beats, 2 done */



/* Clock out: ticks since the internal clock started; F8 every 8th. */





 /* roll_now: 0, or 1 + key */

 /* ticks to the next hit, hits, ticks since start */


 /* leaving open jumps straight to the cutoff */
/* Comb: a feedback delay line of 23 ms, power of two for a mask. The output
 * is scaled by sqrt(1 - fb^2), which holds the average power of a broad
 * signal steady whatever the feedback. Feedback and scale glide per block. */
enum { COMB_SIZE = 1024 };




 /* E2, ~12 ms */
/* Delay in samples for C2..B2: 44100 / f. */
static const uint16_t comb_notes[12] = {674, 636, 601, 567, 535, 505, 477, 450, 425, 401, 378, 357};
#ifdef JUNGLE_TRACE
uint32_t jungle_host_frame;
void jungle_host_trigger(uint32_t frame, unsigned slice, int pitch, int32_t gain);
#endif

void jungle_init(jungle_state_t *ctx)
{
    uint8_t *line = ctx->comb;
    memset(ctx, 0, sizeof *ctx);
    ctx->comb = line;
    ctx->ctl_bpm = JUNGLE_NATIVE_BPM_X10;
    ctx->ctl_reciprocal = 0xffffffffU / (JUNGLE_NATIVE_BPM_X10 * 4U * JUNGLE_TICKS);
    ctx->ctl_filter = FILTER_OPEN;
    ctx->ctl_filter_damp = 20480;
    ctx->ctl_stretch_ratio = 16384;
    ctx->ctl_stretch_gain = 26029;
    ctx->ctl_grain_hop = 1675;
    ctx->ctl_grain_fade = 441;
    ctx->ctl_grain_step = ONE_Q24 / 441;
    ctx->ctl_comb_gain = FULL;
    ctx->ctl_comb_delay = 535;

    memset(ctx->voices, 0, sizeof ctx->voices);
    /* SIX can restore a stopped project without a power cycle: no old tail/control survives. */
    memset(ctx->comb,0,JUNGLE_COMB_BYTES);
    ctx->comb_write=ctx->comb_left=0;ctx->comb_fb=0;ctx->comb_gain=FULL;ctx->filter_low=ctx->filter_band=ctx->filter_coefficient=0;
    ctx->filter_was_open=1;
    ctx->six_start_tick=0;ctx->rng=0x9e3779b9U;ctx->tick_acc=ctx->tick_late=ctx->bar_tick=ctx->run_ticks=ctx->frame_count=0;
    ctx->ctl_playing=ctx->ctl_roll=ctx->ctl_freeze=ctx->ctl_stretch_hold=0;ctx->ext=ctx->ext_playing=0;
    ctx->last_slice=0;ctx->last_pitch=0;ctx->stretch_all=0;ctx->six_chop_mode=ctx->six_seq_roll=ctx->six_seq_release=ctx->six_seq_new=0;ctx->six_seq_hit=-1;ctx->six_seq_gates=0;
    ctx->fill_key = 0;
    for (unsigned i = 0; i < JUNGLE_STEPS; ++i) {
        ctx->steps[i].active = !(i & 1U);
        ctx->steps[i].slice = (uint8_t)(i / 2U % JUNGLE_SLICES);
        ctx->steps[i].pitch = 0;
    }
    ctx->was_playing = ctx->pattern_ready = ctx->roll_now = 0;
    ctx->shot_pending = -1;
    ctx->seen_shot = ctx->ctl_shot;
    ctx->seen_straight = ctx->ctl_straight;
    ctx->view_step = ctx->view_roll = -1;
}
/* The loop is two bars: its own tempo is 480 s / duration, so at bpm the
 * source advances frames * bpm_x10 / (44100 * 4800) frames per sample. */

void jungle_tick_mask(jungle_state_t *ctx, uint32_t mask) { ctx->six_tick_mask=mask; }
static void update_speed(jungle_state_t *ctx)
{
    ctx->ctl_speed = (uint32_t)((((uint64_t)ctx->src_frames * ctx->ctl_bpm << 24) + 105840000U) / 211680000U);
}
int jungle_bind(jungle_state_t *ctx, const uint8_t *sea, unsigned bytes, unsigned frames)
{
    ctx->src_data = NULL;
    ctx->src_generation = (uint8_t)(ctx->src_generation + 1U);
    if (!sea || frames != 48000U || bytes != 72000U) {
        ctx->src_frames = 0;
        return -1;
    }
    ctx->src_frames = frames; /* two bars at 16 kHz, 160 BPM; no duplicate virtual bar */
    ctx->src_slice = frames / JUNGLE_SLICES; /* sixteen unique eighth-note chops in the stored two bars */
    update_speed(ctx);
    ctx->src_data = sea;
    return 0;
}
/* Audio-task only: both slots have identical format. Preserve timeline, pitch,
 * stretch grains and roll state; crossfade the current voice with the new source. */
void jungle_swap(jungle_state_t *ctx, const uint8_t *data){
 if(!data||data==ctx->src_data)return;
 unsigned k=ctx->voices[1].active&&(!ctx->voices[0].active||ctx->voices[1].env>ctx->voices[0].env)?1:0;
 if(ctx->voices[k].active&&!ctx->voices[k].releasing){
  ctx->voices[k^1]=ctx->voices[k];ctx->voices[k].releasing=1;
  struct jungle_voice *v=&ctx->voices[k^1];v->source=data;v->env=0;
  for(unsigned r=0;r<2;r++)v->read[r].cached[0]=v->read[r].cached[1]=v->read[r].state_group=NO_GROUP;
 }
 ctx->src_data=data;ctx->seen_generation=++ctx->src_generation;
}
const uint8_t *jungle_bound(jungle_state_t *ctx) { return ctx->src_data; }
unsigned jungle_native_bpm_x10(jungle_state_t *ctx)
{
    uint32_t frames = ctx->src_frames;
    return frames ? (unsigned)((76800000U + frames / 2U) / frames) : 0U;
}
void jungle_start_phase(jungle_state_t *ctx, unsigned tick){ctx->six_start_tick=tick%(JUNGLE_STEPS*JUNGLE_TICKS);}
void jungle_play(jungle_state_t *ctx, unsigned on) { ctx->ctl_playing = on ? 1U : 0U; }
void jungle_seed(jungle_state_t *ctx, uint32_t seed) { ctx->ctl_seed = seed; }
void jungle_tempo(jungle_state_t *ctx, unsigned bpm_x10)
{
    unsigned bpm = bpm_x10 < JUNGLE_BPM_MIN_X10 ? JUNGLE_BPM_MIN_X10 :
                   bpm_x10 > JUNGLE_BPM_MAX_X10 ? JUNGLE_BPM_MAX_X10 : bpm_x10;
    if(bpm==ctx->ctl_bpm)return; /* same tempo: no repeated divide/speed rebuild per CTL */
    ctx->ctl_reciprocal = 0xffffffffU / (bpm * 4U * JUNGLE_TICKS);
    ctx->ctl_bpm = (uint16_t)bpm;
    update_speed(ctx);
}
static uint8_t clamp127(unsigned v) { return (uint8_t)(v > 127U ? 127U : v); }
void jungle_variation(jungle_state_t *ctx, unsigned rhythm) { ctx->ctl_rhythm = clamp127(rhythm); }
void jungle_pitch(jungle_state_t *ctx, int semitones)
{
    ctx->ctl_transpose = (int8_t)(semitones < -12 ? -12 : semitones > 12 ? 12 : semitones);
}
void jungle_filter(jungle_state_t *ctx, unsigned value) { ctx->ctl_filter = clamp127(value); }
void jungle_warp(jungle_state_t *ctx, unsigned amount) { ctx->ctl_warp = clamp127(amount); }
static uint32_t isqrt(uint32_t v)
{
    uint32_t r = 0;
    for (uint32_t bit = 1U << 30; bit; bit >>= 2) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; }
        else r >>= 1;
    }
    return r;
}
void jungle_feedback(jungle_state_t *ctx, unsigned amount)
{
    int32_t fb = (int32_t)(clamp127(amount) * 29491U / 127U); /* 0..0.9, Q15 */
    if (fb == ctx->ctl_comb_fb) return; /* unchanged panel value: no repeated square root */
    ctx->ctl_comb_gain = (int32_t)isqrt((1U << 30) - (uint32_t)(fb * fb));
    ctx->ctl_comb_fb = fb;
}
void jungle_feedback_note(jungle_state_t *ctx, unsigned pitch_class) { ctx->ctl_comb_delay = comb_notes[pitch_class % 12U]; }
void jungle_feedback_delay(jungle_state_t *ctx, unsigned tenths)
{
    unsigned samples = (tenths < 1U ? 1U : tenths > 230U ? 230U : tenths) * 441U / 100U;
    ctx->ctl_comb_delay = (uint16_t)(samples ? samples : 1U);
}
void jungle_stretch_hold(jungle_state_t *ctx, unsigned on) { ctx->ctl_stretch_hold = on ? 1U : 0U; }
void jungle_stretch_slow(jungle_state_t *ctx, unsigned times)
{
    static const uint16_t gains[3] = {29889, 27853, 26029};
    times = times < 2U ? 2U : times > 4U ? 4U : times;
    ctx->ctl_stretch_gain = gains[times - 2U];
    ctx->ctl_stretch_ratio = 65536U / times;
}
void jungle_stretch_grain(jungle_state_t *ctx, unsigned piece_ms, unsigned fade_tenths)
{
    unsigned hop = (piece_ms < 3U ? 3U : piece_ms > 80U ? 80U : piece_ms) * 441U / 10U;
    unsigned fade = (fade_tenths < 1U ? 1U : fade_tenths > 300U ? 300U : fade_tenths) * 441U / 100U;
    if (fade > hop) fade = hop;
    /* A single hop write switches; the fields are read once per hit. */
    ctx->ctl_grain_fade = (uint16_t)fade;
    ctx->ctl_grain_step = ONE_Q24 / fade;
    ctx->ctl_grain_hop = (uint16_t)hop;
}
void jungle_filter_resonance(jungle_state_t *ctx, unsigned q_tenths) { ctx->ctl_filter_damp = (int32_t)(163840U / (q_tenths < 5U ? 5U : q_tenths > 30U ? 30U : q_tenths)); }
unsigned jungle_filter_hz(unsigned value) { return filter_hz[value > 127U ? 127U : value]; }
void jungle_auto_roll(jungle_state_t *ctx, unsigned on) { ctx->ctl_auto = on ? 1U : 0U; }
void jungle_freeze(jungle_state_t *ctx, unsigned on) { ctx->ctl_freeze = on ? 1U : 0U; }
void jungle_straight(jungle_state_t *ctx) { ctx->ctl_straight = (uint8_t)(ctx->ctl_straight + 1U); }
void jungle_sequence_mode(jungle_state_t *ctx, unsigned chop){if(chop&&!ctx->six_chop_mode)ctx->six_seq_release=1;ctx->six_chop_mode=(uint8_t)(chop!=0);}
void jungle_sequence_note(jungle_state_t *ctx, unsigned command){
 if(command<32)ctx->six_seq_gates|=1u<<command;
 if(command<16){ctx->six_seq_hit=(int16_t)command;ctx->six_seq_roll=0;ctx->six_seq_new=0;ctx->six_seq_release=0;}
 else if(command<32){ctx->six_seq_roll=(uint8_t)(command-15);ctx->six_seq_new=1;ctx->six_seq_release=0;}
}
void jungle_sequence_off(jungle_state_t *ctx, unsigned command){
 if(command>=32){ctx->six_seq_hit=-1;ctx->six_seq_gates=0;}else ctx->six_seq_gates&=~(1u<<command);
 if(command>=16){ctx->six_seq_roll=0;ctx->six_seq_new=0;}
 if(!ctx->six_seq_gates){ctx->six_seq_hit=-1;ctx->six_seq_release=1;}
}
void jungle_roll(jungle_state_t *ctx, unsigned key)
{
    if (key < JUNGLE_ROLL_KEYS) ctx->ctl_roll = (uint8_t)(1U + key);
}
void jungle_roll_off(jungle_state_t *ctx) { ctx->ctl_roll = 0; }
void jungle_one_shot(jungle_state_t *ctx, unsigned slice)
{
    if (slice < JUNGLE_SLICES)
        ctx->ctl_shot = (uint16_t)(((ctx->ctl_shot >> 4) + 1U) << 4 | slice);
}
int jungle_current_step(jungle_state_t *ctx) { return ctx->view_step; }
int jungle_playing(jungle_state_t *ctx) { return ctx->view_playing; }
void jungle_midi_realtime(jungle_state_t *ctx, unsigned status)
{
    if (status == 0xf8U) ++ctx->clk_pulses;
    else if (status == 0xfaU) { ctx->clk_mark = ctx->clk_pulses; ++ctx->clk_starts; }
    else if (status == 0xfbU) { ctx->clk_mark = ctx->clk_pulses; ++ctx->clk_continues; }
    else if (status == 0xfcU) ++ctx->clk_stops;
}
int jungle_clock(jungle_state_t *ctx, unsigned *bpm_x10)
{
    if (ctx->view_rough != ctx->rough_done) {
        ctx->rough_done = ctx->view_rough;
        uint32_t span = ctx->view_rough_span, rate = ctx->view_rate;
        uint32_t rough = span ? (uint32_t)((uint64_t)ROUGH_PULSES * 8U * TICK_THRESHOLD / span) : 0U; /* ticks over span */
        /* Only when the loop is more than 3% out. */
        if (rough && (rough > rate + rate / 33U || rough + rate / 33U < rate)) { ctx->ctl_rough_rate = rough; ++ctx->ctl_rough; }
    }
    *bpm_x10 = (ctx->view_rate + 2U * JUNGLE_TICKS) / (4U * JUNGLE_TICKS);
    return ctx->view_ext;
}
int jungle_roll_active(jungle_state_t *ctx) { return ctx->view_roll; }
void jungle_pattern(jungle_state_t *ctx, struct jungle_step out[JUNGLE_STEPS]) { memcpy(out, ctx->steps, sizeof ctx->steps); }

/* ---- Audio interrupt ---------------------------------------------------- */

static JUNGLE_AUDIO_CODE unsigned rnd(jungle_state_t *ctx)
{
    uint32_t x = ctx->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    ctx->rng = x;
    return x >> 16; /* 0..65535 */
}
/* 0..n-1 without division. */
static JUNGLE_AUDIO_CODE unsigned rnd_below(jungle_state_t *ctx, unsigned n) { return (rnd(ctx) * n) >> 16; }

static JUNGLE_AUDIO_CODE void straight_pattern(jungle_state_t *ctx)
{
    for (unsigned i = 0; i < JUNGLE_STEPS; ++i) {
        ctx->steps[i].active = !(i & 1U);
        ctx->steps[i].slice = (uint8_t)((i >> 1) & (JUNGLE_SLICES - 1));
        ctx->steps[i].pitch = 0;
        ctx->steps[i].stretch = 0;
    }
    ctx->fill_key = 0;
}

/* The plugin's generator: 8th-note base, 16th retrigger fills and slice
 * swaps, plus STRETCH hits; the first four 8ths stay plain to anchor the
 * groove. Probabilities are Q16: 0.55 and 0.2 of the 0..127 amount. */
static JUNGLE_AUDIO_CODE void generate(jungle_state_t *ctx)
{
    unsigned rhythm = ctx->ctl_rhythm;
    straight_pattern(ctx);
    for (unsigned i = 0; i < JUNGLE_STEPS; i += 2) {
        unsigned early = i < 8;
        if (rnd(ctx) < rhythm * 284U) {
            ctx->steps[i + 1].active = 1;
            ctx->steps[i + 1].slice = ctx->steps[i].slice;
        }
        if (rnd(ctx) < rhythm * 103U)
            ctx->steps[i].slice = (uint8_t)rnd_below(ctx, JUNGLE_SLICES);
        /* Warp: up to a one-in-two chance per 8th at the top of the knob;
         * two in three stretch, one in three half speed (an octave down). */
        if (!early && rnd(ctx) < ctx->ctl_warp * 258U) {
            if (rnd(ctx) < 43691U) ctx->steps[i].stretch = 1;
            else ctx->steps[i].pitch = -12;
        }
    }
    /* SEQ fill: the phrase ends with one effect, from 8, 4 or 2 steps before
     * the end. 1/32 rolls only fit the last 2 steps, so 2 steps choose among
     * the 1/16 and 1/32 rolls and STRETCH; longer fills among the other keys. */
    if (ctx->ctl_auto) {
        unsigned length = 8U >> rnd_below(ctx, 3), key;
        if (length == 2U) {
            key = rnd_below(ctx, 9);
            key = key == 8U ? STRETCH_KEY : 8U + key;
        } else key = rnd_below(ctx, 12);
        ctx->fill_key = (uint8_t)(1U + key);
        ctx->fill_start = (uint16_t)((JUNGLE_STEPS - length) * JUNGLE_TICKS);
    }
    ctx->pattern_ready = 1;
}

/* A slice pitched up runs fast into the next one, so it stops after
 * `ticks`; a stretch slice runs at half speed and needs no cut. */
static JUNGLE_AUDIO_CODE unsigned pitch_gate(jungle_state_t *ctx, int pitch, unsigned stretch, unsigned ticks)
{
    return pitch > 0 && !stretch ? ticks : 0U;
}

static JUNGLE_AUDIO_CODE void trigger(jungle_state_t *ctx, unsigned slice, int pitch, int32_t gain, unsigned gate, int32_t tone, unsigned stretch)
{
    pitch += ctx->ctl_transpose;
    if (pitch < -24) pitch = -24;
    if (pitch > 24) pitch = 24;
    stretch |= ctx->ctl_stretch_hold | ctx->stretch_all;
    uint32_t tempo = ctx->ctl_speed; /* Q24 */
    /* Two voices: the new one fades in while the old one fades out. */
    struct jungle_voice *v = !ctx->voices[0].active ? &ctx->voices[0] : !ctx->voices[1].active ? &ctx->voices[1] :
                      ctx->voices[0].env <= ctx->voices[1].env ? &ctx->voices[0] : &ctx->voices[1];
    for (unsigned i = 0; i < 2; ++i)
        if (ctx->voices[i].active) ctx->voices[i].releasing = 1;
    uint32_t pitched = (uint32_t)(((uint64_t)tempo * pitch_q16[pitch + 24]) >> 16);
    v->rate = stretch ? (uint32_t)(((uint64_t)pitched * ctx->ctl_stretch_ratio) >> 16) : pitched;
    v->grain_rate = stretch ? pitched : 0U;
    v->grain_hop = ctx->ctl_grain_hop;
    v->grain_fade = ctx->ctl_grain_fade;
    v->grain_step = ctx->ctl_grain_step;
    v->grain_age = 0;
    v->rising = 1;
    /* The tick fell between samples: start where the slice would be by now. */
    uint32_t ahead = (uint32_t)(((uint64_t)ctx->tick_late * v->rate) >> 16);
    v->pos = slice * ctx->src_slice + (ahead >> 24);
    v->frac = ahead & 0xffffffU;
    for (unsigned r = 0; r < 2; ++r) {
        struct jungle_reader *g = &v->read[r];
        g->pos = v->pos;
        g->frac = v->frac;
        g->cached[0] = g->cached[1] = g->state_group = NO_GROUP;
    }
    v->env = 0;
    v->gain = stretch ? (gain * ctx->ctl_stretch_gain) >> 15 : gain;
    v->tone = tone;
    v->low = 0;
    v->gate = (uint16_t)gate;
    v->source = ctx->src_data;
    v->active = 1;
    v->releasing = 0;
#ifdef JUNGLE_TRACE
    jungle_host_trigger(jungle_host_frame, slice, pitch, gain);
#endif
}

static JUNGLE_AUDIO_CODE unsigned auto_request(jungle_state_t *ctx)
{
    return ctx->ctl_auto && ctx->fill_key && ctx->bar_tick >= ctx->fill_start ? ctx->fill_key : 0U;
}

static JUNGLE_AUDIO_CODE void fire_roll(jungle_state_t *ctx)
{
    unsigned key = (unsigned)(ctx->roll_now - 1U), variant = rolls[key].kind;
    unsigned every = rolls[key].ticks, gate = 0, n = ctx->roll_count < 12 ? ctx->roll_count : 12;
    /* Build: a beat each of 1/4, 1/8 and 1/16, then 1/32 until release. */
    if (variant == BUILD)
        every = ctx->roll_elapsed < BEAT ? BEAT : ctx->roll_elapsed < 2 * BEAT ? BEAT / 2 :
                ctx->roll_elapsed < 3 * BEAT ? BEAT / 4 : BEAT / 8;
    int pitch = ctx->roll_pitch;
    int32_t gain = FULL, tone = FULL;
    if (variant == DOWN) pitch -= 2 * (int)n;
    else if (variant == UP) pitch += 2 * (int)n;
    else if (variant == SHRINK) {
        gain = ctx->roll_gain;
        ctx->roll_gain = (ctx->roll_gain * 27853) >> 15; /* -1.4 dB per repeat */
        gate = (every * (8U - (n < 7 ? n : 7U))) >> 3;
        if (!gate) gate = 1;
    } else if (variant == ECHO) {
        /* -2.5 dB and a lower cutoff per repeat: ~5 kHz, 2 kHz, 900 Hz ... */
        gain = ctx->roll_gain;
        tone = ctx->roll_tone;
        ctx->roll_gain = (ctx->roll_gain * 24576) >> 15;
        ctx->roll_tone = (ctx->roll_tone * 22938) >> 15;
    }
    if (!gate) gate = pitch_gate(ctx, pitch, 0, every);
    trigger(ctx, ctx->roll_slice, pitch, gain, gate, tone, 0);
    ++ctx->roll_count;
    ctx->roll_wait = (uint16_t)every;
}

static JUNGLE_AUDIO_CODE void on_tick(jungle_state_t *ctx)
{
    unsigned step = ctx->bar_tick / JUNGLE_TICKS, offset = ctx->bar_tick % JUNGLE_TICKS;

    for (unsigned i = 0; i < 2; ++i) {
        struct jungle_voice *v = &ctx->voices[i];
        if (v->active && v->gate && --v->gate == 0) v->releasing = 1;
    }
    if (ctx->bar_tick == 0 && (!ctx->ctl_freeze || !ctx->pattern_ready)) generate(ctx);
    if (offset == 0) {
        uint8_t request = ctx->ctl_straight;
        if (request != ctx->seen_straight) { ctx->seen_straight = request; straight_pattern(ctx); }
        ctx->view_step = (int8_t)step;
    }

    unsigned seq_fire = offset == 0 && !ctx->six_chop_mode && ctx->six_seq_gates;
    const struct jungle_step *s = &ctx->steps[step];

    /* Manual rolls win over auto-rolls. A new roll waits for the next 16th
     * (32nd for 1/32 rolls); until then the previous roll or the pattern
     * keeps playing. Switching keys keeps the repeated slice. */
    unsigned request = ctx->ctl_roll;
    if (!request) request = ctx->six_seq_roll;
    if (!request && !ctx->six_chop_mode && ctx->six_seq_gates) request = auto_request(ctx);
    if (!request) {
        /* Released: carry on to the pattern's next hit, or the next 8th. */
        if (ctx->roll_now && (ctx->six_chop_mode || (seq_fire && s->active) || (offset == 0 && !(step & 1U)))) ctx->roll_now = 0;
    } else if (request != ctx->roll_now) {
        unsigned on_grid = rolls[request - 1U].ticks <= JUNGLE_TICKS / 2 ? ctx->bar_tick % (JUNGLE_TICKS / 2) == 0 : offset == 0;
        if (on_grid) {
            if (!ctx->roll_now) {
                unsigned here = seq_fire && s->active;
                ctx->roll_slice = here ? s->slice : ctx->last_slice;
                ctx->roll_pitch = here ? s->pitch : ctx->last_pitch;
            }
            ctx->roll_now = (uint8_t)request;
            ctx->roll_wait = 0;
            ctx->roll_count = 0;
            ctx->roll_elapsed = 0;
            ctx->roll_gain = ctx->roll_tone = FULL;
        }
    }
    ctx->stretch_all = ctx->roll_now && rolls[ctx->roll_now - 1U].kind == STRETCH_ALL;
    if (ctx->roll_now && !ctx->stretch_all) {
        if (ctx->roll_wait == 0) fire_roll(ctx);
        --ctx->roll_wait;
        if (ctx->roll_elapsed < 0xffffU) ++ctx->roll_elapsed;
    } else if (seq_fire && s->active) {
        /* With STRETCH held the pattern itself plays, every hit stretched. */
        trigger(ctx, s->slice, s->pitch, FULL, pitch_gate(ctx, s->pitch, s->stretch | ctx->ctl_stretch_hold | ctx->stretch_all, JUNGLE_TICKS), FULL, s->stretch);
        ctx->last_slice = s->slice;
        ctx->last_pitch = s->pitch;
    }
    ctx->view_roll = (int8_t)(ctx->roll_now ? ctx->roll_now - 1 : -1);

    if (ctx->shot_pending >= 0 && ctx->bar_tick % SHOT_TICKS == 0) {
        trigger(ctx, (unsigned)ctx->shot_pending, 0, FULL, 0, FULL, 0);
        ctx->last_slice = (uint8_t)ctx->shot_pending;
        ctx->last_pitch = 0;
        ctx->shot_pending = -1;
    }
    if (++ctx->bar_tick >= BAR_TICKS) ctx->bar_tick = 0;
    if (!ctx->ext && !(ctx->run_ticks++ & 7U)) jungle_midi_out(0xf8U); /* 24 per beat */
}

/* Interpolated source at the reader, then a step forward; silent past the end. */
static JUNGLE_AUDIO_CODE int32_t read_step(struct jungle_reader *r, const uint8_t *data, uint32_t frames, uint32_t rate)
{
    int32_t s = 0;
    if (r->pos + 1U < frames) {
        /* Decode adjacent packed PCM12 frames together. The bounds above
         * guarantee both exist; no modulo, duplicate address or shared-byte
         * fetch is needed, including at the final pair of the bank. */
        const uint8_t *p = data + (r->pos >> 1) * 3U;
        unsigned a12, b12;
        if (r->pos & 1U) {
            a12 = (p[1] >> 4) | ((unsigned)p[2] << 4);
            b12 = p[3] | ((unsigned)(p[4] & 15U) << 8);
        } else {
            a12 = p[0] | ((unsigned)(p[1] & 15U) << 8);
            b12 = (p[1] >> 4) | ((unsigned)p[2] << 4);
        }
        int32_t a = ((int32_t)(a12 ^ 2048U) - 2048) * 16;
        int32_t b = ((int32_t)(b12 ^ 2048U) - 2048) * 16;
        s = a + (((b - a) * (int32_t)(r->frac >> 9)) >> 15);
    }
    r->frac += rate;
    r->pos += r->frac >> 24;
    r->frac &= 0xffffffU;
    return s;
}

static JUNGLE_AUDIO_CODE int32_t render_voice(jungle_state_t *ctx, struct jungle_voice *v, const uint8_t *data, uint32_t frames)
{
    if (v->releasing) {
        v->env -= FULL / JUNGLE_FADE_OUT;
        if (v->env <= 0) { v->active = 0; return 0; }
    } else if (v->env < FULL) {
        v->env += FULL / JUNGLE_FADE_IN;
        if (v->env > FULL) v->env = FULL;
    }
    if (v->pos + 1U >= frames) { v->active = 0; return 0; }
    uint32_t left = frames - 1U - v->pos;
    if (!v->releasing && left <= ((JUNGLE_FADE_OUT * v->rate) >> 24) + 1U) v->releasing = 1;
    int32_t s;
    if (!v->grain_rate) {
        /* Turntable: read at the timeline. */
        v->read[0].pos = v->pos;
        v->read[0].frac = v->frac;
        s = read_step(&v->read[0], data, frames, 0);
    } else {
        /* Stretch: the newest grain alone, after the splice crossfade. */
        s = read_step(&v->read[v->rising], data, frames, v->grain_rate);
        if (v->grain_age < v->grain_fade) {
            uint32_t w = v->grain_age * v->grain_step;
            int32_t b = read_step(&v->read[v->rising ^ 1U], data, frames, v->grain_rate);
            s = (int32_t)(((int64_t)s * w + (int64_t)b * (ONE_Q24 - w)) >> 24);
        }
    }
    if (v->tone < FULL) {
        v->low += ((s - v->low) * v->tone + 16384) >> 15; /* rounded: no stuck offset */
        s = v->low;
    }
    s = (((s * v->env) >> 15) * v->gain) >> 15;
    v->frac += v->rate;
    v->pos += v->frac >> 24;
    v->frac &= 0xffffffU;
    if (v->grain_rate && ++v->grain_age >= v->grain_hop) {
        /* The older grain is done: it restarts, rising, at the timeline
         * (after this sample's step); the current one fades out. */
        struct jungle_reader *done = &v->read[v->rising ^ 1U];
        v->grain_age = 0;
        v->rising ^= 1U;
        done->pos = v->pos;
        done->frac = v->frac;
    }
    return s;
}

JUNGLE_AUDIO_CODE void jungle_render(jungle_state_t *ctx, int32_t *out, unsigned frames)
{
    const uint8_t *data = ctx->src_data;
    uint32_t frames_bound = ctx->src_frames;
    uint8_t generation = ctx->src_generation;
    if (!data) ctx->voices[0].active = ctx->voices[1].active = 0;
    else if (generation != ctx->seen_generation) {
        for(unsigned k=0;k<2;k++)if(ctx->voices[k].active)ctx->voices[k].releasing=1;
    }
    ctx->seen_generation = generation;
    /* Rhythmer's sequencer supplies the exact per-sample tick mask. MIDI
     * Start/Continue/PLL already belong to that sequencer: do not run a second
     * transport/clock follower inside the break renderer. */
    unsigned playing = data && ctx->ctl_playing;
    ctx->view_playing = (uint8_t)playing;
    ctx->view_ext = 0;
    uint32_t increment = (uint32_t)ctx->ctl_bpm * 4U * JUNGLE_TICKS;
    if (playing != ctx->was_playing) {
        if (!ctx->ext) jungle_midi_out(playing ? 0xfaU : 0xfcU);
        ctx->run_ticks = 0;
        ctx->was_playing = (uint8_t)playing;
        for (unsigned i = 0; i < 2; ++i) if (ctx->voices[i].active) ctx->voices[i].releasing = 1;
        ctx->roll_now = 0;
        ctx->view_roll = -1;
        ctx->view_step = -1;
        if (playing) {
            ctx->rng ^= ctx->ctl_seed * 2654435761U;
            if (!ctx->rng) ctx->rng = 1;
            ctx->bar_tick = ctx->six_start_tick;ctx->six_start_tick=0;
            ctx->pattern_ready = 0;
            if(ctx->bar_tick)generate(ctx);
            ctx->tick_acc = TICK_THRESHOLD - increment;
        }
    }
    ctx->view_rate = increment;
    uint16_t shot = ctx->ctl_shot;
    if (shot != ctx->seen_shot && data) {
        ctx->seen_shot = shot;
        if (playing) ctx->shot_pending = (int8_t)(shot & 15U);
        else {
            ctx->tick_late = 0;
            trigger(ctx, shot & 15U, 0, FULL, 0, FULL, 0);
            ctx->last_slice = (uint8_t)(shot & 15U);
        }
    }
    if(ctx->six_seq_release){
        if(!ctx->ctl_roll&&!ctx->six_seq_roll){ctx->roll_now=0;ctx->view_roll=-1;for(unsigned k=0;k<2;k++)ctx->voices[k].releasing=1;}
        ctx->six_seq_release=0;
    }
    if(ctx->six_seq_new&&data){
        ctx->six_seq_new=0;
        if(!ctx->ctl_roll){
            ctx->roll_now=ctx->six_seq_roll;ctx->roll_slice=ctx->last_slice;ctx->roll_pitch=ctx->last_pitch;
            ctx->roll_wait=ctx->roll_count=ctx->roll_elapsed=0;ctx->roll_gain=ctx->roll_tone=FULL;
            ctx->stretch_all=ctx->roll_now&&rolls[ctx->roll_now-1].kind==STRETCH_ALL;
            ctx->view_roll=(int8_t)(ctx->roll_now?ctx->roll_now-1:-1);
            if(ctx->stretch_all)trigger(ctx, ctx->last_slice,0,FULL,0,FULL,1);
        }
    }
    if(ctx->six_seq_hit>=0&&data){
        unsigned slice=(unsigned)ctx->six_seq_hit;ctx->six_seq_hit=-1;
        unsigned held=ctx->ctl_roll?ctx->ctl_roll:ctx->six_seq_roll;
        unsigned stretch=held&&rolls[held-1].kind==STRETCH_ALL;
        if(!held||stretch){
            if(!held){ctx->roll_now=0;ctx->view_roll=-1;ctx->stretch_all=0;}
            ctx->tick_late=0;trigger(ctx, slice,0,FULL,0,FULL,stretch);
        }
        ctx->last_slice=(uint8_t)slice;ctx->last_pitch=0;
    }
    /* Glide the cutoff a quarter of the way per block: no zipper steps. */
    unsigned filter = ctx->ctl_filter;
    int32_t filter_damp = ctx->ctl_filter_damp;
    unsigned filter_start = ctx->filter_was_open && filter != FILTER_OPEN;
    unsigned filter_fade_step = filter_start && frames ? 32768U / frames : 0;
    if (filter_start) ctx->filter_low = ctx->filter_band = 0;
    if (filter != FILTER_OPEN)
        ctx->filter_coefficient = ctx->filter_was_open ? (int32_t)filter_f[filter] :
            ctx->filter_coefficient + ((int32_t)filter_f[filter] - ctx->filter_coefficient) / 4;
    ctx->filter_was_open = filter == FILTER_OPEN;
    ctx->comb_fb += (ctx->ctl_comb_fb - ctx->comb_fb) / 4;
    ctx->comb_gain += (ctx->ctl_comb_gain - ctx->comb_gain) / 4;
    uint32_t comb_delay = ctx->ctl_comb_delay;
    /* Only exact digital silence can skip sample DSP. Advance transport,
     * pattern RNG and ring position normally; no tail threshold or fades. */
#ifndef JUNGLE_NO_IDLE_FASTPATH
    if(!ctx->voices[0].active&&!ctx->voices[1].active&&!ctx->comb_left&&!ctx->filter_low&&!ctx->filter_band&&
       !ctx->six_seq_gates&&!ctx->ctl_roll&&!ctx->six_seq_roll&&!ctx->roll_now&&ctx->shot_pending<0){
        for(unsigned i=0;i<frames;i++)if(playing&&i<32U&&((ctx->six_tick_mask>>i)&1U)){ctx->tick_late=0;on_tick(ctx);}
        ctx->comb_write=(ctx->comb_write+frames)&(COMB_SIZE-1);
        ctx->frame_count+=frames;
#ifdef JUNGLE_TRACE
        jungle_host_frame+=frames;
#endif
        return;
    }
#endif
    for (unsigned i = 0; i < frames; ++i) {
        /* SIX owns the only transport. At most one 192 PPQN tick per sample.
         * The caller supplies exactly 32 frames; no independent MIDI PLL. */
        if (playing && i < 32U && ((ctx->six_tick_mask >> i) & 1U)) {
            ctx->tick_late = 0;
            on_tick(ctx);
        }
        int32_t mix = 0;
        for (unsigned k = 0; k < 2; ++k)
            if (ctx->voices[k].active) mix += render_voice(ctx, &ctx->voices[k], ctx->voices[k].source, frames_bound);
        /* Comb. The line always runs, so raising the feedback starts clean. */
        int32_t y = mix;
        if(ctx->comb_fb){
            const uint8_t *p=ctx->comb+3u*((ctx->comb_write-comb_delay)&(COMB_SIZE-1));
            uint32_t bits=(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16;
            int32_t delayed=(int32_t)(bits<<8)>>8;
            y += (int32_t)(((int64_t)delayed * ctx->comb_fb) >> 15);
        }
        y = y > (1 << 20) ? 1 << 20 : y < -(1 << 20) ? -(1 << 20) : y;
        if(y)ctx->comb_left=COMB_SIZE;else if(ctx->comb_left)ctx->comb_left--;
        /* The existing clamp fits signed 24 bits exactly: no quantization or lost feedback detail. */
        uint8_t *p=ctx->comb+3u*ctx->comb_write;
        p[0]=(uint8_t)y;p[1]=(uint8_t)((uint32_t)y>>8);p[2]=(uint8_t)((uint32_t)y>>16);
        ctx->comb_write = (ctx->comb_write + 1U) & (COMB_SIZE - 1);
        mix = (int32_t)(((int64_t)y * ctx->comb_gain) >> 15);
        /* Half level inside the filter; 64-bit products, since feedback
         * and resonance can all lift the band state. */
        int32_t x = mix >> 1;
        /* OPEN is a true bypass. The track's separate filter/ENV/LFO still
         * runs downstream; six wide multiplies per frame here did no audible
         * work when this Amenity performance filter was open. */
        for (unsigned k = 0; filter != FILTER_OPEN && k < 2; ++k) {
            ctx->filter_low += (int32_t)(((int64_t)ctx->filter_coefficient * ctx->filter_band + 16384) >> 15);
            int32_t high = x - ctx->filter_low - (int32_t)(((int64_t)filter_damp * ctx->filter_band + 8192) >> 14);
            ctx->filter_band += (int32_t)(((int64_t)ctx->filter_coefficient * high + 16384) >> 15);
            if (k) x = filter < FILTER_OPEN ? ctx->filter_low : filter > FILTER_OPEN ? high : x;
        }
        /* Start a newly closed filter gently instead of using stale state.
         * First block fades from dry to the newly initialized filter. */
        if (filter_start) x = (mix >> 1) + (int32_t)(((int64_t)(x - (mix >> 1)) * ((i + 1U) * filter_fade_step)) >> 15);
        mix = x * 2;
        out[2 * i] += mix * 128;
        out[2 * i + 1] += mix * 128;
#ifdef JUNGLE_TRACE
        ++jungle_host_frame;
#endif
    }
    ctx->frame_count += frames;
}
