"""GameCube J3D models (.bdl, .bmd): each vertex position with the joint or envelope it is drawn through.

Reads VTX1 (positions), JNT1 and INF1 (joints and their hierarchy, bind pose), DRW1 (draw matrix -> joint or
envelope) and SHP1 (display lists: which position index is drawn through which draw matrix, and which vertex
attributes the shapes carry). For Wind Waker HD's normals and vertex colours by the GameCube's positions
(normals.py). Big-endian throughout; the native importer reads the same (runtime/host/src/wwhd/normals.h).
"""
from __future__ import annotations

from dataclasses import dataclass, field
import math
import struct

from .formats import FormatError

IDENTITY = (1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0)
GX_POSITION, GX_NORMAL = 9, 10


def mat_mul(a, b):
    """Two 3x4 row-major matrices (12-tuples) multiplied."""
    out = []
    for r in range(3):
        a0, a1, a2, a3 = a[4 * r:4 * r + 4]
        for c in range(4):
            v = a0 * b[c] + a1 * b[4 + c] + a2 * b[8 + c]
            if c == 3:
                v += a3
            out.append(v)
    return tuple(out)


def apply(m, p):
    return (m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3],
            m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7],
            m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11])


def _joint_matrix(scale, rot, trans):
    x, y, z = (r * math.pi / 0x8000 for r in rot)
    cx, sx, cy, sy, cz, sz = math.cos(x), math.sin(x), math.cos(y), math.sin(y), math.cos(z), math.sin(z)
    r = ((cy * cz, sx * sy * cz - cx * sz, cx * sy * cz + sx * sz),
         (cy * sz, sx * sy * sz + cx * cz, cx * sy * sz - sx * cz),
         (-sy, sx * cy, cx * cy))
    s = scale
    return (r[0][0] * s[0], r[0][1] * s[1], r[0][2] * s[2], trans[0],
            r[1][0] * s[0], r[1][1] * s[1], r[1][2] * s[2], trans[1],
            r[2][0] * s[0], r[2][1] * s[1], r[2][2] * s[2], trans[2])


@dataclass
class Model:
    positions: list                 # raw VTX1 positions as float32 triples (empty unless they are float32)
    joints: list                    # each joint's bind-pose world matrix
    draw: list                      # per draw matrix: (weighted, joint or envelope index)
    uses: dict = field(default_factory=dict)   # position index -> draw matrix indices it is drawn through
    attributes: set = field(default_factory=set)  # GX vertex attributes the shapes carry (9 position, 10 normal)

    def rigid(self):
        """position index -> joint indices it is drawn through rigidly (envelopes left out), in index order."""
        out = {}
        for i in sorted(self.uses):
            if i >= len(self.positions):
                continue
            joints = []
            for d in sorted(self.uses[i], key=lambda v: -1 if v is None else v):
                if d is not None and d < len(self.draw):
                    weighted, j = self.draw[d]
                    if weighted:
                        continue
                else:
                    j = -1
                # A model with joints: a position drawn through none of them is left out; without joints, as is.
                if self.joints and not 0 <= j < len(self.joints):
                    continue
                if not self.joints:
                    j = -1
                if j not in joints:
                    joints.append(j)
            if joints:
                out[i] = joints
        return out

    def world(self, j):
        return self.joints[j] if 0 <= j < len(self.joints) else IDENTITY


def _chunks(d):
    at = 0x20
    while at + 8 <= len(d):
        tag, size = d[at:at + 4], struct.unpack_from(">I", d, at + 4)[0]
        if size < 8:
            break
        yield tag, at, size
        at += size


def _positions(d, at, size):
    """VTX1's positions when they are three float32s each, else []."""
    fmt_off = struct.unpack_from(">I", d, at + 8)[0]
    offs = struct.unpack_from(">13I", d, at + 12)
    p = at + fmt_off
    while True:
        attr, cnt, typ, _frac = struct.unpack_from(">IIIB", d, p)
        if attr == 0xFF:
            return []
        if attr == GX_POSITION:
            break
        p += 16
    if cnt != 1 or typ != 4 or not offs[0]:
        return []
    start = offs[0]
    later = [o for o in offs if o > start]
    end = min(later) if later else size
    n = (end - start) // 12
    return list(struct.iter_unpack(">3f", d[at + start:at + start + 12 * n]))


