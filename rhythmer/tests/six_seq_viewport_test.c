/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static int bad;
static void expect(const char *name,int ok){printf("%s %s\n",ok?"PASS":"FAIL",name);bad+=!ok;}
static void note_at(track_t *t,unsigned step,unsigned note){step_clear(&t->step[step]);t->step[step].time=ST_NOTE;t->step[step].n=1;t->step[step].note[0]=(uint8_t)note;t->step[step].vel=100;}
static int visible(unsigned note){int y=pr_row_y(note);return y>=PR_Y0&&y+PR_RH<=PR_Y0+PR_ROWS*PR_RH;}
static void save_project(const char *dir,const char *name){char p[1024];snprintf(p,sizeof p,"%s/%s.project",dir,name);project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))exit(3);FILE *f=fopen(p,"wb");if(!f)exit(2);fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);}
static void save(const char *dir,const char *name){char p[1024];snprintf(p,sizeof p,"%s/%s.ppm",dir,name);FILE *f=fopen(p,"wb");if(!f)exit(2);fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<57600;i++){unsigned p=swap16(host_screen[i]);unsigned char b[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(b,1,3,f);}fclose(f);}
int main(int argc,char **argv){
 ui_power_on();fm6_init();load_depth++;power_on_parts();load_depth--;six_native_init();trks_put(1);
 song.sel=4;song.playing=0;for(unsigned t=0;t<6;t++)engine_block(&trk[t]);
 track_t *t=&trk[4];for(unsigned i=0;i<NSTEP;i++)step_clear(&t->step[i]);t->p[P_SLEN]=32;note_at(t,0,36);note_at(t,3,43);note_at(t,16,96);note_at(t,20,103);
 press(B_SEQ);ui.bank=ui.cursor=0;proll.init=0;ui.force=1;frame();
 expect("opening SEQ fits low Acid notes in the actual graph path",proll.init&&visible(36)&&visible(43));if(argc>1){save(argv[1],"acid-low");save_project(argv[1],"acid-low");}
 ui.bank=1;ui.cursor=16;ui.force=1;frame();expect("changing bars fits upper-octave notes",visible(96)&&visible(103));if(argc>1)save(argv[1],"upper-bar");
 song.sel=1;t=&trk[1];for(unsigned i=0;i<NSTEP;i++)step_clear(&t->step[i]);t->p[P_SLEN]=16;note_at(t,0,108);note_at(t,3,108);ui.bank=ui.cursor=0;ui.force=1;frame();expect("track switch fits high synth notes",proll.trk==1&&visible(108));if(argc>1)save_project(argv[1],"synth-high");
 note_at(t,0,24);note_at(t,3,24);ui.force=0;for(unsigned i=0;i<160;i++)frame();expect("stopped note edit finishes register movement through the graph cache",visible(24)&&!proll.moving);if(argc>1)save(argv[1],"edited-low");
 note_at(t,0,0);ui.force=1;frame();expect("lowest MIDI note stays inside graph",visible(0));note_at(t,0,127);ui.force=1;frame();expect("highest MIDI note stays inside graph",visible(127));
 song.sel=5;t=&trk[5];for(unsigned i=0;i<NSTEP;i++)step_clear(&t->step[i]);note_at(t,0,0);note_at(t,2,7);ui.bank=ui.cursor=0;ui.force=1;frame();expect("Break command notes follow their own low register",proll.trk==5&&visible(0)&&visible(7));if(argc>1)save(argv[1],"break-command");
 puts(bad?"SEQ viewport regression: FAIL":"SEQ viewport regression: PASS");return !!bad;
}
