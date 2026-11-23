#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 SEN Labs e.U.
#
# Makes an SVG that was drawn with more than svg2hvif.py reads into one that it reads: every shape (path, polygon, polyline, line, rect, circle, ellipse, with a rotate, translate, scale or
# matrix transform) is a path in the coordinates of the drawing, a stroke is a shape of its own (the line grown by half the width, with round or flat caps, as the painter's order has it:
# the fill of an element, then its stroke), the opacity of an element and of its groups is the opacity of its fills, and a gradient in the units of the bounding box of the shape (the default of
# SVG: x1="0%") is one in the units of the drawing, made for that shape. Needs `shapely` (pip install shapely).
#
# Usage: svg-prepare.py <in.svg> <out.svg>      then: svg2hvif.py <out.svg> [--rdef]
#
# What it does not do: clip paths, masks, filters, text, images, `use`; a radial gradient's focal point is dropped; the stroke of a path that has arcs or curves is not made (the stroke of
# lines and polygons is). It says so (an error) where the drawing needs more.

import math
import re
import sys
import xml.etree.ElementTree as ET

from shapely.geometry import LineString, MultiPolygon, Polygon
from shapely.geometry.polygon import orient

NS = '{http://www.w3.org/2000/svg}'
KAPPA = 0.5522847498307936
INHERITED = ('fill', 'stroke', 'stroke-width', 'stroke-linecap', 'stroke-linejoin', 'fill-opacity', 'stroke-opacity')


def tag_of(element):
    return element.tag.replace(NS, '')


def multiply(m, n):
    a, b, c, d, e, f = m
    A, B, C, D, E, F = n
    return (a * A + c * B, b * A + d * B, a * C + c * D, b * C + d * D, a * E + c * F + e, b * E + d * F + f)


def transform_of(text):
    m = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    for name, args in re.findall(r'(\w+)\s*\(([^)]*)\)', text or ''):
        v = [float(x) for x in re.split(r'[\s,]+', args.strip()) if x]
        if name == 'matrix':
            t = tuple(v)
        elif name == 'translate':
            t = (1, 0, 0, 1, v[0], v[1] if len(v) > 1 else 0)
        elif name == 'scale':
            t = (v[0], 0, 0, v[1] if len(v) > 1 else v[0], 0, 0)
        elif name == 'rotate':
            a = math.radians(v[0])
            cos, sin = math.cos(a), math.sin(a)
            t = (cos, sin, -sin, cos, 0, 0)
            if len(v) == 3:
                t = multiply(multiply((1, 0, 0, 1, v[1], v[2]), t), (1, 0, 0, 1, -v[1], -v[2]))
        else:
            raise ValueError('transform not supported: ' + name)
        m = multiply(m, t)
    return m


def apply(m, x, y):
    a, b, c, d, e, f = m
    return (a * x + c * y + e, b * x + d * y + f)


def style_of(element):
    style = {}
    for part in (element.get('style') or '').split(';'):
        if ':' in part:
            k, v = part.split(':', 1)
            style[k.strip()] = v.strip()
    for key in INHERITED + ('opacity',):
        if element.get(key) is not None:
            style.setdefault(key, element.get(key))
    return style


def path_points(d):
    """the subpaths of an absolute path (M, L, H, V, A, C and Z): lists of ('L', x, y) / ('C', x1, y1, x2, y2, x, y) / ('A', rx, ry, rot, large, sweep, x, y) with the closing"""
    tokens = re.findall(r'[A-Za-z]|-?\d*\.?\d+(?:[eE][-+]?\d+)?', d)
    subpaths = []
    current = None
    i = 0
    command = None
    x = y = 0.0

    def take(n):
        nonlocal i
        values = [float(t) for t in tokens[i:i + n]]
        i += n
        return values

    while i < len(tokens):
        if re.match(r'[A-Za-z]', tokens[i]):
            command = tokens[i]
            i += 1
            if command in 'Zz':
                if current is not None:
                    current['closed'] = True
                continue
        if command == 'M':
            x, y = take(2)
            current = {'start': (x, y), 'segments': [], 'closed': False}
            subpaths.append(current)
            command = 'L'
        elif command == 'L':
            x, y = take(2)
            current['segments'].append(('L', x, y))
        elif command == 'H':
            x, = take(1)
            current['segments'].append(('L', x, y))
        elif command == 'V':
            y, = take(1)
            current['segments'].append(('L', x, y))
        elif command == 'C':
            x1, y1, x2, y2, x, y = take(6)
            current['segments'].append(('C', x1, y1, x2, y2, x, y))
        elif command == 'A':
            rx, ry, rot, large, sweep, x, y = take(7)
            current['segments'].append(('A', rx, ry, rot, large, sweep, x, y))
        else:
            raise ValueError('path command not supported: %s (use absolute M L H V C A Z)' % command)
    return subpaths


