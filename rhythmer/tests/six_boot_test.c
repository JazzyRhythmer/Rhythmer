/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static uint16_t still[240*240];
static void shot(const char *dir){char p[1024];snprintf(p,sizeof p,"%s/startup.ppm",dir);FILE*f=fopen(p,"wb");if(!f)exit(2);fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<240*240;i++){unsigned x=swap16(host_screen[i]);unsigned char c[3]={(x>>11)*255/31,((x>>5)&63)*255/63,(x&31)*255/31};fwrite(c,1,3,f);}fclose(f);}
int main(int argc,char**argv){int bad=0;draw_splash();ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(0);ui_prefs&=~PREF_ANIM_OFF;
 bad+=check("startup has fixed Acid and Break identities without assigning tracks",splash_engine(4)==ENGI_ACID&&splash_engine(5)==ENGI_BREAK&&trk[4].eng_req==ENGI_EMPTY&&trk[5].eng_req==ENGI_EMPTY);
 splash_sig(0,900);memcpy(still,host_screen,sizeof still);
 for(unsigned layout=0;layout<4;layout++){
  for(unsigned k=0;k<NTRK;k++)trk[k].engine=trk[k].eng_req=(uint8_t)(layout==0?ENGI_EMPTY:layout==1?ENGI_SERUM:layout==2?ENGI_DRUM:k==0?ENGI_BREAK:ENGI_ACID);
  trks_put(layout&1u);unsigned tracks=fnv(2166136261u,trk,sizeof trk),globals=fnv(2166136261u,&song,sizeof song);
  splash_sig(0,900);bad+=check("startup composition ignores actual engines and four/six-track mode",!memcmp(still,host_screen,sizeof still));
  splash_sig(0,1300);unsigned acid=0,brk=0;for(unsigned y=191;y<213;y++)for(unsigned x=89;x<151;x++)acid+=still[y*240+x]!=host_screen[y*240+x];for(unsigned y=191;y<213;y++)for(unsigned x=165;x<227;x++)brk+=still[y*240+x]!=host_screen[y*240+x];
  bad+=check("Acid and Break glyphs both animate after reveal",acid>0&&brk>0);
  bad+=check("startup never changes instruments, MIDI or song state",tracks==fnv(2166136261u,trk,sizeof trk)&&globals==fnv(2166136261u,&song,sizeof song));
 }
 if(argc>1){splash_sig(0,1000);shot(argv[1]);}
 ui_prefs|=PREF_ANIM_OFF;splash_sig(0,900);memcpy(still,host_screen,sizeof still);splash_sig(0,1300);bad+=check("ANIM OFF still freezes the complete startup",!memcmp(still,host_screen,sizeof still));
 printf("Fixed startup machine identity: %s (%d failures)\n",bad?"FAIL":"PASS",bad);return !!bad;
}
