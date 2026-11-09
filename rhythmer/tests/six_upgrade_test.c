/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static void reset(void){ui_power_on();load_depth++;fm6_init();power_on_parts();test_optional_machines();load_depth--;six_native_init();trks_put(1);}
static void save_project(const char *dir,const char *name){project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))exit(10);char p[1024];snprintf(p,sizeof p,"%s/upgrade-%s.project",dir,name);FILE *f=fopen(p,"wb");if(!f)exit(11);fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);}
static void screen(const char *dir,const char *name){char p[1024];snprintf(p,sizeof p,"%s/%s.ppm",dir,name);FILE *f=fopen(p,"wb");if(!f)exit(9);fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<240*240;i++){unsigned x=swap16(host_screen[i]);unsigned char c[3]={(x>>11)*255/31,((x>>5)&63)*255/63,(x&31)*255/31};fwrite(c,1,3,f);}fclose(f);}
int main(int argc,char **argv){int bad=0;reset();
 bad+=check("59 genres and one complete menu permutation",NGENRE==59&&sizeof GEN_ORDER==NGENRE);
 unsigned seen[59]={0};for(unsigned i=0;i<NGENRE;i++){if(GEN_ORDER[i]>=NGENRE)return 12;seen[GEN_ORDER[i]]++;gp.genre=GEN_ORDER[i]+1;bad+=check("menu positions preserve stable numeric genre IDs",gen_menu_pos()==i+2);}
 for(unsigned i=0;i<NGENRE;i++)bad+=check("each genre remains selectable exactly once",seen[i]==1);

 unsigned notes=0;for(unsigned k=0;k<6;k++)for(unsigned i=0;i<NSTEP;i++)notes+=trk[k].step[i].time==ST_NOTE&&trk[k].step[i].n;
 bad+=check("fresh power-on has no hidden template notes on any track",notes==0&&trk[5].p[P_E4]==0&&trk[5].p[P_LN0]==1);
 /* Every Acid preset passes through the real track gate, not an unrelated oscillator. */
 for(unsigned p=0;p<ENG_ACID.npresets;p++){
  reset();track_t *t=&trk[4];apply_preset_to(t,p);t->engine=ENGI_ACID;t->xf=t->xf_on=0;t->p[P_MUTE]=0;
  voice_t v={0};v.pitch16=36*16;v.vel=127;v.gate=1;t->slide_glide=0;acid_on(t,&v);
  vmod_t m={0};m.amp0=m.amp1=32767;long long energy=0;int peak=0;
  for(unsigned b=0;b<12000;b++){
   if(b==200){v.pitch16=43*16;t->slide_glide=1;acid_on(t,&v);}
   if(b==600)v.gate=0;
   int32_t out[32]={0};acid_render(t,&v,out,32,&m);
   for(unsigned j=0;j<32;j++){int a=abs(out[j]);if(a>peak)peak=a;energy+=(long long)out[j]*out[j];}
  }
  printf("Acid %s peak=%d energy=%lld state=%zu/%zu\n",ENG_ACID.presets[p].name,peak,energy,sizeof(ha_voice_t),sizeof(ha_voice_t));
  bad+=check("Acid preset sounds, stays bounded, and releases",energy>100000&&peak<262144&&acid_done(t,&v));
  if(argc>1){for(unsigned i=0;i<16;i++){step_t *st=&t->step[i];step_clear(st);st->time=i%2?ST_TIE:ST_NOTE;if(!(i%2)){st->n=1;st->vel=i%4?100:127;st->note[0]=36+(i%3)*3;}}t->p[P_SLEN]=16;char label[24];snprintf(label,sizeof label,"acid-%u",p);save_project(argv[1],label);}
  project_capture(&proj_scratch);bad+=check("Acid model survives project codec",proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire)&&proj_scratch.t[4].p[P_E5]==t->p[P_E5]);
 }
 /* Exhaust the fixed-point ladder's control corners and registers under ASan/UBSan. */
 for(unsigned corner=0;corner<32;corner++)for(unsigned note=12;note<=108;note+=24){
  ha_track_t c={0};ha_voice_t v={0};c.pitch16=note*16;c.glide=Q24(.03);v.vel=127;v.active=1;
  for(unsigned i=1;i<6;i++)c.p[i]=(corner>>(i-1))&1?127:0;c.p[6]=corner&1;
  ha_trigger(&c,&v);c.wow=QONE;
  for(unsigned b=0;b<220;b++){int32_t out[32]={0};ha_render(&c,&v,out,32);for(unsigned j=0;j<32;j++)if(abs(out[j])>262144){puts("ladder runaway");return 5;}}
 }
 bad+=check("all fixed-point Acid control corners bounded",1);
 /* All eighteen refined genre profiles: stable four-track arrangements with chords and groove. */
 static const char *names[]={"R&B","NEO SOUL","JAZZY CHORDS","BOOM BAP","JUNGLE","LIQUID DNB","UK GARAGE","FUTURE BASS","BOUNCE HOUSE","90S HOUSE","G-FUNK","NOCTURNAL HH","TRIP HOP","LOFI HOUSE","ABSTRACT IDM","AMBIENT IDM","ATMOS BREAKS","DRUNKEN HH"};
 for(unsigned g=0;g<sizeof(names)/sizeof(names[0]);g++){
  unsigned gi=0;while(gi<NGENRE&&strcmp(GENRE[gi].name,names[g]))gi++;if(gi==NGENRE)return 6;
  unsigned diverse=0,patterns[32]={0},chords=0,micro=0,engines[3]={0},hooks=0,arp=0;
  for(unsigned seed=0;seed<32;seed++){
   reset();gp.form=0;gp.seed=seed*197+31;gp.genre=gi+1;gp.energy=55;gp.complex=65;gp.human=70;gp.swing=GA_AUTO;
   for(unsigned j=0;j<GP_N;j++)gp.amt[j]=GA_AUTO;
   gp.sig=0;ui.home=1;ui.confirm=CF_DICE_ALL;gen_go();
   unsigned sig=steps_sig(&trk[0]),prior=0;for(unsigned u=0;u<seed;u++)prior|=patterns[u]==sig;diverse+=!prior;patterns[seed]=sig;unsigned wavetable=0,localchords=0;
   for(unsigned k=0;k<4;k++){
    if(k)bad+=check("genre keeps safe automatic engine",gen_auto_synth_ok(trk[k].eng_req));
    wavetable+=trk[k].eng_req==ENGI_WAVETABLE;if(k)engines[k-1]|=1u<<trk[k].eng_req;hooks+=k&&gs.role[k]==RL_LEAD;arp+=k&&gs.role[k]==RL_ARP;
    for(unsigned i=0;i<dlen(&trk[k]);i++)if(trk[k].step[i].time==ST_NOTE){
     step_t *st=&trk[k].step[i];localchords+=k&&st->n>=3;micro+=ST_MICRO(st)!=0;
     if(st->n>4)return 7;
    }
   }
   chords+=localchords>0;
   if(gi==50||gi==54)for(unsigned i=0;i<dlen(&trk[0]);i+=4)bad+=check("house retains four-on-the-floor anchors",trk[0].step[i].hit&1);
   if(gi==58){int pos=0,neg=0;for(unsigned i=0;i<dlen(&trk[0]);i++){step_t *st=&trk[0].step[i];if(st->time==ST_NOTE){pos+=ST_MICRO(st)>0;neg+=ST_MICRO(st)<0;}}bad+=check("drunken drums contain deliberate push and pull",pos>0&&neg>0&&GENRE[gi].swing==0);}
   if(gi==52)for(unsigned i=0;i<dlen(&trk[0]);i+=16)bad+=check("nocturnal half-time backbeat and space",(trk[0].step[i+8].hit&2)&&!(trk[0].step[i+4].hit&6)&&!(trk[0].step[i+12].hit&6));
   if(gi==51&&gs.role[3]==RL_LEAD)bad+=check("G-funk hook uses legato glide",trk[3].p[P_VOICE]==V_LEGATO&&trk[3].p[P_GLIDE]==18);
   if(gi==55&&gs.role[3]==RL_ARP)bad+=check("abstract arp has a fifteen-step cycle",trk[3].p[P_SLEN]==15);

   bad+=check("genre keeps the Wavetable ceiling and Acid/Break outside GEN",wavetable<=1&&trk[4].eng_req==ENGI_ACID&&trk[5].eng_req==ENGI_BREAK);
   if(argc>1&&seed==0){song.sel=0;ui.home=1;ui.confirm=CF_DICE_ALL;gp.page=0;fm1_ms+=500;ui.force=1;frame();char label[40];snprintf(label,sizeof label,"GEN-%s",names[g]);screen(argv[1],label);go_home();ui.force=1;frame();screen(argv[1],names[g]);save_project(argv[1],names[g]);}
  }
  printf("GEN %s diversity=%u/32 chord-arrangements=%u/32 timed-hits=%u engines=%x,%x,%x hooks=%u arps=%u\n",names[g],diverse,chords,micro,engines[0],engines[1],engines[2],hooks,arp);
  for(unsigned k=0;k<3;k++)bad+=check("role guidance retains machine variety",__builtin_popcount(engines[k])>=3);
  if(gi==51)bad+=check("G-funk makes a lead hook in most arrangements",hooks>=8);
  bad+=check("genre produces varied, voiced, timed phrases",diverse>=28&&chords>=28&&micro>0);
 }
 /* Repeating the same settings regenerates the same pattern; untouched
  * machine tracks retain music and own the automatic Wavetable allowance. */
 for(unsigned gi=49;gi<NGENRE;gi++){
  unsigned hashes[2][4];
  for(unsigned pass=0;pass<2;pass++){
   reset();gp.form=0;gp.seed=197;gp.genre=gi+1;gp.energy=55;gp.complex=65;gp.human=70;gp.swing=GA_AUTO;for(unsigned j=0;j<GP_N;j++)gp.amt[j]=GA_AUTO;
   for(unsigned k=4;k<6;k++){step_t *st=&trk[k].step[3];st->time=ST_NOTE;st->n=1;st->vel=101;st->note[0]=k==4?41:7;}
   unsigned acid=steps_sig(&trk[4]),brk=steps_sig(&trk[5]);gen_go();
   bad+=check("GEN preserves Acid and Break sequence contents",acid==steps_sig(&trk[4])&&brk==steps_sig(&trk[5]));
   for(unsigned k=0;k<4;k++)hashes[pass][k]=steps_sig(&trk[k])^(trk[k].eng_req<<24)^(trk[k].preset<<16);
  }
  bad+=check("same seed and settings reproduce all four patterns and sounds",!memcmp(hashes[0],hashes[1],sizeof hashes[0]));
 }
 for(unsigned kit=9;kit<17;kit++)bad+=check("all conventional MD factory closed hats use the closed TRX family",md_base(dx_md_id(DX_KITS[kit].lane[3].mode))==10&&DX_KITS[kit].lane[3].a[2]<=16);
 if(argc>1)for(unsigned kit=14;kit<DX_NKIT;kit++){
  reset();dx_kit_load(kit);track_t *t=&trk[0];apply_preset_to(t,kit);
  for(unsigned i=0;i<16;i++){step_t *st=&t->step[i];step_clear(st);st->time=ST_NOTE;st->hit=(i%4==0?1:0)|(i%8==4?2:0)|8;st->vel=110;}
  char name[24];snprintf(name,sizeof name,"md-%u",kit);save_project(argv[1],name);
 }
 printf("NEXT UPGRADE: %s (%d)\n",bad?"FAIL":"PASS",bad);return !!bad;
}
