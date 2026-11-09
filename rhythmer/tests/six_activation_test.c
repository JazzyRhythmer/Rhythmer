/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static void enable(void){trks_request(1);press(B_OCTUP);}
int main(void){
 int bad=0;ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(0);
 bad+=check("four-track startup leaves optional instruments empty",trk[4].eng_req==ENGI_EMPTY&&trk[5].eng_req==ENGI_EMPTY);
 unsigned chr=1;for(unsigned k=0;k<NTRK;k++)chr&=trk[k].p[P_SCALE]==0;
 bad+=check("fresh shared SCL defaults to CHR without a private minor fallback",chr);
 for(unsigned k=4;k<6;k++){step_t*s=&trk[k].step[3];s->time=ST_NOTE;s->n=1;s->note[0]=40+k;s->vel=92;}
 unsigned sig5=steps_sig(&trk[4]),sig6=steps_sig(&trk[5]);
 menu_put(MI_TRACKS,1);press(B_OCTDN);
 bad+=check("cancel keeps four tracks and both assignments empty",trk_live==4&&trk[4].eng_req==ENGI_EMPTY&&trk[5].eng_req==ENGI_EMPTY);
 enable();bad+=check("confirmed six-track mode fills empty T5 Acid and T6 Break",trk_live==6&&trk[4].eng_req==ENGI_ACID&&trk[5].eng_req==ENGI_BREAK&&six_break_track()==5);
 bad+=check("activation preserves existing MIDI without generating a pattern",steps_sig(&trk[4])==sig5&&steps_sig(&trk[5])==sig6);
 bad+=check("activation uses the native Hortator default and Amen bank A",trk[4].preset==0&&trk[4].p[P_E5]==0&&trk[5].p[P_E4]==0);
 set_engine_of(&trk[4],ENGI_SERUM);set_engine_of(&trk[5],ENGI_PHASE);
 int16_t p5[P_COUNT],p6[P_COUNT];memcpy(p5,trk[4].p,sizeof p5);memcpy(p6,trk[5].p,sizeof p6);
 trks_request(0);enable();bad+=check("re-enabling never replaces assigned sounds or parameters",trk[4].eng_req==ENGI_SERUM&&trk[5].eng_req==ENGI_PHASE&&!memcmp(p5,trk[4].p,sizeof p5)&&!memcmp(p6,trk[5].p,sizeof p6));
 set_engine_of(&trk[1],ENGI_BREAK);set_engine_of(&trk[4],ENGI_EMPTY);set_engine_of(&trk[5],ENGI_EMPTY);
 trks_request(0);enable();bad+=check("Break on another track prevents duplicate automatic Break",trk[4].eng_req==ENGI_ACID&&trk[5].eng_req==ENGI_EMPTY&&six_break_track()==1);
 set_engine_of(&trk[4],ENGI_EMPTY);rp_apply();bad+=check("applying saved mode at boot never invents assignments",trk_live==6&&trk[4].eng_req==ENGI_EMPTY&&trk[5].eng_req==ENGI_EMPTY);
 bad+=check("all activation paths keep optional-track sequences",steps_sig(&trk[4])==sig5&&steps_sig(&trk[5])==sig6);
 song.sel=4;list_put(0);set_engine_of(TSEL,ENGI_EMPTY);unsigned total;unsigned pos=preset_all_pos(NENGINES,&total),sound=0;
 bad+=check("EMPTY is outside the preset carousel but remains a selectable model",pos>=total&&six_engine_available(4,ENGI_EMPTY));
 for(unsigned i=0;i<total;i++){unsigned k,e=preset_all_at(NENGINES,i,&k);sound+=e==ENGI_EMPTY;}
 bad+=check("no factory or saved EMPTY entries appear in sound browsing",sound==0);
 unsigned first,first_e=preset_all_at(NENGINES,0,&first);preset_step(1);
 bad+=check("first right detent from EMPTY reaches the first actual sound",TSEL->eng_req==first_e&&TSEL->preset==first);
 set_engine_of(TSEL,ENGI_ACID);apply_preset_to(TSEL,ENG_ACID.npresets-1);preset_step(1);
 bad+=check("next sound after the last Acid preset skips SELECT ENGINE",TSEL->eng_req!=ENGI_EMPTY);
 printf("Six-track activation defaults: %s (%d failures)\n",bad?"FAIL":"PASS",bad);return !!bad;
}
