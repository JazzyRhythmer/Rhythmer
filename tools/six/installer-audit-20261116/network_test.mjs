import {ReadableStream} from 'node:stream/web';
import assert from 'node:assert/strict';
import {fetchChecked} from './stage/Rhythmer/installer/network.mjs';
const original=globalThis.fetch;let n=0;
try{
 globalThis.fetch=async()=>new Response('ready');assert.equal(await(await fetchChecked('x')).text(),'ready');n++;
 globalThis.fetch=async()=>new Response('missing',{status:404});await assert.rejects(()=>fetchChecked('x'),/404/);n++;
 globalThis.fetch=(_,o)=>new Promise((_,reject)=>o.signal.addEventListener('abort',()=>reject(Object.assign(new Error('aborted'),{name:'AbortError'}))));await assert.rejects(()=>fetchChecked('x',{},10),/timed out/);n++;
 globalThis.fetch=async(_,o)=>new Response(new ReadableStream({start(c){o.signal.addEventListener('abort',()=>c.error(Object.assign(new Error('aborted'),{name:'AbortError'})));}}));await assert.rejects(()=>fetchChecked('x',{},10),/timed out/);n++;
 console.log('PASS',n,'network deadlines including stalled response body');
}finally{globalThis.fetch=original;}
