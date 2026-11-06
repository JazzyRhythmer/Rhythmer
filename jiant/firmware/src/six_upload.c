/* SPDX-License-Identifier: GPL-3.0-only */
/* SIX PCM12 upload: cmd 80. No caller-supplied flash address; header committed last. */
static void six_loop_scan(void);
static uint8_t six_up_slot=255, six_up_buf[480];
static uint32_t six_up_pos;
static int six_upload_handle(uint32_t cmd,const uint8_t *a,uint32_t n){
 if(cmd!=80u)return 0;
 if(!n){ed_str("SIX3",8);ed_b(six_loop_mask);return 1;}
 uint32_t rc=1,op=a[0],slot=n>1?a[1]:255,base=0xA0000u+slot*0x14000u;
 if(slot>1u)goto reply;
 if(transport_busy()){rc=3;goto reply;}
#if FELUCCA_FLASH
 if(!flash_ok){rc=4;goto reply;}
 autosave_hold();
 if(op==1u&&n==2u){
  six_up_slot=255;six_up_pos=0;
  uint32_t f=irq_save();
#if FELUCCA_DUAL_CORE
  fm1_mc_park();
#endif
  six_loop_mask&=(uint8_t)~(1u<<slot);six_loops[slot]=0;
  if(six_loop_now==slot)jungle_bind(0,0,0);
  irq_restore(f);
  rc=0;for(uint32_t off=0;off<0x14000u;off+=4096u){fm1_wdt_feed();if(st_erase(base+off)){rc=4;break;}}
  if(!rc)six_up_slot=(uint8_t)slot;
 }else if(op==2u&&n>5u&&six_up_slot==slot){
  uint32_t off=a[2]|(uint32_t)a[3]<<7|(uint32_t)a[4]<<14;
  uint32_t len=ed_unpack7(a+5,n-5,six_up_buf,256);
  if(len&&off==six_up_pos&&len<=72000u-six_up_pos){rc=st_prog(base+512u+off,six_up_buf,len)?4u:0u;if(!rc)six_up_pos+=len;}
 }else if(op==3u&&six_up_slot==slot&&six_up_pos==72000u){
  uint32_t len=ed_unpack7(a+2,n-2,six_up_buf,sizeof six_up_buf);
  uint32_t h[13];if(len!=480u)goto reply;memcpy(h,six_up_buf,sizeof h);
  if(h[0]!=0x504D5346u||six_up_buf[4]!=12||six_up_buf[5]||six_up_buf[6]!=1||h[4]!=72000u||h[8]!=0||h[9]!=48000u||h[12]!=23777u)goto reply;
  if(st_crc32(fm1_xip_ptr(base+512u),72000u)!=h[5]){rc=2;goto reply;}
  rc=st_prog(base,six_up_buf,256)||st_prog(base+256u,six_up_buf+256,224)?4u:0u;
  if(!rc){six_up_slot=255;uint32_t f=irq_save();
#if FELUCCA_DUAL_CORE
   fm1_mc_park();
#endif
   six_loop_scan();irq_restore(f);if(!(six_loop_mask&(1u<<slot)))rc=2;}
 }
#endif
reply:ed_b(op);ed_b(slot);ed_b(rc);ed_b(six_loop_mask);return 1;
}
