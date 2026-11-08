"""The same site routes on standalone tools; the enclosing site owns iframe navigation."""
from pathlib import Path
import hashlib,re,shutil

def apply_site_navigation(root,dist):
    css=root/'site/shared/site-navigation.css';target=dist/'assets/site-navigation.css'
    target.parent.mkdir(exist_ok=True);shutil.copy2(css,target)
    digest=hashlib.sha256(css.read_bytes()).hexdigest()[:12]
    routes=[('PLAY','play'),('GUIDE','guide'),('INSTALL','install'),('EDITOR','editor')]
    for name,active in [('installer/index.html','install'),('installer/verify.html','install'),('editor/index.html','editor'),('emulator/index.html','play')]:
        p=dist/name
        if not p.is_file():continue
        s=p.read_text()
        s=re.sub(r'<header class="rhy-site-header" data-rhythmer-navigation>.*?</header>','',s,count=1,flags=re.S)
        s=re.sub(r'<link rel="stylesheet" href="../assets/site-navigation.css[^"]*">','',s)
        s=s.replace('<script>if(window.self!==window.top)document.documentElement.setAttribute("data-rhythmer-framed","");</script>','')
        if name=='installer/index.html' and 'class="rhy-install-options"' not in s:
            s,n=re.subn(r'<header>.*?</header>', '<nav class="rhy-install-options" aria-label="Installation options"><a href="./">RHYTHMER INSTALL</a><a href="?mode=stock">RETURN TO V15</a></nav>',s,count=1,flags=re.S)
            if n!=1:raise ValueError('Installer header anchor changed')
        links=''.join('<a href="../#'+route+'" target="_top"'+(' aria-current="page"' if route==active else '')+'>'+label+'</a>' for label,route in routes)
        header='<header class="rhy-site-header" data-rhythmer-navigation><div class="rhy-site-inner"><a class="rhy-site-brand" href="../#home" target="_top" aria-label="Rhythmer home"><span aria-hidden="true">▥</span> RHYTHMER</a><nav aria-label="Main navigation">'+links+'</nav><a class="rhy-site-support" href="../#support" target="_top">SUPPORT ↗</a></div></header>'
        head='<link rel="stylesheet" href="../assets/site-navigation.css?v='+digest+'"><script>if(window.self!==window.top)document.documentElement.setAttribute("data-rhythmer-framed","");</script>'
        s=s.replace('</head>',head+'</head>',1)
        s,n=re.subn(r'(<body\b[^>]*>)',lambda m:m[1]+header,s,count=1)
        if n!=1:raise ValueError('Body anchor missing: '+name)
        p.write_text(s)
