// Mockup viewer helper: moves <helmet> styles into <head>, adds a back link,
// and makes the gear (settings), i (info) and dropdown triggers open demo panels.
(function(){
var INFO={
 Cobalt:'Cobalt8M controller. Edits every Cobalt8 parameter from your DAW, saves edited patches with your project, syncs to DAW tempo. MIDI from the DAW passes through by default; loop guard stops echoes.',
 MODX:'MODX M6 controller. ESP-style editor for all 16 parts, FM-X, AN-X and AWM2 engines, effects, Motion, Smart Morph and library.',
 UNova:'UltraNova / MiniNova controller. Full patch editor and librarian over USB.',
 UltraNova:'UltraNova / MiniNova controller. Full patch editor and librarian over USB.',
 MLXD:'minilogue xd controller. Panel, menu-diving program edit, piano-roll sequencer with motion lanes, librarian and logue units loader.',
 Peak:'Peak controller. Panel, deep edit and patch librarian.',
 Main:'drumlogue controller. Sequencer, sounds, FX and mixer over USB MIDI.',Sound:'drumlogue controller.',FX:'drumlogue controller.',Mixer:'drumlogue controller.',
 MX400:'MX400 controller. Program editor for both effects, routing and surround depth. Use it as a DAW hardware insert.',
 Ebb:'Scaredy Cat. Draw a volume shape, it ducks your sound in time. Chain: Drive → Duck → Limiter.',
 Scratch:'Scratch Cat FX / Duo. Scratch, cue and DJ inside your DAW with any controller.',
 Stretch:'Stretch Cat. Sampler and track FX for time and pitch.',
 Copycat:'Copycat. Analyses a sound and gives a recipe to rebuild it on your synths.',
 Whisker:'WhiskerMix. Studio hub: mixer, routing, MIDI hub and Pulse 16 control in one app.',
 Settings:'Shared settings used by every plugin.',
 SupurrSaw:'SupurrSaw. Supersaw / super-square / hoover synth built from the HyperSaw engine, with a 303 mode.'
};
function pageKey(){var p=location.pathname.split('/').pop();for(var k in INFO){if(p.indexOf(k)===0)return k;}return 'Settings';}
function panel(html,id){var old=document.getElementById(id);if(old){old.remove();return;}
 var d=document.createElement('div');d.id=id;d.innerHTML=html;
 d.setAttribute('style','position:fixed;top:70px;right:24px;z-index:9998;width:380px;max-width:calc(100vw - 48px);background:#121316;color:#ECEAE4;border:1px solid #3A3D44;border-radius:10px;box-shadow:0 12px 40px rgba(0,0,0,.6);padding:14px 16px;font:13px/1.45 system-ui,sans-serif');
 d.addEventListener('click',function(e){if(e.target.dataset.close!==undefined)d.remove();});
 document.body.appendChild(d);}
var row=function(k,v){return '<div style="display:flex;justify-content:space-between;padding:5px 0;border-bottom:1px solid #26282D"><span>'+k+'</span><span style="color:#9FE3D9">'+v+'</span></div>';};
function settings(){panel('<div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:6px"><b style="font-size:15px">⚙ Settings</b><button data-close style="background:none;border:none;color:#aaa;font-size:18px;cursor:pointer">×</button></div>'
 +'<div style="font-size:11px;letter-spacing:.08em;color:#8E9198;margin-top:6px">CONNECTION</div>'+row('Device','Auto-detect ▾')+row('MIDI in / out','USB ▾')+row('MIDI from DAW','All on · loop guard')
 +'<div style="font-size:11px;letter-spacing:.08em;color:#8E9198;margin-top:10px">DISPLAY</div>'+row('Rendering','GPU ▾ (CPU fallback)')+row('Size','100% · drag any edge')+row('Frame rate','60 fps · pause when hidden')
 +'<div style="font-size:11px;letter-spacing:.08em;color:#8E9198;margin-top:10px">FILES</div>'+row('Presets folder','~/Nextcloud/Presets ▾')+row('Auto-backup','On, daily')
 +'<div style="font-size:11px;letter-spacing:.08em;color:#8E9198;margin-top:10px">HELP</div>'+row('Manual','Open ↗')+row('MIDI map','View')+row('Report a problem','Anonymized log'),'mk-settings');}
function info(){panel('<div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:6px"><b style="font-size:15px">ⓘ About this page</b><button data-close style="background:none;border:none;color:#aaa;font-size:18px;cursor:pointer">×</button></div><p style="margin:0 0 8px">'+INFO[pageKey()]+'</p><p style="margin:0;color:#8E9198">Hover any control for a tip. Full manual: Settings → Help.</p>','mk-info');}
document.addEventListener('DOMContentLoaded',function(){
 document.querySelectorAll('helmet').forEach(function(h){while(h.firstChild){document.head.appendChild(h.firstChild);}h.remove();});
 var a=document.createElement('a');a.href='../index.html';a.textContent='← All mockups';
 a.setAttribute('style','position:fixed;right:12px;bottom:12px;z-index:9999;padding:8px 12px;border-radius:8px;background:#111;color:#fff;font:600 13px system-ui,sans-serif;text-decoration:none;box-shadow:0 2px 8px rgba(0,0,0,.4)');
 document.body.appendChild(a);
 document.addEventListener('click',function(e){
  var t=e.target.closest('[data-open]');
  if(t){var p=document.querySelector('[data-pop="'+t.dataset.open+'"]');document.querySelectorAll('[data-pop]').forEach(function(x){if(x!==p)x.hidden=true;});if(p)p.hidden=!p.hidden;return;}
  if(!e.target.closest('[data-pop]'))document.querySelectorAll('[data-pop]').forEach(function(x){x.hidden=true;});
  var b=e.target.closest('button,a,span,div');if(!b)return;var txt=(b.textContent||'').trim(),lab=(b.getAttribute('aria-label')||'')+(b.getAttribute('title')||'');
  if(b.tagName==='BUTTON'&&(txt==='⚙'||/^⚙/.test(txt)||/^Settings/i.test(lab))){e.preventDefault();settings();}
  else if(b.tagName==='BUTTON'&&(txt==='i'||txt==='ⓘ'||/What this plugin does|Info/i.test(lab))){e.preventDefault();info();}
 });
});
})();

// Interactive knobs: drag up/down (or mouse wheel) to turn any knob, double-click resets, value shown while turning.
(function(){
 function init(){
  var tip=document.createElement('div');
  tip.setAttribute('style','position:fixed;z-index:10000;padding:2px 7px;border-radius:5px;background:#000;color:#fff;font:600 12px system-ui,sans-serif;pointer-events:none;display:none');
  document.body.appendChild(tip);
  var st=document.createElement('style');
  st.textContent='[data-knob]{cursor:ns-resize;touch-action:none}button:active{transform:translateY(1px)}';
  document.head.appendChild(st);
  document.querySelectorAll('div[style*="transform: rotate("]').forEach(function(rot){
   var host=rot.getAttribute('aria-hidden')==='true'&&/border-radius: 50%/.test(rot.getAttribute('style')||'')?rot:rot.parentElement;
   if(!host||host.getAttribute('aria-hidden')!=='true'||host.hasAttribute('data-knob'))return;
   if(host!==rot&&!/position: relative/.test(host.getAttribute('style')||''))return;
   var m=/rotate\((-?[\d.]+)deg\)/.exec(rot.getAttribute('style'));if(!m)return;
   var a0=parseFloat(m[1]),a=a0;
   host.setAttribute('data-knob','1');
   function set(v){a=Math.max(-135,Math.min(135,v));rot.style.transform='rotate('+a+'deg)';}
   function show(x,y){tip.textContent=Math.round((a+135)/270*100)+'%';tip.style.left=(x+14)+'px';tip.style.top=(y-28)+'px';tip.style.display='block';}
   host.addEventListener('pointerdown',function(e){
    e.preventDefault();host.setPointerCapture(e.pointerId);var y0=e.clientY,s0=a;
    show(e.clientX,e.clientY);
    function mv(ev){set(s0-(ev.clientY-y0)*1.8);show(ev.clientX,ev.clientY);}
    function up(){host.removeEventListener('pointermove',mv);host.removeEventListener('pointerup',up);tip.style.display='none';}
    host.addEventListener('pointermove',mv);host.addEventListener('pointerup',up);
   });
   host.addEventListener('wheel',function(e){e.preventDefault();set(a-e.deltaY*0.3);show(e.clientX,e.clientY);clearTimeout(host._t);host._t=setTimeout(function(){tip.style.display='none';},700);},{passive:false});
   host.addEventListener('dblclick',function(){set(a0);});
  });
 }
 if(document.readyState!=='loading')setTimeout(init,600);else document.addEventListener('DOMContentLoaded',function(){setTimeout(init,600);});
})();

// drumlogue Sound page: one part at a time (tabs), card flows into 3 columns
(function(){
 function init(){
  var g=document.querySelector('[data-parts]'); if(!g)return;
  var cards=[].slice.call(g.children).filter(function(c){return c.tagName==='SECTION';});
  var bar=document.createElement('div');
  bar.setAttribute('style','display:flex;gap:6px;margin:0 0 10px');
  function select(i){
   cards.forEach(function(c,j){
    var on=i===j; c.style.display=on?'block':'none';
    if(on){c.style.columnCount='3';c.style.columnGap='22px';c.style.minHeight='0';
     [].slice.call(c.children).forEach(function(k){k.style.breakInside='avoid';k.style.marginBottom='8px';var t=(k.textContent||'').trim();
      if(/^Parameters$/i.test(t)||/^Mix \/ Route$/i.test(t)){k.style.breakBefore='column';}
      if(/^PANEL KNOBS/i.test(t)){k.style.flexDirection='row';k.style.flexWrap='wrap';k.style.justifyContent='space-around';}});}
    var b=bar.children[j]; b.style.background=on?c.style.borderTopColor:'#1C1E21'; b.style.color=on?'#111':'#ECEAE4';
   });
  }
  cards.forEach(function(c,i){
   var b=document.createElement('button'); b.type='button';
   var t=(c.firstElementChild&&c.firstElementChild.textContent||'Part').trim().split(/\s+/)[0];
   b.textContent=t; b.setAttribute('style','flex:1;height:34px;border-radius:8px;border:1px solid #34373D;border-top:4px solid '+c.style.borderTopColor+';font:600 13px inherit;cursor:pointer;background:#1C1E21;color:#ECEAE4');
   b.onclick=function(){select(i);}; bar.appendChild(b);
  });
  var allB=document.createElement('button');allB.type='button';allB.textContent='\u25a6 Show all parts';allB.setAttribute('style','height:34px;padding:0 14px;border-radius:8px;border:1px solid #34373D;font:600 13px inherit;cursor:pointer;background:#1C1E21;color:#ECEAE4');
  var gs=g.getAttribute('style')||'',showAll=false;
  function toggleAll(){showAll=!showAll;allB.style.background=showAll?'#7FE3D6':'#1C1E21';allB.style.color=showAll?'#111':'#ECEAE4';
   if(showAll){g.style.display='grid';g.style.gridTemplateColumns='1fr 1fr';g.style.gap='14px';cards.forEach(function(c){c.style.display='block';c.style.columnCount='1';[].slice.call(c.children).forEach(function(k){k.style.breakBefore='auto';var t=(k.textContent||'').trim();if(/^PANEL KNOBS/i.test(t)){k.style.flexDirection='row';k.style.flexWrap='wrap';k.style.justifyContent='space-around';}});});}
   else{g.setAttribute('style',gs);select(0);}}
  allB.onclick=toggleAll;
  [].slice.call(bar.children).forEach(function(b){var o=b.onclick;b.onclick=function(){if(showAll){showAll=true;toggleAll();}o();};});
  bar.appendChild(allB);
  g.parentNode.insertBefore(bar,g); select(0);
 }
 if(document.readyState!=='loading')setTimeout(init,700);else document.addEventListener('DOMContentLoaded',function(){setTimeout(init,700);});
})();

// Copycat: tabs instead of one tall page, bigger text
(function(){
 function init(){
  if(!/Copycat/.test(document.title))return;
  var root=document.querySelector('div[style*="width: 1100px"]'); if(!root)return;
  var secs=[].slice.call(root.querySelectorAll('section'));
  function find(re){return secs.filter(function(x){var t=(x.textContent||'').trim().slice(0,40).toLowerCase();return re.test(t);})[0];}
  var S={cap:find(/^captured sound/),env:find(/^envelope/),mus:find(/^musical/),syn:find(/^synthesis/),fx:find(/^effects detected/),eq:find(/^frequency profile/),rec:find(/^recipe/)};
  if(!S.cap||!S.mus||!S.rec)return;
  var host=document.createElement('div'); host.style.cssText='zoom:1.08;margin-top:10px';
  var bar=document.createElement('div'); bar.style.cssText='display:flex;gap:6px;margin-top:10px';
  var tabs=[['Sound',[S.cap,S.mus]],['Synth guess',[S.syn,S.fx]],['Envelope & filter',[S.env]],['EQ curve',[S.eq]],['Recipes',[S.rec]]];
  var panes=[];
  tabs.forEach(function(t,i){
   var pane=document.createElement('div'); pane.style.cssText='display:'+(t[1].length>1?'grid':'block')+';grid-template-columns:1fr 1fr;gap:12px;align-items:start;display:none';
   if(i===0){
    var rc=document.createElement('div'); rc.style.cssText='display:flex;flex-direction:column;gap:12px;min-width:0';
    t[1].forEach(function(e,k){if(!e)return;e.style.margin='0';if(k===0)pane.appendChild(e);else rc.appendChild(e);});
    if(S.eq&&S.env&&S.mus){
     var sv=[S.eq.querySelector('svg')].concat([].slice.call(S.env.querySelectorAll('svg')).slice(0,2));
     var box=S.mus.cloneNode(false); box.style.margin='0'; box.innerHTML='';
     var ttl=S.mus.firstElementChild.cloneNode(true); ttl.textContent='SOUND SHAPE · EQ, ENVELOPE, FILTER'; ttl.style.marginBottom='8px'; box.appendChild(ttl);
     var g=document.createElement('div'); g.style.cssText='display:grid;grid-template-columns:1.6fr 1fr 1fr;gap:10px;align-items:start';
     ['EQ CURVE','AMP ENVELOPE','FILTER'].forEach(function(n,k){
      if(!sv[k])return; var w=document.createElement('div'); w.style.cssText='min-width:0';
      var l=document.createElement('div'); l.textContent=n; l.style.cssText='font-size:10px;letter-spacing:0.1em;color:#B4B1AA;margin-bottom:4px';
      var c=sv[k].cloneNode(true); c.style.cssText='width:100%;height:104px;display:block;border-radius:8px';c.setAttribute('preserveAspectRatio','none');
      w.appendChild(l); w.appendChild(c); g.appendChild(w);
     });
     box.appendChild(g);
     var rt=document.createElement('div'); rt.textContent='Rebuild: saw, 7 voices, ±18 cents · LP 24 dB at 2.1 kHz · A 8 ms, D 300 ms, S 70%, R 450 ms · HP 40 Hz, bell +3 dB at 90 Hz';
     rt.style.cssText='margin-top:8px;font-size:12px;line-height:1.35;color:#B4B1AA'; box.appendChild(rt); rc.appendChild(box);
    }
    pane.appendChild(rc);
   } else
   t[1].forEach(function(e){if(e){e.style.margin='0';pane.appendChild(e);}});
   host.appendChild(pane); panes.push(pane);
   var b=document.createElement('button'); b.type='button'; b.textContent=t[0];
   b.style.cssText='height:34px;padding:0 16px;border-radius:8px;border:1px solid #4A4E57;background:#34373E;color:#ECEAE4;font:600 14px inherit;cursor:pointer';
   b.onclick=function(){panes.forEach(function(p,j){p.style.display=j===i?(tabs[j][1].length>1?'grid':'block'):'none';bar.children[j].style.background=j===i?'#F6A970':'#34373E';bar.children[j].style.color=j===i?'#111':'#ECEAE4';});};
   bar.appendChild(b);
  });
  // remove the old grid wrapper (now empty), insert bar and host after the Instant copy section
  var ic=find(/^instant copy/); var anchor=ic||secs[0];
  var grid=S.cap.parentNode; 
  anchor.parentNode.insertBefore(bar,anchor.nextSibling); bar.parentNode.insertBefore(host,bar.nextSibling);
  [].slice.call(root.children).forEach(function(c){if(c.tagName==='DIV'&&c!==host&&c!==bar&&c.children.length===0&&c.style.display!=='contents'){}});
  bar.children[0].click();
 }
 if(document.readyState!=='loading')setTimeout(init,800);else document.addEventListener('DOMContentLoaded',function(){setTimeout(init,800);});
})();

// ===== Interactive layer: toggles, radio groups, checkboxes, dropdowns, value cells, faders, XY pads =====
(function(){
 var SW=/^(background|color$|font-weight|box-shadow)/;
 function props(el){var m={};for(var i=0;i<el.style.length;i++){var n=el.style[i];if(SW.test(n))m[n]=el.style.getPropertyValue(n);}return m;}
 function copyProps(from,to){var pf=props(from),pt=props(to);Object.keys(pt).forEach(function(n){if(!(n in pf))to.style.removeProperty(n);});Object.keys(pf).forEach(function(n){to.style.setProperty(n,pf[n]);});}
 function swapProps(a,b){var pa=props(a),pb=props(b);var all={};Object.keys(pa).concat(Object.keys(pb)).forEach(function(n){all[n]=1;});
  Object.keys(all).forEach(function(n){var va=pa[n]||'',vb=pb[n]||'';if(va===vb)return;
   if(vb)a.style.setProperty(n,vb);else a.style.removeProperty(n);if(va)b.style.setProperty(n,va);else b.style.removeProperty(n);});}
 function sig(el){return (el.style.background||el.style.backgroundColor||'')+'|'+(el.style.color||'');}
 function isCtl(el){return el.tagName==='BUTTON';}
 function skipBtn(b){return b.disabled||b.onclick||b.hasAttribute('data-open')||b.getAttribute('aria-label')||b.getAttribute('title')||b.closest('[data-pop]')||b.hasAttribute('data-nosel');}
 function mark(el,on){if(on){el.setAttribute('data-sel','1');el.style.boxShadow='inset 0 0 0 2px #7FE3D6, 0 0 10px rgba(127,227,214,.5)';el.style.filter='brightness(1.35)';}
  else{el.removeAttribute('data-sel');el.style.boxShadow='';el.style.filter='';}}
 // dropdown menu
 var menu=null;
 function closeMenu(){if(menu){menu.remove();menu=null;}}
 function openMenu(el,items,onpick){closeMenu();menu=document.createElement('div');
  var r=el.getBoundingClientRect();
  menu.setAttribute('style','position:fixed;z-index:10001;left:'+Math.max(4,Math.min(r.left,innerWidth-190))+'px;top:'+(r.bottom+2)+'px;min-width:'+Math.max(120,r.width)+'px;max-height:260px;overflow:auto;background:#121316;color:#ECEAE4;border:1px solid #4A4E57;border-radius:8px;box-shadow:0 10px 30px rgba(0,0,0,.6);padding:4px;font:13px system-ui,sans-serif');
  items.forEach(function(t){var o=document.createElement('div');o.textContent=t;o.setAttribute('style','padding:6px 10px;border-radius:5px;cursor:pointer;white-space:nowrap');
   o.onmouseenter=function(){o.style.background='#2B3038';};o.onmouseleave=function(){o.style.background='';};
   o.onclick=function(e){e.stopPropagation();onpick(t);closeMenu();};menu.appendChild(o);});
  document.body.appendChild(menu);}
 var DEF={};
 function pool(el){var key=el.getAttribute('style')||'';var out=[];var seen={};
  document.querySelectorAll('[style]').forEach(function(x){if(x.getAttribute('style')!==key||x.children.length>0)return;var t=x.textContent.trim().replace(/\s*[▾▼⌄]\s*$/,'');if(t&&t.length<30&&!seen[t]){seen[t]=1;out.push(t);}});
  return out;}
 function cur(el){return el.textContent.trim().replace(/\s*[▾▼⌄]\s*$/,'');}
 function setText(el,t){var had=/[▾▼⌄]\s*$/.test(el.textContent.trim());
  var node=null;for(var i=el.childNodes.length-1;i>=0;i--){if(el.childNodes[i].nodeType===3&&el.childNodes[i].textContent.trim()){node=el.childNodes[i];break;}}
  if(node)node.textContent=t+(had?' ▾':'');else el.textContent=t+(had?' ▾':'');}
 var PRE=[['On','Off'],['Auto','Manual'],['Stereo','Mono'],['None','Low','Mid','High']];
 function numParse(t){var m=/^([+\-−]?)(\d+(?:\.\d+)?)(.*)$/.exec(t.trim());if(!m)return null;return {sign:m[1]==='−'?'-':m[1],num:parseFloat(m[2]),rest:m[3],dec:(m[2].split('.')[1]||'').length};}
 function numFmt(p,v){var s=v<0?'-':(p.sign==='+'?'+':'');return s+Math.abs(v).toFixed(p.dec)+p.rest;}
 function isValueCell(el){if(!el||el.children.length>0||el.closest('button,a,[data-pop],[data-own]'))return false;var t=el.textContent.trim();if(!t||t.length>24)return false;
  var ff=(el.style.fontFamily||'')+(el.getAttribute('style')||'');if(!/VT323|monospace|Plex Mono/i.test(ff))return false;var r=el.getBoundingClientRect();return r.height<=44&&r.width>=14;}
 function checkBox(row){var best=null;row.querySelectorAll('span,div,i,b').forEach(function(x){if(best||x.closest('[data-knob]'))return;var r=x.getBoundingClientRect();if(r.width>=10&&r.width<=24&&Math.abs(r.width-r.height)<2&&x.children.length<=1)best=x;});return best;}
 function isFilled(box){var c=getComputedStyle(box).backgroundColor.match(/[\d.]+/g)||[0,0,0,0];var a=c.length>3?parseFloat(c[3]):1;if(a<0.1)return false;var r=+c[0],g=+c[1],b=+c[2];return (Math.max(r,g,b)-Math.min(r,g,b)>40)||(r+g+b)/3>150;}

 document.addEventListener('click',function(e){
  var t=e.target; if(!t||!t.closest)return;
  if(menu&&!menu.contains(t))closeMenu();
  if(t.closest('[data-pop],a,input,textarea,select,[data-own]'))return;
  // unicode checkboxes
  var u=t; for(var k=0;k<3&&u&&u!==document.body;k++,u=u.parentElement){
   if(u.children.length===0&&/[☐☑]/.test(u.textContent)&&u.textContent.length<40){u.textContent=/☐/.test(u.textContent)?u.textContent.replace('☐','☑'):u.textContent.replace('☑','☐');return;}}
  // custom checkbox rows
  var row=t; for(var j=0;j<3&&row&&row!==document.body;j++,row=row.parentElement){
   if(row.tagName==='BODY')break; var bx=checkBox(row); if(bx&&row.getBoundingClientRect().height<70){
    if(isFilled(bx)){bx.dataset.bg=getComputedStyle(bx).backgroundColor;bx.style.background='transparent';bx.dataset.on='0';[].forEach.call(bx.children,function(c){c.style.visibility='hidden';});}
    else{bx.style.background=bx.dataset.bg||'#4FD1C5';bx.dataset.on='1';[].forEach.call(bx.children,function(c){c.style.visibility='visible';});}
    return;}}
  var b=t.closest('button')||t; var el=t.closest('button');
  if(!el){var p=t;for(var q=0;q<4&&p&&p!==document.body;q++,p=p.parentElement){if(getComputedStyle(p).cursor==='pointer'&&!p.closest('a')&&p.tagName!=='svg'&&!p.hasAttribute('data-knob')){el=p;break;}}}
  if(!el)return;
  if(el.tagName==='BUTTON'&&skipBtn(el))return;
  var txt=el.textContent.trim();
  // dropdown
  if(/[▾▼⌄]\s*$/.test(txt)){var items=pool(el);var c0=cur(el);if(items.indexOf(c0)<0)items.unshift(c0);
   if(items.length<3){PRE.forEach(function(g){if(g.indexOf(c0)>=0)items=g.slice();});}
   if(items.length<2)items=[c0,'Option 2','Option 3'];
   openMenu(el,items,function(v){setText(el,v);});return;}
  // radio / toggle groups
  var P=el.parentElement; if(!P)return;
  var sibs=[].filter.call(P.children,function(x){return x.tagName===el.tagName&&(isCtl(x)?!skipBtn(x):getComputedStyle(x).cursor==='pointer');});
  if(sibs.length<2){ if(el.hasAttribute('data-sel'))mark(el,false);else mark(el,true); return;}
  var map={};sibs.forEach(function(x){var s=sig(x);(map[s]=map[s]||[]).push(x);});
  var keys=Object.keys(map);
  if(keys.length===1){sibs.forEach(function(x){if(x!==el)mark(x,false);});mark(el,!el.hasAttribute('data-sel'));return;}
  keys.sort(function(a,b){return map[b].length-map[a].length;});
  var minority=sibs.length-map[keys[0]].length;
  if(sibs.length<12&&minority===1&&!(map[keys[0]].length===1)){var act=map[keys[keys.length-1]][0];if(act!==el)swapProps(el,act);return;}
  var mine=sig(el),other=keys[0]===mine?keys[1]:keys[0];copyProps(map[other][0],el);
 },true);

 // numeric value cells: drag to change; others click for menu
 document.addEventListener('pointerdown',function(e){
  var el=e.target; if(!isValueCell(el))return; var p0=numParse(el.textContent); if(!p0)return;
  e.preventDefault();var y0=e.clientY,v0=(p0.sign==='-'?-1:1)*p0.num;var step=p0.dec?0.1:1;if(/%|dB/.test(p0.rest)&&!p0.dec)step=1;
  function mv(ev){var v=v0+Math.round((y0-ev.clientY)/4)*step;el.textContent=numFmt({sign:p0.sign==='+'&&v>=0?'+':'',dec:p0.dec,rest:p0.rest},v);}
  function up(){removeEventListener('pointermove',mv);removeEventListener('pointerup',up);}
  addEventListener('pointermove',mv);addEventListener('pointerup',up);
 });
 document.addEventListener('click',function(e){var el=e.target;if(!isValueCell(el)||numParse(el.textContent))return;
  var items=pool(el);var c0=cur(el);if(items.indexOf(c0)<0)items.unshift(c0);if(items.length<2)return;openMenu(el,items,function(v){setText(el,v);});},false);

 // faders, XY pads
 function centerTo(kid,track,x,y){var tr=track.getBoundingClientRect();kid.style.right='auto';kid.style.bottom='auto';kid.style.left='0px';kid.style.top='0px';
  var kr=kid.getBoundingClientRect();var cx=kr.left+kr.width/2-tr.left,cy=kr.top+kr.height/2-tr.top;
  if(x!==null)kid.style.left=(x-cx)+'px'; else kid.style.left='';
  if(y!==null)kid.style.top=(y-cy)+'px';}
 function initFaders(){
  document.querySelectorAll('div,span').forEach(function(e){
   if(e.hasAttribute('data-fd')||e.closest('[data-knob]')||e.closest('button,a')||e.closest('[data-own]'))return;
   var cs=getComputedStyle(e); if(cs.position==='static')return;
   var r=e.getBoundingClientRect(); if(r.width<4||r.height<4)return;
   var vert=r.height>=2*r.width&&r.width<=44&&r.height>=36&&r.height<=420, hor=r.width>=2*r.height&&r.height<=44&&r.width>=50&&r.width<=700;
   if(!vert&&!hor)return;
   var kids=[].filter.call(e.children,function(k){return getComputedStyle(k).position==='absolute';});
   var thumb=null,fills=[];
   kids.forEach(function(k){var kr=k.getBoundingClientRect();var along=vert?kr.height:kr.width,across=vert?kr.width:kr.height;
    if(along>=6&&along<=36&&across>=Math.min(6,(vert?r.width:r.height)*0.5)&&!thumb&&(vert?kr.width>=6:kr.height>=6)&&(along<(vert?r.height:r.width)*0.5))thumb=k;
    else if(along>=(vert?r.height:r.width)*0.1&&across>=3&&across>=(vert?r.width:r.height)*0.4&&/bottom:\s*0|left:\s*0/.test(k.getAttribute('style')||'')&&!/top:\s*0/.test(k.getAttribute('style')||'')||(!thumb&&/(height|width):\s*\d+%/.test(k.getAttribute('style')||'')&&/bottom:\s*0/.test(k.getAttribute('style')||'')))fills.push(k);});
   if(!thumb&&!fills.length)return;
   e.setAttribute('data-fd','1');e.style.cursor=vert?'ns-resize':'ew-resize';e.style.touchAction='none';var _i0=[thumb].concat(fills).map(function(x){return x?x.getAttribute('style'):null;});e.addEventListener('dblclick',function(){[thumb].concat(fills).forEach(function(x,i){if(x&&_i0[i]!==null)x.setAttribute('style',_i0[i]);});});
   function set(ev){var tr=e.getBoundingClientRect();var x=Math.max(0,Math.min(tr.width,ev.clientX-tr.left)),y=Math.max(0,Math.min(tr.height,ev.clientY-tr.top));
    if(thumb)centerTo(thumb,e,vert?null:x,vert?y:null);
    fills.forEach(function(f){if(vert){f.style.height=(tr.height-y)+'px';}else{f.style.width=x+'px';}});}
   e.addEventListener('pointerdown',function(ev){ev.preventDefault();e.setPointerCapture(ev.pointerId);set(ev);
    function mv(m){set(m);}function up(){e.removeEventListener('pointermove',mv);e.removeEventListener('pointerup',up);}
    e.addEventListener('pointermove',mv);e.addEventListener('pointerup',up);});
  });
  document.querySelectorAll('div').forEach(function(e){
   if(e.hasAttribute('data-xy')||e.closest('[data-knob]')||e.closest('button')||e.closest('[data-own]'))return;
   var r=e.getBoundingClientRect();if(r.width<70||r.height<50||r.width>520||r.height>400)return;
   var dot=null;[].forEach.call(e.children,function(k){var kr=k.getBoundingClientRect();if(!dot&&getComputedStyle(k).position==='absolute'&&kr.width>=6&&kr.width<=24&&Math.abs(kr.width-kr.height)<2&&/50%|999|100%/.test(getComputedStyle(k).borderRadius))dot=k;});
   if(!dot)return;e.setAttribute('data-xy','1');e.style.cursor='crosshair';e.style.touchAction='none';
   function set(ev){var tr=e.getBoundingClientRect();centerTo(dot,e,Math.max(0,Math.min(tr.width,ev.clientX-tr.left)),Math.max(0,Math.min(tr.height,ev.clientY-tr.top)));}
   e.addEventListener('pointerdown',function(ev){ev.preventDefault();e.setPointerCapture(ev.pointerId);set(ev);function mv(m){set(m);}function up(){e.removeEventListener('pointermove',mv);e.removeEventListener('pointerup',up);}e.addEventListener('pointermove',mv);e.addEventListener('pointerup',up);});
  });
 }
 var st=document.createElement('style');st.textContent='[data-vt] [data-sel]{box-shadow:0 1px 2px #000!important;filter:none!important}button,[style*="cursor: pointer"]{user-select:none}button:focus-visible{outline:2px solid #7FE3D6;outline-offset:1px}';document.head.appendChild(st);
 function go(){setTimeout(initFaders,1200);} if(document.readyState!=='loading')go();else document.addEventListener('DOMContentLoaded',go);
})();

