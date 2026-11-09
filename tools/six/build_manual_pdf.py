#!/usr/bin/env python3
"""Build the complete English print manual from the same chapters as the website."""
from pathlib import Path
from html.parser import HTMLParser
from html import escape
import hashlib,json,re,os
from reportlab.platypus import (BaseDocTemplate,PageTemplate,Frame,Paragraph,Spacer,
 PageBreak,Table,TableStyle,Image,NextPageTemplate)
from reportlab.platypus.tableofcontents import TableOfContents
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

R=Path(__file__).resolve().parents[2]
CONFIG=json.loads((R/'site/public.config.json').read_text())
VERSION=CONFIG['version']
BUILD=CONFIG['buildRevision']
OUT=Path(os.environ.get('RHYTHMER_MANUAL_OUT',R/'output/pdf'/f'Rhythmer-Manual-v{VERSION}.pdf'))
QA=Path(os.environ.get('RHYTHMER_MANUAL_QA',R/'qa/hub-submission-20261108'))
DATA=Path(os.environ.get('RHYTHMER_MANUAL_SECTIONS',R/'tmp/pdfs/manual-sections.json'))
WEB='https://jazzyrhythmer.github.io/Rhythmer/'
assert OUT.parent.resolve().is_relative_to(Path('/Volumes/SSD/Development'))
for name,file in [('Body','Arial.ttf'),('Body-Bold','Arial Bold.ttf'),('Body-Italic','Arial Italic.ttf'),('Body-BoldItalic','Arial Bold Italic.ttf')]:
 pdfmetrics.registerFont(TTFont(name,'/System/Library/Fonts/Supplemental/'+file))
pdfmetrics.registerFontFamily('Body',normal='Body',bold='Body-Bold',italic='Body-Italic',boldItalic='Body-BoldItalic')
W,H=A4;M=48;CW=W-2*M
INK=colors.HexColor('#111313');GRAY=colors.HexColor('#5e6666');LINE=colors.HexColor('#d5dada')
styles={
 'Body':ParagraphStyle('Body',fontName='Body',fontSize=9.6,leading=13.7,textColor=INK,spaceAfter=8),
 'Chapter':ParagraphStyle('Chapter',fontName='Body-Bold',fontSize=22,leading=27,spaceAfter=20,keepWithNext=True),
 'h2':ParagraphStyle('h2',fontName='Body-Bold',fontSize=15,leading=19,spaceBefore=9,spaceAfter=10,keepWithNext=True),
 'h3':ParagraphStyle('h3',fontName='Body-Bold',fontSize=11.2,leading=15,spaceBefore=13,spaceAfter=7,keepWithNext=True),
 'Cell':ParagraphStyle('Cell',fontName='Body',fontSize=8.5,leading=12,textColor=INK,spaceAfter=0),
 'Caption':ParagraphStyle('Caption',fontName='Body',fontSize=8,leading=11,textColor=GRAY,spaceAfter=12),
 'Step':ParagraphStyle('Step',fontName='Body',fontSize=9.6,leading=13.7,textColor=INK,leftIndent=19,firstLineIndent=-16,spaceAfter=7),
 'TOC':ParagraphStyle('TOC',fontName='Body',fontSize=10.5,leading=14,spaceBefore=3,textColor=INK),
}
def clean(s):
 for a,b in [('→',' -> '),('↗',''),('↓',''),('↑',''),('↔',' <-> '),('–','-'),('—','-'),('−','-'),('‑','-'),('…','...'),('’',"'"),('‘',"'"),('“','"'),('”','"'),('♯','#'),('♭','b'),('≠',' is not '),('·',' / '),('\xa0',' ')]:s=s.replace(a,b)
 return s
class Node:
 def __init__(self,tag='root',attrs=()):self.tag=tag;self.attrs=dict(attrs);self.children=[]
class DOM(HTMLParser):
 def __init__(self,s):
  super().__init__(convert_charrefs=True);self.root=Node();self.stack=[self.root];self.feed(s)
 def handle_starttag(self,tag,attrs):
  n=Node(tag,attrs);self.stack[-1].children.append(n)
  if tag not in ['img','br','hr','input','meta','link']:self.stack.append(n)
 def handle_endtag(self,tag):
  for i in range(len(self.stack)-1,0,-1):
   if self.stack[i].tag==tag:self.stack=self.stack[:i];break
 def handle_data(self,s):self.stack[-1].children.append(s)
def plain(n):return n if isinstance(n,str) else ''.join(plain(x) for x in n.children)
def href(s):
 if s.startswith('#guide/'):return WEB+s
 if s.startswith('#'):return WEB+s
 if not s.startswith(('https:','http:','mailto:')):return WEB+s
 return s
def inline(n):
 if isinstance(n,str):return escape(clean(n))
 content=''.join(inline(x) for x in n.children)
 if n.tag in ['b','strong']:return '<b>'+content+'</b>'
 if n.tag in ['i','em']:return '<i>'+content+'</i>'
 if n.tag=='br':return '<br/>'
 if n.tag=='a':return '<a href="'+escape(href(n.attrs.get('href','')),quote=True)+'" color="#284747">'+content+'</a>'
 return content
