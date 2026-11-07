/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#ifndef ACID_REFERENCE
#define ACID_REFERENCE "fixtures/hortator_acid_reference.c"
#endif
#include ACID_REFERENCE
int main(int argc,char**argv){int bad=0;
 ui_power_on();fm6_init();power_on_parts();test_optional_machines();six_native_init();trks_put(1);
 bad+=check("Acid has exactly four native Hortator presets and two waves",ENG_ACID.npresets==4&&ENG_ACID.edit[5].max==1&&!ENG_ACID.block);
 unsigned blocks=0;
 for(unsigned corner=0;corner<32;corner++)for(unsigned note=12;note<=108;note+=24){
  ha_track_t c={0};href_track_t ref={0};ha_voice_t v={0};href_voice_t rv={0};
  c.pitch16=note*16;c.glide=Q24(.03);v.vel=rv.vel=127;v.active=rv.active=1;
  for(unsigned i=1;i<6;i++)c.p[i]=(corner>>(i-1))&1?127:0;c.p[6]=corner&1;memcpy(&ref,&c,sizeof c);
  ha_trigger(&c,&v);href_trigger(&ref,&rv);
  for(unsigned b=0;b<300;b++){
   if(b==70){c.pitch16+=7*16;ref.pitch16=c.pitch16;ha_glide(&c,&v);href_glide(&ref,&rv);}
   if(b==180){ha_gate(&c,&v,0);href_gate(&ref,&rv,0);}
   unsigned n=(b%31)?32:(unsigned[]){1,8,16,32}[b%4];int32_t a[32]={0},r[32]={0};
   ha_render(&c,&v,a,n);href_render(&ref,&rv,r,n);if(memcmp(a,r,n*sizeof *a)){printf("PCM mismatch %u %u %u\n",corner,note,b);return 10;}blocks++;
  }
 }
 printf("Hortator exact PCM blocks %u\n",blocks);
 bad+=check("cached pitch/decay preserve saw-square, extremes, slide, accent and release PCM",blocks==48000);
 const char *legacy=argc>1?argv[1]:"rhythmer/tests/fixtures";
 for(unsigned old=0;old<5;old++){
  char p[1024];snprintf(p,sizeof p,"%s/upgrade-acid-%u.project",legacy,old);FILE*f=fopen(p,"rb");if(!f||fread(&proj_wire,1,sizeof proj_wire,f)!=sizeof proj_wire)return 11;fclose(f);
  if(!proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)||project_restore_runtime(&proj_scratch))return 12;
  track_t*t=&trk[4];bad+=check("old Acid projects keep engine, colour, waveform and MIDI notes",t->eng_req==ENGI_ACID&&t->preset==(old?old-1:0)&&t->p[P_E5]==(old==4)&&t->step[0].note[0]==36&&t->step[0].n==1&&t->step[1].time==ST_TIE);
  project_capture(&proj_scratch);bad+=check("migrated Acid project round trips in the unchanged storage format",proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)&&proj_scratch.t[4].preset==(old?old-1:0));
 }
 track_t*t=&trk[1];set_engine_of(t,ENGI_ACID);song.sel=1;song.playing=1;bar_n=1;t->p[P_LN7]=0;
 project_capture(&proj_scratch);proj_scratch.t[1].engine=ENGI_ACID;proj_scratch.t[1].preset=0;proj_scratch.t[1].p[P_E5]=1;proj_scratch.sum=proj_sum(&proj_scratch);
 bad+=check("legacy X0X square reopens as native Hortator square",proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)&&!project_restore_runtime(&proj_scratch)&&trk[1].p[P_E5]==1&&trk[1].eng_req==ENGI_ACID);
 uint8_t foreign[3840];memset(foreign,0x58,sizeof foreign);memcpy(foreign,"XPJ1",4);
 bad+=check("foreign standalone X0X/invalid project is rejected safely",!proj_import(&proj_scratch,foreign,sizeof foreign));
 song.playing=1;bar_n=1;t->p[P_LN7]=0;
 voice_t held={.vel=127,.gate=1,.pitch16=36*16};acid_voice[1].t=FS*6u;acid_on(t,&held);
 bad+=check("every native Acid hit starts a fresh lifetime",acid_voice[1].t==0&&acid_voice[1].active);
 acid_voice[1].active=0;bad+=check("native lifetime completion releases its shared voice even if MIDI is held",acid_done(t,&held));
 unsigned pattern=steps_sig(t),seed=six_gen_seed[1];for(unsigned i=0;i<10000;i++)six_gen_poll();
 for(bar_n=2;bar_n<100;bar_n++)six_gen_poll();
 bad+=check("AUTO off never generates or changes notes during playback",steps_sig(t)==pattern&&six_gen_seed[1]==seed);
 t->p[P_LN7]=4;bar_n=100;six_gen_poll();unsigned next=six_gen_seed[1];for(unsigned i=0;i<1000;i++)six_gen_poll();
 bad+=check("explicit AUTO runs once at a requested bar boundary",six_gen_seed[1]==next);
 printf("Native Acid synth/generator separation: %s\n",bad?"FAIL":"PASS");return !!bad;
}
