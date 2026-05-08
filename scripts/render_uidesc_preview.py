#!/usr/bin/env python3
"""Render a `.uidesc` XML file to a static preview PNG for iteration purposes.

This is a faithful-but-simplified renderer of the subset of VSTGUI views we
emit from `scripts/generate_uidesc.py`:

- CViewContainer / CView background rectangles
- CSlider (orientation, draw-back, draw-value, draw-value-from-center)
- CTextLabel (font + color + alignment)
- CTextButton (rounded-rect + text)
- COptionMenu (rendered as a flat dropdown rect)

The goal is to let us iterate on the layout/design without spinning up a DAW.
The visual is approximate (e.g. rounded-rect corners are square, anti-aliasing
differs from VSTGUI) but pixel positions and sizes are exact.
"""
from __future__ import annotations

import argparse
import os
import re
import sys
import xml.etree.ElementTree as ET
from typing import Optional, Tuple

from PIL import Image, ImageDraw, ImageFont

DEFAULT_BG = (0, 0, 0, 255)


def parse_rgba(s: str) -> Tuple[int, int, int, int]:
    s = s.strip().lstrip('#')
    if len(s) == 6:
        s = s + 'ff'
    r = int(s[0:2], 16)
    g = int(s[2:4], 16)
    b = int(s[4:6], 16)
    a = int(s[6:8], 16)
    return (r, g, b, a)


def parse_origin(s: str) -> Tuple[int, int]:
    parts = re.split(r'\s*,\s*', s.strip())
    return int(float(parts[0])), int(float(parts[1]))


def parse_size(s: str) -> Tuple[int, int]:
    return parse_origin(s)


def find_font(size: int, bold: bool = False) -> ImageFont.ImageFont:
    """Try to find a condensed-style font that approximates SF Pro Condensed."""
    candidates_bold = [
        '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',
        '/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf',
        '/usr/share/fonts/TTF/DejaVuSans-Bold.ttf',
    ]
    candidates_reg = [
        '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',
        '/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf',
        '/usr/share/fonts/TTF/DejaVuSans.ttf',
    ]
    for p in (candidates_bold if bold else candidates_reg):
        if os.path.exists(p):
            try:
                return ImageFont.truetype(p, size)
            except Exception:
                pass
    return ImageFont.load_default()


