// Shared navigation state only. No firmware, account or storage operations.
(()=>{
function activeSection(current,links){
 const path=current.pathname.toLowerCase().replace(/index\.html$/,'');
 if(/\/manual\//.test(path))return 'manual';
 if(/\/installer\//.test(path))return 'start';
 const hash=current.hash.split('/')[0];
 const aliases={"#guide":"manual","#manual":"manual","#install":"start","#recover":"start","#instrument":"engines","#engines":"engines","#hear":"engines","#engine-catalog":"engines","#patterns":"features","#companion":"features","#effects":"features","#sidechain":"features","#update":"features","#support":"studio","#creator":"studio","#notes":"studio"};
 if(aliases[hash])return aliases[hash];
 const exact=links.find(a=>{const t=new URL(a.href);return t.pathname.toLowerCase().replace(/index\.html$/,'')===path&&t.hash===current.hash});
 return exact?.dataset.section||'overview';
}
function mark(){const links=[...document.querySelectorAll('.jr-section-nav a')],section=activeSection(new URL(location.href),links);for(const a of links){if(a.dataset.section===section)a.setAttribute('aria-current','page');else a.removeAttribute('aria-current')}}
window.addEventListener('hashchange',()=>requestAnimationFrame(mark));window.addEventListener('load',mark);window.addEventListener('pageshow',mark);mark();
})();
