/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#define RHYTHMER_USB_HOST 1
#include "ui_test.c"
static int gate_note(const track_t *t,unsigned note){for(unsigned i=0;i<NVOICE;i++)if(t->v[i].active&&t->v[i].gate&&t->v[i].note==note)return 1;return 0;}
int main(void){
 int bad=0;ui_power_on();fm6_init();power_on_parts();six_native_init();song.sel=1;
 bad+=check("USB host starts off",!uh.on&&!uh.want&&!menu_get(MI_HOST));
 song.playing=1;menu_put(MI_HOST,1);bad+=check("playing blocks the host mode switch",!uh.want&&!ui.confirm);song.playing=0;
 ui.menu=1;menu_put(MI_HOST,1);bad+=check("host ON requires a visible on-device confirmation",!uh.want&&ui.confirm==CF_USB_HOST&&!ui.menu);
 press(B_OCTDN);frame();bad+=check("host confirmation cancel keeps computer mode",!uh.want);
 menu_put(MI_HOST,1);press(B_OCTUP);frame();bad+=check("OCT+ authorizes host mode",uh.want&&!ui.confirm);
 menu_put(MI_HOST,0);bad+=check("host OFF requests ordinary USB device mode",!uh.want);
 track_t *t=&trk[0];set_engine_of(t,ENGI_ANALOG);t->engine=ENGI_ANALOG;t->p[P_VOICE]=V_POLY;t->xf=t->xf_on=0;
 song.g[G_ROUTE]=0;panic_req=0;midi_event(0x90,0,60,100);t->seq_n=1;t->seq_notes[0]=60;
 midi_event(0x90,0,62,100);midi_event(0xB0,0,64,127);midi_event(0x80,0,62,0);
 bad+=check("fixture owns a sequenced unison and a sustained external note",mn_get(0,60)&&mn_get(0,62)&&gate_note(t,60)&&gate_note(t,62));
 midi_host_lost();
 bad+=check("lost peer clears MIDI ownership and sustain",!mn_get(0,60)&&!mn_get(0,62)&&!midi_ch[0].pedal&&!midi_owners[0]);
 bad+=check("lost peer preserves the sequencer's held note",gate_note(t,60)&&!gate_note(t,62));
 printf("USB host UI/ownership: %s (%d)\n",bad?"FAIL":"PASS",bad);return bad?1:0;
}
