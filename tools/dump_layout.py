#!/usr/bin/env python3
"""Reads every control's position, size, label and tooltip from the approved mockup (docs/mockup/stretchcat.html) with
Playwright and writes docs/mockup/layout.json. tools/gen_layout.py turns that into src/editor/Layout.h, so the native
editor takes its geometry from the mockup and the two cannot drift apart. Run from the repo root.
The mockup's preview rig (test signals, Stretch FX tab, FX only buttons) is not part of the plugin and is left out."""
import asyncio, json, os
from playwright.async_api import async_playwright

ROOT = os.getcwd()
FONTS = f"""<style>
@font-face{{font-family:'Barlow Condensed';font-weight:500;src:url('file://{ROOT}/assets/fonts/BarlowCondensed-Medium.ttf')}}
@font-face{{font-family:'Barlow Condensed';font-weight:600;src:url('file://{ROOT}/assets/fonts/BarlowCondensed-SemiBold.ttf')}}
@font-face{{font-family:'Barlow Condensed';font-weight:700;src:url('file://{ROOT}/assets/fonts/BarlowCondensed-Bold.ttf')}}
@font-face{{font-family:'JetBrains Mono';font-weight:400;src:url('file://{ROOT}/assets/fonts/JetBrainsMono-Regular.ttf')}}
</style>"""
src = open('docs/mockup/stretchcat.html').read()
src = src.replace('<link rel="stylesheet" href="https://fonts.googleapis.com/css2', '<link rel="x-skip" href="https://fonts.googleapis.com/css2')
open('/tmp/sc_wrapped.html', 'w').write('<!doctype html><html><head><meta charset="utf-8">' + FONTS + '</head><body>' + src + '</body></html>')

JS = r"""
() => {
  const w=document.querySelector('#win').getBoundingClientRect(); const r2=v=>Math.round(v*100)/100;
  const R=e=>{const r=e.getBoundingClientRect();return [r2(r.left-w.left),r2(r.top-w.top),r2(r.width),r2(r.height)]};
  const vis=e=>e.getBoundingClientRect().width>0&&getComputedStyle(e).display!=='none'&&!e.closest('[hidden]');
  const cs=e=>{const c=getComputedStyle(e);return {fs:parseFloat(c.fontSize),fw:c.fontWeight,ls:c.letterSpacing==='normal'?0:parseFloat(c.letterSpacing),tt:c.textTransform,col:c.color,mono:c.fontFamily.indexOf('JetBrains')>=0}};
  const out={win:[0,0,1400,940]};
  const one=(k,s)=>{const e=document.querySelector(s); if(e&&vis(e)) out[k]=R(e);};
  one('cheekL','.cheek.l'); one('cheekR','.cheek.r'); one('panel','.panel'); one('header','header.top'); one('brandSvg','.brand svg'); one('brandName','.brand b'); one('brandSub','.brand small');
  one('rig','.rig'); one('srcBox','#srcBox'); one('resBox','#resBox'); one('laneBox','#laneBox'); one('ctlBox','#ctlBox'); one('rackcol','.rackcol'); one('foot','.foot'); one('sizeT','#sizeT'); one('footL','.foot span'); one('cvM','#cvM'); one('rackInfo','#rackInfo'); one('latInfo','#latInfo'); one('anaInfo','#anaInfo'); one('lenInfo','#lenInfo'); one('laneTabs','#laneTabs');
  one('sliceRow','#sliceRow');
  out.tips={cheek:document.querySelector('.cheek.l').dataset.tip||'', srcBox:(document.querySelector('#srcBox').dataset.tip||''), resBox:(document.querySelector('#resBox').dataset.tip||''), laneBox:(document.querySelector('#laneBox').dataset.tip||''), cvM:(document.querySelector('#cvM').dataset.tip||'')};
  const skip=new Set(['bPlayRaw','bGrab','fileIn','rigSrc']);
  const items=[];
  document.querySelectorAll('#win button, #win select, #win .kw, #win .mini, #win .sh span, #win .rh, #win label.fld > span, #win .rd').forEach(e=>{
    if(!vis(e)||skip.has(e.id)||e.closest('.rig .grp#rigSrc')) return;
    if(e.closest('.plist')||e.closest('.modal')||e.closest('.rz')) return;
    const kw=e.classList.contains('kw'); const o={tag:e.tagName.toLowerCase(),id:e.id||'',cls:e.className||'',text:(e.textContent||'').trim().replace(/\s+/g,' '),r:R(e),tip:'',style:cs(e)};
    const t=e.closest('[data-tip],[data-tipx]'); o.tip=(e.dataset&&e.dataset.tip)||(t&&(t.dataset.tip||t.dataset.tipx))||'';
    o.par=(e.closest('#rack')?'rack':e.closest('#cSt')?'cSt':e.closest('#cPb')?'cPb':e.closest('#laneTabs')?'laneTabs':e.closest('#sliceRow')?'sliceRow':e.closest('#rigHost')?'rigHost':e.closest('.rig')?'rig':e.closest('header')?'header':e.closest('.modes')?'modes':e.closest('.rackcol')?'rackcol':'other');
    if(kw){o.label=e.querySelector('.kl').textContent;o.knob=R(e.querySelector('.knob'));o.kl=R(e.querySelector('.kl'));o.kv=R(e.querySelector('.kv'));o.val=e.querySelector('.kv').textContent;o.ticks=null;}
    if(e.tagName==='SELECT'){o.options=[...e.options].map(x=>x.textContent);o.sel=e.options[e.selectedIndex].textContent;o.fld=e.parentElement.querySelector('span')?e.parentElement.querySelector('span').textContent:'';o.fldR=e.parentElement.querySelector('span')?R(e.parentElement.querySelector('span')):null;}
    if(e.tagName==='BUTTON'){o.pressed=e.getAttribute('aria-pressed');}
    items.push(o);
  });
  out.items=items;
  const rk=[...document.querySelectorAll('#rack .rk')].map(e=>({r:R(e),h:e.querySelector('.rh').textContent.replace(/\s+/g,' ').trim().slice(0,40)})); out.rkSecs=rk;
  out.rackInnerR=R(document.querySelector('#rack'));
  out.rackScrollH=document.querySelector('#rack').scrollHeight;
  return out;
}
"""
async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(executable_path=os.environ.get('CHROMIUM', '/opt/pw-browsers/chromium-1243/chrome-linux/chrome') if os.path.exists('/opt/pw-browsers/chromium-1243/chrome-linux/chrome') else None, args=['--no-sandbox'])
        pg = await b.new_page(viewport={'width': 1500, 'height': 1000})
        await pg.goto('file:///tmp/sc_wrapped.html'); await pg.wait_for_timeout(1500)
        d = await pg.evaluate(JS)
        os.makedirs('docs/mockup', exist_ok=True)
        json.dump(d, open('docs/mockup/layout.json', 'w'), indent=1)
        await pg.screenshot(path='/tmp/sc_mockup_ref.png')
        print('items', len(d['items']), 'rack scrollH', d['rackScrollH'], 'rackInner', d['rackInnerR'])
        await b.close()
asyncio.run(main())
