/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* Host test of the UI (firmware/src/ui*.c with the sound sources of hostsim.c;
 * the display and the HAL are stubs, the buttons and knobs are driven through them):
 *   SOUNDS     a sound load (PRESETS knob / page, TOOLS INIT, user preset) changes the sound only: the
 *              steps, LEN DIV SWING GATE, ARP, SCL, the SLICER and the mix stay the track's. Power-on:
 *              every sequencer empty.
 *   PATTERNS   SEQ > PATTERNS: KNOB 1 PAT (factory patterns, then user presets that hold one), OCT+
 *              LOAD; over the user's steps the REPLACE? dialog (OCT- / OCT+); the PRESETS hint.
 *   UNDO       none (TONIC): SAVE held changes nothing; loads, tools and lock edits are final.
 *   REC        a tap on every screen arms the selected track and starts PLAY, without navigation.
 *              Long REC never clears. On STEP, armed and playing: keys do not write the
 *              cursor step. With the ARP on, the arp's notes are recorded, not the keys.
 *   MIDI IN    GLO > SYSTEM ROUT: CH1-4 (channels 5..16 ignored) / SEL, note-offs follow their note-ons.
 *   SAVE       refused while playing ("STOP TO SAVE"); a used slot asks OVERWRITE? (OCT+ / OCT-).
 *   ACTIONS    PATTERNS, USER, PROJECT, TOOLS: the knobs pick, OCT+ does it, OCT- cancels or goes
 *              HOME, on release, never both together; the OCT LEDs (OCT- lit, OCT+ blinks).
 *   TRACKS     the PRESETS knob does nothing there; KNOB 1 is MUTE.
 *   GRID       SEQ > STEP on a DRUM track: white keys toggle the selected lane's steps of the page, black
 *              keys 1..8 select the lane (and only they play), 9 held = ACC, 10 / 11 the page; KNOB 1..4 STEP
 *              LANE HIT ACC; the key LEDs (hits, the playhead inverted, the lane, ACC, the page keys); a step's
 *              GM notes shown on their lanes and made the lane's own by an edit; live recording into the grid
 *              (lane keys on the grid, GM keys elsewhere; other GM drums as notes; no TIE holds); 12 BEAT from
 *              PATTERNS fills the grid (undo); sound loads keep it, and other engines play it as GM notes.
 *   ROLL       the rolling digits of the card values and the header BPM: strips only, the last frame static,
 *              direction, retarget, the snaps (fast turn, names, shape, page, track, palette, force).
 *   SOAK       #63: 150000 random frames (turns, pages, transport, REC / motion, wrapping clocks): no roll
 *              outlives ROLL_FRAMES, the cards always equal a forced redraw once the rolls are over.
 *   USB SERIAL #67: MENU > USB SERIAL ON (default, the descriptors as before) / OFF (audio + MIDI only, class 0):
 *              the menu, the settings, the descriptor at boot (built with the console, as the firmware: FELUCCA_CDC).
 *   SCREEN OFF (1.1.5): MENU > DISPLAY > SCREEN OFF against a simulated clock and the panel's commands (lcd_power):
 *              dark after the time, nothing drawn, any input wakes it (SLPOUT, 120 ms, redraw, DISPON) and is
 *              swallowed; dialogs / count-in / UPDATE MODE keep it on; saved; the sound bit for bit while dark.
 * Built and run by tests/run_tests.sh. */
#ifndef FELUCCA_CDC
#define FELUCCA_CDC 1                                    /* (as src/felucca.c: the console built in, MENU > USB SERIAL) */
#endif
#include <stddef.h>
#include <stdint.h>
#define main hostsim_main
#include "hostsim.c"
#undef main

