/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca update loader, the portable part (step 2 of the M-UPGRADE update
 * protocol).
 *
 * The host serves the package ("logical image") with cmd 0x30 reads (ota.c).
 * The loader:
 *   1. reads the UFW header and finds flash.bin;
 *   2. checks that the package's app area decrypts with THIS chip's key
 *      (taken from isd_config.ini in the device's own flash head), so a
 *      package for another key is refused before anything is erased;
 *   2b. reads the whole flash.bin once and checks it against the CRC16 of its
 *      UFW entry (the vendor tools' check), keeping each app sector's CRC16:
 *      a damaged or truncated package is refused before anything is erased;
 *   3. writes the app [0x4000, 0x93000) and, for RHYBRK1 packages only,
 *      the two fixed banks [0xA0000, 0xC8000), sector by sector, skipping
 *      sectors that are already equal, and verifies each one; a sector that
 *      comes in different from the checked pass is read again, never written;
 *      the flash head (SPL, isd_config) is never written;
 *   4. asks 0xF0000000 ("success"), then invalidates the update record (RAM
 *      and flash: 4K boundary - 256 between 0x93000 and 0xFC000, outside
 *      Felucca's store, where only user data lives) so the SPL boots the new
 *      app, and resets.
 * Power loss during 3: the record is still there, the SPL runs the loader
 * again on the next power-on and the host can resume.
 *
 * Hooks from the platform (loader.c, ldr_test.c):
 *   ldr_fread(off, p, n)  ldr_erase(off)  ldr_prog(off, p, n)   flash, 0 = ok
 *   ldr_record_clear()    forget the RAM update record
 *   ldr_progress(done, total)
 * plus the ota.c hooks (frames, time, idle). */
#define LDR_APP_LO 0x4000u
#define LDR_APP_HI 0x93000u
#define LDR_REC_LO 0x93000u                     /* update records live above the app ... */
#define LDR_REC_HI 0xFC000u                     /* ... and below Felucca's globals (Felucca's records: 0xE4F00, */
#define LDR_DATA_LO 0x97000u                    /* the stock firmware's: 0xE8F00), never in Felucca's store, whose */
#define LDR_DATA_HI 0xE0000u                    /* bytes (the store) the host writes */
#define LDR_NSEC ((LDR_APP_HI - LDR_APP_LO) >> 12)
#define LDR_BANK_LO 0xA0000u
#define LDR_BANK_HI 0xC8000u
#define LDR_BANK_SIZE 0x14000u
#define LDR_BANK_NSEC 40u
#define LDR_FL_MAX 0x100000u                    /* flash.bin: at most the 1 MiB part */

static int ldr_fread(uint32_t off, void *p, uint32_t n);
static int ldr_erase(uint32_t off);
static int ldr_prog(uint32_t off, const void *p, uint32_t n);
static void ldr_record_clear(void);
static void ldr_progress(uint32_t done, uint32_t total);

/* SFC cipher: per 32-byte block, key ^ (block offset >> 2) */
static void ldr_sfc(uint8_t *p, uint32_t n, uint32_t base_off, uint32_t key)
{
    uint32_t i, j;
    for (i = 0; i < n; i += 32u) {
        uint32_t k = key ^ ((base_off + i) >> 2);
        for (j = 0; j < 32u && i + j < n; j++) {
            p[i + j] ^= (uint8_t)k;
            k = ((k << 1) ^ (k & 0x8000u ? 0x1021u : 0u)) & 0xFFFFu;
        }
    }
}

/* chip key from the device's flash head: JLFS top entries (ENC 0xFFFF) -> isd_config.ini blob */
static int ldr_chip_key(uint32_t *key)
{
    uint8_t e[32], blob[34];
    uint32_t off, i, sum, k;
    for (off = 32; off < 0x400u; off += 32u) {
        if (ldr_fread(off, e, 32))
            return -1;
        ota_jl_enc(e, 32);
        if (ota_crc16(e + 2, 30, 0) != ota_rd16(e))
            return -2;
        if (!ota_memeq(e + 16, (const uint8_t *)"isd_config.ini", 15)) {
            if (ota_rd16(e + 14))
                return -3;                               /* last entry, not found */
            continue;
        }
        if (ldr_fread(ota_rd32(e + 4), blob, 34) || ota_crc16(blob, 32, 0) != ota_rd16(blob + 32))
            return -4;
        for (i = 0, sum = 0; i < 16u; i++)
            sum += blob[i];
        sum &= 0xFFu;
        sum = sum >= 0xE0u ? 0xAAu : sum <= 0x10u ? 0x55u : sum;
        for (i = 0, k = 0; i < 16u; i++)
            if ((uint32_t)(blob[16 + i] ^ blob[15 - i]) < sum)
                k |= 1u << i;
        *key = k;
        return 0;
    }
    return -3;
}

/* An explicit package-only marker opts in to the two fixed factory banks.
 * Addresses are constants: a package cannot widen the write range to songs. */
static uint32_t ldr_crc32(const uint8_t *p,uint32_t n,uint32_t crc)
{
    while(n--){crc^=*p++;for(uint32_t k=0;k<8u;k++)crc=(crc>>1)^((0u-(crc&1u))&0xEDB88320u);}return crc;
}
static int ldr_bank_mode(uint32_t off,uint32_t size,uint8_t *b)
{
    if(size==LDR_APP_HI)return 0;
    if(size!=LDR_BANK_HI||ota_read(off+LDR_APP_HI,b,32))return -20;
    if(!ota_memeq(b,(const uint8_t *)"RHYBRK1",8)||ota_rd32(b+8)!=1u||
       ota_rd32(b+12)!=LDR_BANK_LO||ota_rd32(b+16)!=LDR_BANK_HI||
       ota_rd32(b+20)!=LDR_BANK_SIZE||ota_rd32(b+24)!=2u||
       (ldr_crc32(b,28,0xFFFFFFFFu)^0xFFFFFFFFu)!=ota_rd32(b+28))return -20;
    return 1;
}
/* off = UFW flash.bin offset for host checks, zero for final NOR readback. */
static int ldr_bank_read(uint32_t off,uint32_t addr,uint8_t *b,uint32_t n)
{ return off?ota_read(off+addr,b,n):ldr_fread(addr,b,n); }
static int ldr_banks_check(uint32_t off,uint8_t *b)
{
    for(uint32_t slot=0;slot<2u;slot++){
        uint32_t base=LDR_BANK_LO+slot*LDR_BANK_SIZE,crc=0xFFFFFFFFu,want;
        if(ldr_bank_read(off,base,b,512u))return -21;
        if(ota_rd32(b)!=0x504D5346u||b[4]!=12u||b[5]||b[6]!=1u||
           ota_rd32(b+16)!=72000u||ota_rd32(b+32)||ota_rd32(b+36)!=48000u||ota_rd32(b+48)!=23777u)return -22;
        want=ota_rd32(b+20);
        for(uint32_t at=0;at<72000u;at+=512u){
            uint32_t n=72000u-at<512u?72000u-at:512u;
            if(ldr_bank_read(off,base+512u+at,b,n))return -21;
            crc=ldr_crc32(b,n,crc);ota_idle();
        }
        if((crc^0xFFFFFFFFu)!=want)return -23;
    }
    return 0;
}
static uint32_t ldr_sector_index(uint32_t s)
{ return s<LDR_APP_HI?(s-LDR_APP_LO)>>12:LDR_NSEC+((s-LDR_BANK_LO)>>12); }

/* 2b. flash.bin [0, fl_size) from the host, before any erase: its CRC16 must be the UFW entry's (want); sc gets
 * the CRC16 of each app sector as it came in */
static int ldr_check(uint32_t fl_off, uint32_t fl_size, uint32_t want, uint16_t *sc, uint8_t *b)
{
    uint32_t i, n, crc = 0;
    for (i = 0; i < fl_size; i += n) {
        n = fl_size - i > 512u ? 512u : fl_size - i;
        if (ota_read(fl_off + i, b, n))
            return -7;
        crc = ota_crc16(b, n, crc);
        if ((i >= LDR_APP_LO && i < LDR_APP_HI) || (i >= LDR_BANK_LO && i < LDR_BANK_HI)) {     /* (512-byte reads from 0: never across a sector) */
            uint32_t k = ldr_sector_index(i);
            sc[k] = (uint16_t)ota_crc16(b, n, (i & 0xFFFu) ? sc[k] : 0u);
        }
    }
    return crc == want ? 0 : -12;
}

static int ldr_session(void)
{
    static uint8_t hdr[0x400], sec[0x1000], cur[0x1000];
    static uint16_t sc[LDR_NSEC+LDR_BANK_NSEC];
    uint32_t i, k, fl_off, fl_size, ota_off, ota_len, key, s, want = ~0u, done = 0;   /* (no CRC16: no flash.bin entry) */
    int rc, banks;
    /* 1. UFW header + entry list (ota.c; it leaves the entries decoded in hdr) */
    if ((rc = ota_ufw(hdr, &fl_off, &fl_size, &ota_off, &ota_len)) != 0)
        return rc;
    if (!fl_off || fl_size < LDR_APP_HI || fl_size > LDR_FL_MAX)
        return -3;
    for (i = 0; i < ota_rd16(hdr + 8); i++)
        if (ota_rd16(hdr + 0x40 + i * 0x50u) == 0)
            want = ota_rd16(hdr + 0x40 + i * 0x50u + 4u);   /* flash.bin's data CRC16 */
    /* 2. the package's app area must decrypt with this chip's key */
    if ((rc = ldr_chip_key(&key)) != 0)
        return -40 + rc;
    if (ota_read(fl_off + LDR_APP_LO, sec, 32))
        return -5;
    ldr_sfc(sec, 32, 0, key);
    if (ota_crc16(sec + 2, 30, 0) != ota_rd16(sec))
        return -6;                                       /* package for another chip key */
    banks=ldr_bank_mode(fl_off,fl_size,sec);if(banks<0)return banks;
    /* 2b. the whole package first: nothing is erased for a damaged one */
    if ((rc = ldr_check(fl_off, fl_size, want, sc, sec)) != 0)
        return rc;
    if(banks&&(rc=ldr_banks_check(fl_off,sec))!=0)return rc;
    /* App first, then only A/B. Commit each bank's header sector last.
     * Gaps, projects at 97000..A0000, songs at C8000.. and settings are skipped. */
    uint32_t total=LDR_NSEC+(banks?LDR_BANK_NSEC:0u);
    for (uint32_t index=0;index<total;index++) {
        if(index<LDR_NSEC)s=LDR_APP_LO+(index<<12);
        else{uint32_t j=index-LDR_NSEC;s=LDR_BANK_LO+(j/20u)*LDR_BANK_SIZE+(((j%20u)+1u)%20u)*0x1000u;}
        uint32_t again;
        for (again = 0;; again++) {                      /* as checked in 2b, else read it again */
            for (k = 0; k < 0x1000u; k += 512u)
                if (ota_read(fl_off + s + k, sec + k, 512))
                    return -7;
            if (ota_crc16(sec, 0x1000u, 0) == sc[ldr_sector_index(s)])
                break;
            if (again == 2u)
                return -13;
        }
        if (ldr_fread(s, cur, 0x1000u))
            return -8;
        if (!ota_memeq(sec, cur, 0x1000u)) {
            uint32_t tries;
            for (tries = 0; tries < 2u; tries++) {
                if (ldr_erase(s))
                    return -9;
                for (k = 0; k < 0x1000u; k += 256u)
                    if (ldr_prog(s + k, sec + k, 256))
                        return -10;
                if (ldr_fread(s, cur, 0x1000u))
                    return -8;
                if (ota_memeq(sec, cur, 0x1000u))
                    break;
            }
            if (tries == 2u)
                return -11;                              /* verify failed twice */
        }
        done += 0x1000u;
        ldr_progress(done, total<<12);
    }
    /* Do not report success or clear the resume record until both NOR payloads
     * pass their own CRC32, independently of transport/sector CRC16. */
    if(banks&&(rc=ldr_banks_check(0,sec))!=0)return rc;
    /* 4. finish: the host confirms, the record goes, the SPL boots the app */
    for (i = 0; i < 4u; i++)
        if (!ota_read(0xF0000000u, sec, 8))
            break;
    ldr_record_clear();
    for (s = LDR_REC_HI; s > LDR_REC_LO; s -= 0x1000u) {   /* flash records: 4K boundary - 256 */
        uint8_t r[80];
        if (s - 0x1000u >= LDR_DATA_LO && s - 0x1000u < LDR_DATA_HI)
            continue;                                    /* Felucca's store: user data that may look like one */
        if (ldr_fread(s - 0x100u, r, 80))
            break;
        if (ota_rd16(r + 6) == 0x5441u && ota_rd16(r) && ota_rd16(r) == ota_crc16(r + 2, 78, 0))
            ldr_erase(s - 0x1000u);
    }
    return 0;
}
