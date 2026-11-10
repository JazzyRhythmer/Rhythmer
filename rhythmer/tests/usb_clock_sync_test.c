/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#ifdef TEST_JAZZY
#include "jazzy_host.h"
#else
#include "ui_test.c"
#endif

static void clock_setup(uint32_t source)
{
    ui_power_on();
    fm6_init();
    power_on_parts();
    song.g[G_CLOCK] = (int16_t)source;
    song.g[G_BPM] = 120;
    song.g[G_SWING] = 0;
    trk[0].p[P_SLEN] = 16;
    trk[0].p[P_SDIV] = 2;
    trk[0].p[P_SSWING] = 0;
    for (uint32_t i = 0; i < 16; i++)
        trk[0].step[i] = (step_t){{(uint8_t)(60 + i)}, 1, ST_NOTE, 0, 100};
    events_block(CTL);
}

static void clock_packet(uint32_t source, uint32_t status, uint32_t ms)
{
    fm1_ms = ms;
    midi_enqueue(15u | status << 8, source);
    events_block(CTL);
}

/* Follow a 120 BPM sender under deterministic USB arrival jitter.
 * Count actual sequencer steps and measure their phase, not just the LCD BPM. */
static int jitter_clock_test(void)
{
    clock_setup(1);
    clock_packet(1, 0xFA, 100);
    uint32_t rng = 0xcafeu, previous = trk[0].seq_idx, steps = 0, worst_ms = 0;
    uint32_t lo = 240, hi = 0, now = 100;
    for (uint32_t pulse = 0; pulse <= 24u * 80u + 1u; ++pulse) {
        rng = rng * 1664525u + 1013904223u;
        int32_t jitter = pulse ? (int32_t)((rng >> 24) % 7u) - 3 : 0;
        uint32_t at = 100u + (pulse * 500u + 12u) / 24u + jitter;
        for (; now <= at; ++now) {
            if (now == at)
                clock_packet(1, 0xF8, now);
            else {
                fm1_ms = now;
                events_block(CTL);
            }
            if (trk[0].seq_idx != previous) {
                previous = trk[0].seq_idx;
                ++steps;
                uint32_t expected = 100u + steps * 125u;
                uint32_t error = now >= expected ? now - expected : expected - now;
                if (error > worst_ms) worst_ms = error;
            }
        }
        if (pulse >= 48u) {
            uint32_t bpm = (uint32_t)song.g[G_BPM];
            if (bpm < lo) lo = bpm;
            if (bpm > hi) hi = bpm;
        }
    }
    printf("USB jitter: %u–%u BPM, %u steps, worst phase %u ms\n", lo, hi, steps, worst_ms);
    int bad = check("USB jitter keeps BPM near 120", lo >= 119u && hi <= 121u);
    bad += check("USB follows all 320 steps within 12 ms", steps == 320u && worst_ms <= 12u);
    /* A deliberate change from 120 to 100 BPM must settle promptly. */
    uint32_t at = fm1_ms;
    for (uint32_t i = 1; i <= 48u; ++i) clock_packet(1, 0xF8, at + i * 25u);
    printf("USB tempo change after two beats: %d BPM\n", song.g[G_BPM]);
    bad += check("USB follows a deliberate tempo change", song.g[G_BPM] == 100 && song.playing);
    clock_packet(1, 0xFCu, fm1_ms+1u);
    bad += check("USB Stop releases the sequencer", !song.playing);
    return bad;
}
int main(void) { return jitter_clock_test() != 0; }
