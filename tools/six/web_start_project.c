/* SPDX-License-Identifier: GPL-3.0-only */
/* Website starting project; separate from the populated DSP test fixtures. */
#define UI_TEST_NO_MAIN 1
#include "../../rhythmer/tests/ui_test.c"
int main(int argc,char **argv){
 if(argc!=2)return 1;
 ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(0);
 for(unsigned k=0;k<6;k++){
  track_t *t=&trk[k];motion_clear(t);t->p[P_MUTE]=0;
  for(unsigned i=0;i<NSTEP;i++)step_clear(&t->step[i]);
  t->p[P_SLEN]=16;
 }
 /* All sequences are empty; extra slots have no assigned engine. */
 song.sel=0;song.g[G_BPM]=160;song.playing=0;
 project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))return 2;
 if(!proj_import(&proj_scratch,&proj_wire,sizeof proj_wire))return 3;
 for(unsigned k=0;k<6;k++){
  unsigned notes=0;for(unsigned i=0;i<NSTEP;i++)notes+=proj_scratch.t[k].step[i].time==ST_NOTE&&proj_scratch.t[k].step[i].n;
  if(notes)return 4;
 }
 FILE *f=fopen(argv[1],"wb");if(!f)return 5;
 if(fwrite(&proj_wire,1,sizeof proj_wire,f)!=sizeof proj_wire)return 6;
 fclose(f);puts("Web start: all six tracks empty, T5/T6 EMPTY, no assigned Acid/Break PASS");return 0;
}
