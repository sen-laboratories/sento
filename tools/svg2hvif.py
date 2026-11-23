#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 SEN Labs e.U.
#
# Converts a simple SVG (filled paths with solid colours or linear and radial gradients, nested groups and matrix transforms) to a Haiku Vector Icon
# (HVIF), written as the hex of an rdef resource, so that the icon of a program can be made from its SVG without Icon-O-Matic.
#
# Usage: svg2hvif.py <icon.svg> [--rdef] [--no-fit]    prints the bytes as hex (a resource "vector_icon" for an rdef with --rdef)
#
# The icon is 64x64 units. What is drawn is scaled to fill it (with a margin of 2 units, centred); with --no-fit the viewBox of the SVG is scaled to it. Every path of the SVG is a shape, in the order of the document, with its colour
# (and fill-opacity); a path of several subpaths (holes) is a shape of several paths. Curves are cubic Beziers (c, C), lines (l, L, h, H, v, V).
# Arcs (A, a) are made cubic curves. Gradients (userSpaceOnUse, with a gradientTransform) are written as gradient styles; for a radial one the circle of the radius 64 of Icon-O-Matic's own SVG export maps to the radius
# of the gradient, as it does there (r of any other value is scaled to it). A path that has only a stroke (fill:none) becomes a filled shape that is the path grown about its centre by half the
# stroke width on every side: right for a circle or any shape that is round about its centre (the outline under a globe), not for other shapes.

import re
import sys
import xml.etree.ElementTree as ET

NS = '{http://www.w3.org/2000/svg}'


def parse_transform(text):
    """a, b, c, d, e, f of the matrices in a transform attribute, multiplied (left to right)"""
    m = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    for name, args in re.findall(r'(\w+)\s*\(([^)]*)\)', text or ''):
        v = [float(x) for x in re.split(r'[\s,]+', args.strip()) if x]
        if name == 'matrix':
            t = tuple(v)
        elif name == 'translate':
            t = (1, 0, 0, 1, v[0], v[1] if len(v) > 1 else 0)
        elif name == 'scale':
            t = (v[0], 0, 0, v[1] if len(v) > 1 else v[0], 0, 0)
        else:
            raise ValueError('transform not supported: ' + name)
        m = multiply(m, t)
    return m


def multiply(m, n):
    """the matrix that does n first, then m (SVG: m is the parent)"""
    a, b, c, d, e, f = m
    A, B, C, D, E, F = n
    return (a * A + c * B, b * A + d * B, a * C + c * D, b * C + d * D, a * E + c * F + e, b * E + d * F + f)


def apply(m, x, y):
    a, b, c, d, e, f = m
    return (a * x + c * y + e, b * x + d * y + f)


def style_of(element):
    style = {}
    for part in (element.get('style') or '').split(';'):
        if ':' in part:
            k, v = part.split(':', 1)
            style[k.strip()] = v.strip()
    for key in ('fill', 'fill-opacity', 'opacity', 'stroke', 'stroke-width', 'stop-color', 'stop-opacity'):
        if element.get(key) is not None:
            style.setdefault(key, element.get(key))
    return style


GRADIENTS = {}		# id -> the element of a linearGradient or radialGradient (filled by convert)


def color_of(style, matrix=None):
    """(r, g, b, a) of a solid fill, or ('gradient', ...) of a gradient (matrix: the matrix of the path, which the gradient is in the user space of); None if nothing is drawn"""
    fill = style.get('fill', '#000000')
    if fill == 'none':
        return None
    if fill.startswith('url('):
        return gradient_of(fill[4:].rstrip(')').strip('#\'" '), matrix or (1, 0, 0, 1, 0, 0), float(style.get('fill-opacity', 1)) * float(style.get('opacity', 1)))
    if len(fill) == 4:
        fill = '#' + ''.join(ch * 2 for ch in fill[1:])
    r, g, b = int(fill[1:3], 16), int(fill[3:5], 16), int(fill[5:7], 16)
    alpha = float(style.get('fill-opacity', 1)) * float(style.get('opacity', 1))
    return (r, g, b, max(0, min(255, round(alpha * 255))))


