/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(int argc,char **argv){
 int bad=0;ui_power_on();fm6_init();power_on_parts();six_native_init();song.sel=0;
 bad+=check("LXR-X uses twelve ordinary kits and 96 lane patches",LXRX_NKIT==12&&DX_NKIT==31&&ENG_DRUM.npresets==31);
 FILE *csv=argc>1?fopen(argv[1],"w"):0;
 if(csv)fprintf(csv,"kit,lane,peak,rms,nonzero\n");
 for(unsigned kit=LXRX_BASE_KIT;kit<DX_NKIT;kit++){
  dx_kit_load(kit);if(!dx_kit_ok(dx_kit))return 10;
  project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch)||!proj_import(&proj_scratch,&proj_wire,sizeof proj_wire))return 11;
  dx_lane_t saved[8];memcpy(saved,dx_kit,sizeof saved);
  if(project_restore_runtime(&proj_scratch)||memcmp(saved,dx_kit,sizeof saved))return 12;
  for(unsigned lane=0;lane<8;lane++){
   dx_voice_t v={0};v.trig=1;int32_t out[32];unsigned peak=0,nz=0;uint64_t e=0;
   for(unsigned b=0;b<FS/32;b++){
    dx_run(&dx_kit[lane],&v,0,0,0,0,0,0,0,out,32);
    for(unsigned j=0;j<32;j++){unsigned a=(unsigned)(out[j]<0?-out[j]:out[j]);if(a>peak)peak=a;nz+=a!=0;e+=(int64_t)out[j]*out[j];}
   }
   if(!nz||peak>65535){printf("bad LXR-X voice kit=%u lane=%u peak=%u\n",kit,lane,peak);return 13;}
   if(csv)fprintf(csv,"%u,%u,%u,%.3f,%u\n",kit-LXRX_BASE_KIT,lane,peak,sqrt((double)e/FS),nz);
   dx_ui_lane=(uint8_t)lane;dx_voice_pick(DX_LXRX_BASE+(kit-LXRX_BASE_KIT)*8+lane);
   if(dx_voice_current()!=(int)(DX_LXRX_BASE+(kit-LXRX_BASE_KIT)*8+lane))return 14;
  }
 }
 if(csv)fclose(csv);
 bad+=check("all LXR-X voices render bounded nonzero audio and projects round trip",1);
 bad+=check("all LXR-X individual sound choices retain their exact catalog positions",1);
 printf("LXR-X: %s (%d)\n",bad?"FAIL":"PASS",bad);return bad?1:0;
}
