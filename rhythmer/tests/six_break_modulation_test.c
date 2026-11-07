/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <math.h>
static uint8_t source[72000];
static int bad;
#define C(s,x) do{bad+=check(s,(x));}while(0)
static track_t *reset(void){
 ui_power_on();fm6_init();power_on_parts();six_loops[0]=six_loops[1]=source;six_loop_mask=3;six_loop_now=0;six_native_init();trks_put(1);song.sel=5;song.playing=1;
 track_t *t=TSEL;t->engine=t->eng_req=ENGI_BREAK;t->xf=t->xf_on=0;
 memset(t->v,0,sizeof t->v);t->m_env=0;t->p[P_MUTE]=0;
 t->p[P_ATK]=t->p[P_DEC]=t->p[P_REL]=0;t->p[P_SUS]=127;
 t->p[P_FCUT]=127;t->p[P_FRES]=t->p[P_FTYPE]=0;
 t->p[P_ED_FLT]=t->p[P_ED_PIT]=t->p[P_LD_FLT]=t->p[P_LD_PIT]=t->p[P_LD_AMP]=t->p[P_ELOOP]=0;
 t->p[P_LPHASE]=t->p[P_L2PHASE]=-1;t->p[P_LFADE]=t->p[P_L2FADE]=0;
 t->lfo_ph=t->lfo2_ph=0;t->lfo_val=t->lfo2_val=0;t->lfo_fade=t->lfo2_fade=32767;
 for(unsigned i=0;i<4;i++)t->p[P_M1SRC+3*i]=t->p[P_M1DST+3*i]=t->p[P_M1AMT+3*i]=0;
 memset(tf_ic1,0,sizeof tf_ic1);memset(tf_ic2,0,sizeof tf_ic2);memset(tf_on,0,sizeof tf_on);memset(tf_xb,0,sizeof tf_xb);memset(tf_cmb,0,sizeof tf_cmb);memset(tf_ft,0,sizeof tf_ft);
 jungle_bind(source,sizeof source,48000);jungle_sequence_mode(1);jungle_play(1);jungle_tick_mask(0);
 return t;
}
static double render(track_t *t,unsigned blocks,uint32_t *hash){
 double e=0;uint32_t h=2166136261u;int32_t out[CTL];
 for(unsigned b=0;b<blocks;b++){mod_begin(t);track_render(t,out,CTL);if(MOD.on)mod_end(t);
  for(unsigned j=0;j<CTL;j++){e+=(double)out[j]*out[j];h=(h^(uint32_t)out[j])*16777619u;}}
 if(hash)*hash=h;return e;
}
static void fixture(const char *dir,const char *kind){
 char p[1024];snprintf(p,sizeof p,"%s/break-mod-%s.project",dir,kind);project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))exit(20);FILE *f=fopen(p,"wb");if(!f)exit(21);fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);
}
int main(int argc,char **argv){
 for(unsigned i=0;i<48000;i+=2){int a=(int)(1100*sin(i*6.283185307*500/16000)+600*sin(i*6.283185307*5000/16000));int b=(int)(1100*sin((i+1)*6.283185307*500/16000)+600*sin((i+1)*6.283185307*5000/16000));unsigned x=(unsigned)a&4095u,y=(unsigned)b&4095u,j=i/2*3;source[j]=(uint8_t)x;source[j+1]=(uint8_t)((x>>8)|(y<<4));source[j+2]=(uint8_t)(y>>4);}
 track_t *t=reset();seq_note_on(t,0,127);uint32_t dryhash,hash;double dry=render(t,512,&dryhash);C("Break produces real PCM through the control envelope",dry>1000000);
 C("Break envelope does not allocate a shared synth voice",!t->v[0].active&&t->v[0].gate&&t->m_env>32000);
 t=reset();t->p[P_FCUT]=35;seq_note_on(t,0,127);double lp=render(t,512,&hash);C("EDIT low-pass cutoff changes and attenuates actual Break PCM",hash!=dryhash&&lp<dry*.35);
 t=reset();t->p[P_FTYPE]=FT_HP;t->p[P_FCUT]=80;seq_note_on(t,0,127);double hp=render(t,512,&hash);C("EDIT high-pass type is in the Break audio path",hash!=dryhash&&hp>1000&&hp<dry);
 t=reset();t->p[P_FTYPE]=FT_BP;t->p[P_FCUT]=70;t->p[P_FRES]=50;seq_note_on(t,0,127);render(t,512,&hash);C("Band-pass and resonance alter actual Break PCM",hash!=dryhash);
 t=reset();t->p[P_FCUT]=60;seq_note_on(t,0,127);render(t,512,&dryhash);
 t=reset();t->p[P_FCUT]=60;t->p[P_LD_FLT]=63;t->p[P_LRATE]=127;seq_note_on(t,0,127);render(t,512,&hash);C("LFO 1 direct FILTER destination alters real audio",hash!=dryhash);
 t=reset();t->p[P_FCUT]=60;t->p[P_M1SRC]=MS_LFO2;t->p[P_M1DST]=MD_CUT;t->p[P_M1AMT]=63;t->p[P_L2RATE]=115;seq_note_on(t,0,127);render(t,512,&hash);C("LFO 2 matrix CUT destination alters real audio",hash!=dryhash&&t->v[0].s[BENV_CUT]!=0);
 t=reset();t->p[P_FCUT]=35;t->p[P_ED_FLT]=63;seq_note_on(t,0,127);double fe=render(t,512,0);C("ENV FILTER destination opens the Break filter",fe>lp*1.5);
 t=reset();seq_note_on(t,0,127);double fast=render(t,64,0);
 t=reset();t->p[P_ATK]=100;seq_note_on(t,0,127);C("ENV attack controls actual Break amplitude",render(t,64,0)<fast*.1);
 t=reset();t->p[P_DEC]=15;t->p[P_SUS]=0;seq_note_on(t,0,127);render(t,512,0);C("ENV decay and zero sustain quiet the Break",render(t,128,0)<fast*.01);
 t=reset();t->p[P_REL]=30;seq_note_on(t,0,127);render(t,20,0);seq_note_off(t,0);C("SEQ gate starts ENV release and retains source for its tail",!t->v[0].gate&&t->v[0].stage==3&&t->v[0].s[BENV_OFF]==1);C("ENV release has an audible tail",render(t,10,0)>1000);render(t,12000,0);C("Release completes without a stuck source/envelope",!t->v[0].stage&&!t->v[0].s[BENV_OFF]);
 t=reset();seq_note_on(t,0,127);render(t,32,0);int32_t level=t->v[0].env;seq_note_on(t,33,127);C("A/B source-only event preserves the held envelope",t->v[0].env==level&&t->v[0].stage==2&&t->v[0].gate);
 key_on(0,t);key_off(0,t);C("Releasing a live chop keeps an underlying sequence gate",t->v[0].gate&&t->v[0].s[BENV_SEQ]&&!t->v[0].s[BENV_KEYS]);
 seq_note_off(t,0);C("Sequence release after live playing closes the envelope gate",!t->v[0].gate);
 t=reset();t->p[P_M1SRC]=MS_LFO2;t->p[P_M1DST]=MD_AMP;t->p[P_M1AMT]=63;t->p[P_L2RATE]=115;seq_note_on(t,0,127);double trem=render(t,512,0);C("LFO 2 AMP modulation attenuates real Break output",trem<dry*.9);
 t=reset();seq_note_on(t,0,127);render(t,32,&dryhash);
 t=reset();t->p[P_M1SRC]=MS_ENV;t->p[P_M1DST]=MD_E1+3;t->p[P_M1AMT]=40;seq_note_on(t,0,127);render(t,32,&hash);C("ENV matrix changes Amenity FILTER audio without rewriting its knob",hash!=dryhash&&t->p[P_E3]==64&&t->m_env>0);
 power_on_parts();t=&trk[5];
 unsigned empty=t->p[P_E4]==0&&t->p[P_LN0]==1&&t->p[P_SLEN]==32;
 for(unsigned i=0;i<NSTEP;i++)empty&=t->step[i].time!=ST_NOTE&&!t->step[i].n;
 C("Fresh firmware starts Break A with an empty two-bar page",empty);
 t->p[P_E4]=1;t->step[0].time=ST_NOTE;t->step[0].n=1;t->step[0].vel=100;t->step[0].note[0]=7;project_capture(&proj_scratch);t->p[P_E4]=0;t->step[0].note[0]=2;
 C("Loading a saved Break B and edited chop does not apply factory defaults",!project_restore_runtime(&proj_scratch)&&trk[5].p[P_E4]==1&&trk[5].step[0].note[0]==7);
 system_init();empty=trk[5].p[P_E4]==0&&trk[5].p[P_SLEN]==32&&trk[5].p[P_LN0]==1;
 for(unsigned k=0;k<NTRK;k++)for(unsigned i=0;i<NSTEP;i++)empty&=trk[k].step[i].time!=ST_NOTE&&!trk[k].step[i].n;
 C("NEW PROJECT restores Amen with all sequences empty",empty);
 if(argc>1){
  for(unsigned kind=0;kind<5;kind++){t=reset();song.playing=0;for(unsigned k=0;k<5;k++)trk[k].p[P_MUTE]=1;t->p[P_LN0]=1;t->p[P_SLEN]=32;t->p[P_SGATE]=127;bs_straight(t,32);
   if(kind==1)t->p[P_FCUT]=35;
   if(kind==2){t->p[P_FCUT]=60;t->p[P_LD_FLT]=63;t->p[P_LRATE]=127;}
   if(kind==3){t->p[P_FCUT]=60;t->p[P_M1SRC]=MS_LFO2;t->p[P_M1DST]=MD_CUT;t->p[P_M1AMT]=63;t->p[P_L2RATE]=115;}
   if(kind==4){t->p[P_ATK]=40;t->p[P_DEC]=30;t->p[P_SUS]=40;t->p[P_REL]=35;t->p[P_ED_FLT]=55;t->p[P_FCUT]=40;}
   fixture(argv[1],(const char*[]){"dry","lp","lfo1","lfo2","env"}[kind]);}
 }
 if(argc>2){
  FILE *f=fopen(argv[2],"rb");if(!f||fread(&proj_wire,1,sizeof proj_wire,f)!=sizeof proj_wire)exit(25);fclose(f);
  if(!proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)||project_restore_runtime(&proj_scratch))exit(26);
  t=&trk[5];song.playing=0;for(unsigned k=0;k<6;k++)trk[k].p[P_MUTE]=0;
  t->p[P_E4]=0;t->p[P_FCUT]=60;t->p[P_FRES]=70;t->p[P_LD_FLT]=40;t->p[P_LRATE]=110;
  t->p[P_M1SRC]=MS_LFO2;t->p[P_M1DST]=MD_AMP;t->p[P_M1AMT]=30;t->p[P_L2RATE]=90;
  t->p[P_ATK]=1;t->p[P_DEC]=15;t->p[P_SUS]=110;t->p[P_REL]=30;
  fixture(argv[1],"ensemble");t->p[P_FTYPE]=FT_COMB;t->p[P_FRES]=90;fixture(argv[1],"comb-ensemble");
 }
 printf("Break filter, ENV and LFO audio routing: %s\n",bad?"FAIL":"PASS");return bad?1:0;
}
