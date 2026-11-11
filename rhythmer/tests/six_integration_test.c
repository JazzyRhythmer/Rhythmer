#define TEST_ACID_TRACK 4u /* test fixture position, never a product role */
#define TEST_BREAK_TRACK 5u
/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(int argc,char **argv) {
 int bad=0; ui_power_on(); load_depth++; fm6_init(); power_on_parts();test_optional_machines(); load_depth--; six_native_init();song.master_q12=4096;trk_live=NTRK;ui_rec_prefs|=0x80u;

 /* Track count is independent of machine category; inactive data is retained. */
 bad+=check("six-track test fixture enables all tracks",trk_live==6);
 trks_put(1);uint32_t ac0=steps_sig(&trk[4]),br0=steps_sig(&trk[5]);
 for(unsigned m=0;m<2;m++){
  trks_put(m);bad+=check("GLO four/six track modes",trk_live==(m?6u:4u)&&six_track_enabled(4)==(int)m&&six_track_enabled(5)==(int)m);
  bad+=check("track count preserves patterns",ac0==steps_sig(&trk[4])&&br0==steps_sig(&trk[5]));
  rp_apply();bad+=check("track count survives settings reapply",six_machine_mask==(m?3u:0u)&&menu_get(MI_TRACKS)==m);
 }
 trks_put(0);trk_note_on(&trk[4],36,127);six_break_clock(32,32,beat_samples());
 bad+=check("disabled tracks do not trigger or run",!trk[4].v[0].active&&!six_break_run&&!six_roll_count);
 trks_put(1);
 /* Independent rates, phase, fades and per-slot source routing. */
 track_t *lt=&trk[1];lt->p[P_LRATE]=20;lt->p[P_L2RATE]=90;lt->p[P_L2WAVE]=2;
 lt->lfo_ph=lt->lfo2_ph=0;track_lfo_tick(lt);six_lfo2_tick(lt);
 bad+=check("two LFO clocks advance independently",lt->lfo_ph!=lt->lfo2_ph&&lt->lfo_ph&&lt->lfo2_ph);
 lt->lfo_val=16384;lt->lfo_fade=32767;lt->lfo2_val=-16384;lt->lfo2_fade=32767;
 lt->p[P_M1SRC]=MS_LFO;lt->p[P_M1DST]=MD_CUT;lt->p[P_M1AMT]=32;
 lt->p[P_M2SRC]=MS_LFO2;lt->p[P_M2DST]=MD_SHP;lt->p[P_M2AMT]=32;
 mod_begin(lt);bad+=check("matrix slots independently follow LFO 1 and 2",MOD.cut>0&&MOD.shp<0);mod_end(lt);
 project_capture(&proj_scratch);bad+=check("dual LFO project packs",proj_pack(&proj_wire,&proj_scratch));
 lt->p[P_L2RATE]=0;bad+=check("dual LFO project imports",proj_import(&proj_scratch,&proj_wire,sizeof proj_wire));
 bad+=check("LFO 2 and source selection survive save",proj_scratch.t[1].p[P_L2RATE]==90&&proj_scratch.t[1].p[P_M2SRC]==MS_LFO2);
 power_on_parts();test_optional_machines();six_native_init();trks_put(3);
 for(unsigned k=0;k<6;k++)bad+=check("explicit fixture engine layout",trk[k].eng_req==(k==4?ENGI_ACID:k==5?ENGI_BREAK:TRK_DEF[k][0]));
 bad+=check("160 BPM and four projects",song.g[G_BPM]==160 && NSONG==4);
 for(unsigned t=0;t<6;t++){song.sel=t;uint32_t total;preset_all_pos(NENGINES,&total);int ok=total>0;for(unsigned i=0;i<total;i++){uint32_t k,e=preset_all_at(NENGINES,i,&k);ok&=six_preset_engine(e,NENGINES);}bad+=check("preset browser respects singleton Break ownership",ok);}
 song.sel=1;bad+=check("model list includes EMPTY and keeps stable synth order",eng_step(ENGI_WAVETABLE,-1)==ENGI_EMPTY&&eng_step(ENGI_WAVETABLE,1)==ENGI_PHASE&&eng_step(ENGI_PHASE,1)==ENGI_LOFI);
 bad+=check("stored Acid Break IDs remain stable",ENGI_BREAK==15u&&ENGI_ACID==16u&&ENGI_WAVETABLE==17u);
 project_capture(&proj_scratch);bad+=check("Garden engines pack and round trip",proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)&&proj_scratch.t[1].engine==ENGI_WAVETABLE&&proj_scratch.t[2].engine==ENGI_PHASE&&proj_scratch.t[3].engine==ENGI_LOFI);
 favorite_set(ENGI_ACID,0,1);favorite_set(ENGI_BREAK,0,1);favorite_set(ENGI_WAVETABLE,0,1);favorite_set(ENGI_LOFI,0,1);favorite_set(ENGI_LOFI,0,0);
 bad+=check("new synth favorites preserve Acid Break and machine bits",favorite_has(ENGI_ACID,0)&&favorite_has(ENGI_BREAK,0)&&favorite_has(ENGI_WAVETABLE,0)&&!favorite_has(ENGI_LOFI,0)&&favorites.factory[15][15]==0x61u);

 for(unsigned k=0;k<5;k++){song.sel=k; dice_seq(&trk[k]); bad+=check("JIANT DICE creates sequence",!seq_is_empty(&trk[k]));}
 song.sel=TEST_BREAK_TRACK;dice_seq(TSEL);int chops_ok=1;for(unsigned i=0;i<NSTEP;i++)if(TSEL->step[i].time==ST_NOTE&&TSEL->step[i].n)chops_ok&=TSEL->step[i].n==1&&TSEL->step[i].note[0]<16;bad+=check("JIANT DICE uses valid break chops",chops_ok&&!seq_is_empty(TSEL));for(unsigned i=0;i<NSTEP;i++)step_clear(&TSEL->step[i]);
 song.sel=TEST_ACID_TRACK;go_title("303 GEN 1");frame();uint32_t before=steps_sig(TSEL);press(B_OCTUP);
 bad+=check("X0X generator OCT+ changes the 303 pattern",steps_sig(TSEL)!=before);before=steps_sig(TSEL);press(B_OCTDN);
 bad+=check("X0X generator OCT- mutates the pattern",steps_sig(TSEL)!=before);
 turn(EN_K1,-10);bad+=check("generator density knob",TSEL->p[P_LN0]==60);
 before=steps_sig(TSEL);song.playing=1;TSEL->p[P_LN7]=2;bar_n=1;six_gen_poll();bad+=check("auto mutation waits for interval",steps_sig(TSEL)==before);uint32_t seed_before=six_gen_seed[song.sel];for(bar_n=2;bar_n<=16;bar_n+=2)six_gen_poll();bar_n=16;bad+=check("auto mutation changes pattern across intervals",six_gen_seed[song.sel]!=seed_before&&steps_sig(TSEL)!=before);before=steps_sig(TSEL);six_gen_poll();bad+=check("one mutation per bar",steps_sig(TSEL)==before);song.playing=0;TSEL->p[P_LN7]=0;six_gen_poll();
 song.sel=TEST_BREAK_TRACK;ui.home=1;frame();TSEL->p[P_E1]=90;TSEL->p[P_E2]=90;
 press(B_HOME);bad+=check("HOME preserves break variation warp and straight",TSEL->p[P_E1]==90&&TSEL->p[P_E2]==90&&TSEL->p[P_E6]==0);
 song.rec=0;press(B_REC);bad+=check("HOME REC arms JIANT recording not freeze",song.rec==(1u<<TEST_BREAK_TRACK)&&TSEL->p[P_E5]==0);song.rec=0;transport_req=0;
 int octave_before=song.octave;press(B_OCTDN);bad+=check("break OCT keeps JIANT octave and tempo",song.octave==octave_before-1&&song.g[G_BPM]==160);press(B_OCTUP);
 go_title("BRK SEQ");frame();turn(EN_K1,1);bad+=check("CHOP source selected",TSEL->p[P_LN0]==1);
 go_title("BRK PERF");frame();bad+=check("performance keys confined to BRK PERF",six_perf_enabled);
 song.playing=1;song.rec=1u<<TEST_BREAK_TRACK;TSEL->seq_idx=3;TSEL->seq_pos=0;
 key_on(26,TSEL);bad+=check("performance REC stores roll 16 command",TSEL->step[3].n==1&&TSEL->step[3].note[0]==31&&six_roll_count==1);
 key_on(23,TSEL);bad+=check("last pressed roll wins",six_roll_count==2&&six_roll_held[1]==13);key_off(23,TSEL);bad+=check("release restores and records prior held roll",six_roll_count==1&&six_roll_held[0]==15&&TSEL->step[3].note[0]==31);
 press(B_HOME);six_break_clock(32,32,beat_samples());bad+=check("HOME leaves performance without resetting controls or stuck roll",ui.home&&!six_perf_enabled&&!six_roll_count&&TSEL->p[P_E1]==90&&TSEL->p[P_E2]==90);key_off(26,TSEL);
 song.playing=0;song.rec=0;transport_req=0;
 open_family(FAM_SEQ);frame();press(B_REC);bad+=check("T6 SEQ REC arms step recording",song.rec==(1u<<TEST_BREAK_TRACK));
 ui.cursor=0;seq_entry(1u<<7);bad+=check("T6 step entry maps chop without roll",TSEL->step[0].n&&TSEL->step[0].note[0]==4&&!six_roll_count);
 song.playing=1;TSEL->seq_idx=1;TSEL->seq_pos=0;key_on(26,TSEL);key_off(26,TSEL);bad+=check("T6 live recording writes slice trigger",TSEL->step[1].n==1&&TSEL->step[1].note[0]==15);song.playing=0;song.rec=0;transport_req=0;
 ui.cursor=0;note_edit(1,-1000);bad+=check("SEQ can select first chop C1",TSEL->step[0].note[0]==0);note_edit(1,1000);bad+=check("SEQ clamps command to slot B",TSEL->step[0].note[0]==34);note_edit(1,-30);
 static uint8_t sa[72000],sb[72000];six_loops[0]=sa;six_loops[1]=sb;six_loop_mask=3;six_loop_now=0;jungle_bind(sa,72000,48000);
 go_title("BRK PERF");frame();song.playing=1;song.rec=1u<<TEST_BREAK_TRACK;TSEL->seq_idx=5;TSEL->seq_pos=0;
 key_on(22,TSEL);key_off(22,TSEL);bad+=check("PERF B selects and records slot B",TSEL->p[P_E4]==1&&TSEL->step[5].note[0]==33);
 six_break_ticks=1;six_break_clock(0,32,beat_samples());bad+=check("manual swap waits for grid",six_loop_now==0);
 six_break_ticks=48;six_break_clock(0,32,beat_samples());bad+=check("manual swap commits at sixteenth boundary",six_loop_now==1&&jungle_bound()==sb);
 seq_note_on(TSEL,32,100);bad+=check("sequenced A swap fires without second quantization",six_loop_now==0&&jungle_bound()==sa);
 key_on(25,TSEL);key_off(25,TSEL);bad+=check("toggle resolves to deterministic slot command",TSEL->p[P_E4]==1&&TSEL->step[5].note[0]==33);
 six_loops[0]=0;key_on(20,TSEL);key_off(20,TSEL);bad+=check("missing slot leaves current source intact",TSEL->p[P_E4]==1);six_loops[0]=sa;
 TSEL->seq_idx=6;TSEL->seq_pos=0;key_on(24,TSEL);key_on(22,TSEL);key_off(22,TSEL);
 bad+=check("recorded source swap keeps held roll in same step",TSEL->step[6].n==2&&TSEL->step[6].note[0]==30&&TSEL->step[6].note[1]==33);
 key_off(24,TSEL);
 song.playing=0;song.rec=0;transport_req=0;
 /* Dedicated Amenity return: no JIANT HOME/REC/OCT collision or destructive pattern reset. */
 go_title("BRK RETURN");frame();before=steps_sig(TSEL);octave_before=song.octave;
 TSEL->p[P_E0]=37;TSEL->p[P_E1]=90;TSEL->p[P_E2]=90;TSEL->p[P_E3]=70;TSEL->p[P_E5]=1;TSEL->p[P_E7]=21;
 press(B_OCTDN);bad+=check("RETURN back does not reset or transpose",six_perf_page()&&TSEL->p[P_E1]==90&&song.octave==octave_before);
 go_title("BRK RETURN");frame();press(B_OCTUP);six_break_clock(0,32,beat_samples());
 bad+=check("Amenity RETURN clears variation warp and returns to PERF",six_perf_page()&&TSEL->p[P_E1]==0&&TSEL->p[P_E2]==0&&TSEL->p[P_E6]==0);
 bad+=check("RETURN preserves CHOP pattern source slot FX freeze and tempo",steps_sig(TSEL)==before&&TSEL->p[P_LN0]==1&&TSEL->p[P_E4]==1&&TSEL->p[P_E0]==37&&TSEL->p[P_E3]==70&&TSEL->p[P_E5]==1&&TSEL->p[P_E7]==21&&song.octave==octave_before&&song.g[G_BPM]==160);
 TSEL->p[P_E0]=0;TSEL->p[P_E3]=64;TSEL->p[P_E5]=TSEL->p[P_E7]=0;
 char label[8];note_name(label,31);bad+=check("roll command has readable SEQ label",!strcmp(label,"R16"));
 song.sel=TEST_ACID_TRACK;go_title("303 GEN 1");frame();before=steps_sig(TSEL);press(B_HOME);press(B_OCTUP);bad+=check("303 generation does not intercept HOME octave",steps_sig(TSEL)==before);press(B_OCTDN);
 song.sel=TEST_ACID_TRACK;go_title("303 GEN 2");frame();
 for(unsigned k=0;k<4;k++){
  trk[TEST_ACID_TRACK].p[P_E0]=32+k;bad+=check("save section",project_save(k)==0);
  trk[TEST_ACID_TRACK].p[P_E0]=100;project_load(k);bad+=check("restore section with acid parameters",trk[TEST_ACID_TRACK].p[P_E0]==32+(int)k && trk[TEST_ACID_TRACK].p[P_LN0]==60);
  bad+=check("restore CHOP source and recorded performance",trk[TEST_BREAK_TRACK].p[P_LN0]==1&&trk[TEST_BREAK_TRACK].step[3].note[0]==31&&trk[TEST_BREAK_TRACK].step[5].note[0]==33);
 }
 /* A dedicated break sequence view edits the ordinary event lane. */
 song.sel=TEST_BREAK_TRACK;go_title("BRK SET");frame();bad+=check("break setup reachable from SEQ",cur_page()->fam==FAM_SEQ);
 go_title("BRK STEP");frame();song.playing=0;song.rec=0;transport_req=0;press(B_REC);
 bad+=check("BRK STEP REC arms stopped keyboard entry without starting PLAY",roll_on()&&song.rec==(1u<<TEST_BREAK_TRACK)&&transport_req==0);
 song.rec=0;ui.cursor=0;step_clear(&TSEL->step[0]);
 turn(EN_K2,13);turn(EN_K2,13);bad+=check("SEQ break editor selects 1/16 UP",TSEL->step[0].time==ST_NOTE&&TSEL->step[0].n==1&&TSEL->step[0].note[0]==26);
 turn(EN_K3,3);bad+=check("SEQ break editor length writes three ties",TSEL->step[1].time==ST_TIE&&TSEL->step[3].time==ST_TIE&&note_len(TSEL,0,16)==4);
 turn(EN_K3,-2);bad+=check("shortening break hold clears only its old ties",TSEL->step[1].time==ST_TIE&&TSEL->step[2].time==ST_REST&&TSEL->step[3].time==ST_REST);
 go_title("BRK PLAY");frame();bad+=check("SEQ play page enables original Amenity controls",six_perf_enabled&&cur_page()->fam==FAM_SEQ);
 song.playing=1;song.rec=1u<<TEST_BREAK_TRACK;TSEL->seq_idx=7;TSEL->seq_pos=0;TSEL->p[P_E6]=1;six_break_clock(0,32,beat_samples());
 bad+=check("RETURN while recording becomes explicit sequence event",TSEL->step[7].time==ST_NOTE&&TSEL->step[7].note[0]==34);song.playing=0;song.rec=0;transport_req=0;
 TSEL->p[P_E1]=TSEL->p[P_E2]=90;seq_note_on(TSEL,34,100);bad+=check("sequenced RETURN restores straight break",!TSEL->p[P_E1]&&!TSEL->p[P_E2]);
 project_capture(&proj_scratch);bad+=check("break commands and holds save together",proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)&&proj_scratch.t[5].step[0].note[0]==26&&proj_scratch.t[5].step[1].time==ST_TIE&&proj_scratch.t[5].step[7].note[0]==34);
 song.sel=TEST_ACID_TRACK;go_title("303 GEN 1");TSEL->p[P_ROOT]=TSEL->p[P_SCALE]=TSEL->p[P_LN5]=0;six_gen_apply(0,0);
 bad+=check("default Acid generation follows common CHR without rewriting SCL",TSEL->p[P_SCALE]==0);
 int scales_ok=1;for(unsigned root=0;root<12;root++)for(unsigned sc=0;sc<NELEM(SCALE_MASK);sc++){
  TSEL->p[P_ROOT]=root;TSEL->p[P_SCALE]=sc;TSEL->p[P_LN5]=64;
  for(unsigned mode=0;mode<2;mode++){if(mode){TSEL->step[0].time=ST_NOTE;TSEL->step[0].n=1;TSEL->step[0].note[0]=1;}six_gen_apply(mode,0);for(unsigned i=0;i<(unsigned)TSEL->p[P_SLEN];i++){const step_t *z=&TSEL->step[i];if(z->time==ST_NOTE&&z->n)scales_ok&=(SCALE_MASK[sc]>>((z->note[0]+120-root)%12))&1u;}}
 }
 bad+=check("NEW and MUTATE always match all 12 roots and 16 SCL scales",scales_ok);
 TSEL->p[P_ROOT]=TSEL->p[P_SCALE]=0;TSEL->p[P_LN5]=64;six_gen_apply(0,0);bad+=check("explicit CHR is never replaced with MIN",TSEL->p[P_SCALE]==0);
 go_title("SCL");frame();turn(EN_K2,2);bad+=check("SCL UI changes the common scale directly",TSEL->p[P_SCALE]==2);
 go_title("303 GEN 2");frame();bad+=check("Acid generation page uses actual SCL parameters",cur_page()->id[0]==P_ROOT&&cur_page()->id[1]==P_SCALE);
 favorite_set(ENGI_M2STAT,0,1);favorite_set(ENGI_M2SAW,0,1);favorite_set(ENGI_M2SID,0,1);bad+=check("MONO2K favorites keep old engines and metadata",favorite_has(ENGI_M2STAT,0)&&favorite_has(ENGI_M2SAW,0)&&favorite_has(ENGI_M2SID,0)&&favorite_has(ENGI_ACID,0)&&favorites.factory[15][15]==0x61u);
 const char *titles[]={"SCL","CHORD","ARP","PHASER"};
 for(unsigned k=0;k<4;k++){go_title(titles[k]);frame();bad+=check(titles[k],!ui.home);}
 for(unsigned k=0;k<5;k++){
  int32_t o[64];long long energy=0; for(unsigned j=0;j<6;j++)trk[j].p[P_MUTE]=(j!=k); for(unsigned b=0;b<64;b++)mix_block(o,32);song.sel=k;track_t *t=&trk[k];
  trk_note_on(t,k==SIX_DRUM_TRACK?36:48,127);
  for(unsigned b=0;b<400;b++){mix_block(o,32);for(unsigned i=0;i<64;i++)energy+=llabs(o[i]);}
  bad+=check("each pitched/drum track renders audio",energy>0);trk_all_off(t);
 }
 if(argc>1){
  for(unsigned k=0;k<6;k++){motion_clear(&trk[k]);trk[k].p[P_MUTE]=0;trk[k].p[P_DIST]=35;trk[k].p[P_CHOR]=50;trk[k].p[P_DLY]=25;trk[k].p[P_REV]=25;}
  for(unsigned k=1;k<5;k++){trk[k].p[P_L2RATE]=(int16_t)(25+k*11);trk[k].p[P_M1SRC]=MS_LFO;trk[k].p[P_M1DST]=MD_PAN;trk[k].p[P_M1AMT]=8;trk[k].p[P_M2SRC]=MS_LFO2;trk[k].p[P_M2DST]=MD_CUT;trk[k].p[P_M2AMT]=6;}
  /* Demo starts with the actual two-bar loops unwarped. Sequencing tests above
   * remain populated; the packaged demonstration uses a visible two-bar CHOP pattern. */
  trk[TEST_BREAK_TRACK].p[P_LN0]=1;trk[TEST_BREAK_TRACK].p[P_E4]=0;
  trk[TEST_BREAK_TRACK].p[P_E1]=trk[TEST_BREAK_TRACK].p[P_E2]=0;
  for(unsigned i=0;i<NSTEP;i++){step_t *s=&trk[TEST_BREAK_TRACK].step[i];step_clear(s);if(i<32){s->time=(i&1)?ST_TIE:ST_NOTE;if(!(i&1)){s->n=1;s->note[0]=i/2;s->vel=96;}}}
  trk[TEST_BREAK_TRACK].p[P_SLEN]=32;trk[TEST_BREAK_TRACK].p[P_SGATE]=127;
  for(unsigned k=0;k<5;k++){trk[k].p[P_SLEN]=16;for(unsigned i=16;i<NSTEP;i++)step_clear(&trk[k].step[i]);}
  song.sel=4;song.g[G_BPM]=160;project_capture(&proj_scratch);
  bad+=check("demo captures sequenced two-bar CHOP and slot A",proj_scratch.t[TEST_BREAK_TRACK].p[P_LN0]==1&&proj_scratch.t[TEST_BREAK_TRACK].p[P_E4]==0);
  bad+=check("pack demo",proj_pack(&proj_wire,&proj_scratch));
  bad+=check("packed demo retains sequenced CHOP and slot A",proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)&&proj_scratch.t[TEST_BREAK_TRACK].p[P_LN0]==1&&proj_scratch.t[TEST_BREAK_TRACK].p[P_E4]==0);
  FILE *f=fopen(argv[1],"wb");if(!f)return 2;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 if(argc>3){
  /* Hands-on browser audition: keep all six machines/patterns, but avoid
   * stress-test FX on every track. The full-load fixture above stays intact. */
  for(unsigned k=0;k<6;k++){trk[k].p[P_DIST]=0;trk[k].p[P_CHOR]=0;trk[k].p[P_DLY]=0;trk[k].p[P_REV]=0;}
  trk[TEST_ACID_TRACK].p[P_DIST]=12;song.sel=0;
  project_capture(&proj_scratch);bad+=check("pack six-track hands-on audition",proj_pack(&proj_wire,&proj_scratch));
  FILE *f=fopen(argv[3],"wb");if(!f)return 2;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 if(argc>4){
  for(unsigned k=1;k<4;k++){
   set_engine_of(&trk[k],ENGI_WAVETABLE);apply_preset_to(&trk[k],3);
   static const int16_t e[8]={64,96,20,40,96,80,80,SF_LADDER};
   for(unsigned i=0;i<8;i++)trk[k].p[P_E0+i]=e[i];
  }
  for(unsigned k=0;k<6;k++){trk[k].p[P_DIST]=35;trk[k].p[P_CHOR]=50;trk[k].p[P_DLY]=25;trk[k].p[P_REV]=25;}
  project_capture(&proj_scratch);bad+=check("pack triple-Wavetable stress fixture",proj_pack(&proj_wire,&proj_scratch));
  FILE *f=fopen(argv[4],"wb");if(!f)return 2;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 if(argc>7){
  const unsigned engine[3]={ENGI_M2STAT,ENGI_M2SAW,ENGI_M2SID};
  for(unsigned family=0;family<3;family++){
   for(unsigned k=1;k<4;k++){
    set_engine_of(&trk[k],engine[family]);apply_preset_to(&trk[k],family==1?3:0);
    if(family==1){trk[k].p[P_E0]=trk[k].p[P_E1]=trk[k].p[P_E2]=trk[k].p[P_E4]=trk[k].p[P_E5]=trk[k].p[P_E6]=127;}
    if(family==0){trk[k].p[P_E3]=112;trk[k].p[P_E5]=127;trk[k].p[P_E6]=100;}
    if(family==2){trk[k].p[P_E4]=96;trk[k].p[P_E6]=100;}
   }
   for(unsigned k=0;k<6;k++){trk[k].p[P_DIST]=35;trk[k].p[P_CHOR]=50;trk[k].p[P_DLY]=25;trk[k].p[P_REV]=25;}
   project_capture(&proj_scratch);bad+=check("pack triple MONO2K stress fixture",proj_pack(&proj_wire,&proj_scratch));
   FILE *f=fopen(argv[5+family],"wb");if(!f)return 2;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
  }
 }
 if(argc>2){
  trks_put(0);gp.form=0;gp.genre=1;gp.seed=2401;gen_go();song.sel=0;song.g[G_BPM]=160;
  project_capture(&proj_scratch);bad+=check("pack four-track MONO demo",proj_pack(&proj_wire,&proj_scratch));
  FILE *f=fopen(argv[2],"wb");if(!f)return 2;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 }
 bad+=test_save(); /* inherited JIANT save/overwrite/cancel/load button flow */
 printf("SIX integration: %s (%d failures)\n",bad?"FAIL":"PASS",bad);return bad?1:0;
}
