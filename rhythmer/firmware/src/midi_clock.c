/* SPDX-License-Identifier: GPL-3.0-only
 * Adapted from MIDI clock contributions by ChanceTheMaker and keremimo (2026).
 * Tempo smoothing and display hysteresis adapted from Melodee 3fe570d.
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio).
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Audio ISR state. Input timestamps keep USB/TRS jitter outside the render loop. */
static struct {
    uint32_t pos, rendered, last_ms, start_ms, rem, interval_ms;
    uint32_t pulse_samples, interp_q8, tempo_ms, tempo_q6;
    uint8_t mode, have_pulse, tempo_valid, tempo_n, tempo_count;
} midi_clock;

static __attribute__((noinline)) void midi_clock_transport(uint32_t status, uint32_t ms)
{
    if (status == 0xFAu) {                         /* Start: step zero */
        midi_clock.pos = midi_clock.rendered = midi_clock.rem = 0;
        midi_clock.have_pulse = midi_clock.tempo_valid = 0;
        midi_clock.interval_ms = 0;
        midi_clock.start_ms = ms;
        seq_start();
    } else if (status == 0xFBu) {                  /* Continue: preserve the step */
        midi_clock.pos = midi_clock.rendered;
        midi_clock.rem = 0;
        midi_clock.have_pulse = midi_clock.tempo_valid = 0;
        midi_clock.interval_ms = 0;
        midi_clock.start_ms = ms;
        if (!song.playing)
            motion_begin();
        song.playing = 1;
    } else if (status == 0xFCu) {
        seq_stop();
    }
}

static __attribute__((noinline)) uint32_t midi_clock_rescale(uint32_t position, uint32_t ratio)
{
    return (uint32_t)(((uint64_t)position * ratio + 2048u) >> 12);
}

static __attribute__((noinline)) void midi_clock_pulse(uint32_t ms)
{
    uint32_t q;
    if (!midi_clock.tempo_valid || ms - midi_clock.last_ms > 500u) {
        midi_clock.tempo_q6 = midi_clock.tempo_count = 0;
        midi_clock.tempo_valid = 1;
        midi_clock.tempo_ms = ms;
        midi_clock.tempo_n = 0;
    } else if (++midi_clock.tempo_n == 6u) {
        uint32_t dt = ms - midi_clock.tempo_ms;
        midi_clock.tempo_ms = ms;
        midi_clock.tempo_n = 0;
        /* Six clocks are a quarter of a beat. Reject gaps and corrupt bursts. */
        if (dt >= 60u && dt <= 377u) {
            /* Acquire on six pulses, average over about two beats, and
             * reacquire promptly after a genuine tempo change. */
            uint32_t first = !midi_clock.tempo_q6;
            int32_t delta = (int32_t)(dt * 64u) - (int32_t)midi_clock.tempo_q6;
            uint32_t change = (uint32_t)(delta < 0 ? -delta : delta);
            if (first || change > midi_clock.tempo_q6 / 16u) {
                midi_clock.tempo_q6 = dt * 64u;
                /* A window can straddle a tempo change. Acquire again on
                 * the next full window before blending later measurements. */
                midi_clock.tempo_count = 0;
            } else {
                if (midi_clock.tempo_count < 8u)
                    midi_clock.tempo_count++;
                midi_clock.tempo_q6 += delta / midi_clock.tempo_count;
            }
            uint32_t old_beat = beat_samples();
            uint32_t new_beat = (uint32_t)FS * midi_clock.tempo_q6 / 16000u;
            if (song.playing && new_beat != old_beat) {
                uint32_t i, ratio = (new_beat << 12) / old_beat;
                /* Preserve each track's fractional step phase when the master
                 * changes tempo. The next pulse then lands on its new boundary. */
                for (i = 0; i < NTRK; i++) {
                    if (trk[i].seq_pos < (uint32_t)FS * 2u)
                        trk[i].seq_pos = midi_clock_rescale(trk[i].seq_pos, ratio);
                    if (trk[i].seq_off)
                        trk[i].seq_off = midi_clock_rescale(trk[i].seq_off, ratio);
                }
                if (clk_pos < (uint32_t)FS * 2u)   /* (the metronome's beat as the steps) */
                    clk_pos = midi_clock_rescale(clk_pos, ratio);
            }
            midi_beat_samples = new_beat;
            int32_t bpm_q8 = (int32_t)((245760000u + midi_clock.tempo_q6 / 2u) / midi_clock.tempo_q6);
            int32_t shown = (int32_t)song.g[G_BPM] * 256;
            if (first || bpm_q8 > shown + 224 || bpm_q8 < shown - 224)
                song.g[G_BPM] = (int16_t)clamp((bpm_q8 + 128) / 256, 40, 240);
        }
    }
    if (song.playing) {
        if (midi_clock.have_pulse) {
            uint32_t interval = ms - midi_clock.last_ms;
            if (!midi_clock.tempo_q6 && interval >= 8u && interval <= 80u)
                midi_clock.interval_ms = interval;
            q = beat_samples() + midi_clock.rem;
            midi_clock.pos += q / 24u;
            midi_clock.rem = q % 24u;
        }
        midi_clock.have_pulse = 1;
    }
    midi_clock.pulse_samples = beat_samples() / 24u;
    if (midi_clock.tempo_q6) {
        /* Pulse duration and pulse samples come from the same filtered beat. */
        midi_clock.interp_q8 = (uint32_t)FS * 256u / 1000u;
    } else {
        if (!midi_clock.interval_ms)
            midi_clock.interval_ms = beat_samples() * 1000u / ((uint32_t)FS * 24u);
        midi_clock.interp_q8 = midi_clock.pulse_samples * 256u / midi_clock.interval_ms;
    }
    midi_clock.last_ms = ms;
}

static uint32_t midi_clock_advance(uint32_t now)
{
    uint32_t target, elapsed, offset, n;
    if (!midi_clock.have_pulse)
        return 0;
    elapsed = now - midi_clock.last_ms;
    /* The caller stops on clock loss, so elapsed is at most 500 ms.
     * The sample cap below keeps interpolation inside the current pulse. */
    offset = elapsed * midi_clock.interp_q8 >> 8;
    if (offset >= midi_clock.pulse_samples)
        offset = midi_clock.pulse_samples - 1u;
    target = midi_clock.pos + offset;
    n = (int32_t)(target - midi_clock.rendered) > 0 ? target - midi_clock.rendered : 0u;
    if (n > (uint32_t)FS / 8u)
        n = (uint32_t)FS / 8u;
    midi_clock.rendered += n;
    return n;
}