/* ------------------------------------------------------------ HAL stubs --- */
#define FM1_NCOL 11u
static const int8_t FM1_KEYMAP[6][FM1_NCOL];
static uint8_t fm1_led[FM1_NCOL], fm1_led_dim[FM1_NCOL], fm1_led_breath[FM1_NCOL], fm1_led_mid[FM1_NCOL], fm1_led_hi[FM1_NCOL];
static uint32_t host_dim_lo;                      /* hal/fm1_input.h: the glow MENU > LEDS asked for (1: DIM LO) */
static void fm1_led_dim_level(uint32_t lo) { host_dim_lo = lo; }
static int host_anim;                             /* hal/fm1_input.h: the power-on sweep running (it has the LEDs) */
static int fm1_led_anim_on(void) { return host_anim; }
#define FM1_TICKS_PER_US 1u
static uint32_t host_ticks, host_pressed, host_notes;
static int32_t host_enc[7];
static uint32_t fm1_ticks(void) { return host_ticks; }
static uint32_t fm1_input_edges(int x) { uint32_t p = host_pressed; (void)x; host_pressed = 0; return p; }
static uint32_t fm1_input_note_edges(void) { uint32_t n = host_notes; host_notes = 0; return n; }
/* host_enc_late: detents the TIMER5 scan counts just after a read (#102: while ui_input runs); the next read has them */
static int32_t host_enc_late[7];
static int32_t fm1_enc_take(uint32_t e)
{
    int32_t s = host_enc[e % 7u];
    host_enc[e % 7u] = host_enc_late[e % 7u];
    host_enc_late[e % 7u] = 0;
    return s;
}
static void (*host_wdt_hook)(void);                /* (a test's: time passing in a blocking loop, as panel_setup's) */
static void fm1_wdt_feed(void) { if (host_wdt_hook) host_wdt_hook(); }
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static uint16_t host_screen[240 * 240];
static uint32_t host_big_fills;                    /* (0.7.4) fills of most of the body (a screen cleared: a flicker) */
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    if (w >= 200u && h >= 120u)
        host_big_fills++;
#ifdef UI_FILL_HOOK
    UI_FILL_HOOK(x, y, w, h);                     /* (tests/ui_render.c: what a fill covers is gone) */
#endif
    for (j = 0; j < h && y + j < 240u; j++)
        for (i = 0; i < w && x + i < 240u; i++)
            host_screen[(y + j) * 240u + x + i] = (uint16_t)((c >> 8) | (c << 8));
}
static void lcd_sync(void) {}
/* MENU > SCREEN OFF (lcd.c lcd_power / lcd_wake_now): what the panel was told, in order (0 off: DISPOFF, SLPIN;
 * 1 SLPOUT; 2 DISPON; 3 lcd_wake_now), with the time; host_bl the panel showing (the backlight itself stays on: PA2 is
 * the key matrix's /OE too, lcd.c lcd_power) */
static uint32_t host_pw_n, host_pw[256], host_pw_ms[256];
static int host_bl = 1;
static void host_pw_log(uint32_t s)
{
    if (host_pw_n < 256u) { host_pw[host_pw_n] = s; host_pw_ms[host_pw_n] = fm1_ms; }
    host_pw_n++;
    host_bl = s != 0u;
}
static void lcd_power(uint32_t s) { host_pw_log(s); }
static void lcd_wake_now(void) { host_pw_log(3u); }
#define SCOPE_N 512u                              /* audio.c: the HOME oscilloscope */
static int16_t scope_buf[SCOPE_N];
static uint32_t scope_w;
static uint32_t host_blit_rows;                    /* rows blitted (what a frame draws) */
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    uint32_t i, j;
    host_blit_rows += h;
    for (j = 0; j < h && y + j < 240u; j++)
        for (i = 0; i < w && x + i < 240u; i++) host_screen[(y + j) * 240u + x + i] = p[j * w + i];
}
static struct { uint32_t stage; } felucca_dbg;
#define FELUCCA_FLASH 0
#ifndef FELUCCA_VERSION
#define FELUCCA_VERSION "TEST"
#endif
#ifndef UI_NO_DRIFT
#define UI_NO_DRIFT 1                                    /* (0.6.61) no drifting granules: frames compared pixel by pixel */
#endif
#ifndef PF_MS                                            /* (0.6.58) no page name drawn: the tests read the screen */
#define PF_MS 0u                                         /* (ui_render.c sets the real one: it shows it) */
#endif
static uint64_t ui_sent_px;                              /* (0.9.0) pixels sent to the panel (gfx.c GFX_HOOK_SENT) */
#ifndef GFX_HOOK_SENT
#define GFX_HOOK_SENT(n) (ui_sent_px += (n))
#endif
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/menu_items.c"
#include "../firmware/src/icons.c"
#include "../firmware/src/ui_graph.c"
#include "../firmware/src/ui_draw.c"
#include "../firmware/src/ui_menu.c"
#include "../firmware/src/ui_input.c"
#include "../firmware/src/ui_layer.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"

