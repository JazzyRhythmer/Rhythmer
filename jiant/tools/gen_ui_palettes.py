#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Rhythmer display palettes. No inherited colour themes or thermal ramps.
import argparse
from pathlib import Path
PALETTES=[('MONO',(0,0,0),(255,255,255)),('INVERT',(255,255,255),(0,0,0)),('RED',(0,0,0),(255,80,80)),('RED INV',(255,80,80),(0,0,0))]
def rgb(c):return (c[0]>>3)<<11 | (c[1]>>2)<<5 | c[2]>>3
def luminance(c):
 def lin(v):return v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4
 return sum(k*lin(v/255) for k,v in zip((.2126,.7152,.0722),c))
def main():
 p=argparse.ArgumentParser();p.add_argument('out',nargs='?');p.add_argument('--report',action='store_true');a=p.parse_args()
 rows=[]
 for name,bg,fg in PALETTES:
  lum=sorted((luminance(bg),luminance(fg)));contrast=(lum[1]+.05)/(lum[0]+.05);assert contrast>=4.5,(name,contrast)
  if a.report:print(name,'primary text contrast',round(contrast,2))
  b,f=rgb(bg),rgb(fg);rows.append(f'    {{"{name}",0x{b:04x},0x{b:04x},0x{f:04x},0x{f:04x},0x{f:04x}}},')
 out=['/* generated: four Rhythmer palettes, stable archive ids */','#pragma once','static const ui_pal_t UI_PALETTES[] = {',*rows,'};','static const uint8_t UI_STORAGE_IDS[4]={73,75,76,77};','static const uint8_t UI_RAISE_OF[4]={12,12,12,12};','static const uint16_t UI_KNOBS[4][4]={ {0xffff,0xffff,0xffff,0xffff},{0,0,0,0},{0xfa8a,0xfa8a,0xfa8a,0xfa8a},{0,0,0,0} };', '#define UI_NPALETTES 4u','#define UI_BW_INDEX 0u','#define UI_INVERT_INDEX 1u','#define UI_BW_GREY 0x528au','#define UI_PAL_TAG 64u']
 for k,v in {'MID':70,'DIM':42,'LINE':18,'SEL':80,'TINT':12,'KEY':78,'RAISE':12}.items():out.append(f'#define UI_{k}_PCT {v}')
 out+=['#define UI_REC_DARK 0xffffu','#define UI_REC_LIGHT 0u','#define UI_QR_LIGHT 0xffffu','#define UI_QR_DARK 0u','#define UI_CRASH_BG 0u','#define UI_CRASH_INK 0xffffu']
 if a.out:Path(a.out).write_text('\n'.join(out)+'\n')
if __name__=='__main__':main()
