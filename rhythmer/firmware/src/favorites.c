/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
#define FELUCCA_FAVORITES 1
/* Stable engine/preset references; fixed capacity independent of optional engines. */
typedef struct {
    uint8_t factory[16][32]; /* engine 0..15, preset 0..255 */
    uint32_t user, filter;
} favorites_t;
static favorites_t favorites;
/* Preserve Acid/Break favorites; new engines use retired rows. */
static uint32_t favorite_row(uint32_t e) { return e == ENGI_FMINI || e == ENGI_OBERVAVE ? 14u : e == ENGI_ACID ? 3u : e == ENGI_BREAK ? 4u : e == ENGI_SERUM ? 5u : e == ENGI_LOFI ? 6u : e == ENGI_M2STAT ? 8u : e == ENGI_M2SAW ? 9u : e == ENGI_M2SID ? 11u : e; }
/* The two banks use separate high bits in retired ANIMAL's row. Its old
 * low bits, the crash log, and global settings stay intact. */
static uint32_t favorite_slot(uint32_t engine, uint32_t preset)
{
    if (engine == ENGI_FMINI || engine == ENGI_OBERVAVE)
        return preset < 64u ? preset + (engine == ENGI_FMINI ? 128u : 192u) : 256u;
    return preset;
}
static int favorite_has(uint32_t engine, uint32_t preset)
{
    if (engine == NENGINES)
        return preset < UP_SLOTS && ((favorites.user >> preset) & 1u);
    preset = favorite_slot(engine, preset);
    return engine < NENGINES && favorite_row(engine) < 16u && preset < 256u &&
        ((favorites.factory[favorite_row(engine)][preset / 8u] >> (preset % 8u)) & 1u);
}
static int favorite_set(uint32_t engine, uint32_t preset, int on)
{
    if (engine > NENGINES || (engine == NENGINES ? preset >= UP_SLOTS : favorite_row(engine) >= 16u || favorite_slot(engine,preset) >= 256u))
        return 0;
    if (favorite_has(engine, preset) == !!on) return 0;
    if (engine == NENGINES) {
        if (on) favorites.user |= 1u << preset;
        else favorites.user &= ~(1u << preset);
    } else {
        preset = favorite_slot(engine, preset);
        uint8_t *b = &favorites.factory[favorite_row(engine)][preset / 8u];
        if (on) *b |= (uint8_t)(1u << (preset % 8u));
        else *b &= (uint8_t)~(1u << (preset % 8u));
    }
    return 1;
}
