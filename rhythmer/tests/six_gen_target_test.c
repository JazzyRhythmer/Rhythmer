/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static track_t baseline[NTRK];
static void init(void){ui_power_on();load_depth++;fm6_init();power_on_parts();load_depth--;six_native_init();trks_put(1);gp.target=0;gp.form=0;gp.energy=55;gp.complex=65;gp.human=70;gp.swing=GA_AUTO;for(unsigned j=0;j<GP_N;j++)gp.amt[j]=GA_AUTO;
 song.g[G_BPM]=87;song.g[G_SWING]=13;song.g[G_RTYPE]=2;song.g[G_DTYPE]=1;
 for(unsigned k=0;k<NTRK;k++){track_t *t=&trk[k];t->p[P_ROOT]=2;t->p[P_SCALE]=SC_DOR;t->p[P_SLEN]=32;t->p[P_SDIV]=2;t->p[P_LEVEL]=81+k;t->p[P_DLY]=k*7;for(unsigned i=0;i<NSTEP;i++)gclear(&t->step[i]);
 for(unsigned i=0;i<32;i+=4){step_t *s=&t->step[i];s->time=ST_NOTE;s->vel=90;if(!k){s->hit=i%8?2:1;}else{s->n=k==2?3:1;s->note[0]=(k==1||k==4)?38:k==5?i%16:62;if(k==2){s->note[0]=i<16?50:55;s->note[1]=i<16?53:59;s->note[2]=i<16?57:62;}}}}
 motion_put(&trk[1],3,P_FCUT,78,0);motion_put(&trk[3],7,P_LRATE,32,MOTION_LOCK);memcpy(baseline,trk,sizeof trk);
}
static void capture(const char *dir,const char *name){project_capture(&proj_scratch);if(!proj_pack(&proj_wire,&proj_scratch))exit(4);char p[1024];snprintf(p,sizeof p,"%s/upgrade-target-%s.project",dir,name);FILE *f=fopen(p,"wb");if(!f)exit(5);fwrite(&proj_wire,1,sizeof proj_wire,f);fclose(f);}
int main(int argc,char **argv){int bad=0;init();uint32_t song_sig=fnv(1,&song,sizeof song),mo_sig=fnv(1,&motion,sizeof motion);unsigned changed=0,notes=0;
 for(unsigned genre=0;genre<NGENRE;genre++)for(unsigned k=0;k<NTRK;k++){
  memcpy(trk,baseline,sizeof trk);gp.target=k+1;gp.genre=genre+1;gp.seed=31;ui.home=1;ui.confirm=CF_DICE_ALL;gp.page=0;gen_go();
  bad+=check("target GEN preserves every song/global setting and transport",song_sig==fnv(1,&song,sizeof song));
  bad+=check("target GEN preserves automation records",mo_sig==fnv(1,&motion,sizeof motion));
  for(unsigned j=0;j<NTRK;j++)bad+=check("only the selected track's MIDI steps can change",j==k?!memcmp(trk[j].p,baseline[j].p,sizeof trk[j].p):!memcmp(&trk[j],&baseline[j],sizeof trk[j]));
  changed+=steps_sig(&trk[k])!=steps_sig(&baseline[k]);
  for(unsigned i=0;i<32;i++){step_t *s=&trk[k].step[i];if(s->time==ST_NOTE){notes++;if(k==5){bad+=check("Break target produces valid chop events",s->n>=1&&s->n<=2&&s->note[0]<16&&!s->hit&&(s->n==1||(s->note[1]>=16&&s->note[1]<32)));}else if(k)for(unsigned n=0;n<s->n;n++)bad+=check("MIDI notes obey the existing SCL",(scale_mask(&trk[k])>>((s->note[n]+12-2)%12))&1u);}}
  if(argc>1&&genre==58&&k==1)capture(argv[1],"drunken-bass");
  if(argc>1&&genre==46&&k==5)capture(argv[1],"break-jungle");
 }
 printf("Target GEN: %u cases, changed=%u, note events=%u\n",(unsigned)NGENRE*NTRK,changed,notes);
 bad+=check("all targets receive new genre-driven MIDI",changed==NGENRE*NTRK);
 /* No stale last-genre cache: a fixed seed responds to the currently chosen genre. */
 unsigned sigs[4];static const unsigned genres[]={50,52,58,59};
 for(unsigned j=0;j<4;j++){memcpy(trk,baseline,sizeof trk);gp.target=1;gp.genre=genres[j];gp.seed=31;gen_go();sigs[j]=steps_sig(&trk[0]);}
 bad+=check("changing GEN genre changes only the requested MIDI style",sigs[0]!=sigs[1]&&sigs[1]!=sigs[2]&&sigs[2]!=sigs[3]);
 /* BREAK resolves the singleton owner after a move, without assigning an engine. */
 for(unsigned owner=0;owner<NTRK;owner++){
  memcpy(trk,baseline,sizeof trk);if(owner!=5){trk[5].engine=trk[5].eng_req=ENGI_ANALOG;trk[owner].engine=trk[owner].eng_req=ENGI_BREAK;}
  track_t old[NTRK];memcpy(old,trk,sizeof trk);gp.target=7;gp.genre=59;gp.seed=31;gen_go();
  bad+=check("BREAK target finds the current owner",gen_target_index()==owner&&steps_sig(&trk[owner])!=steps_sig(&old[owner]));
  for(unsigned j=0;j<NTRK;j++)if(j!=owner)bad+=check("BREAK routing leaves other tracks intact",!memcmp(&old[j],&trk[j],sizeof trk[j]));
 }
 memcpy(trk,baseline,sizeof trk);trk[5].engine=trk[5].eng_req=ENGI_ANALOG;track_t none[NTRK];memcpy(none,trk,sizeof trk);gp.target=7;gen_go();bad+=check("missing Break owner is a non-destructive no-op",!memcmp(none,trk,sizeof trk));
 memcpy(trk,baseline,sizeof trk);gp.target=7;gp.genre=47;gp.seed=31;gen_go();unsigned fastroll=0,fasthits=0;for(unsigned i=0;i<32;i++){fasthits+=trk[5].step[i].time==ST_NOTE;fastroll+=trk[5].step[i].n>1;}
 memcpy(trk,baseline,sizeof trk);gp.genre=59;gp.seed=31;gen_go();unsigned slowroll=0,slowhits=0,push=0,pull=0;for(unsigned i=0;i<32;i++){step_t *p=&trk[5].step[i];slowhits+=p->time==ST_NOTE;slowroll+=p->n>1;if(p->time==ST_NOTE){push+=ST_MICRO(p)<0;pull+=ST_MICRO(p)>0;}}
 bad+=check("jungle has stronger break activity and bounded answer rolls",fastroll>slowroll&&fasthits>slowhits&&fastroll<=8);
 printf("Break metrics fast=%u/%u slow=%u/%u push=%u pull=%u\n",fasthits,fastroll,slowhits,slowroll,push,pull);
 bad+=check("Dilla break groove has push/pull, dynamics and space",push>0&&pull>0&&slowhits<24&&slowroll==0);
 if(argc>1)capture(argv[1],"drunken-break");
 song.g[G_BPM]=160;memcpy(trk,baseline,sizeof trk);gp.target=7;gp.genre=59;gp.seed=31;gen_go();unsigned halfnotes=0;for(unsigned i=0;i<32;i++){halfnotes+=trk[5].step[i].time==ST_NOTE;bad+=check("fast-tempo Dilla break uses a half-time event grid",!(i&1)||trk[5].step[i].time!=ST_NOTE);}bad+=check("half-time keeps the host tempo and leaves musical space",song.g[G_BPM]==160&&halfnotes<16);if(argc>1)capture(argv[1],"drunken-half");song.g[G_BPM]=87;
 /* A fixed seed is reproducible, with no dependence on the previous GEN run. */
 memcpy(trk,baseline,sizeof trk);gp.target=2;gp.genre=59;gp.seed=77;gen_go();unsigned first=steps_sig(&trk[1]);
 memcpy(trk,baseline,sizeof trk);gp.seed=77;gen_go();bad+=check("partial GEN is reproducible",first==steps_sig(&trk[1]));
 for(unsigned i=0;i<32;i++){step_t *s=&trk[1].step[i];if(s->time==ST_NOTE){unsigned pc=s->note[0]%12;bad+=check("generated bass follows held chord tones",i<16?(pc==2||pc==5||pc==9):(pc==7||pc==11||pc==2));}}
 gp.target=7;gp.genre=59;gen_open();bad+=check("partial GEN keeps the chosen genre when reopened",gp.genre==59&&gp.target==7);
 /* ALL is always a four-track operation, even with a populated Acid and
  * Break, all six tracks enabled, and a multi-section arrangement. */
 for(unsigned form=0;form<3;form++)for(unsigned genre=0;genre<NGENRE;genre++){
  memcpy(trk,baseline,sizeof trk);gp.target=0;gp.form=form;gp.genre=genre+1;gp.seed=31;
  gen_go();
  for(unsigned k=SIX_GEN_TRACKS;k<NTRK;k++){
   if(!form&&!genre)for(unsigned p=0;p<P_COUNT;p++)if(trk[k].p[p]!=baseline[k].p[p])printf("ALL unexpected T%u parameter %u: %d -> %d\n",k+1,p,baseline[k].p[p],trk[k].p[p]);
   bad+=check("ALL leaves T5/T6 sound and sequence settings intact",!memcmp(trk[k].p,baseline[k].p,sizeof trk[k].p));
   bad+=check("ALL never writes MIDI into T5/T6",!memcmp(trk[k].step,baseline[k].step,sizeof trk[k].step));
   bad+=check("ALL never assigns a T5/T6 engine",trk[k].eng_req==baseline[k].eng_req);
  }
 }
 memcpy(trk,baseline,sizeof trk);gp.target=5;gp.form=0;gp.genre=59;gp.seed=31;gen_go();
 bad+=check("explicit T5 MIDI target still generates Acid notes",steps_sig(&trk[4])!=steps_sig(&baseline[4]));
 memcpy(trk,baseline,sizeof trk);gp.target=2;gp.genre=59;gen_go();int undo=undo_do();unsigned same=1;for(unsigned k=0;k<NTRK;k++)same&=steps_sig(&trk[k])==steps_sig(&baseline[k]);bad+=check("UNDO restores the original loops after partial GEN",!undo&&same);
 /* UI knob 4 exposes all forms and six independent targets without a new page. */
 gp.target=0;gp.form=0;gp.page=0;char value[16];for(unsigned i=0;i<8;i++)gen_knob(3,1);gen_value(0,3,value);bad+=check("GEN target knob reaches T6 MIDI",gp.target==6&&!strcmp(value,"T6 MIDI"));
 for(unsigned i=0;i<8;i++)gen_knob(3,-1);bad+=check("GEN target knob returns to ALL LOOP",!gp.target&&!gp.form);
 /* Preserve all target sound/parameter fields even at short/odd lengths. */
 for(unsigned len=1;len<=64;len++){memcpy(trk,baseline,sizeof trk);trk[1].p[P_SLEN]=len;gp.target=2;gp.genre=59;gp.seed=len;int16_t p[P_COUNT];memcpy(p,trk[1].p,sizeof p);gen_go();bad+=check("partial GEN keeps arbitrary loop length and sound",!memcmp(p,trk[1].p,sizeof p));}
 if(argc>1){memcpy(trk,baseline,sizeof trk);gp.target=2;gp.genre=59;gp.seed=31;ui.home=1;ui.confirm=CF_DICE_ALL;gen_go();fm1_ms+=500;ui.force=1;frame();char path[1024];snprintf(path,sizeof path,"%s/target-gen.ppm",argv[1]);FILE *f=fopen(path,"wb");fprintf(f,"P6\n240 240\n255\n");for(unsigned i=0;i<240*240;i++){unsigned x=swap16(host_screen[i]);unsigned char c[3]={(x>>11)*255/31,((x>>5)&63)*255/63,(x&31)*255/31};fwrite(c,1,3,f);}fclose(f);}
 printf("TARGET GEN: %s (%d)\n",bad?"FAIL":"PASS",bad);return !!bad;
}
