/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static int bad;
#define C(s,x) do{if(!(x)){fprintf(stderr,"FAIL %s\n",s);bad++;}}while(0)
static void setup(unsigned gi,unsigned seed){
 gs.gi=gi;gs.g=&GENRE[gi];gs.nsnd=gs.fm6=0;gs.energy=55;gs.human=65;gs.complex=60;
 gp.seed=(uint16_t)(seed%10000);gx.on=0;gx.eng=-1;gseed(GS_SOUND);
}
int main(int argc,char **argv){
 ui_power_on();load_depth++;fm6_init();power_on_parts();test_optional_machines();load_depth--;six_native_init();trks_put(1);
 set_engine_of(&trk[2],ENGI_ANALOG);set_engine_of(&trk[3],ENGI_ANALOG);
 FILE *out=argc>1?fopen(argv[1],"w"):0;if(out)fprintf(out,"genre,engine,preset,count\n");
 unsigned total=0,fmini=0,ordinary=0,fmini_presets=0;
 uint8_t fmini_seen[UINT8_MAX]={0};
 for(unsigned gi=0;gi<NGENRE;gi++){
  unsigned count[NENGINES][UINT8_MAX]={{0}},different=0,top=0,colours=0;
  for(unsigned seed=0;seed<512;seed++){
   setup(gi,seed*17+31);gen_sound(&trk[1],RL_BASS,gs.g->snd[GP_BASS]);
   unsigned e=trk[1].eng_req,p=trk[1].preset;
   C("ordinary bass pool stays light; named Acid recipe uses native Hortator",gs.g->snd[GP_BASS]==SR_ACID?e==ENGI_ACID:gen_auto_synth_ok(e));
   C("bass tag or explicit BASS name, never arbitrary fallback",gen_bass_ok(e,p));
   C("no automatic FX/bytebeat bass",PR_TAG(&ENGINES[e]->presets[p])!=PT_FX);
   if(e==ENGI_ANALOG)C("original bass oscillator interval stays in tune",trk[1].p[P_E7]==ENGINES[e]->presets[p].e[7]);
   if(e==ENGI_M2SID)C("original SID tuning stays in tune",trk[1].p[P_E7]==ENGINES[e]->presets[p].e[7]);
   count[e][p]++;colours|=1u<<gen_bass_colour(e,p);total++;
   if(e!=ENGI_ACID)ordinary++;
   if(e==ENGI_FMINI){
    fmini++;
    fmini_presets+=!fmini_seen[p];fmini_seen[p]=1;
    C("FMINI bass stays mono and retains its factory patch",trk[1].p[P_VOICE]==V_MONO&&trk[1].p[P_E7]==ENGINES[e]->presets[p].e[7]);
   }
  }
  for(unsigned e=0;e<NENGINES;e++)for(unsigned p=0;p<ENGINES[e]->npresets;p++)if(count[e][p]){
   different++;if(count[e][p]>top)top=count[e][p];
   if(out)fprintf(out,"%s,%s,%s,%u\n",GENRE[gi].name,ENGINES[e]->name,ENGINES[e]->presets[p].name,count[e][p]);
  }
  C("ordinary genres keep ten bass presets; explicit Acid reaches all four native sounds",different>=(GENRE[gi].snd[GP_BASS]==SR_ACID?4:10));
  C("no single preset dominates half the results",top<256);
  if(RHYTHMER_FMINI&&RHYTHMER_SWAP_WLP&&GENRE[gi].drum>=DM_TECHNO&&GENRE[gi].drum<=DM_HARD){
   unsigned techno_fmini=0;
   for(unsigned p=0;p<ENGINES[ENGI_FMINI]->npresets;p++)techno_fmini+=count[ENGI_FMINI][p];
   C("techno allows occasional FMINI bass without the automatic bonus",techno_fmini>0&&techno_fmini<77);
   printf("%s FMINI bass: %u / 512\n",GENRE[gi].name,techno_fmini);
  }
  C("ordinary genres keep at least five sound colours",GENRE[gi].snd[GP_BASS]==SR_ACID||__builtin_popcount(colours)>=5);
 }
 if(RHYTHMER_FMINI&&RHYTHMER_SWAP_WLP){
  C("FMINI is frequently selected without dominating bass",fmini*4>ordinary&&fmini*2<ordinary);
  C("FMINI chooses multiple bass patches",fmini_presets>=8);
  printf("FMINI bass: %u / %u ordinary draws, %u distinct presets\n",fmini,ordinary,fmini_presets);
 }
 /* A missing-role engine is rerouted rather than taking its pad/lead list. */
 setup(0,31);gx.eng=ENGI_M2SAW;gen_sound(&trk[1],RL_BASS,SR_SUB);
 C("expensive forced engine cannot bypass safe pitched-bass selection",gen_auto_synth_ok(trk[1].eng_req)&&gen_bass_ok(trk[1].eng_req,trk[1].preset));
 for(unsigned profile=0;profile<BP_N;profile++)for(unsigned c=0;c<GB_N;c++)C("genre guidance is positive, not an exclusion",GB_WEIGHT[profile][c]>0);
 /* Identical forced Analog pools isolate the genre bias from engine priors. */
 unsigned warm=0,jungle=0,acid=0;
 for(unsigned seed=0;seed<2048;seed++)for(unsigned which=0;which<3;which++){
  setup(which==0?44:which==1?46:4,seed);gx.eng=ENGI_ANALOG;
  gen_sound(&trk[1],RL_BASS,SR_FMBASS);unsigned c=gen_bass_colour(ENGI_ANALOG,trk[1].preset);
  if(which==0)warm+=c==GB_ROUND;
  if(which==1)jungle+=c==GB_REESE||c==GB_SUB;
  if(which==2)acid+=c==GB_RESO;
 }
 C("soul raises several rounded bass choices",warm>920);
 C("jungle nudges sub and Reese together",jungle>1000);
 C("acid nudges resonance without locking one sound",acid>480&&acid<1200);
 setup(0,31);track_t *t=&trk[2];t->p[P_SLEN]=16;
 for(unsigned i=0;i<16;i++){step_clear(&t->step[i]);t->step[i].time=ST_NOTE;t->step[i].n=3;t->step[i].note[0]=72;t->step[i].note[1]=76;t->step[i].note[2]=79;}
 static const uint8_t bases[RL_N]={106,102,80,88,82,54},lifts[RL_N]={0,6,10,8,10,6};
 for(unsigned role=0;role<RL_N;role++){
  t->p[P_LEVEL]=bases[role];if(role!=RL_DRUM)dlevel(t,16);unsigned expected=t->p[P_LEVEL]+lifts[role];
  gen_level(t,role,16);C("role gain lift follows the original register correction",t->p[P_LEVEL]==expected);
  for(unsigned section=0;section<4;section++)gen_level(t,role,16);
  C("A-D generation never compounds level attenuation",t->p[P_LEVEL]==expected);
 }
 if(out)fclose(out);
 printf("Genre-aware bass + GEN mix: %s (%u genre draws; soul=%u jungle=%u acid=%u of 2048)\n",bad?"FAIL":"PASS",total,warm,jungle,acid);
 return !!bad;
}
