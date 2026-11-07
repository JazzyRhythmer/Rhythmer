/* SPDX-License-Identifier: GPL-3.0-only
 * Real ISR + fixed-work simulated clock. No fake time cost per timer read.
 * Tests DMA ownership, stale/partial output, smoothing and musical selection. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#define FM1_AUDIO_HALF 0x80u
#define FM1_TICKS_PER_US 1u
static uint32_t clock_us,block_us,half_origin,half_id,tap_frames,stalls,start_delay;
static uint8_t pending=FM1_AUDIO_HALF;
static volatile uint32_t t5_nested_ticks;
static uint32_t fm1_ticks(void){return clock_us;}
static uint8_t fm1_audio_pending(void){return pending;}
static void fm1_audio_ack_aux(uint8_t p){(void)p;}
static uint32_t fm1_audio_free_half(void){return half_id^((clock_us-half_origin)/(HALF_FRAMES*1000000u/FS)&1u);}
static void fm1_audio_ack_half(void){}
static void fm1_audio_init(int32_t *b,uint32_t n,void(*isr)(void),uint32_t p){(void)b;(void)n;(void)isr;(void)p;}
void isr_alnk0(void){}
#define FELUCCA_UAC 1
static uint32_t rendered;
static void uac_tap(const int32_t *out,uint32_t n){
    (void)out;tap_frames+=n;
}
#define FM1_AUDIO_WORK_DONE() do{clock_us+=block_us;rendered++;if(stalls){clock_us+=(HALF_FRAMES*1000000u/FS)+50;stalls--;}}while(0)
static void uac_render_start(void){tap_frames=rendered=0;clock_us+=start_delay;}
static void uac_render_end(void){}
#include "../firmware/src/audio.c"
static unsigned bad,checks;
#define C(msg,x) do{int ok=!!(x);checks++;if(!ok){bad++;fprintf(stderr,"FAIL: %s\n",msg);}}while(0)
static void fresh(void){
    memset(trk,0,sizeof trk);memset(&song,0,sizeof song);memset(&chain,0,sizeof chain);
    host_tracks_init();fm6_init();mc_host=mc_host_v12=1;mc_off=0;
    for(unsigned p=0;p<NTRK;p++){host_preset(&trk[p],ENGI_ANALOG,0);trk[p].p[P_VOICE]=V_POLY;}
    gov_cap=16;gov_lite=gov_hot=gov_calm=gov_win=gov_lcalm=0;
    load_tight=tight_calm=shed_req=shed_over=0;shed_count=0;
    memset(&audio_guard,0,sizeof audio_guard);clock_us=block_us=half_origin=half_id=stalls=start_delay=0;
    memset(&felucca_dbg,0,sizeof felucca_dbg);
    fm1_ms=1000;pending=FM1_AUDIO_HALF;audio_last_half=2;
}
static voice_t *note(unsigned p,unsigned n,unsigned a){
    voice_t *v=&trk[p].v[n];memset(v,0,sizeof *v);v->active=v->gate=1;
    v->note=(uint8_t)a;v->stage=2;v->age=++vage;v->env_out=1000+n*100;
    voice_born[p][n]=fm1_ms-100;return v;
}
static void musical(void){
    fresh();voice_t *bass=note(0,0,48),*inner=note(0,1,55),*top=note(0,2,64);
    inner->env_out=2000;top->env_out=1;shed_voice();
    C("chord inner voice goes before bass or quiet top line",inner->stage==4&&bass->gate&&top->gate);
    fresh();voice_t *tail=note(0,0,48),*quieter=note(1,0,55);tail->gate=quieter->gate=0;
    tail->env_out=1000;quieter->env_out=10;shed_voice();
    C("quietest released tail first",quieter->stage==4&&tail->stage!=4);
    fresh();bass=note(0,0,48);inner=note(0,1,55);top=note(0,2,64);
    voice_born[0][1]=fm1_ms;voice_born[0][2]=fm1_ms;shed_voice();
    C("new 24 ms attacks are not forcibly shed",bass->gate&&inner->gate&&top->gate);
    fresh();trk[1].p[P_VOICE]=V_UNISON;voice_t *lead=note(1,0,50),*extra=note(1,1,50);
    bass=note(0,0,48);inner=note(0,1,55);top=note(0,2,64);shed_voice();
    C("duplicate unison before held chord",extra->stage==4&&lead->gate&&inner->gate);
    fresh();for(unsigned p=0;p<NTRK;p++)note(p,0,40+p);shed_voice();
    C("never remove a track's only primary voice",!shed_count);
}
static void governor(void){
    fresh();for(unsigned i=0;i<500;i++)gov_tick(1,0,0,0);
    C("soft CPU warnings preserve twelve voices and chosen FX",!gov_lite&&gov_cap==12);
    trk[0].engine=ENGI_FM6;unsigned full=trk_nvoice(&trk[0]);
    C("normal FM6 local polyphony retained",full==ENGINES[ENGI_FM6]->poly);
    sys_load=LOAD_LITE;C("explicit user LITE retains its local cap",trk_nvoice(&trk[0])==2);sys_load=0;
    for(unsigned i=0;i<100;i++)gov_tick(1,1,0,1);C("silent startup cannot lower the cap",gov_cap==12);
    gov_hot=gov_win=0;for(unsigned p=0;p<NTRK;p++)for(unsigned j=0;j<2;j++)note(p,j,40+p*3+j);
    for(unsigned i=0;i<GOV_STEP-1;i++)gov_tick(1,1,0,0);C("isolated hot blocks do not remove chord tones",gov_cap==12);
    gov_tick(1,1,0,0);C("repeated hot pressure removes one voice",gov_cap==11&&!gov_lite);
    for(unsigned i=0;i<GOV_STEP-1;i++)gov_tick(1,1,0,1);C("critical pressure still respects cooldown",gov_cap==11);
    gov_tick(1,1,0,1);C("next critical window removes only one more",gov_cap==10);
    for(unsigned i=0;i<1000;i++)gov_tick(1,1,0,0);C("noncritical heat preserves eight shared voices",gov_cap==8);
    for(unsigned i=0;i<1000;i++)gov_tick(1,1,0,1);C("actual deadline pressure retains six primary voices",gov_cap==6);
    gov_calm=0;for(unsigned i=0;i<GOV_SECOND/2-1;i++)gov_tick(0,0,1,0);C("recovery does not jump voices",gov_cap==6);
    gov_tick(0,0,1,0);C("half a calm second restores one voice",gov_cap==7);
    for(unsigned i=0;i<GOV_SECOND*8;i++)gov_tick(0,0,1,0);C("full restoration never changes FX",gov_cap==12&&!gov_lite);
}
static void guard(void){
    fresh();for(unsigned i=0;i<2*HALF_WORDS;i++)abuf[i]=1048576;
    start_delay=AUDIO_DEADLINE-20;fm1_alnk0_irq();
    C("late publication is detected without omitting DSP",audio_guard.hit&&rendered==16&&tap_frames==HALF_FRAMES);
    int good=1;for(unsigned i=0;i<2*HALF_WORDS;i++)good&=abuf[i]==1048576;
    C("late output retains complete prior PCM without zeroing or gain ramps",good&&!audio_guard.recover&&!audio_guard.clock_only);
    fresh();stalls=1;for(unsigned i=0;i<2*HALF_WORDS;i++)abuf[i]=1048576;
    trk_note_on(&trk[0],48,100);fm1_alnk0_irq();
    C("DMA flip cannot publish partially rendered Q15",audio_guard.hit&&felucca_dbg.late&&rendered==16);
    good=1;for(unsigned i=0;i<HALF_WORDS;i++)good&=abuf[i]==1048576;C("now-playing half is never overwritten after ownership loss",good);
    half_id=1;half_origin=clock_us;fm1_alnk0_irq();C("next half immediately renders at normal gain",!audio_guard.hit&&!audio_guard.recover&&rendered==16);
    fresh();audio_guard.cost=AUDIO_DEADLINE/4;block_us=640;note(0,0,48);fm1_alnk0_irq();
    C("88 percent actual work is not cut by a stale peak estimate",!audio_guard.hit&&rendered==16&&clock_us<AUDIO_DEADLINE);
    fresh();block_us=800;note(0,0,48);fm1_alnk0_irq();
    C("true overload is reported honestly, with a complete DSP and USB timeline",audio_guard.hit&&rendered==16&&tap_frames==HALF_FRAMES&&felucca_dbg.late&&audio_guard.clock_only==0);
}
static int normal(const char *path,int reference){
    fresh();for(unsigned p=0;p<NTRK;p++){trk[p].p[P_VOICE]=V_POLY;trk_note_on(&trk[p],36+p*5,100);trk_note_on(&trk[p],40+p*5,80);}
    FILE *f=fopen(path,"wb");if(!f)return 2;
    for(unsigned h=0;h<64;h++){
        if(reference){for(unsigned b=0;b<HALF_FRAMES;b+=CTL)audio_block(abuf+2*b,CTL);}
        else{half_id=h&1u;half_origin=clock_us;fm1_alnk0_irq();}
        fwrite(abuf+(reference?0:half_id*HALF_WORDS),sizeof(int32_t),HALF_WORDS,f);
    }
    fclose(f);if(!reference&&(audio_guard.trips||gov_cap!=12))return 3;return 0;
}
/* Identical fixed sends isolate FX from legitimate voice stealing. Even CLOUD
 * at more than six grains and pitched delay must retain their exact output
 * when the automatic pressure flags change. Separate processes start clean. */
