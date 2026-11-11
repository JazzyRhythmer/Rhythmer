/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void){int bad=0;ui_power_on();fm6_init();power_on_parts();test_optional_machines();six_native_init();trks_put(1);song.sel=5;
 static uint8_t a[72000],b[72000];six_loops[0]=a;six_loops[1]=b;six_loop_mask=3;jungle_bind(six_break_prepare(&trk[5]),a,72000,48000);
 track_t *t=TSEL;t->engine=t->eng_req;t->xf=t->xf_on=0;t->p[P_SLEN]=3;t->p[P_SGATE]=64;
 for(unsigned i=0;i<NSTEP;i++)step_clear(&t->step[i]);
 t->step[0].time=t->step[1].time=ST_NOTE;t->step[0].n=t->step[1].n=1;t->step[0].note[0]=3;t->step[1].note[0]=33;
 t->seq_idx=0;seq_step(t,&t->step[0],1000,0);bad+=check("next A/B command keeps preceding chop held",t->seq_n==1&&t->seq_notes[0]==3&&t->seq_hold);
 t->seq_idx=1;seq_step(t,&t->step[1],1000,0);bad+=check("B changes source without replacing the active chop",t->p[P_E4]==1&&t->seq_n==1&&t->seq_notes[0]==3&&six_break_state(TSEL)->loop_now==1);
 t->seq_idx=2;seq_step(t,&t->step[2],1000,0);bad+=check("REST after source change releases the actual sequence",t->seq_n==0);
 t->step[1].note[0]=34;t->seq_idx=0;seq_step(t,&t->step[0],1000,0);t->seq_idx=1;seq_step(t,&t->step[1],1000,0);bad+=check("RETURN is a non-destructive held-step modifier",t->seq_n==1&&t->seq_notes[0]==3&&t->step[0].note[0]==3);
 seq_release(t);t->step[1].note[0]=32;seq_step(t,&t->step[1],1000,0);bad+=check("source-only event does not invent a trigger on empty sequence",t->seq_n==0&&t->p[P_E4]==0);
 printf("Break sequence modifiers: %s\n",bad?"FAIL":"PASS");return bad?1:0;}
