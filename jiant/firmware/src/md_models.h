/* SPDX-License-Identifier: GPL-3.0-only
 * Adapted Hortator MD factory defaults; original authors retained in kernel files. */
typedef struct {const char *name; const char *label[3]; int16_t def[8]; uint8_t ctl[3]; void (*trigger)(md_track_t*,md_voice_t*); void (*render)(md_track_t*,md_voice_t*,int32_t*,uint32_t);} md_model_t;
static const md_model_t MD_MODELS[]={
 {"HHEFM",{"MOD","MFRQ","FB"},{-2,52,0,0,3,12,78,26},{4,5,7},hhefm_trigger,hhefm_render},
 {"CBEFM",{"MOD","MFRQ","FB"},{-2,61,92,56,17,112,42,0},{4,5,3},cbefm_trigger,cbefm_render},
 {"CYEFM",{"MOD","MFRQ","FB"},{-2,93,22,107,76,88,86,0},{4,5,2},cyefm_trigger,cyefm_render},
 {"BDTRX",{"SWEEP","PUNCH","NOISE"},{-4,68,35,35,14,14,35,24},{2,4,5},ktrx_trigger,ktrx_render},
 {"SDTRX",{"SNAP","TONE","BUMP"},{0,7,127,0,0,64,92,0},{2,3,4},sdtrx_trigger,sdtrx_render},
 {"TMTRX",{"RAMP","DAMP","DIST"},{0,48,48,102,64,0,0,0},{2,4,5},xttrx_trigger,xt_render},
 {"CGTRX",{"RAMP","DAMP","DIST"},{11,15,30,29,105,127,107,0},{2,4,5},xctrx_trigger,xt_render},
 {"CPTRX",{"CLPY","HARD","RATE"},{10,127,56,7,43,56,0,93},{2,3,4},cptrx_trigger,cptrx_render},
 {"RSTRX",{"DIST","-","-"},{-4,0,0,0,0,0,0,0},{2,3,4},rstrx_trigger,rstrx_render},
 {"CBTRX",{"ENH","DAMP","TONE"},{0,48,127,127,0,0,105,0},{2,3,4},cbtrx_trigger,cbtrx_render},
 {"CHTRX",{"GAP","HPF","LPF"},{0,8,0,0,127,0,0,0},{2,3,4},chtrx_trigger,trxm_render},
 {"OHTRX",{"GAP","HPF","LPF"},{0,61,57,8,127,0,0,0},{2,3,4},ohtrx_trigger,trxm_render},
 {"CYTRX",{"RICH","TOP","TTUN"},{0,35,127,93,107,127,0,0},{2,3,4},cytrx_trigger,trxm_render},
 {"SHTRX",{"ATT","REV","DAMP"},{0,64,0,0,0,0,64,64},{2,3,4},matrx_trigger,matrx_render},
 {"CLTRX",{"DUAL","TUNE2","ENH"},{0,64,0,0,64,0,0,0},{2,4,3},cltrx_trigger,cltrx_render},
 {"B2TRX",{"RAMP","HOLD","TICK"},{-1,74,33,22,7,4,31,6},{2,3,4},b2trx_trigger,b2trx_render},
 {"BDEFM",{"MOD","MFRQ","RAMP"},{-13,68,10,32,29,5,4,24},{4,5,2},bdefm_trigger,bdefm_render},
 {"SDEFM",{"MOD","MFRQ","NOIS"},{-9,60,14,79,0,5,127,29},{4,5,2},sdefm_trigger,sdefm_render},
 {"TMEFM",{"MOD","MFRQ","RAMP"},{12,85,62,36,64,4,45,51},{4,5,2},xtefm_trigger,bdefm_render},
 {"CPEFM",{"MOD","MFRQ","CLPS"},{24,45,91,30,75,36,127,61},{4,5,2},cpefm_trigger,cpefm_render},
 {"RSEFM",{"MOD","HPF","SNAR"},{1,10,8,15,38,21,59,0},{2,3,4},rsefm_trigger,rsefm_render},
};
#define MD_NMODEL 21u
