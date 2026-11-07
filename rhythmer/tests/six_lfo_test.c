/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static void capture(const char *dir,const char *name){
 if(!dir)return;char path[1024];snprintf(path,sizeof path,"%s/lfo-%s.ppm",dir,name);
 ui.force=1;frame();FILE *f=fopen(path,"wb");if(!f)exit(2);
 fprintf(f,"P6\n240 240\n255\n");
 for(unsigned i=0;i<240*240;i++){unsigned p=swap16(host_screen[i]);unsigned char rgb[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);
}
int main(int argc,char **argv){
 int bad=0;ui_power_on();fm6_init();power_on_parts();test_optional_machines();six_native_init();trks_put(1);
 const char *dir=argc>1?argv[1]:0;
 for(unsigned k=0;k<NTRK;k++){
  song.sel=k;track_t *t=TSEL;
  t->p[P_LRATE]=12+k*7;t->p[P_L2RATE]=75+k*6;
  t->p[P_LWAVE]=1;t->p[P_L2WAVE]=3;t->p[P_LPHASE]=17+k;t->p[P_L2PHASE]=43+k;
  t->p[P_LFADE]=t->p[P_L2FADE]=0;t->lfo_ph=t->lfo2_ph=0;
  uint32_t other=trk[(k+1)%NTRK].lfo2_ph;
  track_lfo_tick(t);six_lfo2_tick(t);
  bad+=check("each track has two independent clocks and no cross-track phase writes",t->lfo_ph&&t->lfo2_ph&&t->lfo_ph!=t->lfo2_ph&&trk[(k+1)%NTRK].lfo2_ph==other);
  t->lfo_fade=12345;t->lfo2_fade=23456;
  go_title("LFO 2");frame();turn(EN_K3,1);
  bad+=check("LFO 2 MODE selects FREE without resetting LFO 1",t->p[P_L2PHASE]==-1&&t->p[P_LPHASE]==17+(int)k&&t->lfo_fade==12345&&t->lfo2_fade==0);
  go_title("LFO 2 PHASE");frame();turn(EN_K1,1);
  bad+=check("LFO 2 phase edits in FREE preserve its running mode",t->p[P_L2PHASE]==-1);
  go_title("LFO 2");frame();turn(EN_K3,-1);
  bad+=check("LFO 2 returns to its own remembered TRIG phase",t->p[P_L2PHASE]==44+(int)k&&t->p[P_LPHASE]==17+(int)k);
  go_title("LFO");frame();turn(EN_K3,1);
  bad+=check("LFO 1 FREE does not reset or edit LFO 2",t->p[P_LPHASE]==-1&&t->p[P_L2PHASE]==44+(int)k);
  turn(EN_K3,-1);
  go_title("MOD");frame();turn(EN_K1,3); /* The slot picker advances one slot per turn. */
  turn(EN_K1,1);turn(EN_K1,1);
  for(unsigned slot=0;slot<4;slot++){
   mod_ui_slot=slot;t->p[P_M1SRC+3*slot]=MS_LFO;frame();turn(EN_K2,1);turn(EN_K3,1);turn(EN_K4,20);
   bad+=check("all four slots expose LFO 2 as an assignable source",t->p[P_M1SRC+3*slot]==MS_LFO2);
  }
  int16_t matrix[12];memcpy(matrix,&t->p[P_M1SRC],sizeof matrix);
  go_title("LFO 2 DEST");frame();
  bad+=check("new LFO 2 direct depths start at zero",lfo2_dest_value(t,0)==0&&lfo2_dest_value(t,1)==0&&lfo2_dest_value(t,2)==0&&lfo2_dest_value(t,3)==0);
  turn(EN_K1,12);turn(EN_K2,-13);turn(EN_K3,14);turn(EN_K4,80);
  bad+=check("LFO 2 DEST edits four dedicated depths without consuming MOD slots",lfo2_dest_value(t,0)>0&&lfo2_dest_value(t,1)<0&&lfo2_dest_value(t,2)>0&&lfo2_dest_value(t,3)>0&&!memcmp(matrix,&t->p[P_M1SRC],sizeof matrix));
  if(!k)capture(dir,"two-dest");
  up_store(k,"DUAL LFO");lfo2_dest_put(t,0,0);up_load(k);
  bad+=check("user sound preset restores independent LFO2 direct depths",lfo2_dest_value(t,0)>0&&lfo2_dest_value(t,1)<0&&lfo2_dest_value(t,3)>0);
  /* Avoid mode/rate edits from preset tests changing the expected project values. */
 }
 project_capture(&proj_scratch);bad+=check("six independent dual-LFO tracks pack in the existing project format",proj_pack(&proj_wire,&proj_scratch));
 memset(&proj_scratch,0,sizeof proj_scratch);bad+=check("dual-LFO project imports",proj_import(&proj_scratch,&proj_wire,sizeof proj_wire));
 for(unsigned k=0;k<NTRK;k++)bad+=check("project retains both rates, waves, phases and four LFO2 routes on every track",proj_scratch.t[k].p[P_LRATE]==12+(int)k*7&&proj_scratch.t[k].p[P_L2RATE]==75+(int)k*6&&proj_scratch.t[k].p[P_L2WAVE]==3&&proj_scratch.t[k].p[P_LPHASE]==17+(int)k&&proj_scratch.t[k].p[P_L2PHASE]==44+(int)k&&proj_scratch.t[k].p[P_M4SRC]==MS_LFO2);
 bad+=check("all six dedicated destination states are tagged in the project",proj_scratch.lfo2_mask==63);
 bad+=check("actual project restore retains direct depths and unified HUMAN",!project_restore_runtime(&proj_scratch)&&lfo2_dest_value(&trk[0],0)>0&&lfo2_dest_value(&trk[5],1)<0&&trk[0].p[P_HVEL]==127);
 project_capture(&proj_scratch);proj_scratch.lfo2_mask=0;proj_scratch.sum=proj_sum(&proj_scratch);
 bad+=check("untagged old project never activates reclaimed raw values",!project_restore_runtime(&proj_scratch)&&!lfo2_dest_active(&trk[0]));
 bad+=check("SRC display order preserves all historical source IDs",param_turn(&TP[P_M1SRC],MS_OFF,1)==MS_LFO&&param_turn(&TP[P_M1SRC],MS_LFO,1)==MS_LFO2&&param_turn(&TP[P_M1SRC],MS_LFO2,1)==MS_ENV&&MS_LFO2==15&&MS_ENV==2);
 track_t *probe=&trk[1];for(unsigned j=0;j<12;j++)probe->p[P_M1SRC+j]=0;
 probe->lfo2_val=16384;probe->lfo2_fade=32767;
 lfo2_dest_put(probe,0,16);lfo2_dest_put(probe,1,-17);lfo2_dest_put(probe,2,18);lfo2_dest_put(probe,3,100);
 bad+=check("dedicated LFO2 depths work with all four MOD slots empty",mod_begin(probe)&&MOD.pit>0&&MOD.cut<0&&MOD.shp>0&&MOD.gain<32767&&MOD.nv==0&&MOD.nk==0);mod_end(probe);
 for(unsigned j=0;j<4;j++)lfo2_dest_put(probe,j,0);
 bad+=check("zero direct depths retain the no-modulation fast path",!mod_begin(probe));
 song.sel=1;go_title("MOD");frame();capture(dir,"mod");
 press(B_LFO);bad+=check("repeated LFO button reaches LFO 1 tab",!strcmp(cur_page()->title,"LFO"));capture(dir,"one");
 press(B_LFO);bad+=check("repeated LFO button reaches LFO 2 tab",!strcmp(cur_page()->title,"LFO 2"));capture(dir,"two");
 nav_select(1);bad+=check("SELECT reaches separate LFO 2 PHASE settings",!strcmp(cur_page()->title,"LFO 2 PHASE"));capture(dir,"two-phase");
 nav_select(1);bad+=check("SELECT reaches LFO 2 DEST in its own tab",!strcmp(cur_page()->title,"LFO 2 DEST"));
 press(B_LFO);bad+=check("repeated LFO button reaches MSEQ then MOD",!strcmp(cur_page()->title,"MSEQ"));press(B_LFO);bad+=check("LFO tabs wrap to MOD",cur_page()->graph==GR_MOD);
 if(argc>2){
  FILE *f=fopen(argv[2],"rb");if(!f||fread(&proj_wire,1,sizeof proj_wire,f)!=sizeof proj_wire)return 3;fclose(f);
  if(!proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)||project_restore_runtime(&proj_scratch))return 4;
  for(unsigned k=0;k<NTRK;k++){track_t *t=&trk[k];t->p[P_L2RATE]=30+k*9;for(unsigned j=0;j<4;j++)lfo2_dest_put(t,j,j==3?16:6+j*2);}
  project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))return 5;
  char p[1024];snprintf(p,sizeof p,"%s/upgrade-lfo2-direct.project",argv[1]);f=fopen(p,"wb");if(!f)return 6;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 printf("Rhythmer dual LFO: %s (%d failures)\n",bad?"FAIL":"PASS",bad);return !!bad;
}
