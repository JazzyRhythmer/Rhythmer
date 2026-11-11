/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception */
/* JUNGLE: live amen break slicer, sequencer and roll engine.
 * The sound engine of AMENITY, a port of the Jungle Choppa VST plugin. Integer-only; renders in the APP
 * output stage. Control functions belong to the control task and only write
 * single words, which the audio interrupt picks up at its next block. */
#ifndef JUNGLE_H
#define JUNGLE_H
#include <stdint.h>

#ifndef JUNGLE_AUDIO_CODE
#ifdef FM1_HOST_TEST
#define JUNGLE_AUDIO_CODE
#else
#include "fm1.h"
#define JUNGLE_AUDIO_CODE FM1_AUDIO_CODE
#endif
#endif

enum {
    JUNGLE_RATE = 44100,         /* output */
    JUNGLE_NATIVE_BPM_X10 = 1600, /* the amen; the real value follows the bound loop */
    JUNGLE_STEPS = 32,           /* 16ths over the two bars */
    JUNGLE_SLICES = 16,          /* 8ths, each plays to the end of the loop */
    JUNGLE_TICKS = 48,           /* clock ticks per 16th step */
    JUNGLE_BPM_MIN_X10 = 400,
    JUNGLE_BPM_MAX_X10 = 3200
};

/* Rolls, one per white key, in four groups of four:
 *   keys 0..3    1/4 repeat, build (1/4 -> 1/8 -> 1/16 -> 1/32, a beat
 *                each), STRETCH (no roll: every hit of the pattern plays
 *                stretched while held), echo (dotted 1/8, softer and darker
 *                on every repeat)
 *   keys 4..7    1/8  repeat, down, up, shrink
 *   keys 8..11   1/16 repeat, down, up, shrink
 *   keys 12..15  1/32 repeat, down, up, shrink
 * Repeat: same slice and pitch. Down/up: a whole tone per repeat.
 * Shrink: every repeat shorter and softer. */
enum { JUNGLE_ROLL_KEYS = 16 };

struct jungle_step { uint8_t active, slice, stretch; int8_t pitch; };

struct jungle_reader {
    uint32_t pos, frac;              /* grain position, Q24 fraction */
    uint32_t cached[2], state_group; /* decoded group per slot; group the state follows */
    int32_t state[8];
    int16_t pcm[2][20];
};
struct jungle_voice {
    const uint8_t *source; /* SIX: retain old loop during the 48-sample crossfade */
    uint32_t pos, frac, rate; /* timeline: source frame, Q24 fraction and step */
    uint32_t grain_rate;      /* stretch: grain step, Q24; 0 = read at the timeline */
    uint32_t grain_step;      /* crossfade rise per sample, Q24 */
    uint16_t grain_hop, grain_fade, grain_age;
    int32_t env, gain;        /* Q15 */
    int32_t tone, low;        /* one-pole low-pass: Q15 coefficient (FULL = open) and state */
    uint16_t gate;            /* ticks until release, 0 = play to the end */
    uint8_t active, releasing, rising; /* rising: the reader whose window goes up */
    struct jungle_reader read[2];
};
enum { JUNGLE_COMB_SIZE = 1024, JUNGLE_COMB_BYTES = JUNGLE_COMB_SIZE * 3 };
typedef struct jungle_state {
    volatile int32_t ctl_filter_damp;
    volatile uint16_t ctl_bpm;
    volatile uint32_t ctl_reciprocal;
    volatile uint8_t ctl_playing, ctl_rhythm, ctl_auto, ctl_freeze;
    uint8_t six_chop_mode, six_seq_roll;
    uint32_t six_seq_gates;
    int16_t six_seq_hit;
    uint8_t six_seq_release, six_seq_new;
    volatile uint8_t ctl_roll;
    volatile uint8_t ctl_straight;
    volatile uint16_t ctl_shot;
    volatile uint32_t ctl_seed;
    volatile int8_t ctl_transpose;
    volatile uint8_t ctl_filter;
    volatile uint8_t ctl_warp, ctl_stretch_hold;
    volatile uint32_t ctl_stretch_ratio;
    volatile int32_t ctl_stretch_gain;
    volatile uint16_t ctl_grain_hop, ctl_grain_fade;
    volatile uint32_t ctl_grain_step;
    volatile uint32_t src_frames, src_slice;
    volatile uint32_t ctl_speed;
    volatile uint8_t src_generation;
    volatile int8_t view_step, view_roll;
    struct jungle_voice voices[2];
    struct jungle_step steps[JUNGLE_STEPS];
    uint8_t fill_key;
    uint16_t fill_start;
    uint8_t stretch_all;
    uint32_t tick_acc, tick_late, rng;
    uint16_t bar_tick;
    uint8_t was_playing, pattern_ready, last_slice, seen_straight, seen_generation;
    volatile uint32_t clk_pulses, clk_mark;
    volatile uint8_t clk_starts, clk_continues, clk_stops;
    uint8_t ext, ext_playing, seen_starts, seen_continues, seen_stops;
    uint32_t pulse_base, ext_ticks, last_pulses, frame_count, last_pulse_frame;
    uint32_t lock_rate, lock_seen;
    int32_t lock_err;
    volatile uint32_t view_rate;
    uint32_t rough_base, rough_frame;
    uint8_t rough_state, rough_seen, rough_done;
    volatile uint32_t view_rough_span, ctl_rough_rate;
    volatile uint8_t view_rough, ctl_rough;
    volatile uint8_t view_ext, view_playing;
    unsigned six_start_tick;
    uint32_t run_ticks;
    int8_t last_pitch;
    uint16_t seen_shot;
    int8_t shot_pending;
    uint8_t roll_now, roll_slice;
    int8_t roll_pitch;
    uint16_t roll_wait, roll_count, roll_elapsed;
    int32_t roll_gain, roll_tone;
    int32_t filter_low, filter_band, filter_coefficient;
    uint8_t filter_was_open;
    uint8_t *comb;
    uint32_t comb_write, comb_left;
    int32_t comb_fb, comb_gain;
    volatile int32_t ctl_comb_fb, ctl_comb_gain;
    volatile uint16_t ctl_comb_delay;
    uint32_t six_tick_mask;
    const uint8_t *volatile src_data;
    uint32_t clock_acc, clock_ticks, keys, filter_age, filter_q_age;
    int16_t old[8];
    uint8_t roll_held[16], key_cmd[27];
    uint8_t held_count, run, loop_now, filter_q;
} jungle_state_t;