def path_d(subpaths):
    out = []
    for sp in subpaths:
        out.append('M %.3f,%.3f' % sp['start'])
        for seg in sp['segments']:
            if seg[0] == 'L':
                out.append('L %.3f,%.3f' % seg[1:])
            elif seg[0] == 'C':
                out.append('C %.3f,%.3f %.3f,%.3f %.3f,%.3f' % seg[1:])
            else:
                out.append('A %.3f,%.3f %.3f %d,%d %.3f,%.3f' % (seg[1], seg[2], seg[3], int(seg[4]), int(seg[5]), seg[6], seg[7]))
        if sp['closed']:
            out.append('Z')
    return ' '.join(out)


def polygon_subpaths(points, closed=True):
    return [{'start': points[0], 'segments': [('L', px, py) for px, py in points[1:]], 'closed': closed}]


def ellipse_subpaths(cx, cy, rx, ry, m):
    """four cubic curves, in the coordinates after the matrix m"""
    k = KAPPA
    pts = [
        (cx + rx, cy), (cx + rx, cy + k * ry), (cx + k * rx, cy + ry), (cx, cy + ry),
        (cx - k * rx, cy + ry), (cx - rx, cy + k * ry), (cx - rx, cy),
        (cx - rx, cy - k * ry), (cx - k * rx, cy - ry), (cx, cy - ry),
        (cx + k * rx, cy - ry), (cx + rx, cy - k * ry), (cx + rx, cy),
    ]
    pts = [apply(m, *p) for p in pts]
    segs = [('C',) + pts[1 + 3 * n] + pts[2 + 3 * n] + pts[3 + 3 * n] for n in range(4)]
    return [{'start': pts[0], 'segments': segs, 'closed': True}]


def transformed(subpaths, m):
    if m == (1.0, 0.0, 0.0, 1.0, 0.0, 0.0):
        return subpaths
    out = []
    for sp in subpaths:
        segs = []
        for seg in sp['segments']:
            if seg[0] == 'L':
                segs.append(('L',) + apply(m, seg[1], seg[2]))
            elif seg[0] == 'C':
                segs.append(('C',) + apply(m, seg[1], seg[2]) + apply(m, seg[3], seg[4]) + apply(m, seg[5], seg[6]))
            else:
                raise ValueError('an arc in a path with a transform is not supported')
        out.append({'start': apply(m, *sp['start']), 'segments': segs, 'closed': sp['closed']})
    return out


def bounds(subpaths):
    xs, ys = [], []
    for sp in subpaths:
        xs.append(sp['start'][0])
        ys.append(sp['start'][1])
        for seg in sp['segments']:
            if seg[0] == 'L':
                xs.append(seg[1]); ys.append(seg[2])
            elif seg[0] == 'C':
                xs += [seg[1], seg[3], seg[5]]; ys += [seg[2], seg[4], seg[6]]
            else:
                xs.append(seg[6]); ys.append(seg[7])
    return min(xs), min(ys), max(xs), max(ys)


def flatten(sp):
    """the points of a subpath for the stroke: the curves are sampled (a stroked path with arcs is not supported)"""
    points = [sp['start']]
    for seg in sp['segments']:
        if seg[0] == 'L':
            points.append((seg[1], seg[2]))
        elif seg[0] == 'C':
            x0, y0 = points[-1]
            x1, y1, x2, y2, x3, y3 = seg[1:]
            steps = 12
            for n in range(1, steps + 1):
                t = n / steps
                u = 1 - t
                points.append((u * u * u * x0 + 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t * x3,
                               u * u * u * y0 + 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t * y3))
        else:
            raise ValueError('the stroke of a path with arcs is not supported')
    if sp['closed'] and len(points) > 1 and points[0] == points[-1]:
        points.pop()
    return points


