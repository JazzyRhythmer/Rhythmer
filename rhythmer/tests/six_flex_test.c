/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static int break_in_browser(void){unsigned n,found=0;preset_all_pos(NENGINES,&n);for(unsigned i=0;i<n;i++){unsigned k,e=preset_all_at(NENGINES,i,&k);found+=e==ENGI_BREAK||(e==NENGINES&&up_engine(k)==ENGI_BREAK);}return found;}
static int break_in_models(void){unsigned e=ENGI_WAVETABLE,found=0;for(unsigned i=0;i<NENG_SHOWN;i++){e=eng_step(e,1);found+=e==ENGI_BREAK;}return found;}
static void settle(void){int32_t out[64];for(unsigned i=0;i<24;i++)mix_block(out,32);}
static void save_fixture(const char *path){project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))exit(3);FILE *f=fopen(path,"wb");if(!f)exit(4);fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);}
int main(int argc,char **argv){
 int bad=0;ui_power_on();load_depth++;fm6_init();power_on_parts();test_optional_machines();load_depth--;six_native_init();trks_put(1);song.master_q12=4096;
 song.sel=5;int stored=up_store(0,"BREAK U");bad+=check("save a BREAK user preset",(stored==0||stored==3)&&up_used(0)&&up_engine(0)==ENGI_BREAK);favorite_set(ENGI_BREAK,0,1);favorite_set(NENGINES,0,1);
 for(unsigned k=0;k<6;k++){
  song.sel=k;int own=1;
  bad+=check("assigned BREAK visibility follows owner in factory and user presets",break_in_browser()==(own?ENG_BREAK.npresets+1:0));
  bad+=check("assigned BREAK visibility follows owner in model chooser",(break_in_models()>0)==own);
  unsigned e=ENGI_WAVETABLE;int backwards=0;for(unsigned j=0;j<NENG_SHOWN;j++){e=eng_step(e,-1);backwards|=e==ENGI_BREAK;}bad+=check("reverse model browsing obeys BREAK ownership",backwards==own);
  list_put(1);unsigned filtered; preset_pos(&filtered);int fav_ok=1;for(unsigned j=0;j<filtered;j++){unsigned slot,e=preset_at(j,&slot);if(!own)fav_ok&=e!=ENGI_BREAK&&(e!=NENGINES||up_engine(slot)!=ENGI_BREAK);}bad+=check("favorites obey BREAK ownership",fav_ok);list_put(0);
  if(!own){unsigned before=trk[k].eng_req;set_engine_of(TSEL,ENGI_BREAK);bad+=check("direct model selection cannot duplicate BREAK",TSEL->eng_req==before);bad+=check("direct user preset load cannot duplicate BREAK",up_load(0)!=0&&TSEL->eng_req==before);}
 }
 trk[5].p[P_MUTE]=1;trks_put(0);song.sel=0;
 bad+=check("inactive muted Break does not hide shared sample engine",break_in_browser()>0&&break_in_models()>0);
 trks_put(1);bad+=check("track count preserves existing mute",trk[5].p[P_MUTE]==1);trk[5].p[P_MUTE]=0;
 set_engine_of(&trk[5],ENGI_ACID);settle();
 for(unsigned k=0;k<6;k++){song.sel=k;bad+=check("releasing BREAK makes it available on every track",break_in_browser()>0&&break_in_models()>0);}
 /* Move singleton through every track, including a normally melodic/drum slot. */
 for(unsigned k=0;k<6;k++){
  song.sel=k;set_engine_of(TSEL,ENGI_BREAK);settle();bad+=check("BREAK can move to any of six tracks",TSEL->eng_req==ENGI_BREAK&&six_break_track()==(int)k);
  go_home();open_family(FAM_SEQ);frame();bad+=check("relocated BREAK opens its own step editor",cur_page()->graph==GR_SIXSTEP);
  song.playing=1;six_break_clock(32,32,beat_samples());bad+=check("relocated BREAK clock uses actual owner",six_break_state(TSEL)->run);song.playing=0;six_break_clock(0,32,beat_samples());
  set_engine_of(TSEL,ENGI_ACID);settle();
 }
 for(unsigned k=0;k<6;k++){
  song.sel=k;set_engine_of(TSEL,ENGI_ACID);settle();go_title("303 GEN 1");frame();
  bad+=check("any track gets Acid generator UI",six_gen_page());
  TSEL->p[P_ROOT]=k;TSEL->p[P_SCALE]=2;TSEL->p[P_LN5]=64;TSEL->p[P_LN7]=2;six_gen_apply(0,0);
 }
 six_native_init();
 for(unsigned k=0;k<6;k++){track_t *t=&trk[k];t->xf_on=t->xf=0;t->engine=t->eng_req;trk_note_on(t,36+k*3,127);}
 settle();int independent=1;for(unsigned k=0;k<6;k++)independent&=(acid_voice[k].x[1]/16)==36+(int)k*3&&(acid_voice[k].x[2]&1);
 bad+=check("six Acid tracks keep separate notes and gates",independent);
 ha_voice_t untouched=acid_voice[1];trk_all_off(&trk[0]);settle();
 bad+=check("Acid note-off does not release another track",!(acid_voice[0].x[2]&1)&&(acid_voice[1].x[2]&1)&&(acid_voice[1].x[1]/16)==39);
 (void)untouched;
 uint32_t seeds[6];memcpy(seeds,six_gen_seed,sizeof seeds);song.playing=1;bar_n=100;six_gen_poll();int auto_all=1;for(unsigned k=0;k<6;k++)auto_all&=seeds[k]!=six_gen_seed[k];bad+=check("auto mutation follows each Acid independently",auto_all);song.playing=0;
 project_capture(&proj_scratch);bad+=check("six Acid project round trips",proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire));
 proj_scratch.t[0].engine=proj_scratch.t[1].engine=ENGI_BREAK;bad+=check("multiple Break tracks can be packed",proj_pack(&proj_wire,&proj_scratch));
 for(unsigned k=0;k<6;k++){trk_all_off(&trk[k]);dice_seq(&trk[k]);trk[k].p[P_DIST]=35;trk[k].p[P_CHOR]=50;trk[k].p[P_DLY]=25;trk[k].p[P_REV]=25;}
 if(argc>1)save_fixture(argv[1]);
 song.sel=0;set_engine_of(TSEL,ENGI_BREAK);settle();TSEL->p[P_LN0]=0;TSEL->p[P_E1]=TSEL->p[P_E2]=TSEL->p[P_E4]=0;
 for(unsigned i=0;i<NSTEP;i++){step_t *s=&TSEL->step[i];step_clear(s);if(i<32){s->time=(i&1)?ST_TIE:ST_NOTE;if(!(i&1)){s->n=1;s->note[0]=i/2;s->vel=127;}}}TSEL->p[P_SLEN]=32;TSEL->p[P_SGATE]=127;TSEL->p[P_LN0]=1;
 bad+=check("five Acid plus relocated BREAK allowed",six_break_track()==0&&trk[1].eng_req==ENGI_ACID&&trk[5].eng_req==ENGI_ACID);
 trks_put(0);song.playing=1;six_break_clock(32,32,beat_samples());bad+=check("four-track mode can play BREAK on T1",six_break_state(TSEL)->run);song.playing=0;trks_put(1);
 if(argc>2)save_fixture(argv[2]);
 printf("SIX flexible machines: %s (%d failures)\n",bad?"FAIL":"PASS",bad);return bad?1:0;
}
