/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static int bad;
#define C(m,x) do{bad+=check(m,(x));}while(0)
static unsigned valid(const track_t *t){for(unsigned i=0;i<NSTEP;i++){const step_t *s=&t->step[i];if(s->n>4)return 0;for(unsigned j=0;s->time==ST_NOTE&&j<s->n;j++)if(s->note[j]>34)return 0;}return 1;}
static void fixture(const char *dir,const char *name){
 char path[1024];snprintf(path,sizeof path,"%s/break-tools-%s.project",dir,name);
 project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))exit(20);
 FILE *f=fopen(path,"wb");if(!f)exit(21);fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 snprintf(path,sizeof path,"%s/break-tools-%s.steps",dir,name);f=fopen(path,"wb");if(!f)exit(23);fwrite(TSEL->step,1,sizeof TSEL->step,f);fclose(f);
}
int main(int argc,char **argv){
 ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(1);song.sel=5;
 if(argc>1){char p[1024];snprintf(p,sizeof p,"%s/break-tools-layout.json",argv[1]);FILE *f=fopen(p,"w");if(!f)return 22;
  fprintf(f,"{\"layer\":%u,\"rec\":%u,\"seq_idx\":%u,\"source\":%u,\"every\":%u,\"action\":%u}\n",(unsigned)offsetof(__typeof__(ui),layer),(unsigned)offsetof(song_t,rec),(unsigned)offsetof(track_t,seq_idx),P_LN0,P_MPAT,P_LN1);fclose(f);}
 track_t *t=TSEL;for(unsigned k=0;k<5;k++)trk[k].p[P_MUTE]=1;
 t->p[P_MUTE]=0;ui.home=1;
 C("Break SEQ hold is available from HOME",ly_avail(LAYER_SEQ));
 song.sel=1;C("Synth HOME keeps the original SEQ gesture",!ly_avail(LAYER_SEQ));song.sel=5;
 fm1_in.buttons|=1u<<panel.btn[B_SEQ];host_pressed|=1u<<panel.btn[B_SEQ];frames(850);
 C("physical SEQ hold opens the Break-only keyboard",layer_open()==LAYER_SEQ&&!strcmp(layer_head(),"[SEQ] BREAK"));
 C("Break held keys are masked from ordinary note recording",(kb_mask&(1u<<panel.btn[B_SEQ]))!=0);
 char nm[12];ly_keyname(LAYER_SEQ,0,0,nm);C("first white key advertises STRAIGHT",!strcmp(nm,"STRAIGHT"));
 C("held knobs expose LEN AMOUNT EVERY AUTO ACT",ly_page(LAYER_SEQ)->id[1]==P_MAMT&&ly_page(LAYER_SEQ)->id[2]==P_MPAT&&ly_page(LAYER_SEQ)->id[3]==P_LN1);
 int16_t fx[8];memcpy(fx,t->p+P_E0,sizeof fx);step_t other[5][NSTEP];for(unsigned k=0;k<5;k++)memcpy(other[k],trk[k].step,sizeof other[k]);
 tools_break_key(BS_STRAIGHT,0);unsigned straight=1;for(unsigned i=0;i<32;i++)straight&=(i&1)?t->step[i].time==ST_TIE:t->step[i].time==ST_NOTE&&t->step[i].note[0]==i/2;
 C("STRAIGHT authors all sixteen chops over two bars",straight&&t->p[P_SLEN]==32&&t->p[P_SDIV]==2&&t->p[P_SGATE]==127&&t->p[P_LN0]==1);
 C("straight generation keeps Break sound and source controls",!memcmp(fx,t->p+P_E0,sizeof fx));
 if(argc>1)fixture(argv[1],"straight");
 step_t base[NSTEP];memcpy(base,t->step,sizeof base);tools_break_key(BS_MUTATE,0);
 unsigned anchors=1;for(unsigned i=0;i<32;i+=4)anchors&=!memcmp(&base[i],&t->step[i],sizeof(step_t));
 C("MUTATE changes chops while preserving quarter-note anchors",anchors&&memcmp(base,t->step,32*sizeof(step_t))&&valid(t));
 if(argc>1)fixture(argv[1],"mutate");
 tools_break_key(BS_RANDOM,0);C("RANDOM authors bounded sample commands regardless of SCL",valid(t)&&bs_nonempty(t,32)&&t->step[0].note[0]==0);
 if(argc>1)fixture(argv[1],"random");
 tools_break_key(BS_STRAIGHT,0);tools_break_key(BS_AB,0);
 C("A/B becomes two real sequencer source modifiers",t->step[0].n==2&&t->step[0].note[1]==32&&t->step[16].n==2&&t->step[16].note[1]==33);
 tools_break_key(BS_RANDOM,0);C("randomization preserves sequenced A/B source changes",t->step[0].note[1]==32&&t->step[16].note[1]==33&&valid(t));
 if(argc>1)fixture(argv[1],"ab-random");
 tools_break_key(BS_STRAIGHT,0);memcpy(base,t->step,sizeof base);tools_break_key(BS_FILL,0);
 C("FILL writes only the phrase tail with slice then roll events",!memcmp(base,t->step,28*sizeof(step_t))&&t->step[28].note[0]<16&&t->step[29].note[0]>=24&&t->step[31].note[0]<=31);
 if(argc>1)fixture(argv[1],"fill");
 tools_break_key(BS_STRAIGHT,0);memcpy(base,t->step,sizeof base);tools_break_key(BS_SHUFFLE,0);
 unsigned beats=1,used=0;for(unsigned i=0;i<8;i++){unsigned src=t->step[i*4].note[0]/2;beats&=src<8&&!(used&(1u<<src))&&!memcmp(&t->step[i*4],&base[src*4],4*sizeof(step_t));used|=1u<<src;}
 C("SHUFFLE retains intact beats and the opening beat",beats&&used==255&&!memcmp(base,t->step,4*sizeof(step_t))&&memcmp(base,t->step,32*sizeof(step_t)));
 for(unsigned k=0;k<5;k++)C("Break actions never change another track's sequence",!memcmp(other[k],trk[k].step,sizeof other[k]));
 fm1_in.buttons&=~(1u<<panel.btn[B_SEQ]);frame();C("release closes the layer and restores normal keyboard routing",!layer_open());
 tools_break_key(2,1);tools_break_key(7,1);tools_break_key(9,1);
 C("black keys select every four bars RANDOM and DRIFT",t->p[P_MPAT]==3&&t->p[P_LN1]==2&&t->p[P_MMODE]==1);
 C("automation settings survive project save/load",!project_save(0));t->p[P_MPAT]=t->p[P_LN1]=t->p[P_MMODE]=0;project_load(0);
 C("saved Break automation parameters restore unchanged",t->p[P_MPAT]==3&&t->p[P_LN1]==2&&t->p[P_MMODE]==1);
 tools_break_key(BS_STRAIGHT,0);t->p[P_MPAT]=3;t->p[P_LN1]=0;t->p[P_MMODE]=0;t->p[P_MAMT]=64;
 song.playing=1;bar_n=0;beat_n=0;beat_pos=0;mutate_poll();memcpy(base,t->step,sizeof base);
 bar_n=3;beat_n=3;beat_pos=beat_samples()*7/8;mutate_poll();C("saved automation mutates on the fourth-bar scheduling boundary",memcmp(base,t->step,32*sizeof(step_t))&&valid(t));
 memcpy(base,t->step,sizeof base);mutate_poll();C("one automatic mutation per boundary",!memcmp(base,t->step,sizeof base));
 t->p[P_E5]=1;bar_n=7;mutate_poll();C("Break FREEZE pauses automatic pattern edits",!memcmp(base,t->step,sizeof base));t->p[P_E5]=0;song.playing=0;
 if(argc>1){t->p[P_LN1]=1;t->p[P_MPAT]=2;fixture(argv[1],"auto-fill");t->p[P_LN1]=2;fixture(argv[1],"auto-random");}
 for(unsigned len=1;len<=64;len++){
  t->p[P_SLEN]=(int16_t)len;bs_straight(t,len);
  for(unsigned a=1;a<128;a+=31)for(unsigned kind=BS_MUTATE;kind<=BS_FILL;kind++){bs_vary(t,kind,a);if(!valid(t)){bad++;goto done;}}
 }
 C("all 1-64 step lengths and variation amounts keep commands in range",1);
 t->p[P_SLEN]=32;bs_straight(t,32);memcpy(base,t->step,sizeof base);bs_vary(t,BS_MUTATE,0);C("zero automatic amount is a no-op",!memcmp(base,t->step,sizeof base));
 if(argc>1){
  t->p[P_MPAT]=2;t->p[P_LN1]=2;t->p[P_MAMT]=64;t->p[P_MMODE]=0;
  for(unsigned k=0;k<5;k++){track_t *a=&trk[k];a->p[P_MUTE]=0;a->p[P_SLEN]=16;a->p[P_SGATE]=127;
   for(unsigned i=0;i<NSTEP;i++){step_clear(&a->step[i]);if(i>=16)continue;
    if(k==0){if(!(i&1)){a->step[i].time=ST_NOTE;a->step[i].n=1;a->step[i].note[0]=!(i%8)?36:i%8==4?38:42;a->step[i].vel=100;}}
    else if(!(i%4)){a->step[i].time=ST_NOTE;a->step[i].n=1;a->step[i].note[0]=k==4?36:48+k*2;a->step[i].vel=100;}else a->step[i].time=ST_TIE;
   }
  }
  fixture(argv[1],"ensemble-auto");
 }
done:printf("Break sequence authoring and automation: %s\n",bad?"FAIL":"PASS");return !!bad;
}
