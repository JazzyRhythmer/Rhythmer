/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static unsigned busy(track_t *t){unsigned n=0;for(unsigned i=0;i<NVOICE;i++)n+=t->v[i].active&&t->v[i].stage!=4u;return n;}
static unsigned held(track_t *t,unsigned note){for(unsigned i=0;i<NVOICE;i++)if(t->v[i].active&&t->v[i].stage!=4u&&t->v[i].note==note)return 1;return 0;}
static void setup(unsigned md){
 ui_power_on();fm6_init();power_on_parts();test_optional_machines();six_native_init();trks_put(1);
 mc_host=mc_host_v12=1;mc_off=0;sys_load=LOAD_FULL;gov_cap=16;load_tight=gov_lite=0;song.playing=0;
 for(unsigned k=0;k<6;k++){
  unsigned e=k==0?ENGI_DRUM:k==4?ENGI_ACID:k==5?ENGI_BREAK:ENGI_ANALOG;
  set_engine_of(&trk[k],e);trk[k].engine=e;apply_preset_to(&trk[k],0);
  trk[k].xf=trk[k].xf_on=0;trk[k].p[P_VOICE]=k==4?V_MONO:V_POLY;trk[k].p[P_MUTE]=0;memset(trk[k].v,0,sizeof trk[k].v);
 }
 dx_kit_load(md?11:0);if(md)for(unsigned i=0;i<8;i++)md_patch_default(&dx_kit[i],9);
 panic_req=0;xf_snap_t=0xff;drum_md_reset();memset(drum_kit,0,sizeof drum_kit);
}
static void fixtures(const char *dir){
 for(unsigned md=0;md<2;md++)for(unsigned chord=0;chord<3;chord++){
  setup(md);song.g[G_BPM]=160;
  for(unsigned k=0;k<6;k++){
   track_t *t=&trk[k];if(!chord&&k>=1&&k<=3){unsigned e=(unsigned[]){ENGI_SERUM,ENGI_PHASE,ENGI_LOFI}[k-1];set_engine_of(t,e);t->engine=e;apply_preset_to(t,0);t->xf=t->xf_on=0;}
   if(chord&&k>=1&&k<=3)t->p[P_VOICE]=V_POLY;
   t->p[P_MUTE]=chord==1&&k==5;t->p[P_SLEN]=16;t->p[P_SGATE]=127;
   t->p[P_DIST]=chord?0:60;t->p[P_CHOR]=chord?0:90;t->p[P_DLY]=chord?0:80;t->p[P_REV]=chord?0:80;
   for(unsigned i=0;i<NSTEP;i++){step_t *st=&t->step[i];step_clear(st);if(i<16&&k<5){st->time=i?ST_TIE:ST_NOTE;if(!i){st->vel=100;st->n=k==0?(chord?2:3):k==4?1:chord?3:1;for(unsigned j=0;j<st->n;j++)st->note[j]=k==0?DRUM_LANE_NOTE[(unsigned[]){0,1,3}[j]]:k==4?36:48+k*2+j*4;}}}
  }
  /* Repeated real Break chops; not just a stream's nonzero aggregate energy. */
  trk[5].p[P_LN0]=1;for(unsigned i=0;i<16;i++){step_t *st=&trk[5].step[i];st->time=(i&1)?ST_TIE:ST_NOTE;if(!(i&1)){st->n=1;st->note[0]=i;st->vel=100;}}
  project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))exit(6);
  char path[1024];snprintf(path,sizeof path,"%s/ensemble-%s-%s.project",dir,md?"md":"classic",chord==2?"analog-load":chord?"chords":"default");FILE *f=fopen(path,"wb");if(!f)exit(7);fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
}
int main(int argc,char **argv){int bad=0;if(argc>1)fixtures(argv[1]);
 setup(0);bad+=check("JIANT eight voices per part and twelve shared restored",NVOICE==8&&voice_budget()==12);
 for(unsigned i=0;i<8;i++)trk_note_on(&trk[1],48+i,100);
 bad+=check("one normal synth retains eight held notes",busy(&trk[1])==8);
 setup(0);
 for(unsigned i=0;i<3;i++)trk_note_on(&trk[0],DRUM_LANE_NOTE[(unsigned[]){0,1,3}[i]],127);
 for(unsigned k=1;k<=4;k++)trk_note_on(&trk[k],48+k,100);
 bad+=check("default ensemble retains kick snare hat and four melodic parts",busy(&trk[0])==3&&busy(&trk[1])==1&&busy(&trk[2])==1&&busy(&trk[3])==1&&busy(&trk[4])==1);
 for(unsigned md=0;md<2;md++){
  setup(md);trk_note_on(&trk[0],36,127);trk_note_on(&trk[0],38,127);
  for(unsigned k=1;k<=3;k++)for(unsigned j=0;j<3;j++)trk_note_on(&trk[k],48+k*2+j*4,100);
  trk_note_on(&trk[4],36,100);
  bad+=check("three triads plus drum pair plus Acid fit twelve voices",voices_busy()==12&&busy(&trk[0])==2&&busy(&trk[4])==1);
  for(unsigned k=1;k<=3;k++)for(unsigned j=0;j<3;j++)bad+=check("every chord tone survives simultaneous note input",held(&trk[k],48+k*2+j*4));
  unsigned energy[5]={0};for(unsigned b=0;b<24;b++)for(unsigned k=0;k<5;k++){
   int32_t out[32]={0};track_render(&trk[k],out,32);for(unsigned j=0;j<32;j++)energy[k]|=out[j]!=0;
  }
  for(unsigned k=0;k<5;k++)bad+=check("each ordinary ensemble track produces audio",energy[k]);
 }
 /* Drum hit allocation and retriggers are not subject to an MD-only cap.
  * Ordinary JIANT synth voice stealing still applies to the remaining budget. */
 for(unsigned md=0;md<2;md++){
  setup(md);for(unsigned j=0;j<8;j++)trk_note_on(&trk[0],DRUM_LANE_NOTE[j],127);
  bad+=check("all eight drum lanes admitted without MD-specific ceilings",busy(&trk[0])==8);
  voice_t saved[NVOICE];memcpy(saved,trk[0].v,sizeof saved);
  for(unsigned k=1;k<=3;k++)for(unsigned j=0;j<8;j++)trk_note_on(&trk[k],48+j,100);
  bad+=check("new synth notes cannot steal any drum hits",!memcmp(saved,trk[0].v,sizeof saved));
  bad+=check("MD leaves JIANT nominal shared budget unchanged",voice_budget()==12);
 }
 setup(1);gov_cap=4;
 for(unsigned k=1;k<=4;k++)trk_note_on(&trk[k],48+k,100);
 trk_note_on(&trk[0],36,127);
 bad+=check("governor below ensemble minimum still admits primary MD drum",busy(&trk[0])==1&&voices_busy()==5);
 for(unsigned k=1;k<=4;k++)bad+=check("JIANT primary voice protection retains other tracks",held(&trk[k],48+k));
 mc_host=mc_host_v12=0;bad+=check("JIANT single-core shared budget is eight",voice_budget()==8);
 sys_load=LOAD_ECO;bad+=check("explicit ECO retains JIANT six-voice budget",voice_budget()==6);
 printf("Rhythmer ensemble and chord ownership: %s; voice %zu B track %zu B\n",bad?"FAIL":"PASS",sizeof(voice_t),sizeof(track_t));return !!bad;
}