void jungle_init(jungle_state_t *ctx);
/* SIX adaptation: 72000 bytes signed packed PCM12, 48000 mono frames at 16 kHz.
 * Two 160 BPM 4/4 bars.
 * Runtime owner only, before dispatch. NULL unbinds; returns -1 when unbound/invalid. */
int jungle_bind(jungle_state_t *ctx, const uint8_t *sea, unsigned bytes, unsigned frames);
/* SIX: master 192 PPQN ticks at sample offsets in the next 32-frame block. */
void jungle_tick_mask(jungle_state_t *ctx, uint32_t mask);
/* The built-in break (jungle_fallback.c): 1 with its data, or 0 when the
 * firmware was built without the sample. */
int jungle_fallback(const uint8_t **data, unsigned *bytes, unsigned *frames);
/* The packed PCM12 data the engine plays now, or NULL. */
const uint8_t *jungle_bound(jungle_state_t *ctx);
/* Tempo of the bound loop at its own speed if it is a 32 kHz bank slot;
 * 0 when nothing is bound. (Playback speed follows the frame count and
 * needs no sample rate: the loop is always two bars.) */
unsigned jungle_native_bpm_x10(jungle_state_t *ctx);
/* Control task. Stopping releases the sound; starting begins at step 1. */
void jungle_play(jungle_state_t *ctx, unsigned on);
/* SIX: join the transport at a 192-PPQN position on the next start. */
void jungle_start_phase(jungle_state_t *ctx, unsigned tick);
/* Mixed into the random generator at the next start (e.g. a millisecond clock). */
void jungle_seed(jungle_state_t *ctx, uint32_t seed);
void jungle_tempo(jungle_state_t *ctx, unsigned bpm_x10);
/* 0..127. Variation and the SEQ fill apply from the next pattern. */
/* Transpose everything, -12..+12 semitones, from the next hit. */
void jungle_pitch(jungle_state_t *ctx, int semitones);
/* STRETCH: a hit stretched slower at its own pitch, the way an old sampler's
 * time-stretch did it: short pieces of the slice, each played `slow` times
 * (2..4), spliced with a short crossfade. The repeats ring metallic.
 * WARP amount 0..127: the chance per 8th that the pattern warps a hit, from
 * the next pattern (never on the first four 8ths): two in three become
 * STRETCH, one in three plays at half speed, an octave down. Hold makes every
 * hit stretch, rolls and one-shots too. Grain: the piece length in ms (3..80)
 * and the splice crossfade in tenths of a ms (1..300, at most the piece). */
