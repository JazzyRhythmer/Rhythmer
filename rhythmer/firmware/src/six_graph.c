/* SPDX-License-Identifier: GPL-3.0-only
 * SIX monochrome instrument surface. Integer geometry only, UI thread only.
 * Motion comes from sequencer position and parameter values, never a free-running creature.
 */
static void six_graph_lanes(unsigned count){
 unsigned h=graph_h(),rh=(h-16)/count;
 for(unsigned k=0;k<count;k++){
  const track_t *t=&trk[k];unsigned y=4+k*rh;char n[3]={(char)('1'+k),0};
  if(k==song.sel){cv_rect(4,y,12,12,T_TEXT);cv_text_in(4,y,12,&AF_X,n,T_BG,T_TEXT);}else cv_text_in(4,y,12,&AF_X,n,T_TEXT,T_BG);
  cv_text_in(20,y,43,&AF_X,six_track_enabled(k)?six_kind_label(trk[k].eng_req):"OFF",(!six_track_enabled(k)||t->p[P_MUTE])?T_DIM:T_TEXT,T_BG);
  for(unsigned j=0;j<16;j++){
   unsigned si=(t->seq_idx/16)*16+j;const step_t *s=&seq_steps(t)[si];int on=s->time!=ST_REST&&(s->n||s->hit||s->time==ST_TIE);
   int x=69+j*10;cv_rect(x,y+4,on?5:2,on?5:2,t->p[P_MUTE]?T_DIM:T_TEXT);
   if(song.playing&&six_track_enabled(k)&&!t->p[P_MUTE]&&si==t->seq_idx)cv_frame(x-2,y+1,9,11,T_TEXT);
  }
 }
 cv_text_in(5,h-12,230,&AF_X,count==4?"05 / 06 OFF / GLO TRACK MODE":"SELECT TRACK / PLAY / REC",T_TEXT,T_BG);
}
static void six_graph_vectors(const track_t *t){
 const page_t *p=cur_page();unsigned h=graph_h();
 for(unsigned x=12;x<232;x+=8)for(unsigned y=10;y<h-18;y+=8)cv_rect(x,y,1,1,T_DIM);
 /* Four distinct dot traces: each is determined by its corresponding knob. */
 for(unsigned k=0;k<4;k++){
  unsigned id=p->id[k],v=id<P_COUNT?(unsigned)(t->p[id]+64):64;
  if(p->scope==SC_GLOBAL)v=id<G_COUNT?(unsigned)(song.g[id]+64):64;
  if(!(ui_prefs&PREF_ANIM_OFF))v+=(p->scope==SC_GLOBAL?fm1_ms/160u:song.playing?t->seq_idx:0u)*(k+1);
  int px=0,py=0;
  for(unsigned j=0;j<22;j++){
   int x=14+j*10,y=14+(int)((j*(k+2)+v*(k+1))%(h>40?h-40:60));
   if(j&&k==(ui.hot_col&3))cv_line_t(px,py,x,y,T_TEXT,1);
   cv_rect(x,y,k==(ui.hot_col&3)?3:1,k==(ui.hot_col&3)?3:1,T_TEXT);px=x;py=y;
  }
 }
 cv_text_in(8,h-14,224,&AF_X,six_kind_label(trk[song.sel%6].eng_req),T_TEXT,T_BG);
}
static void six_graph_matrix(const track_t *t){
 cv_text_in(8,0,224,&AF_X,"MODULATION / 4 SLOTS",T_TEXT,T_BG);
 for(unsigned k=0;k<4;k++){
  int y=22+(int)k*28;unsigned src=t->p[P_M1SRC+3*k],dst=t->p[P_M1DST+3*k];
  char n[2]={(char)('1'+k),0},a[12];fmt_int(a,t->p[P_M1AMT+3*k]);
  cv_text_in(8,y,12,&AF_X,n,T_TEXT,T_BG);cv_text_in(24,y,55,&AF_X,N_MSRC[src<MS_N?src:0],T_TEXT,T_BG);
  cv_line_t(81,y+5,108,y+5,T_DIM,1);
  int value=src==MS_LFO?mulq15(t->lfo_val,t->lfo_fade):src==MS_LFO2?mulq15(t->lfo2_val,t->lfo2_fade):0;
  cv_rect(94+value/4096,y+3,3,5,T_TEXT);
  cv_text_in(114,y,68,&AF_X,N_MDST[dst<MD_N?dst:0],T_TEXT,T_BG);cv_text_r(232,y,&AF_X,a,T_TEXT,T_BG);
  if(k==mod_ui_slot)cv_frame(3,y-4,234,20,T_TEXT);
 }
}
/* JIANT's animation sites, with original geometric machine marks in place of organisms. */
static int six_note_punch(const track_t *t){
 unsigned k=(unsigned)(t-trk),at=six_vis_note_at[k];
 if(!at||(ui_prefs&PREF_ANIM_OFF))return 0;
 unsigned age=fm1_ms-(at-1u);if(age>=280u)return 0;
 return (int)((280u-age)*(48u+six_vis_note_vel[k]*79u/127u)/280u);
}
static int six_visual_level(const track_t *t){
 unsigned ti=(unsigned)(t-trk);if(!six_track_enabled(ti)||t->p[P_MUTE])return 0;
 int en=clamp(t->m_env/258,0,127);
 for(unsigned v=0;v<NVOICE;v++)if(t->v[v].active){int e=clamp(t->v[v].env>>17,0,127);if(e>en)en=e;}
 if(t->engine==ENGI_BREAK&&jungle_playing())en=64+((jungle_current_step()&1)?0:63);
 if(t->engine==ENGI_DRUM){const drum_lane_t *k=drum_kit_of(t);for(unsigned j=0;k&&j<8;j++){int e=clamp(k[j].x.ea>>23,0,127);if(k[j].x.live&&e>en)en=e;}}
 int punch=six_note_punch(t);if(punch>en)en=punch;
 return en;
}
static void six_dot_background(int h){
 for(int y=8;y<h-4;y+=12)for(int x=8;x<236;x+=12)cv_rect(x,y,1,1,T_DIM);
}
/* Shared geometry for the live instrument and startup. A NULL track uses a
 * bounded visual preview: startup precedes track/kit/sample initialization. */
