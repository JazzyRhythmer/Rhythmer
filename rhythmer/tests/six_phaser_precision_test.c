/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
/* Independent double-precision reference for the original four allpasses. */
int main(void){
 double z[4]={0},fb=0,c=0,error=0,signal=0;unsigned clipped=0;
 memset(six_ph_ap,0,sizeof six_ph_ap);six_ph_fb=six_ph_coeff=0;
 for(unsigned b=0;b<9000;b++){
  uint32_t phase=b*2454321u;int depth=b/1500%2?105:40,feedback=(int)(b/1500%3)*50-50;
  double tri=(double)((phase>>8)&0xffffffu)/16777216.0;tri=tri<.5?tri*2:2-tri*2;
  double g=(180+depth*(2800.0/127)*tri)*(3.14159265/44100.0),target=(1-g)/(1+g),dc=(target-c)*.014879;
  double gain=(feedback+64)*(.7/127);
  six_phaser_prepare(phase,depth,feedback,32);
  for(unsigned j=0;j<32;j++){
   unsigned at=b*32+j;int in=b<7500?(int)(sin(at*.037)*10000+sin(at*.091)*6000):0;
   c+=dc;double x=in/32768.0+fb*gain;if(x>4)x=4;if(x< -4)x=-4;
   for(unsigned k=0;k<4;k++){double y=-c*x+z[k];z[k]=x+c*y;x=y;}
   fb=x;int actual=six_phaser(in);double wanted=x*16384,d=actual-wanted;
   if(b>100){error+=d*d;signal+=wanted*wanted;}
   clipped+=abs(actual)>131072;
  }
 }
 double snr=10*log10(signal/error);printf("Phaser reference SNR %.2f dB, silent tail %d, bound violations %u\n",snr,six_ph_fb,clipped);
 return !(snr>55&&!clipped&&abs(six_ph_fb)<16);
}
