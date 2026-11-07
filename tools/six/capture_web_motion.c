/* SPDX-License-Identifier: GPL-3.0-only
 * Host-render the unchanged firmware UI with real note/envelope input.
 * These are UI demonstrations, not physical-device captures or CPU tests. */
#define UI_TEST_NO_MAIN 1
#include "../../rhythmer/tests/ui_test.c"
static unsigned char banks[2][81920];
static void write_frame(FILE *f){
 for(unsigned i=0;i<240*240;i++){
  unsigned p=swap16(host_screen[i]);
  unsigned char rgb[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};
  fwrite(rgb,1,3,f);
 }
}
int main(int argc,char **argv){
 if(argc!=4)return 2;
 for(unsigned i=0;i<2;i++){FILE *f=fopen(argv[2+i],"rb");if(!f||fread(banks[i],1,81920,f)!=81920)return 3;fclose(f);}
 ui_power_on();fm6_init();load_depth++;power_on_parts();load_depth--;six_native_init();trk_live=6;
 menu_put(MI_COLOR,0);menu_put(MI_SKIN,1);menu_put(MI_STYLE,1);
 static const unsigned engines[]={ENGI_DRUM,ENGI_SERUM,ENGI_PHASE,ENGI_LOFI,ENGI_ANALOG,ENGI_FM6,ENGI_WHEEL,ENGI_M2STAT,ENGI_M2SAW,ENGI_M2SID,ENGI_ACID,ENGI_BREAK,ENGI_ACID,ENGI_BREAK};
 static const char *names[]={"drum","serum","phase","lofi","analog","fm6","wheel","fmstat","swavsaw","sid","acid","break","acid-generate","break-perform"};
 for(unsigned k=0;k<sizeof engines/sizeof engines[0];k++){
  const char *only=getenv("RHYTHMER_CAPTURE_ONLY");if(only&&strcmp(only,names[k]))continue;
  unsigned e=engines[k],ti=e==ENGI_DRUM?0:e==ENGI_BREAK?5:e==ENGI_ACID?4:1;
  song.playing=0;song.sel=(uint8_t)ti;track_t *t=&trk[ti];set_engine_of(t,e);apply_preset_to(t,0);panic_req=0;if(e!=ENGI_DRUM&&e!=ENGI_BREAK&&e!=ENGI_ACID){t->p[P_ATK]=0;t->p[P_DEC]=25;t->p[P_SUS]=40;t->p[P_REL]=10;}
  int32_t audio[CTL];memset(t->v,0,sizeof t->v);t->xf_on=0;engine_block(t);track_render(t,audio,CTL);if(t->engine!=e)return 5;ui.home=ui.menu=ui.confirm=ui.layer=ui.ly=0;
  for(unsigned i=0;i<NPAGES;i++)if((k==12?PAGES[i].graph==GR_SIXGEN:k==13?PAGES[i].graph==GR_SIXBRK:PAGES[i].fam==FAM_EDIT&&PAGES[i].scope==SC_ENGINE&&PAGES[i].id[0]==P_E0)){ui.page=(uint8_t)i;break;}
  if(e==ENGI_BREAK){six_loop_mask=3;six_loop_now=0;for(unsigned i=0;i<2;i++)six_loops[i]=banks[i]+512;jungle_bind(six_loops[0],72000,48000);six_break_owner=ti;break_default_sequence(t);jungle_sequence_mode(1);jungle_play(1);six_wave_src[0]=six_wave_src[1]=0;six_wave_cache(0);six_wave_cache(1);}
  if(e==ENGI_ACID)six_gen_apply_to(t,0,1);
  song.playing=1;t->p[P_MUTE]=0;
  char path[1024];snprintf(path,sizeof path,"%s/%s.rgb",argv[1],names[k]);FILE *f=fopen(path,"wb");if(!f)return 4;
  unsigned previous=0;
  for(unsigned n=0;n<48;n++){
   if(e==ENGI_FM6&&n%8==0){apply_preset_to(t,n/8);panic_req=0;}
   fm1_ms+=64;t->seq_idx=(uint8_t)(n*2/3%32);
   if(k==12&&(n==0||n==24)){six_gen_apply_to(t,n!=0,1);}
   if(e==ENGI_BREAK){if(n==24)six_break_select(1,1);if(k==13&&n%12==4)jungle_roll(4+(n/12)%3);if(n%12==10)jungle_roll_off();}
   if(n%4==0){unsigned note=e==ENGI_DRUM?36+(n/4)%8:e==ENGI_BREAK?(n/4)%16:36+(n/4)%7;previous=note;seq_note_on(t,note,70+(n/4*17)%58);}
   if(n%4==2)seq_note_off(t,previous);
   for(unsigned b=0;b<110;b++){engine_block(t);track_lfo_tick(t);six_lfo2_tick(t);if(e==ENGI_BREAK)six_break_clock(CTL,CTL,beat_samples());mod_begin(t);track_render(t,audio,CTL);if(MOD.on)mod_end(t);}
   ui.force=1;frame();write_frame(f);
  }
  fclose(f);seq_note_off(t,previous);song.playing=0;fprintf(stderr,"Captured %s\n",names[k]);
 }
 return 0;
}
