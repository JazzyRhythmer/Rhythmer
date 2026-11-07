/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#define RHYTHMER_MD_TEST 1
#include "ui_test.c"
int main(int argc,char **argv){int bad=0;ui_power_on();fm6_init();power_on_parts();test_optional_machines();six_native_init();trks_put(1);song.sel=0;
 bad+=check("all 55 MD factory sounds and 57 articulations present",MD_NSOUND==55&&MD_NMODEL==57&&DX_VOICES==127&&DX_NKIT==19);
 for(unsigned lane=0;lane<8;lane++){
  dx_ui_lane=lane;
  for(unsigned sound=0;sound<MD_NSOUND;sound++){
   dx_lane_t before[8];memcpy(before,dx_kit,sizeof before);unsigned hi=dx_md_hi;
   dx_voice_pick(72+sound);unsigned kind=md_kind(&dx_kit[lane]);int intact=1;
   for(unsigned j=0;j<8;j++)if(j!=lane)intact&=!memcmp(&before[j],&dx_kit[j],sizeof before[j]);
   intact&=!((hi^dx_md_hi)&~(1u<<lane));
   if(!intact||kind!=MD_SOUNDS[sound].kind){puts("lane selection corrupted another sound");return 3;}
   /* Reconstruct the exact production macro adapter, including source tuning. */
   int16_t actual[8];const md_model_t *m=&MD_MODELS[kind];memcpy(actual,m->def,sizeof actual);const dx_lane_t *p=&dx_kit[lane];
   actual[0]=(int16_t)((p->a[0]-64)*3/4);actual[1]=p->a[2];actual[m->ctl[0]]=p->a[1];actual[m->ctl[1]]=p->a[3];actual[m->ctl[2]]=p->a[4];
   if(memcmp(actual,m->def,sizeof actual)){puts("factory parameters not reproduced");return 4;}
   project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch)||!proj_import(&proj_scratch,&proj_wire,sizeof proj_wire))return 5;
   dx_md_hi=0;memset(dx_kit,0,sizeof dx_kit);if(project_restore_runtime(&proj_scratch)||md_kind(&dx_kit[lane])!=kind)return 6;
  }
 }
 bad+=check("every MD sound independently selects and round trips on every lane",1);
 for(unsigned ki=0;ki<DX_NKIT;ki++){
  dx_kit_load(ki);if(!dx_kit_ok(dx_kit)||!md_ext_ok(dx_kit,dx_md_hi))return 7;
  project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch)||!proj_import(&proj_scratch,&proj_wire,sizeof proj_wire))return 8;
  for(unsigned l=0;l<8;l++)if(ki<9&&l==3&&(DX_KITS[ki].lay[l]||DX_KITS[ki].lane[l].a[2]>24||DX_KITS[ki].lane[l].a[3]<96))return 9;
 }
 bad+=check("conventional kits use short noise-rich closed hats without extra layers",1);
 song.sel=0;go_home();int reachable=0;for(unsigned i=0;i<NPAGES*2;i++){press(B_EDIT);frame();if(!strcmp(cur_page()->title,"DRUM VOICE")){reachable=1;break;}}bad+=check("repeated EDIT reaches DRUM VOICE through physical navigation",reachable);
 dx_kit_load(0);dx_ui_seen=last_note;dx_ui_lane=3;go_title("DRUM VOICE");ui.force=1;frame();
 bad+=check("dedicated sound page is visible on Drum",!strcmp(cur_page()->title,"DRUM VOICE")&&dx_pagen(cur_page())==9);
 for(unsigned bank=0;bank<4;bank++){
  dx_voice_bank=bank;unsigned n=dx_voice_count();int unique[DX_VOICES]={0};
  for(unsigned i=0;i<n;i++){unsigned id=dx_voice_at(i);if(id>=DX_VOICES||unique[id]++||!dx_voice_accept(id))return 10;dx_voice_pick(id);if(dx_voice_current()!=(int)id&&id>=72)return 11;}
 }
 bad+=check("ALL, KITS, MD TRX and MD EFM browse complete filtered lists",1);
 dx_voice_bank=2;dx_voice_pick(72);dx_ui_lane=3;int before=dx_voice_pos();dx_knob(cur_page(),2,1);bad+=check("physical sound knob loads the next actual source",dx_voice_pos()==before+1);
 unsigned voice_count=voices_busy();dx_knob(cur_page(),3,1);bad+=check("level/browsing do not allocate audio voices",voices_busy()==voice_count);
 for(unsigned k=4;k<6;k++){song.sel=k;unsigned visible=0;for(unsigned i=0;i<NPAGES;i++)if(!strcmp(PAGES[i].title,"DRUM VOICE"))visible=page_visible(i);bad+=check("Drum voice page is hidden on Acid and Break",!visible);}
 song.sel=0;dx_voice_bank=0;dx_ui_lane=3;dx_voice_pick(72+49);go_title("DRUM VOICE");fm1_ms+=5000;ui.force=1;frame();
 if(argc>1){FILE *f=fopen(argv[1],"wb");if(!f)return 12;fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<57600;i++){unsigned p=swap16(host_screen[i]);unsigned char c[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(c,1,3,f);}fclose(f);}
 printf("Drum named sound selection: %s (%d)\n",bad?"FAIL":"PASS",bad);return !!bad;
}
