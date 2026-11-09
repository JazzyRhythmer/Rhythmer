/* SPDX-License-Identifier: GPL-3.0-only
 * FMini-1 / existing Jazzy adapter. Original code and licensing in ports/ and LICENSES/ported-fmini.
 * Curated from factory programs 1-50. Dense PATCH numbers select the retained programs.
 */
#include "../ports/fmini.c"
#include "../ports/fmini_programs.h"
#define JP(n,d) {n,F_INT,0,127,d,0,0}
static const engine_t ENG_FMINI={.name="FMINI-1",.page_title={"LADDER","CONTOUR"},.edit={JP("CUTOFF",64),JP("EMPH",40),JP("CONTOUR",64),JP("F.ATK",0),JP("F.DEC",64),JP("F.SUS",40),{0},{"PATCH",F_INT,0,FMINI_CURATED_COUNT-1,0,0,0}},.presets=FMINI_PROGRAMS,.npresets=NELEM(FMINI_PROGRAMS),.knob={P_E0,P_E1,P_E2,P_E4},.poly=3,.keep=0x13,.note_on=fmini_note_on,.render=fmini_render,.block=fmini_block};
#undef JP