static int fx_identity(const char *path,int pressure){
    fresh();sys_load=LOAD_FULL;
    song.g[G_RSIZE]=100;song.g[G_RRATE]=127;song.g[G_RMOD]=90;
    song.g[G_CRATE]=80;song.g[G_CDEPTH]=90;
    song.g[G_DMIX]=80;song.g[G_DFDBK]=70;song.g[G_DPIT]=7;song.g[G_DSPRY]=80;
    FILE *f=fopen(path,"wb");if(!f)return 2;
    unsigned grains=0;uint32_t noise=17;
    int32_t c[CTL],d[CTL],r[CTL],w[CTL];
    for(unsigned model=0;model<5;model++){
        song.g[G_RTYPE]=(int16_t)model;
        for(unsigned b=0;b<2048;b++){
            load_tight=pressure&&(b%11u)<8u;gov_lite=pressure;
            for(unsigned i=0;i<CTL;i++){
                noise=noise*1664525u+1013904223u;
                c[i]=d[i]=r[i]=(int32_t)(noise>>18)-8192;
            }
            fx_buses(c,d,r,w,CTL);fwrite(w,sizeof *w,CTL,f);
            if(model==4){unsigned n=0;for(unsigned i=0;i<CL_N;i++)n+=!!cl_g[i].len;if(n>grains)grains=n;}
        }
    }
    fclose(f);
    C("automatic CPU pressure preserves selected full FX algorithms",LOAD_EFF()==LOAD_FULL&&sys_load==LOAD_FULL);
    C("CLOUD identity test exercises more than six simultaneous grains",grains>CL_N/2u);
    printf("Fixed-send FX %s: %s; max CLOUD grains=%u\n",pressure?"pressure":"reference",bad?"FAIL":"PASS",grains);
    return !!bad;
}
/* Real musical state after every DMA half, with/without omitted DSP. Repeated
 * overload followed by a mute/relief must not stretch a beat or a bar. */
