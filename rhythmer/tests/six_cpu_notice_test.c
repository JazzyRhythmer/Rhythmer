/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void){
 ui_power_on();memset(&cpu_notice,0,sizeof cpu_notice);song.playing=1;song.cpu_q8=220;ui.menu=ui.confirm=ui.layer=ui.msg_t=0;
 fm1_ms=1000;ui_cpu_notice();fm1_ms=1800;ui_cpu_notice();if(ui.msg_t)return 1;
 fm1_ms=2300;ui_cpu_notice();if(strcmp(ui.msg,CPU_NOTICE)||!ui.msg_t||!song.playing)return 2;
 fm1_ms=4900;ui_cpu_notice();if(ui.msg_t)return 3;
 fm1_ms=10000;ui_cpu_notice();if(ui.msg_t)return 4;
 ui_message("PROJECT SAVED");fm1_ms=33000;ui_cpu_notice();if(strcmp(ui.msg,"PROJECT SAVED"))return 5;
 ui.msg_t=0;ui.menu=3;ui_cpu_notice();if(ui.msg_t)return 6;
 ui.menu=0;ui_cpu_notice();if(strcmp(ui.msg,CPU_NOTICE)||!ui.msg_t)return 7;
 if(ui.confirm||ui.layer||!song.playing)return 8;
 memset(&cpu_notice,0,sizeof cpu_notice);ui.msg_t=0;song.cpu_q8=100;load_diag.late=0;fm1_ms=40000;ui_cpu_notice();
 load_diag.late=1;fm1_ms=40100;ui_cpu_notice();fm1_ms=41200;ui_cpu_notice();if(ui.msg_t)return 9;
 puts("CPU notice: transient peaks, sustained load, cooldown, message/menu priority and transport PASS");return 0;
}
