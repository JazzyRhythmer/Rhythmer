// Rhythmer shares the creator site's existing Kit Studio Notes form.
// The official embed owns subscription delivery and confirmation.
let observer;
export function cleanupStudioNotes(){observer?.disconnect();observer=null;}
export function mountStudioNotes(){
 cleanupStudioNotes();
 const slot=document.getElementById('studio-notes-form');
 if(!slot)return;
 const load=()=>{
  if(!slot.isConnected||slot.dataset.loaded)return;
  slot.dataset.loaded='true';observer?.disconnect();
  const hint=slot.querySelector('[data-signup-loading]');
  const script=document.createElement('script');
  script.async=true;script.dataset.uid='6dd4006c7c';
  script.src='https://jazzy-rhythmer.kit.com/6dd4006c7c/index.js';
  script.onload=()=>{if(slot.querySelector('form'))hint?.remove();};
  script.onerror=()=>{if(hint)hint.textContent='The signup form could not load. Use the Studio Notes link below.';};
  slot.append(script);
 };
 if('IntersectionObserver' in window){
  observer=new IntersectionObserver(entries=>{if(entries.some(e=>e.isIntersecting))load();},{rootMargin:'240px'});
  observer.observe(slot);
 }else load();
}
