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

void jungle_init(void);
/* SIX adaptation: 72000 bytes signed packed PCM12, 48000 mono frames at 16 kHz.
 * Two 160 BPM 4/4 bars.
 * Runtime owner only, before dispatch. NULL unbinds; returns -1 when unbound/invalid. */
int jungle_bind(const uint8_t *sea, unsigned bytes, unsigned frames);
/* SIX: master 192 PPQN ticks at sample offsets in the next 32-frame block. */
void jungle_tick_mask(uint32_t mask);
/* The built-in break (jungle_fallback.c): 1 with its data, or 0 when the
 * firmware was built without the sample. */
int jungle_fallback(const uint8_t **data, unsigned *bytes, unsigned *frames);
/* The packed PCM12 data the engine plays now, or NULL. */
const uint8_t *jungle_bound(void);
/* Tempo of the bound loop at its own speed if it is a 32 kHz bank slot;
 * 0 when nothing is bound. (Playback speed follows the frame count and
 * needs no sample rate: the loop is always two bars.) */
unsigned jungle_native_bpm_x10(void);
/* Control task. Stopping releases the sound; starting begins at step 1. */
void jungle_play(unsigned on);
/* SIX: join the transport at a 192-PPQN position on the next start. */
void jungle_start_phase(unsigned tick);
/* Mixed into the random generator at the next start (e.g. a millisecond clock). */
void jungle_seed(uint32_t seed);
void jungle_tempo(unsigned bpm_x10);
/* 0..127. Variation and the SEQ fill apply from the next pattern. */
/* Transpose everything, -12..+12 semitones, from the next hit. */
void jungle_pitch(int semitones);
/* STRETCH: a hit stretched slower at its own pitch, the way an old sampler's
 * time-stretch did it: short pieces of the slice, each played `slow` times
 * (2..4), spliced with a short crossfade. The repeats ring metallic.
 * WARP amount 0..127: the chance per 8th that the pattern warps a hit, from
 * the next pattern (never on the first four 8ths): two in three become
 * STRETCH, one in three plays at half speed, an octave down. Hold makes every
 * hit stretch, rolls and one-shots too. Grain: the piece length in ms (3..80)
 * and the splice crossfade in tenths of a ms (1..300, at most the piece). */
void jungle_warp(unsigned amount);
void jungle_stretch_hold(unsigned on);
void jungle_stretch_grain(unsigned piece_ms, unsigned fade_tenths);
void jungle_stretch_slow(unsigned times);
/* Short feedback delay (a comb) on the whole break, before the DJ filter:
 * it rings metallic at the delay's pitch. Feedback 0..127 is off to 90%;
 * the average level stays put as it rises. The pitch is a root note, 0..11
 * from C, in the octave C2..B2 (delays 15.3..8.1 ms); E by default.
 * jungle_feedback_delay sets any delay in tenths of a ms (1..230). */
void jungle_feedback(unsigned amount);
/* Tone shaping uses track filters, ENV/LFO and shared effects. */
void jungle_feedback_note(unsigned pitch_class);
void jungle_feedback_delay(unsigned tenths);
/* DJ filter, 0..127: below 64 low-pass (150 Hz at 0), 64 open, above 64
 * high-pass (4 kHz at 127). jungle_filter_hz gives the cutoff for a value. */
void jungle_filter(unsigned value);
unsigned jungle_filter_hz(unsigned value);
/* The DJ filter's Q in tenths (5..30); 8 (flat) by default. */
void jungle_filter_resonance(unsigned q_tenths);
void jungle_variation(unsigned rhythm);
/* SEQ fill: each phrase ends with one white-key effect chosen at random,
 * from 8, 4 or 2 steps before the end (1/32 rolls only in the last 2). */
void jungle_auto_roll(unsigned on);
/* Freeze keeps the current pattern instead of generating a new one. */
void jungle_freeze(unsigned on);
/* Replace the pattern with the straight break at the next step. */
void jungle_straight(void);
/* Hold a roll key as above. Release with jungle_roll_off. A new roll starts
 * on the next 16th (32nd for 1/32 rolls) and repeats the slice that was
 * playing; the pattern keeps running underneath. After a release the roll
 * goes on until the pattern's next hit (at most the next 8th), so letting
 * go a little early never leaves a gap. */
void jungle_roll(unsigned key);
void jungle_roll_off(void);
/* Play one slice on the next 32nd. */
void jungle_one_shot(unsigned slice);
/* SIX: 0..15 slice, 16..31 performance command; sequence gate and manual hold are independent. */
void jungle_swap(const uint8_t *data);
void jungle_sequence_mode(unsigned chop);
void jungle_sequence_note(unsigned command);
void jungle_sequence_off(unsigned command);

/* MIDI clock in: the MIDI routing task passes every realtime status here
 * (F8 clock, FA start, FB continue, FC stop). After a Start or Continue the
 * break follows the clock: 24 pulses a beat, the first pulse after Start on
 * step 1; a second without pulses stops it and hands back to the internal
 * clock. While it follows, jungle_clock reports 1 and the tempo it has
 * locked to (BPM x 10), for the control task to show and to set the
 * break's speed with (jungle_tempo). */
void jungle_midi_realtime(unsigned status);
int jungle_clock(unsigned *bpm_x10);
/* MIDI clock out, the other way round: while the internal clock plays, the
 * audio interrupt sends Start, 24 pulses a beat and Stop through this
 * (app/midi_router.c queues them for the MIDI outputs). */
void jungle_midi_out(unsigned status);

/* Views for the control task. */
int jungle_playing(void);
int jungle_current_step(void);        /* -1 while stopped */
int jungle_roll_active(void);         /* roll key while sounding, else -1 */
void jungle_pattern(struct jungle_step out[JUNGLE_STEPS]);

/* Audio interrupt: add frames of interleaved stereo Q23 to out. */
JUNGLE_AUDIO_CODE void jungle_render(int32_t *out, unsigned frames);
#endif
