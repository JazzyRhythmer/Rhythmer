#define UI_TEST_NO_MAIN 1
#ifdef TEST_JAZZY
#include "jazzy_host.h"
#else
#include "ui_test.c"
#endif
static int sends(track_t *t,int d,int p,int l,int r){return t->p[P_DIST]==d&&t->p[P_CHOR]==p&&t->p[P_DLY]==l&&t->p[P_REV]==r;}
int main(void){int bad=0;ui_power_on();fm6_init();power_on_parts();trks_put(1);
 unsigned tested=0;for(unsigned ei=0;ei<NENGINES;ei++){const engine_t *eng=ENGINES[ei];
 for(unsigned pi=0;pi<eng->npresets;pi++){
#ifdef TEST_JAZZY
 const preset_t *pr=factory_preset(eng,pi);
#else
 const preset_t *pr=&eng->presets[pi];
#endif
 int16_t expected[P_COUNT],actual[P_COUNT];for(unsigned i=0;i<P_COUNT;i++)expected[i]=actual[i]=73;
 const unsigned char fx[4]={0,24,28,36};for(unsigned i=0;i<8;i++)expected[P_E0+i]=pr->e[i];
 expected[P_ATK]=pr->env[0];expected[P_DEC]=pr->env[1];expected[P_SUS]=pr->env[2];expected[P_REL]=pr->env[3];expected[P_ED_FLT]=pr->fenv;expected[P_VOICE]=PR_MONO(pr)?V_LEGATO:V_POLY;
 for(unsigned i=0;i<4;i++)expected[P_DIST+i]=pr->fx[i]?pr->fx[i]-1:fx[i];
 preset_sound_values(actual,pr);if(memcmp(expected,actual,sizeof expected)){bad+=check("factory loader bit-identical",0);return 1;}tested++;
 }}printf("Factory preset decoding bit-identical: %u presets\n",tested);

 for(unsigned k=0;k<NTRK;k++)for(unsigned e=0;e<2;e++){
  track_t *t=&trk[k];unsigned ei=e?ENGI_ACID:ENGI_BREAK;set_engine_of(t,ei);
  for(unsigned p=0;p<ENGINES[ei]->npresets;p++){
   t->p[P_DIST]=t->p[P_CHOR]=t->p[P_DLY]=t->p[P_REV]=99;apply_preset_to(t,p);
   bad+=check("factory preset uses requested Break / Acid sends",sends(t,e?38:0,0,0,e?13:0));
  }
  init_sound_of(t,ei);bad+=check("INIT SOUND uses requested sends",sends(t,e?38:0,0,0,e?13:0));
  t->p[P_DIST]=11;t->p[P_CHOR]=22;t->p[P_DLY]=33;t->p[P_REV]=44;
 }
 project_capture(&proj_scratch);bad+=check("project packs and imports",proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire));
 bad+=check("runtime project restores",project_restore_runtime(&proj_scratch)==0);
 for(unsigned k=0;k<NTRK;k++)bad+=check("saved project retains user sends",sends(&trk[k],11,22,33,44));
 power_on_parts();for(unsigned k=0;k<NTRK;k++)bad+=check("startup defaults use Acid sends and leave other tracks dry",sends(&trk[k],trk[k].eng_req==ENGI_ACID?38:0,0,0,trk[k].eng_req==ENGI_ACID?13:0));
 return bad?1:0;}