static int check(const char *what, int ok)
{
    printf("ui: %-74s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

/* main.c felucca_init, on the parts of 0.9.8 (TRK_OLD: T1 ANALOG ACID, T2 FM6 PAD, T3 LOFI PULSE LD, T4 DRUM-X, T5 PHASE,
 * T6 WHEEL, each its preset's sound with its sends), the music the UI tests, the renders (ui_render.c) and the CPU
 * sections (cpu_vg.c) are written on; (0.9.9) the device's own power-on (TRK_DEF: DRUM-X VOICE ANIMAL LOFI ANALOG FM6)
 * is ui_power_on_099 */
static void ui_power_on(void)
{
    uint32_t i;
    if (!lyb_ok)
        sing_init();                              /* (0.9.0: as felucca_init, VOICE's English generator words) */
    memset(trk, 0, sizeof trk);
    memset(&song, 0, sizeof song);
    memset(&chain, 0, sizeof chain);
    chain_defaults(&chain_config);
    live.mode = 0;                                /* (0.6.81: PLAY the loop, as at power-on; a DICE ALL leaves SONG) */
    for (i = 0; i < 4u; i++)                      /* (and the sections a DICE ALL wrote emptied: power-on has none) */
        if ((dice_secs >> i) & 1u) memset(proj_slot[i].raw, 0, 4);
    dice_secs = 0;
    memset(&ui, 0, sizeof ui);
    cin_left = cin_total = 0;                     /* (no count-in) */
    memset(&nm, 0, sizeof nm);                    /* NAME closed, no project name */
    proj_name[0] = 0;
    proj_cur = PROJ_NO_SLOT;
    memset(&favorites, 0, sizeof favorites);
    memset((void *)&ml, 0, sizeof ml);            /* (0.9.12) MIDI LEARN off (its state is the pool's: zero at boot) */
    ui_prefs = PREF_ACCEL_OFF;                    /* (the tests turn a detent a frame: one step each; KNOB ACCEL, ON on the
                                                   * device by default, has its own test) */
    mac_latch = 0;
    fx_lock = 0;                                  /* (0.6.44) */
    memset(pat_last, 0, sizeof pat_last);
    memset(proj_slot, 0, sizeof proj_slot);
    memset(up_bank, 0, sizeof up_bank);
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    load_depth++;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        track_defaults(t);
        set_engine_of(t, TRK_OLD[i][0]);           /* (0.9.9) the parts the UI tests are written on: 0.9.8's power-on, */
        apply_preset_to(t, TRK_OLD[i][1]);         /* T1 ANALOG ACID .. T4 DRUM-X (engines.c TRK_OLD); today's power-on */
        t->engine = t->eng_req;                    /* (TRK_DEF, ui.c power_on_parts): test_power_on_099 */
        track_defaults_steps(t);
        if (TRK_OLD[i][2])
            load_pat16(t, PATTERNS[TRK_OLD[i][2] - 1u].note, PATTERNS[TRK_OLD[i][2] - 1u].flags);
        pat_sig[i] = steps_sig(t);
        pat_last[i] = TRK_OLD[i][2];
    }
    load_depth--;
    song.sel = 0;
    ui.home = 1;
    ui.force = 1;
    panel = PANEL_DEFAULT;
    settings.palette = UI_BW_INDEX;
    palette_set(UI_BW_INDEX);
    transport_req = 0;
    panic_req = 0;
    mi_r = mi_w = midi_in_overflow = 0;
    memset(midi_sel_on, 0, sizeof midi_sel_on);
    memset(midi_ch, 0, sizeof midi_ch);
    memset(midi_owners, 0, sizeof midi_owners);
    memset(midi_bend_q8, 0, sizeof midi_bend_q8);
    memset(midi_bend_target, 0, sizeof midi_bend_target);
    memset(kb_on, 0, sizeof kb_on);                     /* no key held */
    perf_held = perf_act = kb_layer = perf_mask = 0;      /* the FX layer: nothing held */
    perf_kill = 0;
    perf_mask = 1u << panel.btn[B_FX];                  /* (ui_input sets them every frame) */
    kb_mask = layer_bits();
    perf_solo = 0;
    memset(&lys, 0, sizeof lys);
    perf_m[0] = perf_m[1] = perf_m[2] = perf_m[3] = 0;   /* (0.9.14: the macros off, nothing to revert) */
    {   /* (0.9.17) no glitcher key down, its knobs at their defaults, the buffer free */
        static const int8_t GP0[4][4] = {{50, 0, 0, 1}, {0, 4, 0, 0}, {60, 2, 20, 1}, {50, 0, 30, 2}};
        perf_down = 0;
        memcpy((void *)perf_gp, GP0, sizeof GP0);
        pf.mode = BM_NONE;
        pf.state = 0;
    }
    pm_on = 0;
    settings_hold = HOLD_DEF;
    fm1_in.buttons = fm1_in.notes = 0;
    memset(&motion, 0, sizeof motion);                  /* (main.c: no motion at power-on) */
    memset(motion_active, 0, sizeof motion_active);
    memset(motion_locked, 0, sizeof motion_locked);
    motion_base_valid = motion_full = 0;
    memset(&scrn, 0, sizeof scrn);                        /* MENU > SCREEN OFF: on, idle from now (main.c) */
    scrn.idle = fm1_ms;
    kb_asleep = 0;
    panel_moved = 0;
    memset(&lv, 0, sizeof lv);                          /* (0.9.11) no LIVE modifier, no key on MANO (seq_live.c) */
}
/* card slot drawn with MOTION's mark (ui_draw.c card_mot: its icon, or a long label in the theme colour) */
static int card_motion(uint32_t slot) { return (card_mot >> slot) & 1u; }

/* one UI frame (input, then the drawing on the stub display) */
static void frame(void)
{
    host_ticks += 16000u;
    fm1_ms += 16u;
    ui_input();
    six_gen_poll();
    ui_draw();
}
static void press(uint32_t label)                 /* a tap: down, a frame, up, a frame */
{
    fm1_in.buttons |= 1u << panel.btn[label];
    host_pressed |= 1u << panel.btn[label];
    frame();
    fm1_in.buttons &= ~(1u << panel.btn[label]);
    frame();
}
static void frames(uint32_t ms) { uint32_t i; for (i = 0; i < ms / 16u; i++) frame(); }
static void hold(uint32_t label)                  /* held 0.8 s, then let go */
{
    uint32_t k;
    fm1_in.buttons |= 1u << panel.btn[label];
    host_pressed |= 1u << panel.btn[label];
    for (k = 0; k < 52u; k++)
        frame();
    fm1_in.buttons &= ~(1u << panel.btn[label]);
    frame();
}
static void turn(uint32_t role, int32_t s)
{
    host_enc[panel.enc[role]] += s * panel.dir[role];
    frame();
    host_ticks += 200000u;                        /* (no knob acceleration between the turns) */
}
static void held_turn(uint32_t label, int32_t s)  /* (0.6.67) a button held + SELECT, let go after */
{
    fm1_in.buttons |= 1u << panel.btn[label]; host_pressed |= 1u << panel.btn[label];
    turn(EN_SELECT, s);
    fm1_in.buttons &= ~(1u << panel.btn[label]);
}
static void tempo_turn(int32_t s) { held_turn(B_GLO, s); }   /* (0.6.67) the tempo: GLO + SELECT */
static void stop_transport(void) { transport_req = 0; song.playing = 0; }
static int16_t stored_param(uint32_t slot, uint32_t track, uint32_t id)
{
    project_t p;
    return proj_import(&p, &proj_slot[slot], sizeof proj_slot[slot]) ? p.t[track].p[id] : -32768;
}
static uint8_t stored_note(uint32_t slot, uint32_t track, uint32_t step)
{
    project_t p;
    return proj_import(&p, &proj_slot[slot], sizeof proj_slot[slot]) ? p.t[track].step[step].note[0] : 0;
}
static int msg_is(const char *s) { return ui.msg_t && str_eq(ui.msg, s); }
static uint32_t menu_knob(uint32_t row)            /* (0.9.18) the MENU knob of a setting on its page (its page shown) */
{
    uint32_t c, p = menu_page_of(row);
    for (c = 0; c < 3u && MPAGE[p].row[c] != row; c++)
        ;
    return EN_K1 + c;
}
/* (0.9.5) the layer buttons' "HOLD [X] QUICK" hint (gone): anything of it said */
static int layer_hint_said(void)
{
    uint32_t i, n = str_len(ui.msg);
    if (!ui.msg_t)
        return 0;
    for (i = 0; i + 5u <= n; i++)
        if (!memcmp(ui.msg + i, "QUICK", 5) || (i + 6u <= n && !memcmp(ui.msg + i, "HOLD [", 6)))
            return 1;
    return 0;
}
/* frames, true when a layer hint was said in any of them */
static int frames_layer_hint(uint32_t n)
{
    int said = 0;
    while (n--) { frame(); said |= layer_hint_said(); }
    return said;
}
static void my_steps(track_t *t)                  /* "recorded" steps */
{
    uint32_t i;
    track_defaults_steps(t);
    for (i = 0; i < 32u; i += 3u) {
        t->step[i].note[0] = (uint8_t)(40 + i);
        t->step[i].n = 1;
        t->step[i].time = ST_NOTE;
        t->step[i].vel = 99;
    }
    t->p[P_SLEN] = 32;
}

static void go_page(uint32_t graph)               /* the page that draws graph */
{
    uint32_t i;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].graph == graph)
            break;
    ui.home = 0;
    ui.page = (uint8_t)i;
    page_entered();
}
static int steps_are(const track_t *t, uint32_t n)  /* steps 1..16 = PATTERNS[n], 17..64 empty, LEN 16 */
{
    uint32_t i;
    for (i = 0; i < NSTEP; i++) {
        uint8_t want = i < 16u ? PATTERNS[n].note[i] : 0u;
        if (t->step[i].note[0] != want || t->step[i].n != (want ? 1u : 0u))
            return 0;
    }
    return t->p[P_SLEN] == 16;
}
/* the sound of a track: engine, preset and the parameters that are not the track's own (param_kept) */
static int same_sound(const track_t *a, const track_t *b)
{
    uint32_t i;
    for (i = 0; i < P_COUNT; i++)
        if (!param_kept(i) && a->p[i] != b->p[i])
            return 0;
    return a->eng_req == b->eng_req && a->preset == b->preset;
}

