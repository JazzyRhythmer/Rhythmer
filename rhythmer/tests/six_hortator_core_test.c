/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(int argc,char **argv){
 if(argc!=5)return 2;FILE *out=fopen(argv[2],"wb");if(!out)return 3;
 static uint8_t sample[2][72000];
 for(unsigned k=0;k<2;k++){char path[1024];snprintf(path,sizeof path,"%s/%s.slot",argv[3],k?"think-160":"amen-160");FILE *f=fopen(path,"rb");if(!f)return 4;fseek(f,512,SEEK_SET);if(fread(sample[k],1,72000,f)!=72000)return 5;fclose(f);}
 int bad=0;
 for(unsigned style=0;style<2;style++){
  ui_power_on();fm6_init();power_on_parts();test_optional_machines();six_native_init();trks_put(1);song.master_q12=4096;mc_host=atoi(argv[1]);mc_host_v12=1;mc_off=0;gov_cap=16;sys_load=LOAD_FULL;
  six_loops[0]=sample[0];six_loops[1]=sample[1];six_loop_mask=3;six_loop_now=0;jungle_bind(sample[0],72000,48000);
  for(unsigned k=0;k<6;k++){
   track_t *t=&trk[k];unsigned e=k==5?ENGI_BREAK:style?ENGI_ACID:(unsigned[]){ENGI_DRUM,ENGI_WAVETABLE,ENGI_PHASE,ENGI_LOFI,ENGI_ACID}[k];
   set_engine_of(t,e);t->engine=e;apply_preset_to(t,e==ENGI_ACID?k%4:e==ENGI_DRUM?14:0);t->xf=t->xf_on=0;t->p[P_MUTE]=0;t->p[P_SLEN]=32;t->p[P_SGATE]=100;
   for(unsigned i=0;i<NSTEP;i++){step_t *s=&t->step[i];step_clear(s);if(i<32){s->time=i%8?ST_TIE:ST_NOTE;if(!(i%8)){s->vel=120;s->n=k==2&&!style?3:1;for(unsigned j=0;j<s->n;j++)s->note[j]=k==5?i/2:k==0&&!style?36:36+k*5+j*4;}}}
  }
  trk[5].p[P_LN0]=1;dx_kit_load(14);panic_req=0;xf_snap_t=0xff;song.g[G_BPM]=170;
  project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))return 6;
  char path[1024];snprintf(path,sizeof path,"%s/upgrade-ensemble-%u.project",argv[4],style);FILE *f=fopen(path,"wb");if(!f)return 7;fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
  song.playing=1;unsigned split=0,energy=0;
  for(unsigned b=0;b<2400;b++){
   if(!(b%160)){
    for(unsigned k=0;k<5;k++){
     if(!style&&k==0){trk_note_on(&trk[k],36,127);trk_note_on(&trk[k],42,100);}
     else for(unsigned j=0;j<(!style&&k==2?3u:1u);j++)trk_note_on(&trk[k],36+k*5+j*4,120);
    }
    six_break_note(&trk[5],(b/160)%16,127);
    if(b%320==0)six_break_select((b/320)&1u,1);
   }
   int32_t y[64];mix_block(y,32);split+=mc_mask_last&&mc_mask_last!=63;
   for(unsigned j=0;j<64;j++)energy|=y[j]!=0;fwrite(y,sizeof y,1,out);
  }
  bad+=check("Hortator ensemble audibly renders with an active worker split",energy&&(!mc_host||split));
 }
 fclose(out);printf("Hortator %s-core PCM: %s\n",mc_host?"two":"one",bad?"FAIL":"PASS");return !!bad;
}
