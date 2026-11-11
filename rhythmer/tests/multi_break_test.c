/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#ifdef TEST_JAZZY
#include "jazzy_host.h"
#else
#include "ui_test.c"
#endif

static uint8_t bank[2][72000];

static void reset_breaks(void)
{
    ui_power_on();
    fm6_init();
    power_on_parts();
    trks_put(1);
    for(unsigned k=0;k<NTRK;k++){
        track_t *t=&trk[k];
        set_engine_of(t,ENGI_BREAK);
        t->engine=t->eng_req=ENGI_BREAK;
        t->xf=t->xf_on=0;
        apply_preset_to(t,k&1u);
        t->engine=t->eng_req=ENGI_BREAK;
        t->xf=t->xf_on=0;
        t->p[P_MUTE]=0;
        t->p[P_LN0]=1;
    }
    six_loops[0]=bank[0];six_loops[1]=bank[1];six_loop_mask=3;
    six_native_init();
    song.master_q12=4096;
}

static int run_tests(void)
{
    int bad=0;
    reset_breaks();
    unsigned available=1,independent=1,audible=1;
    for(unsigned k=0;k<NTRK;k++){
        available&=six_engine_available(k,ENGI_BREAK);
        jungle_state_t *ctx=six_break_state(&trk[k]);
        independent&=jungle_bound(ctx)==bank[k&1u];
        for(unsigned j=0;j<k;j++)independent&=ctx!=six_break_state(&trk[j])&&ctx->comb!=six_break_state(&trk[j])->comb;
        six_break_note(&trk[k],0,100);
        unsigned energy=0;int32_t out[32];
        for(unsigned n=0;n<64;n++){
            mc_host_cpu=(uint8_t)(k&1u);
            energy|=six_break_render(&trk[k],out,32);
        }
        audible&=energy!=0;
    }
    mc_host_cpu=0;
    bad+=check("all six tracks offer Break",available);
    bad+=check("six tracks use independent runtime and feedback lines with shared source banks",independent);
    bad+=check("all six Break tracks render audio",audible);
    jungle_state_t *first=six_break_state(&trk[0]),*second=six_break_state(&trk[1]);
    jungle_state_t saved=*first;
    uint8_t saved_line[JUNGLE_COMB_BYTES];
    memcpy(saved_line,first->comb,sizeof saved_line);
    int32_t reference[32],actual[32],other[32];
    six_break_render(&trk[0],reference,32);
    *first=saved;memcpy(first->comb,saved_line,sizeof saved_line);
    six_break_select(&trk[1],0,1);
    jungle_pitch(second,7);jungle_filter(second,92);jungle_roll(second,13);
    six_break_render(&trk[1],other,32);
    six_break_render(&trk[0],actual,32);
    bad+=check("other track source, pitch, filter and repeat do not change this track's PCM",!memcmp(reference,actual,sizeof actual));
    bad+=check("second track's A/B command changes only its own source parameter",trk[0].p[P_E4]==0&&trk[1].p[P_E4]==0&&jungle_bound(first)==bank[0]&&jungle_bound(second)==bank[0]);
    saved=*first;memcpy(saved_line,first->comb,sizeof saved_line);
    trk_all_off(&trk[1]);six_break_render(&trk[1],other,32);
    bad+=check("releasing one Break preserves another track's entire playback state",!memcmp(&saved,first,sizeof saved)&&!memcmp(saved_line,first->comb,sizeof saved_line));
    project_capture(&proj_scratch);
    bad+=check("six Break tracks pack and import in the existing project format",proj_pack(&proj_wire,&proj_scratch)&&proj_import(&proj_scratch,&proj_wire,sizeof proj_wire));

    track_t *changed=&trk[2];
    unsigned other_gates=six_break_state(&trk[3])->six_seq_gates;
    changed->eng_req=ENGI_ANALOG;engine_block(changed);
    set_engine_of(changed,ENGI_BREAK);changed->xf=changed->xf_on=0;engine_block(changed);
    jungle_state_t *returned=six_break_prepare(changed);
    bad+=check("returning from a synth to Break clears old gates, rolls and feedback without touching another track",!returned->six_seq_gates&&!returned->ctl_roll&&!returned->comb_left&&six_break_state(&trk[3])->six_seq_gates==other_gates);

    reset_breaks();
    gp.target=2;gp.genre=1;gp.seed=42;gp.page=0;
    char label[16];gen_value(0,3,label);
    bad+=check("T2 target identifies the assigned Break engine",!strcmp(label,"T2 BREAK")&&gen_target_index()==1);
    int16_t before=trk[0].p[P_MPAT];
    gp.page=1;gen_knob(0,1);
    bad+=check("Break GEN knobs edit the selected target's saved automatic controls",trk[1].p[P_MPAT]==1&&trk[0].p[P_MPAT]==before);
    gen_value(1,1,label);
    bad+=check("Break GEN shows a repeat-generation action",!strcmp(label,SIX_BREAK_ACTIONS[trk[1].p[P_LN1]]));
    gp.page=2;unsigned energy=gp.energy;gen_knob(0,-1);
    bad+=check("Break density knob changes generation density",gp.energy==energy-5);
    static step_t kept[NTRK][NSTEP];
    static int16_t parameters[NTRK][P_COUNT];
    for(unsigned k=0;k<NTRK;k++){memcpy(kept[k],trk[k].step,sizeof kept[k]);memcpy(parameters[k],trk[k].p,sizeof parameters[k]);}
    unsigned bpm=song.g[G_BPM];
    gen_go();
    unsigned others=1,valid=1,hits=0;
    for(unsigned k=0;k<NTRK;k++){
        others&=!memcmp(parameters[k],trk[k].p,sizeof parameters[k]);
        if(k!=1)others&=!memcmp(kept[k],trk[k].step,sizeof kept[k]);
    }
    for(unsigned i=0;i<(unsigned)trk[1].p[P_SLEN];i++){
        const step_t *step=&trk[1].step[i];
        if(step->time==ST_NOTE){hits++;for(unsigned n=0;n<step->n;n++)valid&=step->note[n]<=34;}
    }
    bad+=check("target GEN creates only valid Break commands and keeps other tracks, sounds and tempo",others&&valid&&hits&&song.g[G_BPM]==bpm);
    gp.page=0;gp.target=6;gen_knob(3,1);
    bad+=check("GEN target ends at T6 with no automatic BREAK target",gp.target==6);
    unsigned form=gp.form;
    gen_knob(3,-1);bad+=check("GEN target moves directly from T6 to T5",gp.target==5&&gp.form==form);
    printf("Multi-Break integration: %s\n",bad?"FAIL":"PASS");
    return !!bad;
}


#ifndef MULTI_BREAK_NO_MAIN
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    for(unsigned i=0;i<2;i++){
        FILE *file=fopen(argv[i+1],"rb");
        if(!file||fseek(file,512,SEEK_SET)||fread(bank[i],1,sizeof bank[i],file)!=sizeof bank[i])return 3;
        fclose(file);
    }
    return run_tests();
}

#endif