def stroke_subpaths(subpaths, width, cap, join, quad):
    """the stroke of lines and polygons as the shapes of a buffer"""
    pieces = []
    cap_style = {'round': 'round', 'square': 'square'}.get(cap, 'flat')
    join_style = {'miter': 'mitre', 'bevel': 'bevel'}.get(join, 'round')
    for sp in subpaths:
        points = flatten(sp)
        if sp['closed'] and len(points) >= 3:
            geometry = Polygon(points).exterior.buffer(width / 2.0, quad_segs=quad, join_style=join_style)
        else:
            geometry = LineString(points).buffer(width / 2.0, quad_segs=quad, cap_style=cap_style, join_style=join_style)
        pieces.append(geometry)
    out = []
    for geometry in pieces:
        for polygon in (geometry.geoms if isinstance(geometry, MultiPolygon) else [geometry]):
            polygon = orient(polygon.simplify(0.03), 1.0)
            rings = [polygon.exterior] + list(polygon.interiors)
            for ring in rings:
                coords = [(round(px, 3), round(py, 3)) for px, py in ring.coords][:-1]
                out.append({'start': coords[0], 'segments': [('L', px, py) for px, py in coords[1:]], 'closed': True})
    return out


class Preparer:
    def __init__(self, root, quad):
        self.root = root
        self.quad = quad
        self.gradients = {}
        for element in root.iter():
            if tag_of(element) in ('linearGradient', 'radialGradient') and element.get('id'):
                self.gradients[element.get('id')] = element
        self.defs = []
        self.shapes = []
        self.count = 0

    def stops_of(self, element):
        source = element
        href = element.get('{http://www.w3.org/1999/xlink}href') or element.get('href')
        while not [c for c in source if tag_of(c) == 'stop'] and href:
            source = self.gradients[href.lstrip('#')]
            href = source.get('{http://www.w3.org/1999/xlink}href') or source.get('href')
        return [c for c in source if tag_of(c) == 'stop']

    def gradient_for(self, ident, box):
        """the id of a gradient in the units of the drawing for the shape with the bounding box"""
        element = self.gradients[ident]
        x0, y0, x1, y1 = box
        w, h = max(x1 - x0, 1e-6), max(y1 - y0, 1e-6)
        self.count += 1
        new = 'g%d' % self.count
        kind = tag_of(element)
        if kind == 'linearGradient':
            def coordinate(name, default, origin, extent):
                text = element.get(name, default)
                return origin + (float(text[:-1]) / 100.0 * extent if text.endswith('%') else float(text) * extent)
            g = ET.Element('linearGradient', {
                'id': new, 'gradientUnits': 'userSpaceOnUse',
                'x1': '%.3f' % coordinate('x1', '0%', x0, w), 'y1': '%.3f' % coordinate('y1', '0%', y0, h),
                'x2': '%.3f' % coordinate('x2', '100%', x0, w), 'y2': '%.3f' % coordinate('y2', '0%', y0, h)})
        else:
            def fraction(name, default):
                text = element.get(name, default)
                return float(text[:-1]) / 100.0 if text.endswith('%') else float(text)
            g = ET.Element('radialGradient', {
                'id': new, 'gradientUnits': 'userSpaceOnUse',
                'cx': '%.3f' % (x0 + fraction('cx', '50%') * w), 'cy': '%.3f' % (y0 + fraction('cy', '50%') * h),
                'r': '%.3f' % (fraction('r', '50%') * max(w, h))})
        for stop in self.stops_of(element):
            style = style_of(stop)
            attrs = {'offset': stop.get('offset', '0'), 'stop-color': style.get('stop-color', stop.get('stop-color', '#000000'))}
            if style.get('stop-opacity') is not None:
                attrs['stop-opacity'] = style['stop-opacity']
            ET.SubElement(g, 'stop', attrs)
        self.defs.append(g)
        return new

    def emit(self, subpaths, fill, opacity):
        if not subpaths or fill in (None, 'none'):
            return
        attrs = {'d': path_d(subpaths)}
        match = re.match(r'url\(#([^)]+)\)', fill)
        if match:
            attrs['fill'] = 'url(#%s)' % self.gradient_for(match.group(1), bounds(subpaths))
        else:
            attrs['fill'] = fill
        if opacity < 1.0:
            attrs['fill-opacity'] = '%.3f' % opacity
        self.shapes.append(ET.Element('path', attrs))

    def walk(self, parent, matrix, inherited, opacity):
        for element in parent:
            kind = tag_of(element)
            if kind in ('defs', 'metadata', 'title', 'desc') or 'namedview' in kind:
                continue
            style = style_of(element)
            if style.get('display') == 'none':
                continue
            m = multiply(matrix, transform_of(element.get('transform')))
            own = dict(inherited)
            for key in INHERITED:
                if key in style:
                    own[key] = style[key]
            alpha = opacity * float(style.get('opacity', 1))
            if kind == 'g':
                self.walk(element, m, own, alpha)
                continue
            if kind == 'path':
                subpaths = transformed(path_points(element.get('d')), m)
            elif kind in ('polygon', 'polyline'):
                nums = [float(v) for v in re.split(r'[\s,]+', element.get('points').strip()) if v]
                points = [apply(m, nums[n], nums[n + 1]) for n in range(0, len(nums), 2)]
                subpaths = polygon_subpaths(points, kind == 'polygon')
            elif kind == 'line':
                p1 = apply(m, float(element.get('x1', 0)), float(element.get('y1', 0)))
                p2 = apply(m, float(element.get('x2', 0)), float(element.get('y2', 0)))
                subpaths = polygon_subpaths([p1, p2], False)
            elif kind == 'rect':
                x, y, w, h = (float(element.get(k, 0)) for k in ('x', 'y', 'width', 'height'))
                subpaths = polygon_subpaths([apply(m, x, y), apply(m, x + w, y), apply(m, x + w, y + h), apply(m, x, y + h)])
            elif kind in ('circle', 'ellipse'):
                cx, cy = float(element.get('cx', 0)), float(element.get('cy', 0))
                rx = float(element.get('r') or element.get('rx'))
                ry = float(element.get('r') or element.get('ry'))
                subpaths = ellipse_subpaths(cx, cy, rx, ry, m)
            else:
                raise ValueError('element not supported: ' + kind)
            fill = own.get('fill', '#000000')
            fill_opacity = alpha * float(own.get('fill-opacity', 1))
            # (an open line has no area: its fill is nothing)
            if kind != 'polyline' and kind != 'line':
                self.emit(subpaths, fill, fill_opacity)
            stroke = own.get('stroke', 'none')
            if stroke != 'none':
                scale = math.sqrt(abs(m[0] * m[3] - m[1] * m[2]))
                width = float(own.get('stroke-width', 1)) * scale
                pieces = stroke_subpaths(subpaths, width, own.get('stroke-linecap', 'butt'), own.get('stroke-linejoin', 'miter'), self.quad)
                self.emit(pieces, stroke, alpha * float(own.get('stroke-opacity', 1)))

    def result(self):
        box = self.root.get('viewBox') or '0 0 64 64'
        svg = ET.Element('svg', {'xmlns': 'http://www.w3.org/2000/svg', 'viewBox': box})
        if self.defs:
            defs = ET.SubElement(svg, 'defs')
            for g in self.defs:
                defs.append(g)
        for shape in self.shapes:
            svg.append(shape)
        return svg


def main():
    if len(sys.argv) < 3:
        sys.exit('usage: svg-prepare.py <in.svg> <out.svg> [segments per quarter circle of a round cap, default 2]')
    root = ET.parse(sys.argv[1]).getroot()
    preparer = Preparer(root, int(sys.argv[3]) if len(sys.argv) > 3 else 2)
    preparer.walk(root, (1.0, 0.0, 0.0, 1.0, 0.0, 0.0), {}, 1.0)
    ET.indent(preparer.result())
    ET.ElementTree(preparer.result()).write(sys.argv[2], encoding='unicode', xml_declaration=False)
    print('%d shapes, %d gradients' % (len(preparer.shapes), len(preparer.defs)))


if __name__ == '__main__':
    main()