/* the track's E values are DRUM's default kit (core.h DRUM_KIT_E: what a SAMPLE PERC sound loads as) */
static int perc_kit(const track_t *t)
{
    static const int16_t KIT[8] = DRUM_KIT_E;
    return !memcmp(&t->p[P_E0], KIT, sizeof KIT);
}
/* the GM drums a DRUM track plays: a kick, a snare and a closed hat each strike their lane and sound */
static int gm_hits_play(track_t *t)
{
    static const uint8_t N[3] = {36, 38, 42}, L[3] = {DV_KICK, DV_SNARE, DV_HATC};
    int32_t o[2u * CTL];
    uint32_t k, i, b, ok = 1, m = t->p[P_MUTE], mq = song.master_q12;
    t->p[P_MUTE] = 0;
    song.master_q12 = 4096;                       /* (the MASTER knob: main.c; 0 in this test) */
    for (b = 0; b < 256u && (t->xf_on || t->eng_req != t->engine); b++)   /* the engine switch done (its fade) */
        mix_block(o, CTL);
    for (k = 0; k < 3u; k++) {
        uint64_t e = 0;
        int lane = 0;
        trk_note_on(t, N[k], 100);
        for (b = 0; b < 16u; b++) {
            memset(o, 0, sizeof o);
            mix_block(o, CTL);
            for (i = 0; i < 2u * CTL; i++)
                e += (uint64_t)(o[i] < 0 ? -o[i] : o[i]);
            for (i = 0; i < NVOICE; i++)
                lane |= t->v[i].active && t->v[i].note == N[k] && (uint32_t)t->v[i].s[0] == L[k];
        }
        trk_note_off(t, N[k]);
        ok &= ENGINES[t->engine] == &ENG_DRUM && drum_lane(N[k]) == L[k] && lane && e > 0;
    }
    t->p[P_MUTE] = (int16_t)m;
    song.master_q12 = mq;
    return (int)ok;
}

