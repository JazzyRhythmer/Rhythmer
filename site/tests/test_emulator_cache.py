#!/usr/bin/env python3
"""Dependency-free JS model/protocol tests through macOS JavaScriptCore + asset checks."""
import ctypes, hashlib, json, re, os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
SRC=ROOT/'site/src'
DIST=Path(os.environ.get('RHYTHMER_SITE_DIST',ROOT/'site/dist'))
jsc=ctypes.CDLL('/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore')
ptr=ctypes.c_void_p
jsc.JSGlobalContextCreate.argtypes=[ptr];jsc.JSGlobalContextCreate.restype=ptr
jsc.JSStringCreateWithUTF8CString.argtypes=[ctypes.c_char_p];jsc.JSStringCreateWithUTF8CString.restype=ptr
jsc.JSEvaluateScript.argtypes=[ptr,ptr,ptr,ptr,ctypes.c_int,ctypes.POINTER(ptr)];jsc.JSEvaluateScript.restype=ptr
jsc.JSValueToStringCopy.argtypes=[ptr,ptr,ctypes.POINTER(ptr)];jsc.JSValueToStringCopy.restype=ptr
jsc.JSStringGetMaximumUTF8CStringSize.argtypes=[ptr];jsc.JSStringGetMaximumUTF8CStringSize.restype=ctypes.c_size_t
jsc.JSStringGetUTF8CString.argtypes=[ptr,ctypes.c_char_p,ctypes.c_size_t];jsc.JSStringGetUTF8CString.restype=ctypes.c_size_t
jsc.JSStringRelease.argtypes=[ptr];jsc.JSGlobalContextRelease.argtypes=[ptr]
ctx=jsc.JSGlobalContextCreate(None)
def run(code):
 s=jsc.JSStringCreateWithUTF8CString(code.encode());error=ptr()
 value=jsc.JSEvaluateScript(ctx,s,None,None,1,ctypes.byref(error));jsc.JSStringRelease(s)
 value=error.value or value
 out=jsc.JSValueToStringCopy(ctx,value,None);n=jsc.JSStringGetMaximumUTF8CStringSize(out);buf=ctypes.create_string_buffer(n);jsc.JSStringGetUTF8CString(out,buf,n);jsc.JSStringRelease(out)
 if error.value:raise AssertionError(buf.value.decode())
 return buf.value.decode()
source=(DIST/'emulator/main.js').read_text()
start=source.index('async function packageOf(f)')
end=source.index('function noteVersion(',start)
run(source[start:end]+"""
const RHYTHMER_HASH='new';const packages=new Map();let fetches=0,keptReads=0,remote='new';
const packageHash=async x=>x;
const keptEntry=async id=>{keptReads++;return {package:'old',url:'saved'};};
const fetchPackage=async()=>{fetches++;return {bytes:remote,url:'current'};};
const noteVersion=()=>{};
let finished=false,checks=0,failure='';const check=(ok,label)=>{if(!ok)throw Error(label);checks++;};
(async()=>{
 let x=await packageOf({id:'rhythmer'});check(x.bytes==='new'&&fetches===1&&keptReads===0,'new release bypasses old saved firmware');
 x=await packageOf({id:'rhythmer'});check(x.bytes==='new'&&fetches===1,'same verified package reused');
 packages.set('rhythmer',{bytes:'old'});x=await packageOf({id:'rhythmer'});check(x.bytes==='new'&&fetches===2,'stale in-memory firmware refreshed');
 packages.delete('rhythmer');remote='wrong';let rejected=false;try{await packageOf({id:'rhythmer'});}catch{rejected=true;}check(rejected&&!packages.has('rhythmer'),'mixed deployment hash rejected');
 x=await packageOf({id:'local',bytes:'explicit-old'});check(x.bytes==='explicit-old','explicit old save remains selectable');
 x=await packageOf({id:'other'});check(x.bytes==='old'&&keptReads===1,'other firmware saved-package semantics preserved');
 finished=true;
})().catch(e=>{failure=String(e);});
""")
assert run('failure')=='',run('failure')
assert run('finished')=='true','async cache checks did not finish'
assert run('checks')=='6'
assert '?v=' in (DIST/'emulator/index.html').read_text()
assert '?build=' in source and 'cache:"no-store"' in source
print('PASS: six package-selection regressions; build-tagged runtime and seed')
jsc.JSGlobalContextRelease(ctx)
