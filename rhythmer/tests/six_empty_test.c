/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static void settle_empty(void){int32_t out[64];for(unsigned i=0;i<80;i++)mix_block(out,32);}
int main(int argc,char **argv){
 int bad=0;ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(1);song.master_q12=4096;
 bad+=check("fresh T5/T6 have no assigned engines",trk[4].engine==ENGI_EMPTY&&trk[5].engine==ENGI_EMPTY&&six_break_track()<0);
 for(unsigned k=4;k<6;k++){
  for(unsigned n=0;n<128;n++)trk_note_on(&trk[k],n,127);
  for(unsigned v=0;v<NVOICE;v++)bad+=check("EMPTY never allocates a synthesis voice",!trk[k].v[v].active);
  unsigned ph=trk[k].lfo_ph,ph2=trk[k].lfo2_ph;int32_t out[32];uin_on=0;
  mix_part(&trk[k],32);bad+=check("EMPTY skips modulation clocks and engine dispatch",ph==trk[k].lfo_ph&&ph2==trk[k].lfo2_ph&&mc_cost(&trk[k])==0);
 }
 project_capture(&proj_scratch);bad+=check("unassigned tracks round trip without format growth",proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)&&proj_scratch.t[4].engine==ENGI_EMPTY&&proj_scratch.t[5].engine==ENGI_EMPTY);
 for(unsigned k=0;k<NTRK;k++){
  set_engine_of(&trk[k],ENGI_ACID);settle_empty();song.sel=k;go_title("303 GEN 1");
  bad+=check("Acid generator resolves the actual assigned track",six_gen_page());
  six_gen_apply(0,0);trk_note_on(&trk[k],36+k,100);settle_empty();bad+=check("relocated Acid receives notes on its own state",(acid_voice[k].x[1]/16)==36+(int)k);
  set_engine_of(&trk[k],ENGI_EMPTY);settle_empty();
  bad+=check("engine can be cleared back to EMPTY",trk[k].engine==ENGI_EMPTY);
  set_engine_of(&trk[k],ENGI_BREAK);settle_empty();
  bad+=check("Break ownership follows all six track positions",six_break_track()==(int)k);
  set_engine_of(&trk[k],ENGI_EMPTY);settle_empty();bad+=check("clearing Break releases singleton ownership",six_break_track()<0);
 }
 power_on_parts();test_optional_machines();six_native_init();trks_put(0);
 set_engine_of(&trk[1],ENGI_ACID);settle_empty();song.sel=1;go_title("303 GEN 1");song.playing=1;ui.msg[0]=ui.msg2[0]=0;ui.msg_t=0;
 for(unsigned i=0;i<100;i++){trk_note_on(&trk[4],36,127);trk_note_on(&trk[5],0,127);fm1_ms+=80;ui_input();}
 bad+=check("inactive background notes never cover a relocated Acid editor",!strstr(ui.msg,"ASLEEP")&&!strstr(ui.msg2,"6 TRACK")&&!strstr(ui.msg,"INACTIVE")&&song.sel==1&&six_gen_page());
 project_capture(&proj_scratch);unsigned old=proj_scratch.t[4].engine;bad+=check("existing assigned Acid/Break projects remain valid",old==ENGI_ACID&&proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)&&proj_scratch.t[4].engine==ENGI_ACID&&proj_scratch.t[5].engine==ENGI_BREAK);
 song.playing=0;system_init();bad+=check("NEW PROJECT clears optional assignments",trk[4].eng_req==ENGI_EMPTY&&trk[5].eng_req==ENGI_EMPTY);
 trks_put(1);song.sel=4;go_home();ui.home=0;ui.msg_t=0;ui.msg[0]=ui.msg2[0]=0;open_family(FAM_EDIT);ui.force=1;frame();
 if(argc>1){char path[1024];snprintf(path,sizeof path,"%s/empty-track.ppm",argv[1]);FILE*f=fopen(path,"wb");if(!f)return 3;fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<240*240;i++){unsigned q=swap16(host_screen[i]);unsigned char rgb[3]={(q>>11)*255/31,((q>>5)&63)*255/63,(q&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);}
 printf("EMPTY tracks and relocated machine alerts: %s\n",bad?"FAIL":"PASS");return !!bad;
}
