/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-loop GEN only. Soft genre colour, with a hard pitched-bass role fence.
 * No audio callback work or new voices. Numeric genre/preset IDs stay stable. */
enum { GB_SUB,GB_ROUND,GB_PUNCH,GB_RESO,GB_REESE,GB_ORGAN,GB_CHIP,GB_N };
enum { BP_HOUSE,BP_DEEP,BP_ACID,BP_TECH,BP_SYNTH,BP_FUNK,BP_ROCK,
       BP_HIPHOP,BP_TRAP,BP_GARAGE,BP_JUNGLE,BP_LIQUID,BP_DUB,
       BP_SOUL,BP_AMBIENT,BP_IDM,BP_N };
/* Relative design weights, not claims of measured usage percentages. Every
 * suitable colour remains reachable. Existing engine priors still apply. */
static const uint8_t GB_WEIGHT[BP_N][GB_N]={
 {5,7,8,4,3,7,3}, {8,7,6,3,3,7,3}, {5,6,4,9,4,3,3}, {7,7,7,6,6,3,4},
 {5,8,8,6,5,3,5}, {6,9,7,5,3,6,3}, {4,9,6,4,4,5,3}, {8,8,6,3,3,5,3},
 {9,7,5,4,3,3,3}, {7,6,8,5,6,7,3}, {9,6,5,4,9,3,3}, {9,7,6,3,7,4,3},
 {9,8,4,3,3,5,4}, {8,9,7,3,3,7,3}, {9,7,5,3,5,5,4}, {7,6,8,6,6,3,8}
};
static const uint8_t GB_PROFILE[NGENRE]={
 BP_HOUSE,BP_DEEP,BP_HOUSE,BP_HOUSE,BP_ACID,BP_TECH,BP_SYNTH,BP_DUB,BP_TECH,BP_TECH,
 BP_SYNTH,BP_SYNTH,BP_SYNTH,BP_SYNTH,BP_SYNTH,BP_SYNTH,BP_ROCK,BP_ROCK,BP_SYNTH,BP_SYNTH,
 BP_ROCK,BP_ROCK,BP_ROCK,BP_ROCK,BP_FUNK,BP_FUNK,BP_HIPHOP,BP_TRAP,BP_GARAGE,BP_GARAGE,
 BP_JUNGLE,BP_JUNGLE,BP_DUB,BP_DUB,BP_SOUL,BP_SOUL,BP_FUNK,BP_AMBIENT,BP_AMBIENT,BP_AMBIENT,
 BP_AMBIENT,BP_IDM,BP_IDM,BP_SOUL,BP_SOUL,BP_SOUL,BP_JUNGLE,BP_LIQUID,BP_TRAP,BP_FUNK,
 BP_HOUSE,BP_FUNK,BP_TRAP,BP_HIPHOP,BP_DEEP,BP_IDM,BP_AMBIENT,BP_LIQUID,BP_HIPHOP
};
static int gen_bass_ok(unsigned e,unsigned p)
{
    const preset_t *s=&ENGINES[e]->presets[p];unsigned tag=PR_TAG(s);
    if(gnever(s->name)||tag==PT_FX)return 0;
    if(e==ENGI_LOFI&&s->e[1]>=RW_BYTE)return 0;
    return tag==PT_BASS||dname_has(s->name,"BASS");
}
static unsigned gen_bass_colour(unsigned e,unsigned p)
{
    const char *n=ENGINES[e]->presets[p].name;
    if(dname_has(n,"SUB")||dname_has(n,"TRI"))return GB_SUB;
    if(dname_has(n,"REESE")||dname_has(n,"GROWL"))return GB_REESE;
    if(dname_has(n,"RESO")||dname_has(n,"SKEW")||dname_has(n,"WOBBLE"))return GB_RESO;
    if(e==ENGI_WHEEL)return GB_ORGAN;
    if(e==ENGI_M2SID)return GB_CHIP;
    if(dname_has(n,"FB BASS")||dname_has(n,"SYN BASS")||dname_has(n,"SEQ BASS")||dname_has(n,"OCTAVE PUMP"))return GB_PUNCH;
    return GB_ROUND;
}
static unsigned gen_bass_weight(unsigned e,unsigned p)
{
    return GB_WEIGHT[GB_PROFILE[gs.gi%NGENRE]][gen_bass_colour(e,p)];
}
/* Normalize by candidate count: an engine with five bass presets must not
 * automatically outweigh an engine with two just because its list is longer. */
static unsigned gen_bass_engine_weight(unsigned e,unsigned prior)
{
    unsigned n=0,sum=0;
    for(unsigned p=0;p<ENGINES[e]->npresets;p++)if(gen_bass_ok(e,p)){n++;sum+=gen_bass_weight(e,p);}
    return n?(prior*sum+n*2u)/(n*4u):0;
}
