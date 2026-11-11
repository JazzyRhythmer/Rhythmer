/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define __attribute__(x)
#define FELUCCA_FLASH 1
#define FELUCCA_DUAL_CORE 0
static uint8_t flash[1<<20],reply[64],rn,flash_ok=1,six_loop_mask;
enum {NTRK=6, ENGI_BREAK=15};
static struct {unsigned engine;} trk[NTRK]={{ENGI_BREAK},{ENGI_BREAK},{ENGI_BREAK}};
typedef struct {unsigned loop_now;const uint8_t *bound;} jungle_state_t;
static jungle_state_t states[NTRK];
static jungle_state_t *six_break_state(const void *t){return &states[((const char*)t-(const char*)trk)/sizeof trk[0]];}
#define six_break_prepare six_break_state
#define test_bound states[0].bound
#define six_loop_now states[0].loop_now
static const uint8_t *six_loops[2];
static int busy;
static int st_read(uint32_t o,void *p,uint32_t n){memcpy(p,flash+o,n);return 0;}
static int st_erase(uint32_t o){assert(o>=0xA0000&&o+4096<=0xC8000);memset(flash+o,255,4096);return 0;}
static int st_prog(uint32_t o,const void *p,uint32_t n){assert(o>=0xA0000&&o+n<=0xC8000);for(unsigned i=0;i<n;i++)flash[o+i]&=((const uint8_t*)p)[i];return 0;}
#include "../firmware/src/storage.c"
static const uint8_t *fm1_xip_ptr(uint32_t o){return flash+o;}
static void jungle_bind(jungle_state_t *ctx,const uint8_t *p,unsigned n,unsigned f){ctx->bound=p;(void)n;(void)f;}
static int transport_busy(void){return busy;}
static void autosave_hold(void){}
static uint32_t irq_save(void){return 0;}
static void irq_restore(uint32_t f){(void)f;}
static void fm1_wdt_feed(void){}
static void ed_b(unsigned x){reply[rn++]=x&127;}
static void ed_str(const char *s,unsigned n){while(n--&&*s)ed_b(*s++);ed_b(0);}
static uint32_t ed_unpack7(const uint8_t *a, uint32_t na, uint8_t *out, uint32_t max)
{
    uint32_t n = 0;                                 /* groups: msb byte, then up to 7 bytes */
    while (na && n < max) {
        uint32_t m = *a++, j;
        na--;
        uint32_t k = na > 7u ? 7u : na;
        if (!k || n + k > max || (m >> k))
            return 0;                              /* incomplete group, overflow, or unused mask bits */
        for (j = 0; j < k; j++, na--)
            out[n++] = (uint8_t)(*a++ | ((m >> j) & 1u) << 7);
    }
    return na ? 0u : n;
}

#include "../firmware/src/six_upload.c"
#include "../firmware/src/six_slots.c"
static unsigned pack(const uint8_t *p,unsigned n,uint8_t *o){unsigned r=0;while(n){unsigned k=n>7?7:n,m=0;for(unsigned i=0;i<k;i++)m|=(p[i]>>7)<<i;o[r++]=m;for(unsigned i=0;i<k;i++)o[r++]=p[i]&127;p+=k;n-=k;}return r;}
static unsigned call(uint8_t *a,unsigned n){rn=0;assert(six_upload_handle(80,a,n));return reply[2];}
int main(int argc,char **argv){
 assert(argc==2);uint8_t slot[81920],a[600];FILE *f=fopen(argv[1],"rb");assert(f);assert(fread(slot,1,sizeof slot,f)==sizeof slot);fclose(f);memset(flash,0x5A,sizeof flash);
 states[1].loop_now=0;states[2].loop_now=1;
 a[0]=1;a[1]=2;assert(call(a,2)==1);a[1]=0;busy=1;assert(call(a,2)==3);assert(flash[0xA0000]==0x5A);busy=0;
 assert(call(a,2)==0);assert(flash[0xA0000]==255);
 for(unsigned off=0;off<72000;off+=256){unsigned n=72000-off>256?256:72000-off;a[0]=2;a[1]=0;a[2]=off&127;a[3]=(off>>7)&127;a[4]=(off>>14)&127;unsigned len=5+pack(slot+512+off,n,a+5);assert(call(a,len)==0);assert(call(a,len)==1);}
 slot[20]^=1;a[0]=3;a[1]=0;unsigned len=2+pack(slot,480,a+2);assert(call(a,len)==2);assert(flash[0xA0000]==255);slot[20]^=1;len=2+pack(slot,480,a+2);assert(call(a,len)==0);assert(six_loop_mask==1);
 for(unsigned i=0;i<sizeof flash;i++)if(i<0xA0000||i>=0xB4000)assert(flash[i]==0x5A);
 a[0]=1;a[1]=0;assert(call(a,2)==0);six_loop_scan();assert(!six_loop_mask);
 memcpy(flash+0xB4000,slot,sizeof slot);six_loop_scan();assert(six_loop_mask==2&&six_loop_now==1&&test_bound==flash+0xB4000+512);
 puts("SIX upload PASS: bounds, busy, sequence, CRC, commit-last, interrupted slot, other data preserved");return 0;
}