def para(s,style='Body'):return Paragraph(s,styles[style])
def nodes(n,tag):
 if isinstance(n,str):return []
 return ([n] if n.tag==tag else [])+sum((nodes(c,tag) for c in n.children),[])
def blocks(children):
 out=[]
 for n in children:
  if isinstance(n,str):
   if n.strip():out.append(para(escape(clean(n))))
   continue
  tag=n.tag
  if tag in ['p','h2','h3']:out.append(para(inline(n),tag if tag in ['h2','h3'] else 'Body'))
  elif tag in ['ol','ul']:
   for i,c in enumerate([x for x in n.children if isinstance(x,Node) and x.tag=='li']):
    prefix=f'{i+1:02d}.' if tag=='ol' else '-'
    out.append(para('<b>'+prefix+'</b>  '+inline(c),'Step'))
  elif tag=='figure':
   image=nodes(n,'img')[0];src=image.attrs['src'].replace('assets/','');path=R/'site/media'/src
   assert path.is_file(),path
   size=180 if 'six-track-warning' in src else 195
   pic=Image(str(path),width=size,height=size,hAlign='LEFT');out.extend([Spacer(1,4),pic,Spacer(1,5)])
   for c in nodes(n,'figcaption'):out.append(para(inline(c),'Caption'))
  elif tag=='table':
   rows=[]
   for row in nodes(n,'tr'):
    cells=[x for x in row.children if isinstance(x,Node) and x.tag in ['th','td']]
    rows.append([para(('<b>'+inline(c)+'</b>') if c.tag=='th' else inline(c),'Cell') for c in cells])
   count=len(rows[0]);widths=[CW*.28,CW*.72] if count==2 else [CW*.24,CW*.38,CW*.38] if count==3 else [CW/count]*count
   table=Table(rows,colWidths=widths,repeatRows=1,hAlign='LEFT',spaceBefore=6,spaceAfter=13)
   table.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),colors.HexColor('#edf0ef')),('LINEBELOW',(0,0),(-1,0),.7,INK),('LINEBELOW',(0,1),(-1,-1),.35,LINE),('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),('TOPPADDING',(0,0),(-1,-1),8),('BOTTOMPADDING',(0,0),(-1,-1),8)]));out.append(table)
  elif tag=='aside':
   text='<br/>'.join(inline(x) for x in n.children if not isinstance(x,str) or x.strip())
   box=Table([[para(text)]],colWidths=[CW],hAlign='LEFT',spaceBefore=7,spaceAfter=12)
   box.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,-1),colors.HexColor('#f1f3f2')),('LINEBEFORE',(0,0),(-1,-1),2,INK),('LEFTPADDING',(0,0),(-1,-1),12),('RIGHTPADDING',(0,0),(-1,-1),12),('TOPPADDING',(0,0),(-1,-1),10),('BOTTOMPADDING',(0,0),(-1,-1),7)]));out.append(box)
  else:out.extend(blocks(n.children))
 return out
class ManualDoc(BaseDocTemplate):
 def afterFlowable(self,f):
  if getattr(f,'_bookmarkName',None):
   key=f._bookmarkName;title=f.getPlainText();self.canv.bookmarkPage(key);self.canv.addOutlineEntry(title,key,0)
   self.notify('TOCEntry',(0,title,self.page,key))