def parse_color(text):
    text = text.strip()
    if len(text) == 4:
        text = '#' + ''.join(ch * 2 for ch in text[1:])
    return int(text[1:3], 16), int(text[3:5], 16), int(text[5:7], 16)


def gradient_of(ident, matrix, opacity):
    """a hashable description of a gradient for the shape with the matrix: ('gradient', type, (matrix of the 64-unit gradient space to the icon), ((offset, r, g, b, a), ...))"""
    element = GRADIENTS[ident]
    kind = element.tag.replace(NS, '')
    # a gradient may take its stops (and more) from another one by href
    stops_source = element
    href = element.get('{http://www.w3.org/1999/xlink}href') or element.get('href')
    while not [c for c in stops_source if c.tag.replace(NS, '') == 'stop'] and href:
        stops_source = GRADIENTS[href.lstrip('#')]
        href = stops_source.get('{http://www.w3.org/1999/xlink}href') or stops_source.get('href')
    stops = []
    for stop in stops_source:
        if stop.tag.replace(NS, '') != 'stop':
            continue
        style = style_of(stop)
        r, g, b = parse_color(style.get('stop-color', stop.get('stop-color', '#000000')))
        alpha = float(style.get('stop-opacity', stop.get('stop-opacity', 1))) * opacity
        offset = float((stop.get('offset') or '0').rstrip('%')) / (100.0 if (stop.get('offset') or '').endswith('%') else 1.0)
        stops.append((max(0.0, min(1.0, offset)), r, g, b, max(0, min(255, round(alpha * 255)))))
    if not stops:
        raise ValueError('gradient without stops: ' + ident)
    transform = parse_transform(element.get('gradientTransform'))
    if kind == 'radialGradient':
        cx, cy, radius = float(element.get('cx', 0)), float(element.get('cy', 0)), float(element.get('r', 64))
        # the space of the gradient: a circle of the radius 64 around the origin; here: a circle of the radius r around (cx, cy)
        local = (radius / 64.0, 0, 0, radius / 64.0, cx, cy)
        return ('gradient', 1, multiply(multiply(matrix, transform), local), tuple(stops))
    if kind == 'linearGradient':
        x1, y1, x2, y2 = (float(element.get(k, d)) for k, d in (('x1', 0), ('y1', 0), ('x2', 1), ('y2', 0)))
        import math
        length = math.hypot(x2 - x1, y2 - y1) or 1.0
        angle = math.atan2(y2 - y1, x2 - x1)
        # the space of the gradient: a line from (-64, 0) to (64, 0); here: from (x1, y1) to (x2, y2)
        scale = length / 128.0
        local = (scale * math.cos(angle), scale * math.sin(angle), -scale * math.sin(angle), scale * math.cos(angle), (x1 + x2) / 2, (y1 + y2) / 2)
        return ('gradient', 0, multiply(multiply(matrix, transform), local), tuple(stops))
    raise ValueError('gradient not supported: ' + kind)


