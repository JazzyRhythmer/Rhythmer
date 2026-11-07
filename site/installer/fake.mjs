// SPDX-License-Identifier: GPL-3.0-only
// FakeFM1 adapted from JIANT's pinned web/test_web.mjs. Test-only, never requests real MIDI.
import {pack7,unpack7} from './transport.mjs';
import {bkU32,bkR32,bkPack,bkUnpack,bkCrc} from './fm1backup.js';
const HS=[240,0,50,69,0,0,0,64,127,247],UPGRADE=[240,34,36,53,127,247];
const eq=(a,b)=>a.length===b.length&&a.every((v,i)=>v===b[i]);
export const infoOf=(name='Rhythmer v0.1.1-beta3')=>[...new TextEncoder().encode(name),0,21,127,71,64,119,...Array.from({length:21},()=>[65,0]).flat(),6,16,0x42,1,3];
export class FakeFM1 {
  constructor(image, { unplugAfter = Infinity, ...o } = {}) {
    this.image = image; this.unplugAfter = unplugAfter; this.served = 0; this.bad = 0; this.o = o;
    this.upgrades = 0; this.editorCmds = []; this.after = o.after || "FM-1_900";
    this.access = { inputs: new Map(), outputs: new Map() };
    this.boot(o.identity || "FM-1_015", o.name || "FM-1");
  }
  boot(identity, name, outName = this.o.outName) {
    this.identity = identity; this.waiting = null; this.queue = [];
    for (const m of [this.access.inputs, this.access.outputs]) { for (const p of m.values()) p.state = "disconnected"; m.clear(); }
    const id = Math.random().toString(36).slice(2);
    const open = this.o.busy ? async () => { const e = new Error("Port is not available"); e.name = "InvalidAccessError"; throw e; } : async () => {};
    this.input = { id: "i" + id, name, manufacturer: "fake", state: "connected", onmidimessage: null, open };
    this.output = { id: "o" + id, name: identity.startsWith("ota-") || !outName ? name : outName, state: "connected", open, send: (d) => {
      if (this.output.state !== "connected") throw new Error("InvalidStateError");
      setTimeout(() => this.rx(Array.from(d)), 1);
    } };
    this.access.inputs.set(this.input.id, this.input);
    this.access.outputs.set(this.output.id, this.output);
  }
  tx(bytes) { const i = this.input; setTimeout(() => { if (i.state === "connected" && i.onmidimessage) i.onmidimessage({ data: Uint8Array.from(bytes) }); }, 1); }
  editor(cmd,a){
    if(cmd!==80)return this.editorOriginal(cmd,a);
    const reply=b=>this.tx([240,125,70,76,80,...b,247]);
    this.sampleOps=(this.sampleOps||0)+1;if(a.length)this.sampleWrites=(this.sampleWrites||0)+1;
    if(!a.length)return reply([...new TextEncoder().encode('SIX3'),0,this.mask||0]);
    const [op,slot]=a;let rc=0;
    if(this.o.failSample)return reply([op,slot,2,this.mask||0]);
    if(op===1){this.slot=slot;this.payload=new Uint8Array(72000);this.pos=0;this.mask=(this.mask||0)&~(1<<slot);}
    else if(op===2){const off=a[2]|a[3]<<7|a[4]<<14;const count=Math.min(256,72000-off);const bytes=bkUnpack(a.slice(5),count);if(this.slot!==slot||off!==this.pos)rc=1;else{this.payload.set(bytes,off);this.pos+=count;}}
    else if(op===3){const h=bkUnpack(a.slice(2),480);if(this.pos!==72000||new DataView(h.buffer).getUint32(20,true)!==bkCrc(this.payload))rc=2;else{this.mask=(this.mask||0)|(1<<slot);this.banks??={};this.banks[slot]=this.payload.slice();}}
    else rc=1;
    reply([op,slot,rc,this.mask||0]);
  }
  editorOriginal(cmd, a) {
    this.editorCmds.push(cmd);
    const reply = (body) => this.tx([240, 125, 70, 76, cmd, ...body, 247]);
    if (!this.o.info || this.identity.startsWith("ota-")) return;      // an official firmware, a loader: silence
    if (cmd === 1) return reply(this.o.info);
    const objs = this.o.objs || [];
    if (cmd === 65) {
      const out = [1, 0, objs.length];
      for (const [oid, v] of objs) out.push(oid, ...bkU32(v.length), ...bkU32(v.length ? bkCrc(v) : 0));
      return reply(out);
    }
    if (cmd === 66) {
      if (this.o.dropGet && this.o.dropGet-- > 0) return;
      const oid = a[0], off = bkR32(a, 1), n = a[6] | a[7] << 7, v = objs.find(([k]) => k === oid)[1];
      return reply([oid, 0, ...bkU32(off), n & 127, n >> 7, ...bkPack(v.subarray(off, off + n))]);
    }
  }
  rx(d) {
    if (d[1] === 125 && d[2] === 70 && d[3] === 76) return this.editor(d[4], d.slice(5, -1));
    if (eq(d, HS)) {
      if (this.o.mute) return;
      const t = [...new TextEncoder().encode(this.identity)];
      const body = [0, 0x59, 0x11, 0, 0, 0, ...t, ...new Array(28 - t.length).fill(0)];
      this.tx([0xF0, ...pack7(body), 0xF7]);
    } else if (eq(d, UPGRADE)) {
      this.upgrades++;
      if (this.o.noUpgrade && !this.identity.startsWith("ota-")) return;
      this.queue = this.identity.startsWith("ota-")
        ? [...Array.from({ length: 6 }, (_, k) => [k * 512, 512]), [0xF0000000, 8]]
        : [[0, 64], [0x40, 160], [0x1000, 512], [0xE0000000, 8]];
      this.next();
    } else if (this.waiting) {
      const u = unpack7(d.slice(1, -1));
      const [addr, len] = this.waiting;
      const got = u.slice(14, 14 + (addr >= 0xE0000000 ? 8 : len));
      const want = addr >= 0xE0000000 ? [...new TextEncoder().encode("success"), 0] : Array.from(this.image.subarray(addr, addr + len));
      if (!eq(got, want)) this.bad++;
      this.waiting = null;
      this.served++;
      if (this.served >= this.unplugAfter) { this.input.state = this.output.state = "disconnected"; return; }
      if (addr === 0xE0000000) { if (!this.o.noLoader) setTimeout(() => this.boot("ota-FM-1_900", this.o.loaderName || "JIANT Update"), 300); }
      else if (addr === 0xF0000000) {if(this.o.embeddedBanks&&!this.o.noEmbeddedBanks){this.mask=3;this.banks=Object.fromEntries(this.o.embeddedBanks.map((x,i)=>[i,x.slice(512,72512)]));}setTimeout(() => this.boot(this.after, this.o.afterName || "JIANT"), 300);}
      else this.next();
    }
  }
  next() {
    const r = this.queue.shift();
    if (!r) return;
    this.waiting = r;
    const [addr, len] = r;
    const u = [0, 0x59, 0x30, 0, 0, 0, 0, addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF, len & 0xFF, len >> 8, 0];
    let s = 0;
    for (let i = 6; i < 14; i++) s += u[i];
    u.push(~s & 0xFF);
    this.tx([0xF0, ...pack7(u), 0xF7]);
  }
}

