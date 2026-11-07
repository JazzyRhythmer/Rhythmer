/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static unsigned body(void){return fnv(1,host_screen+84*240,140*240*sizeof *host_screen);}
int main(void){
 ui_power_on();load_depth++;fm6_init();power_on_parts();test_optional_machines();load_depth--;six_native_init();trks_put(1);fm1_ms+=8000;
 song.sel=0;gp.target=2;gp.genre=59;gp.seed=31;gp.energy=55;gp.complex=65;gp.human=70;ui.home=1;ui.confirm=CF_DICE_ALL;gp.page=0;
 unsigned edit=page_first(FAM_EDIT),drum=0;for(unsigned i=0;i<NPAGES;i++)if(!strcmp(PAGES[i].title,"DRUM VOICE"))drum=i;
 ui.page=edit;ui.force=1;frame();unsigned want=body();ui.page=drum;ui.force=1;frame();
 int bad=check("GEN graph does not inherit the remembered Drum sound list",body()==want);
 ui.confirm=0;ui.page=edit;go_home();ui.force=1;frame();want=body();ui.page=drum;go_home();ui.force=1;frame();
 bad+=check("HOME graph does not inherit the remembered Drum sound list",body()==want);
 ui.home=0;ui.page=drum;ui.force=1;frame();bad+=check("Drum sound list remains available in EDIT",body()!=want);
 unsigned seen[NGENRE]={0},pos[3]={NGENRE,NGENRE,NGENRE};
 for(unsigned i=0;i<NGENRE;i++){
  unsigned id=GEN_ORDER[i];if(id<NGENRE)seen[id]++;
  if(!strcmp(GENRE[id].name,"IDM"))pos[0]=i;
  if(!strcmp(GENRE[id].name,"ABSTRACT IDM"))pos[1]=i;
  if(!strcmp(GENRE[id].name,"AMBIENT IDM"))pos[2]=i;
 }
 for(unsigned i=0;i<NGENRE;i++)bad+=check("genre ordering retains every stable ID exactly once",seen[i]==1);
 bad+=check("three IDM genres are adjacent in GEN",pos[1]==pos[0]+1&&pos[2]==pos[1]+1);
 printf("GEN / Drum navigation: %s\n",bad?"FAIL":"PASS");return !!bad;
}
