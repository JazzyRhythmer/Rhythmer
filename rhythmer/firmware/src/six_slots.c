/* SPDX-License-Identifier: GPL-3.0-only */
/* Two 80 KiB PCM12 slots at A0000 and B4000; songs begin at C8000. */
static void six_loop_scan(void){
 six_loop_mask=0;
 for(unsigned k=0;k<2;k++){
  const uint8_t *p=fm1_xip_ptr(0xA0000u+k*0x14000u);const uint32_t *h=(const uint32_t *)p;
  six_loops[k]=0;
  if(!flash_ok||h[0]!=0x504d5346u||p[4]!=12||p[5]||p[6]!=1||h[4]!=72000u||
     h[8]!=0||h[9]!=48000u||h[12]!=23777u||st_crc32(p+512,72000)!=h[5])continue;
  six_loops[k]=p+512;six_loop_mask|=(uint8_t)(1u<<k);
 }
 for(unsigned i=0;i<NTRK;i++)if(trk[i].engine==ENGI_BREAK){
  jungle_state_t *ctx=six_break_prepare(&trk[i]);
  if(!six_loops[ctx->loop_now])ctx->loop_now=(six_loop_mask&1u)?0:1;
  if(six_loop_mask)jungle_bind(ctx,six_loops[ctx->loop_now],72000,48000);else jungle_bind(ctx,0,0,0);
 }
}
