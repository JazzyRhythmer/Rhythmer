/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static int tabs_match(const char *machine){uint8_t cnt[16],gi[16];unsigned g,p,i,m,n=nav_groups(cnt,gi,&g,&p,&i,&m),found=0,other=0;for(unsigned k=0;k<n;k++){const char *name=nav_group_name(gi[k]);found+=!strcmp(name,machine);other+=!strcmp(name,!strcmp(machine,"ACID")?"BREAK":"ACID");}return found==1&&!other&&n==4;}
int main(int argc,char **argv){
 int bad=0;ui_power_on();load_depth++;fm6_init();power_on_parts();test_optional_machines();load_depth--;six_native_init();trks_put(1);
 settings_fresh();rp_apply();bad+=check("fresh persisted settings start with four tracks",trk_live==4&&favorites.factory[15][15]==0x60);
 trks_put(0);cpt_cores=1;rp_apply();bad+=check("CPU-test result cannot overwrite track count",trk_live==4&&favorites.factory[15][15]==0x60&&cpt_cores==1);trks_put(1);
 trks_put(0);unsigned saved_notes=steps_sig(&trk[5]);
 menu_put(MI_TRACKS,1);bad+=check("MENU six-track request waits for confirmation",trk_live==4&&ui.confirm==CF_TRACKS6);
 if(argc>1){ui.force=1;frame();FILE *f=fopen(argv[1],"wb");if(!f)return 2;fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<240*240;i++){unsigned p=swap16(host_screen[i]);unsigned char rgb[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);}
 press(B_OCTDN);bad+=check("OCT minus cancels without enabling tracks",trk_live==4&&!ui.confirm);
 trks_request(1);press(B_OCTUP);bad+=check("OCT plus enables six tracks and retains sequence",trk_live==6&&!ui.confirm&&steps_sig(&trk[5])==saved_notes);
 rp_apply();bad+=check("confirmed six-track choice survives settings apply",trk_live==6);
 favorites.factory[15][15]=0x51;rp_apply();bad+=check("legacy six-track settings migrate safely to four",trk_live==4);trks_put(1);
 for(unsigned pass=0;pass<2;pass++){
  unsigned k=pass?0:4;song.sel=k;if(pass)set_engine_of(TSEL,ENGI_ACID);go_title("EDIT 1");frame();
  press(B_EDIT);bad+=check("EDIT from Acid SOUND opens ACID / GEN 1",six_gen_page()&&!strcmp(cur_page()->title,"303 GEN 1")&&tabs_match("ACID"));
  press(B_EDIT);bad+=check("repeated EDIT reaches ACID / GEN 2",!strcmp(cur_page()->title,"303 GEN 2")&&tabs_match("ACID"));
  press(B_EDIT);bad+=check("EDIT leaves final Acid action for TONE",!strcmp(nav_group_name(nav_ch(SEC_EDIT)->e[nav_cur()].g),"TONE"));
  press(B_EDIT);bad+=check("Acid EDIT reaches FOCUS",!strcmp(cur_page()->title,"FOCUS"));press(B_EDIT);bad+=check("Acid EDIT wraps to SOUND",!strcmp(cur_page()->title,"EDIT 1"));
  nav_select(2);bad+=check("SELECT still reaches GEN 1",!strcmp(cur_page()->title,"303 GEN 1"));nav_select(1);bad+=check("SELECT still reaches GEN 2",!strcmp(cur_page()->title,"303 GEN 2"));
 }
 for(unsigned pass=0;pass<2;pass++){
  unsigned k=pass?1:5;if(pass)set_engine_of(&trk[5],ENGI_SERUM);song.sel=k;if(pass)set_engine_of(TSEL,ENGI_BREAK);go_title("EDIT 1");frame();
  press(B_EDIT);bad+=check("EDIT from Break SOUND opens BREAK / PERF",six_perf_page()&&tabs_match("BREAK"));
  press(B_EDIT);bad+=check("repeated EDIT reaches BREAK / SEQ",!strcmp(cur_page()->title,"BRK SEQ")&&tabs_match("BREAK"));
  press(B_EDIT);bad+=check("repeated EDIT reaches BREAK / RETURN",six_return_page()&&tabs_match("BREAK"));
  press(B_EDIT);bad+=check("EDIT leaves final Break action for TONE",!strcmp(nav_group_name(nav_ch(SEC_EDIT)->e[nav_cur()].g),"TONE"));
  press(B_EDIT);press(B_EDIT);bad+=check("Break EDIT wraps back to SOUND",!strcmp(cur_page()->title,"EDIT 1"));nav_select(2);bad+=check("SELECT still reaches BRK PERF",six_perf_page());
 }
 song.sel=2;go_title("EDIT 1");frame();uint8_t cnt[16],gi[16];unsigned g,p,i,m,n=nav_groups(cnt,gi,&g,&p,&i,&m);bad+=check("normal synth tabs do not expose ACID or BREAK",n==3);
 unsigned before=steps_sig(TSEL);press(B_EDIT);bad+=check("normal synth EDIT still jumps to TONE",!strcmp(nav_group_name(nav_ch(SEC_EDIT)->e[nav_cur()].g),"TONE")&&steps_sig(TSEL)==before);
 printf("SIX machine navigation: %s (%d failures)\n",bad?"FAIL":"PASS",bad);return bad?1:0;
}
