/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline landing-page demo: use the two MIDI-target GEN passes and native mixer. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static unsigned find_preset(unsigned engine, const char *name)
{
    const engine_t *e = ENGINES[engine];
    for (unsigned i = 0; i < e->npresets; i++)
        if (!strcmp(e->presets[i].name, name)) return i;
    fprintf(stderr, "Missing preset: %s\n", name);
    exit(2);
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    ui_power_on();
    fm6_init();
    power_on_parts();
    six_native_init();
    for (unsigned i = 0; i < NTRK; i++) {
        trk[i].p[P_MUTE] = 1;
        for (unsigned j = 0; j < NSTEP; j++) gclear(&trk[i].step[j]);
    }
    gp.seed = 909;
    gp.complex = 20;
    gp.human = gp.swing = 0;
    for (unsigned i = 0; i < NGENRE; i++)
        if (!strcmp(GENRE[i].name, "90S HOUSE")) { gp.genre = i + 1; break; }
    if (!gp.genre) return 2;

    const unsigned engines[] = {ENGI_DRUM, ENGI_ANALOG};
    const char *names[] = {"909 CLUB", "SQR BASS"};
    const char *roles[] = {"DRUM MIDI", "BASS MIDI"};
    for (unsigned i = 0; i < 2; i++) {
        track_t *t = &trk[i];
        set_engine_of(t, engines[i]);
        apply_preset_to(t, find_preset(engines[i], names[i]));
        t->engine = t->eng_req;
        t->p[P_SLEN] = 32;
        t->p[P_SDIV] = 2;
        t->p[P_SSWING] = 0;
        t->p[P_ROOT] = 0;
        t->p[P_SCALE] = SC_MIN;
        t->p[P_LEVEL] = i ? 88 : 90;
        t->p[P_PAN] = t->p[P_MUTE] = 0;
        t->p[P_DIST] = t->p[P_CHOR] = t->p[P_DLY] = t->p[P_REV] = 0;
        const char *role = gen_track(t);
        unsigned hits = 0;
        for (unsigned j = 0; j < 32; j++)
            hits += t->step[j].time == ST_NOTE && (t->step[j].n || t->step[j].hit);
        printf("%s: %s, %u active steps\n", names[i], role, hits);
        if (strcmp(role, roles[i]) || hits < 4) return 3;
    }

    song.master_q12 = 4096;
    song.g[G_BPM] = 124;
    song.g[G_CLIP] = song.g[G_PUNCH] = song.g[G_DUCK] = song.g[G_DJF] = 0;
    FILE *out = fopen(argv[1], "wb");
    if (!out) return 4;
    fx_stems = 1;
    seq_start();
    const unsigned groove = 64 * div_samples(2); /* two passes through the two-bar GEN phrase */
    uint64_t energy[2] = {0};
    int32_t pcm[2 * CTL];
    for (unsigned pos = 0; pos < groove + FS / 5u; pos += CTL) {
        if (song.playing && pos + CTL >= groove) seq_stop();
        mix_block(pcm, CTL);
        for (unsigned i = 0; i < 2; i++)
            for (unsigned j = 0; j < CTL; j++) {
                int64_t s = stem_buf[i][j];
                energy[i] += s * s;
            }
        if (fwrite(pcm, sizeof pcm, 1, out) != 1) return 4;
    }
    printf("Mixer stems: drums=%llu bass=%llu; sample rate %u\n",
           (unsigned long long)energy[0], (unsigned long long)energy[1], FS);
    return fclose(out) || !energy[0] || !energy[1] ? 4 : 0;
}
