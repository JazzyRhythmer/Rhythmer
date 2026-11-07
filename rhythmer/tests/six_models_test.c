/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void){
 int bad=0;ui_power_on();load_depth++;fm6_init();power_on_parts();load_depth--;six_native_init();trks_put(3);
 char layout_path[1024];snprintf(layout_path,sizeof layout_path,"%s/six-model-layout.json",getenv("RHYTHMER_BUILD_DIR")?getenv("RHYTHMER_BUILD_DIR"):"rhythmer/build");
 FILE *layout=fopen(layout_path,"w");if(!layout)return 2;
 fprintf(layout,"{\"song_sel\":%u,\"track_stride\":%u,\"engine\":%u,\"p\":%u,\"step\":%u,\"step_stride\":%u,\"step_note\":%u,\"voices\":%u,\"voice_stride\":%u,\"voice_note\":%u,\"voice_active\":%u,\"voice_stage\":%u,\"voice_age\":%u}\n",(unsigned)offsetof(song_t,sel),(unsigned)sizeof(track_t),(unsigned)offsetof(track_t,engine),(unsigned)offsetof(track_t,p),(unsigned)offsetof(track_t,step),(unsigned)sizeof(step_t),(unsigned)offsetof(step_t,note),(unsigned)offsetof(track_t,v),(unsigned)sizeof(voice_t),(unsigned)offsetof(voice_t,note),(unsigned)offsetof(voice_t,active),(unsigned)offsetof(voice_t,stage),(unsigned)offsetof(voice_t,age));fclose(layout);
 const unsigned model[6]={ENGI_SERUM,ENGI_PHASE,ENGI_LOFI,ENGI_M2STAT,ENGI_M2SAW,ENGI_M2SID},count[6]={8,16,17,5,6,6};
 track_t *t=&trk[1];song.sel=1;
 for(unsigned e=0;e<6;e++){
  const engine_t *eng=ENGINES[model[e]];
  bad+=check("complete Garden and adapted MONO2K preset count",eng->npresets==count[e]);
  for(unsigned p=0;p<eng->npresets;p++){
   init_sound_of(t,model[e]);apply_preset_to(t,p);t->engine=t->eng_req;t->xf_on=0;
   voice_t v={0};vmod_t m={0};int32_t out[32];long long energy=0;int peak=0;
   v.note=60;v.vel=100;v.pitch16=60*16;v.active=v.gate=1;
   m.inc=pitch_inc(60*16);m.pitch16=60*16;m.amp0=m.amp1=32767;m.shape=64<<8;
   eng->note_on(t,&v);
   for(unsigned block=0;block<44100/32;block++){
    memset(out,0,sizeof out);eng->render(t,&v,out,32,&m);
    for(unsigned i=0;i<32;i++){int a=out[i]<0?-out[i]:out[i];energy+=a;if(a>peak)peak=a;}
   }
   printf("model: %s / %s peak=%d\n",eng->name,eng->presets[p].name,peak);
   bad+=check("every selected model preset renders bounded non-silent audio",energy>0&&peak<3*32767);
   project_capture(&proj_scratch);int ok=proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire);
   ok&=proj_scratch.t[1].engine==model[e];
   for(unsigned i=0;i<8;i++)ok&=proj_scratch.t[1].p[P_E0+i]==t->p[P_E0+i];
   bad+=check("every selected model preset keeps engine and parameters in project",ok);
  }
 }
 for(unsigned e=3;e<6;e++)for(unsigned k=0;k<8;k++)for(unsigned edge=0;edge<2;edge++){
  init_sound_of(t,model[e]);apply_preset_to(t,0);t->engine=t->eng_req;t->xf_on=0;t->p[P_E0+k]=edge?127:0;
  voice_t v={0};vmod_t m={0};v.note=60;v.vel=100;v.pitch16=960;v.active=v.gate=1;m.inc=pitch_inc(960);m.pitch16=960;m.amp0=m.amp1=32767;m.shape=64<<8;
  int bounded=1;ENGINES[model[e]]->note_on(t,&v);for(unsigned j=0;j<64;j++){int32_t out[32]={0};ENGINES[model[e]]->render(t,&v,out,32,&m);for(unsigned i=0;i<32;i++)bounded&=out[i]>-65536&&out[i]<65536;}bad+=check("MONO2K control edge bounded",bounded);
 }
 printf("Garden + MONO2K model presets: %s (%d failures)\n",bad?"FAIL":"PASS",bad);return bad?1:0;
}
