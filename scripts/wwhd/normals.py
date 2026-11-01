"""Wind Waker HD's normals and vertex colours, keyed by the GameCube's exact vertex positions, for the HD look's sun
shadows (BlueWake's facing term and HD's light; RecompCore's aurora_set_hd_normals and aurora_set_hd_object_tables).

HD remade its models on the GameCube's vertex positions, with normals and vertex colours of its own. For each
GameCube model with an HD model of the same name (a stage's rooms against HD's <stage>_<arc>.szs; an object archive
against HD's archive of the same name), each used GameCube position is placed by its joint's bind-pose matrix and
matched with HD's vertices placed by HD's bones; HD's normal is turned back into the joint's own frame.

  optional/hd-normals/<stage>.bin   a stage's rooms and Stage.arc (models matched by half their positions or more)
  optional/hd-normals/objects.bin   every object model drawn without normals of its own, matched by 95% or more
  optional/hd-colours/objects.bin   those objects' HD vertex colours
  optional/hd-colours/<stage>.bin   a stage's matched rooms' HD vertex colours (HD remade them; not the GameCube's)

HD's lava and fire rooms (GanonB, Cave01, MiniKaz, Ganon's Tower's lava parts) keep a mask in their materials'
vertex colours: (glow, 0, shade). Every room shader there lights by the blue alone, as a grey (GanonB's PS 4342 and
4345 in Cemu), and the materials with a konst2 colour animation (fmdl.glow_curves) add the lava's light, lava K x 4
x the red squared, times the texture, outside the shade. So a material whose vertices' green is all 0 gives
(b, b, b); one with the animation also its glow (red squared) and its curve.

Normals: b'BWHN', u32 version 1, u32 count, then count rows of little-endian float32 x, y, z, nx, ny, nz: the key
(x, y, z) is the position exactly as stored in the GameCube model (its raw joint-local float32s, bit for bit); a
position with several normals (HD's hard edges, or the same raw position in several models) has one row for each,
consecutive; rows sorted by the key's bytes. Colours: b'BWHC', u32 version 1, u32 count, then x, y, z, r, g, b, a
(0..1, HD's averaged over its vertices at the position); a position whose colour differs between models is left
out. A stage with lava glow writes version 2: u32 curve count, each curve's u32 frame count and its frames' linear
r, g, b (HD's konst2 at frames 0, 1, ...), then the rows, each x, y, z, r, g, b, a, glow, curve (-1 none; glow, the
vertex red squared, 0 there). Positions drawn through a skinning envelope are left out. Each joint's points take a voted offset (HD moved a
few pieces whole), sampling points evenly so the native importer (runtime/host/src/wwhd/normals.h) makes the same
bytes.
"""
from __future__ import annotations

from collections import Counter, defaultdict
import math
import re
import struct

from .formats import FormatError, leaves
from . import fmdl, j3d

NORMALS_FOLDER = "optional/hd-normals"
COLOURS_FOLDER = "optional/hd-colours"
OBJECTS_FILE = "objects.bin"
TOL = 0.1           # the match radius once a joint's offset is voted
TOL_FALLBACK = 1.0  # a stage model that matches nothing at TOL tries this
STAGE_SHARE = 0.5   # a stage model under this share of its positions matched was remade by HD: left out
OBJECT_SHARE = 0.95  # an object model needs this share
LARGE = 1.5         # an offset longer than this moves a whole piece and must be backed by more points
DECIMALS = 4

GC_STAGE_ARC = re.compile(r"res/Stage/([^/]+)/(Room\d+|Stage)\.arc", re.IGNORECASE)
GC_OBJECT_ARC = re.compile(r"res/Object/([^/]+)\.arc", re.IGNORECASE)
HD_CONTAINER = re.compile(r"(?:^|/)([^/]+)\.szs/")
HD_STAGE = re.compile(r"(.+)_(Room\d+|Stage)$")


def _arc_name(arc):
    return "Stage" if arc.lower() == "stage" else "Room" + str(int(arc[4:]))


def spaced(seq, n):
    """At most n items of seq, evenly spaced (the same in both importers)."""
    if len(seq) <= n:
        return list(seq)
    step = len(seq) / n
    return [seq[int(k * step)] for k in range(n)]


