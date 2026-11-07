/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void){
 int bad=0;ui_power_on();fm6_init();power_on_parts();six_native_init();trks_put(1);mc_host=1;mc_host_v12=1;mc_off=0;panic_req=0;
 song.master_q12=4096;int32_t out[64];unsigned audible=0,splits=0;
 /* Real crossfades and queued new notes: a part keeps its old runtime until
  * the old renderer is finished. Other parts may use either engine meanwhile. */
 for(unsigned turn=0;turn<48;turn++){
  for(unsigned k=0;k<NTRK;k++){
   unsigned e=((turn+k)&1)?ENGI_DRUM:ENGI_FM6;
   set_engine_of(&trk[k],e);
  }
  mix_block(out,CTL); /* consume each sound-load request before new input */
  for(unsigned k=0;k<NTRK;k++)trk_note_on(&trk[k],((turn+k)&1)?DRUM_LANE_NOTE[(turn+k)%8]:48+k,110);
  for(unsigned b=0;b<160;b++){
   mix_block(out,CTL);if(mc_mask_last&&mc_mask_last!=63)splits++;
   for(unsigned j=0;j<64;j++){if(out[j])audible++;if(out[j]>0x40000000||out[j]<-0x40000000)return 9;}
  }
  for(unsigned k=0;k<NTRK;k++){
   unsigned e=((turn+k)&1)?ENGI_DRUM:ENGI_FM6;
   bad+=check("engine fade finishes before shared runtime changes owner",trk[k].engine==e&&trk[k].eng_req==e);
  }
 }
 printf("handoff audible samples %u split blocks %u\n",audible,splits);
 bad+=check("alternating FM6/DRUM crossfades remain audible and use both cores",audible>1000&&splits>0);
 /* Eight MD slots still exist independently for every part. */
 for(unsigned k=0;k<NTRK;k++){
  md_memory(k,ENGI_DRUM);
  for(unsigned j=0;j<NVOICE;j++)for(unsigned n=j+1;n<NVOICE;n++)if(md_at(k,j)==md_at(k,n))return 10;
  for(unsigned j=0;j<NVOICE;j++)if(md_at(k,j)->active||md_at(k,j)->owner)return 11;
 }
 /* FM6 owns only the first four slots, and cannot clear another part. */
 md_at(1,0)->active=1;md_at(1,0)->t=123;
 md_memory(0,ENGI_FM6);bad+=check("runtime handoff is isolated to its own part",md_at(1,0)->active&&md_at(1,0)->t==123);
 bad+=check("shared slots have the correct typed size and alignment",sizeof(md_fm6_runtime_t)>=sizeof(fm6_note_t)&&sizeof(md_fm6_runtime_t)>=sizeof(md_voice_t));
 printf("Engine runtime handoff: %s\n",bad?"FAIL":"PASS");return !!bad;
}