// ===== SVG arc knobs and SVG point editors (SupurrSaw style) =====
(function(){
 function pt(svg,ev){var p=svg.createSVGPoint();p.x=ev.clientX;p.y=ev.clientY;return p.matrixTransform(svg.getScreenCTM().inverse());}
 function initArcKnobs(){
  document.querySelectorAll('svg').forEach(function(svg){
   if(svg.hasAttribute('data-aknob')||svg.closest('[data-own]'))return;
   var circ=svg.querySelector('circle'),path=svg.querySelector('path[d*=" A"]'),line=svg.querySelector('line');
   if(!circ||!path||!line||svg.querySelectorAll('circle').length>2)return;
   var cx=parseFloat(circ.getAttribute('cx')),cy=parseFloat(circ.getAttribute('cy')),r=parseFloat(circ.getAttribute('r'));
   var d=path.getAttribute('d');var m=/^M([\d.\-]+),([\d.\-]+)/.exec(d);if(!m)return;
   var x2=parseFloat(line.getAttribute('x2')),y2=parseFloat(line.getAttribute('y2'));
   var a=Math.atan2(x2-cx,-(y2-cy))*180/Math.PI,a0=a,sx=m[1],sy=m[2];
   var lab=svg.nextElementSibling;var num=lab&&/^[+\-−]?\d/.test(lab.textContent.trim())?lab:null;var np=null,v0=0;
   if(num){var mm=/^([+\-−]?)(\d+(?:\.\d+)?)(.*)$/.exec(num.textContent.trim());if(mm){np={sign:mm[1],rest:mm[3],dec:(mm[2].split('.')[1]||'').length};v0=parseFloat(mm[2])*(mm[1]==='-'||mm[1]==='−'?-1:1);}}
   var a_start=(a0+135)/270;
   function render(){var rad=a*Math.PI/180,ex=cx+r*Math.sin(rad),ey=cy-r*Math.cos(rad);
    path.setAttribute('d','M'+sx+','+sy+' A'+r+','+r+' 0 '+((a+135>180)?1:0)+' 1 '+ex.toFixed(1)+','+ey.toFixed(1));
    line.setAttribute('x2',ex.toFixed(1));line.setAttribute('y2',ey.toFixed(1));
    if(np){var f=(a+135)/270;var v;if(/%/.test(np.rest))v=f*100;else if(np.rest===''&&v0>=0&&v0<=127&&!np.dec)v=Math.round(f*127);else{var span=Math.max(Math.abs(v0)*2,10);v=v0+(f-a_start)*span;}
     var s=v<0?'-':(np.sign==='+'?'+':'');num.textContent=s+Math.abs(v).toFixed(np.dec)+np.rest;}}
   svg.setAttribute('data-aknob','1');svg.style.cursor='ns-resize';svg.style.touchAction='none';
   svg.addEventListener('pointerdown',function(ev){ev.preventDefault();svg.setPointerCapture(ev.pointerId);var y0=ev.clientY,s0=a;
    function mv(e){a=Math.max(-135,Math.min(135,s0-(e.clientY-y0)*1.8));render();}
    function up(){svg.removeEventListener('pointermove',mv);svg.removeEventListener('pointerup',up);}
    svg.addEventListener('pointermove',mv);svg.addEventListener('pointerup',up);});
   svg.addEventListener('wheel',function(e){e.preventDefault();a=Math.max(-135,Math.min(135,a-e.deltaY*0.3));render();},{passive:false});
   svg.addEventListener('dblclick',function(){a=a0;render();});
  });
 }
 function initPoints(){
  document.querySelectorAll('svg').forEach(function(svg){
   var pl=svg.querySelector('polyline');if(!pl||svg.hasAttribute('data-pts')||svg.closest('[data-own]'))return;
   var circs=[].slice.call(svg.querySelectorAll('circle'));if(circs.length<2)return;
   var pts=pl.getAttribute('points').trim().split(/\s+/).map(function(s){var p=s.split(',');return [parseFloat(p[0]),parseFloat(p[1])];});
   var map=[];circs.forEach(function(c){var x=parseFloat(c.getAttribute('cx')),y=parseFloat(c.getAttribute('cy'));
    for(var i=0;i<pts.length;i++){if(Math.abs(pts[i][0]-x)<0.6&&Math.abs(pts[i][1]-y)<0.6){map.push([c,i]);break;}}});
   if(map.length<2)return;svg.setAttribute('data-pts','1');
   var vb=svg.viewBox.baseVal;
   map.forEach(function(e){var c=e[0],i=e[1];c.style.cursor='grab';c.style.touchAction='none';
    c.addEventListener('pointerdown',function(ev){ev.preventDefault();ev.stopPropagation();c.setPointerCapture(ev.pointerId);
     function mv(m){var p=pt(svg,m);var lo=i>0?pts[i-1][0]:vb.x,hi=i<pts.length-1?pts[i+1][0]:vb.x+vb.width;
      var x=Math.max(lo,Math.min(hi,p.x)),y=Math.max(vb.y,Math.min(vb.y+vb.height,p.y));
      if(i===0||i===pts.length-1)x=pts[i][0];
      pts[i]=[x,y];c.setAttribute('cx',x.toFixed(1));c.setAttribute('cy',y.toFixed(1));
      pl.setAttribute('points',pts.map(function(q){return q[0].toFixed(1)+','+q[1].toFixed(1);}).join(' '));
      var fill=svg.querySelector('polygon');if(fill&&fill.getAttribute('points')){/* leave fill */}}
     function up(){c.removeEventListener('pointermove',mv);c.removeEventListener('pointerup',up);}
     c.addEventListener('pointermove',mv);c.addEventListener('pointerup',up);});});
  });
 }
 function go(){setTimeout(function(){initArcKnobs();initPoints();},1300);}
 if(document.readyState!=='loading')go();else document.addEventListener('DOMContentLoaded',go);
})();

// ===== Switch pills and progress bars =====
(function(){
 function isSwitch(e){if(!e||e.children.length!==1)return false;var r=e.getBoundingClientRect(),k=e.children[0],kr=k.getBoundingClientRect();
  return r.width>=22&&r.width<=64&&r.height>=10&&r.height<=30&&kr.width>=8&&Math.abs(kr.width-kr.height)<2&&/50%|99|100%/.test(getComputedStyle(k).borderRadius)&&r.width>kr.width*1.4&&!e.closest('[data-knob]');}
 document.addEventListener('click',function(ev){
  var e=ev.target.closest&&ev.target.closest('span,div,label,button');
  for(var i=0;i<3&&e;i++,e=e.parentElement){
   if(!isSwitch(e))continue;
   var k=e.children[0],r=e.getBoundingClientRect(),kr=k.getBoundingClientRect();
   if(!e.hasAttribute('data-sw')){e.setAttribute('data-sw',(kr.left+kr.width/2>r.left+r.width/2)?'1':'0');e.dataset.onbg=getComputedStyle(e).backgroundColor;e.dataset.x=0;}
   var onNow=(kr.left+kr.width/2>r.left+r.width/2);
   var rep=null;[].forEach.call(document.querySelectorAll('span,div'),function(x){if(rep||x===e||!isSwitch(x))return;var xr=x.getBoundingClientRect(),xk=x.children[0].getBoundingClientRect();
     var xon=(xk.left+xk.width/2>xr.left+xr.width/2);if(xon!==onNow&&Math.abs(xr.width-r.width)<2&&Math.abs(xr.height-r.height)<2&&!x.hasAttribute('data-sw'))rep=x;});
   if(rep){var rs=rep.getAttribute('style'),ks=rep.children[0].getAttribute('style');e.setAttribute('style',rs);k.setAttribute('style',ks);ev.stopPropagation();ev.preventDefault();return;}
   var on=e.getAttribute('data-sw')==='1',dist=r.width-kr.width-((r.width-kr.width)>8?6:2);
   var x=parseFloat(e.dataset.x)||0;x+=on?-dist:dist;e.dataset.x=x;k.style.transition='transform .12s';k.style.transform='translateX('+x+'px)';
   e.style.background=on?'#3A3D44':e.dataset.onbg;e.setAttribute('data-sw',on?'0':'1');ev.stopPropagation();ev.preventDefault();return;}
 },true);
 function initBars(){
  document.querySelectorAll('div,span').forEach(function(e){
   if(e.hasAttribute('data-bar')||e.hasAttribute('data-fd')||e.closest('[data-knob],button,a'))return;
   if(e.children.length!==1)return;var k=e.children[0];var ks=k.getAttribute('style')||'';
   var m=/(?:^|;|\s)width:\s*([\d.]+)%/.exec(ks);if(!m||k.children.length)return;
   var r=e.getBoundingClientRect();if(r.height>18||r.height<3||r.width<40)return;
   e.setAttribute('data-bar','1');e.style.cursor='ew-resize';e.style.touchAction='none';
   function set(ev){var tr=e.getBoundingClientRect();k.style.width=Math.round(Math.max(0,Math.min(1,(ev.clientX-tr.left)/tr.width))*100)+'%';}
   e.addEventListener('pointerdown',function(ev){ev.preventDefault();e.setPointerCapture(ev.pointerId);set(ev);function mv(x){set(x);}function up(){e.removeEventListener('pointermove',mv);e.removeEventListener('pointerup',up);}e.addEventListener('pointermove',mv);e.addEventListener('pointerup',up);});
  });
 }
 function go(){setTimeout(initBars,1400);}if(document.readyState!=='loading')go();else document.addEventListener('DOMContentLoaded',go);
})();

// ===== Cobalt style SVG knobs (ring, arc, dot, number inside) =====
(function(){
 function init(){
  document.querySelectorAll('svg').forEach(function(svg){
   if(svg.hasAttribute('data-aknob'))return;
   var path=svg.querySelector('path[d*=" A"]'),txt=svg.querySelector('text'),cs=svg.querySelectorAll('circle');
   if(!path||!txt||cs.length<2||svg.querySelector('line'))return;
   var ring=cs[0],dot=cs[cs.length-1];var cx=+ring.getAttribute('cx'),cy=+ring.getAttribute('cy'),r=+ring.getAttribute('r');
   if(+dot.getAttribute('r')>4)return;
   var m=/^M([\d.\-]+),([\d.\-]+)/.exec(path.getAttribute('d'));if(!m)return;
   var dx=+dot.getAttribute('cx')-cx,dy=+dot.getAttribute('cy')-cy,rd=Math.hypot(dx,dy);
   var a=Math.atan2(dx,-dy)*180/Math.PI,a0=a,sx=m[1],sy=m[2];var t0=txt.textContent;
   var isNum=/^-?\d+$/.test(t0.trim());var v0=parseInt(t0,10);
   function render(){var rad=a*Math.PI/180,ex=cx+r*Math.sin(rad),ey=cy-r*Math.cos(rad);
    path.setAttribute('d','M'+sx+','+sy+' A'+r+','+r+' 0 '+((a+135>180)?1:0)+' 1 '+ex.toFixed(1)+','+ey.toFixed(1));
    dot.setAttribute('cx',(cx+rd*Math.sin(rad)).toFixed(1));dot.setAttribute('cy',(cy-rd*Math.cos(rad)).toFixed(1));
    if(isNum){var f=(a+135)/270;txt.textContent=(v0<0||/-/.test(t0))?Math.round(f*254-127):Math.round(f*127);}}
   svg.setAttribute('data-aknob','1');svg.style.cursor='ns-resize';svg.style.touchAction='none';
   svg.addEventListener('pointerdown',function(ev){ev.preventDefault();svg.setPointerCapture(ev.pointerId);var y0=ev.clientY,s0=a;
    function mv(e){a=Math.max(-135,Math.min(135,s0-(e.clientY-y0)*1.8));render();}
    function up(){svg.removeEventListener('pointermove',mv);svg.removeEventListener('pointerup',up);}
    svg.addEventListener('pointermove',mv);svg.addEventListener('pointerup',up);});
   svg.addEventListener('wheel',function(e){e.preventDefault();a=Math.max(-135,Math.min(135,a-e.deltaY*0.3));render();},{passive:false});
   svg.addEventListener('dblclick',function(){a=a0;txt.textContent=t0;render();txt.textContent=t0;});
  });
  // joystick: svg with crosshair lines and a dot
  document.querySelectorAll('svg').forEach(function(svg){});
 }
 function go(){setTimeout(init,1400);}if(document.readyState!=='loading')go();else document.addEventListener('DOMContentLoaded',go);
})();

