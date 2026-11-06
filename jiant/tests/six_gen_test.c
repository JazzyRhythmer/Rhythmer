/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void){
 draw_splash(); /* Startup must also be safe before tracks, kits and samples exist. */
 ui_power_on();load_depth++;fm6_init();power_on_parts();load_depth--;six_native_init();trk_live=NTRK;
 int bad=0;
 trks_put(1);dice_seq(&trk[4]);dice_seq(&trk[5]);
 const unsigned ac_saved=steps_sig(&trk[4]),br_saved=steps_sig(&trk[5]);
 for(unsigned f=0;f<3;f++)for(unsigned n=0;n<4;n++){
  memset(proj_slot,0,sizeof proj_slot);dice_secs=0;
  gp.form=f;gp.seed=117*n+23;gp.genre=n&1?GEN_EXPLORE:1;
  ui.home=1;ui.confirm=CF_DICE_ALL;gp.page=f;gen_go();
  bad+=check("GEN remains open after generating sections",gen_on()&&ui.home&&gp.page==f);
  bad+=check("GEN keeps T1 drum, T5/6 patterns and six-track mode",trk[0].eng_req==ENGI_DRUM&&steps_sig(&trk[4])==ac_saved&&steps_sig(&trk[5])==br_saved&&six_machine_mask==3);
  for(unsigned k=0;k<4;k++)if(proj_import(&proj_scratch,&proj_slot[k],sizeof proj_slot[k])){
   int ok=!memcmp(proj_scratch.t[4].step,trk[4].step,sizeof trk[4].step)&&!memcmp(proj_scratch.t[5].step,trk[5].step,sizeof trk[5].step);
   bad+=check("generated section preserves T5/6 patterns",ok);
  }
 }
 /* UI reads boundary pitches and all break command classes without altering music. */
 static uint8_t sample[72000];static uint16_t pixels[240*150];
 six_loops[0]=six_loops[1]=sample;six_loop_mask=3;six_loop_now=0;
 trks_put(3);song.sel=SIX_ACID_TRACK;trk[4].p[P_SLEN]=64;
 for(unsigned i=0;i<64;i++){step_clear(&trk[4].step[i]);trk[4].step[i].time=ST_NOTE;trk[4].step[i].n=1;trk[4].step[i].note[0]=i&1?127:0;}
 unsigned before=steps_sig(&trk[4]);ui_prefs|=PREF_ANIM_OFF;six_gen_fx_at=5000;six_gen_fx_kind=2;fm1_ms=5100;
 cv_begin(240,140,T_BG);six_graph_gen();memcpy(pixels,cv_px,240*140*2);
 fm1_ms=5900;cv_begin(240,140,T_BG);six_graph_gen();
 bad+=check("ANIM OFF gives stable acid diagram",!memcmp(pixels,cv_px,240*140*2));
 bad+=check("acid drawing never mutates pattern",before==steps_sig(&trk[4]));
 song.sel=SIX_BREAK_TRACK;trk[5].p[P_LN0]=1;trk[5].p[P_SLEN]=32;
 for(unsigned i=0;i<32;i++){step_clear(&trk[5].step[i]);trk[5].step[i].time=ST_NOTE;trk[5].step[i].n=1;trk[5].step[i].note[0]=i==30?32:i==31?33:i;}
 before=steps_sig(&trk[5]);cv_begin(240,140,T_BG);six_wave_cache(0);six_wave_cache(1);six_graph_break();
 bad+=check("wave cache and chop roll A/B drawing preserve steps",before==steps_sig(&trk[5]));
 for(unsigned k=0;k<6;k++){
  song.sel=k;unsigned before=steps_sig(&trk[k]);
  cv_begin(240,140,T_BG);six_home_cards();
  cv_begin(240,140,T_BG);six_instrument(&trk[k]);
  cv_begin(240,140,T_BG);six_home_routes();
  bad+=check("Rhythmer HOME and EDIT marks preserve all six engines",before==steps_sig(&trk[k]));
 }
 ui_prefs&=~PREF_ANIM_OFF;song.sel=1;track_t *hit=&trk[1];
 hit->xf_on=0;hit->engine=hit->eng_req;hit->p[P_MUTE]=0;trk_hush=0;
 fm1_ms=1000;trk_note_on(hit,60,24);int soft=six_note_punch(hit);
 fm1_ms=1100;trk_note_on(hit,60,127);int hard=six_note_punch(hit);
 bad+=check("note input retriggers a velocity-sensitive visual impulse",hard>soft&&hard==127);
 fm1_ms=1240;int mid=six_note_punch(hit);fm1_ms=1400;
 bad+=check("visual impulse decays independently from held-note sustain",mid>0&&mid<hard&&six_note_punch(hit)==0);
 ui_prefs|=PREF_ANIM_OFF;fm1_ms=1100;
 bad+=check("ANIM OFF suppresses note-punch motion",six_note_punch(hit)==0);
 ui_prefs&=~PREF_ANIM_OFF;draw_splash();splash_sig(0,0);unsigned s0=fnv(2166136261u,host_screen,sizeof host_screen);
 splash_sig(0,400);unsigned s1=fnv(2166136261u,host_screen,sizeof host_screen);
 bad+=check("Rhythmer startup animates inside the existing bounded canvas",s0!=s1&&cv_w*cv_h<=CV_MAX);
 unsigned music=fnv(2166136261u,trk,sizeof trk);
 splash_sig(0,900);s0=fnv(2166136261u,host_screen,sizeof host_screen);
 splash_sig(0,1300);s1=fnv(2166136261u,host_screen,sizeof host_screen);
 bad+=check("startup machine symbols keep animating after the reveal",s0!=s1);
 bad+=check("startup symbols never change track or DSP state",music==fnv(2166136261u,trk,sizeof trk));
 for(unsigned k=0;k<6;k++){
  cv_begin(240,40,T_BG);six_machine_glyph(0,trk_def_engine(k),24,0u,8,4,62,22,T_TEXT);
  memcpy(pixels,cv_px,240*40*2);
  cv_begin(240,40,T_BG);six_machine_glyph(0,trk_def_engine(k),92,0x90000000u,8,4,62,22,T_TEXT);
  bad+=check("each startup symbol uses animated instrument geometry",memcmp(pixels,cv_px,240*40*2)!=0);
  cv_begin(240,140,T_BG);memcpy(pixels,cv_px,240*140*2);
  six_machine_glyph(0,trk_def_engine(k),127,0x90000000u,8,93,30,24,T_TEXT);
  int bounded=1,visible=0;
  for(unsigned y=0;y<140;y++)for(unsigned x=0;x<240;x++)if(pixels[y*240+x]!=cv_px[y*240+x]){
   visible=1;if(x<7||x>39||y<93||y>117)bounded=0;
  }
  bad+=check("compact HOME symbols stay clear of labels and step rail",bounded&&visible);
 }
 ui_prefs|=PREF_ANIM_OFF;splash_sig(0,0);s0=fnv(2166136261u,host_screen,sizeof host_screen);
 splash_sig(0,800);s1=fnv(2166136261u,host_screen,sizeof host_screen);
 bad+=check("ANIM OFF keeps the complete Rhythmer startup stable",s0==s1);
 if(getenv("RHYTHMER_BOOT_PREVIEW")){
  FILE *f=fopen(getenv("RHYTHMER_BOOT_PREVIEW"),"wb");if(!f)return 2;fprintf(f,"P6\n240 240\n255\n");
  for(unsigned j=0;j<240*240;j++){unsigned v=swap16(host_screen[j]);unsigned char rgb[3]={(unsigned char)(((v>>11)&31)*255/31),(unsigned char)(((v>>5)&63)*255/63),(unsigned char)((v&31)*255/31)};fwrite(rgb,1,3,f);}fclose(f);
 }
 printf("SIX GEN forms: %s\n",bad?"FAIL":"PASS");return bad?1:0;
}