def float24(value):
    """a number as the 24 bit float of HVIF (1 bit sign, 6 bits exponent with the bias 32, 17 bits mantissa), big endian"""
    import math
    if abs(value) < 2.0 ** -30:		# (zero, or what the arithmetic of a rotation leaves of it: below the smallest number of the format)
        return b'\x00\x00\x00'
    sign = 1 if value < 0 else 0
    mantissa, exponent = math.frexp(abs(value))		# value = mantissa * 2**exponent, 0.5 <= mantissa < 1
    mantissa, exponent = mantissa * 2, exponent - 1		# 1 <= mantissa < 2
    exponent += 32
    bits = int(round((mantissa - 1.0) * (1 << 17)))
    if bits == 1 << 17:
        bits, exponent = 0, exponent + 1
    if not 0 <= exponent < 64:
        raise ValueError('number out of range for a gradient transform: %s' % value)
    n = (sign << 23) | (exponent << 17) | bits
    return bytes([(n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF])


def tokens(d):
    return re.findall(r'[A-Za-z]|[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?', d)


def subpaths(d, matrix):
    """the subpaths of a path as (points, closed): a point is [x, y, xin, yin, xout, yout], in the units after the matrix"""
    toks = tokens(d)
    i = 0
    cmd = None
    x = y = 0.0
    start = (0.0, 0.0)
    result = []
    points = None

    def tx(px, py):
        return apply(matrix, px, py)

    def number():
        nonlocal i
        v = float(toks[i])
        i += 1
        return v

    def new_point(px, py):
        qx, qy = tx(px, py)
        return [qx, qy, qx, qy, qx, qy]

    def finish(closed):
        nonlocal points
        if points:
            if closed and len(points) > 1 and abs(points[0][0] - points[-1][0]) < 1e-6 and abs(points[0][1] - points[-1][1]) < 1e-6:
                # the last point is the first one again: they are one point, with the handles of both
                points[0][2], points[0][3] = points[-1][2], points[-1][3]
                points.pop()
            result.append((points, closed))
        points = None

    while i < len(toks):
        if re.match(r'[A-Za-z]', toks[i]):
            cmd = toks[i]
            i += 1
            if cmd in 'Zz':
                finish(True)
                x, y = start
                continue
        rel = cmd.islower()
        c = cmd.upper()
        if c == 'M':
            nx, ny = number(), number()
            if rel:
                nx, ny = x + nx, y + ny
            finish(False)
            x, y = nx, ny
            start = (x, y)
            points = [new_point(x, y)]
            cmd = 'l' if rel else 'L'	# more pairs are lines
        elif c == 'L':
            nx, ny = number(), number()
            if rel:
                nx, ny = x + nx, y + ny
            x, y = nx, ny
            points.append(new_point(x, y))
        elif c == 'H':
            nx = number()
            x = x + nx if rel else nx
            points.append(new_point(x, y))
        elif c == 'V':
            ny = number()
            y = y + ny if rel else ny
            points.append(new_point(x, y))
        elif c == 'C':
            c1 = (number(), number())
            c2 = (number(), number())
            p = (number(), number())
            if rel:
                c1 = (x + c1[0], y + c1[1])
                c2 = (x + c2[0], y + c2[1])
                p = (x + p[0], y + p[1])
            out = tx(*c1)
            points[-1][4], points[-1][5] = out
            point = new_point(*p)
            point[2], point[3] = tx(*c2)
            points.append(point)
            x, y = p
        elif c == 'A':
            rx, ry, rotation, large, sweep = number(), number(), number(), number(), number()
            p = (number(), number())
            if rel:
                p = (x + p[0], y + p[1])
            for c1, c2, end in arc_to_cubics(x, y, rx, ry, rotation, large != 0, sweep != 0, p[0], p[1]):
                out = tx(*c1)
                points[-1][4], points[-1][5] = out
                point = new_point(*end)
                point[2], point[3] = tx(*c2)
                points.append(point)
            x, y = p
        else:
            raise ValueError('path command not supported: ' + cmd)
    finish(False)
    return result


def arc_to_cubics(x1, y1, rx, ry, phi_degrees, large, sweep, x2, y2):
    """an elliptical arc of a path (SVG implementation notes F.6.5) as cubic Bezier curves [(c1, c2, end), ...], one for each quarter turn at most"""
    import math
    if (x1, y1) == (x2, y2):
        return []
    rx, ry = abs(rx), abs(ry)
    if rx == 0 or ry == 0:
        return [((x1, y1), (x2, y2), (x2, y2))]		# (a straight line)
    phi = math.radians(phi_degrees)
    cos_phi, sin_phi = math.cos(phi), math.sin(phi)
    dx, dy = (x1 - x2) / 2.0, (y1 - y2) / 2.0
    x1p = cos_phi * dx + sin_phi * dy
    y1p = -sin_phi * dx + cos_phi * dy
    lam = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry)
    if lam > 1:
        scale = math.sqrt(lam)
        rx, ry = rx * scale, ry * scale
    numerator = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p
    denominator = rx * rx * y1p * y1p + ry * ry * x1p * x1p
    factor = math.sqrt(max(0.0, numerator / denominator)) if denominator else 0.0
    if bool(large) == bool(sweep):
        factor = -factor
    cxp = factor * rx * y1p / ry
    cyp = -factor * ry * x1p / rx
    cx = cos_phi * cxp - sin_phi * cyp + (x1 + x2) / 2.0
    cy = sin_phi * cxp + cos_phi * cyp + (y1 + y2) / 2.0

    def angle(ux, uy, vx, vy):
        a = math.atan2(ux * vy - uy * vx, ux * vx + uy * vy)
        return a

    theta = angle(1, 0, (x1p - cxp) / rx, (y1p - cyp) / ry)
    delta = angle((x1p - cxp) / rx, (y1p - cyp) / ry, (-x1p - cxp) / rx, (-y1p - cyp) / ry)
    if not sweep and delta > 0:
        delta -= 2 * math.pi
    elif sweep and delta < 0:
        delta += 2 * math.pi
    count = max(1, int(math.ceil(abs(delta) / (math.pi / 2) - 1e-9)))
    step = delta / count
    t = 4.0 / 3.0 * math.tan(step / 4.0)
    curves = []
    a0 = theta
    for _ in range(count):
        a1 = a0 + step
        cos0, sin0, cos1, sin1 = math.cos(a0), math.sin(a0), math.cos(a1), math.sin(a1)
        # the points on the unit circle, then scaled, turned and moved
        p0 = (cos0, sin0)
        p3 = (cos1, sin1)
        p1 = (cos0 - t * sin0, sin0 + t * cos0)
        p2 = (cos1 + t * sin1, sin1 - t * cos1)

        def place(point):
            px, py = point[0] * rx, point[1] * ry
            return (cos_phi * px - sin_phi * py + cx, sin_phi * px + cos_phi * py + cy)

        curves.append((place(p1), place(p2), place(p3)))
        a0 = a1
    # (the last point is exactly the end that the path names)
    c1, c2, _ = curves[-1]
    curves[-1] = (c1, c2, (x2, y2))
    return curves


