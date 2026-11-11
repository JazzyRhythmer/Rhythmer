#define MULTI_BREAK_NO_MAIN 1
#include "multi_break_test.c"

int main(int argc,char**argv){
 if(argc!=7)return 2;
 for(unsigned i=0;i<2;i++){FILE*f=fopen(argv[i+1],"rb");if(!f)return 3;fseek(f,512,SEEK_SET);if(fread(bank[i],1,72000,f)!=72000)return 3;fclose(f);}
 reset_breaks();mc_host=atoi(argv[3]);mc_host_v12=1;mc_off=0;
 unsigned count=atoi(argv[6]);
 for(unsigned k=0;k<NTRK;k++){
  track_t*t=&trk[k];t->p[P_LEVEL]=80;t->p[P_DIST]=0;t->p[P_CHOR]=20;t->p[P_DLY]=24;t->p[P_REV]=28;
  t->p[P_SLEN]=32;t->p[P_SGATE]=127;t->p[P_MUTE]=k>=count;
  t->p[P_E0]=k*7;t->p[P_E2]=k*5;t->p[P_E3]=k&1?80:64;
  for(unsigned i=0;i<NSTEP;i++)step_clear(&t->step[i]);
  for(unsigned i=0;i<32;i+=2){step_t*s=&t->step[i];s->time=ST_NOTE;s->n=1;s->note[0]=(i/2+k*3)%16;s->vel=85+k*5;}
 }
 panic_req=0;project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))return 4;
 FILE*f=fopen(argv[5],"wb");if(!f)return 3;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
 f=fopen(argv[4],"wb");if(!f)return 3;
 transport_req=1;unsigned split=0;uint64_t energy=0;
 for(unsigned b=0;b<12000;b++){
  int32_t out[64];mix_block(out,32);split|=mc_mask_last;
  for(unsigned i=0;i<64;i++)energy+=llabs(out[i]);fwrite(out,sizeof out,1,f);
 }
 fclose(f);printf("%u Break tracks plus chorus, delay, reverb: core1 mask=%02x energy=%llu\n",count,split,(unsigned long long)energy);
 return !energy||(mc_host&&!split);
}
