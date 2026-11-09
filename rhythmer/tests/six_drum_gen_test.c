/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(int argc,char **argv) {
 int bad=0;ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(1);song.sel=0;
 bad+=check("Elixir Euclid endpoints and exact hit counts",dg_euclid(0,4,0)==0&&dg_euclid(64,64,64)==~(uint64_t)0);
 for(unsigned len=1;len<=64;len++)for(unsigned hits=0;hits<=len;hits++)
  for(unsigned rot=0;rot<65;rot++)if(__builtin_popcountll(dg_euclid(len,hits,rot))!=hits)return 10;
 drum_gen_t cfg={DG_GRIDS,0,72,0,64,64,1,31,0,0,0,8};
 for(unsigned alg=0;alg<DG_COUNT;alg++)for(unsigned len=1;len<=64;len++)for(unsigned seed=0;seed<128;seed++){
  cfg.type=alg;cfg.seed=seed;cfg.variation=seed*2;cfg.density=seed;cfg.x=seed;cfg.y=127-seed;cfg.lane=seed&7;
  uint64_t late,flam;drum_gen_row_t a=drum_generate(&cfg,len,0x53189a55cc16f001ull,0x10101101,&late,&flam);
  drum_gen_row_t b=drum_generate(&cfg,len,0x53189a55cc16f001ull,0x10101101,0,0);
  if(a.hits!=b.hits||a.accents!=b.accents||((a.hits|late|flam)&~dg_mask(len))||(a.accents&~a.hits))return 11;
 }
 bad+=check("all five algorithms deterministic and bounded for every length/seed",1);
 cfg.type=DG_TURING;cfg.reglen=8;cfg.variation=127;cfg.density=81;
 drum_gen_row_t loop=drum_generate(&cfg,64,0x59,0,0,0);
 for(unsigned i=8;i<64;i++)if((loop.hits>>i&1)!=(loop.hits>>(i%8)&1))return 12;
 bad+=check("Turing LOCK 127 repeats its register without drift",1);
 cfg.type=DG_EUCLID;cfg.density=64;cfg.hits2=4;cfg.rotate2=1;
 for(unsigned join=0;join<4;join++){
  cfg.join=join;drum_gen_row_t r=drum_generate(&cfg,16,0,0,0,0);
  uint64_t a=dg_euclid(16,(64*16+63)/127,cfg.variation),b=dg_euclid(16,4,1);
  if(r.hits!=(join==1?a^b:join==2?a&b:join==3?a&~b:a|b))return 13;
 }
 bad+=check("second Euclid pulse combines OR XOR AND SUB",1);
 go_title("SEQ GEN");frame();bad+=check("sequence generator has a reachable common SEQ page",cur_page()->graph==GR_DGEN&&page_visible(ui.page));
 for(unsigned alg=0;alg<DG_COUNT;alg++) {
  song.sel=0;TSEL->p[P_SLEN]=32;
  for(unsigned i=0;i<NSTEP;i++){step_clear(&TSEL->step[i]);grid_hit(TSEL,i,1,1);if(i&1)grid_hit(TSEL,i,3,1);}
  step_t old[NSTEP];memcpy(old,TSEL->step,sizeof old);
  drum_gen_t *g=dg_current();*g=(drum_gen_t){alg,0,80,30,64,64,1,31,0,0,0,8};
  int bpm=song.g[G_BPM],lev=TSEL->p[P_LEVEL];unsigned eng=trk_eng(TSEL);
  drumgen_apply(0);
  for(unsigned i=0;i<NSTEP;i++){
   if(i>=32&&memcmp(&old[i],&TSEL->step[i],sizeof(step_t)))return 14;
   if((step_lanes(&old[i])&~1u)!=(step_lanes(&TSEL->step[i])&~1u))return 15;
  }
  if(song.g[G_BPM]!=bpm||TSEL->p[P_LEVEL]!=lev||trk_eng(TSEL)!=eng)return 16;
  project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch)||!proj_import(&proj_scratch,&proj_wire,sizeof proj_wire))return 17;
  step_t generated[NSTEP];memcpy(generated,TSEL->step,sizeof generated);
  if(project_restore_runtime(&proj_scratch)||memcmp(generated,TSEL->step,sizeof generated))return 18;
 }
 bad+=check("generation preserves other lanes, steps past LEN, sound and tempo; projects round trip",1);
 song.sel=1;unsigned idx=0;while(idx<NPAGES&&strcmp(PAGES[idx].title,"SEQ GEN"))idx++;
 bad+=check("sequence generator is also visible on synth tracks",idx<NPAGES&&page_visible(idx));
 for(unsigned engine=0;engine<NENGINES;engine++)if(eng_ok(engine)&&engine!=ENGI_DRUM&&engine!=ENGI_EMPTY){
  song.sel=1;set_engine_of(TSEL,engine);TSEL->p[P_SLEN]=32;
  for(unsigned i=0;i<NSTEP;i++)step_clear(&TSEL->step[i]);
  if(engine==ENGI_BREAK){TSEL->step[7].time=ST_NOTE;TSEL->step[7].n=1;TSEL->step[7].note[0]=33;}
  for(unsigned alg=0;alg<DG_COUNT;alg++){
   drum_gen_t *g=dg_current();*g=(drum_gen_t){alg,0,90,30,64,64,1,31,0,0,0,8};
   int bpm=song.g[G_BPM];drumgen_apply(0);
   for(unsigned i=0;i<32;i++){const step_t *st=&TSEL->step[i];if(st->n>4)return 20;
    for(unsigned j=0;st->time==ST_NOTE&&j<st->n;j++)if(st->note[j]>(engine==ENGI_BREAK?34:127))return 21;
    if(engine==ENGI_ACID&&st->n>1)return 22;
   }
   if(song.g[G_BPM]!=bpm)return 23;
   if(engine==ENGI_BREAK&&(TSEL->p[P_LN0]!=1||TSEL->step[7].time!=ST_NOTE))return 24;
  }
 }
 bad+=check("all live synth, Acid and Break engines accept safe normal step output",1);
 song.sel=1;set_engine_of(TSEL,ENGI_ANALOG);go_home();int found=0;
 for(unsigned k=0;k<NPAGES*2;k++){press(B_SEQ);frame();if(!strcmp(cur_page()->title,"SEQ GEN")){found=1;break;}}
 bad+=check("physical SEQ navigation reaches the common GEN tab",found);
 song.octave=0;int8_t oct=song.octave;press(B_OCTUP);frame();
 bad+=check("OCT+ generates without transposing the keyboard",song.octave==oct);
 song.sel=0;go_title("SEQ GEN");ui.force=1;frame();
 if(argc>1){FILE *f=fopen(argv[1],"wb");if(!f)return 19;fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<57600;i++){unsigned p=swap16(host_screen[i]);unsigned char c[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(c,1,3,f);}fclose(f);}
 printf("Elixir drum generator adapter: %s (%d)\n",bad?"FAIL":"PASS",bad);return bad?1:0;
}
