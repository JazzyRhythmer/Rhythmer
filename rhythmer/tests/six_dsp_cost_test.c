/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void){
 uint32_t seed=0x701123u;unsigned checks=0;
 for(unsigned i=0;i<128;i++){
  int32_t a=m2_T_SWHP[i];if(a<0||a>32767)return 2;
  static const int32_t edge[]={INT32_MIN,INT32_MIN+1,-65536,-32769,-32768,-1,0,1,32767,32768,65535,INT32_MAX};
  for(unsigned j=0;j<sizeof edge/sizeof edge[0];j++){if(m2_hp_q15(a,edge[j])!=(int32_t)(((int64_t)a*edge[j])>>15))return 3;checks++;}
  for(unsigned j=0;j<1000;j++){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;int32_t b=(int32_t)seed;if(m2_hp_q15(a,b)!=(int32_t)(((int64_t)a*b)>>15))return 4;checks++;}
 }
 printf("SWAV highpass: %u exact wide-reference comparisons PASS\n",checks);return 0;
}
