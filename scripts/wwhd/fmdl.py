"""Wind Waker HD's models (Wii U BFRES v3 FMDL): each vertex in model space (bind pose), with its normal and colour.

FMDL -> FSKL (bones: scale, rotation, translation, parent; the matrix-to-bone list), FVTX (vertex buffers: _p0
positions, _n0 normals, _i0 skin indices, _c0 colours), FSHP (shapes: which buffer, which bone, which material,
skin count), FMAT (the materials' names). A shape with skin count 0 is rigid to its bone, 1 picks a bone per vertex
through _i0, 2 or more is stored in model space already. Also the materials' konst2 colour animations (FSHU, a
BFRES's fourth and fifth index groups), HD's lava glow colour (glow_curves). For HD's normals and vertex colours by
the GameCube's positions (normals.py); the native importer reads the same (runtime/host/src/wwhd/normals.h).
"""
from __future__ import annotations

from dataclasses import dataclass, field
import math
import struct

from .formats import FormatError
from .j3d import IDENTITY, apply, mat_mul

WANTED = ("_p0", "_n0", "_i0", "_c0")


def _u16(d, at):
    return struct.unpack_from(">H", d, at)[0]


def _u32(d, at):
    return struct.unpack_from(">I", d, at)[0]


def _rel(d, at):
    v = struct.unpack_from(">i", d, at)[0]
    return at + v if v else 0


def _name(d, at):
    if not at:
        return ""
    end = d.find(b"\0", at, at + 1024)
    if end < 0:
        raise FormatError(f"unterminated name at {at:#x}")
    return d[at:end].decode("ascii", "replace")


def _group(d, at):
    """A BFRES v3 index group's (name, data offset) entries (entry 0 is the tree's root)."""
    if not at:
        return []
    count = struct.unpack_from(">i", d, at + 4)[0]
    return [(_name(d, _rel(d, at + 16 + 16 * i)), _rel(d, at + 20 + 16 * i)) for i in range(1, count + 1)]


# GX2 attribute formats: the low byte is the layout; 0x100 integer, 0x200 signed, 0x800 integer to float.
_LAYOUT = {0x00: (1, "B", 1), 0x02: (1, "H", 2), 0x03: (1, "e", 2), 0x04: (2, "B", 1), 0x05: (1, "I", 4),
           0x06: (1, "f", 4), 0x07: (2, "H", 2), 0x08: (2, "e", 2), 0x0A: (4, "B", 1), 0x0C: (2, "I", 4),
           0x0D: (2, "f", 4), 0x0E: (4, "H", 2), 0x0F: (4, "e", 2), 0x10: (3, "I", 4), 0x11: (3, "f", 4),
           0x12: (4, "I", 4), 0x13: (4, "f", 4)}
_FLOATS = {0x03, 0x06, 0x08, 0x0D, 0x0F, 0x11, 0x13}


def _attribute(d, fmt, count, stride, start):
    kind, typ = fmt >> 8, fmt & 0xFF
    signed, integer = bool(kind & 0x2), bool(kind & 0x1)
    if typ == 0x0B:  # 10_10_10_2, x in the low bits
        out = []
        for i in range(count):
            w = _u32(d, start + i * stride)
            comps = []
            for s in (0, 10, 20):
                v = (w >> s) & 0x3FF
                if signed and v & 0x200:
                    v -= 0x400
                comps.append(v if integer or kind & 0x8 else (max(v / 511.0, -1.0) if signed else v / 1023.0))
            out.append(tuple(comps))
        return out
    if typ not in _LAYOUT:
        raise FormatError(f"vertex attribute format {fmt:#x} not supported")
    n, code, size = _LAYOUT[typ]
    if typ not in _FLOATS and signed:
        code = code.lower()
    if count and start + (count - 1) * stride + n * size > len(d):
        raise FormatError("vertex buffer runs past the file")
    st = struct.Struct(">" + code * n)
    raw = [st.unpack_from(d, start + i * stride) for i in range(count)]
    if typ in _FLOATS or integer or kind & 0x8:
        return raw
    top = float((1 << (8 * size - (1 if signed else 0))) - 1)
    if signed:
        return [tuple(max(v / top, -1.0) for v in r) for r in raw]
    return [tuple(v / top for v in r) for r in raw]


