/* SPDX-License-Identifier: GPL-3.0-only */
/* TB-3PO: generative 303 lines. A port of schwung-tb3po's generator (itself a port
 * of the Phazerville Hemisphere Suite TB_3PO applet, (c) djphazer and contributors,
 * GPL-3.0). The random draws happen in the same order as schwung-tb3po's
 * generate_pattern / mutate_pattern, so a seed and settings give the same line on
 * both. */
#pragma once
#include "acid_pattern.h"

void tb3po_scale_mask(acid_gen_line_t *b,uint32_t *rng,uint16_t mask,int mutate);
