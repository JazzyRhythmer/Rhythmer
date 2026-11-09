#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Losslessly pack FMini's 200 x 32 native program bytes; no audio quantization.

Input is a host extraction of FMINI_FACTORY from the retained upstream header.
The native header remains the independent reference used by the host tests.
"""
import argparse
import hashlib
from pathlib import Path

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('raw',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
    raw=a.raw.read_bytes()
    if len(raw)!=200*32:raise SystemExit('Expected exactly 200 x 32 bytes')
    rows=[raw[i:i+32]for i in range(0,len(raw),32)]
    lo=[min(r[i]for r in rows)for i in range(32)]
    widths=[(max(r[i]for r in rows)-lo[i]).bit_length()for i in range(32)]
    size=(sum(widths)+7)//8;packed=[]
    for r in rows:
        n=shift=0
        for i in range(32):n|=(r[i]-lo[i])<<shift;shift+=widths[i]
        packed.append(n.to_bytes(size,'little'))
    # Independent inverse before emitting the bank.
    for row,original in zip(packed,rows):
        n=int.from_bytes(row,'little');out=[]
        for l,w in zip(lo,widths):out.append(l+(n&((1<<w)-1)));n>>=w
        assert bytes(out)==original
    header=('/* SPDX-License-Identifier: GPL-3.0-only\n'
            ' * Lossless FMini-1 native program packing; original authors/licence retained.\n'
            f' * Reference SHA256 {hashlib.sha256(raw).hexdigest()}. */\n')
    header+='static const uint8_t FMNI_LO[32]={'+','.join(map(str,lo))+'};\n'
    header+='static const uint8_t FMNI_W[32]={'+','.join(map(str,widths))+'};\n'
    header+=f'static const uint8_t FMNI_PACK[200][{size}]={{\n'
    header+=''.join(' {'+','.join(map(str,r))+'},\n'for r in packed)+'};\n'
    header+='''static void fmini_factory_load(unsigned slot,uint8_t *out){
 if(slot>=200)slot=0;
 const uint8_t *row=FMNI_PACK[slot];unsigned bit=0;
 for(unsigned i=0;i<32;i++){
  unsigned w=FMNI_W[i],v=0;
  if(w){unsigned byte=bit>>3,shift=bit&7;v=row[byte];if(shift+w>8)v|=(unsigned)row[byte+1]<<8;v=(v>>shift)&((1u<<w)-1);}
  out[i]=(uint8_t)(FMNI_LO[i]+v);bit+=w;
 }
}
'''
    a.output.write_text(header)
    print(f'{len(raw)} -> {200*size+64} ROM data bytes, all 200 programs exact')

if __name__=='__main__':main()