def _local(scale, rotation, translation, euler):
    if euler:  # X, then Y, then Z
        x, y, z = rotation[:3]
        cx, sx, cy, sy, cz, sz = math.cos(x), math.sin(x), math.cos(y), math.sin(y), math.cos(z), math.sin(z)
        r = ((cy * cz, sx * sy * cz - cx * sz, cx * sy * cz + sx * sz),
             (cy * sz, sx * sy * sz + cx * cz, cx * sy * sz - sx * cz),
             (-sy, sx * cy, cx * cy))
    else:
        x, y, z, w = rotation
        r = ((1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)),
             (2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)),
             (2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)))
    sx, sy, sz = scale
    return (r[0][0] * sx, r[0][1] * sy, r[0][2] * sz, translation[0],
            r[1][0] * sx, r[1][1] * sy, r[1][2] * sz, translation[1],
            r[2][0] * sx, r[2][1] * sy, r[2][2] * sz, translation[2])


def _normal_matrix(m):
    """The inverse transpose of a 3x4's 3x3 part."""
    a, b, c, _, d_, e, f, _, g, h, i, _ = m
    det = a * (e * i - f * h) - b * (d_ * i - f * g) + c * (d_ * h - e * g)
    if abs(det) < 1e-12:
        return IDENTITY
    k = 1.0 / det
    return ((e * i - f * h) * k, (f * g - d_ * i) * k, (d_ * h - e * g) * k, 0.0,
            (c * h - b * i) * k, (a * i - c * g) * k, (b * g - a * h) * k, 0.0,
            (b * f - c * e) * k, (c * d_ - a * f) * k, (a * e - b * d_) * k, 0.0)


def _turn(nm, v):
    x, y, z = v[:3]
    tx, ty, tz = nm[0] * x + nm[1] * y + nm[2] * z, nm[4] * x + nm[5] * y + nm[6] * z, nm[8] * x + nm[9] * y + nm[10] * z
    ln = math.sqrt(tx * tx + ty * ty + tz * tz) or 1.0
    return (tx / ln, ty / ln, tz / ln)


@dataclass
class Placed:
    """One HD model's vertices in model space: positions, normals (None where it has none), colours (RGBA 0..1,
    None where it has none) and their shapes' material names, one entry per vertex."""
    positions: list
    normals: list
    colours: list
    materials: list = field(default_factory=list)


def _skeleton(d, at):
    if d[at:at + 4] != b"FSKL":
        raise FormatError(f"no FSKL at {at:#x}")
    nbone, nsmooth, nrigid = _u16(d, at + 8), _u16(d, at + 0xA), _u16(d, at + 0xC)
    bones_at, m2b_at = _rel(d, at + 0x14), _rel(d, at + 0x18)
    m2b = list(struct.unpack_from(f">{nsmooth + nrigid}H", d, m2b_at)) if m2b_at and nsmooth + nrigid else []
    local, parent = [], []
    for i in range(nbone):
        b = bones_at + 0x40 * i
        parent.append(struct.unpack_from(">h", d, b + 6)[0])
        local.append(_local(struct.unpack_from(">3f", d, b + 0x14), struct.unpack_from(">4f", d, b + 0x20),
                            struct.unpack_from(">3f", d, b + 0x30), (_u32(d, b + 0x10) & 0x7000) == 0x1000))
    world = [None] * nbone

    def place(i, depth=0):
        if world[i] is None:
            if depth > 256:
                raise FormatError("bone parent loop")
            p = parent[i]
            world[i] = local[i] if p < 0 or p >= nbone else mat_mul(place(p, depth + 1), local[i])
        return world[i]

    for i in range(nbone):
        place(i)
    return world, m2b


