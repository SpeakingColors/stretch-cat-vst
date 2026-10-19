#!/usr/bin/env python3
"""Builds the PDF manual of Stretch Cat from docs/manual.txt (sections start with '# ', bullets with '- ').
Usage: python3 tools/make_manual.py <ignored> <docs/manual.txt> <out.pdf>      Needs: pip install reportlab"""
import sys, re
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, ListFlowable, ListItem

_, txt, out = sys.argv[1:4]
h1 = ParagraphStyle('h1', fontName='Helvetica-Bold', fontSize=22, textColor=colors.HexColor('#1B0264'), spaceAfter=6)
h2 = ParagraphStyle('h2', fontName='Helvetica-Bold', fontSize=13, textColor=colors.HexColor('#7900E3'), spaceBefore=12, spaceAfter=4)
body = ParagraphStyle('b', fontName='Helvetica', fontSize=10, leading=13.5, spaceAfter=4)
esc = lambda s: s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
story = [Paragraph('Stretch Cat', h1), Paragraph('Studio 87. Free and open source (AGPL-3.0-or-later).', body)]
sections, cur = [], None
for line in open(txt, encoding='utf-8').read().splitlines():
    if line.startswith('# '): cur = (line[2:].strip(), []); sections.append(cur)
    elif cur is not None: cur[1].append(line)
for title, lines in sections:
    story.append(Paragraph(esc(title), h2)); items = []; para = []
    def flush_para():
        global para
        if para: story.append(Paragraph(esc(' '.join(para)), body)); para = []
    def flush_items():
        global items
        if items: story.append(ListFlowable([ListItem(Paragraph(esc(i), body)) for i in items], bulletType='bullet', leftIndent=14)); items = []
    for l in lines:
        if l.startswith('- '): flush_para(); items.append(l[2:])
        elif not l.strip(): flush_para(); flush_items()
        else: flush_items(); para.append(l.strip())
    flush_para(); flush_items()
SimpleDocTemplate(out, pagesize=A4, leftMargin=18*mm, rightMargin=18*mm, topMargin=16*mm, bottomMargin=16*mm, title='Stretch Cat manual').build(story)