void jungle_warp(jungle_state_t *ctx, unsigned amount);
void jungle_stretch_hold(jungle_state_t *ctx, unsigned on);
void jungle_stretch_grain(jungle_state_t *ctx, unsigned piece_ms, unsigned fade_tenths);
void jungle_stretch_slow(jungle_state_t *ctx, unsigned times);
/* Short feedback delay (a comb) on the whole break, before the DJ filter:
 * it rings metallic at the delay's pitch. Feedback 0..127 is off to 90%;
 * the average level stays put as it rises. The pitch is a root note, 0..11
 * from C, in the octave C2..B2 (delays 15.3..8.1 ms); E by default.
 * jungle_feedback_delay sets any delay in tenths of a ms (1..230). */
void jungle_feedback(jungle_state_t *ctx, unsigned amount);
/* Tone shaping uses track filters, ENV/LFO and shared effects. */
void jungle_feedback_note(jungle_state_t *ctx, unsigned pitch_class);
void jungle_feedback_delay(jungle_state_t *ctx, unsigned tenths);
/* DJ filter, 0..127: below 64 low-pass (150 Hz at 0), 64 open, above 64
 * high-pass (4 kHz at 127). jungle_filter_hz gives the cutoff for a value. */
void jungle_filter(jungle_state_t *ctx, unsigned value);
unsigned jungle_filter_hz(unsigned value);
/* The DJ filter's Q in tenths (5..30); 8 (flat) by default. */
void jungle_filter_resonance(jungle_state_t *ctx, unsigned q_tenths);
void jungle_variation(jungle_state_t *ctx, unsigned rhythm);
/* SEQ fill: each phrase ends with one white-key effect chosen at random,
 * from 8, 4 or 2 steps before the end (1/32 rolls only in the last 2). */
void jungle_auto_roll(jungle_state_t *ctx, unsigned on);
/* Freeze keeps the current pattern instead of generating a new one. */
void jungle_freeze(jungle_state_t *ctx, unsigned on);
/* Replace the pattern with the straight break at the next step. */
void jungle_straight(jungle_state_t *ctx);
/* Hold a roll key as above. Release with jungle_roll_off. A new roll starts
 * on the next 16th (32nd for 1/32 rolls) and repeats the slice that was
 * playing; the pattern keeps running underneath. After a release the roll
 * goes on until the pattern's next hit (at most the next 8th), so letting
 * go a little early never leaves a gap. */
void jungle_roll(jungle_state_t *ctx, unsigned key);
void jungle_roll_off(jungle_state_t *ctx);
/* Play one slice on the next 32nd. */
void jungle_one_shot(jungle_state_t *ctx, unsigned slice);
/* SIX: 0..15 slice, 16..31 performance command; sequence gate and manual hold are independent. */
void jungle_swap(jungle_state_t *ctx, const uint8_t *data);
void jungle_sequence_mode(jungle_state_t *ctx, unsigned chop);
void jungle_sequence_note(jungle_state_t *ctx, unsigned command);
void jungle_sequence_off(jungle_state_t *ctx, unsigned command);

/* MIDI clock in: the MIDI routing task passes every realtime status here
 * (F8 clock, FA start, FB continue, FC stop). After a Start or Continue the
 * break follows the clock: 24 pulses a beat, the first pulse after Start on
 * step 1; a second without pulses stops it and hands back to the internal
 * clock. While it follows, jungle_clock reports 1 and the tempo it has
 * locked to (BPM x 10), for the control task to show and to set the
 * break's speed with (jungle_tempo). */
void jungle_midi_realtime(jungle_state_t *ctx, unsigned status);
int jungle_clock(jungle_state_t *ctx, unsigned *bpm_x10);
/* MIDI clock out, the other way round: while the internal clock plays, the
 * audio interrupt sends Start, 24 pulses a beat and Stop through this
 * (app/midi_router.c queues them for the MIDI outputs). */
void jungle_midi_out(unsigned status);

/* Views for the control task. */
int jungle_playing(jungle_state_t *ctx);
int jungle_current_step(jungle_state_t *ctx);        /* -1 while stopped */
int jungle_roll_active(jungle_state_t *ctx);         /* roll key while sounding, else -1 */
void jungle_pattern(jungle_state_t *ctx, struct jungle_step out[JUNGLE_STEPS]);

/* Audio interrupt: add frames of interleaved stereo Q23 to out. */
JUNGLE_AUDIO_CODE void jungle_render(jungle_state_t *ctx, int32_t *out, unsigned frames);
#endif