def coord(value):
    n = int(round((value + 128.0) * 102.0))
    if not 0 <= n < 0x8000:
        raise ValueError('coordinate out of range: %s' % value)
    return bytes([0x80 | (n >> 8), n & 0xFF])


def visible(color):
    return color is not None and (color[0] == 'gradient' or color[3] != 0)


def drawn_paths(element, matrix):
    """(path element, matrix, fill) of all the visible paths, in the order of the document: the fill is a colour or a gradient"""
    for child in element:
        tag = child.tag.replace(NS, '')
        if tag in ('defs', 'metadata', 'title') or 'namedview' in tag:
            continue
        m = multiply(matrix, parse_transform(child.get('transform')))
        if tag == 'g':
            if (child.get('style') or '').find('display:none') >= 0:
                continue
            yield from drawn_paths(child, m)
        elif tag == 'path':
            style = style_of(child)
            color = color_of(style, m)
            if color is None and style.get('stroke', 'none') != 'none':
                # only a stroke: the path grown by half the width on every side, filled with the colour of the stroke
                width = float(style.get('stroke-width', 1))
                xs, ys = [], []
                for points, _closed in subpaths(child.get('d'), (1, 0, 0, 1, 0, 0)):
                    for p in points:
                        xs += [p[0]]
                        ys += [p[1]]
                cx, cy = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
                radius = (max(xs) - min(xs)) / 2
                k = (radius + width / 2) / radius
                grow = (k, 0, 0, k, cx - k * cx, cy - k * cy)
                color = color_of({'fill': style['stroke'], 'fill-opacity': style.get('stroke-opacity', 1), 'opacity': style.get('opacity', 1)}, m)
                if visible(color):
                    yield child, multiply(m, grow), color
            elif visible(color):
                yield child, m, color


