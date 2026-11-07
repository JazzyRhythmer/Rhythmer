/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(int argc,char **argv){
 if(argc!=3)return 2;ui_power_on();fm6_init();power_on_parts();test_optional_machines();six_native_init();trks_put(1);song.master_q12=4096;
 mc_host=atoi(argv[1]);mc_host_v12=1;mc_off=0;gov_cap=8; /* Equal voice cap for PCM parity, independent of core count. */
 for(unsigned k=0;k<NTRK;k++){set_engine_of(&trk[k],ENGI_DRUM);trk[k].engine=trk[k].eng_req;trk[k].xf=trk[k].xf_on=0;apply_preset_to(&trk[k],0);trk[k].xf=trk[k].xf_on=0;trk[k].p[P_DXIMG]=70;trk[k].p[P_DIST]=60;trk[k].p[P_CHOR]=90;trk[k].p[P_DLY]=80;trk[k].p[P_REV]=80;}
 for(unsigned l=0;l<8;l++)dx_snd[l]=(uint8_t)(((l%3+1)<<6)|35);
 FILE *f=fopen(argv[2],"wb");if(!f)return 3;unsigned split=0;
 for(unsigned b=0;b<1500;b++){
  if(!(b%32))for(unsigned k=0;k<6;k++)trk_note_on(&trk[k],DRUM_LANE_NOTE[(k+b/32)%8],127);
  int32_t out[64];mix_block(out,32);if(mc_mask_last&&mc_mask_last!=63)split++;
  fwrite(out,sizeof out,1,f);
 }
 fclose(f);if(mc_host&&!split)return 4;
 mc_host_cpu=0;int32_t *a=dx_dump;mc_host_cpu=1;int32_t *b=dx_dump;mc_host_cpu=0;if(a==b)return 5;
 printf("Drum %s-core render PASS; balanced blocks %u; independent discard buffers\n",mc_host?"two":"one",split);return 0;
}
