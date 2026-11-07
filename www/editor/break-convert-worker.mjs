// SPDX-License-Identifier: GPL-3.0-only
import {resampleTwoBars,packBreak} from './break-audio.mjs?v=9bd5b4d878ac';
self.onmessage=e=>{try{const pcm=resampleTwoBars(e.data.mono,value=>self.postMessage({type:'progress',value}),e.data.plan||{}),out=packBreak(pcm,e.data.name);self.postMessage({type:'done',bytes:out.bytes,quant:out.quant,quality:out.quality,gainDb:out.gainDb},[out.bytes.buffer,out.quant.buffer]);}catch(error){self.postMessage({type:'error',message:error.message});}};
