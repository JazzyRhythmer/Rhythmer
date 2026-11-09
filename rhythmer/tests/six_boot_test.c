/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static uint16_t still[240*240];
static void shot(const char *dir,unsigned n){char p[1024];if(n==999)snprintf(p,sizeof p,"%s/startup.ppm",dir);else snprintf(p,sizeof p,"%s/startup-%03u.ppm",dir,n);FILE*f=fopen(p,"wb");if(!f)exit(2);fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<240*240;i++){unsigned x=swap16(host_screen[i]);unsigned char c[3]={(x>>11)*255/31,((x>>5)&63)*255/63,(x&31)*255/31};fwrite(c,1,3,f);}fclose(f);}
static unsigned logo_hash(void){unsigned h=2166136261u;for(unsigned y=76;y<104;y++)h=fnv(h,&host_screen[y*240+34],176*2);return h;}
int main(int argc,char**argv){int bad=0;draw_splash();ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(0);ui_prefs&=~PREF_ANIM_OFF;
#if RHYTHMER_FMINI && RHYTHMER_OBERVAVE
 static const unsigned fixed[6]={ENGI_DRUM,ENGI_ANALOG,ENGI_FMINI,ENGI_OBERVAVE,ENGI_ACID,ENGI_BREAK};
#else
 static const unsigned fixed[6]={ENGI_DRUM,ENGI_ANALOG,ENGI_PHASE,ENGI_LOFI,ENGI_ACID,ENGI_BREAK};
#endif
 unsigned ids=1;for(unsigned k=0;k<6;k++)ids&=splash_engine(k)==fixed[k];
 bad+=check("all six startup identities are fixed without assigning tracks",ids&&trk[4].eng_req==ENGI_EMPTY&&trk[5].eng_req==ENGI_EMPTY);
 bad+=check("startup is only slightly longer and stays below the fallback timeout",SPL_WRITE+SPL_HOLD==2100u&&SPL_WRITE+SPL_HOLD<2400u);
 splash_sig(0,160);unsigned early=logo_hash();splash_sig(0,480);unsigned build=logo_hash();splash_sig(0,900);unsigned complete=logo_hash();splash_sig(0,1800);
 bad+=check("wordmark assembles in stages then settles sharply",early!=build&&build!=complete&&complete==logo_hash());
 splash_sig(0,900);memcpy(still,host_screen,sizeof still);
 for(unsigned layout=0;layout<4;layout++){
  for(unsigned k=0;k<NTRK;k++)trk[k].engine=trk[k].eng_req=(uint8_t)(layout==0?ENGI_EMPTY:layout==1?ENGI_WAVETABLE:layout==2?ENGI_DRUM:k==0?ENGI_BREAK:ENGI_ACID);
  trks_put(layout&1u);unsigned tracks=fnv(2166136261u,trk,sizeof trk),globals=fnv(2166136261u,&song,sizeof song);
  splash_sig(0,900);bad+=check("startup composition ignores actual engines and four/six-track mode",!memcmp(still,host_screen,sizeof still));
  splash_sig(0,1300);unsigned moving=1;for(unsigned k=0;k<6;k++){unsigned change=0,x0=13+k%3*76,y0=149+k/3*42;for(unsigned y=y0;y<y0+20;y++)for(unsigned x=x0;x<x0+60;x++)change+=still[y*240+x]!=host_screen[y*240+x];moving&=change>0;}
  bad+=check("every fixed machine glyph moves, independently of its corner indicator",moving);
  bad+=check("startup never changes instruments, MIDI or song state",tracks==fnv(2166136261u,trk,sizeof trk)&&globals==fnv(2166136261u,&song,sizeof song));
 }
 for(unsigned size=0;size<2;size++){
  unsigned idle[NENGINES]={0},unique=1,moving=1,visible=0;
  int w=size?208:30,h=size?82:24;
  for(unsigned e=0;e<NENGINES;e++){
   if(!eng_ok(e)||e==ENGI_EMPTY)continue;
   visible++;
   cv_begin((unsigned)w,(unsigned)h,T_BG);
   six_machine_glyph(0,e,0,0,0,0,w,h,T_TEXT);
   cv_blit(0,0);
   unsigned hash=2166136261u;
   for(int y=0;y<h;y++)hash=fnv(hash,host_screen+y*240,w*2);
   idle[e]=hash;
   for(unsigned prev=0;prev<e;prev++)if(idle[prev]==hash)unique=0;
   cv_begin((unsigned)w,(unsigned)h,T_BG);
   six_machine_glyph(0,e,96,0x56789abcu,0,0,w,h,T_TEXT);
   cv_blit(0,0);
   hash=2166136261u;
   for(int y=0;y<h;y++)hash=fnv(hash,host_screen+y*240,w*2);
   moving&=hash!=idle[e];
  }
  bad+=check(size?"EDIT silhouettes are all distinct":"HOME silhouettes are all distinct",visible>=9&&unique);
  bad+=check(size?"every EDIT glyph reacts to activity":"every HOME glyph reacts to activity",moving);
 }
 if(argc>1){for(unsigned ms=0;ms<SPL_WRITE+SPL_HOLD;ms+=80){splash_sig(0,ms);shot(argv[1],ms/80);}splash_sig(0,1500);shot(argv[1],999);}
 ui_prefs|=PREF_ANIM_OFF;splash_sig(0,900);memcpy(still,host_screen,sizeof still);splash_sig(0,1300);bad+=check("ANIM OFF still freezes the complete startup",!memcmp(still,host_screen,sizeof still));
 printf("Fixed startup machine identity: %s (%d failures)\n",bad?"FAIL":"PASS",bad);return !!bad;
}
