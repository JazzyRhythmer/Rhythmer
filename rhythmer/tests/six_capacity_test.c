/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(int argc,char **argv){
 if(argc<2)return 2;ui_power_on();load_depth++;fm6_init();power_on_parts();test_optional_machines();load_depth--;six_native_init();trks_put(1);song.master_q12=4096;
 const unsigned model[]={ENGI_WAVETABLE,ENGI_FM6,ENGI_PHASE,ENGI_M2STAT,ENGI_M2SAW,ENGI_M2SID,ENGI_DRUM};
 for(unsigned m=0;m<sizeof model/sizeof *model;m++){
  for(unsigned k=0;k<6;k++)set_engine_of(&trk[k],ENGI_ANALOG);
  for(unsigned k=0;k<6;k++){
   track_t *t=&trk[k];set_engine_of(t,model[m]);t->engine=t->eng_req;t->xf_on=t->xf=0;
   apply_preset_to(t,0);t->p[P_SLEN]=16;t->p[P_SGATE]=127;
   t->p[P_DIST]=60;t->p[P_CHOR]=90;t->p[P_DLY]=80;t->p[P_REV]=80;
   t->p[P_M1SRC]=MS_LFO;t->p[P_M1DST]=MD_PAN;t->p[P_M1AMT]=24;t->p[P_M2SRC]=MS_LFO2;t->p[P_M2DST]=MD_CUT;t->p[P_M2AMT]=24;
   for(unsigned i=0;i<NSTEP;i++){step_t *s=&t->step[i];step_clear(s);if(i<16){s->time=(i%2)?ST_TIE:ST_NOTE;if(!(i%2)){s->n=4;s->vel=127;for(unsigned n=0;n<4;n++)s->note[n]=(uint8_t)(36+k*2+n*4);}}}
  }
  song.g[G_BPM]=240;song.sel=0;project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch)){printf("capacity fixture pack failed %u\n",model[m]);return 3;}
  char path[1024];snprintf(path,sizeof path,"%s/six-capacity-%u.project",argv[1],model[m]);FILE *f=fopen(path,"wb");if(!f)return 4;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 for(unsigned model=0;model<MD_NMODEL;model++){
  for(unsigned k=0;k<6;k++){
   track_t *t=&trk[k];set_engine_of(t,ENGI_DRUM);t->engine=ENGI_DRUM;t->xf=t->xf_on=0;apply_preset_to(t,11);
   t->p[P_MUTE]=0;t->p[P_SLEN]=16;t->p[P_SGATE]=127;t->p[P_DIST]=60;t->p[P_CHOR]=90;t->p[P_DLY]=80;t->p[P_REV]=80;
   for(unsigned i=0;i<NSTEP;i++){step_t *s=&t->step[i];step_clear(s);if(i<16){s->time=ST_NOTE;s->n=4;s->vel=127;for(unsigned j=0;j<4;j++)s->note[j]=DRUM_LANE_NOTE[j*2];}}
  }
  for(unsigned j=0;j<8;j++)md_patch_default(&dx_kit[j],model);
  song.g[G_BPM]=240;project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))return 12;
  char path[1024];snprintf(path,sizeof path,"%s/six-capacity-md-model-%u.project",argv[1],model);FILE *f=fopen(path,"wb");if(!f)return 13;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 for(unsigned variant=0;variant<4;variant++){
  unsigned model=(unsigned[]){2,7,9,12}[variant];
  for(unsigned k=0;k<6;k++){set_engine_of(&trk[k],ENGI_DRUM);trk[k].engine=ENGI_DRUM;trk[k].xf=trk[k].xf_on=0;apply_preset_to(&trk[k],11);trk[k].p[P_MUTE]=k!=0;}
  for(unsigned j=0;j<8;j++){md_patch_default(&dx_kit[j],model);dx_kit[j].a[2]=dx_kit[j].b[2]=100;}
  track_t *t=&trk[0];t->p[P_SLEN]=16;t->p[P_SGATE]=127;t->p[P_DIST]=60;t->p[P_CHOR]=90;t->p[P_DLY]=80;t->p[P_REV]=80;
  for(unsigned i=0;i<NSTEP;i++){step_t *s=&t->step[i];step_clear(s);if(i<16){s->time=ST_NOTE;s->n=4;s->vel=127;for(unsigned j=0;j<4;j++)s->note[j]=DRUM_LANE_NOTE[j*2];}}
  song.g[G_BPM]=240;project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))return 14;
  char path[1024];snprintf(path,sizeof path,"%s/six-capacity-md-single-%u.project",argv[1],model);FILE *f=fopen(path,"wb");if(!f)return 15;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 for(unsigned variant=0;variant<2;variant++){
  for(unsigned k=0;k<6;k++){
   track_t *t=&trk[k];set_engine_of(t,ENGI_DRUM);t->engine=ENGI_DRUM;t->xf=t->xf_on=0;apply_preset_to(t,9+variant);
   t->p[P_MUTE]=0;t->p[P_SLEN]=16;t->p[P_SGATE]=127;t->p[P_DIST]=60;t->p[P_CHOR]=90;t->p[P_DLY]=80;t->p[P_REV]=80;
   for(unsigned i=0;i<NSTEP;i++){step_t *s=&t->step[i];step_clear(s);if(i<16){s->time=ST_NOTE;s->n=4;s->vel=127;for(unsigned j=0;j<4;j++)s->note[j]=DRUM_LANE_NOTE[3+j+(j==3)];}}
  }
  dx_kit_load(9+variant);
  song.g[G_BPM]=240;project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))return 10;
  char path[1024];snprintf(path,sizeof path,"%s/six-capacity-md-%u.project",argv[1],variant);FILE *f=fopen(path,"wb");if(!f)return 11;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 for(unsigned k=0;k<6;k++){
  track_t *t=&trk[k];set_engine_of(t,ENGI_DRUM);t->engine=ENGI_DRUM;apply_preset_to(t,11);t->xf=t->xf_on=0;t->p[P_MUTE]=0;t->p[P_SLEN]=16;t->p[P_SGATE]=127;
  t->p[P_DIST]=60;t->p[P_CHOR]=90;t->p[P_DLY]=80;t->p[P_REV]=80;
  t->p[P_M1SRC]=MS_LFO;t->p[P_M1DST]=MD_PAN;t->p[P_M1AMT]=24;t->p[P_M2SRC]=MS_LFO2;t->p[P_M2DST]=MD_CUT;t->p[P_M2AMT]=24;
  for(unsigned i=0;i<NSTEP;i++){step_t *s=&t->step[i];step_clear(s);if(i<16){s->time=ST_NOTE;s->n=4;s->vel=127;for(unsigned j=0;j<4;j++)s->note[j]=DRUM_LANE_NOTE[j*2];}}
 }
 for(unsigned j=0;j<8;j++)md_patch_default(&dx_kit[j],9);
 song.g[G_BPM]=240;project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))return 16;
 {char path[1024];snprintf(path,sizeof path,"%s/six-capacity-md-lfo.project",argv[1]);FILE *f=fopen(path,"wb");if(!f)return 17;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);}
 for(unsigned k=0;k<6;k++){set_engine_of(&trk[k],ENGI_ANALOG);trk[k].p[P_MUTE]=1;}
 song.sel=0;set_engine_of(&trk[0],ENGI_BREAK);trk[0].engine=ENGI_BREAK;trk[0].xf=trk[0].xf_on=0;apply_preset_to(&trk[0],0);trk[0].p[P_MUTE]=0;trk[0].p[P_LN0]=1;trk[0].p[P_SLEN]=32;trk[0].p[P_SGATE]=127;
 for(unsigned i=0;i<NSTEP;i++){step_t *s=&trk[0].step[i];step_clear(s);if(i<32){s->time=(i&1)?ST_TIE:ST_NOTE;if(!(i&1)){s->n=1;s->note[0]=i/2;s->vel=127;}}}
 for(unsigned variant=0;variant<2;variant++){
  if(variant){trk[0].p[P_LN0]=0;for(unsigned i=0;i<NSTEP;i++)step_clear(&trk[0].step[i]);}
  project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))return 6;char path[1024];snprintf(path,sizeof path,"%s/six-capacity-break-%s.project",argv[1],variant?"empty":"only");FILE *f=fopen(path,"wb");if(!f)return 7;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 apply_preset_to(&trk[0],1);trk[0].p[P_LN0]=1;song.g[G_BPM]=160;
 for(unsigned variant=0;variant<2;variant++){
  for(unsigned i=0;i<NSTEP;i++){step_t *s=&trk[0].step[i];step_clear(s);if(i<32){s->time=(i&1)?ST_TIE:ST_NOTE;if(!(i&1)){s->n=variant?2:1;s->note[0]=i/2;if(variant)s->note[1]=i<16?32:33;s->vel=127;}}}
  project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))return 8;char path[1024];snprintf(path,sizeof path,"%s/think-%s.project",argv[1],variant?"ab-switch":"b-seq");FILE *f=fopen(path,"wb");if(!f)return 9;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 return 0;
}
