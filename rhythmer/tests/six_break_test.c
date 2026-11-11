#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define JUNGLE_AUDIO_CODE
#define JUNGLE_TRACE
#include "../firmware/six/jungle.c"
static unsigned hits, slice_seen;
void jungle_midi_out(unsigned status){(void)status;}
void jungle_host_trigger(uint32_t f,unsigned s,int p,int32_t g){(void)f;(void)p;(void)g;hits++;slice_seen=s;}
static uint8_t sample[72000],sample_b[72000];
static void render(unsigned blocks){int32_t o[64];for(unsigned b=0;b<blocks;b++){memset(o,0,sizeof o);jungle_tick_mask(1);jungle_render(o,32);}}
static int check(const char *s,int ok){printf("break: %s %s\n",s,ok?"ok":"FAIL");return !ok;}
int main(void){int bad=0;for(unsigned i=0;i<sizeof sample;i++)sample[i]=(uint8_t)(i*17);
 /* Independently unpack a 12-bit bitstream, including every pair boundary
  * and the bank's last sample. ASan guards the odd-frame lookahead. */
 for(unsigned pos=0;pos<48000;pos++)for(unsigned f=0;f<3;f++){
  int32_t reference[2]={0};
  for(unsigned j=0;j<2&&pos+j<48000;j++){
   unsigned word=0,bit=(pos+j)*12;
   for(unsigned b=0;b<12;b++)word|=((sample[(bit+b)/8]>>((bit+b)%8))&1u)<<b;
   reference[j]=((int32_t)(word^2048u)-2048)*16;
  }
  struct reader r={0};r.pos=pos;r.frac=(uint32_t[]){0,0x800000,0xffffff}[f];
  int32_t want=pos+1<48000?reference[0]+(((reference[1]-reference[0])*(int32_t)(r.frac>>9))>>15):0;
  if(read_step(&r,sample,48000,0)!=want){printf("PCM12 mismatch at %u/%u\n",pos,f);return 3;}
 }
 bad+=check("144000 packed PCM12 boundary/interpolation cases",1);
 jungle_init();jungle_bind(sample,sizeof sample,48000);jungle_sequence_mode(1);bad+=check("two bars have sixteen unique chop positions",src_slice==3000&&src_slice*15==45000);jungle_play(1);render(1);hits=0;render(800);
 bad+=check("CHOP empty sequence has no autonomous hits",hits==0);
 jungle_sequence_note(7);render(1);bad+=check("JIANT chop fires immediately once",hits==1&&slice_seen==7);render(100);bad+=check("no auto loop underneath chop",hits==1);
 jungle_sequence_off(7);render(32);bad+=check("REST releases chop voices",!voices[0].active&&!voices[1].active);
 for(unsigned r=0;r<16;r++){jungle_sequence_note(16+r);render(1);bad+=check("recorded performance selects exact roll",jungle_roll_active()==(int)r);render(100);jungle_sequence_off(16+r);render(1);bad+=check("recorded roll release clears effect",jungle_roll_active()==-1);}
 jungle_roll(2);render(48);jungle_sequence_note(4);render(1);bad+=check("manual STRETCH applies to sequenced chop",(voices[0].active&&voices[0].grain_rate)||(voices[1].active&&voices[1].grain_rate));
 jungle_roll_off();render(48);jungle_sequence_note(3);render(1);bad+=check("released performance returns to next sequenced chop",slice_seen==3&&jungle_roll_active()==-1);
 jungle_sequence_off(32);jungle_sequence_mode(0);hits=0;render(800);bad+=check("LOOP empty sequence stays silent",hits==0);
 jungle_sequence_note(0);hits=0;render(800);bad+=check("LOOP source retains Amenity automatic chops",hits>0);straight_pattern();bad+=check("straight loop plays all sixteen chops across two bars",steps[0].slice==0&&steps[2].slice==1&&steps[14].slice==7&&steps[16].slice==8);
 jungle_sequence_off(0);hits=0;render(800);bad+=check("LOOP REST stops autonomous hits",hits==0&&!voices[0].active&&!voices[1].active);
 jungle_sequence_note(0);jungle_roll(10);render(96);bad+=check("manual UP roll is retained",jungle_roll_active()==10);jungle_roll_off();render(96);bad+=check("manual roll returns to underlying loop",jungle_roll_active()==-1);
 jungle_sequence_note(6);render(1);unsigned tick_before=bar_tick,k=voices[1].active&&!voices[1].releasing?1:0;uint32_t pos_before=voices[k].pos;
 jungle_swap(sample_b);bad+=check("A/B swap preserves transport and playback position",bar_tick==tick_before&&voices[k^1].pos==pos_before&&voices[k^1].source==sample_b&&voices[k].source==sample);
 bad+=check("A/B swap crossfades in existing two voices",voices[k].releasing&&voices[k^1].env==0&&!voices[k^1].releasing);render(64);
 jungle_play(0);render(32);bad+=check("transport stop releases audio",!voices[0].active&&!voices[1].active);
 jungle_start_phase(8*48);jungle_play(1);render(1);bad+=check("machine restart joins the transport phase",jungle_current_step()==8);
 jungle_init();jungle_play(1);render(1);bad+=check("init clears any pending start phase",jungle_current_step()==0);
 return bad?1:0;}
