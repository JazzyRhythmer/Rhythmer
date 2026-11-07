/* SPDX-License-Identifier: GPL-3.0-only */
#define __attribute__(x)
#define main uac_schedule_reference_main
#include "uac_sched_test.c"
#undef main
int main(void){
    reset(0);uac_render_start();
    for(unsigned i=0;i<HALF_FRAMES;i++)rs_in[i]=0x12340000u+i;
    rq.busy=1;rq.job=1;rq.nin=0;rq.k=0;c1_done=0;uac.go=1;
    check("padding cannot reclaim the previous resampler job",!uac_guard_ready()&&rq.busy&&uac.go&&rq.nin==0);
    int ok=1;for(unsigned i=0;i<HALF_FRAMES;i++)ok&=rs_in[i]==0x12340000u+i;
    check("busy resampler input is untouched",ok);
    c1_done=1;check("completed job may be collected without waiting",uac_guard_ready()&&!rq.busy&&uac.go);
    puts(fails?"USB deadline padding ownership: FAIL":"USB deadline padding ownership: PASS");return !!fails;
}
