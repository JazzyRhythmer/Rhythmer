/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void){
 int bad=0;ui_power_on();fm6_init();power_on_parts();six_native_init();song_index_load();song_cur=0;chain_defaults(&chain_config);
 for(unsigned section=0;section<4;section++){
  trk[1].step[0].time=ST_NOTE;trk[1].step[0].n=1;trk[1].step[0].note[0]=(uint8_t)(60+section);
  section_store(section);
 }
 project_store_t saved[4];memcpy(saved,proj_slot,sizeof saved);
 chain_config.count=5;unsigned rows[]={0,1,2,1,3};
 for(unsigned i=0;i<5;i++){chain_config.row[i].slot=rows[i];chain_config.row[i].bars=4;song_idx.scene[0][i].on=1;}
 song_idx.rows[1].count=1;song_idx.rows[1].row[0].slot=2;chain_config_t other=song_idx.rows[1];
 live.cur=1;song.g[G_SLOT]=2;go_title("LIB CLEAR");ui.act=3;act_do();
 bad+=check("clear section opens a scope-specific confirmation",ui.confirm==CF_ERASE_SECTION&&ui.confirm_trk==1);
 char a[24],b[24];confirm_text(a,b);bad+=check("dialog identifies the selected stored section",strstr(a,"SECTION B")&&strstr(b,"NO UNDO"));
 press(B_OCTDN);frame();bad+=check("cancel preserves every saved section",!memcmp(saved,proj_slot,sizeof saved));
 go_title("LIB CLEAR");ui.act=3;act_do();press(B_OCTUP);frame();
 bad+=check("confirm clears only selected section B",project_used(0)&&!project_used(1)&&project_used(2)&&project_used(3));
 bad+=check("other saved sections remain byte-identical",!memcmp(&saved[0],&proj_slot[0],sizeof saved[0])&&!memcmp(&saved[2],&proj_slot[2],sizeof saved[2])&&!memcmp(&saved[3],&proj_slot[3],sizeof saved[3]));
 bad+=check("arrangement loses only rows referencing erased section",chain_config.count==3&&chain_config.row[0].slot==0&&chain_config.row[1].slot==2&&chain_config.row[2].slot==3);
 unsigned notes=0;for(unsigned t=0;t<NTRK;t++)for(unsigned i=0;i<NSTEP;i++)notes+=trk[t].step[i].time!=ST_REST;
 bad+=check("currently loaded erased section becomes an empty workspace",!notes&&live.cur<0);
 bad+=check("another project's arrangement is untouched",!memcmp(&other,&song_idx.rows[1],sizeof other));
 project_store_t before[4];memcpy(before,proj_slot,sizeof before);song.playing=1;
 bad+=check("playing blocks persistent library erasure",library_erase(0,0,1)&&!memcmp(before,proj_slot,sizeof before));song.playing=0;
 bad+=check("frozen dialog cannot erase a newly selected project",library_erase(1,0,1)&&!memcmp(before,proj_slot,sizeof before));
 go_title("LIB CLEAR");ui.act=4;act_do();bad+=check("whole-project clear has its own confirmation",ui.confirm==CF_ERASE_PROJECT);press(B_OCTUP);frame();
 bad+=check("project clear empties A-D and song order",!project_used(0)&&!project_used(1)&&!project_used(2)&&!project_used(3)&&!chain_config.count);
 printf("Library clear: %s (%d)\n",bad?"FAIL":"PASS",bad);return bad?1:0;
}
