// SPDX-License-Identifier: GPL-3.0-only
import {resampleTwoBars,packBreak} from './break-audio.mjs?v=45963ff270f5';
self.onmessage=e=>{try{const pcm=resampleTwoBars(e.data.mono,value=>self.postMessage({type:'progress',value})),out=packBreak(pcm,e.data.name);self.postMessage({type:'done',bytes:out.bytes,quant:out.quant},[out.bytes.buffer,out.quant.buffer]);}catch(error){self.postMessage({type:'error',message:error.message});}};