static int timeline(const char *path,int pressure){
    fresh();song.g[G_BPM]=160;
    for(unsigned p=0;p<NTRK;p++){
        track_t *t=&trk[p];t->p[P_SLEN]=16;t->p[P_SDIV]=2;t->p[P_SGATE]=64;
        for(unsigned i=0;i<16;i++){memset(&t->step[i],0,sizeof t->step[i]);t->step[i].time=ST_REST;if(!(i%4)){t->step[i].time=ST_NOTE;t->step[i].n=1;t->step[i].note[0]=(uint8_t)(36+p*5);t->step[i].vel=100;}}
    }
    seq_start();FILE *f=fopen(path,"wb");if(!f)return 2;
    for(unsigned h=0;h<1024;h++){
        half_id=h&1u;half_origin=clock_us;fm1_ms=(uint32_t)((uint64_t)h*HALF_FRAMES*1000u/FS);
        block_us=pressure&&h<640&&(h%11)<8?900:0;
        if(h==640)trk[0].p[P_MUTE]=1;
        fm1_alnk0_irq();
        uint32_t state[15]={song.tick,clk_pos,clk_step,clk_n,beat_pos,beat_n,bar_n,ms_clock,(uint32_t)song.playing};
        for(unsigned p=0;p<NTRK;p++)state[9+p]=trk[p].seq_idx;
        fwrite(state,sizeof state,1,f);
        C("one half advances sixteen musical blocks regardless of DSP omission",song.tick==(h+1u)*(HALF_FRAMES/CTL));
    }
    fclose(f);
    if(pressure)C("timing test exercised actual late publication without clock-only substitution",audio_guard.trips>100&&audio_guard.clock_only==0);
    printf("Musical timeline %s: %s; ticks=%u guarded=%u clock-only=%u\n",pressure?"pressure/mute":"reference",bad?"FAIL":"PASS",song.tick,audio_guard.trips,audio_guard.clock_only);
    return !!bad;
}
int main(int argc,char **argv){
    if(argc==3&&(!strcmp(argv[1],"fx-reference")||!strcmp(argv[1],"fx-pressure")))return fx_identity(argv[2],!strcmp(argv[1],"fx-pressure"));
    if(argc==3&&(!strcmp(argv[1],"time-reference")||!strcmp(argv[1],"time-pressure")))return timeline(argv[2],!strcmp(argv[1],"time-pressure"));
    if(argc==3)return normal(argv[2],!strcmp(argv[1],"reference"));
    musical();governor();guard();printf("Rhythmer musical overload + DMA guard: %s (%u checks)\n",bad?"FAIL":"PASS",checks);return !!bad;
}