class UIDescRenderer:
    def __init__(self, root: ET.Element):
        self.colors: dict[str, Tuple[int, int, int, int]] = {}
        self.fonts: dict[str, dict] = {}
        for c in root.findall('./colors/color'):
            self.colors[c.attrib['name']] = parse_rgba(c.attrib['rgba'])
        for f in root.findall('./fonts/font'):
            self.fonts[f.attrib['name']] = {
                'size': int(f.attrib.get('size', '10')),
                'bold': f.attrib.get('bold', 'false') == 'true',
            }
        self.template = root.find('./template')

    def col(self, name: str) -> Tuple[int, int, int, int]:
        if name in self.colors:
            return self.colors[name]
        try:
            return parse_rgba(name)
        except Exception:
            return (255, 0, 255, 255)

    def render(self) -> Image.Image:
        if self.template is None:
            raise ValueError('no template element found')
        w, h = parse_size(self.template.attrib['size'])
        bg = self.col(self.template.attrib.get('background-color', 'Backdrop'))
        img = Image.new('RGBA', (w, h), bg)
        draw = ImageDraw.Draw(img, 'RGBA')
        self._render_children(draw, self.template, 0, 0, w, h)
        return img

    # ---------------- rendering -----------------
    def _render_children(self, draw, parent, ox, oy, pw, ph):
        for child in parent:
            self._render_view(draw, child, ox, oy, pw, ph)

    def _render_view(self, draw, view, ox, oy, pw, ph):
        if view.tag != 'view':
            return
        attrs = view.attrib
        cls = attrs.get('class', '')
        x_local, y_local = parse_origin(attrs.get('origin', '0, 0'))
        w, h = parse_size(attrs.get('size', '0, 0'))
        x = ox + x_local
        y = oy + y_local

        if w <= 0 or h <= 0:
            return

        if cls in ('CView', 'CViewContainer'):
            transparent = attrs.get('transparent', 'true') == 'true'
            if not transparent:
                bg = self.col(attrs.get('background-color', 'Backdrop'))
                draw.rectangle([x, y, x + w - 1, y + h - 1], fill=bg)
            self._render_children(draw, view, x, y, w, h)
        elif cls == 'CSlider':
            self._draw_slider(draw, attrs, x, y, w, h)
        elif cls == 'CTextLabel':
            self._draw_label(draw, attrs, x, y, w, h)
        elif cls == 'CTextButton':
            self._draw_button(draw, attrs, x, y, w, h)
        elif cls == 'COptionMenu':
            self._draw_dropdown(draw, attrs, x, y, w, h)
        else:
            return

    def _draw_slider(self, draw, attrs, x, y, w, h):
        opacity = float(attrs.get('opacity', '1'))
        if opacity == 0:
            return
        orientation = attrs.get('orientation', 'horizontal')
        draw_back = attrs.get('draw-back', 'true') == 'true'
        draw_val = attrs.get('draw-value', 'true') == 'true'
        from_center = attrs.get('draw-value-from-center', 'false') == 'true'
        back_color = self.col(attrs.get('draw-back-color', 'Backdrop'))
        fill_color = self.col(attrs.get('draw-value-color', 'Backdrop'))
        default_value = float(attrs.get('default-value', '0.5'))

        if draw_back:
            draw.rectangle([x, y, x + w - 1, y + h - 1], fill=back_color)
        if not draw_val:
            return

        if orientation == 'vertical':
            if from_center:
                cy = y + h // 2
                v = default_value - 0.5
                fill_h = int(abs(v) * h)
                if v >= 0:
                    draw.rectangle([x, cy - fill_h, x + w - 1, cy], fill=fill_color)
                else:
                    draw.rectangle([x, cy, x + w - 1, cy + fill_h], fill=fill_color)
            else:
                fill_h = int(default_value * h)
                draw.rectangle([x, y + h - fill_h, x + w - 1, y + h - 1], fill=fill_color)
        else:
            if from_center:
                cx = x + w // 2
                v = default_value - 0.5
                fill_w = int(abs(v) * w)
                if v >= 0:
                    draw.rectangle([cx, y, cx + fill_w, y + h - 1], fill=fill_color)
                else:
                    draw.rectangle([cx - fill_w, y, cx, y + h - 1], fill=fill_color)
            else:
                fill_w = int(default_value * w)
                draw.rectangle([x, y, x + fill_w, y + h - 1], fill=fill_color)

    def _draw_label(self, draw, attrs, x, y, w, h):
        title = attrs.get('title', '')
        font_name = attrs.get('font', '')
        color = self.col(attrs.get('font-color', 'TextBright'))
        align = attrs.get('text-alignment', 'center')
        finfo = self.fonts.get(font_name, {'size': 10, 'bold': False})
        font = find_font(finfo['size'], finfo['bold'])

        if not (attrs.get('transparent', 'true') == 'true'):
            bg = self.col(attrs.get('back-color', 'Backdrop'))
            draw.rectangle([x, y, x + w - 1, y + h - 1], fill=bg)

        try:
            tw_box = draw.textbbox((0, 0), title, font=font)
            tw = tw_box[2] - tw_box[0]
            th = tw_box[3] - tw_box[1]
        except Exception:
            tw, th = font.getsize(title)

        if align == 'center':
            tx = x + (w - tw) // 2
        elif align == 'right':
            tx = x + w - tw - 2
        else:
            tx = x + 2
        ty = y + max(0, (h - th) // 2) - 1
        draw.text((tx, ty), title, font=font, fill=color)

    def _draw_button(self, draw, attrs, x, y, w, h):
        title = attrs.get('title', '')
        fill = self.col(attrs.get('back-color', 'Backdrop'))
        frame = self.col(attrs.get('frame-color', 'Backdrop'))
        text_color = self.col(attrs.get('font-color', 'TextBright'))
        radius = int(float(attrs.get('round-rect-radius', '0')))
        finfo = self.fonts.get(attrs.get('font', ''), {'size': 10, 'bold': False})
        font = find_font(finfo['size'], finfo['bold'])
        if hasattr(draw, 'rounded_rectangle') and radius > 0:
            draw.rounded_rectangle([x, y, x + w - 1, y + h - 1], radius=radius, fill=fill, outline=frame)
        else:
            draw.rectangle([x, y, x + w - 1, y + h - 1], fill=fill, outline=frame)
        try:
            tw_box = draw.textbbox((0, 0), title, font=font)
            tw = tw_box[2] - tw_box[0]
            th = tw_box[3] - tw_box[1]
        except Exception:
            tw, th = font.getsize(title)
        tx = x + (w - tw) // 2
        ty = y + max(0, (h - th) // 2) - 1
        draw.text((tx, ty), title, font=font, fill=text_color)

    def _draw_dropdown(self, draw, attrs, x, y, w, h):
        fill = self.col(attrs.get('back-color', 'Backdrop'))
        frame = self.col(attrs.get('frame-color', 'Backdrop'))
        radius = int(float(attrs.get('round-rect-radius', '0')))
        if hasattr(draw, 'rounded_rectangle') and radius > 0:
            draw.rounded_rectangle([x, y, x + w - 1, y + h - 1], radius=radius, fill=fill, outline=frame)
        else:
            draw.rectangle([x, y, x + w - 1, y + h - 1], fill=fill, outline=frame)
        # tiny down chevron
        cx = x + w - 10
        cy = y + h // 2
        draw.polygon([(cx, cy - 2), (cx + 6, cy - 2), (cx + 3, cy + 2)],
                     fill=self.col(attrs.get('font-color', 'TextBright')))


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument('uidesc', help='path to .uidesc XML')
    ap.add_argument('-o', '--output', default='preview.png')
    args = ap.parse_args(argv)

    tree = ET.parse(args.uidesc)
    root = tree.getroot()
    renderer = UIDescRenderer(root)
    img = renderer.render()
    img.save(args.output)
    print(f'wrote {args.output} ({img.size[0]}x{img.size[1]})')


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
