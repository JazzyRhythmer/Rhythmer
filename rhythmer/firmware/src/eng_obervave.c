/* SPDX-License-Identifier: GPL-3.0-only
 * OberVave DSP by zvenson, Jazzy integration at upstream 792535d23ef4265475b1c2f9895ca30e857fe596.
 * Rhythmer reuses its voice phase/state, amp envelope, modulation and effects.
 */
#include "../ports/jo_dsp.h"
#include "../ports/obervave_programs.h"

static void obervave_note_on(track_t *t, voice_t *v)
{
    (void)t;
    v->ph[0] = v->ph[1] = 0;
    memset(v->s, 0, sizeof v->s);
}

static void obervave_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p + P_E0;
    int32_t cut = clamp(p[0] * 256 + m->cutoff, 0, 127 * 256);
    int32_t res = clamp(p[1], 0, 120);
    jo_svf_coef_t filter;
    jo_svf_coef(&filter, 25 * 256 + cut * 100 / 127, 8192 - res * 7600 / 127);

    uint32_t inc2 = cents_inc(clamp(m->pitch16 + p[7] * 16, 0, 2047), p[3], m->fine);
    uint32_t pw = 0x10000000u + (uint32_t)clamp(p[4] + (m->shape - 16384) / 258, 0, 127) * 0x1a00000u;
    uint32_t wave = p[2] < 43 ? jo_OSC_SAW : p[2] < 86 ? jo_OSC_SAW | jo_OSC_PLS : jo_OSC_PLS;
    jo_osc_t osc_a = {v->ph[0], v->s[0]}, osc_b = {v->ph[1], v->s[1]};
    jo_svf_state_t first = {v->s[2], v->s[3]}, second = {v->s[4], v->s[5]};

    if (n > CTL)
        n = CTL;
    for (unsigned i = 0; i < n; ++i) {
        int32_t a = jo_osc_step(&osc_a, m->inc, (m->inc >> 15) | 1u, pw, wave, 0, -1);
        int32_t b = jo_osc_step(&osc_b, inc2, (inc2 >> 15) | 1u, 0x80000000u, jo_OSC_SAW, 0, -1);
        int32_t lp, bp, hp;
        jo_svf_step(&filter, &first, (a + b) / 6, &lp, &bp, &hp);
        int32_t y = p[5] < 43 ? lp : p[5] < 86 ? bp : hp;
        if (p[6] > 63)
            y = jo_svf_lp(&filter, &second, y);
        /* Compensate the oscillator mix headroom without driving the filter. */
        int32_t sample = voice_amp(y, m, i);
        out[i] += (sample * 3) >> 1;
    }

    v->ph[0] = osc_a.ph;
    v->ph[1] = osc_b.ph;
    v->s[0] = osc_a.prev;
    v->s[1] = osc_b.prev;
    v->s[2] = first.ic1;
    v->s[3] = first.ic2;
    v->s[4] = second.ic1;
    v->s[5] = second.ic2;
}

#define OB_PARAM(name, lo, hi, def) {name, F_INT, lo, hi, def, 0, 0}
static const engine_t ENG_OBERVAVE = {
    .name = "OBERVAVE",
    .page_title = {"FILTER / OSC", "VOICE"},
    .edit = {OB_PARAM("CUTOFF",0,127,80), OB_PARAM("RESO",0,127,20),
             OB_PARAM("SAW/PULSE",0,127,48), OB_PARAM("FINE B",-64,63,6),
             OB_PARAM("WIDTH",0,127,64), OB_PARAM("LP/BP/HP",0,127,0),
             OB_PARAM("4-POLE",0,127,0), OB_PARAM("SEMI B",-24,24,0)},
    .presets = OBERVAVE_PROGRAMS,
    .npresets = NELEM(OBERVAVE_PROGRAMS),
    .knob = {P_E0,P_E1,P_E2,P_E3},
    .poly = 4,
    .note_on = obervave_note_on,
    .render = obervave_render,
};
#undef OB_PARAM