def _joints(d, jnt_at, inf_at):
    n = struct.unpack_from(">H", d, jnt_at + 8)[0]
    entries, remap, _names = struct.unpack_from(">3I", d, jnt_at + 0xC)
    order = struct.unpack_from(f">{n}H", d, jnt_at + remap) if remap else tuple(range(n))
    local = []
    for i in range(n):
        e = jnt_at + entries + 0x40 * order[i]
        local.append(_joint_matrix(struct.unpack_from(">3f", d, e + 4), struct.unpack_from(">3h", d, e + 0x10),
                                   struct.unpack_from(">3f", d, e + 0x18)))
    parents = [-1] * n
    if inf_at is not None:
        p = inf_at + struct.unpack_from(">I", d, inf_at + 0x14)[0]
        stack, last = [], -1
        while True:
            typ, idx = struct.unpack_from(">HH", d, p)
            p += 4
            if typ == 0x00:
                break
            if typ == 0x01:
                stack.append(last)
            elif typ == 0x02:
                last = stack.pop() if stack else -1
            elif typ == 0x10:
                if idx < n:
                    parents[idx] = next((j for j in reversed(stack) if j >= 0), -1)
                last = idx
    world = []
    for i in range(n):  # INF1 lists parents before children
        world.append(mat_mul(world[parents[i]], local[i]) if 0 <= parents[i] < i else local[i])
    return world


def _draws(d, at):
    n = struct.unpack_from(">H", d, at + 8)[0]
    w_at, i_at = struct.unpack_from(">2I", d, at + 0xC)
    return [(d[at + w_at + i] != 0, struct.unpack_from(">H", d, at + i_at + 2 * i)[0]) for i in range(n)]


def _uses(d, at):
    n = struct.unpack_from(">H", d, at + 8)[0]
    init_at, _remap, _names, desc_at, mtx_tab_at, dl_at, mtx_init_at, draw_init_at = struct.unpack_from(">8I", d, at + 0xC)
    uses, attributes = {}, set()
    for s in range(n):
        e = at + init_at + 0x28 * s
        mtx_type = d[e]
        groups, desc_off, mtx_first, draw_first = struct.unpack_from(">4H", d, e + 2)
        desc = []
        p = at + desc_at + desc_off
        while True:
            attr, kind = struct.unpack_from(">II", d, p)
            p += 8
            if attr == 0xFF:
                break
            desc.append((attr, kind))
            attributes.add(attr)
        slots = []  # a multi-matrix shape's draw matrix table (0xFFFF keeps the previous entry)
        for g in range(groups):
            use, count, first = struct.unpack_from(">HHI", d, at + mtx_init_at + 8 * (mtx_first + g))
            for k, v in enumerate(struct.unpack_from(f">{count}H", d, at + mtx_tab_at + 2 * first) if count else ()):
                if k >= len(slots):
                    slots.append(None)
                if v != 0xFFFF:
                    slots[k] = v
            dl_size, dl_off = struct.unpack_from(">II", d, at + draw_init_at + 8 * (draw_first + g))
            q, end = at + dl_at + dl_off, at + dl_at + dl_off + dl_size
            while q < end:
                op = d[q]
                q += 1
                if op == 0:
                    continue
                if op & 0xF8 not in (0x80, 0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xB8):
                    break
                nv = struct.unpack_from(">H", d, q)[0]
                q += 2
                for _ in range(nv):
                    mtx = pos = None
                    for attr, kind in desc:
                        if kind == 1:  # direct
                            if attr <= 8:
                                if attr == 0:
                                    mtx = d[q]
                                q += 1
                            elif attr in (11, 12):
                                q += 4
                            else:
                                raise FormatError(f"direct vertex attribute {attr} not supported")
                        elif kind == 2:
                            if attr == GX_POSITION:
                                pos = d[q]
                            q += 1
                        elif kind == 3:
                            if attr == GX_POSITION:
                                pos = struct.unpack_from(">H", d, q)[0]
                            q += 2
                    if pos is None:
                        continue
                    if mtx_type == 3:
                        k = mtx // 3 if mtx is not None else 0
                        draw = slots[k] if k < len(slots) else (use if mtx is None else None)
                    else:
                        draw = use
                    uses.setdefault(pos, set()).add(draw)
    return uses, attributes


def read_model(d: bytes) -> Model:
    if d[:4] not in (b"J3D1", b"J3D2") or d[4:8] not in (b"bdl4", b"bmd3", b"bmd2"):
        raise FormatError("not a J3D model")
    found = {tag: (at, size) for tag, at, size in _chunks(d)}
    if b"VTX1" not in found or b"SHP1" not in found:
        raise FormatError("no VTX1 or SHP1")
    try:
        joints = _joints(d, found[b"JNT1"][0], found[b"INF1"][0] if b"INF1" in found else None) if b"JNT1" in found else []
        model = Model(positions=_positions(d, *found[b"VTX1"]), joints=joints,
                      draw=_draws(d, found[b"DRW1"][0]) if b"DRW1" in found else [])
        model.uses, model.attributes = _uses(d, found[b"SHP1"][0])
    except (struct.error, IndexError) as ex:
        raise FormatError(f"J3D model: {ex}") from None
    return model
