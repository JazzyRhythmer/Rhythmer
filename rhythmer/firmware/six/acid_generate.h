/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* MIDI-only command. cfg = density/accent/slide/octaves/base-octave.
 * Root and scale mask come directly from the selected track's common SCL. */
void acid_generate(uint8_t *notes,uint8_t *flags,unsigned len,const uint8_t cfg[5],uint32_t *seed,int mutate,unsigned root,uint16_t mask);
