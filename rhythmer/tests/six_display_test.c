/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(int argc, char **argv) {
 int bad=0; ui_power_on(); fm6_init(); power_on_parts();test_optional_machines(); six_native_init(); trks_put(1);
 bad+=check("HOME GEN opens with FORM LOOP",gp.form==0);
 bad+=check("legacy palette imports once into MONO",palette_from_stored(74)==UI_BW_INDEX&&palette_from_stored(65)==UI_BW_INDEX);
 unsigned saved[2]={steps_sig(&trk[4]),steps_sig(&trk[5])},seen=0;
 for(unsigned genre=0;genre<2;genre++)for(unsigned seed=1;seed<=64;seed++){
  /* Exercise new automatic assignments each time; a previously generated
   * Acid is deliberately preserved by the next ALL transaction. */
  for(unsigned k=1;k<4;k++)if(trk[k].eng_req==ENGI_ACID)set_engine_of(&trk[k],ENGI_ANALOG);
  gp.genre=genre?1:GEN_EXPLORE;gp.form=0;gp.seed=seed*73;ui.home=1;ui.confirm=CF_DICE_ALL;gen_go();
  int ok=trk[0].eng_req==ENGI_DRUM;
  for(unsigned k=1;k<4;k++){unsigned e=trk[k].eng_req;ok&=e!=ENGI_BREAK&&e!=ENGI_DRUM&&(e!=ENGI_ACID||(gs.role[k]==RL_BASS&&gsnd(RL_BASS)==SR_ACID));seen|=1u<<e;}
  if(!ok||saved[0]!=steps_sig(&trk[4])||saved[1]!=steps_sig(&trk[5])){bad+=check("GEN preserves machines 5/6 and melodic engine allowlist",0);break;}
 }
 unsigned newer=(1u<<ENGI_SERUM)|(1u<<ENGI_PHASE)|(1u<<ENGI_LOFI);
 bad+=check("normal and EXPLORE GEN reach the three core added synth models",(seen&newer)==newer);
 bad+=check("automatic GEN never selects the excluded heavy engines",!(seen&((1u<<ENGI_M2STAT)|(1u<<ENGI_M2SAW)|(1u<<ENGI_FM6))));
 ui.home=ui.confirm=0; song.playing=0; ui_prefs&=~PREF_ANIM_OFF;
 for(unsigned p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_GLOBAL&&PAGES[p].id[0]==G_BPM){ui.page=p;break;}
 bad+=check("DISPLAY exposes four colors and three geometric skins",menu_n(MI_COLOR)==4&&menu_n(MI_SKIN)==3);
 static uint16_t image[CV_MAX];
 for(unsigned c=0;c<4;c++)for(unsigned s=0;s<3;s++)for(unsigned style=0;style<2;style++){
  menu_put(MI_COLOR,c);menu_put(MI_SKIN,s);menu_put(MI_STYLE,style);style_apply();
  bad+=check("stored COLOR/SKIN/STYLE survives style_apply",palette_from_stored(palette_to_stored(settings.palette))==settings.palette&&menu_get(MI_COLOR)==c&&menu_get(MI_SKIN)==s&&menu_get(MI_STYLE)==style);
  cv_begin(240,graph_h(),T_BG);fm1_ms=1600;tech_global();memcpy(image,cv_px,cv_w*cv_h*2);
  cv_begin(240,graph_h(),T_BG);fm1_ms=1760;tech_global();
  bad+=check("GLO idle clock moves in every color and skin",memcmp(image,cv_px,cv_w*cv_h*2)!=0);
  ui_prefs|=PREF_ANIM_OFF;cv_begin(240,graph_h(),T_BG);tech_global();memcpy(image,cv_px,cv_w*cv_h*2);
  fm1_ms+=640;cv_begin(240,graph_h(),T_BG);tech_global();
  bad+=check("ANIM OFF holds GLO still in every skin",!memcmp(image,cv_px,cv_w*cv_h*2));ui_prefs&=~PREF_ANIM_OFF;
  if(argc>1&&style==1){char path[1024];snprintf(path,sizeof path,"%s/theme-%u-%u.ppm",argv[1],c,s);ui.force=1;frame();FILE *f=fopen(path,"wb");if(!f)return 2;fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<240*240;i++){unsigned p=swap16(host_screen[i]);unsigned char rgb[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);}
 }
 menu_put(MI_COLOR,0);menu_put(MI_SKIN,1);menu_put(MI_STYLE,1);
 ui.menu=2;ui.menu_scroll=0;ui.force=1;frame();
 if(argc>1){char path[1024];snprintf(path,sizeof path,"%s/about.ppm",argv[1]);FILE *f=fopen(path,"wb");if(!f)return 2;fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<240*240;i++){unsigned p=swap16(host_screen[i]);unsigned char rgb[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);}
 bad+=check("creator document identifies Rhythmer with upstream credits",menu_document(0,0)>MENU_DOC_H);
 printf("Rhythmer display and GEN: %s\n",bad?"FAIL":"PASS");return !!bad;
}