static void six_machine_glyph(const track_t *t,unsigned engine,int en,unsigned ph,int x,int y,int w,int h,uint16_t ink){
 int amp=h*(10+en*30/127)/100,cy=y+h/2;
 if(engine==ENGI_DRUM){
  const drum_lane_t *k=t?drum_kit_of(t):0;for(unsigned j=0;j<8;j++){
   int e=t?(k&&k[j].x.live?clamp(k[j].x.ea>>23,0,127):0):en*(osc_sine(ph+j*0x39000000u)+32768)/65536;
   if(ui_prefs&PREF_ANIM_OFF)e=0;
   int a=3+(h-7)*e/127,xx=x+(int)j*w/8;cv_frame(xx,y+h-a,w/8-2,a,ink);
  }return;
 }
 if(engine==ENGI_M2STAT){
  int px=x+w*3/4,py=cy;for(unsigned j=0;j<2;j++){int xx=x+w/4,yy=y+h*(j?3:1)/4,r=1+en*(w<48?1:3)/127;cv_frame(xx-r,yy-r,2*r+1,2*r+1,ink);cv_line_t(xx+r,yy,px,py,ink,1);}cv_frame(px-2-en/64,py-2-en/64,5+en/32,5+en/32,ink);return;
 }
 if(engine==ENGI_FM6){
  const uint8_t *p=t?fm6_patch[(unsigned)(t-trk)]:0;unsigned alg=p?p[FP_ALG]&31u:0;int bus[3]={-1,-1,-1};
  for(unsigned j=0;j<6;j++){
   int xx=x+w/8+(int)(j%3)*w/3,yy=y+h/4+(int)(j/3)*h/2,r=2+en*((p?p[j*FP_OP+FP_OL]:80)+32)/4200;
   if(w<48)r=1+en/48; /* Compact HOME cells keep all six operators inside their channel. */
   unsigned f=FM6_ALG[alg][j],in=(f>>4)&3u,out=f&3u;
   if(in&&bus[in-1]>=0){int q=bus[in-1];cv_line_t(x+w/8+(q%3)*w/3,y+h/4+(q/3)*h/2,xx,yy,ink,1);}
   if(out)bus[out-1]=(int)j;else cv_line_t(xx,yy,xx,y+h-1,T_DIM,1);
   if(t&&(cur_page()->graph==GR_FMOP||cur_page()->graph==GR_FMOP2||cur_page()->graph==GR_FMENV)&&j==fop_op%6u)cv_rect(xx-r-2,yy-r-2,r*2+5,r*2+5,T_DIM);
   cv_frame(xx-r,yy-r,r*2+1,r*2+1,ink);cv_rect(xx,yy,1+en/64,1+en/64,ink);
  }return;
 }
 if(engine==ENGI_WHEEL){
  int cx=x+w/2;for(unsigned j=0;j<12;j++){uint32_t a=ph+j*(0xFFFFFFFFu/12u);int r=h/4+en*h/900;
   int dx=osc_sine(a)*r/32768,dy=osc_sine(a+0x40000000u)*r/32768;
   cv_line_t(cx+dx/3,cy+dy/3,cx+dx,cy+dy,ink,1);cv_rect(cx+dx-1,cy+dy-1,3,3,ink);
  }return;
 }
 if(engine==ENGI_BREAK){
  static const uint8_t preview[16]={112,48,70,22,100,36,60,28,124,52,76,24,94,44,66,32};
  for(unsigned j=0;j<16;j++){int a=2+(t?six_wave_peak[six_loop_now&1u][j]:preview[j])*(h-4)/127,xx=x+(int)j*w/16;
   cv_rect(xx,cy-a/2,w/16>2?w/16-1:1,a,ink);
   if(t?(jungle_playing()&&j==(unsigned)(jungle_current_step()/2)):(en&&j==(ph>>28)))cv_frame(xx-1,y,w/16+1,h,ink);
  }return;
 }
 /* Analog / phase / acid: the wave geometry follows their E parameters, voiced energy and LFO phase. */
 /* At 30 px a single 16-point trace resolves the same shape. Drawing all
  * three 32-point traces here only overdraws pixels and churns the DSP cache. */
 unsigned compact=w<48,points=compact?16u:32u,shift=compact?4u:5u;
 uint32_t stride=(compact?0x11111111u:0x08421084u)*(1u+(t?(unsigned)t->p[P_E0]%3u:1u));
 unsigned wt=engine==ENGI_SERUM?(unsigned)clamp(t?t->p[P_E0]:64,0,127)*7u*256u/127u:0u,wi=wt>>8;
 const serum_frame_t *sa=&SERUM_FR[wi],*sb=&SERUM_FR[wi<7u?wi+1u:7u];
 for(unsigned lane=compact?1u:0u;lane<(compact?2u:3u);lane++){
  int px=0,py=cy;for(unsigned j=0;j<points;j++){
   uint32_t a=ph+j*stride+lane*0x18000000u;
   int z=engine==ENGI_SERUM?serum_read(sa,sb,serum_warp(a>>16,t?(unsigned)t->p[P_E1]:32u),wt&255u):
         engine==ENGI_PHASE?osc_sine(a+((uint32_t)osc_sine(a)*((t?(unsigned)t->p[P_E1]:32u)+1u)*1024u)):
         engine==ENGI_M2SAW?(int)(a>>16)-32768:
         engine==ENGI_M2SID?((t&&(unsigned)t->p[P_E3]*5u/128u==2u)?((a>>24)<((unsigned)t->p[P_E0]*2u+1u)?32767:-32767):osc_tri(a)):
         engine==ENGI_ACID?((t&&t->p[P_E5])? (a<0x80000000u?32767:-32767):(int32_t)(a>>16)-32768):osc_tri(a);
   if(engine==ENGI_LOFI){
    unsigned wave=t?(unsigned)t->p[P_E1]:RW_PLS,duty=t?(unsigned)t->p[P_E2]:64u;
    z=wave==RW_PLS?((a>>24)<(duty*2u+1u)?32767:-32767):wave==RW_SAW?(int)(a>>16)-32768:osc_tri(a);
    if(wave==RW_WRAM){unsigned n=a>>27,b=WRAM[(duty/8u)&15u][n/2u];z=(int)((n&1u)?b&15u:b>>4)*4368-32760;}
    else {int q=t&&t->p[P_E0]==CHIP_1BIT?32768:4096;z=(z/q)*q;}
   }
   int xx=x+(int)((j*(unsigned)w)>>shift),yy=cy+z*amp/32768+(int)lane-1;
   cv_rect(xx,yy,2,2,ink);if(j&&lane==1)cv_line_t(px,py,xx,yy,T_DIM,1);px=xx;py=yy;
  }
 }
}
static void six_machine_mark(const track_t *t,int x,int y,int w,int h){
 int en=(ui_prefs&PREF_ANIM_OFF)?0:six_visual_level(t);
 unsigned ph=(en&&!(ui_prefs&PREF_ANIM_OFF))?t->lfo_ph:0u;
 unsigned at=six_vis_note_at[t-trk];
 if(en&&at&&!(ui_prefs&PREF_ANIM_OFF))ph+=(fm1_ms-(at-1u))*0x800000u*(1u+six_vis_note_vel[t-trk]/64u);
 six_machine_glyph(t,t->engine,en,ph,x,y,w,h,T_TEXT);
}
/* Main workstation: machine identity + six channels + the selected track's 16-step rail. */
static void six_home_cards(void){
 const track_t *t=TSEL;char n[5]={'0',(char)('1'+song.sel),0},name[40],line[40]="P0";
 cv_rect(6,3,27,27,T_TEXT);cv_text_in(6,8,27,&AF_S,n,T_BG,T_TEXT);
 cv_text_in(40,2,104,&AF_S,trk_engp(t)->name,T_TEXT,T_BG);
 sound_name(t,name);cv_clip_text(40,21,&AF_X,name,T_TEXT,T_BG,104);
 line[1]=(char)('1'+song_cur);line[2]=' ';line[3]='/';line[4]=' ';line[5]=live.cur>=0?(char)('A'+live.cur):'-';line[6]=' ';line[7]='/';line[8]=' ';line[9]=0;
 fmt_int(line+9,t->p[P_SLEN]);str_cpy(line+str_len(line)," STEPS",sizeof line-str_len(line));
 char pg[5]={' ',(char)('1'+t->seq_idx/16),'/',(char)('0'+((unsigned)clamp(t->p[P_SLEN],1,64)+15u)/16u),0};str_cpy(line+str_len(line),pg,sizeof line-str_len(line));
 cv_text_in(8,43,140,&AF_X,line,T_DIM,T_BG);
 if(six_track_enabled(song.sel))six_machine_mark(t,158,4,70,38);
 else cv_text_in(158,18,70,&AF_S,"OFF",T_DIM,T_BG);
 cv_line_t(6,58,234,58,T_TEXT,1);
 for(unsigned k=0;k<6;k++){
  int x=6+(int)k*38,selected=k==song.sel,en=six_visual_level(&trk[k]);char num[3]={'0',(char)('1'+k),0};
  if(selected)cv_rect(x,64,34,12,T_TEXT);
  cv_text_in(x,66,34,&AF_X,num,selected?T_BG:T_TEXT,selected?T_TEXT:T_BG);
  cv_text_in(x,81,34,&AF_X,!six_track_enabled(k)?"OFF":trk[k].p[P_MUTE]?"MUT":six_kind_label(trk[k].eng_req),T_TEXT,T_BG);
  if(six_track_enabled(k)&&!trk[k].p[P_MUTE])six_machine_mark(&trk[k],x+2,93,30,24);
  else six_machine_glyph(0,trk[k].engine,0,0,x+2,93,30,24,T_DIM);
  /* Sound level remains a small rail below the shared machine symbol. */
  cv_rect(x+3,121,28,1,T_DIM);
  if(en)cv_rect(x+3,121,1+en*27/127,1,T_TEXT);
  if(k<5)cv_line_t(x+36,65,x+36,122,T_DIM,1);
 }
 unsigned base=(t->seq_idx/16)*16;
 for(unsigned j=0;j<16;j++){
  const step_t *st=&seq_steps(t)[base+j];int x=8+(int)j*14,on=st->time!=ST_REST&&(st->n||st->hit||st->time==ST_TIE);
  if(on)cv_rect(x+2,129,7,5,T_TEXT);else cv_rect(x+5,131,1,1,T_DIM);
  if((j&3u)==0)cv_rect(x,139,1,1,T_TEXT);
  if(song.playing&&six_track_enabled(song.sel)&&!t->p[P_MUTE]&&j==(t->seq_idx&15u))cv_frame(x,126,11,11,T_TEXT);
 }
}
static void six_home_routes(void){
 for(unsigned m=0;m<4;m++){
  int y=6+(int)m*32;char n[3]={'M',(char)('1'+m),0},v[12];fmt_int(v,macro_v[m]);
  cv_text_in(8,y,22,&AF_X,n,T_TEXT,T_BG);cv_frame(34,y,62,9,T_DIM);cv_rect(35,y+1,macro_v[m]*60/127,7,T_TEXT);cv_text_in(102,y,26,&AF_X,v,T_TEXT,T_BG);
  unsigned count=0;for(unsigned k=0;k<6;k++)for(unsigned z=0;z<4;z++)if(trk[k].p[P_M1SRC+z*3]==(int)(MS_M1+m)&&trk[k].p[P_M1DST+z*3]&&trk[k].p[P_M1AMT+z*3]){
   char t[3]={'T',(char)('1'+k),0};int xx=136+(int)(count++%5)*19;cv_line_t(96,y+4,xx,y+4,T_DIM,1);cv_text_in(xx,y+11,18,&AF_X,t,T_TEXT,T_BG);
  }
  if(!count)cv_text_in(136,y,94,&AF_X,"NO ROUTE",T_DIM,T_BG);
 }
}
static void six_instrument(const track_t *t){
 char name[40];sound_name(t,name);six_dot_background((int)graph_h());
 cv_text_in(8,0,224,&AF_X,trk_engp(t)->name,T_TEXT,T_BG);
 six_machine_mark(t,16,22,208,82);
 cv_text_in(8,124,224,&AF_X,name,T_TEXT,T_BG);
 int lv=six_visual_level(t);for(unsigned k=0;k<16;k++)cv_rect(16+(int)k*13,113,9,2,k<(unsigned)lv*16/128?T_TEXT:T_DIM);
}
static const char *six_break_event_name(unsigned n){
 static const char *const name[19]={"1/4 REPEAT","BUILD","STRETCH","ECHO","1/8 REPEAT","1/8 DOWN","1/8 UP","1/8 SHRINK","1/16 REPEAT","1/16 DOWN","1/16 UP","1/16 SHRINK","1/32 REPEAT","1/32 DOWN","1/32 UP","1/32 SHRINK","SOURCE A / AMEN","SOURCE B / THINK","STRAIGHT RETURN"};
 return n<16?"SLICE TRIGGER":name[n<35?n-16:18];
}
static void six_graph_break_step(void){
 const track_t *t=TSEL;unsigned len=(unsigned)clamp(t->p[P_SLEN],1,64),base=(ui.cursor/16)*16;
 six_dot_background(graph_h());
 cv_text_in(8,7,224,&AF_X,t->p[P_LN0]?"CHOP / STEP EVENTS":"LOOP + PERFORMANCE EVENTS",T_TEXT,T_BG);
 for(unsigned j=0;j<16&&base+j<len;j++){
  const step_t *s=&t->step[base+j];int x=8+(j%8)*28,y=32+(j/8)*34;char b[8];
  if(s->time==ST_TIE)str_cpy(b,"=",8);else if(s->time==ST_REST||!s->n)str_cpy(b,".",8);else note_name(b,s->note[0]);
  cv_frame(x,y,25,25,base+j==ui.cursor?T_TEXT:T_DIM);cv_text_in(x,y+7,25,&AF_X,b,T_TEXT,T_BG);
  if(song.playing&&base+j==t->seq_idx)cv_rect(x,y+27,25,2,T_TEXT);
 }
 const step_t *s=&t->step[ui.cursor%len];
 cv_text_in(8,108,224,&AF_X,s->time==ST_REST?"REST":s->time==ST_TIE?"TIE / KEEP HELD EFFECT":six_break_event_name(s->note[0]),T_TEXT,T_BG);
 cv_text_in(8,128,224,&AF_X,"HOLD SEQ: STRAIGHT / MUT / RND",T_DIM,T_BG);
}
static void six_voice_list_graph(void){
 char name[13],count[20];int pos=dx_voice_pos();unsigned n=dx_voice_count();
 cv_text_in(8,2,224,&AF_X,"SECTION KIT / INDIVIDUAL SOUND",T_DIM,T_BG);
 for(int row=-2;row<=2;row++){int ix=pos+row;if(ix<0||ix>=(int)n)continue;unsigned id=dx_voice_at((unsigned)ix);dx_voice_name(name,id);int y=24+(row+2)*17;if(!row)cv_rect(6,y-2,228,15,T_TEXT);cv_text_in(12,y,208,&AF_X,name,row?T_DIM:T_BG,row?T_BG:T_TEXT);}
 fmt_int(count,pos<0?0:pos+1);str_cpy(count+str_len(count)," / ",sizeof count-str_len(count));fmt_int(count+str_len(count),(int)n);cv_text_in(8,118,224,&AF_X,count,T_DIM,T_BG);cv_text_in(8,130,224,&AF_X,"PLAY LANE KEY / SAVE PROJECT",T_DIM,T_BG);
}
static void six_graph_surface(const track_t *t,const page_t *pg){
 cv_oy=0;cv_bg=T_BG;
 if(ui.confirm==CF_DICE_ALL){gen_panel();return;}
 if(ui.home){if(home_set())six_home_routes();else if(home_level())six_instrument(t);else six_home_cards();return;}
 if(pg->graph==GR_DXSND&&dx_pagen(pg)==9u){six_voice_list_graph();return;}
 if(pg->graph==GR_TRK){six_graph_lanes(6);return;}
 if(pg->graph==GR_SIXSTEP){six_graph_break_step();return;}
 if(pg->graph==GR_SIXBRK||pg->graph==GR_SIXBRKSEQ){six_graph_break();return;}
 if(six_gen_page()){six_graph_gen();return;}
 if(six_return_page()){
  cv_text_in(8,18,224,&AF_S,"STRAIGHT BREAK",T_TEXT,T_BG);
  cv_text_in(8,48,224,&AF_X,"VAR / WARP -> 0",T_TEXT,T_BG);
  cv_text_in(8,72,224,&AF_X,"SOURCE / CHOPS / FX KEPT",T_TEXT,T_BG);
  cv_text_in(8,104,224,&AF_X,"OCT+ APPLY / OCT- BACK",T_TEXT,T_BG);return;
 }
 if(seq_roll_view(pg)){if(drum_track(t))graph_grid(t,T_TEXT);else graph_roll(t,T_TEXT);seq_overlay(t,pg);return;}
 if(lz_on()&&lz_fam()==LZ_FX){graph_fx_lz(t,pg);return;}
 if(pg->fam==FAM_ENV&&graph_env_lz(t,pg))return;
 if(pg->graph==GR_MSEQ||pg->id[0]==P_MSDIV){graph_mseq(t,T_TEXT);return;}
 if(pg->scope==SC_ENGINE){six_instrument(t);return;}
 if(pg->fam==FAM_EDIT){
  if(pg->id[0]==P_FTYPE){graph_filter_lz(t,pg);return;}
  if(pg->id[0]==P_VOICE||pg->id[0]==P_ALLOC){graph_voice_lz(t,pg);return;}
  if(pg->graph==GR_FOCUS){graph_focus_lz(t);return;}
  if(pg->graph==GR_MORPH){graph_morph(t);return;}
  if(pg->graph==GR_FMOP||pg->graph==GR_FMOP2||pg->graph==GR_FMENV){six_instrument(t);return;}
 }
 switch(pg->graph){
 case GR_ADSR:graph_adsr(t,T_TEXT);return;
 case GR_PATS:graph_pats();return;
 case GR_LIVE:graph_live();return;
 default:break;
 }
 /* Keep operational library/FX/key/automation diagrams; organic engine art is unreachable. */
 if(lz_on()&&lz_fam()==LZ_SAVE){graph_save_lz(pg);return;}
 if(lz_on()&&lz_fam()==LZ_KEY&&graph_key_lz(t,pg))return;
 if(pg->graph==GR_MOD){six_graph_matrix(t);return;}
 if(pg->graph==GR_LFO){graph_lfo(t,T_TEXT);return;}
 if(!graph_tech(t,pg))six_graph_vectors(t);
}
