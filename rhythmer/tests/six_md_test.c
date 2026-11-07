/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#define RHYTHMER_MD_TEST 1
#include "ui_test.c"
int main(int argc,char **argv){
 if(argc!=3)return 2;ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(1);int bad=0;
 bad+=check("DRUM retains nine original kits plus ten MD kits",DX_NKIT==19&&ENG_DRUM.npresets==19);
 for(unsigned i=0;i<45;i++)bad+=check("classic and all MD choices round trip",dx_mode_ix(dx_mode_of(i))==i);
 for(unsigned kind=0;kind<MD_NMODEL;kind++){
  int peak=0;int64_t energy=0;
  for(unsigned variant=0;variant<8;variant++){
   drum_md_reset();drum_lane_t *l=&drum_kit[0][3];memset(l,0,sizeof *l);l->x.trig=1;
   dx_lane_t *pp=&dx_kit[3];md_patch_default(pp,kind);dx_lane_t p=*pp;
   if(variant)for(unsigned k=0;k<5;k++)pp->a[k]=pp->b[k]=(variant>>(k%3))&1?127:0;
   for(unsigned b=0;b<700;b++){
    int32_t y[32];md_lane_run(&trk[0],l,pp,0,(variant&1)?384:-384,0,0,0,y,32);
    for(unsigned j=0;j<32;j++){int a=y[j]<0?-y[j]:y[j];if(a>peak)peak=a;energy+=(int64_t)y[j]*y[j];if(a>1048576)return 3;}
   }
  }
  printf("MD %s: peak %d energy %lld\n",MD_MODELS[kind].name,peak,(long long)energy);
  bad+=check("MD extremes remain bounded and audible",peak>10&&energy>100000);
 }
 go_title("SOUND 3");dx_ui_lane=0;dx_kit[0].mode=dx_mode_of(0);
 for(unsigned i=1;i<45;i++){dx_knob(cur_page(),1,1);bad+=check("physical MODE knob reaches every MD model",dx_mode_ix(dx_kit[0].mode)==i);}
 uint8_t mode=dx_kit[0].mode;dx_knob(cur_page(),2,1);bad+=check("MD FAMILY readout cannot corrupt model bits",dx_kit[0].mode==mode);
 for(unsigned i=0;i<21;i++){char name[8];dx_mode_name(name,DX_MD(i));bad+=check("all MD names match UI model selection",!strcmp(name,MD_MODELS[i].name));}
 drum_md_reset();memset(drum_kit,0,sizeof drum_kit);
 dx_lane_t hat;md_patch_default(&hat,0);int32_t hit[32];
 drum_lane_t *closed=&drum_kit[0][3],*open=&drum_kit[0][4];open->x.trig=1;
 md_lane_run(&trk[0],open,&hat,0,0,0,0,0,hit,32);closed->x.trig=1;
 md_lane_run(&trk[0],closed,&hat,0,0,0,0,0,hit,32);
 bad+=check("MD hats transfer choke ownership with a short tail",md_find(0,&closed->x)!=0&&open->x.b2==32);
 md_lane_run(&trk[0],open,&hat,0,0,0,0,0,hit,32);
 bad+=check("choked MD hat ends after the handoff tail",!open->x.live&&md_find(0,&closed->x)!=0);
 md_voice_t before=*md_find(0,&closed->x);drum_kit[1][3].x.trig=1;
 md_lane_run(&trk[1],&drum_kit[1][3],&hat,0,0,0,0,0,hit,32);
 bad+=check("MD phases and ownership are independent per track",!memcmp(&before,md_find(0,&closed->x),sizeof before)&&md_find(1,&drum_kit[1][3].x)!=0);
 ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(1);
 for(unsigned k=0;k<6;k++){trk_all_off(&trk[k]);trk[k].p[P_MUTE]=k!=0;}
 set_engine_of(&trk[0],ENGI_DRUM);trk[0].engine=ENGI_DRUM;trk[0].xf=trk[0].xf_on=0;apply_preset_to(&trk[0],9);dx_kit_load(9);panic_req=0;xf_snap_t=0xFF;trk[0].xf=trk[0].xf_on=0;memset(drum_kit,0,sizeof drum_kit);
 trk_note_on(&trk[0],DRUM_LANE_NOTE[3],127);int32_t trans[64];mix_block(trans,32);bad+=check("initial MD note is actually rendered",dx_is_md(drum_kit[0][3].x.model)&&md_find(0,&drum_kit[0][3].x)!=0);
 dx_lane_t current=dx_kit[3];md_patch_default(&dx_kit[3],16);mix_block(trans,32);
 bad+=check("changing MD type waits for next hit",drum_kit[0][3].x.model==current.mode);
 trk_note_on(&trk[0],DRUM_LANE_NOTE[3],127);mix_block(trans,32);bad+=check("new hit starts newly selected MD model",dx_md_id(drum_kit[0][3].x.model)==16);
 current=dx_kit[3];dx_kit[3]=DX_KIT_DEF[3];mix_block(trans,32);bad+=check("MD to classic change also waits for new hit",drum_kit[0][3].x.model==current.mode);
 trk_note_on(&trk[0],DRUM_LANE_NOTE[3],127);mix_block(trans,32);bad+=check("new classic hit leaves MD state safely",!dx_is_md(drum_kit[0][3].x.model));
 ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(1);
 for(unsigned k=0;k<6;k++){set_engine_of(&trk[k],ENGI_DRUM);trk[k].engine=ENGI_DRUM;trk[k].xf=trk[k].xf_on=0;apply_preset_to(&trk[k],11);trk[k].xf=trk[k].xf_on=0;}
 bad+=check("MD kit fits the existing section format",dx_kit_ok(dx_kit));
 for(unsigned kind=0;kind<MD_NMODEL;kind++){
  md_patch_default(&dx_kit[3],kind);project_capture(&proj_scratch);bad+=check("all MD models pack",proj_pack(&proj_wire,&proj_scratch)!=0);
  project_t copy;bad+=check("all MD models import",proj_import(&copy,&proj_wire,sizeof proj_wire)!=0);
  project_store_t packed;bad+=check("MD archive preserves mode and parameters exactly",proj_pack(&packed,&copy)!=0&&!memcmp(&packed,&proj_wire,sizeof packed));
 }
 dx_kit_load(11);panic_req=0;xf_snap_t=0xFF;apply_preset_to(&trk[0],11);panic_req=0;
 for(unsigned k=0;k<6;k++)memset(trk[k].v,0,sizeof trk[k].v);
 sys_load=LOAD_FULL;mc_host=1;mc_host_v12=1;mc_off=0;
 bad+=check("MD does not reduce the JIANT twelve-voice budget",voice_budget()==12u);
 bad+=check("unused expensive lane does not lower normal drum polyphony",trk_nvoice(&trk[0])==8u);
 trk_note_on(&trk[0],56,127);bad+=check("sounding TRX cowbell limits MD only",md_track_poly(&trk[0])==2u&&trk_nvoice(&trk[0])==8u);
 trk_note_on(&trk[0],36,127);bad+=check("two costly MD hits leave ordinary shared polyphony intact",voice_budget()==12u);
 for(unsigned k=0;k<6;k++)memset(trk[k].v,0,sizeof trk[k].v);drum_md_reset();memset(drum_kit,0,sizeof drum_kit);
 for(unsigned k=0;k<6;k++)for(unsigned j=0;j<4;j++){
  trk_note_on(&trk[k],DRUM_LANE_NOTE[j*2],127);
  bad+=check("dense MD note input never exceeds actual shared budget",voices_busy()<=voice_cap());
 }
 md_test_triggers=md_test_renders=0;int32_t burst[64];mix_block(burst,32);
 bad+=check("stolen pending MD notes never execute a full trigger",md_test_triggers<=4u);
 bad+=check("MD burst renders only the retained voice budget",md_test_renders<=4u);
 for(unsigned k=0;k<6;k++)memset(trk[k].v,0,sizeof trk[k].v);drum_md_reset();memset(drum_kit,0,sizeof drum_kit);
 mc_host=atoi(argv[1]);mc_host_v12=1;mc_off=0;gov_cap=4;song.master_q12=4096;FILE *f=fopen(argv[2],"wb");if(!f)return 4;unsigned split=0;
 for(unsigned b=0;b<1600;b++){
  if(!(b%32))for(unsigned k=0;k<6;k++)trk_note_on(&trk[k],DRUM_LANE_NOTE[(b/32+k)%8],127);
  int32_t out[64];mix_block(out,32);if(mc_mask_last&&mc_mask_last!=63)split++;fwrite(out,sizeof out,1,f);
 }
 fclose(f);
 song.sel=0;ui.home=0;ui.menu=0;go_title("SOUND 3");dx_ui_lane=7;dx_ui_seen=last_note;md_patch_default(&dx_kit[7],12);ui.force=1;frame();
 char path[1024];snprintf(path,sizeof path,"%s.ui.ppm",argv[2]);FILE *screen=fopen(path,"wb");if(!screen)return 5;
 fprintf(screen,"P6\n240 240\n255\n");for(unsigned i=0;i<240*240;i++){unsigned p=swap16(host_screen[i]);unsigned char rgb[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,screen);}fclose(screen);
 bad+=check("MD tracks participate in both cores",!mc_host||split>0);
 printf("MD %s-core render: %s (split blocks %u, hit storage %zu B)\n",mc_host?"two":"one",bad?"FAIL":"PASS",split,sizeof md_pool+sizeof md_aux);return !!bad;
}
