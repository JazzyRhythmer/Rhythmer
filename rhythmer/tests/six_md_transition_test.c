/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static void status(const char *name){drum_lane_t *l=&drum_kit[0][3];printf("%s: engine %u/%u xf %u mute %d mode %u live %u trig %u owner %u md %d v0 %u/%u/%u note %u\n",name,trk[0].engine,trk[0].eng_req,trk[0].xf_on,trk[0].p[P_MUTE],l->x.model,l->x.live,l->x.trig,l->owner,md_find(0,&l->x)!=0,trk[0].v[0].active,trk[0].v[0].gate,trk[0].v[0].stage,trk[0].v[0].note);}
int main(void){ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(1);song.playing=0;int bad=0;
 for(unsigned k=0;k<6;k++){set_engine_of(&trk[k],ENGI_DRUM);trk[k].engine=ENGI_DRUM;apply_preset_to(&trk[k],9);trk[k].xf=trk[k].xf_on=0;trk[k].p[P_MUTE]=k!=0;}
 dx_kit_load(9);panic_req=0;xf_snap_t=0xFF;memset(drum_kit,0,sizeof drum_kit);drum_md_reset();int32_t out[64];
 trk_note_on(&trk[0],42,127);status("first before");mix_block(out,32);status("first after");
 bad+=check("initial MD note rendered",md_find(0,&drum_kit[0][3].x)!=0);
 md_patch_default(&dx_kit[3],16);mix_block(out,32);status("changed");bad+=check("mode waits for hit",drum_kit[0][3].x.model==DX_MD(0));
 trk_note_on(&trk[0],42,127);status("new before");mix_block(out,32);status("new after");bad+=check("next hit uses new model",drum_kit[0][3].x.model==DX_MD(16));
 dx_kit[3]=DX_KIT_DEF[3];mix_block(out,32);bad+=check("classic mode waits for hit",drum_kit[0][3].x.model==DX_MD(16));
 trk_note_on(&trk[0],42,127);mix_block(out,32);bad+=check("next hit safely uses classic model",!dx_is_md(drum_kit[0][3].x.model));return bad;
}
