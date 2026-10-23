"""Interface-tour pages for a Studio 87 manual, built from docs/tour/tour.json (written by tools/shoot_tour_generic.js).
Every row is a control of the interactive mockup and its info bubble text, read from the page, nothing written by hand."""
import json, os
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.lib.utils import ImageReader
from reportlab.platypus import Paragraph, Spacer, Table, TableStyle, Image, PageBreak, KeepTogether, CondPageBreak

PINK, TEAL, VIOLET = colors.HexColor('#FF1383'), colors.HexColor('#00B894'), colors.HexColor('#7900E3')

def _img(path, width, maxh):
    w, h = ImageReader(path).getSize()
    if width * h / w > maxh: width = maxh * w / h
    return Image(path, width=width, height=width * h / w)

def tour_story(tdir, h2, body, esc):
    p = os.path.join(tdir, 'tour.json')
    if not os.path.exists(p): return []
    d = json.load(open(p))
    cell = ParagraphStyle('tc', parent=body, fontSize=8.6, leading=11, spaceAfter=0)
    cellb = ParagraphStyle('tcb', parent=cell, fontName='Helvetica-Bold')
    sec = ParagraphStyle('ts', parent=body, fontName='Helvetica-Bold', fontSize=10.5, textColor=TEAL, spaceBefore=7, spaceAfter=3)
    out = [PageBreak(), Paragraph('Interface tour', h2),
           Paragraph(esc('Every pink number marks a control that has an info bubble in the interactive mockup (docs/mockup), and the tables below carry the text of those bubbles. The pictures come from the mockup, which mirrors the plugin window; the compiled plugin can differ by a few pixels of layout.'), body), Spacer(1, 4)]
    for t in d['tabs']:
        head = [Paragraph(esc(t['label']), ParagraphStyle('tt', parent=h2, spaceBefore=8))] if len(d['tabs']) > 1 else []
        out += [KeepTogether(head + [_img(os.path.join(tdir, t['img']), 174 * mm, 150 * mm)]), Spacer(1, 4)]
        groups = {}
        for it in t['items']:
            if it['fresh']: groups.setdefault(it['sec'], []).append(it)
        for s, items in groups.items():
            rows = [[Paragraph(str(i['n']), cellb), Paragraph(esc(i['name']), cellb), Paragraph(esc(i['tip']), cell)] for i in items]
            tb = Table(rows, colWidths=[11 * mm, 36 * mm, 127 * mm], repeatRows=0)
            tb.setStyle(TableStyle([('VALIGN', (0, 0), (-1, -1), 'TOP'), ('TEXTCOLOR', (0, 0), (0, -1), PINK), ('LINEBELOW', (0, 0), (-1, -1), 0.4, colors.HexColor('#dddddd')), ('TOPPADDING', (0, 0), (-1, -1), 2.5), ('BOTTOMPADDING', (0, 0), (-1, -1), 2.5)]))
            out += [CondPageBreak(40 * mm), Paragraph(esc(s), sec), tb]
    return out
