/* SPDX-License-Identifier: GPL-3.0-only
 * Current firmware UI, driven through its input/actions; presentation only. */
#define UI_TEST_NO_MAIN 1
#include "../../rhythmer/tests/ui_test.c"
static unsigned char bank[2][81920];
static FILE *clipout(const char *dir,const char *name){char p[1024];snprintf(p,sizeof p,"%s/%s.rgb",dir,name);FILE*f=fopen(p,"wb");if(!f)exit(2);return f;}
static void pixels(FILE*f){for(unsigned i=0;i<57600;i++){unsigned p=swap16(host_screen[i]);unsigned char c[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(c,1,3,f);}}
static void advance(FILE*f){int32_t buf[CTL*2];for(unsigned b=0;b<(44100u*80u/1000u)/CTL;b++){mix_block(buf,CTL);for(unsigned j=1;j<CTL;j+=2)scope_buf[scope_w++&(SCOPE_N-1)]=(int16_t)buf[2*j];}for(unsigned j=0;j<5;j++)frame();ui.force=1;ui_draw();pixels(f);}
static void reset(void){ui_power_on();fm6_init();load_depth++;power_on_parts();load_depth--;six_native_init();trks_put(0);menu_put(MI_COLOR,0);menu_put(MI_SKIN,1);menu_put(MI_STYLE,1);fm1_ms+=8000;song.g[G_BPM]=120;}
static void demo(void){for(unsigned k=0;k<4;k++){track_t*t=&trk[k];for(unsigned n=0;n<32;n++){step_clear(&t->step[n]);if(n%(k?4:2)==0){step_t*s=&t->step[n];s->time=ST_NOTE;s->vel=90;s->n=1;s->note[0]=k?48+k*5+(n/4)%4:(n%8?42:36);}}t->p[P_SLEN]=32;t->p[P_SGATE]=70;}press(B_PLAY);}
static void breaksetup(void){trks_put(1);song.sel=5;track_t*t=TSEL;set_engine_of(t,ENGI_BREAK);apply_preset_to(t,0);engine_block(t);six_loop_mask=3;six_loop_now=0;for(unsigned k=0;k<2;k++)six_loops[k]=bank[k]+512;six_break_owner=5;jungle_bind(six_loops[0],72000,48000);jungle_sequence_mode(1);six_wave_src[0]=six_wave_src[1]=0;six_wave_cache(0);six_wave_cache(1);go_page(GR_ROLL);}
static void seqhold(void){fm1_in.buttons|=1u<<panel.btn[B_SEQ];host_pressed|=1u<<panel.btn[B_SEQ];frames(850);if(layer_open()!=LAYER_SEQ)exit(4);}
static void sequp(void){fm1_in.notes=0;fm1_in.buttons&=~(1u<<panel.btn[B_SEQ]);frame();ui.msg_t=0;go_page(GR_ROLL);}
static void tapkey(unsigned k){unsigned before=steps_sig(TSEL);fm1_in.notes|=1u<<k;host_notes|=1u<<k;frame();if(steps_sig(TSEL)==before)layer_key(LAYER_SEQ,k);}
int main(int argc,char**argv){if(argc!=4)return 1;for(unsigned k=0;k<2;k++){FILE*f=fopen(argv[k+2],"rb");if(!f||fread(bank[k],1,81920,f)!=81920)return 3;fclose(f);}FILE*f;
 reset();demo();ui.home=1;f=clipout(argv[1],"four-home");for(unsigned n=0;n<50;n++){if(n==18||n==34)turn(EN_ALGO,1);advance(f);}fclose(f);
 reset();go_title("SYSTEM");f=clipout(argv[1],"track-system");for(unsigned n=0;n<75;n++){if(n==36)turn(EN_K1,1);advance(f);}fclose(f);if(ui.confirm!=CF_TRACKS6||trk_live!=4)return 5;
 f=clipout(argv[1],"cpu-confirm");for(unsigned n=0;n<75;n++){if(n==53){press(B_OCTUP);if(trk_live!=6)return 6;ui.home=1;demo();}advance(f);}fclose(f);
 reset();breaksetup();f=clipout(argv[1],"held-seq");seqhold();unsigned sig=0;for(unsigned n=0;n<100;n++){if(n==10||n==43||n==76){tapkey(n==10?0:n==43?2:4);if(n==10&&!bs_nonempty(TSEL,32))return 7;unsigned now=steps_sig(TSEL);if(n!=10&&now==sig)return 8;sig=now;}if(n==16||n==49||n==82){sequp();if(!song.playing)press(B_PLAY);}if(n==33||n==66)seqhold();advance(f);}fclose(f);sequp();stop_transport();
 reset();song.sel=0;dx_kit_load(0);dx_ui_seen=last_note;go_title("DRUM VOICE");f=clipout(argv[1],"drum-swap");unsigned original=dx_voice_current();for(unsigned n=0;n<63;n++){if(n==9)turn(EN_K1,3);if(n>=18&&n<=42&&n%4==2)turn(EN_K3,1);if(n%8==0)trk_note_on(TSEL,42,100);advance(f);}fclose(f);if(original==dx_voice_current())return 9;
 reset();trks_put(1);set_engine_of(&trk[4],ENGI_ACID);apply_preset_to(&trk[4],0);engine_block(&trk[4]);song.sel=4;go_page(GR_SIXGEN);f=clipout(argv[1],"acid-create");for(unsigned n=0;n<57;n++){if(n==12||n==37)press(n==12?B_OCTUP:B_OCTDN);if(n==25)press(B_PLAY);advance(f);}fclose(f);
 const char*names[]={"target-gen","break-gen"};for(unsigned k=0;k<2;k++){reset();trks_put(1);demo();stop_transport();if(k)breaksetup();gp.target=k?7:2;gp.form=0;gp.genre=59;gp.seed=31;gp.energy=55;gp.complex=65;gp.human=70;gp.swing=GA_AUTO;for(unsigned j=0;j<GP_N;j++)gp.amt[j]=GA_AUTO;ui.home=1;ui.confirm=CF_DICE_ALL;gp.page=0;gp.hot=k?0:3;gen_fresh=1;f=clipout(argv[1],names[k]);unsigned last=0;
 for(unsigned n=0;n<125;n++){
  if(!k){if(n==17||n==57)gen_knob(3,1);if(n==30||n==70){gen_go();gen_fresh=1;ui.confirm=CF_DICE_ALL;}}
  else{unsigned phase=n%42;if(phase==0){const char*styles[]={"BOOM BAP","JUNGLE","DRUNKEN HH"};unsigned st=n/42;if(st>2)st=2;for(unsigned j=0;j<NGENRE;j++)if(!strcmp(GENRE[j].name,styles[st]))gp.genre=(uint8_t)(j+1);ui.home=1;ui.confirm=CF_DICE_ALL;gp.hot=0;gen_fresh=1;}
   if(phase==11){gen_go();unsigned now=steps_sig(TSEL);if(n>42&&now==last)return 10;last=now;fprintf(stderr,"Genre %s pattern %08x seed %u\n",GENRE[gp.genre-1].name,now,gp.seed);}
   if(phase==18){ui.confirm=0;ui.msg_t=0;song.sel=5;go_page(GR_ROLL);seq_start();}}
  if(gp.seed!=31)return 11;advance(f);
 }fclose(f);}
 fprintf(stderr,"PASS: real 4/6-track confirmation, drum knob selection, Acid NEW/MUTATE, targeted GEN, genre Break GEN and three held-SEQ pattern changes. No hardware I/O.\n");return 0;}
