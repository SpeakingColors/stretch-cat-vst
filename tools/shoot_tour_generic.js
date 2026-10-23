// Interface-tour pictures for a Studio 87 mockup page: numbered badges on every control that has an info bubble (data-tip),
// one picture per tab, and docs/tour/tour.json with the control names and bubble texts read from the page.
// Run: node tools/shoot_tour_generic.js docs/mockup/<page>.html   (needs Playwright with Chromium; PLAYWRIGHT_MODULE, CHROMIUM override)
const path = require('path'), fs = require('fs');
const pw = require(process.env.PLAYWRIGHT_MODULE || '/home/claude/.npm-global/lib/node_modules/playwright');
const root = path.resolve(__dirname, '..'), out = path.join(root, 'docs/tour');
const page0 = path.resolve(process.argv[2]);
// Most kit mockups attach their bubbles from script (attachTip); record label and text on the element so the tour can read them.
let html = fs.readFileSync(page0, 'utf8');
const hooked = html.replace(/function attachTip\(el, label, text\) \{/, 'function attachTip(el, label, text) { try { if (text) { el.setAttribute("data-tipx", text); el.setAttribute("data-tl2", label || ""); } } catch (e) {}');
const page = path.join(path.dirname(page0), '.tour_tmp.html'); fs.writeFileSync(page, hooked);
process.on('exit', () => { try { fs.unlinkSync(page); } catch (e) {} });
(async () => {
  fs.mkdirSync(out, { recursive: true });
  const b = await pw.chromium.launch({ executablePath: process.env.CHROMIUM || '/opt/pw-browsers/chromium', args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required'] });
  const p = await b.newPage({ viewport: { width: 1920, height: 1300 }, deviceScaleFactor: 1.5, colorScheme: 'dark' });
  await p.goto('file://' + page); await p.waitForTimeout(1200);
  await p.addStyleTag({ content: '#tip{display:none!important}' });
  const tabSel = await p.evaluate(() => {
    for (const s of ['.tabs > button', 'nav.modes > button', '[role=tab]', '.tabs button']) { const l = [...document.querySelectorAll(s)].filter(e => e.offsetWidth > 0); if (l.length > 1) { l.forEach((e, i) => e.setAttribute('data-tourtab', i)); return l.length; } }
    return 0;
  });
  const title = await p.evaluate(() => document.title.replace(/\s*[-|:·].*$/, '').trim());
  const result = { title, page: path.relative(root, page0), tabs: [] };
  const seen = new Map(); let n = 0;
  const count = Math.max(1, tabSel);
  for (let t = 0; t < count; t++) {
    let label = 'Window';
    if (tabSel) { const el = p.locator(`[data-tourtab="${t}"]`); label = (await el.getAttribute('data-tl')) || (await el.innerText()).split('\n')[0].trim(); await el.click(); await p.waitForTimeout(500); }
    const d = await p.evaluate(() => {
      const frame = ['#frame', '#win', '#unit', '.unit'].map(s => document.querySelector(s)).find(Boolean) || document.body;
      const fr = frame.getBoundingClientRect();
      const vis = e => { const r = e.getBoundingClientRect(); const cs = getComputedStyle(e); return r.width > 6 && r.height > 6 && cs.visibility !== 'hidden' && cs.display !== 'none' && e.closest('[hidden]') === null && r.bottom > fr.top && r.top < fr.bottom; };
      let els = [...frame.querySelectorAll('[data-tip],[data-tipx]')].filter(vis);
      els = els.filter(e => !els.some(o => o !== e && e.contains(o)));   // leaf-most only
      const secOf = e => { for (let a = e.parentElement; a && a !== frame.parentElement; a = a.parentElement) { const h = [...a.children].find(c => c.matches('h2,h3,h4,legend,.sh,.title,.ttl,.hd') && c.textContent.trim().length < 40); if (h) { const own = [...h.childNodes].filter(x => x.nodeType === 3).map(x => x.textContent).join(' ').trim(); return (own || h.textContent.trim()).replace(/\s+/g, ' '); } if (a.tagName === 'HEADER') return 'Header bar'; } return 'Window'; };
      const first = s => { const m = s.match(/^[^.:!?]{3,40}/); return (m ? m[0] : s.slice(0, 40)).trim(); };
      const nameOf = e => {
        const tip = e.getAttribute('data-tipx') || e.getAttribute('data-tip') || '';
        let n = e.getAttribute('data-tl2') || e.getAttribute('aria-label') || '';
        if (!n && e.matches('button,select,a,.seg,.btn')) { n = (e.innerText || '').trim().split('\n')[0]; if (e.tagName === 'SELECT') n = (e.selectedOptions[0] || {}).textContent || ''; }
        if (!n || n.length > 28 || !/[A-Za-z0-9]{2}/.test(n)) {
          const wrap = e.closest('.knobwrap,.kw,.ctl,.cell,.fader,.col'); const lab = wrap && wrap.querySelector('.lbl,.label,label,.n,.name,small,span');
          n = lab && lab !== e && lab.textContent.trim().length < 24 && /[A-Za-z]/.test(lab.textContent) ? lab.textContent.trim() : first(tip);
        }
        return n.replace(/\s+/g, ' ');
      };
      const kindOf = e => e.tagName === 'CANVAS' || e.tagName === 'SVG' || e.matches('.display,.gfx,.scope,.wave') ? 'display' : e.matches('.knob,[role=slider]') ? 'knob' : e.tagName === 'SELECT' ? 'menu' : 'button';
      return { fr: [fr.x + scrollX, fr.y + scrollY, fr.width, fr.height], items: els.map(e => { const r = e.getBoundingClientRect(); return { name: nameOf(e), tip: (e.getAttribute('data-tipx') || e.getAttribute('data-tip') || '').replace(/\s+/g, ' ').trim(), kind: kindOf(e), sec: secOf(e), box: [r.x + scrollX, r.y + scrollY, r.width, r.height] }; }) };
    });
    const tab = { label, img: `window_${t + 1}.png`, items: [] }, badges = [];
    for (const it of d.items) {
      const key = it.name + '|' + it.tip; let num = seen.get(key), fresh = false;
      if (!num) { num = ++n; seen.set(key, num); fresh = true; }
      badges.push([num, it.box[0], it.box[1], it.kind === 'display' ? '#00B894' : '#FF1383']);
      tab.items.push({ n: num, name: it.name, tip: it.tip, kind: it.kind, sec: it.sec, fresh });
    }
    await p.evaluate(list => list.forEach(([txt, x, y, col]) => { const e = document.createElement('div'); e.className = 'badge87'; e.textContent = txt;
      e.style.cssText = `position:absolute;left:${x - 6}px;top:${y - 6}px;min-width:18px;height:18px;border-radius:9px;background:${col};color:#fff;font:bold 11px sans-serif;display:flex;align-items:center;justify-content:center;z-index:99999;box-shadow:0 0 0 2px #0a0a22;padding:0 3px;pointer-events:none`; document.body.appendChild(e); }), badges);
    await p.screenshot({ path: path.join(out, tab.img), fullPage: true, clip: { x: Math.max(0, d.fr[0]), y: Math.max(0, d.fr[1]), width: d.fr[2], height: d.fr[3] } });
    await p.evaluate(() => document.querySelectorAll('.badge87').forEach(e => e.remove()));
    result.tabs.push(tab);
  }
  fs.writeFileSync(path.join(out, 'tour.json'), JSON.stringify(result, null, 1));
  console.log(result.title, result.tabs.map(t => t.label + ':' + t.items.length).join(' | '), 'controls', n);
  await b.close();
})();
