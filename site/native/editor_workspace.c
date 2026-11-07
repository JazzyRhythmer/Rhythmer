// SPDX-License-Identifier: GPL-3.0-only
// Rhythmer browser data engine: actual editor, storage, project and archive C code. No audio / hardware I/O.
/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* The firmware editor handler, USB framing and UART recovery against RAM flash.
 * Build with the generated tables and the same flags as hostsim.c. */
static unsigned char host_flash[0x100000];                 /* fl_erase4k / fl_write (OTA staging) over 0xE0000.. */
#define FELUCCA_OTA 1
#define FELUCCA_FLASH 1
#define FELUCCA_VERSION "dev"
#define FELUCCA_CDC 1                                    /* (as the firmware: MENU > USB SERIAL switches the console) */
#define main hostsim_main
#include "../../rhythmer/tests/hostsim.c"
#undef main

static uint32_t host_progress = 1, host_erases, host_writes;
static uint8_t host_wire[4096];
static uint32_t host_wire_n;
static void host_drain(void)
{
    while (so_r != so_w) {
        uint32_t p = sx_out_q[so_r++ % SXQ], cin = p & 15u, i;
        uint32_t n = cin == 4u || cin == 7u ? 3u : cin == 6u ? 2u : 1u;
        for (i = 0; i < n && host_wire_n < sizeof host_wire; i++)
            host_wire[host_wire_n++] = (uint8_t)(p >> (8u * (i + 1u)));
    }
}
static uint32_t ota_now_ms(void) { return fm1_ms; }
static void ota_idle(void) { host_drain(); fm1_ms++; }
static void fm1_wdt_feed(void)
{
    fm1_ms++;
    if (host_progress && transport_req == 2u) { seq_stop(); transport_req = 0; }
}
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static int32_t fm1_enc_take(uint32_t e) { (void)e; return 0; }
static void lcd_sync(void) {}
static void lcd_power(uint32_t s) { (void)s; }   /* (MENU > SCREEN OFF: lcd.c) */
static void lcd_wake_now(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#include "../../rhythmer/firmware/src/gfx.c"
#include "../../rhythmer/firmware/src/panel.c"
#include "../../rhythmer/firmware/src/ui.c"
#include "../../rhythmer/firmware/src/menu_items.c"
static void panel_setup(void) {}


static uint32_t flash_ok = 1;
static void audio_silence(void) {}
static void fl_inval(uint32_t off, uint32_t n) { (void)off; (void)n; }
static uint8_t *host_flash_ptr(uint32_t off) { return host_flash + off; }
static int fl_erase4k(uint32_t off, uint32_t *took)
{
    memset(host_flash_ptr(off), 0xFF, 4096); *took = 0; host_erases++; return 0;
}
static int fl_write(uint32_t off, const void *p, uint32_t n)
{
    memcpy(host_flash_ptr(off), p, n); host_writes++; return 0;
}
static int st_read(uint32_t off, void *p, uint32_t n) { if(off>sizeof host_flash||n>sizeof host_flash-off)return -1;memcpy(p,host_flash+off,n);return 0; }
static int st_prog(uint32_t off, const void *p, uint32_t n) { if(off>sizeof host_flash||n>sizeof host_flash-off)return -1;memcpy(host_flash+off,p,n);return 0; }
static int st_erase(uint32_t off) { if(off>sizeof host_flash-4096)return -1;memset(host_flash+off,0xff,4096);return 0; }
static uint32_t irq_save(void){return 0;}
static void irq_restore(uint32_t x){(void)x;}
static uint32_t fl_jedec_ram(void){return 0x856014u;}
static void fl_plain_window_init(void){}
#define FL_FAR(fn) (fn)
#include "../../rhythmer/firmware/src/storage.c"
#include "../../rhythmer/firmware/src/upreset.c"
#include "../../rhythmer/firmware/src/break_sequence.c"
#include "../../rhythmer/firmware/src/project.c"
static const void *fm1_xip_ptr(uint32_t off){return host_flash_ptr(off);}
#include "../../rhythmer/firmware/src/six_slots.c"
#include "../../rhythmer/firmware/src/editor.c"

static uint8_t ws_in[1024], ws_archive_data[120000];
uint32_t ws_input(void){return (uint32_t)(uintptr_t)ws_in;}
uint32_t ws_output(void){return (uint32_t)(uintptr_t)host_wire;}
uint32_t ws_flash(void){return (uint32_t)(uintptr_t)host_flash;}
uint32_t ws_archive_ptr(void){return (uint32_t)(uintptr_t)ws_archive_data;}
void ws_init(void){
 memset(host_flash,0xff,sizeof host_flash);memset(&song,0,sizeof song);memset(trk,0,sizeof trk);
 memset(&chain,0,sizeof chain);chain_defaults(&chain_config);memset(&ed_w,0,sizeof ed_w);memset(&ui,0,sizeof ui);
 memset(&favorites,0,sizeof favorites);memset(&settings,0,sizeof settings);settings_init();panel=PANEL_DEFAULT;
 memset(proj_slot,0,sizeof proj_slot);memset(up_bank,0,sizeof up_bank);memset(&persist_saved,0,sizeof persist_saved);
 so_r=so_w=mi_r=mi_w=0;usb.config=1;usb.resets=0;transport_req=panic_req=sx_ready=sx_collect=sx_busy=0;
 host_progress=1;host_erases=host_writes=host_wire_n=0;flash_ok=1;fm1_ms=0;
 for(uint32_t i=0;i<G_COUNT;i++)song.g[i]=GP[i].def;
 fm6_init();power_on_parts();trk_live=6;song.sel=0;
}
void ws_boot(void){persist_boot();settings_init();if(project_used(0))project_load(0);autosave_boot(1);song.playing=0;transport_req=0;}
uint32_t ws_request(uint32_t n){
 if(n<6||n>sizeof ws_in||ws_in[0]!=0xf0||ws_in[1]!=0x7d||ws_in[2]!=0x46||ws_in[3]!=0x4c||ws_in[n-1]!=0xf7)return 0;
 /* Only the musical editor protocol. Never OTA, flash addresses or sample upload. */
 uint32_t cmd=ws_in[4];if(cmd==80||cmd>77)return 0;
 host_wire_n=0;ota_frame_done();fm1_ms++;
 for(uint32_t i=0;i<n;i++)sysex_byte(ws_in[i]);
 ed_service();host_drain();
 return host_wire_n;
}
static void ws_u32(uint32_t *pos,uint32_t value){memcpy(ws_archive_data+*pos,&value,4);*pos+=4;}
uint32_t ws_archive(void){
 song.playing=0;transport_req=0;
 if(ed_bk_capture())return 0;
 uint32_t pos=0;ws_u32(&pos,ED_BK_N);
 for(uint32_t k=0;k<ED_BK_N;k++){uint32_t len;const uint8_t *body=ed_bk_object(ED_BK_IDS[k],&len);if(len>3840||pos+12+len>sizeof ws_archive_data)return 0;
  ws_u32(&pos,ED_BK_IDS[k]);ws_u32(&pos,len);ws_u32(&pos,st_crc32(body,len));memcpy(ws_archive_data+pos,body,len);pos+=len;}
 return pos;
}
uint32_t ws_project(uint32_t n){if(n>=4)return 1;if(n==song_cur)return 0;song.playing=0;transport_req=0;return song_select(n)!=0;}
uint32_t ws_project_current(void){return song_cur;}
uint32_t ws_section_current(void){return proj_cur<4?proj_cur:255;}
uint32_t ws_section_save(uint32_t n){if(n>=4)return 1;song.playing=0;transport_req=0;return project_save(n);}
uint32_t ws_project_mask(uint32_t n){
 if(n>=NSONG)return 0;char name[13];if(n==song_cur)return lib_scan(n,name);
 uint32_t mask=0;for(uint32_t k=0;k<4;k++){st_hdr_t h;if(st_current(ed_bk_song_obj(n,k),&h)>=0&&proj_store_len(h.len))mask|=1u<<k;}return mask;
}
uint32_t ws_acid(uint32_t track,uint32_t mutate){if(track>=NTRK||trk[track].eng_req!=ENGI_ACID)return 1;six_gen_apply_to(&trk[track],mutate!=0,0);return 0;}

uint32_t ws_runtime_flash(void){
 if(ed_bk_capture())return 1;
 if(st_save(OBJ_AUTOSAVE,ED_BK_RAW,sizeof(project_store_t)))return 2;
 ui_prefs|=PREF_RESTORE_ON;settings_export(&persist_saved);
 return st_save(OBJ_SETTINGS,&persist_saved,sizeof persist_saved)?3:0;
}