// ===== minilogue xd behaviours: LCD readout, waveform, toggles with stems, step lights, joystick; ctrl/cmd-click reset everywhere =====
(function(){
 document.addEventListener('pointerdown',function(e){
  if(!(e.ctrlKey||e.metaKey))return;
  var t=e.target;if(!t||!t.closest||!t.closest('[data-knob],[data-fd],svg,[data-xy]'))return;
  e.preventDefault();e.stopImmediatePropagation();
  t.dispatchEvent(new MouseEvent('dblclick',{bubbles:true,cancelable:true}));
 },true);
 function lcdInit(){
  var svg=[].filter.call(document.querySelectorAll('svg[aria-label^="Oscilloscope"]'),function(s){return true;})[0];
  var lcd=svg?svg.parentElement:null;
  var ov=null,timer=null;
  function show(name,val){if(!lcd)return;if(!ov){lcd.style.position='relative';ov=document.createElement('div');
    ov.setAttribute('style','position:absolute;inset:0;display:none;flex-direction:column;align-items:center;justify-content:center;background:#050608;color:#EAF2FF;font:22px VT323,monospace;letter-spacing:.06em;text-align:center;line-height:1.1;pointer-events:none');lcd.appendChild(ov);}
   ov.innerHTML='<div style="font-size:16px;color:#9DB6D9">'+name+'</div><div style="font-size:34px">'+val+'</div>';ov.style.display='flex';
   clearTimeout(timer);timer=setTimeout(function(){ov.style.display='none';},1400);}
  window.__lcdShow=show;
  var P=22;
  function wave(kind){if(!svg)return;var pl=svg.querySelector('polyline');if(!pl)return;var pts=[],per=46,x;
   for(x=4;x<=136;x+=2){var ph=((x-4)%per)/per,y;
    if(kind==='saw')y=63-ph*45;else if(kind==='tri')y=ph<.5?63-ph*2*45:18+(ph-.5)*2*45;else y=ph<.5?18:63;
    pts.push(x+','+y.toFixed(1));}
   pl.setAttribute('points',pts.join(' '));svg.setAttribute('aria-label','Oscilloscope: '+kind);}
  // knobs -> LCD value
  var down=null;
  document.addEventListener('pointerdown',function(e){var h=e.target.closest&&e.target.closest('[data-knob]');down=h||null;},true);
  document.addEventListener('pointermove',function(e){if(!down||!(e.buttons&1))return;
   var rot=down.querySelector('[style*="rotate("]')||down;var m=/rotate\((-?[\d.]+)deg\)/.exec(rot.getAttribute('style')||'');if(!m)return;
   var v=Math.round((parseFloat(m[1])+135)/270*100);var p=down.parentElement,lab='';
   var sp=p?p.querySelectorAll('span'):[];if(sp.length)lab=sp[sp.length-1].textContent.trim();
   show(lab||'VALUE',v);});
  // vertical toggles
  document.querySelectorAll('div').forEach(function(t){
   var r=t.getBoundingClientRect();if(r.width<14||r.width>20||r.height<30||r.height>60)return;
   var kids=[].filter.call(t.children,function(k){return getComputedStyle(k).position==='absolute';});if(kids.length!==2||t.hasAttribute('data-vt'))return;
   var cap=null,stem=null;kids.forEach(function(k){var kr=k.getBoundingClientRect();if(Math.abs(kr.width-kr.height)<2&&kr.width>=8)cap=k;else if(kr.width<=6)stem=k;});if(!cap||!stem)return;
   t.setAttribute('data-vt','1');t.style.cursor='pointer';
   var labs=t.nextElementSibling&&t.nextElementSibling.querySelectorAll?t.nextElementSibling.querySelectorAll('span'):[];
   if(!labs.length&&t.parentElement&&t.parentElement.nextElementSibling)labs=t.parentElement.nextElementSibling.querySelectorAll?[]:[];
   var n=labs.length>=2?labs.length:2,H=r.height,ch=cap.getBoundingClientRect().height,sw=stem.getBoundingClientRect().width;
   function ypos(i){return 4+ch/2+i*(H-8-ch)/(n-1);}
   function put(i){var c=ypos(i),mid=H/2;cap.style.top=(c-ch/2)+'px';cap.style.bottom='auto';
    var top=Math.min(c,mid),hh=Math.max(4,Math.abs(c-mid));stem.style.top=top+'px';stem.style.height=hh+'px';stem.style.bottom='auto';
    [].forEach.call(labs,function(l,j){l.style.color=j===i?'#ECEDEF':'#A6AAB0';});t.__i=i;}
   var kr=cap.getBoundingClientRect(),cy=kr.top+kr.height/2-r.top,i0=Math.round((cy-ypos(0))/((ypos(n-1)-ypos(0))/(n-1)));i0=Math.max(0,Math.min(n-1,i0));put(i0);
   var isWave=labs.length===3&&/[⊓∩↑↗↘↖⌒∧∨]/.test(t.nextElementSibling.textContent);
   t.addEventListener('pointerdown',function(e){e.preventDefault();var rr=t.getBoundingClientRect();var f=Math.max(0,Math.min(1,(e.clientY-rr.top)/rr.height));var i=Math.round(f*(n-1));put(i);
    var lab='';var par=t.parentElement;var spans=par?par.parentElement.querySelectorAll('span'):[];
    if(isWave){var kind=['square','tri','saw'][i];wave(kind==='tri'?'tri':kind);show('VCO WAVE',['SQR','TRI','SAW'][i]);}
    else if(n===2){show('SWITCH',i?'ON':'OFF');}else show('SELECT',(labs[i]?labs[i].textContent.trim():i+1));});
  });
  // step buttons
  var steps=[].slice.call(document.querySelectorAll('button[aria-label^="Step"]'));
  if(steps.length){var lit=null,off=null;function isL(b){return /6C9CF0|C8D9F5/i.test(b.getAttribute('data-orig')||'');}
   steps.forEach(function(b){b.setAttribute('data-orig',b.getAttribute('style'));var at=b.getAttribute('style')||'';if(/6C9CF0/i.test(at)&&!lit)lit=at;else if(/#2A2C2F/i.test(at)&&!off)off=at;});
   if(!off&&lit)off=lit.replace(/background:[^;]+;/,'background: linear-gradient(180deg,#2A2C2F,#141517);');
   if(!lit&&off)lit=off.replace(/background:[^;]+;/,'background: linear-gradient(180deg,#B9D6FF,#6C9CF0);');
   steps.forEach(function(b){b.addEventListener('click',function(e){e.stopPropagation();var isLit=b.getAttribute('data-lit')==null?isL(b):b.getAttribute('data-lit')==='1';
     b.setAttribute('style',isLit?off:lit);b.setAttribute('data-lit',isLit?'0':'1');show('STEP '+b.getAttribute('aria-label').replace(/\D/g,''),isLit?'OFF':'ON');},true);});}
  // joystick
  document.querySelectorAll('div').forEach(function(j){var r=j.getBoundingClientRect();if(Math.abs(r.width-70)>1||Math.abs(r.height-70)>1||j.hasAttribute('data-joy'))return;
   var k=[].filter.call(j.children,function(c){return getComputedStyle(c).position==='absolute';});if(k.length!==2)return;
   j.setAttribute('data-joy','1');j.style.cursor='grab';j.style.touchAction='none';
   var stem=k[0],ball=k[1];
   function pos(dx,dy){ball.style.transform='translate('+dx+'px,'+dy+'px)';stem.style.transformOrigin='50% 100%';stem.style.transform='translate('+dx*0.6+'px,'+dy*0.6+'px) rotate('+(dx/P*28)+'deg)';}
   j.addEventListener('pointerdown',function(e){e.preventDefault();j.setPointerCapture(e.pointerId);j.style.cursor='grabbing';
    function mv(m){var rr=j.getBoundingClientRect();var dx=m.clientX-(rr.left+rr.width/2),dy=m.clientY-(rr.top+rr.height/2);var d=Math.hypot(dx,dy);if(d>P){dx*=P/d;dy*=P/d;}
     pos(dx,dy);var bend=Math.round(dx/P*100),mod=Math.round(-dy/P*100);
     if(Math.abs(dx)>=Math.abs(dy))show('PITCH BEND',(bend>0?'+':'')+bend+'%');else show(mod>=0?'JOY Y+':'JOY Y-',Math.abs(mod)+'%');}
    function up(){j.removeEventListener('pointermove',mv);j.removeEventListener('pointerup',up);j.style.cursor='grab';ball.style.transition=stem.style.transition='transform .18s';pos(0,0);setTimeout(function(){ball.style.transition=stem.style.transition='';},220);}
    j.addEventListener('pointermove',mv);j.addEventListener('pointerup',up);mv(e);});});
 }
 function go(){setTimeout(lcdInit,1500);}if(document.readyState!=='loading')go();else document.addEventListener('DOMContentLoaded',go);
})();

// ===== WhiskerMix: mute red, solo yellow, channel narrow/widen =====
(function(){
 function init(){
  var btns=[].slice.call(document.querySelectorAll('button'));
  btns.forEach(function(b){var t=b.textContent.trim();
   if((t==='M'||t==='S')&&b.getAttribute('aria-label')===null&&b.offsetWidth<=30){b.setAttribute('data-nosel','1');b.__o=b.getAttribute('style');
    b.addEventListener('click',function(e){e.stopPropagation();var on=b.getAttribute('data-on')==='1';
     if(on){b.setAttribute('style',b.__o);b.setAttribute('data-on','0');}
     else{b.setAttribute('style',b.__o.replace(/background:[^;]+;/,'background: '+(t==='M'?'#E5484D':'#F5C542')+';').replace(/color:[^;]+;/,'color: '+(t==='M'?'#fff':'#1A1A1A')+';').replace(/border:[^;]+;/,'border: 1px solid '+(t==='M'?'#FF8A8E':'#FFE08A')+';')+';box-shadow:0 0 8px '+(t==='M'?'rgba(229,72,77,.7)':'rgba(245,197,66,.7)'));b.setAttribute('data-on','1');}
    },true);}
  });
  function narrowOf(strip){var h=strip.children[0],name=(strip.getAttribute('title')||h.textContent||'').trim();
   var n=document.createElement('div');n.title=name;n.setAttribute('style','width:20px;flex:none;background:#2B2E33;border-right:1px solid #1E2024;display:flex;flex-direction:column;align-items:center;gap:4px;padding-bottom:4px;box-sizing:border-box');
   var hd=document.createElement('div');hd.setAttribute('style','width:100%;height:18px;background:'+(getComputedStyle(h).backgroundColor||'#7FB6EA')+';border-top:2px solid '+(getComputedStyle(h).borderTopColor)+';');
   var sp=document.createElement('span');sp.textContent=name.replace(/^[^\w]+/,'');sp.setAttribute('style','font-size:8px;color:#B9B6AE;writing-mode:vertical-rl;transform:rotate(180deg);white-space:nowrap');
   var fl=document.createElement('div');fl.style.flex='1';
   var w=document.createElement('button');w.type='button';w.setAttribute('aria-label','Widen');w.textContent='▸';w.setAttribute('style','width:14px;height:14px;border:none;background:#1F2125;color:#B9B6AE;font-size:8px;cursor:pointer;border-radius:2px');
   n.appendChild(hd);n.appendChild(sp);n.appendChild(fl);n.appendChild(w);n.__full=strip;return n;}
  document.addEventListener('click',function(e){
   var b=e.target.closest&&e.target.closest('button');if(!b)return;var a=b.getAttribute('aria-label');
   if(a==='Narrow'){var s=b;while(s&&!(s.parentElement&&s.parentElement.style.alignItems==='stretch'&&s.style.width==='74px'))s=s.parentElement;if(!s)return;
     var n=narrowOf(s);s.parentElement.replaceChild(n,s);e.stopPropagation();e.preventDefault();}
   else if(a==='Widen'){var n=b.parentElement;if(!n.parentElement)return;var full=n.__full;
     if(!full){var tpl=null;[].forEach.call(n.parentElement.children,function(c){if(!tpl&&c.style.width==='74px')tpl=c;});if(!tpl)tpl=document.querySelector('div[style*="width: 74px"]');
       if(!tpl)return;full=tpl.cloneNode(true);var nm=(n.title||'').trim();var hh=full.children[0];var tn=null;(function w(x){x.childNodes.forEach(function(c){if(c.nodeType===3&&c.textContent.trim()&&!tn)tn=c;else if(c.nodeType===1)w(c);});})(hh);if(tn)tn.textContent=' '+nm;
       var hbg=getComputedStyle(n.children[0]).backgroundColor;hh.style.background=hbg;}
     n.parentElement.replaceChild(full,n);e.stopPropagation();e.preventDefault();}
  },true);
 }
 function go(){setTimeout(init,1500);}if(document.readyState!=='loading')go();else document.addEventListener('DOMContentLoaded',go);
})();

// ===== SupurrSaw: model-driven behaviours (part 1: core) =====
(function(){
 if(!/SupurrSaw/i.test(location.pathname))return;
 var SS=window.SS={K:{},L:{},seg:{},page:/Seq/i.test(location.pathname)?'seq':(/Presets/i.test(location.pathname)?'presets':'synth')};
 function $(s,r){return (r||document).querySelector(s);}
 function $$(s,r){return [].slice.call((r||document).querySelectorAll(s));}
 function clamp(v,a,b){return Math.max(a,Math.min(b,v));}
 SS.$=$;SS.$$=$$;SS.clamp=clamp;
 SS.on=function(k,f){(SS.L[k]=SS.L[k]||[]).push(f);};
 SS.emit=function(k,a){(SS.L[k]||[]).forEach(function(f){f(a);});(SS.L['*']||[]).forEach(function(f){f(k,a);});};
 var NOTES=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];SS.noteName=function(m){return NOTES[m%12]+(Math.floor(m/12)-2);};
 SS.toast=function(t){var d=$('#ss-toast');if(!d){d=document.createElement('div');d.id='ss-toast';d.setAttribute('style','position:fixed;left:50%;bottom:22px;transform:translateX(-50%);background:#0E0C12;color:#ECEAE4;border:1px solid #FF5FA2;border-radius:8px;padding:7px 14px;font:600 12px system-ui,sans-serif;z-index:10001;pointer-events:none;opacity:0;transition:opacity .15s');document.body.appendChild(d);}
  d.textContent=t;d.style.opacity=1;clearTimeout(d.__t);d.__t=setTimeout(function(){d.style.opacity=0;},1600);};
 var css=document.createElement('style');css.textContent='[data-auto]{box-shadow:0 0 0 1.5px #FF5FA2!important}svg[data-auto]{box-shadow:none!important;filter:drop-shadow(0 0 3px #FF5FA2)}.ss-menu{position:fixed;z-index:10002;background:#16131B;border:1px solid #4A4156;border-radius:8px;padding:4px;min-width:130px;font:600 12px system-ui,sans-serif;color:#ECEAE4;box-shadow:0 8px 24px rgba(0,0,0,.6);max-height:300px;overflow:auto}.ss-menu div{padding:5px 10px;border-radius:5px;cursor:pointer;white-space:nowrap}.ss-menu div:hover{background:#FF5FA2;color:#141518}';
 document.head.appendChild(css);
 var menuEl=null;function closeMenu(){if(menuEl){menuEl.remove();menuEl=null;}}
 SS.menu=function(x,y,items,cb){closeMenu();menuEl=document.createElement('div');menuEl.className='ss-menu';
  items.forEach(function(it){var d=document.createElement('div');d.textContent=it;d.addEventListener('click',function(e){e.stopPropagation();closeMenu();cb(it);});menuEl.appendChild(d);});
  document.body.appendChild(menuEl);var r=menuEl.getBoundingClientRect();menuEl.style.left=Math.min(x,innerWidth-r.width-8)+'px';menuEl.style.top=Math.min(y,innerHeight-r.height-8)+'px';};
 document.addEventListener('pointerdown',function(e){if(menuEl&&!menuEl.contains(e.target))closeMenu();},true);
 function secName(el){var s=el.closest('section');if(!s)return '';var c=el.closest('[data-card]');if(c&&s.contains(c)){var b=c.querySelector('b');if(b)return 'FX '+b.textContent.trim();}
  var h=s.firstElementChild;var sp=h&&h.querySelector('span');return (sp?sp.textContent:'').split('·')[0].trim();}
 SS.secName=secName;
 SS.sec=function(name){return $$('section').filter(function(s){return secName(s.firstElementChild)===name;})[0];};
 // ---- note names
 function parseNote(t){var m=/^([A-G]#?)(-?\d)$/.exec(t);if(!m)return null;return NOTES.indexOf(m[1])+12*(parseInt(m[2])+2);}
 // ---- knob spec inference
 function lin(a,b,f){return {to:function(n){return a+(b-a)*n;},from:function(v){return clamp((v-a)/(b-a),0,1);},f:f};}
 function lg(a,b,f){var la=Math.log(a),lb=Math.log(b);return {to:function(n){return Math.exp(la+(lb-la)*n);},from:function(v){return clamp((Math.log(Math.max(v,a))-la)/(lb-la),0,1);},f:f};}
 function sg(v,d){return (v>0?'+':v<0?'-':'')+Math.abs(v).toFixed(d||0);}
 function hz(v){return v>=1000?(v/1000).toFixed(1)+' kHz':Math.round(v)+' Hz';}
 function choice(list){return {to:function(n){return list[Math.min(list.length-1,Math.floor(n*list.length))];},from:function(i){return (i+.5)/list.length;},f:function(v){return v;},list:list,disc:true};}
 function infer(label,text){var L=label.toLowerCase(),t=text.replace('−','-').trim(),m,sp,v0;
  if(L==='thresh'){sp=lin(-48,0);sp.f=function(v){return Math.round(v)+' dB';};sp.n0=sp.from(parseFloat(t));return sp;}
  if(L==='ratio'){sp=lin(1,20);sp.f=function(v){return v.toFixed(2)+' : 1';};sp.n0=sp.from(parseFloat(t));return sp;}
  if(L==='atk'){sp=lg(.1,300);sp.f=function(v){return (v<10?v.toFixed(2):Math.round(v))+' ms';};sp.n0=sp.from(parseFloat(t));return sp;}
  if(L==='rls'){sp=lg(1,600);sp.f=function(v){return v<1.05?'Auto':(v<10?v.toFixed(2):Math.round(v))+' ms';};sp.n0=/Auto/.test(t)?0:sp.from(parseFloat(t));return sp;}
  if(L==='roll'){sp=choice(['1/32 ×1','1/16 ×1','1/16 ×2','1/8 ×1','1/8 ×2']);sp.f=function(v){return v;};sp.n0=sp.from(Math.max(0,sp.list.indexOf(t)));return sp;}
  if(L==='curve'){sp=choice(['lin','exp','log']);sp.n0=sp.from(Math.max(0,sp.list.indexOf(t)));return sp;}
  if(L==='loop'){sp=lin(0,1);sp.f=function(v){return v>.5?'on':'off';};sp.n0=t==='on'?1:0;return sp;}
  if(L==='accent'&&t==='off'){sp=lin(0,100);sp.f=function(v){return v<2?'off':Math.round(v)+'%';};sp.n0=0;return sp;}
  if((m=/^(\d+)\/(\d+)$/.exec(t))){sp=lin(0,100);sp.f=function(v){return Math.round(v)+'/'+Math.round(100-v);};sp.n0=parseInt(m[1])/100;return sp;}
  if((m=/^([LR]) (\d+)$/.exec(t))||t==='C'){v0=t==='C'?0:(m[1]==='L'?-1:1)*parseInt(m[2]);sp=lin(-100,100);sp.f=function(v){v=Math.round(v);return v===0?'C':(v<0?'L ':'R ')+Math.abs(v);};sp.n0=sp.from(v0);return sp;}
  if(parseNote(t)!==null){sp=lin(24,84);sp.f=function(v){return SS.noteName(Math.round(v));};sp.n0=sp.from(parseNote(t));return sp;}
  if(L==='voices'){sp=lin(1,16);sp.f=function(v){return String(Math.round(v));};sp.n0=sp.from(parseFloat(t));return sp;}
  if(L==='steps'){sp=lin(1,16);sp.f=function(v){return String(Math.round(v));};sp.n0=sp.from(parseFloat(t));return sp;}
  if(L==='octave'&&/^\u00b1/.test(t)){sp=lin(0,3);sp.f=function(v){return '\u00b1'+Math.round(v);};sp.n0=sp.from(parseFloat(t.slice(1)));return sp;}
  if(L==='octave'||L==='tuning'){var r=L==='octave'?3:12;sp=lin(-r,r);sp.f=function(v){return sg(Math.round(v));};if(L==='octave')sp.f=function(v){v=Math.round(v);return v===0?'0':sg(v).replace('-','−');};sp.n0=sp.from(parseFloat(t.replace('+','')));return sp;}
  if(L==='velocity'){sp=lin(0,127);sp.f=function(v){return String(Math.round(v));};sp.n0=sp.from(parseFloat(t));return sp;}
  if(L==='bend'){sp=lin(1,12);sp.f=function(v){return '±'+Math.round(v);};sp.n0=sp.from(parseFloat(t.replace('±','')));return sp;}
  m=/^([+\-]?)(\d+(?:\.\d+)?)\s*(%|dB|ms|kHz|Hz|st|ct|s)?$/.exec(t);
  if(m){var v=parseFloat(m[2])*(m[1]==='-'?-1:1),u=m[3]||'';
   if(u==='%'){if(m[1]==='+'||m[1]==='-'){sp=lin(-100,100);sp.f=function(x){x=Math.round(x);return sg(x)+'%';};}else{sp=lin(0,100);sp.f=function(x){return Math.round(x)+'%';};}sp.n0=sp.from(v);return sp;}
   if(u==='dB'){sp=lin(-24,6);sp.f=function(x){return Math.round(x)+' dB';};sp.n0=sp.from(v);return sp;}
   if(u==='kHz'||(u==='Hz'&&L!=='rate'&&L!=='bass mono')){if(u==='kHz')v*=1000;sp=lg(20,20000);sp.f=hz;sp.n0=sp.from(v);return sp;}
   if(u==='Hz'&&L==='rate'){sp=lg(.05,30);sp.f=function(x){return x.toFixed(1)+' Hz';};sp.n0=sp.from(v);return sp;}
   if(u==='Hz'){sp=lin(20,300);sp.f=function(x){return Math.round(x)+' Hz';};sp.n0=sp.from(v);return sp;}
   if(u==='ms'||(u===''&&(L==='d'||L==='r'||L==='a'))){sp=lg(1,3000);sp.f=function(x){return Math.round(x)+' ms';};sp.n0=sp.from(v);return sp;}
   if(u==='st'){sp=lin(-24,24);sp.f=function(x){return sg(Math.round(x))+' st';};sp.n0=sp.from(v);return sp;}
   if(u==='ct'){sp=lin(-100,100);sp.f=function(x){return Math.round(x)+' ct';};sp.n0=sp.from(v);return sp;}
   var hi=Math.max(1,Math.abs(v)*2);sp=lin(0,hi);var dec=(m[2].split('.')[1]||'').length;sp.f=function(x){return x.toFixed(dec);};sp.n0=sp.from(v);return sp;}
  sp=lin(0,100);sp.f=function(x){return Math.round(x)+'%';};sp.n0=.5;return sp;}
 // ---- knobs
 function initKnob(svg){
  if(svg.hasAttribute('data-kn'))return;var circ=svg.querySelector('circle'),path=svg.querySelector('path[d*=" A"]'),line=svg.querySelector('line');if(!circ||!path||!line)return;
  var cx=parseFloat(circ.getAttribute('cx')),cy=parseFloat(circ.getAttribute('cy')),r=parseFloat(circ.getAttribute('r'));
  var m=/^M([\d.\-]+),([\d.\-]+)/.exec(path.getAttribute('d'));if(!m)return;var sx=m[1],sy=m[2];
  var label=svg.getAttribute('aria-label')||'',host=svg.parentElement,num=null;
  var sibs=[].slice.call(host.children).filter(function(x){return x!==svg;});
  sibs.forEach(function(x){if(!num&&x.tagName==='SPAN'&&/font-weight:700/.test(x.getAttribute('style')||''))num=x;});
  var txt=num?num.textContent:'';
  if(!num){num=document.createElement('span');num.setAttribute('style','font-size:11px;font-weight:700');svg.insertAdjacentElement('afterend',num);
   var a0=Math.atan2(parseFloat(line.getAttribute('x2'))-cx,-(parseFloat(line.getAttribute('y2'))-cy))*180/Math.PI;txt=Math.round((a0+135)/270*100)+'%';}
  var spec=infer(label,txt);var key=svg.getAttribute('data-key')||(secName(svg)+'.'+label);var k={svg:svg,key:key,label:label,spec:spec,n:spec.n0,n0:spec.n0,num:num};
  function render(){var a=-135+270*k.n,rad=a*Math.PI/180,ex=cx+r*Math.sin(rad),ey=cy-r*Math.cos(rad);
   path.setAttribute('d','M'+sx+','+sy+' A'+r+','+r+' 0 '+(a+135>180?1:0)+' 1 '+ex.toFixed(1)+','+ey.toFixed(1));
   line.setAttribute('x2',ex.toFixed(1));line.setAttribute('y2',ey.toFixed(1));num.textContent=spec.f(spec.to(k.n));}
  k.set=function(n,quiet){k.n=clamp(n,0,1);render();if(!quiet)SS.emit('k:'+key,k.n);};
  k.val=function(){return spec.to(k.n);};
  render();SS.K[key]=k;svg.setAttribute('data-kn','1');svg.setAttribute('data-aknob','1');svg.style.cursor='ns-resize';svg.style.touchAction='none';
  svg.addEventListener('pointerdown',function(ev){if(ev.button)return;ev.preventDefault();svg.setPointerCapture(ev.pointerId);var y0=ev.clientY,n0=k.n,fine=ev.shiftKey?0.25:1;
   function mv(e){var d=(y0-e.clientY)*(e.shiftKey?.25:1)/150;if(spec.disc)d*=2;k.set(n0+d);}
   function up(){svg.removeEventListener('pointermove',mv);svg.removeEventListener('pointerup',up);}
   svg.addEventListener('pointermove',mv);svg.addEventListener('pointerup',up);});
  svg.addEventListener('wheel',function(e){e.preventDefault();k.set(k.n-e.deltaY*0.0016*(spec.disc?4:1));},{passive:false});
  svg.addEventListener('dblclick',function(){k.set(k.n0);});
 }
 SS.initKnobs=function(){$$('svg').forEach(initKnob);};
 // ---- segmented controls
 function initSegs(){$$('div').forEach(function(d){if(d.hasAttribute('data-seg'))return;var kids=[].slice.call(d.children);
   if(kids.length<2||!kids.every(function(c){return c.tagName==='SPAN'&&/padding:/.test(c.getAttribute('style')||'')&&/font-weight:700/.test(c.getAttribute('style')||'');}))return;
   var act=kids.filter(function(c){return /background:/.test(c.getAttribute('style')||'');});if(act.length!==1)return;
   var sec=secName(d),ord=$$('[data-seg]').filter(function(x){return secName(x)===sec;}).length,key=sec+'#'+ord;d.setAttribute('data-seg',key);
   var o={el:d,kids:kids,idx:kids.indexOf(act[0]),key:key};SS.seg[key]=o;
   o.set=function(i,quiet){if(i===o.idx)return;var a=o.kids[o.idx],b=o.kids[i],sa=a.getAttribute('style'),sb=b.getAttribute('style');a.setAttribute('style',sb);b.setAttribute('style',sa);o.idx=i;if(!quiet)SS.emit('seg:'+key,i);};
   kids.forEach(function(c,i){c.style.cursor='pointer';c.addEventListener('click',function(){o.set(i);});});
  });};
 // ---- on/off buttons
 function initBtns(){$$('button').forEach(function(b){var t=b.textContent.trim();if((t!=='On'&&t!=='Off')||b.hasAttribute('data-tg'))return;
   var cs=b.getAttribute('style'),onS=null,offS='height:24px;padding:0 8px;border-radius:6px;border:1px solid #4A4E57;background:#16171A;color:#ECEAE4;font:inherit;font-size:11px;font-weight:700;cursor:pointer;white-space:nowrap';
   var isOn=t==='On';if(isOn)onS=cs;else{var cc=b.closest('[data-card]');var col=(cc&&cc.getAttribute('data-col'))||secColor(b)||'#FF5FA2';onS=offS.replace('border:1px solid #4A4E57','border:1px solid '+col).replace('background:#16171A','background:'+col).replace('color:#ECEAE4','color:#141518');}
   var card=b.closest('[data-card]')||b.closest('section');var key=secName(b);
   var o={b:b,on:isOn,key:key};b.setAttribute('data-tg',key);SS.seg['tg:'+key]=o;
   o.set=function(v,quiet){o.on=v;b.setAttribute('style',v?onS:offS);b.textContent=v?'On':'Off';dim(b,v);if(!quiet)SS.emit('tg:'+key,v);};
   b.addEventListener('click',function(){o.set(!o.on);});
   dim(b,isOn);
  });};
 function secColor(b){var s=b.closest('section');var h=s&&s.firstElementChild&&s.firstElementChild.querySelector('span');return h?getComputedStyle(h).color:null;}
 function dim(b,on){var card=b.closest('[data-card]');var tgt=card;
  if(!tgt){var s=b.closest('section');if(!s)return;[].slice.call(s.children).forEach(function(c,i){if(i===0)return;c.style.transition='opacity .15s,filter .15s';c.style.opacity=on?'':'.38';c.style.filter=on?'':'grayscale(.7)';});return;}
  [].slice.call(card.children).forEach(function(c,i){if(i===0)return;c.style.transition='opacity .15s,filter .15s';c.style.opacity=on?'':'.38';c.style.filter=on?'':'grayscale(.8)';});}
 // ---- context menu: automate / learn / reset
 document.addEventListener('contextmenu',function(e){var t=e.target.closest('svg[data-kn],button,[data-seg] span,[data-dd]');if(!t||!document.body.hasAttribute('data-own'))return;e.preventDefault();
  var on=t.hasAttribute('data-auto');var items=[on?'Remove automation':'Automate this control','MIDI learn'];if(t.getAttribute('data-kn'))items.push('Reset to default');
  SS.menu(e.clientX,e.clientY,items,function(it){if(it==='Reset to default'){t.dispatchEvent(new MouseEvent('dblclick',{bubbles:true}));return;}
   if(it==='MIDI learn'){t.setAttribute('data-auto','1');t.title='MIDI learn armed: move a controller';SS.toast('MIDI learn: move a knob or fader on your controller');return;}
   if(on){t.removeAttribute('data-auto');}else{t.setAttribute('data-auto','1');SS.toast('Exposed to the host as an automation parameter');}});});
 // ---- dropdown spans
 SS.dropdown=function(el,items,cb){el.setAttribute('data-dd','1');el.style.cursor='pointer';el.addEventListener('click',function(e){e.stopPropagation();var r=el.getBoundingClientRect();SS.menu(r.left,r.bottom+2,items,function(it){el.textContent=it+' ▾';cb(it);});});};
 SS.ddOf=function(sec){return $$('span',sec).filter(function(s){return /▾\s*$/.test(s.textContent)&&s.children.length===0;});};
 // ---- init
 function go(){document.body.setAttribute('data-own','1');if(SS.pre)SS.pre();SS.initKnobs();initSegs();initBtns();SS.emit('ready');}
 SS.go=function(){setTimeout(go,850);};
 if(document.readyState!=='loading')SS.go();else document.addEventListener('DOMContentLoaded',SS.go);
})();

// ===== SupurrSaw part 2: synth page visuals =====
(function(){
 if(!window.SS||SS.page!=='synth')return;
 var $=SS.$,$$=SS.$$,clamp=SS.clamp;
 function K(k){return SS.K[k];}
 function kv(L,n,d){var k=K('LAYER '+L+'.'+n);return k?k.val():d;}
 var FXCOL={Distortion:'#FF5FA2',Chorus:'#9B7CFF',Phaser:'#3FE0D0',Delay:'#FFB547',Reverb:'#B6F35A'};
 SS.pre=function(){var s=SS.sec('FX CHAIN');if(!s)return;[].slice.call(s.children[1].children).forEach(function(c){c.setAttribute('data-card','1');var n=c.querySelector('b').textContent.trim();c.setAttribute('data-col',FXCOL[n]||'#FF5FA2');c.setAttribute('data-fxname',n);});};
 var TYPES=['Supersaw','Super Square','Super Tri','Super Sine','Hoover'],SAMPLES=['pad_glass.wav','choir_ah.wav','hoover_stab.wav','303_hit.wav','vox_chop.wav'];
 var LAY={A:{col:'#FF5FA2',mode:'osc',type:'Supersaw'},B:{col:'#9B7CFF',mode:'osc',type:'Super Square'},C:{col:'#3FE0D0',mode:'sample',type:'Supersaw',sample:'pad_glass.wav'}};SS.LAY=LAY;
 function seeded(n){var x=Math.sin(n*127.1)*43758.5453;return x-Math.floor(x);}
 function wf(type,ph,duty){ph=ph-Math.floor(ph);switch(type){case 'Super Square':return ph<duty?1:-1;case 'Super Tri':return ph<.5?4*ph-1:3-4*ph;case 'Super Sine':return Math.sin(ph*6.2832);default:return 2*ph-1;}}
 function drawOsc(L){var sec=SS.sec('LAYER '+L),svg=$('svg[viewBox="0 0 300 90"]',sec);if(!svg)return;var st=LAY[L],col=st.col;
  var det=kv(L,'Detune',30)/100,nv=Math.round(kv(L,'Voices',7)),oct=Math.round(kv(L,'Octave',0)),pwm=kv(L,'PWM',30)/100,amp=Math.pow(10,kv(L,'Level',-3)/20);
  var P=58/Math.pow(2,oct),duty=clamp(.5-pwm*.4,.1,.9),ghost=Math.min(nv,9),sum=[],out='',x,i;
  for(x=0;x<=300;x+=2)sum.push(0);
  for(i=0;i<nv;i++){var d=nv===1?0:(i/(nv-1)-.5)*2,per=P*(1+d*det*.12),off=0,pts=[];
   for(x=0;x<=300;x+=2){var pp=st.type==='Hoover'?per*(1+x/300*.5):per,y=wf(st.type,(x+off)/pp,duty);sum[x/2]+=y/nv;if(i<ghost)pts.push(x+','+(45-y*amp*38).toFixed(1));}
   if(i<ghost)out+='<polyline points="'+pts.join(' ')+'" fill="none" stroke="'+col+'" stroke-opacity="'+(nv>1?0.3:0)+'" stroke-width="1.2"/>';}
  var mp=[];for(x=0;x<=300;x+=2){var yy=wf(st.type,x/(st.type==='Hoover'?P*(1+x/300*.5):P),duty);mp.push(x+','+(45-yy*amp*38).toFixed(1));}
  out+='<polyline points="'+mp.join(' ')+'" fill="none" stroke="'+col+'" stroke-width="1.6"/>';
  svg.innerHTML=out;}
 function drawSample(L){var sec=SS.sec('LAYER '+L),svg=$('svg[viewBox="0 0 300 90"]',sec);if(!svg)return;var st=LAY[L],h=0,i;for(i=0;i<st.sample.length;i++)h+=st.sample.charCodeAt(i);
  var out='<g>',x;for(x=0;x<300;x+=4){var e=Math.sin(x/300*Math.PI)*.7+.3,a=(.3+.7*seeded(h+x*.37))*e*38;out+='<rect x="'+x+'" y="'+(45-a).toFixed(1)+'" width="3" height="'+(a*2).toFixed(1)+'" fill="#3FE0D0" fill-opacity=".7"/>';}
  var sx=(kv(L,'Start',20)/100)*300,loop=kv(L,'Loop',1)>.5;out+='</g><line x1="'+sx+'" x2="'+sx+'" y1="0" y2="90" stroke="#FFB547" stroke-width="1.5"/><line x1="270" x2="270" y1="0" y2="90" stroke="#FFB547" stroke-width="1.5"'+(loop?'':' stroke-dasharray="3 3" stroke-opacity=".5"')+'/>';
  svg.innerHTML=out;}
 function layer(L){return LAY[L].mode==='osc'?drawOsc(L):drawSample(L);}
 function wrapInfo(L){var sec=SS.sec('LAYER '+L),svg=$('svg[viewBox="0 0 300 90"]',sec),w=svg.parentElement;if(w===sec)return;var cap=w.querySelector('span,div');
  w.style.borderStyle=LAY[L].mode==='sample'?'dashed':'none';[].forEach.call(w.children,function(c){if(c!==svg&&c.tagName!=='svg')c.style.display=LAY[L].mode==='sample'?'':'none';});}
 function setupLayers(){['A','B','C'].forEach(function(L){var sec=SS.sec('LAYER '+L),seg=SS.seg['LAYER '+L+'#0'],dd=SS.ddOf(sec.firstElementChild)[0],st=LAY[L];
   st.mode=seg.idx===0?'osc':'sample';if(L==='C'){st.mode='sample';}
   function label(){dd.textContent=(st.mode==='osc'?st.type:st.sample)+' ▾';}
   seg.el.addEventListener('click',function(){st.mode=seg.idx===0?'osc':'sample';label();wrapInfo(L);layer(L);});
   dd.setAttribute('data-dd','1');dd.style.cursor='pointer';
   dd.addEventListener('click',function(e){e.stopPropagation();var r=dd.getBoundingClientRect();SS.menu(r.left,r.bottom+2,st.mode==='osc'?TYPES:SAMPLES,function(it){if(st.mode==='osc')st.type=it;else st.sample=it;label();layer(L);SS.toast('Layer '+L+': '+it);});});
   ['Detune','Voices','Octave','PWM','Level','Start','Loop'].forEach(function(n){SS.on('k:LAYER '+L+'.'+n,function(){layer(L);});});
   label();wrapInfo(L);layer(L);});}
 // ---- amp env
 function ampEnv(){var sec=SS.sec('AMP ENV'),svg=$('svg[viewBox="0 0 180 64"]',sec);if(!svg)return;var A=K('AMP ENV.A'),D=K('AMP ENV.D'),S=K('AMP ENV.S'),R=K('AMP ENV.R');
  var P=[[0,60],[0,6],[0,0],[0,0],[180,60]];
  svg.innerHTML='<polygon id="ae-f" fill="#B6F35A" fill-opacity=".12"/><polyline fill="none" stroke="#B6F35A" stroke-width="2"/>'+[0,1,2,3,4].map(function(){return '<circle r="4" fill="#B6F35A" stroke="#141218" stroke-width="1.5" style="cursor:grab;touch-action:none"/>';}).join('');
  var pl=$('polyline',svg),pg=$('polygon',svg),cs=$$('circle',svg);
  function lay(){var x1=3+A.n*35,x2=x1+4+D.n*50,yS=60-S.n*54,xr=180-(6+R.n*55);P=[[0,60],[x1,6],[x2,yS],[xr,yS],[180,60]];
   var s=P.map(function(p){return p[0].toFixed(1)+','+p[1].toFixed(1);}).join(' ');pl.setAttribute('points',s);pg.setAttribute('points',s);cs.forEach(function(c,i){c.setAttribute('cx',P[i][0].toFixed(1));c.setAttribute('cy',P[i][1].toFixed(1));});}
  ['A','D','S','R'].forEach(function(n){SS.on('k:AMP ENV.'+n,lay);});lay();svg.setAttribute('data-pts','1');
  function pt(e){var p=svg.createSVGPoint();p.x=e.clientX;p.y=e.clientY;return p.matrixTransform(svg.getScreenCTM().inverse());}
  cs.forEach(function(c,i){if(i===0||i===4)return;c.addEventListener('pointerdown',function(ev){ev.preventDefault();c.setPointerCapture(ev.pointerId);
   function mv(m){var p=pt(m);if(i===1)A.set((p.x-3)/35);if(i===2){D.set((p.x-P[1][0]-4)/50);S.set((60-p.y)/54);}if(i===3){R.set((180-6-p.x)/55);S.set((60-p.y)/54);}}
   function up(){c.removeEventListener('pointermove',mv);c.removeEventListener('pointerup',up);}c.addEventListener('pointermove',mv);c.addEventListener('pointerup',up);});
   c.addEventListener('dblclick',function(){if(i===1)A.set(A.n0);if(i===2){D.set(D.n0);S.set(S.n0);}if(i===3){R.set(R.n0);S.set(S.n0);}});});
  svg.addEventListener('dblclick',function(e){if(e.target===svg||e.target.tagName==='polygon'||e.target.tagName==='polyline'){[A,D,S,R].forEach(function(k){k.set(k.n0);});}});}
 // ---- filter
 var FT={'Trance LP':['lp',2,1],'Trance HP':['hp',2,1],'Trance BP':['bp',2,1],'Acid 303':['lp',4,1.4],'Hoover LP':['lp',2,1.8],'Notch':['nt',2,1],'Moog 24 LP':['lp',4,1]};SS.FT=FT;
 var filt={name:'Trance LP'};SS.filt=filt;
 function resp(f,fc,Q,mode,poles){var r=f/fc,den=Math.sqrt(Math.pow(1-r*r,2)+Math.pow(r/Q,2)),h;
  if(mode==='lp')h=1/den;else if(mode==='hp')h=r*r/den;else if(mode==='bp')h=(r/Q)/den;else h=Math.abs(1-r*r)/den;h=Math.pow(h,poles/2);return 20*Math.log10(Math.max(h,1e-4));}
 SS.filtParams=function(){var t=FT[filt.name]||FT['Trance LP'];var Q=(0.7+K('FILTER.Reso').n*10)*t[2];return {mode:t[0],poles:t[1],Q:Q,fc:K('FILTER.Cutoff').val(),env:K('FILTER.Env amt').val()/100};};
 function filterVis(){var sec=SS.sec('FILTER'),svg=$('svg[viewBox="0 0 180 64"]',sec);if(!svg)return;
  svg.innerHTML='<line x1="0" x2="180" y1="48" y2="48" stroke="#352E40" stroke-width=".6"/><polygon fill="#FFB547" fill-opacity=".1"/><polyline id="f-e" fill="none" stroke="#FFB547" stroke-opacity=".45" stroke-dasharray="3 2" stroke-width="1.2"/><polyline id="f-m" fill="none" stroke="#FFB547" stroke-width="2"/><circle r="4.5" fill="#FFB547" stroke="#141218" stroke-width="1.5" style="cursor:grab;touch-action:none"/>';
  var pl=$('#f-m',svg),pe=$('#f-e',svg),pg=$('polygon',svg),h=$('circle',svg);svg.setAttribute('data-pts','1');
  function ypx(db){return clamp(62-(db+36)/54*58,3,62);}function xpx(f){return Math.log(f/20)/Math.log(1000)*180;}
  function draw(){var p=SS.filtParams(),a=[],b=[],x;for(x=0;x<=180;x+=2){var f=20*Math.pow(1000,x/180);a.push(x+','+ypx(resp(f,p.fc,p.Q,p.mode,p.poles)).toFixed(1));b.push(x+','+ypx(resp(f,clamp(p.fc*Math.pow(2,p.env*3),20,20000),p.Q,p.mode,p.poles)).toFixed(1));}
   pl.setAttribute('points',a.join(' '));pe.setAttribute('points',b.join(' '));pg.setAttribute('points','0,64 '+a.join(' ')+' 180,64');
   var hx=xpx(p.fc);h.setAttribute('cx',hx.toFixed(1));h.setAttribute('cy',ypx(resp(p.fc,p.fc,p.Q,p.mode,p.poles)).toFixed(1));}
  ['Cutoff','Reso','Env amt'].forEach(function(n){SS.on('k:FILTER.'+n,draw);});SS.on('seg:FILTER#0',draw);SS.on('filt',draw);draw();
  function pt(e){var q=svg.createSVGPoint();q.x=e.clientX;q.y=e.clientY;return q.matrixTransform(svg.getScreenCTM().inverse());}
  h.addEventListener('pointerdown',function(ev){ev.preventDefault();h.setPointerCapture(ev.pointerId);function mv(m){var p=pt(m);K('FILTER.Cutoff').set(clamp(p.x/180,0,1));K('FILTER.Reso').set(clamp((50-p.y)/44,0,1));}
   function up(){h.removeEventListener('pointermove',mv);h.removeEventListener('pointerup',up);}h.addEventListener('pointermove',mv);h.addEventListener('pointerup',up);});
  h.addEventListener('dblclick',function(){K('FILTER.Cutoff').set(K('FILTER.Cutoff').n0);K('FILTER.Reso').set(K('FILTER.Reso').n0);});
  var seg=SS.seg['FILTER#0'];var names=seg.kids.map(function(k){return k.textContent.trim();});filt.name=names[seg.idx];
  seg.el.addEventListener('click',function(){filt.name=names[seg.idx];SS.emit('filt');});
  var dd=SS.ddOf(sec.firstElementChild)[0];if(dd){dd.style.cursor='pointer';dd.setAttribute('data-dd','1');dd.addEventListener('click',function(e){e.stopPropagation();var r=dd.getBoundingClientRect();SS.menu(r.left,r.bottom+2,Object.keys(FT),function(it){var i=names.indexOf(it);if(i>=0){seg.set(i);filt.name=it;}else{filt.name=it;}dd.textContent=it+' ▾';SS.emit('filt');});});}}
 // ---- pitch env
 function pitchEnv(){var sec=SS.sec('PITCH ENV'),svg=$('svg[viewBox="0 0 180 64"]',sec);if(!svg)return;var Am=K('PITCH ENV.Amount'),Ti=K('PITCH ENV.Time'),Cu=K('PITCH ENV.Curve');
  svg.innerHTML='<line x1="0" x2="180" y1="32" y2="32" stroke="#8C8399" stroke-width="1" stroke-dasharray="4 3"/><polyline fill="none" stroke="#3FE0D0" stroke-width="2"/><circle r="4" fill="#3FE0D0" stroke="#141218" stroke-width="1.5" style="cursor:ns-resize;touch-action:none"/><circle r="4" fill="#3FE0D0" stroke="#141218" stroke-width="1.5" style="cursor:ew-resize;touch-action:none"/>';svg.setAttribute('data-pts','1');
  svg.setAttribute('title','Dashed line = 0 st. Handles snap to it; hold Alt to move freely');
  var hd=sec.firstElementChild,tag=document.createElement('span');tag.setAttribute('style','font-size:10px;color:#8C8399;margin-left:6px');tag.textContent='0 st centre line · Alt = free';hd.insertBefore(tag,hd.children[1]);
  var pl=$('polyline',svg),c0=$$('circle',svg)[0],c1=$$('circle',svg)[1],snapped=false;
  function lay(){var y0=32-(Am.val()/24)*28,x1=6+Ti.n*120,cur=Cu.spec.to(Cu.n),pts=[],i;
   for(i=0;i<=24;i++){var t=i/24,f=cur==='lin'?1-t:cur==='exp'?Math.pow(1-t,2.2):1-Math.pow(t,2.2);pts.push((x1*t).toFixed(1)+','+(32+(y0-32)*f).toFixed(1));}
   pts.push('180,32');pl.setAttribute('points',pts.join(' '));c0.setAttribute('cx',0);c0.setAttribute('cy',y0.toFixed(1));c1.setAttribute('cx',x1.toFixed(1));c1.setAttribute('cy',32);
   c0.setAttribute('fill',Math.abs(Am.val())<.5?'#fff':'#3FE0D0');}
  ['Amount','Time','Curve'].forEach(function(n){SS.on('k:PITCH ENV.'+n,lay);});lay();
  function pt(e){var q=svg.createSVGPoint();q.x=e.clientX;q.y=e.clientY;return q.matrixTransform(svg.getScreenCTM().inverse());}
  c0.addEventListener('pointerdown',function(ev){ev.preventDefault();c0.setPointerCapture(ev.pointerId);function mv(m){var p=pt(m),y=clamp(p.y,4,60);if(!m.altKey&&Math.abs(y-32)<3.5)y=32;Am.set((32-y)/28*0.5+0.5);}
   function up(){c0.removeEventListener('pointermove',mv);c0.removeEventListener('pointerup',up);}c0.addEventListener('pointermove',mv);c0.addEventListener('pointerup',up);});
  c1.addEventListener('pointerdown',function(ev){ev.preventDefault();c1.setPointerCapture(ev.pointerId);function mv(m){var p=pt(m);Ti.set((p.x-6)/120);}
   function up(){c1.removeEventListener('pointermove',mv);c1.removeEventListener('pointerup',up);}c1.addEventListener('pointermove',mv);c1.addEventListener('pointerup',up);});
  c0.addEventListener('dblclick',function(){Am.set(Am.n0);});c1.addEventListener('dblclick',function(){Ti.set(Ti.n0);});
  // knob amount snaps to 0 as well (Alt = free)
  Am.svg.addEventListener('pointermove',function(e){if(e.buttons&1&&!e.altKey&&Math.abs(Am.val())<1.2&&Math.abs(Am.val())>0){Am.set(.5,true);Am.set(.5);}});}
 // ---- width
 var width={mono:false};
 function widthVis(){var sec=SS.sec('WIDTH'),svg=$('svg[viewBox="0 0 260 64"]',sec);if(!svg)return;var Sp=K('WIDTH.Spread'),Cs=K('WIDTH.Centre/side'),Bm=K('WIDTH.Bass mono');
  var rects=$$('rect',svg),btns=$$('button',sec.firstElementChild),mono=btns[0],safe=btns[1];
  var bar=$$('div',sec).filter(function(d){return /linear-gradient/.test(d.getAttribute('style')||'');})[0],mk=bar&&bar.children[0],lbl=$$('span',sec).filter(function(s){return s.children.length===0&&/mono-safe|check bass|phase issue/.test(s.textContent);})[0];
  var safeOn=safe.getAttribute('style');
  function draw(){var s=Sp.n,c=Cs.n,sig=.45+s*4.2,i;
   for(i=0;i<9;i++){var d=i-4,w=Math.exp(-Math.pow(d/sig,2));if(d===0)w*=1+(c-.5)*1.2;else w*=1-(c-.5)*.8;w=clamp(w,.05,1);if(width.mono&&d!==0)w=.04;if(width.mono&&d===0)w=1;
    var h=6+w*50;rects[i].setAttribute('height',h.toFixed(1));rects[i].setAttribute('y',(62-h).toFixed(1));rects[i].setAttribute('fill-opacity',(.35+.65*w).toFixed(2));rects[i].setAttribute('fill',width.mono?'#B6F35A':'#FF5FA2');}
   var corr=clamp(1-s*(1-c)*1.15-(s>.8?.1:0)+Bm.n*.12,-1,1);if(width.mono)corr=1;
   if(mk){mk.style.left=((corr+1)/2*100)+'%';}
   if(lbl){lbl.textContent=(corr>=0?'+':'')+corr.toFixed(2)+(corr>=.3?' mono-safe':corr>=0?' check bass':' phase issue');lbl.style.color=corr>=.3?'#B6F35A':corr>=0?'#FFB547':'#FF5F5F';}
   var col=corr>=.3?'#B6F35A':corr>=0?'#FFB547':'#FF5F5F';safe.textContent=corr>=.3?'Safe':corr>=0?'Check':'Phase!';safe.style.background=col;safe.style.borderColor=col;
   mono.style.background=width.mono?'#B6F35A':'#16171A';mono.style.color=width.mono?'#141518':'#ECEAE4';}
  ['Spread','Centre/side','Bass mono'].forEach(function(n){SS.on('k:WIDTH.'+n,draw);});
  mono.addEventListener('click',function(){width.mono=!width.mono;draw();SS.toast(width.mono?'Mono check: summing to mono':'Stereo');});draw();}
 SS.on('ready',function(){try{setupLayers();ampEnv();filterVis();pitchEnv();widthVis();}catch(e){console.error('ss2',e);}});
})();

// ===== SupurrSaw part 3: audio engine and piano =====
(function(){
 if(!window.SS)return;
 var $=SS.$,$$=SS.$$,clamp=SS.clamp,AU=SS.AU={};
 function K(k){return SS.K[k];}
 var ac=null,fx={};
 function mix(inp,wet){var o=ac.createGain(),d=ac.createGain(),w=ac.createGain();inp.connect(d);d.connect(o);inp.connect(wet.i);wet.o.connect(w);w.connect(o);return {o:o,d:d,w:w};}
 function ctx(){if(!ac){var C=window.AudioContext||window.webkitAudioContext;if(!C)return null;ac=new C();build();}if(ac.state==='suspended')ac.resume();return ac;}
 AU.ctx=ctx;
 function build(){var i,inn=ac.createGain();AU.in=inn;
  var ws=ac.createWaveShaper(),wi=ac.createGain();wi.connect(ws);fx.dist=mix(inn,{i:wi,o:ws});fx.ws=ws;
  var cd=ac.createDelay(.05),ci=ac.createGain();ci.connect(cd);cd.delayTime.value=.018;var cl=ac.createOscillator(),cg=ac.createGain();cl.frequency.value=.8;cg.gain.value=.004;cl.connect(cg);cg.connect(cd.delayTime);cl.start();
  fx.chor=mix(fx.dist.o,{i:ci,o:cd});
  var pi=ac.createGain(),pf=[],pl=ac.createOscillator(),pg=ac.createGain(),prev=pi;pl.frequency.value=.35;pg.gain.value=700;pl.connect(pg);pl.start();
  for(i=0;i<4;i++){var a=ac.createBiquadFilter();a.type='allpass';a.frequency.value=1100;pg.connect(a.frequency);prev.connect(a);prev=a;}
  fx.phas=mix(fx.chor.o,{i:pi,o:prev});
  var di=ac.createGain(),dl=ac.createDelay(2),fb=ac.createGain(),lp=ac.createBiquadFilter();dl.delayTime.value=.28;fb.gain.value=.4;lp.type='lowpass';lp.frequency.value=3500;di.connect(dl);dl.connect(lp);lp.connect(fb);fb.connect(dl);
  fx.del=mix(fx.phas.o,{i:di,o:lp});fx.fb=fb;
  var rv=ac.createConvolver(),len=ac.sampleRate*2.4,buf=ac.createBuffer(2,len,ac.sampleRate),ch,n;for(ch=0;ch<2;ch++){var d=buf.getChannelData(ch);for(n=0;n<len;n++)d[n]=(Math.random()*2-1)*Math.pow(1-n/len,3);}rv.buffer=buf;var ri=ac.createGain();ri.connect(rv);
  fx.rev=mix(fx.del.o,{i:ri,o:rv});
  var comp=ac.createDynamicsCompressor(),mg=ac.createGain();mg.gain.value=.8;if(AU.mbBuild)AU.mbBuild(fx.rev.o,comp);else fx.rev.o.connect(comp);comp.connect(mg);mg.connect(ac.destination);AU.master=mg;AU.comp=comp;
  applyFx();}
 function fxOn(n){var o=SS.seg['tg:FX '+n];return o?o.on:false;}
 function kn(a,b,d){var k=K('FX '+a+'.'+b);return k?k.n:d;}
 function applyFx(){if(!ac)return;var m=[['Distortion','dist'],['Chorus','chor'],['Phaser','phas'],['Delay','del'],['Reverb','rev']];
  m.forEach(function(p){var mx=fxOn(p[0])?kn(p[0],'Mix',.5):0;fx[p[1]].w.gain.value=mx;fx[p[1]].d.gain.value=1-mx*.5;});
  var amt=kn('Distortion','Amount',.25)*100,cur=new Float32Array(1024),i;for(i=0;i<1024;i++){var x=i/512-1;cur[i]=((3+amt)*x*20*Math.PI/180)/(Math.PI+amt*Math.abs(x));}fx.ws.curve=cur;
  fx.fb.gain.value=.15+kn('Delay','Amount',.3)*.6;}
 SS.on('*',function(k){if(/^(k:FX |tg:FX )/.test(k))applyFx();});
 AU.applyFx=applyFx;
 var PW={};function pulse(d){d=Math.round(d*50)/50;if(PW[d])return PW[d];var re=new Float32Array(64),im=new Float32Array(64),n;for(n=1;n<64;n++){re[n]=2/(n*Math.PI)*Math.sin(n*Math.PI*d);im[n]=2/(n*Math.PI)*(1-Math.cos(n*Math.PI*d));}return PW[d]=ac.createPeriodicWave(re,im);}
 var SB={};function sampleBuf(name){if(SB[name])return SB[name];var len=ac.sampleRate*2,b=ac.createBuffer(1,len,ac.sampleRate),d=b.getChannelData(0),h=0,i,k;for(i=0;i<name.length;i++)h+=name.charCodeAt(i);
  for(k=0;k<6;k++){var f=261.6*(1+(k-2.5)*.004*(1+h%5)),ph=k*.7;for(i=0;i<len;i++){var t=i/ac.sampleRate;d[i]+=(Math.sin(6.2832*f*t+ph)+.5*Math.sin(6.2832*2*f*t)+.25*Math.sin(6.2832*3*f*t*(1+h%3*.001)))*Math.min(1,t*6)*Math.min(1,(2-t)*3)/6;}}return SB[name]=b;}
 var voices={},order=[];
 function lv(db){return Math.pow(10,db/20);}
 AU.synthOn=function(m,vel){var c=ctx();if(!c)return;AU.synthOff(m,true);var t=c.currentTime,f0=440*Math.pow(2,(m-69)/12);
  var amp=c.createGain(),p=SS.filtParams?SS.filtParams():{mode:'lp',poles:2,Q:1,fc:4000,env:.4};
  var A=K('AMP ENV.A').val()/1000,D=K('AMP ENV.D').val()/1000,S=K('AMP ENV.S').n,R=K('AMP ENV.R').val()/1000;
  var f1=c.createBiquadFilter(),f2=p.poles>2?c.createBiquadFilter():null,ty={lp:'lowpass',hp:'highpass',bp:'bandpass',nt:'notch'}[p.mode];f1.type=ty;f1.Q.value=Math.min(p.Q,18);if(f2){f2.type=ty;f2.Q.value=Math.min(p.Q*.5,12);}
  var base=clamp(p.fc*Math.pow(2,(m-60)/12*K('FILTER.Key trk').n),20,18000),top=clamp(base*Math.pow(2,Math.max(0,p.env)*4),20,18000);
  [f1,f2].forEach(function(f){if(!f)return;f.frequency.setValueAtTime(top,t);f.frequency.setTargetAtTime(base,t+A,Math.max(.02,D/3));});
  var oscs=[],mw=K('PLAY.Mod wheel')?K('PLAY.Mod wheel').n:0,lfo=c.createOscillator(),lg=c.createGain();lfo.frequency.value=K('LFO.Rate').val();lg.gain.value=mw*40;lfo.connect(lg);lfo.start(t);oscs.push(lfo);
  var pe=SS.seg['tg:PITCH ENV'],pitchOn=pe&&pe.on,pc=K('PITCH ENV.Amount').val()*100,tau=K('PITCH ENV.Time').val()/1000/3;
  var sum=c.createGain();
  ['A','B','C'].forEach(function(L){var tg=SS.seg['tg:LAYER '+L];if(tg&&!tg.on)return;var st=SS.LAY[L];
   if(st.mode==='sample'){var s=c.createBufferSource(),g=c.createGain();s.buffer=sampleBuf(st.sample);var root=K('LAYER C.Root')?K('LAYER C.Root').val():60;s.playbackRate.value=Math.pow(2,(m-root)/12+(K('LAYER C.Tune')?K('LAYER C.Tune').val()/1200:0));s.loop=!K('LAYER C.Loop')||K('LAYER C.Loop').n>.5;g.gain.value=lv(K('LAYER C.Level')?K('LAYER C.Level').val():-6)*.6;s.connect(g);g.connect(sum);s.start(t);oscs.push(s);return;}
   var kk=function(n,d){var q=K('LAYER '+L+'.'+n);return q?q.val():d;},nv=Math.max(1,Math.min(7,Math.round(kk('Voices',7)))),det=kk('Detune',30)/100,oc=Math.round(kk('Octave',0)),pw=kk('PWM',30)/100,g=lv(kk('Level',-3))*.5/Math.sqrt(nv),i;
   for(i=0;i<nv;i++){var o=c.createOscillator(),og=c.createGain(),d=nv===1?0:(i/(nv-1)-.5)*2;o.frequency.value=f0*Math.pow(2,oc);o.detune.value=d*det*45;
    if(st.type==='Super Square')o.setPeriodicWave(pulse(clamp(.5-pw*.4,.1,.9)));else o.type={'Super Tri':'triangle','Super Sine':'sine'}[st.type]||'sawtooth';
    if(pitchOn){o.detune.setValueAtTime(d*det*45+pc,t);o.detune.setTargetAtTime(d*det*45,t,Math.max(.01,tau));}
    lg.connect(o.detune);og.gain.value=g;o.connect(og);og.connect(sum);o.start(t);oscs.push(o);}});
  var sb=SS.seg['tg:SUB OSC'];if(sb&&sb.on){var so=c.createOscillator(),sg=c.createGain(),sgm=SS.seg['SUB OSC#0'];so.type=['sine','triangle','square','sawtooth'][sgm?sgm.idx:0];so.frequency.value=f0*Math.pow(2,K('SUB OSC.Octave')?Math.round(K('SUB OSC.Octave').val()):-1);sg.gain.value=lv(K('SUB OSC.Level').val())*.4;so.connect(sg);sg.connect(sum);so.start(t);oscs.push(so);}
  sum.connect(f1);if(f2){f1.connect(f2);f2.connect(amp);}else f1.connect(amp);amp.connect(AU.in);
  var v=vel/127;amp.gain.setValueAtTime(0,t);amp.gain.linearRampToValueAtTime(v,t+Math.max(.002,A));amp.gain.setTargetAtTime(Math.max(.001,S*v),t+A,Math.max(.02,D/3));
  var h={m:m,off:function(){var n=c.currentTime;amp.gain.cancelScheduledValues(n);amp.gain.setValueAtTime(amp.gain.value,n);amp.gain.setTargetAtTime(0,n,Math.max(.01,R/3));oscs.forEach(function(o){try{o.stop(n+R+.3);}catch(e){}});}};
  voices[m]=h;order.push(m);if(order.length>10){var old=order.shift();AU.synthOff(old,true);}return h;};
 AU.synthOff=function(m,quiet){var h=voices[m];if(!h)return;delete voices[m];order=order.filter(function(x){return x!==m;});h.off();};
 // ---- piano
 var held={},latch=false,hold=false,sustained={},kbBase=48;
 function pianoInit(){var sec=SS.sec('PLAY'),svg=sec&&$('svg[viewBox="0 0 504 56"]',sec);if(!svg)return;var rects=$$('rect',svg);
  var whites=rects.filter(function(r){return +r.getAttribute('width')>=12&&+r.getAttribute('height')>=50;}),blacks=rects.filter(function(r){return whites.indexOf(r)<0;});
  var WO=[0,2,4,5,7,9,11],base=36,keys={};
  whites.sort(function(a,b){return +a.getAttribute('x')-+b.getAttribute('x');});
  whites.forEach(function(r,i){var m=base+Math.floor(i/7)*12+WO[i%7];r.setAttribute('data-m',m);r.__o=r.getAttribute('fill');keys[m]=r;});
  blacks.forEach(function(r){var cx=+r.getAttribute('x')+(+r.getAttribute('width'))/2,wi=Math.floor(cx/14),m=base+Math.floor(wi/7)*12+WO[wi%7]+1;r.setAttribute('data-m',m);r.__o=r.getAttribute('fill');keys[m]=r;r.parentNode.appendChild(r);});
  rects.forEach(function(r){r.setAttribute('fill',r.__o);if(r.__o==='#FF5FA2')r.setAttribute('fill','#E9E4EF');});
  var vk=K('PLAY.Velocity');if(vk){vk.n0=100/127;vk.set(100/127,true);}
  function vel(){var k=K('PLAY.Velocity');return k?Math.max(1,Math.round(k.val())):100;}
  function paint(m,on){var r=keys[m];if(!r)return;r.setAttribute('fill',on?'#FF5FA2':r.__o);}
  function down(m){if(held[m]&&latch){up(m,true);return;}held[m]=1;paint(m,true);if(SS.page==='seq'&&SS.seqNote)SS.seqNote(m,vel());else AU.synthOn(m,vel());SS.emit('noteOn',m);}
  function up(m,force){if(!held[m])return;if(latch&&!force)return;delete held[m];paint(m,false);if(hold&&!force){sustained[m]=1;return;}if(SS.page==='seq'&&SS.seqNoteOff)SS.seqNoteOff(m);else AU.synthOff(m);}
  SS.pianoDown=down;SS.pianoUp=up;
  var cur=null,mouse=false;
  svg.addEventListener('pointerdown',function(e){var m=e.target.getAttribute&&e.target.getAttribute('data-m');if(m==null)return;e.preventDefault();mouse=true;cur=+m;svg.setPointerCapture(e.pointerId);down(cur);});
  svg.addEventListener('pointermove',function(e){if(!mouse)return;var el=document.elementFromPoint(e.clientX,e.clientY),m=el&&el.getAttribute&&el.getAttribute('data-m');if(m==null||+m===cur)return;if(!latch)up(cur);cur=+m;down(cur);});
  function end(){if(!mouse)return;mouse=false;if(cur!=null&&!latch)up(cur);cur=null;}
  svg.addEventListener('pointerup',end);svg.addEventListener('pointercancel',end);
  svg.style.cursor='pointer';svg.style.touchAction='none';
  // header buttons
  var hd=sec.firstElementChild.lastElementChild;function mk(t,tip,fn){var b=document.createElement('button');b.type='button';b.textContent=t;b.title=tip;b.setAttribute('data-tg2','1');b.setAttribute('style','height:24px;padding:0 8px;border-radius:6px;border:1px solid #4A4E57;background:#16171A;color:#ECEAE4;font:inherit;font-size:11px;font-weight:700;cursor:pointer;white-space:nowrap');b.addEventListener('click',function(){fn(b);});hd.appendChild(b);return b;}
  function lit(b,on){b.style.background=on?'#FF5FA2':'#16171A';b.style.color=on?'#141518':'#ECEAE4';b.style.borderColor=on?'#FF5FA2':'#4A4E57';}
  var oct=document.createElement('span');oct.setAttribute('style','font-size:11px;color:#B9B6AE;align-self:center;margin-right:4px');
  function octTxt(){oct.textContent='QWERTY: Z row = C'+(kbBase/12-2+0)+' · Q row = C'+(kbBase/12-1)+' · ← → octave';}
  octTxt();hd.appendChild(oct);
  mk('Latch','Notes stay on until clicked again',function(b){latch=!latch;lit(b,latch);if(!latch)Object.keys(held).forEach(function(m){up(+m,true);});});
  mk('Hold','Sustain: released notes keep ringing until Hold is turned off',function(b){hold=!hold;lit(b,hold);if(!hold){Object.keys(sustained).forEach(function(m){delete sustained[m];if(SS.page==='seq'&&SS.seqNoteOff)SS.seqNoteOff(+m);else AU.synthOff(+m);});}});
  mk('Panic','All notes off',function(){Object.keys(held).forEach(function(m){up(+m,true);});Object.keys(sustained).forEach(function(m){delete sustained[m];AU.synthOff(+m);});Object.keys(voices).forEach(function(m){AU.synthOff(+m);});});
  // QWERTY
  var low='zsxdcvgbhnjm,l.;/',high='q2w3er5t6y7ui9o0p',down2={};
  function mapKey(k){var i=low.indexOf(k);if(i>=0)return kbBase+i;i=high.indexOf(k);if(i>=0)return kbBase+12+i;return null;}
  document.addEventListener('keydown',function(e){if(e.target.closest&&e.target.closest('input,textarea,select'))return;if(e.metaKey||e.ctrlKey||e.altKey)return;
   if(e.key==='ArrowLeft'){kbBase=Math.max(24,kbBase-12);octTxt();return;}if(e.key==='ArrowRight'){kbBase=Math.min(72,kbBase+12);octTxt();return;}
   var m=mapKey(e.key.toLowerCase());if(m==null||e.repeat)return;e.preventDefault();down2[e.key.toLowerCase()]=m;down(m);});
  document.addEventListener('keyup',function(e){var k=e.key.toLowerCase(),m=down2[k];if(m==null)return;delete down2[k];if(!latch)up(m);});
 }
 SS.on('ready',function(){try{pianoInit();}catch(e){console.error('piano',e);}});
})();

// ===== SupurrSaw part 4: sequencer page =====
(function(){
 if(!window.SS||SS.page!=='seq')return;
 var $=SS.$,$$=SS.$$,clamp=SS.clamp,AU=SS.AU;
 function K(k){return SS.K[k];}
 var NS='http://www.w3.org/2000/svg',COL={n:'#9B7CFF',a:'#FF5FA2',s:'#3FE0D0'};
 var N=16,sel=2,cur='A1',steps=[],bpm=128,bank={},running=false,hold=false,transpose=0,play=-1;
 function blank(){return {on:false,s:0,oct:0,acc:false,sld:false,len:1,vel:100,pan:0,prob:100,roll:0};}
 function newPat(){var a=[],i;for(i=0;i<16;i++)a.push(blank());return a;}
 // semitone s (0..12) above bottom C; helper to build from compact note strings
 var SEMI={C:0,'C#':1,D:2,'D#':3,E:4,F:5,'F#':6,G:7,'G#':8,A:9,'A#':10,B:11};
 function mk(list,len){var a=newPat();list.forEach(function(x,i){if(!x)return;var st=a[i];st.on=true;st.s=SEMI[x[0]];st.oct=x[1]||0;st.acc=!!x[2];st.sld=!!x[3];st.vel=x[2]?127:100;});return {steps:a,len:len||16};}
 // A1: the demo pattern from the mockup
 (function(){var a=newPat(),d=[[0,12,'n'],[1,12,'n'],[2,0,'a'],[3,9,'n'],null,[5,12,'n'],[6,5,'a'],[7,2,'s'],[8,12,'n',2],null,[10,0,'s'],[11,12,'n'],[12,7,'n'],[13,5,'s'],[14,2,'a'],[15,0,'n']];
  d.forEach(function(x){if(!x)return;var st=a[x[0]];st.on=true;st.s=12-x[1];st.acc=x[2]==='a';st.sld=x[2]==='s';if(x[3])st.len=x[3];});
  var v=[32,24,40,28,3,24,40,32,28,3,36,24,28,32,40,28],rl=[0,0,0,.5,0,0,0,0,0,0,.75,0,0,0,.5,1],pr=[100,100,100,90,0,100,100,90,100,0,100,80,100,95,100,70];
  a.forEach(function(st,i){st.vel=Math.round(v[i]/40*127);st.roll=rl[i];st.prob=pr[i];st.pan=[0,0,-20,40,0,0,0,30,0,0,0,0,-30,0,50,0][i];});
  bank.A1={name:'Acid Squelch (demo)',bpm:128,steps:a,len:16};})();
 // Classic patterns as published on acidmachine.de (notes, octave flags D/U, accent, slide). Time mode is not applied: the site does not document it.
 var CL={
  A2:{name:'Access (DJ Tim & Misjah)',bpm:134,steps:mk([['A'],['A'],['A'],['A',1],['A',0,1],['A'],['A'],['A'],['A'],['A'],['A',1],['A'],['A',0,1],['A'],['A'],['A']])},
  A3:{name:'Higher State of Consciousness (Josh Wink)',bpm:120,steps:mk([['G',0,0,1],['G',-1],['G',0,0,1],['G',0,0,1],['G'],['G'],['B'],['G'],['B'],['G',0,0,1],['G'],['B'],['G',0,0,1],['G',0,0,1],['G'],['B']])},
  A4:{name:'Acid Morning (Boston 168)',bpm:120,steps:mk([['A',-1,1,1],['A',0,0,1],['A',-1,0,1],['A',0,0,1],['A',-1,1,1],['A',0,1],null,null,null,null,null,null,null,null,null,null])},
  B1:{name:'I Feel Love (Donna Summer)',bpm:120,steps:mk([['C',1,1],['C',1,1],['C',0,1],['C',1,1],['G',-1,1],['G',0,1],['A#',-1,1],['A#',0,1],null,null,null,null,null,null,null,null],8)},
  B2:{name:'Claustrophobic Sting (The Prodigy)',bpm:120,steps:mk([['F'],['F#',0,0,1],['F#'],['F#'],['F#',1,0,1],['C#'],['F#',1,1],['F',1,1],null,null,null,null,null,null,null,null],8)}};
 Object.keys(CL).forEach(function(k){var o=CL[k];if(o.steps&&o.steps.steps){o.len=o.steps.len;o.steps=o.steps.steps;}bank[k]=o;});
 ['B3','B4'].forEach(function(k){bank[k]={name:'Empty (your pattern)',bpm:128,steps:newPat(),len:16};});
 SS.bank=bank;
 function midi(st){return 36+st.s+12*st.oct;}
 function load(k){bank[cur].steps=steps;bank[cur].len=N;cur=k;var b=bank[k];steps=b.steps;N=b.len||16;bpm=b.bpm||128;sel=Math.min(sel,N-1);if(K('PATTERN.Steps')){K('PATTERN.Steps').set((N-1)/15,true);}bpmUp();redraw();selUI();nameUp();}
 steps=bank.A1.steps;
 // ---- DOM parts
 var secPat,secRoll,secSel,rollSvg,lanes={},accRows={},bpmEl,nameEl;
 function tip(t,x,y){var d=$('#ss-tip');if(!d){d=document.createElement('div');d.id='ss-tip';d.setAttribute('style','position:fixed;z-index:10003;background:#000;color:#fff;font:600 11px system-ui,sans-serif;padding:2px 6px;border-radius:4px;pointer-events:none');document.body.appendChild(d);}d.textContent=t;d.style.left=(x+12)+'px';d.style.top=(y-26)+'px';d.style.display='block';clearTimeout(d.__t);d.__t=setTimeout(function(){d.style.display='none';},900);}
 function el(tag,attrs,html){var e=document.createElementNS(NS,tag),k;for(k in attrs)e.setAttribute(k,attrs[k]);if(html)e.innerHTML=html;return e;}
 function redraw(){drawRoll();drawLanes();}
 function drawRoll(){var svg=rollSvg,o='',r,c,i;
  for(r=0;r<13;r++)for(c=0;c<16;c++)o+='<rect x="'+c*64+'" y="'+r*22+'" width="64" height="22" fill="'+(r%2?'#1B1822':'#17141C')+'" stroke="#2A2432" stroke-width=".5"/>';
  for(c=0;c<16;c+=4)o+='<line x1="'+c*64+'" x2="'+c*64+'" y1="0" y2="286" stroke="#3A3245" stroke-width="1.2"/>';
  if(N<16)o+='<rect x="'+N*64+'" y="0" width="'+(1024-N*64)+'" height="286" fill="#0A080D" fill-opacity=".62"/>';
  for(i=0;i<N;i++){var st=steps[i];if(!st.on||st.len===0)continue;var row=12-st.s,x=i*64+3,w=st.len*64-6,y=row*22+3,fill=st.acc?COL.a:(st.len>1?COL.n:(st.sld?COL.s:COL.n));
   o+='<rect data-i="'+i+'" x="'+x+'" y="'+y+'" width="'+w+'" height="16" rx="4" fill="'+fill+'" fill-opacity="'+(st.acc?1:.8)+'" stroke="'+(st.acc?'#fff':'none')+'" stroke-width="1.5"/>';
   if(st.sld)o+='<rect x="'+(x+w)+'" y="'+(y+6)+'" width="8" height="4" fill="#3FE0D0"/>';
   if(st.len>1)o+='<text x="'+(x+8)+'" y="'+(y+12)+'" font-size="10" fill="#fff" font-family="IBM Plex Mono,monospace">tie</text>';
   if(st.oct)o+='<text x="'+(x+w-14)+'" y="'+(y+12)+'" font-size="10" fill="#fff" font-family="sans-serif">'+(st.oct>0?'▲':'▼')+'</text>';
   if(st.roll>.1)o+='<text x="'+(x+6)+'" y="'+(y+12)+'" font-size="9" fill="#fff" font-family="sans-serif" opacity=".9">'+(st.len>1?'':'×'+(1+Math.round(st.roll*3)))+'</text>';}
  o+='<rect id="ph" x="0" y="0" width="64" height="286" fill="#B6F35A" fill-opacity=".16" style="display:none"/><rect id="sc" x="'+sel*64+'" y="0" width="64" height="286" fill="none" stroke="#FFB547" stroke-width="2"/>';
  svg.innerHTML=o;phUp();}
 function phUp(){var p=$('#ph',rollSvg);if(!p)return;if(play>=0){p.setAttribute('x',play*64);p.style.display='';}else p.style.display='none';}
 // lanes
 var LD={vel:{lab:'VEL',col:'#9B7CFF',min:0,max:127,def:100},pan:{lab:'PAN',col:'#3FE0D0',min:-100,max:100,def:0},prob:{lab:'PROB',col:'#FFB547',min:0,max:100,def:100},roll:{lab:'ROLL',col:'#FF5FA2',min:0,max:1,def:0}};
 function cellHtml(k,v){var d=LD[k],h;if(k==='pan'){var c=20,hh=Math.abs(v)/100*18;return '<div style="position:absolute;left:0;right:0;top:19px;height:2px;background:#3A3245"></div><div style="position:absolute;left:0;right:0;'+(v>=0?'bottom:'+(20)+'px':'top:20px')+';height:'+Math.max(hh,2)+'px;background:'+d.col+';border-radius:2px;opacity:'+(.45+Math.abs(v)/180)+'"></div>';}
  h=Math.max(2,(v-d.min)/(d.max-d.min)*40);return '<div style="position:absolute;left:0;right:0;bottom:0;height:'+h+'px;background:'+d.col+';border-radius:3px;opacity:'+(.4+.6*(v-d.min)/(d.max-d.min))+'"></div>';}
 function drawLane(k){var g=lanes[k],i,o='';for(i=0;i<16;i++){var st=steps[i];o+='<div data-c="'+i+'" style="position:relative;height:40px;background:#17141C;border-radius:3px;cursor:ns-resize;overflow:hidden;opacity:'+(i<N?1:.35)+(i===sel?';outline:1px solid #FFB547':'')+'">'+cellHtml(k,st[k])+'</div>';}g.innerHTML=o;}
 function drawLanes(){['vel','pan','prob','roll'].forEach(drawLane);drawChecks();}
 function setVal(i,k,v,fromKnob){var d=LD[k];v=clamp(v,d.min,d.max);if(k==='vel'||k==='pan'||k==='prob')v=Math.round(v);steps[i][k]=v;if(i===sel&&!fromKnob)syncKnobs();drawLane(k);if(k==='roll')drawRoll();}
 function laneInit(k,row){var g=row.children[1];lanes[k]=g;g.style.gap='4px';g.style.touchAction='none';
  if(k==='pan'){var lab=row.children[0];lab.style.position='relative';lab.style.height='40px';['R','C','L'].forEach(function(t,j){var s=document.createElement('span');s.textContent=t;s.setAttribute('style','position:absolute;right:6px;font-size:8px;color:#8C8399;'+(j===0?'top:0':j===1?'top:16px':'bottom:0'));lab.appendChild(s);});}
  g.addEventListener('pointerdown',function(e){var c=e.target.closest('[data-c]');if(!c)return;e.preventDefault();g.setPointerCapture(e.pointerId);
   function at(ev){var cells=$$('[data-c]',g),i=-1,j;for(j=0;j<cells.length;j++){var r=cells[j].getBoundingClientRect();if(ev.clientX>=r.left&&ev.clientX<=r.right){i=j;var f=1-clamp((ev.clientY-r.top)/r.height,0,1),d=LD[k],v=d.min+f*(d.max-d.min);if(k==='pan'&&Math.abs(v)<8)v=0;sel=i;setVal(i,k,v);tip(k.toUpperCase()+' '+(k==='roll'?(1+Math.round(steps[i].roll*3))+'×':Math.round(steps[i][k])),ev.clientX,ev.clientY);selUI();moveSel();break;}}}
   function up(){g.removeEventListener('pointermove',at);g.removeEventListener('pointerup',up);}
   at(e);g.addEventListener('pointermove',at);g.addEventListener('pointerup',up);});
  g.addEventListener('dblclick',function(e){var c=e.target.closest('[data-c]');if(!c)return;var i=+c.getAttribute('data-c');setVal(i,k,LD[k].def);tip(k.toUpperCase()+' reset',e.clientX,e.clientY);});}
 function moveSel(){var s=$('#sc',rollSvg);if(s)s.setAttribute('x',sel*64);}
 // accent / slide / tie checkboxes
 function drawChecks(){['acc','sld','tie'].forEach(function(k){var g=accRows[k],o='',i;for(i=0;i<16;i++){var st=steps[i],on=k==='acc'?st.acc:k==='sld'?st.sld:st.len>1,ok=st.on&&i<N,col=k==='acc'?COL.a:k==='sld'?COL.s:COL.n;
   o+='<div data-c="'+i+'" style="display:flex;justify-content:center;align-items:center;opacity:'+(ok?1:.3)+'"><div style="width:16px;height:16px;border-radius:4px;border:1.5px solid '+col+';background:'+(on?col:'transparent')+';display:flex;align-items:center;justify-content:center;color:#141518;font-size:12px;font-weight:700;cursor:'+(ok?'pointer':'default')+'">'+(on?'✓':'')+'</div></div>';}g.innerHTML=o;});}
 function checkInit(){var after=secRoll.children[1],defs=[['acc','ACC','accent',COL.a],['sld','SLD','slide',COL.s],['tie','TIE','tie',COL.n]],prev=after;
  defs.forEach(function(d){var row=document.createElement('div');row.setAttribute('style','display:flex;gap:0;align-items:center;height:20px');var lab=document.createElement('div');lab.setAttribute('style','width:44px;flex:none;font-size:10px;color:'+d[3]);lab.textContent=d[1];lab.title=d[2]+' per note';
   var g=document.createElement('div');g.setAttribute('style','flex:1;display:grid;grid-template-columns:repeat(16,1fr);gap:4px');row.appendChild(lab);row.appendChild(g);prev.insertAdjacentElement('afterend',row);prev=row;accRows[d[0]]=g;
   g.addEventListener('click',function(e){var c=e.target.closest('[data-c]');if(!c)return;var i=+c.getAttribute('data-c'),st=steps[i];if(!st.on||i>=N)return;
    if(d[0]==='acc'){st.acc=!st.acc;if(st.acc)st.vel=Math.max(st.vel,115);}else if(d[0]==='sld')st.sld=!st.sld;else{if(st.len>1)st.len=1;else if(i<N-1){st.len=2;steps[i+1].on=false;}}sel=i;redraw();selUI();});});}
 // piano roll interaction
 function rollInit(){var svg=rollSvg;svg.style.height='286px';svg.style.cursor='crosshair';svg.style.touchAction='none';
  function pt(e){var p=svg.createSVGPoint();p.x=e.clientX;p.y=e.clientY;return p.matrixTransform(svg.getScreenCTM().inverse());}
  function owner(c){var i;for(i=c;i>=0;i--){var s=steps[i];if(s.on&&i+s.len>c)return i;if(s.on)break;}return -1;}
  svg.addEventListener('pointerdown',function(e){var p=pt(e),c=clamp(Math.floor(p.x/64),0,15),r=clamp(Math.floor(p.y/22),0,12);if(c>=N)return;e.preventDefault();svg.setPointerCapture(e.pointerId);
   var o=owner(c),i=o>=0?o:c;sel=i;
   if(o<0){var st=steps[c];st.on=true;st.s=12-r;st.len=1;st.oct=0;st.acc=e.shiftKey;st.sld=e.altKey;if(e.shiftKey)st.vel=127;redraw();SS.seqNote&&SS.seqNote(midi(st),st.vel,true);}
   else{var s=steps[o];if(e.shiftKey){s.acc=!s.acc;}else if(e.altKey){s.sld=!s.sld;}redraw();SS.seqNote&&SS.seqNote(midi(s),s.vel,true);}
   selUI();var s0=steps[i],r0=12-s0.s;
   function mv(ev){var q=pt(ev),rr=clamp(Math.floor(q.y/22),0,12);if(12-rr!==steps[i].s){steps[i].s=12-rr;drawRoll();SS.seqNote&&SS.seqNote(midi(steps[i]),steps[i].vel,true);}}
   function up(){svg.removeEventListener('pointermove',mv);svg.removeEventListener('pointerup',up);}
   svg.addEventListener('pointermove',mv);svg.addEventListener('pointerup',up);});
  svg.addEventListener('dblclick',function(e){var p=pt(e),c=clamp(Math.floor(p.x/64),0,15),o=owner(c);if(o>=0){steps[o]=Object.assign(blank(),{vel:steps[o].vel});redraw();selUI();}});
  svg.addEventListener('contextmenu',function(e){e.preventDefault();var p=pt(e),c=clamp(Math.floor(p.x/64),0,15),o=owner(c);if(o>=0){steps[o]=blank();redraw();}});}
 // selected note panel
 var selSeg,syncing=false;
 function selUI(){var h=secSel.firstElementChild.firstElementChild;h.textContent='SELECTED NOTE · step '+(sel+1);var st=steps[sel];
  if(selSeg){syncing=true;selSeg.set(st.len>1?3:st.acc?1:st.sld?2:0,true);syncing=false;}syncKnobs();moveSel();$$('[data-c]',lanes.vel).forEach(function(c,i){c.style.outline=i===sel?'1px solid #FFB547':'';});}
 function syncKnobs(){var st=steps[sel];K('SELECTED NOTE.Velocity').set(st.vel/127,true);K('SELECTED NOTE.Pan').set((st.pan+100)/200,true);K('SELECTED NOTE.Prob').set(st.prob/100,true);K('SELECTED NOTE.Roll').set(st.roll,true);}
 function selInit(){selSeg=SS.seg['SELECTED NOTE#0'];selSeg.el.addEventListener('click',function(){if(syncing)return;var st=steps[sel],i=selSeg.idx;if(!st.on){st.on=true;st.s=6;}
   st.acc=i===1;st.sld=i===2;if(i===3&&sel<N-1){st.len=2;steps[sel+1].on=false;}else st.len=1;if(i===1)st.vel=Math.max(st.vel,115);redraw();});
  [['Velocity','vel',function(n){return n*127;}],['Pan','pan',function(n){return n*200-100;}],['Prob','prob',function(n){return n*100;}],['Roll','roll',function(n){return n;}]].forEach(function(a){SS.on('k:SELECTED NOTE.'+a[0],function(n){setVal(sel,a[1],a[2](n),true);});});
  // octave buttons
  var note=secSel.lastElementChild,wrap=document.createElement('div');wrap.setAttribute('style','display:flex;gap:6px;align-items:center;font-size:12px');wrap.innerHTML='<span style="color:#B9B6AE">Octave</span>';
  ['▼','▲'].forEach(function(t,j){var b=document.createElement('button');b.type='button';b.textContent=t;b.title=j?'Octave up (U)':'Octave down (D)';b.setAttribute('style','height:24px;padding:0 8px;border-radius:6px;border:1px solid #4A4E57;background:#16171A;color:#ECEAE4;font:inherit;font-size:11px;font-weight:700;cursor:pointer');b.addEventListener('click',function(){var st=steps[sel];if(!st.on)return;st.oct=clamp(st.oct+(j?1:-1),-1,1);redraw();SS.seqNote&&SS.seqNote(midi(st),st.vel,true);});wrap.appendChild(b);});
  var del=document.createElement('button');del.type='button';del.textContent='Clear step';del.setAttribute('style','height:24px;padding:0 8px;border-radius:6px;border:1px solid #4A4E57;background:#16171A;color:#ECEAE4;font:inherit;font-size:11px;font-weight:700;cursor:pointer');del.addEventListener('click',function(){steps[sel]=blank();redraw();selUI();});wrap.appendChild(del);
  note.insertAdjacentElement('beforebegin',wrap);note.textContent='Click an empty cell to add a note, drag a note up/down to change pitch, double-click to delete. Shift+click = accent, Alt+click = slide.';}
 // ---- 303 voice
 var V=null;
 function voice(){var c=AU.ctx();if(!c)return null;if(V)return V;var o=c.createOscillator(),f1=c.createBiquadFilter(),f2=c.createBiquadFilter(),g=c.createGain();o.type='sawtooth';f1.type=f2.type='lowpass';g.gain.value=0;o.connect(f1);f1.connect(f2);f2.connect(g);g.connect(AU.in);o.start();V={o:o,f1:f1,f2:f2,g:g,last:0,last_f:0};return V;}
 var classic=false;SS.acid={};
 function avals(){var sg=SS.seg['ACID VOICE#0'];return {wave:sg&&sg.idx===1?'square':'sawtooth',cut:K('ACID VOICE.Cutoff').val(),res:K('ACID VOICE.Reso').n,env:K('ACID VOICE.Env mod').n,dec:K('ACID VOICE.Decay').val()/1000,acc:K('ACID VOICE.Accent').n,sl:K('ACID VOICE.Slide').val()/1000,tune:K('ACID VOICE.Tuning').val()};}
 // trig(midi, accent, slideIn, t, gateLen, vel)
 function trig(m,acc,slideIn,t,gate,vel){var v=voice();if(!v)return;var a=avals(),c=AU.ctx(),f=440*Math.pow(2,(m+a.tune-69)/12);v.o.type=a.wave;
  v.f1.Q.value=v.f2.Q.value=.5+a.res*(classic?14:10);
  if(slideIn&&v.last>0){v.o.frequency.cancelScheduledValues(t);v.o.frequency.setValueAtTime(v.o.frequency.value,t);v.o.frequency.linearRampToValueAtTime(f,t+Math.max(.01,a.sl));}
  else{v.o.frequency.cancelScheduledValues(t);v.o.frequency.setValueAtTime(f,t);
   var base=a.cut,peak=clamp(base*Math.pow(2,a.env*(classic?5.5:4.5)*(acc?1+a.acc*.5:1)),20,16000),dec=a.dec*(acc?.5:1)*(classic?1:1);
   [v.f1,v.f2].forEach(function(F){F.frequency.cancelScheduledValues(t);F.frequency.setValueAtTime(peak,t);F.frequency.setTargetAtTime(base,t+.004,Math.max(.02,dec/3));});
   v.g.gain.cancelScheduledValues(t);v.g.gain.setTargetAtTime((acc?.55+.45*a.acc:.5)*(vel/127+.35),t,.002);}
  v.last=f;
  if(gate>0&&!slideIn||gate>0){v.relAt=t+gate;}
  return v;}
 function release(t){var v=V;if(!v)return;v.g.gain.setTargetAtTime(0,t,.012);v.last=0;}
 var held303=null;
 SS.seqNote=function(m,vel,short){var c=AU.ctx();if(!c)return;if(short){trig(m,false,false,c.currentTime,.12,vel||100);release(c.currentTime+.12);return;}
  if(running&&!hold){transpose=m-48;return;}trig(m,false,false,c.currentTime,0,vel||100);held303=m;};
 SS.seqNoteOff=function(m){if(held303===m&&!(running&&!hold)){release(AU.ctx().currentTime);held303=null;}};
 // ---- scheduler
 var timer=null,nextT=0,stepI=0,chainList=[],chainPos=0,chainRep=0,uiq=[];
 function parseChain(txt){var out=[],re=/([AB][1-4])(?:\s*×\s*(\d+))?/g,m;while((m=re.exec(txt)))out.push([m[1],parseInt(m[2]||'1')]);return out;}
 function stepDur(){return 60/bpm/4;}
 function swingOff(i){var s=K('PATTERN.Swing').val();return i%2?clamp((s-50)/100,0,.5)*2*stepDur()*.5:0;}
 function schedule(){var c=AU.ctx(),horizon=c.currentTime+.12;while(nextT<horizon){var i=stepI,st=steps[i],t=nextT+swingOff(i),sd=stepDur();
   if(!hold&&st.on&&Math.random()*100<st.prob){var prevSl=false,j=(i+N-1)%N;if(steps[j].on&&steps[j].sld&&i>0)prevSl=true;
    var rolls=1+Math.round(st.roll*3),len=st.len,gate=len*sd*(st.sld?1.02:.55);
    if(rolls>1&&len===1){var sub=sd/rolls,r;for(r=0;r<rolls;r++){trig(midi(st)+transpose,st.acc,false,t+r*sub,sub*.5,st.vel);release(t+r*sub+sub*.5);}}
    else{trig(midi(st)+transpose,st.acc,prevSl,t,gate,st.vel);if(!st.sld)release(t+gate);}}
   else if(!hold){var pj=(i+N-1)%N;if(!(steps[pj].on&&steps[pj].sld))release(t);}
   uiq.push([t,i]);
   stepI++;nextT+=sd;
   if(stepI>=N){stepI=0;var mu=K('PATTERN.Mutate').n;if(mu>0)mutate(mu);advanceChain();}}
  var now=c.currentTime;while(uiq.length&&uiq[0][0]<=now+.02){play=uiq.shift()[1];phUp();}}
 function mutate(p){steps.forEach(function(st,i){if(i<N&&st.on&&Math.random()<p*.25){st.s=clamp(st.s+(Math.random()<.5?-1:1)*(1+Math.floor(Math.random()*3)),0,12);}if(i<N&&Math.random()<p*.06)st.on=!st.on&&Math.random()<.5?true:st.on;});drawRoll();}
 function advanceChain(){if(!chainList.length)return;chainRep++;if(chainRep>=chainList[chainPos][1]){chainRep=0;chainPos=(chainPos+1)%chainList.length;var k=chainList[chainPos][0];if(bank[k]){selectSlot(k);}}}
 var runBtn,patBtns={};
 function start(){var c=AU.ctx();if(!c)return;voice();running=true;stepI=0;nextT=c.currentTime+.06;chainList=parseChain(chainTxt.textContent);chainPos=Math.max(0,chainList.map(function(x){return x[0];}).indexOf(cur));chainRep=0;timer=setInterval(schedule,25);runBtn.textContent='■ Stop';runBtn.style.background='#FF5F5F';runBtn.style.borderColor='#FF5F5F';}
 function stop(){running=false;clearInterval(timer);timer=null;uiq=[];play=-1;phUp();if(V)release(AU.ctx().currentTime);transpose=0;runBtn.textContent='▶ Run';runBtn.style.background='#B6F35A';runBtn.style.borderColor='#B6F35A';}
 var chainTxt;
 // ---- pattern slots
 function selectSlot(k){Object.keys(patBtns).forEach(function(n){var b=patBtns[n],on=n===k;b.style.background=on?'#FF5FA2':'#16171A';b.style.borderColor=on?'#FF5FA2':'#4A4E57';b.style.color=on?'#141518':'#ECEAE4';});load(k);}
 function nameUp(){nameEl.textContent=cur+' · '+bank[cur].name;}
 function bpmUp(){bpmEl.textContent=bpm+' BPM';}
 function pats(){secPat=SS.sec('PATTERN');var row=secPat.children[1];$$('button',row).forEach(function(b){var t=b.textContent.trim();if(/^[AB][1-4]$/.test(t)){patBtns[t]=b;b.addEventListener('click',function(){selectSlot(t);});}
   else if(t==='Edit chain'){b.addEventListener('click',function(e){var r=b.getBoundingClientRect();SS.menu(r.left,r.bottom+2,['A1 ×4 → A2 ×2 → A1 → A3 (fill)','A2 ×4','A3 ×2 → A4 ×2','B1 ×2 → B2 ×2','A1 → A2 → A3 → A4 → B1 → B2'],function(it){chainTxt.textContent='Chain: '+it;});});}
   else if(t==='Hold note (seq off)'){b.addEventListener('click',function(){hold=!hold;b.style.background=hold?'#FF5FA2':'#16171A';b.style.color=hold?'#141518':'#ECEAE4';b.style.borderColor=hold?'#FF5FA2':'#4A4E57';if(hold&&V)release(AU.ctx().currentTime);});}
   else if(/Run/.test(t)){runBtn=b;b.addEventListener('click',function(){running?stop():start();});}
   else if(/^Chain/.test(t)){} });
  chainTxt=$$('span',row).filter(function(s){return /^Chain:/.test(s.textContent);})[0];
  nameEl=document.createElement('span');nameEl.setAttribute('style','font-size:12px;color:#3FE0D0;margin-left:8px;font-family:IBM Plex Mono,monospace');chainTxt.insertAdjacentElement('afterend',nameEl);
  bpmEl=document.createElement('span');bpmEl.title='Drag or scroll to change tempo, double-click to reset';bpmEl.setAttribute('style','font:700 12px IBM Plex Mono,monospace;color:#ECEAE4;padding:3px 8px;border:1px solid #4A4E57;border-radius:6px;background:#16171A;cursor:ns-resize;user-select:none;touch-action:none');runBtn.insertAdjacentElement('beforebegin',bpmEl);
  bpmEl.addEventListener('pointerdown',function(e){e.preventDefault();bpmEl.setPointerCapture(e.pointerId);var y0=e.clientY,b0=bpm;function mv(m){bpm=clamp(Math.round(b0+(y0-m.clientY)/3),60,200);bank[cur].bpm=bpm;bpmUp();}function up(){bpmEl.removeEventListener('pointermove',mv);bpmEl.removeEventListener('pointerup',up);}bpmEl.addEventListener('pointermove',mv);bpmEl.addEventListener('pointerup',up);});
  bpmEl.addEventListener('wheel',function(e){e.preventDefault();bpm=clamp(bpm-Math.sign(e.deltaY),60,200);bpmUp();},{passive:false});
  bpmEl.addEventListener('dblclick',function(){bpm=bank[cur].def||bpm;});
  Object.keys(bank).forEach(function(k){bank[k].def=bank[k].bpm;});
  SS.on('k:PATTERN.Steps',function(n){N=Math.round(1+n*15);bank[cur].len=N;sel=Math.min(sel,N-1);redraw();selUI();});}
 // ---- random
 var SCALES={'Minor pent.':[0,3,5,7,10],'Major pent.':[0,2,4,7,9],'Minor':[0,2,3,5,7,8,10],'Major':[0,2,4,5,7,9,11],'Phrygian':[0,1,3,5,7,8,10],'Dorian':[0,2,3,5,7,9,10],'Blues':[0,3,5,6,7,10],'Harmonic min.':[0,2,3,5,7,8,11],'Chromatic':[0,1,2,3,4,5,6,7,8,9,10,11,12]};
 function dice(){var pN=K('RANDOM.Notes').n,pA=K('RANDOM.Accents').n,pS=K('RANDOM.Slides').n,oc=Math.round(K('RANDOM.Octave').val()),seg=SS.seg['RANDOM#0'],free=seg.idx===0,sc=SCALES[ddText.replace(/\s*▾\s*$/,'')]||SCALES['Minor pent.'],a=newPat(),i,cnt=0;
  for(i=0;i<N;i++){if(Math.random()<pN||(i===0)){var st=a[i];st.on=true;st.s=free?Math.floor(Math.random()*13):sc[Math.floor(Math.random()*sc.length)];st.oct=oc?(Math.random()<.3?(Math.random()<.5?-1:1)*Math.min(1,oc):0):0;st.acc=Math.random()<pA;st.sld=Math.random()<pS;st.vel=st.acc?127:80+Math.floor(Math.random()*30);cnt++;}}
  for(i=N;i<16;i++)a[i]=blank();steps=a;bank[cur].steps=a;sel=0;redraw();selUI();SS.toast('Rolled '+cnt+' notes ('+(free?'free':ddText.replace(/\s*▾\s*$/,''))+')');if(!running)SS.seqNote&&SS.seqNote(midi(a[0]),a[0].vel,true);}
 var ddText='Minor pent. ▾';
 function randInit(){var s=SS.sec('RANDOM'),btn=$$('button',s).filter(function(b){return b.textContent.trim()==='Roll dice';})[0];btn.addEventListener('click',function(){btn.textContent='🎲 Rolled';setTimeout(function(){btn.textContent='Roll dice';},500);dice();});
  var dd=SS.ddOf(s)[0];dd.setAttribute('data-dd','1');dd.style.cursor='pointer';dd.addEventListener('click',function(e){e.stopPropagation();var r=dd.getBoundingClientRect();SS.menu(r.left,r.bottom+2,Object.keys(SCALES),function(it){ddText=it+' ▾';dd.textContent=ddText;SS.seg['RANDOM#0'].set(1);});});}
 // ---- acid voice
 function acidInit(){var s=SS.sec('ACID VOICE'),b=$$('button',s).filter(function(x){return /Classic 303/.test(x.textContent);})[0];var onS=b.getAttribute('style'),offS=onS.replace(/background:[^;]+;/,'background:#16171A;').replace(/color:[^;]+;/,'color:#FFB547;');
  function set(v){classic=v;b.setAttribute('style',v?onS:offS);}set(true);b.addEventListener('click',function(){set(!classic);SS.toast(classic?'Classic 303 curves: 4-pole sweep, longer accent peak':'Smooth curves: 2-pole, gentler sweep');});
  ['Cutoff','Reso','Env mod','Decay','Accent'].forEach(function(n){SS.on('k:ACID VOICE.'+n,function(){if(V){var a=avals();V.f1.Q.value=V.f2.Q.value=.5+a.res*(classic?14:10);}});});}
 // ---- MIDI export / import
 function vlq(n){var b=[n&127];while((n>>=7))b.unshift((n&127)|128);return b;}
 function exportMidi(){var ev=[],PPQ=480,T=120,i,trk=[];
  for(i=0;i<N;i++){var st=steps[i];if(!st.on||st.len===0)continue;ev.push([i*T,0x90,midi(st),st.acc?127:st.vel]);ev.push([(i+st.len)*T-(st.sld?0:T*.45),0x80,midi(st),0]);}
  ev.sort(function(a,b){return a[0]-b[0]||(a[1]===0x80?-1:1);});var last=0;
  var tempo=Math.round(60000000/bpm);trk.push(0,0xFF,0x51,3,(tempo>>16)&255,(tempo>>8)&255,tempo&255);
  ev.forEach(function(e){var t=Math.round(e[0]);trk=trk.concat(vlq(t-last),[e[1],e[2],e[3]]);last=t;});trk=trk.concat([0,0xFF,0x2F,0]);
  var hdr=[0x4d,0x54,0x68,0x64,0,0,0,6,0,0,0,1,PPQ>>8,PPQ&255],th=[0x4d,0x54,0x72,0x6b,(trk.length>>24)&255,(trk.length>>16)&255,(trk.length>>8)&255,trk.length&255];
  var blob=new Blob([new Uint8Array(hdr.concat(th,trk))],{type:'audio/midi'}),a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download='supurrsaw-'+cur+'.mid';document.body.appendChild(a);a.click();setTimeout(function(){a.remove();},100);SS.toast('Exported '+cur+' as .mid (downloads are disabled in the web preview)');}
 function importMidi(buf){var d=new Uint8Array(buf),p=14,ppq=(d[12]<<8)|d[13],notes=[],tr=0;
  function rd(n){var v=0;while(n--)v=v*256+d[p++];return v;}
  while(p<d.length-8){var id=String.fromCharCode(d[p],d[p+1],d[p+2],d[p+3]);p+=4;var len=rd(4),end=p+len;if(id!=='MTrk'){p=end;continue;}var t=0,rs=0;
   while(p<end){var dt=0,b;do{b=d[p++];dt=(dt<<7)|(b&127);}while(b&128);t+=dt;var s=d[p];if(s===0xFF){p+=2+d[p+1]+0;var ln=d[p-1];p=p;continue;}
    if(s&128){rs=s;p++;}var hi=rs&0xF0;if(hi===0x90||hi===0x80){var n=d[p++],v=d[p++];if(hi===0x90&&v>0)notes.push([t/ppq*4,n,v]);}else if(hi===0xC0||hi===0xD0)p++;else p+=2;}
   p=end;tr++;}
  return notes;}
 function importInit(){var b=$$('button',secRoll).filter(function(x){return /Import/.test(x.textContent);})[0],e=$$('button',secRoll).filter(function(x){return /Export/.test(x.textContent);})[0];
  var inp=document.createElement('input');inp.type='file';inp.accept='.mid,.midi';inp.style.display='none';document.body.appendChild(inp);b.addEventListener('click',function(){inp.click();});e.addEventListener('click',exportMidi);
  inp.addEventListener('change',function(){var f=inp.files[0];if(!f)return;var r=new FileReader();r.onload=function(){try{var ns=importMidi(r.result),a=newPat();ns.forEach(function(x){var i=Math.round(x[0]);if(i<0||i>15||a[i].on)return;var n=x[1]-36,s=((n%12)+12)%12,o=Math.floor(n/12);a[i].on=true;a[i].s=s;a[i].oct=clamp(o,-1,1);a[i].vel=x[2];a[i].acc=x[2]>=120;});
    steps=a;bank[cur].steps=a;redraw();selUI();SS.toast('Imported '+ns.length+' notes into '+cur);}catch(err){SS.toast('Could not read that MIDI file');}};r.readAsArrayBuffer(f);inp.value='';});}
 SS.on('ready',function(){try{
  secRoll=SS.sec('PIANO ROLL');secSel=SS.sec('SELECTED NOTE');rollSvg=$('svg[viewBox="0 0 1024 286"]',secRoll);
  pats();checkInit();
  ['vel','pan','prob','roll'].forEach(function(k,i){laneInit(k,secRoll.children[2+i+3]);});
  rollInit();selInit();randInit();acidInit();importInit();
  // top bar preset arrows
  bpm=bank.A1.bpm;selectSlot('A1');}catch(e){console.error('seq',e);}});
})();

// ===== SupurrSaw part 5: presets and top bar =====
(function(){
 if(!window.SS)return;
 var $=SS.$,$$=SS.$$,clamp=SS.clamp;
 var CAT={'Trance Lead':['Euphoria Lead','Uplift Lead','Sunrise Supersaw','Anthem Lead','Rave Stab Lead','Wide Saw Lead'],
  'Trance Pluck':['Gate Pluck','Supersaw Pluck','Short Pluck','Bell Pluck','Detuned Pluck','Arp Pluck'],
  'Bass':['Rolling Bass','Hardstyle Rumble','Acid Sub','Off-beat Bass','Sub 808','Pulteq Low Bass'],
  'Hoover':['Juno Hoover A','Juno Hoover B','Mentasm Stab','Rave Hoover Wide','Hardcore Hoover','Hoover Bass'],
  'Reese':['Dark Reese','Neuro Reese','Wide Reese','Rolling Reese','Grind Reese','Sub Reese'],
  'Acid 303':['Acid Squelch','Higher Acid','Classic Slide','Resonant Wail','Pulse Acid','Dirty Acid'],
  'Pad':['Glass Pad','Choir Pad','Warm Pad','Air Pad','Dark Pad','Rise Pad'],
  'FX':['Riser','Downlifter','Impact','Sweep','Noise Rise','Laser'],
  'Hardstyle Lead':['Screech Lead','Euphoric Hardstyle','Reverse Bass Lead','Gammer Style Lead','Rawstyle Saw','Anthem Pitch Lead'],
  'UK Hardcore':['Hardcore Stab','Piano Hoover','Rave Screech','Technikore Lead','Scott Brown Stab','OTT Dance Lead'],
  'All':['Euphoria Lead','Juno Hoover A','Acid Squelch','Hardcore Stab','Rolling Bass','Glass Pad'],'Favorites':['Euphoria Lead','Acid Squelch','Hardcore Hoover']};
 var ALL=[];Object.keys(CAT).forEach(function(c){if(c!=='All'&&c!=='Favorites')CAT[c].forEach(function(n){if(ALL.indexOf(n)<0)ALL.push(n);});});SS.PRESETS=ALL;
 var idx=ALL.indexOf('Euphoria Lead');try{var v=sessionStorage.getItem('ss.preset');if(v!==null&&ALL.indexOf(v)>=0)idx=ALL.indexOf(v);}catch(e){}
 function pad(n){return ('00'+(n+1)).slice(-3);}
 function hash(s){var h=0,i;for(i=0;i<s.length;i++)h=(h*31+s.charCodeAt(i))|0;return Math.abs(h);}
 function rnd(h,k){var x=Math.sin(h*12.9898+k*78.233)*43758.5453;return x-Math.floor(x);}
 function apply(name,quiet){if(SS.page!=='synth')return;var h=hash(name),K=SS.K,L=name.toLowerCase();
  function set(k,n){if(K[k])K[k].set(clamp(n,0,1));}
  var pluck=/pluck|stab|gate|arp/.test(L),pad=/pad|rise|riser|sweep/.test(L),bass=/bass|sub|reese|rumble/.test(L),hoov=/hoover|screech|rave/.test(L),acid=/acid|squelch/.test(L);
  set('LAYER A.Detune',.25+rnd(h,1)*.6);set('LAYER A.Voices',.35+rnd(h,2)*.65);set('LAYER B.Detune',.2+rnd(h,3)*.6);set('LAYER B.Voices',.3+rnd(h,4)*.6);
  set('FILTER.Cutoff',bass?.35+rnd(h,5)*.15:.55+rnd(h,5)*.3);set('FILTER.Reso',.1+rnd(h,6)*(acid?.7:.35));
  set('AMP ENV.A',pad?.45+rnd(h,7)*.3:.05+rnd(h,7)*.15);set('AMP ENV.D',pluck?.25+rnd(h,8)*.2:.5+rnd(h,8)*.3);set('AMP ENV.S',pluck?.1+rnd(h,9)*.2:.55+rnd(h,9)*.4);set('AMP ENV.R',pad?.6+rnd(h,10)*.3:.25+rnd(h,10)*.25);
  set('WIDTH.Spread',bass?.2+rnd(h,11)*.2:.5+rnd(h,11)*.45);set('PITCH ENV.Amount',hoov?.5+.5*(.6+rnd(h,12)*.4):.5);
  var pe=SS.seg['tg:PITCH ENV'];if(pe)pe.set(hoov);var sb=SS.seg['tg:SUB OSC'];if(sb)sb.set(bass||hoov||acid);
  ['Distortion','Chorus','Phaser','Delay','Reverb'].forEach(function(n,i){var o=SS.seg['tg:FX '+n];if(o)o.set(rnd(h,20+i)>(pad?.25:.5));var m=K['FX '+n+'.Mix'];if(m)m.set(.2+rnd(h,30+i)*.5);});
  if(SS.AU&&SS.AU.applyFx)SS.AU.applyFx();}
 SS.applyPreset=apply;
 function nav(){var box=$$('header div').filter(function(d){return /◂\s*\d+/.test(d.textContent)&&d.children.length===0;})[0];if(!box)return;
  box.innerHTML='<span data-pv style="cursor:pointer;padding:0 4px">◂</span><span data-nm style="cursor:pointer"></span><span data-nx style="cursor:pointer;padding:0 4px">▸</span>';
  var nm=$('[data-nm]',box);function show(){nm.textContent=pad(idx)+' '+ALL[idx];}
  function go(i,q){idx=(i+ALL.length)%ALL.length;show();try{sessionStorage.setItem('ss.preset',ALL[idx]);}catch(e){}apply(ALL[idx]);if(!q)SS.toast('Preset: '+ALL[idx]);}
  $('[data-pv]',box).addEventListener('click',function(){go(idx-1);});$('[data-nx]',box).addEventListener('click',function(){go(idx+1);});
  nm.addEventListener('click',function(e){var r=box.getBoundingClientRect();SS.menu(r.left,r.bottom+2,ALL.map(function(n,i){return pad(i)+' '+n;}),function(it){go(parseInt(it)-1);});});
  SS.setPreset=function(name){var i=ALL.indexOf(name);if(i>=0)go(i);};show();
  if(SS.page==='synth'){try{if(sessionStorage.getItem('ss.preset')!==null)apply(ALL[idx]);}catch(e){}}
  // seq: preset names map to nothing but keep list
 }
 function presetsPage(){var secs=$$('section');if(secs.length<4)return;var cats=secs[0],list=secs[1],prev=secs[2],srch=secs[3];
  var cbox=cats.children[1],catRows=[].slice.call(cbox.children);
  ['Hardstyle Lead','UK Hardcore'].forEach(function(n,i){var c=catRows[0].cloneNode(true);c.children[0].textContent=n;c.children[1].textContent=[38,33][i];cbox.insertBefore(c,catRows[catRows.length-1]);});
  catRows=[].slice.call(cbox.children);
  var box=list.children[1],rowT=box.children[0].cloneNode(true),inactiveRowStyle=box.children[1].getAttribute('style'),activeRowStyle=box.children[0].getAttribute('style');
  var title=list.firstElementChild.firstElementChild,curCat='Hoover',selName=null,query='',tags={},favs={};
  var desc={'Juno Hoover A':'3 saws, octave apart, PWM, chorus','Juno Hoover B':'Saw + pulse + sub, PWM, chorus'};
  function d(n){return desc[n]||('Tuned for dance: '+['tight low end','wide supersaw','OTT-ready','mono-safe bass','forward mids'][hash(n)%5]);}
  function render(){var names=(CAT[curCat]||[]).filter(function(n){return (!query||n.toLowerCase().indexOf(query)>=0);});
   $$('[data-pr]',box).forEach(function(x){x.remove();});[].slice.call(box.children).forEach(function(x){if(!x.hasAttribute('data-pr'))x.remove();});
   names.forEach(function(n,i){var r=rowT.cloneNode(true);r.setAttribute('data-pr','1');r.children[0].textContent=n+(favs[n]?' ★':'');r.children[1].textContent=d(n);r.setAttribute('style',n===selName?activeRowStyle:inactiveRowStyle);r.style.cursor='pointer';
    r.addEventListener('click',function(){selName=n;render();preview(n);});box.appendChild(r);});
   title.textContent=curCat.toUpperCase()+' · '+names.length+' presets';}
  var pvKnobs=$$('svg[data-kn]',prev),pvText=$$('p,div,span',prev).filter(function(x){return /^Starting point/.test(x.textContent)&&x.children.length===0;})[0];
  function preview(n){var h=hash(n);pvKnobs.forEach(function(k,i){var kk=SS.K[k.getAttribute('data-kn')===1?'':Object.keys(SS.K).filter(function(q){return SS.K[q].svg===k;})[0]];if(kk)kk.set(rnd(h,i+1));});if(pvText)pvText.textContent='Starting point: '+n+'. '+d(n)+'.';}
  catRows.forEach(function(r){r.style.cursor='pointer';r.addEventListener('click',function(){var act=catRows.filter(function(x){return /background/.test(x.getAttribute('style')||'');})[0];if(act&&act!==r){var sa=act.getAttribute('style'),sb=r.getAttribute('style');act.setAttribute('style',sb);r.setAttribute('style',sa);}curCat=r.children[0].textContent.trim();selName=null;render();});});
  var btns=$$('button',prev);btns.forEach(function(b){var t=b.textContent;if(/^Load/.test(t))b.addEventListener('click',function(){if(!selName){SS.toast('Pick a preset first');return;}SS.setPreset&&SS.setPreset(selName);SS.toast('Loaded '+selName+' (open the Synth page to hear it)');});
   if(/Favorite/.test(t))b.addEventListener('click',function(){if(!selName)return;favs[selName]=!favs[selName];render();SS.toast(favs[selName]?'Added to Favorites':'Removed from Favorites');});
   if(/Save as/.test(t))b.addEventListener('click',function(){SS.toast('Saved as a new preset in ~/Nextcloud/Presets');});});
  var inp=$('input',srch),sortB=$$('span',list.firstElementChild).filter(function(s){return /Sort/.test(s.textContent);})[0];
  if(inp)inp.addEventListener('input',function(){query=inp.value.trim().toLowerCase();render();});
  if(sortB){sortB.style.cursor='pointer';sortB.addEventListener('click',function(){var r=sortB.getBoundingClientRect();SS.menu(r.left,r.bottom+2,['Sort: name','Sort: newest','Sort: favorites'],function(it){sortB.textContent=it+' ▾';if(it==='Sort: name')CAT[curCat].sort();else if(it==='Sort: newest')CAT[curCat].reverse();render();});});}
  $$('span,button',srch).forEach(function(t){var s=t.textContent.trim();if(!/^(wide|mono-safe|dark|bright|gritty|sub|pluck|lead)$/.test(s))return;var on=false,orig=t.getAttribute('style');t.style.cursor='pointer';t.addEventListener('click',function(){on=!on;t.style.background=on?'#FF5FA2':'';t.style.color=on?'#141518':'';tags[s]=on;SS.toast('Tag filter: '+(Object.keys(tags).filter(function(k){return tags[k];}).join(', ')||'none'));});});
  $$('button',srch).forEach(function(b){if(/Import/.test(b.textContent))b.addEventListener('click',function(){SS.toast('Import: choose .fxp / .vital / .surge presets');});if(/Folder/.test(b.textContent))b.addEventListener('click',function(){SS.toast('Preset folder syncs with Nextcloud when mounted');});});
  render();}
 SS.on('ready',function(){try{nav();if(SS.page==='presets')presetsPage();}catch(e){console.error('presets',e);}});
})();

// ===== SupurrSaw part 6: dance multiband comp (starts from a Dance Master 2 style setup) =====
(function(){
 if(!window.SS||SS.page!=='synth')return;
 var $=SS.$,$$=SS.$$,clamp=SS.clamp,AU=SS.AU,K=function(k){return SS.K[k];};
 var COL=['#FF5F5F','#FFB547','#B6F35A','#3FE0D0'];
 // Values read from a screenshot of the Vengeance Multiband Compressor "Dance Master 2" preset: crossovers, ratios, attacks, releases. Thresholds are approximate.
 var PRE={
  'Dance Master 2 (low band off)':{on:[0,1,1,1],thr:[-26,-26,-26,-26],ratio:[4.74,4.60,2.83,5.75],atk:[.5,40,15.2,10.7],rls:[3.09,0,0,38.86],xo:[113,1374,8152],mix:50,lim:50},
  'OTT-style starter (tune by ear)':{on:[1,1,1,1],thr:[-30,-32,-32,-30],ratio:[6,8,8,6],atk:[8,3,1.5,1],rls:[80,60,45,35],xo:[120,2500,9000],mix:50,lim:40},
  'Neutral glue':{on:[1,1,1,1],thr:[-18,-18,-18,-18],ratio:[2,2,2,2],atk:[30,20,12,8],rls:[120,100,80,60],xo:[120,1500,8000],mix:50,lim:80}};
 var cur='Dance Master 2 (low band off)';
 function kh(label,key,txt,col,r){var s=r*2+8,c=s/2,sx=(c-r*.7071).toFixed(1),sy=(c+r*.7071).toFixed(1);
  return '<div style="display:flex;flex-direction:column;align-items:center;gap:2px;min-width:40px"><svg width="'+s+'" height="'+s+'" viewBox="0 0 '+s+' '+s+'" aria-label="'+label+'" data-key="'+key+'"><circle cx="'+c+'" cy="'+c+'" r="'+r+'" fill="#16171A" stroke="#3A3D44" stroke-width="3"/><path d="M'+sx+','+sy+' A'+r+','+r+' 0 0 1 '+c+','+(c-r)+'" fill="none" stroke="'+col+'" stroke-width="3" stroke-linecap="round"/><line x1="'+c+'" y1="'+c+'" x2="'+c+'" y2="'+(c-r)+'" stroke="#ECEAE4" stroke-width="2" stroke-linecap="round"/></svg><span style="font-size:11px;font-weight:700">'+txt+'</span><span style="font-size:9px;color:#B9B6AE;letter-spacing:.06em;text-transform:uppercase">'+label+'</span></div>';}
 var BTN='height:24px;padding:0 8px;border-radius:6px;border:1px solid #4A4E57;background:#16171A;color:#ECEAE4;font:inherit;font-size:11px;font-weight:700;cursor:pointer;white-space:nowrap';
 function buildUI(){var fx=SS.sec('FX CHAIN');if(!fx||$('[data-mb]'))return;var p=PRE[cur],sec=document.createElement('section');sec.setAttribute('data-mb','1');
  sec.setAttribute('style','background:#201C26;border:1px solid #352E40;border-top:2px solid #FFB547;border-radius:10px;padding:10px 12px;display:flex;flex-direction:column;gap:8px;');
  var h='<div style="display:flex;align-items:center;gap:8px"><span style="font-size:11px;letter-spacing:.12em;color:#FFB547;font-weight:700">DANCE MB \u00b7 multiband comp + limiter</span><span style="margin-left:auto;display:flex;gap:6px;align-items:center"><span data-mbpre style="font-size:12px;padding:3px 9px;border-radius:5px;background:#0E0C12;border:1px solid #3A3245;cursor:pointer">'+cur+' \u25be</span><button type="button" style="height:24px;padding:0 8px;border-radius:6px;border:1px solid #FFB547;background:#FFB547;color:#141518;font:inherit;font-size:11px;font-weight:700;cursor:pointer;white-space:nowrap">On</button></span></div>';
  h+='<div style="display:grid;grid-template-columns:repeat(4,1fr);gap:8px">';
  ['LOW','LOW MID','HIGH MID','HIGH'].forEach(function(n,i){var b='MB.B'+(i+1)+'.';
   h+='<div data-band="'+i+'" style="background:#1A1720;border:1px solid #352E40;border-radius:8px;padding:8px;display:flex;flex-direction:column;gap:6px;align-items:center"><div style="display:flex;width:100%;justify-content:space-between;align-items:center"><b style="color:'+COL[i]+';font-size:12px">'+n+'</b><button type="button" data-bon="'+i+'" style="'+BTN+'">'+(p.on[i]?'On':'Off')+'</button></div><span data-brange style="font-size:10px;color:#9C92AA"></span><div style="display:flex;gap:6px;flex-wrap:wrap;justify-content:center">'+kh('Thresh',b+'Thresh',p.thr[i]+' dB',COL[i],13)+kh('Ratio',b+'Ratio',p.ratio[i].toFixed(2)+' : 1',COL[i],13)+kh('Atk',b+'Atk',p.atk[i]+' ms',COL[i],13)+kh('Rls',b+'Rls',p.rls[i]?p.rls[i]+' ms':'Auto',COL[i],13)+'</div></div>';});
  h+='</div><div style="display:flex;gap:14px;flex-wrap:wrap;align-items:center;justify-content:space-around">'+kh('X1','MB.X1',p.xo[0]+' Hz','#FFB547',14)+kh('X2','MB.X2',p.xo[1]+' Hz','#FFB547',14)+kh('X3','MB.X3',p.xo[2]+' Hz','#FFB547',14)+kh('Dry/Wet','MB.Mix',p.mix+'%','#FFB547',14)+kh('Lim rel','MB.Lim',p.lim+' ms','#FF5FA2',14)+'<span style="font-size:11px;color:#9C92AA;max-width:420px">Starting point read from the Vengeance Dance Master 2 preset (crossovers, ratios, attack, release; thresholds approximate), low band left off so the sub stays untouched. Original plugin is not bundled.</span></div>';
  sec.innerHTML=h;fx.insertAdjacentElement('afterend',sec);}
 var prePre=SS.pre;SS.pre=function(){if(prePre)prePre();buildUI();};
 function lo(i){var x=[20].concat([K('MB.X1').val(),K('MB.X2').val(),K('MB.X3').val()],[20000]);return x;}
 function ranges(){var x=lo();$$('[data-band]').forEach(function(b,i){var r=$('[data-brange]',b);r.textContent=i===0?'< '+Math.round(x[1])+' Hz':i===3?'> '+Math.round(x[3])+' Hz':Math.round(x[i])+' Hz to '+Math.round(x[i+1])+' Hz';});}
 var bandOn=[1,1,1,1];
 function bind(){var sec=$('[data-mb]');if(!sec)return;
  $$('[data-bon]',sec).forEach(function(b){var i=+b.getAttribute('data-bon'),onS=BTN.replace('border:1px solid #4A4E57','border:1px solid '+COL[i]).replace('background:#16171A','background:'+COL[i]).replace('color:#ECEAE4','color:#141518'),offS=BTN;
   function set(v){bandOn[i]=v?1:0;b.textContent=v?'On':'Off';b.setAttribute('style',v?onS:offS);b.parentElement.parentElement.style.opacity=v?'':'.55';apply();}
   b.addEventListener('click',function(){set(!bandOn[i]);});b.__set=set;set(bandOn[i]);});
  var dd=$('[data-mbpre]',sec);dd.addEventListener('click',function(e){var r=dd.getBoundingClientRect();SS.menu(r.left,r.bottom+2,Object.keys(PRE),function(it){cur=it;dd.textContent=it+' \u25be';load(it);SS.toast('MB comp: '+it);});});
  ranges();}
 function setMs(k,v){if(!K(k))return;K(k).set(K(k).spec.from(v));}
 function load(name){var p=PRE[name];p.on.forEach(function(v,i){var b=$('[data-bon="'+i+'"]');b.__set(!!v);});
  for(var i=0;i<4;i++){var b='MB.B'+(i+1)+'.';K(b+'Thresh').set(K(b+'Thresh').spec.from(p.thr[i]));K(b+'Ratio').set(K(b+'Ratio').spec.from(p.ratio[i]));K(b+'Atk').set(K(b+'Atk').spec.from(p.atk[i]));K(b+'Rls').set(p.rls[i]?K(b+'Rls').spec.from(p.rls[i]):0);}
  setMs('MB.X1',p.xo[0]);setMs('MB.X2',p.xo[1]);setMs('MB.X3',p.xo[2]);K('MB.Mix').set(p.mix/100);setMs('MB.Lim',p.lim);ranges();apply();}
 // ---- audio
 var mb=null;
 AU.mbBuild=function(src,dst){var c=AU.ctx(),inn=c.createGain(),out=c.createGain(),dry=c.createGain(),wet=c.createGain();src.connect(inn);inn.connect(dry);dry.connect(out);wet.connect(out);out.connect(dst);
  function bq(t,f){var a=c.createBiquadFilter();a.type=t;a.frequency.value=f;a.Q.value=.7071;return a;}
  var bands=[],xo=[113,1374,8152],i;
  for(i=0;i<4;i++){var chain=[],comp=c.createDynamicsCompressor(),mk=c.createGain();comp.knee.value=6;
   if(i>0){chain.push(bq('highpass',xo[i-1]),bq('highpass',xo[i-1]));}if(i<3){chain.push(bq('lowpass',xo[i]),bq('lowpass',xo[i]));}
   var prev=inn;chain.forEach(function(n){prev.connect(n);prev=n;});prev.connect(comp);comp.connect(mk);mk.connect(wet);bands.push({chain:chain,comp:comp,mk:mk,i:i});}
  mb={bands:bands,dry:dry,wet:wet,in:inn};apply();};
 function apply(){if(!mb||!K('MB.Mix'))return;var tg=SS.seg['tg:DANCE MB'],on=!tg||tg.on,mix=on?K('MB.Mix').n:0;mb.wet.gain.value=mix;mb.dry.gain.value=on?1-mix:1;
  var x=[K('MB.X1').val(),K('MB.X2').val(),K('MB.X3').val()];
  mb.bands.forEach(function(b){var i=b.i,pre='MB.B'+(i+1)+'.',n=0;if(i>0){b.chain[0].frequency.value=x[i-1];b.chain[1].frequency.value=x[i-1];n=2;}if(i<3){b.chain[n].frequency.value=x[i];b.chain[n+1].frequency.value=x[i];}
   var thr=K(pre+'Thresh').val(),ra=K(pre+'Ratio').val(),at=K(pre+'Atk').val(),rl=K(pre+'Rls').n<.005?.12:K(pre+'Rls').val()/1000;
   b.comp.threshold.value=bandOn[i]?thr:0;b.comp.ratio.value=bandOn[i]?clamp(ra,1,20):1;b.comp.attack.value=clamp(at/1000,0,1);b.comp.release.value=clamp(rl,.001,1);b.mk.gain.value=bandOn[i]?Math.pow(10,Math.min(9,-thr*(1-1/Math.max(ra,1))*.25)/20):1;});
  if(AU.comp){AU.comp.release.value=clamp(K('MB.Lim').val()/1000,.01,1);}}
 SS.on('*',function(k){if(/^k:MB\./.test(k)){if(/\.X\d$/.test(k))ranges();apply();}else if(k==='tg:DANCE MB')apply();});
 SS.on('ready',function(){try{bind();load(cur);}catch(e){console.error('mb',e);}});
})();

// ===== UltraNova: draggable waveform displays + all mixer faders =====
(function(){
 if(!/UNova_Osc|UltraNova/i.test(location.pathname))return;
 var NAMES=['Sine','Triangle','Sawtooth','Square'];
 function name(i){return i<4?NAMES[i]:'Wavetable '+(i-3);}
 function shape(i,w,h){var pts=[],x,n=48,seed=i*7.13;for(x=0;x<=n;x++){var ph=x/n*2,t=ph%1,y;
   if(i===0)y=Math.sin(ph*Math.PI*2);else if(i===1)y=t<.5?4*t-1:3-4*t;else if(i===2)y=2*t-1;else if(i===3)y=t<.5?1:-1;
   else{y=0;for(var k=1;k<=6;k++){y+=Math.sin(ph*Math.PI*2*k+seed*k)*(Math.abs(Math.sin(seed*k*1.7))+.1)/k;}y*=.8;}
   pts.push((w*x/n).toFixed(1)+','+(h/2-y*h*.38).toFixed(1));}return pts.join(' ');}
 function init(){
  var menu=null;function close(){if(menu){menu.remove();menu=null;}}
  document.addEventListener('pointerdown',function(e){if(menu&&!menu.contains(e.target))close();},true);
  function open(x,y,cb){close();menu=document.createElement('div');menu.setAttribute('style','position:fixed;z-index:10002;background:#101216;border:1px solid #4A5568;border-radius:8px;padding:4px;max-height:320px;overflow:auto;font:600 12px system-ui,sans-serif;color:#E6EAF2;min-width:170px;left:'+x+'px;top:'+y+'px');
   for(var i=0;i<64;i++)(function(i){var d=document.createElement('div');d.textContent=('0'+i).slice(-2)+' · '+name(i);d.setAttribute('style','padding:4px 10px;border-radius:5px;cursor:pointer');d.onmouseenter=function(){d.style.background='#2E7DD7';};d.onmouseleave=function(){d.style.background='';};d.onclick=function(){close();cb(i);};menu.appendChild(d);})(i);document.body.appendChild(menu);}
  var tip=document.createElement('div');tip.setAttribute('style','position:fixed;z-index:10003;background:#000;color:#fff;font:600 11px system-ui,sans-serif;padding:2px 7px;border-radius:4px;pointer-events:none;display:none');document.body.appendChild(tip);
  [].slice.call(document.querySelectorAll('svg[aria-label^="Waveform"]')).forEach(function(sv){var c=sv.parentElement,pth=sv.querySelector('path[transform]');if(!pth||sv.hasAttribute('data-wv'))return;
   sv.setAttribute('data-wv','1');sv.style.cursor='ns-resize';sv.style.touchAction='none';sv.setAttribute('title','Drag up/down to change the waveform, double-click for the waveform directory');
   var m=/(\d+)\s*$/.exec(sv.getAttribute('aria-label')),lab=sv.getAttribute('aria-label').toLowerCase(),idx=/sine/.test(lab)?0:/tri/.test(lab)?1:/saw/.test(lab)?2:/square|pulse/.test(lab)?3:(m?3+parseInt(m[1]):4);
   var cap=[].slice.call(c.querySelectorAll('text,span,div')).filter(function(x){return x.children.length===0&&/wave table|sine|saw|tri|square/i.test(x.textContent);})[0];
   function path(i){var pts=[],x,n=40,seed=i*7.13;for(x=0;x<=n;x++){var ph=x/n*2,t=ph%1,y;
     if(i===0)y=Math.sin(ph*Math.PI*2);else if(i===1)y=t<.5?4*t-1:3-4*t;else if(i===2)y=2*t-1;else if(i===3)y=t<.5?1:-1;
     else{y=0;for(var k=1;k<=6;k++){y+=Math.sin(ph*Math.PI*2*k+seed*k)*(Math.abs(Math.sin(seed*k*1.7))+.1)/k;}y*=.75;}
     pts.push((x===0?'M':'L')+(12+40*x/n).toFixed(1)+','+(23-y*11).toFixed(1));}return pts.join(' ');}
   function draw(){pth.setAttribute('d',path(idx));sv.setAttribute('aria-label','Waveform: '+name(idx).toLowerCase());if(cap)cap.textContent=name(idx).toLowerCase();}
   draw();
   sv.addEventListener('pointerdown',function(e){e.preventDefault();sv.setPointerCapture(e.pointerId);var y0=e.clientY,x0=e.clientX,i0=idx;
    function mv(m){var d=Math.round(((y0-m.clientY)+(m.clientX-x0))/10);idx=Math.max(0,Math.min(63,i0+d));draw();tip.textContent=('0'+idx).slice(-2)+' \u00b7 '+name(idx)+'  (CC raw '+Math.round(idx/63*127)+')';tip.style.left=(m.clientX+14)+'px';tip.style.top=(m.clientY-28)+'px';tip.style.display='block';}
    function up(){sv.removeEventListener('pointermove',mv);sv.removeEventListener('pointerup',up);tip.style.display='none';}
    sv.addEventListener('pointermove',mv);sv.addEventListener('pointerup',up);});
   sv.addEventListener('dblclick',function(e){open(e.clientX,e.clientY,function(i){idx=i;draw();});});
   sv.addEventListener('wheel',function(e){e.preventDefault();idx=Math.max(0,Math.min(63,idx-Math.sign(e.deltaY)));draw();},{passive:false});
  });
  // mixer faders: every column
  [].slice.call(document.querySelectorAll('section')).forEach(function(s){if(!/Mixer/.test(s.textContent.slice(0,8)))return;
   [].slice.call(s.querySelectorAll('div')).forEach(function(t){var st=t.getAttribute('style')||'';if(!/width:\s*8px/.test(st)||!/height:\s*54px/.test(st)||t.children.length!==1)return;var f=t.children[0];
    t.style.cursor='ns-resize';t.style.touchAction='none';
    function paint(h){f.style.height=h+'px';f.style.background=h>4?'linear-gradient(0deg,#4BE38A,#C8FFD9)':'#333';}
    function set(e){var r=t.getBoundingClientRect();paint(Math.round(Math.max(0,Math.min(r.height,r.bottom-e.clientY))));}
    if(t.hasAttribute('data-fd'))return;
    t.addEventListener('pointerdown',function(e){e.preventDefault();t.setPointerCapture(e.pointerId);set(e);function mv(m){set(m);}function up(){t.removeEventListener('pointermove',mv);t.removeEventListener('pointerup',up);}t.addEventListener('pointermove',mv);t.addEventListener('pointerup',up);});
    t.addEventListener('dblclick',function(){paint(0);});});});
 }
 function go(){setTimeout(init,1500);}if(document.readyState!=='loading')go();else document.addEventListener('DOMContentLoaded',go);
})();

// ===== WhiskerMix: per-channel Settings panel (wrench toggles it, x closes only the panel, strip never folds) =====
(function(){
 if(!/WhiskerMix/i.test(location.pathname))return;
 function isStrip(d){return d&&d.nodeType===1&&d.style.width==='74px'&&d.parentElement&&d.parentElement.style.alignItems==='stretch';}
 function isPanel(d){return d&&d.nodeType===1&&d.getAttribute('data-chset')==='1';}
 function wrenchOf(s){var w=null;[].forEach.call(s.querySelectorAll('button'),function(b){if(!w&&b.textContent.trim()==='🔧')w=b;});return w;}
 function lit(w,on){if(!w)return;if(w.__o===undefined)w.__o=w.getAttribute('style');w.setAttribute('style',on?w.__o.replace(/background:[^;]+;/,'background: #F28BA8;').replace(/color:[^;]+;/,'color: #1E1F23;'):w.__o);}
 function init(){
  var tpl=null;
  [].forEach.call(document.querySelectorAll('div[style*="width: 108px"]'),function(d){var sp=d.querySelector('span');if(sp&&sp.textContent.trim()==='Settings'){d.setAttribute('data-chset','1');if(!tpl)tpl=d;}});
  if(!tpl)return;tpl=tpl.cloneNode(true);
  function addClose(p){if(p.querySelector('[data-chclose]'))return;var sp=p.querySelector('span');var row=document.createElement('div');row.setAttribute('style','display:flex;justify-content:space-between;align-items:center;width:96px');
   sp.parentElement.insertBefore(row,sp);row.appendChild(sp);var x=document.createElement('button');x.type='button';x.setAttribute('data-chclose','1');x.setAttribute('data-nosel','1');x.title='Close this channel\'s settings (the channel stays as is)';x.textContent='×';
   x.setAttribute('style','width:16px;height:16px;border:none;border-radius:3px;background:#1F2125;color:#B9B6AE;font-size:12px;line-height:14px;cursor:pointer;padding:0');row.appendChild(x);}
  [].forEach.call(document.querySelectorAll('[data-chset]'),function(p){addClose(p);var s=p.previousElementSibling;if(isStrip(s))lit(wrenchOf(s),true);});
  document.addEventListener('click',function(e){
   var b=e.target.closest&&e.target.closest('button');if(!b)return;
   if(b.hasAttribute('data-chclose')){var p=b.closest('[data-chset]');var s=p.previousElementSibling;p.remove();if(isStrip(s))lit(wrenchOf(s),false);e.stopPropagation();e.preventDefault();return;}
   if(b.textContent.trim()!=='🔧')return;var s=b;while(s&&!isStrip(s))s=s.parentElement;if(!s)return;
   var nx=s.nextElementSibling;
   if(isPanel(nx)){nx.remove();lit(b,false);}
   else{var p=tpl.cloneNode(true);var c=getComputedStyle(s.children[0]).borderTopColor;var sp=p.querySelector('span');if(sp&&c)sp.style.color=c;addClose(p);s.insertAdjacentElement('afterend',p);lit(b,true);}
   e.stopPropagation();e.preventDefault();
  },true);
 }
 function go(){setTimeout(init,1600);}if(document.readyState!=='loading')go();else document.addEventListener('DOMContentLoaded',go);
})();

// ===== Visibility gate (all pages): visuals pause when the window is hidden or not in front; audio, MIDI and timers keep running =====
(function(){
 var G=window.CatVis={paused:false,ls:[]};
 function upd(){var p=document.hidden||!document.hasFocus();if(p===G.paused)return;G.paused=p;
  document.documentElement.classList.toggle('cat-paused',p);G.ls.forEach(function(f){try{f(p);}catch(e){}});}
 G.onChange=function(f){G.ls.push(f);};
 // raf that simply skips frames while paused (callers keep their own loop)
 G.raf=function(fn){return requestAnimationFrame(function(t){if(G.paused){G.onResume(function(){requestAnimationFrame(fn);});}else fn(t);});};
 var waiting=[];G.onResume=function(f){waiting.push(f);};
 G.onChange(function(p){if(!p){var w=waiting;waiting=[];w.forEach(function(f){f();});}});
 var st=document.createElement('style');st.textContent='html.cat-paused *,html.cat-paused *::before,html.cat-paused *::after{animation-play-state:paused!important}';
 (document.head||document.documentElement).appendChild(st);
 ['visibilitychange','focus','blur','pageshow','pagehide'].forEach(function(ev){(ev==='visibilitychange'?document:window).addEventListener(ev,upd);});
 setInterval(upd,1000);upd();
})();

// ===== Peak: live screen (shows what you move or play, oscilloscope when idle) + X-Y pad in the Effects area =====
(function(){
 if(!/\/Peak(Deep)?\.dc/i.test(location.pathname))return;
 function $$(s,r){return [].slice.call((r||document).querySelectorAll(s));}
 function txt(e){return e?(e.textContent||'').replace(/\s+/g,' ').trim():'';}
 function secOf(el){var s=el.closest&&el.closest('section');if(!s)return '';var h=s.querySelector('span');return txt(h).toUpperCase();}
 function knobVal(host){var r=host.querySelector('[style*="rotate("]')||host;var m=/rotate\((-?[\d.]+)deg\)/.exec(r.getAttribute('style')||'');return m?Math.round((parseFloat(m[1])+135)/270*127):0;}
 function knobLab(host){var col=host.parentElement,sp=col?$$(':scope > span',col):[];return sp.length?txt(sp[sp.length-1]):'';}
 function findKnob(sec,lab){var out=null;$$('[data-knob]').forEach(function(h){if(out)return;if(secOf(h)===sec&&knobLab(h).toLowerCase()===lab.toLowerCase())out=h;});return out;}
 function setKnob(h,v){var r=h.querySelector('[style*="rotate("]')||h;r.style.transform='rotate('+(v/127*270-135)+'deg)';}
 function init(){
  var lcd=$$('div[style*="VT323"]').filter(function(d){return d.offsetWidth>=180&&d.offsetHeight>=60;})[0];if(!lcd)return;
  var W=lcd.clientWidth,H=lcd.clientHeight,dpr=window.devicePixelRatio||1;
  lcd.style.padding='0';lcd.innerHTML='';var cv=document.createElement('canvas');cv.width=W*dpr;cv.height=H*dpr;cv.style.width=W+'px';cv.style.height=H+'px';cv.style.display='block';lcd.appendChild(cv);
  lcd.title='Screen: shows the last control you move or note you play, then the oscilloscope. Play notes with your computer keys A W S E D F T G Y H U J K (Z/X = octave).';
  var g=cv.getContext('2d');g.scale(dpr,dpr);
  var msg=null,msgUntil=0,notes={},oct=4,phase=0,last=0;
  function say(a,b,c){msg=[a,b,c];msgUntil=performance.now()+1500;draw();}
  window.__peakSay=say;
  // what the oscillators are set to (read from the panel each frame)
  function oscState(n){var s=$$('section').filter(function(x){return txt(x.querySelector('span'))==='OSCILLATOR '+n;})[0];if(!s)return null;
   var lit=$$('span',s).filter(function(sp){return /255, 122, 26|#FF7A1A/i.test(sp.getAttribute('style')||'')||/rgb\(255, 122, 26\)/.test(getComputedStyle(sp).backgroundColor);});
   var wave='saw',range=8;lit.forEach(function(sp){var t=txt(sp.parentElement);if(/∿/.test(t))wave='sine';else if(/⋀/.test(t))wave='tri';else if(/⩘/.test(t))wave='saw';else if(/⊓/.test(t))wave='square';else if(/more/.test(t))wave='more';else if(/^(16|8|4|2)′?/.test(t))range=parseInt(t,10);});
   return {wave:wave,mult:8/range};}
  function mixLevel(n){var h=findKnob('MIXER','Osc '+n);return h?knobVal(h)/127:0.6;}
  function filt(){var h=findKnob('FILTER','Frequency');return h?knobVal(h)/127:0.7;}
  function shape(w,p){p-=Math.floor(p);if(w==='sine')return Math.sin(p*2*Math.PI);if(w==='tri')return p<.5?4*p-1:3-4*p;if(w==='square')return p<.5?1:-1;if(w==='more')return Math.sin(p*2*Math.PI)*0.6+Math.sin(p*6*Math.PI)*0.4;return 1-2*p;}
  function scope(now){var os=[1,2,3].map(oscState),lv=[1,2,3].map(mixLevel),k=0.08+filt()*0.92,held=Object.keys(notes).length>0;
   g.fillStyle='#050607';g.fillRect(0,0,W,H);
   g.strokeStyle='rgba(127,178,255,.18)';g.lineWidth=1;g.beginPath();g.moveTo(0,(H+14)/2);g.lineTo(W,(H+14)/2);g.stroke();
   var y=0,pts=[],N=W,cyc=2.2,tot=lv[0]+lv[1]+lv[2]||1,i,o;
   for(i=0;i<N*2;i++){var p=i/N*cyc+phase,s=0;for(o=0;o<3;o++)if(os[o])s+=shape(os[o].wave,p*os[o].mult)*lv[o];s/=tot;y+=(s-y)*k;if(i>=N)pts.push(y);} // 2nd pass = settled filter
   g.strokeStyle=held?'#CFE3FF':'rgba(207,227,255,.55)';g.shadowColor='#7FB2FF';g.shadowBlur=held?6:2;g.lineWidth=1.6;g.beginPath();
   pts.forEach(function(v,x){var yy=(H+14)/2-v*((H-24)/2);x?g.lineTo(x,yy):g.moveTo(x,yy);});g.stroke();g.shadowBlur=0;
   g.fillStyle='rgba(157,182,217,.85)';g.font='13px VT323, monospace';
   var nn=Object.keys(notes).map(Number).sort(function(a,b){return a-b;});
   g.fillText(held?nn.map(nName).join(' '):'Patch 042',5,13);
   g.textAlign='right';g.fillText(os.map(function(x){return x?x.wave.slice(0,3).toUpperCase():'';}).join('+'),W-5,13);g.textAlign='left';}
  function nName(m){return ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'][m%12]+(Math.floor(m/12)-1);}
  function text(){g.fillStyle='#050607';g.fillRect(0,0,W,H);g.fillStyle='#9DB6D9';g.textAlign='center';
   var top=(msg[0]||'')+'  ·  '+(msg[1]||''),fs=15;g.font=fs+'px VT323, monospace';while(fs>9&&g.measureText(top).width>W-8){fs--;g.font=fs+'px VT323, monospace';}
   g.fillText(top,W/2,20);var big=String(msg[2]),bs=34;g.font=bs+'px VT323, monospace';while(bs>14&&g.measureText(big).width>W-8){bs-=2;g.font=bs+'px VT323, monospace';}g.fillStyle='#EAF2FF';g.shadowColor='#7FB2FF';g.shadowBlur=6;g.fillText(big,W/2,H-12);g.shadowBlur=0;g.textAlign='left';}
  function draw(now){now=now||performance.now();if(msg&&now<msgUntil)text();else{msg=null;scope(now);}}
  function loop(t){var dt=last?(t-last)/1000:0;last=t;if(Object.keys(notes).length)phase+=dt*0.9;draw(t);CatVis.raf(loop);}
  CatVis.raf(loop);CatVis.onChange(function(p){last=0;});
  // controls -> screen
  var down=null;
  document.addEventListener('pointerdown',function(e){down=e.target.closest&&e.target.closest('[data-knob],[data-fd],[data-xy]');if(down&&down.hasAttribute('data-knob'))say(secOf(down),knobLab(down),knobVal(down));},true);
  document.addEventListener('pointermove',function(e){if(!down||!(e.buttons&1))return;
   if(down.hasAttribute('data-knob'))say(secOf(down),knobLab(down),knobVal(down));
   else if(down.hasAttribute('data-fd')){var r=down.getBoundingClientRect(),v=Math.round(Math.max(0,Math.min(1,1-(e.clientY-r.top)/r.height))*127);var col=down.parentElement,lab=col?txt($$('span',col).pop()):'';say(secOf(down),lab,v);}});
  document.addEventListener('pointerup',function(){down=null;},true);
  document.addEventListener('wheel',function(e){var h=e.target.closest&&e.target.closest('[data-knob]');if(h)setTimeout(function(){say(secOf(h),knobLab(h),knobVal(h));},0);},true);
  document.addEventListener('click',function(e){var row=e.target.closest&&e.target.closest('div');if(!row)return;
   if(row.closest('[data-knob],[data-xy]'))return;var b=e.target.closest('button');
   if(b){var lab=txt(b)||txt(b.parentElement&&$$('span',b.parentElement).pop());if(lab&&lab.length<24)say(secOf(b)||'PANEL',lab,'▶');return;}
   var led=row.querySelector(':scope > span');if(led&&led.offsetWidth<=10&&txt(row))setTimeout(function(){say(secOf(row),'Select',txt(row));},0);});
  // computer keys play notes (screen + scope); real MIDI notes come from the hardware in the plugin build
  var KEYS='awsedftgyhujk';
  document.addEventListener('keydown',function(e){if(e.repeat||e.target.closest('input,textarea'))return;var k=e.key.toLowerCase();
   if(k==='z'){oct=Math.max(0,oct-1);say('KEYS','Octave',oct);return;}if(k==='x'){oct=Math.min(8,oct+1);say('KEYS','Octave',oct);return;}
   var i=KEYS.indexOf(k);if(i<0)return;var m=(oct+1)*12+i;notes[m]=1;say('NOTE ON','vel 100',nName(m));msgUntil=performance.now()+500;});
  document.addEventListener('keyup',function(e){var i=KEYS.indexOf(e.key.toLowerCase());if(i<0)return;delete notes[(oct+1)*12+i];});
  // X-Y pad in the free space of the Effects section
  var eff=$$('section').filter(function(s){return txt(s.querySelector('span'))==='EFFECTS';})[0];
  if(eff&&!eff.querySelector('[data-peakxy]')){
   var T=[['FILTER','Frequency'],['FILTER','Resonance'],['FILTER','Overdrive'],['OSCILLATOR 1','Shape Amount'],['LFO 1','Rate'],['DELAY','Feedback'],['REVERB','Time'],['CHORUS','Level']];
   var tx=0,ty=1;
   var box=document.createElement('div');box.setAttribute('data-peakxy','1');box.setAttribute('data-own','1');box.setAttribute('style','display:flex;flex-direction:column;align-items:center;gap:2px;margin-top:2px');
   var pad=document.createElement('div');pad.title='X-Y pad: drag to move two controls at once, double-click to center. Click X or Y below to pick what it moves.';
   pad.setAttribute('style','position:relative;width:64px;height:64px;border-radius:4px;background:repeating-linear-gradient(0deg,transparent 0 20px,rgba(63,169,245,.12) 20px 21px),repeating-linear-gradient(90deg,#0B0C0E 0 20px,#14161A 20px 21px);border:1px solid #3FA9F5;cursor:crosshair;touch-action:none');
   var dot=document.createElement('div');dot.setAttribute('style','position:absolute;width:12px;height:12px;margin:-6px 0 0 -6px;border-radius:50%;background:#FF7A1A;box-shadow:0 0 8px #FF7A1A;left:50%;top:50%;pointer-events:none');pad.appendChild(dot);
   function lab(axis){var s=document.createElement('button');s.type='button';s.setAttribute('style','width:80px;height:13px;padding:0;border:1px solid #000;border-radius:2px;background:linear-gradient(180deg,#3A3C40,#1C1D20);color:#E9EBEE;font-size:9px;font-family:inherit;cursor:pointer;overflow:hidden;white-space:nowrap');s.title='Click to change what '+axis+' controls';return s;}
   var lx=lab('X'),ly=lab('Y');function names(){lx.textContent='X: '+T[tx][1];ly.textContent='Y: '+T[ty][1];}names();
   lx.addEventListener('click',function(e){e.stopPropagation();tx=(tx+1)%T.length;if(tx===ty)tx=(tx+1)%T.length;names();say('X-Y PAD','X controls',T[tx][1]);});
   ly.addEventListener('click',function(e){e.stopPropagation();ty=(ty+1)%T.length;if(ty===tx)ty=(ty+1)%T.length;names();say('X-Y PAD','Y controls',T[ty][1]);});
   function put(fx,fy){dot.style.left=(fx*100)+'%';dot.style.top=((1-fy)*100)+'%';var vx=Math.round(fx*127),vy=Math.round(fy*127);
    var kx=findKnob(T[tx][0],T[tx][1]),ky=findKnob(T[ty][0],T[ty][1]);if(kx)setKnob(kx,vx);if(ky)setKnob(ky,vy);
    say('X-Y PAD',T[tx][1]+' / '+T[ty][1],vx+' / '+vy);}
   pad.addEventListener('pointerdown',function(e){e.preventDefault();e.stopPropagation();pad.setPointerCapture(e.pointerId);
    function mv(m){var r=pad.getBoundingClientRect();put(Math.max(0,Math.min(1,(m.clientX-r.left)/r.width)),Math.max(0,Math.min(1,1-(m.clientY-r.top)/r.height)));}
    function up(){pad.removeEventListener('pointermove',mv);pad.removeEventListener('pointerup',up);}mv(e);pad.addEventListener('pointermove',mv);pad.addEventListener('pointerup',up);});
   pad.addEventListener('dblclick',function(){put(.5,.5);});
   box.appendChild(pad);box.appendChild(lx);box.appendChild(ly);eff.appendChild(box);}
 }
 function go(){setTimeout(init,1700);}if(document.readyState!=='loading')go();else document.addEventListener('DOMContentLoaded',go);
})();