def _buffer(d, at):
    if d[at:at + 4] != b"FVTX":
        raise FormatError(f"no FVTX at {at:#x}")
    nattr, nbuf, count = d[at + 4], d[at + 5], _u32(d, at + 8)
    attr_at, buf_at = _rel(d, at + 0x10), _rel(d, at + 0x18)
    bufs = [(_rel(d, buf_at + 0x18 * b + 0x14), _u16(d, buf_at + 0x18 * b + 0xC)) for b in range(nbuf)]
    attrs = {}
    for a in range(nattr):
        aa = attr_at + 0xC * a
        name = _name(d, _rel(d, aa))
        if name in WANTED:
            start, stride = bufs[d[aa + 4]]
            try:
                attrs[name] = _attribute(d, _u32(d, aa + 8), count, stride, start + _u16(d, aa + 6))
            except FormatError:
                attrs[name] = None
    return attrs


def read_models(bfres: bytes) -> dict:
    """Every FMDL of a Wii U BFRES: name -> Placed. A shape that can't be placed is left out."""
    if bfres[:4] != b"FRES":
        raise FormatError("not a BFRES file")
    if _u32(bfres, 4) >> 24 != 3:
        raise FormatError("BFRES version not supported (v3 only)")
    out = {}
    try:
        for name, fmdl in _group(bfres, _rel(bfres, 0x20)):
            if bfres[fmdl:fmdl + 4] != b"FMDL":
                raise FormatError(f"no FMDL at {fmdl:#x}")
            world, m2b = _skeleton(bfres, _rel(bfres, fmdl + 0xC))
            vtx_at = _rel(bfres, fmdl + 0x10)
            buffers = [_buffer(bfres, vtx_at + 0x20 * v) for v in range(_u16(bfres, fmdl + 0x20))]
            materials = [m for m, _ in _group(bfres, _rel(bfres, fmdl + 0x18))]
            placed = Placed([], [], [], [])
            for _sname, at in _group(bfres, _rel(bfres, fmdl + 0x14)):
                if bfres[at:at + 4] != b"FSHP":
                    raise FormatError(f"no FSHP at {at:#x}")
                bone, buf, skin = _u16(bfres, at + 0x10), _u16(bfres, at + 0x12), bfres[at + 0x16]
                mi = _u16(bfres, at + 0xE)
                material = materials[mi] if mi < len(materials) else ""
                if buf >= len(buffers):
                    continue
                attrs = buffers[buf]
                pos, nrm, col = attrs.get("_p0"), attrs.get("_n0"), attrs.get("_c0")
                if not pos:
                    continue
                if skin == 0:
                    if bone >= len(world):
                        continue
                    mats = [world[bone]] * len(pos)
                elif skin == 1:
                    idx = attrs.get("_i0")
                    if idx is None:
                        continue
                    picks = [int(v[0]) for v in idx]
                    if any(j >= len(m2b) or m2b[j] >= len(world) for j in picks):
                        continue
                    mats = [world[m2b[j]] for j in picks]
                else:
                    mats = [IDENTITY] * len(pos)
                normal_mats = {}
                for k, p in enumerate(pos):
                    m = mats[k]
                    placed.positions.append(apply(m, p[:3]))
                    if nrm:
                        if id(m) not in normal_mats:
                            normal_mats[id(m)] = _normal_matrix(m) if m is not IDENTITY else IDENTITY
                        placed.normals.append(_turn(normal_mats[id(m)], nrm[k]))
                    else:
                        placed.normals.append(None)
                    placed.colours.append((tuple(col[k]) + (1.0,) * (4 - len(col[k])))[:4] if col else None)
                    placed.materials.append(material)
            if name not in out:
                out[name] = placed
    except (struct.error, IndexError) as ex:
        raise FormatError(f"BFRES model: {ex}") from None
    return out


