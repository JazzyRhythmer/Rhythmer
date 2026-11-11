/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(int argc,char **argv){
 if(argc!=4)return 2;ui_power_on();fm6_init();power_on_parts();test_optional_machines();six_native_init();trks_put(1);song.master_q12=4096;
 static uint8_t sample[72000];FILE *f=fopen(argv[3],"rb");if(!f)return 3;fseek(f,512,SEEK_SET);if(fread(sample,1,sizeof sample,f)!=sizeof sample)return 3;fclose(f);
 six_loops[0]=six_loops[1]=sample;six_loop_mask=3;jungle_bind(six_break_prepare(&trk[5]),sample,72000,48000);
 for(unsigned k=0;k<5;k++){set_engine_of(&trk[k],ENGI_FM6);trk[k].engine=trk[k].eng_req;trk[k].xf=trk[k].xf_on=0;apply_preset_to(&trk[k],0);trk[k].xf=trk[k].xf_on=0;}
 trk[5].engine=trk[5].eng_req=ENGI_BREAK;trk[5].xf=trk[5].xf_on=0;trk[5].p[P_LN0]=0;
 mc_host=atoi(argv[1]);mc_host_v12=1;mc_off=0;song.playing=1;
 f=fopen(argv[2],"wb");if(!f)return 3;unsigned split=0,energy=0;
 for(unsigned b=0;b<1200;b++){
  if(!(b%180)){trk_note_on(&trk[0],48,100);trk_note_on(&trk[0],55,100);}
  int32_t out[64];mix_block(out,32);if(mc_mask_last&(1u<<5))split++;
  for(unsigned j=0;j<64;j++)energy|=out[j]!=0;fwrite(out,sizeof out,1,f);
 }
 fclose(f);if(mc_host&&!split)return 4;if(!energy||!mc_cost(&trk[5]))return 5;
 printf("Break %s-core render PASS; Break worker blocks %u\n",mc_host?"two":"one",split);return 0;
}
