/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static void fresh(unsigned six){ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(six);gp.target=0;gp.form=0;gp.energy=55;gp.complex=60;gp.human=65;gp.genre=50;for(unsigned i=0;i<GP_N;i++)gp.amt[i]=GA_AUTO;}
int main(void){int bad=0;unsigned sid=0,mel=0,unsafe=0,all6=1,pack_ok=1;
 fresh(0);track_t fifth=trk[4],sixth=trk[5];gp.seed=31;gen_go();
 bad+=check("GEN four-track mode preserves both inactive EMPTY slots",!memcmp(&fifth,&trk[4],sizeof fifth)&&!memcmp(&sixth,&trk[5],sizeof sixth));
 unsigned max_engine_copies=0,wrong_role=0,bad_osc=0,over_poly=0,role_models[NENGINES][RL_TEX+1u]={{0}};
 for(unsigned n=0;n<240;n++){
  fresh(1);gp.genre=n%NGENRE+1;gp.seed=47*n+1;gen_go();project_capture(&proj_scratch);pack_ok&=proj_pack(&proj_wire,&proj_scratch)!=0;
  unsigned uses[NENGINES]={0};
  for(unsigned k=0;k<NTRK;k++){
   unsigned e=trk[k].eng_req;all6&=e!=ENGI_EMPTY;
   unsafe+=(e==ENGI_ACID&&(gs.role[k]!=RL_BASS||gsnd(RL_BASS)!=SR_ACID))||e==ENGI_BREAK||e==ENGI_FM6||e==ENGI_M2STAT||e==ENGI_M2SAW;
   if(e!=ENGI_DRUM)mel++;
   unsigned role=gs.role[k];
   role_models[e][role]++;
   if(e==ENGI_SERUM||e==ENGI_FMINI||e==ENGI_OBERVAVE){
    if(++uses[e]>max_engine_copies)max_engine_copies=uses[e];
   }
   if(e==ENGI_FMINI||e==ENGI_OBERVAVE){
    const preset_t *p=&ENGINES[e]->presets[trk[k].preset];
    unsigned tag=PR_TAG(p);
    if(role==RL_BASS)wrong_role+=!gen_bass_ok(e,trk[k].preset)||trk[k].p[P_VOICE]!=V_MONO;
    else if(role==RL_PAD)wrong_role+=(tag!=PT_PAD&&tag!=PT_KEYS)||PR_MONO(p)||trk[k].p[P_VOICE]!=V_POLY;
    else if(role==RL_ARP||role==RL_LEAD)wrong_role+=tag==PT_PAD||tag==PT_BASS||tag==PT_FX;
    for(unsigned i=0;i<8;i++)bad_osc+=trk[k].p[P_E0+i]!=p->e[i];
    if(role==RL_PAD)for(unsigned j=0;j<NSTEP;j++)
     if(trk[k].step[j].time==ST_NOTE)over_poly+=trk[k].step[j].n>ENGINES[e]->poly;
   }
   if(e==ENGI_M2SID){sid++;bad+=check("SID automatic assignments are mono leads only",gs.role[k]==RL_LEAD&&(trk[k].p[P_VOICE]==V_MONO||trk[k].p[P_VOICE]==V_LEGATO));for(unsigned j=0;j<NSTEP;j++)if(trk[k].step[j].time==ST_NOTE)bad+=check("SID lead sequences contain single notes",trk[k].step[j].n<=1);}
  }
 }
 bad+=check("full GEN keeps at most one of each of Serum, FMINI and OberVave",max_engine_copies==1);
 bad+=check("imported patches keep their exact engine parameters and fit their role",!wrong_role&&!bad_osc&&!over_poly);
 if(RHYTHMER_FMINI&&RHYTHMER_OBERVAVE&&RHYTHMER_SWAP_WLP){
  bad+=check("FMINI and OberVave occur in real arrangements across bass, chords and melodic roles",
      role_models[ENGI_FMINI][RL_BASS]&&role_models[ENGI_FMINI][RL_LEAD]&&
      role_models[ENGI_OBERVAVE][RL_PAD]&&role_models[ENGI_OBERVAVE][RL_ARP]);
  for(unsigned e=ENGI_FMINI;e<=ENGI_OBERVAVE;e+=2)
    printf("%s GEN roles (bass/chord/lead/arp/texture): %u/%u/%u/%u/%u\n",ENGINES[e]->name,
      role_models[e][RL_BASS],role_models[e][RL_PAD],role_models[e][RL_LEAD],role_models[e][RL_ARP],role_models[e][RL_TEX]);
 }
 bad+=check("GEN six-track mode assigns all six previously empty/normal slots",all6);
 bad+=check("automatic Acid is exclusive to its named bass recipe; Break and heavy synths remain excluded",unsafe==0);
 bad+=check("all generated six-track pieces fit actual project storage",pack_ok);
 printf("SID automatic assignments %u / %u melodic slots\n",sid,mel);
 bad+=check("SID remains available at a low assignment rate",sid>0&&sid*20u<mel);
 for(unsigned a=0;a<NTRK;a++)for(unsigned form=0;form<3;form++){
  fresh(1);gp.form=form;unsigned b=(a+1)%NTRK;set_engine_of(&trk[a],ENGI_ACID);set_engine_of(&trk[b],ENGI_BREAK);
  trk[a].p[P_LEVEL]=73;trk[b].p[P_FCUT]=61;trk[a].step[2].time=ST_NOTE;trk[a].step[2].n=1;trk[a].step[2].note[0]=42;
  track_t ac=trk[a],br=trk[b];gp.seed=97+a;gen_go();
  bad+=check("GEN preserves Acid sound and MIDI at every track position",!memcmp(ac.p,trk[a].p,sizeof ac.p)&&!memcmp(ac.step,trk[a].step,sizeof ac.step)&&trk[a].eng_req==ENGI_ACID);
  bad+=check("GEN preserves Break sound and MIDI at every track position",!memcmp(br.p,trk[b].p,sizeof br.p)&&!memcmp(br.step,trk[b].step,sizeof br.step)&&trk[b].eng_req==ENGI_BREAK);
 }
 for(unsigned e=ENGI_M2STAT;e<=ENGI_M2SID;e++){
  fresh(1);set_engine_of(&trk[2],e);trk[2].engine=(uint8_t)e;gp.target=3;
  for(unsigned j=0;j<16;j++){trk[2].step[j].time=ST_NOTE;trk[2].step[j].n=3;trk[2].step[j].note[0]=60;trk[2].step[j].note[1]=64;trk[2].step[j].note[2]=67;}
  gen_go();unsigned mono=1;for(unsigned j=0;j<NSTEP;j++)if(trk[2].step[j].time==ST_NOTE)mono&=trk[2].step[j].n<=1;
  bad+=check("target MIDI respects every Mono2k model's mono lead role",mono&&gs.role[2]==RL_LEAD&&trk[2].eng_req==e);
 }
 unsigned acid_cases=0,acid_notes=0,acid_bad=0;
 for(unsigned owner=0;owner<NTRK;owner++)for(unsigned genre=0;genre<NGENRE;genre++){
  fresh(1);track_t *t=&trk[owner];set_engine_of(t,ENGI_ACID);t->engine=ENGI_ACID;
  apply_preset_to(t,(owner+genre)%ENG_ACID.npresets);t->p[P_ROOT]=5;t->p[P_SCALE]=SC_DOR;
  /* A previous polyphonic phrase must not make Acid a chord or lead target. */
  for(unsigned j=0;j<32;j++){t->step[j].time=ST_NOTE;t->step[j].n=3;t->step[j].note[0]=60;t->step[j].note[1]=64;t->step[j].note[2]=67;}
  track_t before[NTRK];memcpy(before,trk,sizeof before);song_t song_before=song;
  gp.target=owner+1;gp.genre=genre+1;gp.seed=31;gen_go();
  acid_bad+=gs.role[owner]!=RL_BASS||trk_engp(t)!=&ENG_ACID||t->eng_req!=ENGI_ACID||memcmp(t->p,before[owner].p,sizeof t->p)||t->preset!=before[owner].preset||memcmp(&song,&song_before,sizeof song);
  for(unsigned j=0;j<NTRK;j++)if(j!=owner)acid_bad+=memcmp(&trk[j],&before[j],sizeof trk[j])!=0;
  for(unsigned j=0;j<NSTEP;j++)if(t->step[j].time==ST_NOTE){acid_notes++;acid_bad+=t->step[j].n!=1||t->step[j].hit!=0;acid_bad+=!(scale_mask(t)>>( (t->step[j].note[0]+12u-5u)%12u)&1u);}
  acid_cases++;
 }
 printf("Native Hortator Acid target GEN: %u genre/track cases, %u mono events\n",acid_cases,acid_notes);
 bad+=check("Acid target GEN uses the native engine as SCL-matched mono bass without changing sounds or other tracks",acid_cases==NTRK*NGENRE&&acid_notes&& !acid_bad);
 unsigned acid_genre=NGENRE;
 for(unsigned g=0;g<NGENRE;g++)if(!strcmp(GENRE[g].name,"ACID HOUSE"))acid_genre=g;
 for(unsigned form=0;form<3;form++){
  fresh(1);memset(proj_slot,0,sizeof proj_slot);dice_secs=0;gp.genre=acid_genre+1;gp.form=form;gp.seed=31;gen_go();unsigned found=0,hits=0;
  for(unsigned k=0;k<NTRK;k++)if(trk[k].eng_req==ENGI_ACID){found++;bad+=check("named Acid recipe allocates only a native mono bass",gs.role[k]==RL_BASS&&trk_engp(&trk[k])==&ENG_ACID);for(unsigned j=0;j<NSTEP;j++)if(trk[k].step[j].time==ST_NOTE){hits++;bad+=check("automatically assigned Acid receives its monophonic sequence",trk[k].step[j].n==1);}}
  bad+=check("ALL LOOP/PARTS/SONG completes a newly assigned Acid before releasing its transaction mask",found==1&&hits>0&&!six_gen_work_mask);
  unsigned sections=0;
  for(unsigned s=0;s<4;s++)if(proj_import(&proj_scratch,&proj_slot[s],sizeof proj_slot[s])){for(unsigned k=0;k<NTRK;k++)if(proj_scratch.t[k].engine==ENGI_ACID){unsigned notes=0;for(unsigned j=0;j<NSTEP;j++)if(proj_scratch.t[k].step[j].time==ST_NOTE){notes++;bad+=check("stored Acid section has no chords",proj_scratch.t[k].step[j].n==1);}bad+=check("stored Acid section contains its generated MIDI",notes>0);sections++;}}
  bad+=check("generated sections retain the native Acid assignment",!form||sections>0);
 }
 fresh(1);gx.eng=ENGI_ACID;gen_sound(&trk[1],RL_BASS,SR_SUB);bad+=check("ordinary bass recipe cannot force Acid",trk[1].eng_req!=ENGI_ACID);
 gx.eng=ENGI_ACID;gen_sound(&trk[2],RL_PAD,SR_ACID);bad+=check("Acid-named recipe cannot place Acid on chords",trk[2].eng_req!=ENGI_ACID);
 gx.eng=ENGI_ACID;gen_sound(&trk[3],RL_LEAD,SR_ACID);bad+=check("Acid-named recipe cannot place Acid on melody",trk[3].eng_req!=ENGI_ACID);gx.eng=-1;
 printf("GEN active slots and mono model policy: %s\n",bad?"FAIL":"PASS");return !!bad;
}
