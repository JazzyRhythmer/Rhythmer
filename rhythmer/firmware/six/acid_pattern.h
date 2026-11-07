/* SPDX-License-Identifier: GPL-3.0-only */
/* Compact MIDI-only input for the TB-3PO generator. No drum machines,
 * break player, project bank, playback direction or audio engine state. */
#pragma once
#include <stdint.h>
#define NSTEPS 64
enum { G_REST, G_NOTE, G_TIE };                             /* 303 step gate */

/* 303 step: one byte of note, one of flags */
#define BS_GATE_MASK 0x03u
#define BS_ACCENT 0x04u
#define BS_SLIDE 0x08u                /* slide INTO the next step (303 semantics) */
typedef struct {
    uint8_t note;                     /* MIDI note */
    uint8_t flags;                    /* gate | BS_ACCENT | BS_SLIDE */
} acid_gen_step_t;

/* TB-3PO settings of one 303 line: the line is reproducible from these and the seed */
typedef struct {
    uint8_t density, accent, slide;   /* probabilities 0..100 % */
    uint8_t oct_range;                /* 1..3 */
    uint8_t root;                     /* 0 = C .. 11 = B */
    uint8_t base_oct;                 /* octave of the root: note = 12 * (base_oct + 1) + root */
    uint32_t seed;
} tb3po_cfg_t;

typedef struct {
    acid_gen_step_t step[NSTEPS];
    uint8_t len;                      /* 1..64 */
    tb3po_cfg_t gen;
} acid_gen_line_t;


static inline int bstep_gate(const acid_gen_step_t *s) { return s->flags & BS_GATE_MASK; }