def _curve_frames(d, at, count, kind):
    if kind == 0:
        return list(struct.unpack_from(f">{count}f", d, at))
    if kind == 1:
        return [v / 32 for v in struct.unpack_from(f">{count}h", d, at)]
    return list(d[at:at + count])


def _curve(d, at):
    """One animation curve's segments, (start frame, end frame, coefficients) each (cubic: 4, linear: 2), or None
    for a kind the glow doesn't use (steps and baked curves)."""
    flags, count = _u16(d, at), _u16(d, at + 2)
    scale, offset = struct.unpack_from(">2f", d, at + 0x10)
    frame_kind, key_kind, curve_kind = flags & 3, (flags >> 2) & 3, (flags >> 4) & 7
    if curve_kind > 1 or key_kind > 2 or frame_kind > 2 or count == 0:
        return None
    per = 4 if curve_kind == 0 else 2
    frames = _curve_frames(d, _rel(d, at + 0x1C), count, frame_kind)
    keys_at, n = _rel(d, at + 0x20), count * per
    raw = (struct.unpack_from(f">{n}f", d, keys_at) if key_kind == 0 else
           struct.unpack_from(f">{n}h", d, keys_at) if key_kind == 1 else struct.unpack_from(f">{n}b", d, keys_at))
    segments = []
    for i in range(count):
        coeffs = [raw[i * per + j] * scale for j in range(per)]
        coeffs[0] += offset
        segments.append((frames[i], frames[i + 1] if i + 1 < count else frames[i], coeffs))
    return segments


def _value(segments, f):
    """A curve at frame f: its segment's polynomial in t, the share of the segment gone (the last key after its
    end)."""
    for start, end, c in segments:
        if start <= f <= end and end > start:
            t = (f - start) / (end - start)
            v = c[0] + c[1] * t
            if len(c) == 4:
                v = v + c[2] * t * t + c[3] * t * t * t
            return v
    return segments[-1][2][0]


def glow_curves(bfres: bytes) -> dict:
    """Material name -> its konst2 colour animation at each of its frames 0 .. count - 1, (r, g, b) linear: the
    lava's glow colour HD's lava shaders add (lava K x 4 x the vertex red squared, or in Dragon Roost Cavern
    (1 - alpha) squared; GanonB's walls, PS 4345 in Cemu, take it at the animation's frame, 0.453, 0.0415, 0.0005 at
    frame 23). Only materials whose r, g and b all
    animate (cubic or linear curves); the first animation that has a material wins."""
    out = {}
    if bfres[:4] != b"FRES":
        return out
    try:
        for group in (3, 4):
            for _name_, at in _group(bfres, _rel(bfres, 0x20 + 4 * group)):
                if bfres[at:at + 4] != b"FSHU":
                    continue
                frames, count, mats = _u32(bfres, at + 0x10), _u16(bfres, at + 0x14), _rel(bfres, at + 0x2C)
                if not 0 < frames <= 4096:
                    continue
                for m in range(count):
                    e = mats + 0x20 * m
                    nparam, material = _u16(bfres, e), _name(bfres, _rel(bfres, e + 0x10))
                    info, curves = _rel(bfres, e + 0x14), _rel(bfres, e + 0x18)
                    if material in out:
                        continue
                    for p in range(nparam):
                        pe = info + 0x10 * p
                        begin, floats = _u16(bfres, pe), _u16(bfres, pe + 2)
                        if _name(bfres, _rel(bfres, pe + 0xC)) != "konst2":
                            continue
                        rgb = {}
                        for c in range(floats):
                            ca = curves + 0x24 * (begin + c)
                            target = _u32(bfres, ca + 4)
                            if target in (0, 4, 8) and target // 4 not in rgb:
                                rgb[target // 4] = _curve(bfres, ca)
                        if len(rgb) == 3 and all(rgb.values()):
                            out[material] = tuple(tuple(_value(rgb[c], float(f)) for c in range(3))
                                                  for f in range(frames))
                        break
    except (struct.error, IndexError, FormatError):
        return {}
    return out