/* sounds never touch the steps; the arp, the scale, the SLICER and the pattern parameters are the track's */
static void go_title(const char *title)
{
    uint32_t i;
    for (i = 0; i < NPAGES && !str_eq(PAGES[i].title, title); i++)
        ;
    if (i == NPAGES) {                          /* (0.6.80: a page renamed under a script; it read past PAGES) */
        fprintf(stderr, "go_title: no page \"%s\"\n", title);
        exit(2);
    }
    ui.home = 0;
    ui.page = (uint8_t)i;
    page_entered();
}
static int test_save(void)
{
    int bad = 0;
    uint32_t i;
    ui_power_on();
    bad += check("SAVE tabs read SONG, PROJECT, SOUND",
                 str_eq(CG_SAVE[0].nm, "SONG") && str_eq(CG_SAVE[1].nm, "PROJECT") &&
                 str_eq(CG_SAVE[2].nm, "SOUND"));
    press(B_SAVE);
    bad += check("SAVE opens PROJECTS ready to load",
                 cur_page()->graph == GR_SLOTS && save_level() == SV_LIB && ui.act == 3u);
    turn(EN_K1, 2);
    turn(EN_K2, 1);
    bad += check("PROJECTS selects the project and section with K1 and K2",
                 song_cur == 2u && song.g[G_SLOT] == 2 && ui.act == 3u);
    trk[0].p[P_LEVEL] = 61;
    (void)project_save(1);
    trk[0].p[P_LEVEL] = 13;
    press(B_OCTUP);
    bad += check("OCT+ loads the chosen section directly from PROJECTS",
                 live.cur == 1 && trk[0].p[P_LEVEL] == 61);
    song.playing = 1;
    turn(EN_K1, -1);
    press(B_OCTUP);
    bad += check("PROJECTS refuses a switch or load during playback",
                 song_cur == 2u && msg_is("STOP TO LOAD"));
    stop_transport();
    (void)song_open(0);
    song.g[G_SLOT] = 1;
    turn(EN_SELECT, 4);                          /* PROJECTS -> SECTIONS -> CLEAR -> TOOLS -> USER */
    trk[0].p[P_E0] = 5;
    go_home();
    press(B_SAVE);
    bad += check("SAVE returns to SOUND > USER with SAVE armed",
                 !ui.home && cur_page()->graph == GR_USER && ui.act == 4u && !up_used(0));
    press(B_OCTUP);
    bad += check("SAVE then OCT+ opens NAME (the automatic name), writes nothing yet",
                 name_on() && nm.kind == NK_USER_SAVE && str_eq(nm.s, nm.ph) && nm.cur == nm.len && !up_used(0));
    press(B_OCTUP);
    {
        char nmb[16], au[16];
        up_name(0, nmb);
        up_auto_name(au, trk[0].eng_req, 0);
        bad += check("..OCT+ again stores the edited sound with that name (SAVE, OCT+, OCT+)",
                     !name_on() && up_used(0) && up_value(up_rec(0), P_E0) == 5 && ui.act == 0u && str_eq(nmb, au));
    }
    press(B_SAVE);
    bad += check("SAVE advances from USER to PRESETS under SOUND",
                 cur_page()->graph == GR_BROWSE && save_level() == SV_SOUND);
    press(B_SAVE);
    bad += check("SAVE wraps from PRESETS to SONG arrangement",
                 cur_page()->graph == GR_SONG && nav_ch(SEC_SAVE)->e[nav_cur()].g == 0u);
    press(B_SAVE);
    bad += check("SAVE advances from SONG to PROJECTS",
                 cur_page()->graph == GR_SLOTS && save_level() == SV_LIB && ui.act == 3u);
    press(B_SAVE);
    press(B_SAVE);
    bad += check("SAVE reaches PROJECT's CLEAR after SECTIONS",
                 cur_page()->graph == GR_LIBCLEAR);
    turn(EN_SELECT, 3);                          /* CLEAR -> TOOLS -> USER -> PRESETS */
    go_home();
    press(B_SAVE);
    bad += check("0.9.2 SAVE from HOME: the page left, PRESETS (rule 2; was always USER)", cur_page()->graph == GR_BROWSE);
    turn(EN_SELECT, -1);
    bad += check("0.9.2 SELECT onto USER from inside: SAVE not armed", cur_page()->graph == GR_USER && ui.act == 0u);
    go_home();
    press(B_SAVE);
    bad += check("0.9.2 SAVE from HOME onto USER: SAVE armed (SAVE . OCT+ . OCT+ as before)",
                 cur_page()->graph == GR_USER && ui.act == 4u);
    press(B_OCTUP);
    bad += check("direct SAVE still asks before overwriting an occupied slot",
                 ui.confirm == CF_OVR_USER && up_value(up_rec(0), P_E0) == 5);
    press(B_OCTDN);
    ui_power_on();
    song.playing = 1;
    project_save(0);
    bad += check("project save while playing: refused, STOP TO SAVE", !project_used(0) && msg_is("STOP TO SAVE"));
    bad += check("user preset save while playing: refused", up_store(0, "X") == 2 && !up_used(0));
    song.playing = 0;
    transport_req = 1;
    project_save(0);
    bad += check("a pending PLAY cannot save a project", !project_used(0) && msg_is("STOP TO SAVE"));
    bad += check("a pending PLAY cannot save a user preset", up_store(0, "X") == 2 && !up_used(0));
    transport_req = 0;
    chain.armed = 1;
    bad += check("a pending SONG cannot save a user preset", up_store(0, "X") == 2 && !up_used(0));
    chain.armed = 0;
    song.playing = 1;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].graph == GR_SLOTS)
            break;
    ui.home = 0;
    ui.page = (uint8_t)i;
    page_entered();
    ui.msg_t = 0;
    turn(EN_K4, 1);                               /* SAVE picked */
    bad += check("PROJECT KNOB 4 picks SAVE, saves nothing", ui.act == 4u && !project_used(0) && !ui.msg_t);
    press(B_OCTUP);
    bad += check("OCT+ while playing: STOP TO SAVE, no NAME", msg_is("STOP TO SAVE") && !project_used(0) &&
                 song.g[G_SAVE] == 0 && !name_on());
    stop_transport();
    turn(EN_K4, 1);
    press(B_OCTUP);
    bad += check("stopped, an empty slot: OCT+ opens NAME (no name yet: PROJECT A)", name_on() && nm.kind == NK_PROJ_SAVE &&
                 nm.len == 0u && str_eq(nm.ph, "PROJECT A") && !project_used(0));
    press(B_OCTUP);
    bad += check("..OCT+ saves it unnamed, SAVE dropped", !name_on() && project_used(0) && msg_is("SAVED (NO FLASH)") &&
                 ui.act == 0u);
    turn(EN_K4, 1);
    press(B_OCTUP);
    bad += check("a used slot: the OVERWRITE? dialog", ui.confirm == CF_OVR_PROJ && ui.confirm_trk == 0);
    trk[0].p[P_E0] = 5;
    press(B_OCTDN);
    bad += check("OCT- keeps the slot", ui.confirm == CF_NONE && stored_param(0, 0, P_E0) != 5);
    turn(EN_K4, 1);
    press(B_OCTUP);
    press(B_OCTUP);
    bad += check("OCT+ (YES): NAME, the slot not written yet", name_on() && stored_param(0, 0, P_E0) != 5);
    press(B_OCTUP);
    bad += check("OCT+ overwrites it", ui.confirm == CF_NONE && !name_on() && stored_param(0, 0, P_E0) == 5);
    trk[0].p[P_LEVEL] = 11;
    turn(EN_K3, 1);
    press(B_OCTUP);
    bad += check("KNOB 3 LOAD, OCT+: the project back; LOAD stays picked", trk[0].p[P_LEVEL] != 11 &&
                 trk[0].p[P_LEVEL] == stored_param(0, 0, P_LEVEL) && msg_is("LOADED") && ui.act == 3u);
    /* user presets */
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].graph == GR_USER)
            break;
    ui.page = (uint8_t)i;
    page_entered();
    up_store(4, "OLD");
    ui.uslot = 4;
    turn(EN_K4, 1);
    press(B_OCTUP);
    bad += check("USER SAVE over a used slot: the OVERWRITE? dialog", ui.confirm == CF_OVR_USER && ui.confirm_trk == 4u);
    press(B_OCTUP);
    bad += check("YES: NAME with the sound's name (not the old slot's)", name_on() && !str_eq(nm.s, "OLD"));
    press(B_OCTUP);
    {
        char nm[16];
        up_name(4, nm);
        bad += check("OCT+ stores the sound there", ui.confirm == CF_NONE && !str_eq(nm, "OLD"));
    }
    song.playing = 1;
    ui.msg_t = 0;
    turn(EN_K3, 1);                               /* ERASE */
    press(B_OCTUP);
    bad += check("USER ERASE while playing: STOP TO SAVE", msg_is("STOP TO SAVE") && up_used(4));
    stop_transport();
    return bad;
}
/* Feature-test fixture, deliberately distinct from real unassigned defaults. */
static void test_optional_machines(void){
 set_engine_of(&trk[4],ENGI_ACID);set_engine_of(&trk[5],ENGI_BREAK);
 for(unsigned k=4;k<6;k++){trk[k].engine=trk[k].eng_req;trk[k].xf=trk[k].xf_on=0;memset(trk[k].v,0,sizeof trk[k].v);}
 trk[5].p[P_SLEN]=32;panic_req=0;xf_snap_t=0xff;
}
#ifndef UI_TEST_NO_MAIN
static int test_page_cycles(void)
{
    static const uint8_t buttons[SEC_N] = {
        B_HOME, B_EDIT, B_SEQ, B_FX, B_GLO, B_ENV, B_LFO, B_SCL, B_ARP, B_SAVE
    };
    int bad = 0;
    ui_power_on();
    fm6_init();
    for (uint32_t engine = 0; engine < NENGINES; engine++) {
        if (!eng_ok(engine))
            continue;
        set_engine_of(TSEL, engine);
        for (uint32_t load = 0; load <= LOAD_ECO; load++) {
            load_put(LOAD_ECO - load);
            for (uint32_t sec = 0; sec < SEC_N; sec++) {
                const ch_t *chain = nav_ch(sec);
                uint32_t first = 0;
                while (!nv_ok(sec, first))
                    first++;
                nav_go(sec, first, 0);
                for (uint32_t i = first + 1u; i <= chain->n; i++) {
                    uint32_t expected = i == chain->n ? first : i;
                    if (!nv_ok(sec, expected))
                        continue;
                    press(buttons[sec]);
                    if (nav_sec() != sec || nav_cur() != expected) {
                        fprintf(stderr, "Page tap skipped %s entry %u (engine %u, load %u)\n",
                                SEC_NAME[sec], expected, engine, load);
                        bad++;
                        break;
                    }
                }
                for (uint32_t i = first + 1u; i < chain->n; i++) {
                    if (!nv_ok(sec, i))
                        continue;
                    turn(EN_SELECT, 1);
                    if (nav_sec() != sec || nav_cur() != i) {
                        fprintf(stderr, "SELECT skipped %s entry %u (engine %u, load %u)\n",
                                SEC_NAME[sec], i, engine, load);
                        bad++;
                        break;
                    }
                }
                uint32_t last = nav_cur();
                turn(EN_SELECT, 1);
                bad += nav_cur() != last;
            }
        }
    }
    printf("All section button and SELECT page chains: %s\n", bad ? "FAIL" : "PASS");
    return bad;
}

int main(void) { int bad = test_save(); return (bad + test_page_cycles()) != 0; }
#endif