def float_bytes(matrix):
    return b''.join(float24(v) for v in matrix)


def convert(svg_file, fit=True):
    root = ET.parse(svg_file).getroot()
    GRADIENTS.clear()
    for element in root.iter():
        if element.tag.replace(NS, '') in ('linearGradient', 'radialGradient') and element.get('id'):
            GRADIENTS[element.get('id')] = element
    if root.get('viewBox'):
        box = [float(v) for v in re.split(r'[\s,]+', root.get('viewBox').strip())]
    else:
        # (no viewBox: the size of the drawing is its width and height, in the units of the paths; `64px` and the like are read as numbers)
        box = [0.0, 0.0, float(re.sub(r'[^0-9.]', '', root.get('width') or '64') or 64), float(re.sub(r'[^0-9.]', '', root.get('height') or '64') or 64)]
    scale = 64.0 / max(box[2], box[3])
    base = (scale, 0, 0, scale, -box[0] * scale, -box[1] * scale)

    if fit:
        # the bounds of what is drawn (the points and the handles of the curves)
        xs, ys = [], []
        for path, m, _ in drawn_paths(root, base):
            for points, _closed in subpaths(path.get('d'), m):
                for p in points:
                    xs += [p[0], p[2], p[4]]
                    ys += [p[1], p[3], p[5]]
        width, height = max(xs) - min(xs), max(ys) - min(ys)
        factor = 60.0 / max(width, height)
        offset_x = 32.0 - (min(xs) + max(xs)) / 2 * factor
        offset_y = 32.0 - (min(ys) + max(ys)) / 2 * factor
        fitting = (factor, 0, 0, factor, offset_x, offset_y)
        base = multiply(fitting, base)

    styles = []		# colours and gradients
    paths = []		# (points, closed)
    shapes = []		# (style index, [path indices])

    for element, m, color in drawn_paths(root, base):
        if color not in styles:
            styles.append(color)
        indices = []
        for points, closed in subpaths(element.get('d'), m):
            if len(points) < 2:
                continue
            paths.append((points, closed))
            indices.append(len(paths) - 1)
        if indices:
            shapes.append((styles.index(color), indices))

    out = bytearray(b'ncif')
    out.append(len(styles))
    for color in styles:
        if color[0] == 'gradient':
            _, kind, matrix, stops = color
            opaque = all(stop[4] == 255 for stop in stops)
            out += bytes([2, kind, 0x02 | (0x04 if opaque else 0), len(stops)])		# gradient, with a transform, without alpha if all are opaque
            out += float_bytes(matrix)
            for offset, r, g, b, a in stops:
                out += bytes([round(offset * 255), r, g, b] if opaque else [round(offset * 255), r, g, b, a])
        else:
            r, g, b, a = color
            out += bytes([1, r, g, b, a])	# solid colour with alpha
    out.append(len(paths))
    for points, closed in paths:
        out.append(0x02 if closed else 0x00)	# closed, curves for every point
        out.append(len(points))
        for p in points:
            for v in p:
                out += coord(v)
    out.append(len(shapes))
    for style, indices in shapes:
        out += bytes([10, style, len(indices)] + indices + [0])	# a shape of paths, no transform, no transformers
    if len(styles) > 255 or len(paths) > 255 or len(shapes) > 255:
        raise ValueError('too many styles, paths or shapes')
    return bytes(out)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    data = convert(sys.argv[1], fit='--no-fit' not in sys.argv)
    if '--rdef' in sys.argv:
        print('resource vector_icon array {')
        for i in range(0, len(data), 32):
            print('\t$"%s"' % data[i:i + 32].hex().upper())
        print('};')
    else:
        print(data.hex())
    return 0


if __name__ == '__main__':
    sys.exit(main())