class Grid:
    """Points in cubes `cell` wide, for radius queries."""

    def __init__(self, points, cell):
        self.points, self.inv = points, 1.0 / cell
        self.cells = defaultdict(list)
        for i, (x, y, z) in enumerate(points):
            self.cells[(math.floor(x * self.inv), math.floor(y * self.inv), math.floor(z * self.inv))].append(i)

    def near(self, q, tol):
        inv, pts, cells = self.inv, self.points, self.cells
        qx, qy, qz = q
        t2 = tol * tol
        out = []
        for cx in range(math.floor((qx - tol) * inv), math.floor((qx + tol) * inv) + 1):
            for cy in range(math.floor((qy - tol) * inv), math.floor((qy + tol) * inv) + 1):
                for cz in range(math.floor((qz - tol) * inv), math.floor((qz + tol) * inv) + 1):
                    for i in cells.get((cx, cy, cz), ()):
                        p = pts[i]
                        dx, dy, dz = p[0] - qx, p[1] - qy, p[2] - qz
                        if dx * dx + dy * dy + dz * dz <= t2:
                            out.append(i)
        return sorted(out)

    def nearest(self, q, tol):
        """The nearest point within tol's offset from q (point - q), or None."""
        best, bd = None, tol * tol
        for i in self.near(q, tol):
            p = self.points[i]
            d = (p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2 + (p[2] - q[2]) ** 2
            if d <= bd:
                best, bd = i, d
        if best is None:
            return None
        p = self.points[best]
        return (p[0] - q[0], p[1] - q[1], p[2] - q[2])


def _quantize(v):
    """v to 1/10,000, halves to even (C++'s nearbyint does the same), and never -0."""
    return round(v * 10 ** DECIMALS) / 10 ** DECIMALS + 0.0


def _vote(deltas, step=0.1):
    """The mean of the most common bin of deltas (ties: the first bin to reach the count)."""
    bins = {}
    for d in deltas:
        bins.setdefault((round(d[0] / step), round(d[1] / step), round(d[2] / step)), []).append(d)
    best = max(bins.values(), key=len)
    n = len(best)
    return (sum(d[0] for d in best) / n, sum(d[1] for d in best) / n, sum(d[2] for d in best) / n)


def _hits(grid, pts, off, tol):
    return [k for k, q in enumerate(pts) if grid.near((q[0] + off[0], q[1] + off[1], q[2] + off[2]), tol)]


def _offsets(pts, fine, coarse, hd_pts, tol):
    """One joint's offsets (HD - GameCube) in the order tried, each with the points it rescues: round 0 looks for a
    small one (usually none); later rounds vote a large one from the points still missing (a piece HD moved)."""
    remaining = list(range(len(pts)))
    found = []
    for round_no in range(4):
        if len(remaining) < (1 if round_no == 0 else 4):
            break
        sub = [pts[k] for k in remaining]
        sample = spaced(sub, 400)
        cands = []
        deltas = [d for q in sample if (d := coarse.nearest(q, 1.0)) is not None]
        if deltas:
            cands.append(_vote(deltas))
        if round_no == 0:
            cands.append((0.0, 0.0, 0.0))
        best_small = max((len(_hits(fine, sub, c, tol)) for c in cands), default=0)
        if len(sub) - best_small >= 4:
            votes = Counter()
            pool = spaced(hd_pts, 6000)
            for gx, gy, gz in spaced(sub, 16):
                for h in pool:
                    votes[(round(h[0] - gx), round(h[1] - gy), round(h[2] - gz))] += 1
            for off, n in sorted(votes.items(), key=lambda kv: (-kv[1], kv[0]))[:3]:
                if n < 3:
                    break
                refine = [d for q in sample
                          if (d := coarse.nearest((q[0] + off[0], q[1] + off[1], q[2] + off[2]), 1.5)) is not None]
                if refine:
                    v = _vote(refine)
                    cands.append((v[0] + off[0], v[1] + off[1], v[2] + off[2]))
        best, best_hits = None, []
        for c in cands:
            hits = _hits(fine, sub, c, tol)
            if math.sqrt(c[0] ** 2 + c[1] ** 2 + c[2] ** 2) > LARGE:
                ok = len(hits) >= max(8, 0.05 * len(sub)) or len(hits) >= max(4, 0.5 * len(sub))
            else:
                ok = len(hits) >= (1 if round_no == 0 else max(4, 0.05 * len(sub)))
            if ok and len(hits) > len(best_hits):
                best, best_hits = c, hits
        if best is None:
            break
        rescued = [remaining[k] for k in best_hits]
        found.append((best, rescued))
        taken = set(rescued)
        remaining = [k for k in remaining if k not in taken]
    return found


def match(gm: j3d.Model, hd: fmdl.Placed, tol: float, moved=None):
    """(rows: key -> normals, matched position indices) for one GameCube model against one HD model. moved, a dict:
    (position index, joint) -> the offset that matched it, where it is longer than TOL (HD moved the piece: all of
    Hyroom's hall by 3.29, 2.33, 3.97)."""
    fine = Grid(hd.positions, 0.25 if tol <= 0.25 else 1.0)
    coarse = Grid(hd.positions, 1.0)
    groups = defaultdict(list)  # joint -> [(position index, model-space point)]
    for i, joints in gm.rigid().items():
        for j in joints:
            groups[j].append((i, j3d.apply(gm.world(j), gm.positions[i])))
    rows, matched = {}, set()
    for j in sorted(groups):
        items = groups[j]
        m = gm.world(j)
        pts = [q for _, q in items]
        for off, rescued in _offsets(pts, fine, coarse, hd.positions, tol):
            for k in rescued:
                i, q = items[k]
                hits = fine.near((q[0] + off[0], q[1] + off[1], q[2] + off[2]), tol)
                if not hits:
                    continue
                matched.add(i)
                if moved is not None and math.sqrt(off[0] ** 2 + off[1] ** 2 + off[2] ** 2) > TOL:
                    moved.setdefault((i, j), off)
                key = struct.pack("<3f", *gm.positions[i])
                for h in hits:
                    n = hd.normals[h]
                    if n is None:
                        continue
                    # model space -> joint-local: the transpose of the joint's 3x3
                    lx = m[0] * n[0] + m[4] * n[1] + m[8] * n[2]
                    ly = m[1] * n[0] + m[5] * n[1] + m[9] * n[2]
                    lz = m[2] * n[0] + m[6] * n[1] + m[10] * n[2]
                    ln = math.sqrt(lx * lx + ly * ly + lz * lz)
                    if ln < 1e-9:
                        continue
                    t = (_quantize(lx / ln), _quantize(ly / ln), _quantize(lz / ln))
                    lst = rows.setdefault(key, [])
                    if t not in lst:
                        lst.append(t)
    return rows, matched


def blue_shades(hd: fmdl.Placed):
    """The materials whose vertices all have colours with green 0: HD's lava rooms' (glow, 0, shade) masks."""
    seen, other = set(), set()
    for m, c in zip(hd.materials, hd.colours):
        if c is not None and c[1] == 0:
            seen.add(m)
        else:
            other.add(m)
    return seen - other


def room_colour(c, glow):
    """A stage vertex's colour as HD's room shaders take it: (b, b, b, a) for a mask, with its glow (red squared,
    a konst2 material's) or None."""
    if glow is None:
        return c[2], c[2], c[2], c[3], 0
    return c[2], c[2], c[2], c[3], c[0] * c[0]


def colours(gm: j3d.Model, hd: fmdl.Placed, curves=None, moved=None):
    """key -> HD's colour averaged over its vertices within TOL of the position (the offset that matched it where
    match()'s moved has one, else none). For a stage (curves:
    the model's konst2 animations, fmdl.glow_curves), (r, g, b, a, glow, curve): the masks as room_colour takes them,
    the glow averaged too, and the curve the hits' (None if none, or if they differ)."""
    grid = Grid(hd.positions, 0.25)
    blue = blue_shades(hd) if curves is not None else set()
    out = {}
    for i, joints in gm.rigid().items():
        q = j3d.apply(gm.world(joints[0]), gm.positions[i])
        if moved and (i, joints[0]) in moved:
            off = moved[(i, joints[0])]
            q = (q[0] + off[0], q[1] + off[1], q[2] + off[2])
        hits = [h for h in grid.near(q, TOL) if hd.colours[h] is not None]
        if not hits:
            continue
        key = struct.pack("<3f", *gm.positions[i])
        if curves is None:
            out[key] = tuple(sum(hd.colours[h][c] for h in hits) / len(hits) for c in range(4))
            continue
        cols, kinds = [], []
        for h in hits:
            m = hd.materials[h]
            if m in blue:
                curve = curves.get(m)
                cols.append(room_colour(hd.colours[h], curve))
                kinds.append(curve)
            else:
                cols.append(tuple(hd.colours[h]) + (0,))
                kinds.append(None)
        glowing = [k for k in kinds if k is not None]
        curve = glowing[0] if glowing and all(k == glowing[0] for k in glowing) else None
        avg = tuple(sum(c[k] for c in cols) / len(cols) for k in range(5))
        out[key] = avg[:4] + ((avg[4], curve) if curve is not None else (0, None))
    return out


def _merge(into, rows):
    for key, ns in rows.items():
        lst = into.setdefault(key, [])
        for n in ns:
            if n not in lst:
                lst.append(n)


def shade_colour(c):
    """A vertex colour HD uses as a colour: not a mask (no green under 0.02 beside red or blue over 0.1, as the lava
    rooms' (glow, 0, shade) before room_colour, which turns those to greys) and nothing below 0."""
    return min(c[:3]) >= 0.0 and not (c[1] < 0.02 and max(c[0], c[2]) > 0.1)


def _merge_colours(into, clashes, cols, stage=False):
    """A model's colours into a table: the first kept, a position whose colours differ by over 0.02 (or whose glow
    curves differ) left out; a stage's only those HD uses as colours (shade_colour)."""
    for key, c in cols.items():
        if stage and not shade_colour(c):
            continue
        old = into.get(key)
        if old is None:
            into[key] = c
        elif max(abs(old[k] - c[k]) for k in range(len(c) if stage else 4)
                 if not (stage and k == 5)) > 0.02 or (stage and old[5] != c[5]):
            clashes.add(key)


def normals_table(rows) -> bytes:
    keys = sorted(rows)
    out = bytearray(b"BWHN" + struct.pack("<II", 1, sum(len(rows[k]) for k in keys)))
    for k in keys:
        for n in rows[k]:
            out += k + struct.pack("<3f", *n)
    return bytes(out)


def colours_table(cols) -> bytes:
    """Version 1 (objects, and a stage without glow: rows of (r, g, b, a)), else version 2 with the curves."""
    keys = sorted(cols)
    curves = []
    for k in keys:
        c = cols[k]
        if len(c) > 4 and c[5] is not None and c[5] not in curves:
            curves.append(c[5])
    if not curves:
        out = bytearray(b"BWHC" + struct.pack("<II", 1, len(keys)))
        for k in keys:
            out += k + struct.pack("<4f", *cols[k][:4])
        return bytes(out)
    out = bytearray(b"BWHC" + struct.pack("<III", 2, len(keys), len(curves)))
    for curve in curves:
        out += struct.pack("<I", len(curve))
        for rgb in curve:
            out += struct.pack("<3f", *rgb)
    for k in keys:
        c = cols[k]
        out += k + struct.pack("<6f", *c[:5], float(curves.index(c[5])) if c[5] is not None else -1.0)
    return bytes(out)


class Builder:
    """Collects the tables while the importer reads HD's archives once: `feed` each BFRES with its path, `flush`
    after each HD archive, then `tables`. The GameCube's arcs are read from its disc as their HD files turn up."""

    def __init__(self, gc_disc, entries):
        self.disc = gc_disc  # an open file
        self.stage_arcs, self.object_arcs = {}, {}
        for e in entries:
            if e["is_dir"]:
                continue
            if m := GC_STAGE_ARC.fullmatch(e["path"]):
                self.stage_arcs[f"{m.group(1)}_{_arc_name(m.group(2))}"] = (m.group(1), e)
            elif m := GC_OBJECT_ARC.fullmatch(e["path"]):
                self.object_arcs.setdefault(m.group(1).lower(), e)
        self.done = set()
        self.container, self.models, self.curves, self._curves = None, {}, {}, {}
        self.stages = defaultdict(dict)   # stage -> key -> normals
        self.stage_colours = defaultdict(dict)  # stage -> key -> colour
        self.stage_clashes = defaultdict(set)
        self._model_colours = None
        self.objects, self.object_colours, self.clashes = {}, {}, set()
        self.report = {"stage_models": 0, "stage_models_skipped": 0, "object_models": 0,
                       "object_models_skipped": 0, "errors": []}

    def wanted(self, path):
        m = None
        for m in HD_CONTAINER.finditer(path):
            pass
        if m is None:
            return None
        name = m.group(1)
        if name in self.done or (name not in self.stage_arcs and name.lower() not in self.object_arcs):
            return None
        return name

    def feed(self, path, data):
        name = self.wanted(path)
        if name is None or data[:4] != b"FRES":
            return
        if name != self.container:
            self.flush()
            self.container = name
        try:
            models = fmdl.read_models(data)
            curves = fmdl.glow_curves(data) if any(m not in self.models for m in models) else {}
            for model, placed in models.items():
                if model not in self.models:
                    self.models[model] = placed
                    self.curves[model] = curves
        except FormatError as ex:
            self.report["errors"].append({"source": path, "reason": str(ex)})

    def _gc_models(self, entry):
        self.disc.seek(entry["offset"])
        for path, leaf in leaves(self.disc.read(entry["size"]), entry["path"]):
            if path.lower().endswith((".bdl", ".bmd")):
                yield path, path.rsplit("/", 1)[-1].rsplit(".", 1)[0], leaf

    def flush(self):
        name, models, self._curves = self.container, self.models, self.curves
        self.container, self.models, self.curves = None, {}, {}
        if name is None:
            return
        self.done.add(name)
        if name in self.stage_arcs:
            stage, entry = self.stage_arcs[name]
            for path, model, leaf in self._gc_models(entry):
                rows = self._stage_model(path, model, leaf, models)
                if rows is None:
                    self.report["stage_models_skipped"] += 1
                else:
                    self.report["stage_models"] += 1
                    _merge(self.stages[stage], rows)
                    _merge_colours(self.stage_colours[stage], self.stage_clashes[stage], self._model_colours, stage=True)
        else:
            for path, model, leaf in self._gc_models(self.object_arcs[name.lower()]):
                if not self._object_model(path, model, leaf, models):
                    self.report["object_models_skipped"] += 1

    def _read(self, path, leaf):
        try:
            gm = j3d.read_model(leaf)
        except FormatError as ex:
            self.report["errors"].append({"source": path, "reason": str(ex)})
            return None
        return gm if gm.positions else None

    def _stage_model(self, path, model, leaf, hd_models):
        hd = hd_models.get(model)
        if model.startswith("vr_") or hd is None or not hd.positions or all(n is None for n in hd.normals):
            return None
        gm = self._read(path, leaf)
        if gm is None or not (positions := len(gm.rigid())):
            return None
        moved = {}
        rows, matched = match(gm, hd, TOL, moved)
        if not matched:
            moved = {}
            rows, matched = match(gm, hd, TOL_FALLBACK, moved)
        if len(matched) < STAGE_SHARE * positions:
            return None
        self._model_colours = colours(gm, hd, self._curves.get(model, {}), moved)
        return rows

    def _object_model(self, path, model, leaf, hd_models):
        hd = next((p for n, p in hd_models.items() if n.lower() == model.lower()), None)
        if hd is None or not hd.positions:
            return False
        gm = self._read(path, leaf)
        if gm is None or j3d.GX_NORMAL in gm.attributes or not (positions := len(gm.rigid())):
            return False
        rows, matched = match(gm, hd, TOL)
        if len(matched) < OBJECT_SHARE * positions:
            return False
        _merge(self.objects, rows)
        _merge_colours(self.object_colours, self.clashes, colours(gm, hd))
        self.report["object_models"] += 1
        return True

    def tables(self):
        """{relative path: bytes} to write, and the report's entry."""
        self.flush()
        out = {f"{NORMALS_FOLDER}/{stage}.bin": normals_table(rows) for stage, rows in sorted(self.stages.items())
               if rows}
        for stage, table in sorted(self.stage_colours.items()):
            kept = {k: v for k, v in table.items() if k not in self.stage_clashes[stage]}
            if kept and self.stages.get(stage):
                out[f"{COLOURS_FOLDER}/{stage}.bin"] = colours_table(kept)
        cols = {k: v for k, v in self.object_colours.items() if k not in self.clashes}
        if self.objects:
            out[f"{NORMALS_FOLDER}/{OBJECTS_FILE}"] = normals_table(self.objects)
        if cols:
            out[f"{COLOURS_FOLDER}/{OBJECTS_FILE}"] = colours_table(cols)
        report = dict(self.report, stages=sum(1 for rows in self.stages.values() if rows),
                      object_positions=len(self.objects), object_colours=len(cols),
                      object_colours_left_out=len(self.clashes))
        return out, report
