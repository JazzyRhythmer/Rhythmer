// SPDX-License-Identifier: GPL-3.0-only
export async function fetchChecked(url,options={},timeout=45000){
 const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),timeout);
 try{const r=await fetch(url,{...options,signal:controller.signal});
  if(!r.ok)throw Error('Download failed ('+r.status+'): '+url);
  // Keep the deadline through the body; otherwise arrayBuffer() can hang after headers.
  const bytes=await r.arrayBuffer();return new Response(bytes,{status:r.status,statusText:r.statusText,headers:r.headers});
 }catch(e){if(e.name==='AbortError')throw Error('Download timed out. No new transfer can start until the complete package is verified. Check the connection and reload the page.');throw e;}finally{clearTimeout(timer);}
}