def cover(c,doc):
 c.saveState();c.setFillColor(INK);c.rect(0,0,W,H,fill=1,stroke=0);c.setFillColor(colors.HexColor('#eeeee2'))
 c.setFont('Body',10);c.drawString(M,H-58,'R / OS');c.drawRightString(W-M,H-58,'M-VAVE FM-1')
 c.setFont('Body-Bold',50);c.drawString(M,H-144,'RHYTHMER');c.setFont('Body-Bold',18);c.drawString(M,H-185,'SOUND INTO MOTION.')
 c.setStrokeColor(colors.HexColor('#66706c'));c.line(M,H-218,W-M,H-218)
 c.setFont('Body-Bold',24);c.drawString(M,H-265,'USER MANUAL');c.setFont('Body',10.5);c.drawString(M,H-294,f'v{VERSION} / Build {BUILD} / English')
 labels=[('drum','DRUM'),('serum','SERUM'),('fmini','FMINI-1'),('obervave','OBERVAVE'),('acid-generate','ACID'),('break-perform','BREAK')]
 for i,(name,label) in enumerate(labels):
  x=M+(i%3)*172;y=275-(i//3)*170
  c.drawImage(str(R/'site/media/motion'/(name+'.png')),x,y,145,145,mask='auto')
  c.setFont('Body',8);c.drawString(x,y-15,label)
 c.setFont('Body',10);c.drawString(M,66,'A groovebox with up to six assignable tracks.')
 c.setFont('Body',8.5);c.drawString(M,45,'Jazzy Rhythmer / Built on JIANT / Free and open source')
 c.linkURL(WEB,(M,37,W-M,60),relative=0);c.restoreState()
def regular(c,doc):
 c.saveState();c.setFillColor(GRAY);c.setFont('Body',8);c.drawString(M,H-30,'RHYTHMER / USER MANUAL');c.drawRightString(W-M,H-30,f'v{VERSION} / {BUILD}')
 c.setStrokeColor(LINE);c.line(M,38,W-M,38);c.setFont('Body',8);c.drawString(M,24,'Jazzy Rhythmer / Sound into motion.');c.drawRightString(W-M,24,str(doc.page));c.restoreState()
doc=ManualDoc(str(OUT),pagesize=A4,leftMargin=M,rightMargin=M,topMargin=54,bottomMargin=55,title=f'Rhythmer User Manual / v{VERSION}',author='Jazzy Rhythmer',subject='Sound into motion. Complete FM-1 firmware manual.')
doc.addPageTemplates([PageTemplate(id='Cover',frames=Frame(M,55,CW,H-110,id='cover'),onPage=cover),PageTemplate(id='Body',frames=Frame(M,55,CW,H-109,id='body',leftPadding=0,rightPadding=0,topPadding=0,bottomPadding=0),onPage=regular)])
toc=TableOfContents();toc.levelStyles=[styles['TOC']]
story=[Spacer(1,1),NextPageTemplate('Body'),PageBreak(),para('Find the button.<br/>Make the move.','Chapter'),para('Read the manual before your first session. Follow a chapter, try its controls, then return when you need the next move. This print edition contains the same sixteen chapters as the web guide, plus credits and useful links.'),para('Up to six assignable tracks are available. Fresh settings activate four; Chapter 03 explains how to enable six and confirm the CPU notice.'),para('<a href="'+WEB+'#guide">Open the searchable web manual</a> / <a href="'+WEB+'#play">Play in the browser</a>'),Spacer(1,12),toc]
sections=json.loads(DATA.read_text())
for ident,title,html in sections:
 story.append(PageBreak());p=para(escape(clean(title)),'Chapter');p._bookmarkName=ident;story.append(p);story.extend(blocks(DOM(html).root.children))
story.append(PageBreak());p=para('Credits & useful links','Chapter');p._bookmarkName='credits';story.append(p)
for h,t in [
 ('Built on shared work','Rhythmer is developed by Jazzy Rhythmer on JIANT\'s sequencing and system foundation. Felucca provides the platform; Hortator provides native Acid and MD percussion; X0X / TB-3PO contributes MIDI generation; Amenity contributes Break manipulation; Garden-FM and MONO2K contribute synthesis models; Felucca and OberVave contribute the two new synths; Elixir contributes SEQ generation and inspires the LXR-X kits. Original authors, source links and component licenses remain credited in the repository.'),
 ('Code and recordings are different','The source is GPL-3.0-only with retained component notices. Amen / The Winstons and Think / Lyn Collins are third-party recordings, separate from the code license. The owner directs their inclusion with credits; recording/composition redistribution permission is not documented. Credits or installation acknowledgements do not grant a sample license. Replace either audio bank with your own material or obtain rights appropriate to your use.'),
 ('Independent community firmware','Rhythmer is not affiliated with M-VAVE or Elektron. Product names belong to their owners. Follow the installer\'s backup, power, risk and audio-rights notices. This manual describes the current controls; it does not establish physical installation or recovery results for a package.')]:story.extend([para(h,'h2'),para(escape(t))])
for label,url in [('Project / manual',WEB+'#guide'),('Install / official V15 return',WEB+'installer/'),('Web editor',WEB+'editor/'),('Source / licenses','https://github.com/JazzyRhythmer/Rhythmer'),('Third-party credits','https://github.com/JazzyRhythmer/Rhythmer/blob/main/docs/THIRD_PARTY.md'),('Creator','https://jazzyrhythmer.github.io/')]:story.append(para('<b>'+label+'</b><br/><a href="'+url+'">'+url+'</a>'))
doc.multiBuild(story)
from pypdf import PdfReader
pdf=PdfReader(str(OUT));text='\n'.join(p.extract_text() for p in pdf.pages)
for phrase in ['STRAIGHT','MUTATE','RANDOM','LFO 2','CPU HIGH','Hortator','Source BPM']:
 assert phrase.lower() in text.lower(),phrase
assert len(pdf.pages)>16
assert len(sections)==16
report={'file':OUT.name,'pages':len(pdf.pages),'chapters':16,'edition':f'{VERSION} / {BUILD}','sha256':hashlib.sha256(OUT.read_bytes()).hexdigest(),'manualSourceSha256':hashlib.sha256((R/'site/src/manual.mjs').read_bytes()).hexdigest(),'links':sum(len(p.get('/Annots',[])) for p in pdf.pages),'scope':'Complete English print edition from current web manual chapters, with actual host-rendered UI and credits.'}
(QA/'pdf-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
