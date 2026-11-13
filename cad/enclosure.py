# /// script
# requires-python = ">=3.10,<3.13"
# dependencies = [
#   "cadquery-ocp==7.7.2",
#   "numpy<2",
#   "matplotlib",
# ]
# ///
"""Generate Mural enclosure parts that fit the Makerbase MKS DLC32 MAX controller.

Reads the printed parts straight from the upstream STEP (exact B-rep), then:
  * STRETCHES the bottom and the cover along the wall-vertical axis (+z) by STRETCH_DZ at a
    featureless plane STRETCH_Z0. Everything above the plane (top corner posts, PD trigger clamp,
    top wall) moves up; the gap is filled by extruding the cut cross-section. All upstream
    features are preserved.
  * Removes the original controller's 4 standoffs and trims the old left port-panel block.
  * Adds 4 heat-set-insert bosses (same dimensions as upstream's) on the DLC32 MAX 92 x 62 mm
    hole pattern, and cuts a right-wall opening for the DC barrel plug + USB.
  * Builds an envelope model of the DLC32 MAX (PCB, stepstick drivers + heatsinks, connectors,
    cable keep-outs) and checks it for interference against the new parts and every other
    component of the upstream assembly (servo, OLED, screen holder, PD board, ...).
  * Exports STLs + preview renders + a text report into cad/out/.

Run:  uv run cad/enclosure.py            (first run parses the 40 MB STEP, ~2 min; then cached)

Coordinates (upstream STEP frame): x = across the bot (pen centre at x=45), y = height away from the
wall (bottom plate top at y=-3), z = vertical on the wall (+z = up, towards the belt pins).

Board placement: the DLC32 MAX lies flat, 100 mm side along z, 70 mm side along x.
Board coords: u along the 100 mm edge, v along the 70 mm edge, as in the Makerbase dimension
drawing (u=0 at the laser/spindle terminal edge, v=0 at the motor-connector edge,
v=70 at the DC/USB/SD/RJ11 edge). Mapping: z = BOARD_Z0 + u, x = BOARD_X0 + v.
-> motor connectors face the left wall, DC/USB face the right wall.
-> drivers go in the Y and Z sockets (X socket sits under the OLED wiring, A under nothing useful
   and uses the ESP32-S3 native-USB pins).
"""

from __future__ import annotations

import os
import sys
from dataclasses import dataclass

import numpy as np
from OCP.BRep import BRep_Builder
from OCP.BRepAdaptor import BRepAdaptor_Surface
from OCP.BRepAlgoAPI import BRepAlgoAPI_Common, BRepAlgoAPI_Cut, BRepAlgoAPI_Fuse
from OCP.BRepBndLib import BRepBndLib
from OCP.BRepBuilderAPI import BRepBuilderAPI_Transform
from OCP.BRepExtrema import BRepExtrema_DistShapeShape
from OCP.BRepGProp import BRepGProp
from OCP.BRepMesh import BRepMesh_IncrementalMesh
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox, BRepPrimAPI_MakeCylinder, BRepPrimAPI_MakePrism
from OCP.BRepTools import BRepTools
from OCP.Bnd import Bnd_Box
from OCP.GeomAbs import GeomAbs_Plane
from OCP.GProp import GProp_GProps
from OCP.ShapeUpgrade import ShapeUpgrade_UnifySameDomain
from OCP.StlAPI import StlAPI_Writer
from OCP.STEPControl import STEPControl_Reader
from OCP.TopAbs import TopAbs_FACE, TopAbs_SOLID
from OCP.TopExp import TopExp_Explorer
from OCP.TopLoc import TopLoc_Location
from OCP.TopoDS import TopoDS, TopoDS_Compound, TopoDS_Shape
from OCP.TopTools import TopTools_ListOfShape
from OCP.BRep import BRep_Tool
from OCP.gp import gp_Ax2, gp_Dir, gp_Pnt, gp_Trsf, gp_Vec

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
STEP_PATH = os.path.join(REPO, "STLs", "Mural v2.step")
CACHE = os.path.join(HERE, ".cache")
OUT = os.path.join(HERE, "out")

# ----------------------------------------------------------------------------------------------
# Parameters (all mm). Change here, re-run.
# ----------------------------------------------------------------------------------------------
STRETCH_Z0 = -25.0     # cut plane: prismatic region of both parts (between PD clamp and port panel)
STRETCH_DZ = 30.0      # how much longer the bay gets; top corner posts clear the board corners

BOARD_L, BOARD_W, BOARD_T = 100.0, 70.0, 1.6   # MKS DLC32 MAX (manual: 100 x 70, holes 92 x 62)
HOLE_INSET = 4.0
BOARD_X0 = 10.0        # v=0 edge; centred on the pen axis x=45, between the corner posts
BOARD_Z0 = -67.5       # u=0 edge; 1.8 mm above the servo body (top at z=-69.3)
PLATE_TOP_Y = -3.0     # bottom plate top surface
BOSS_TOP_Y = 0.0       # upstream boss height (3 mm); PCB underside sits here
BOSS_D, INSERT_D, INSERT_BOTTOM_Y = 7.0, 4.2, -5.4   # identical to upstream bosses (M3 heat-set)

OLD_STANDOFFS = [(13.0, -65.5), (13.0, -1.5), (67.0, -65.5), (67.0, -1.5)]  # upstream controller

RIGHT_WALL_X = (88.5, 93.0)       # cut range through the right wall (inner face x=89, outer 92)
POWER_USB_OPENING_U = (13.0, 43.0)  # board-u span of DC jack (u15-29.6) + USB (u29.4-41.5) + margin
POWER_USB_OPENING_TOP = 17.0        # y of opening top (connectors ~11 mm above PCB top at y=1.6)

CLEARANCE_MIN = 0.5    # required gap between envelope and any part
FOOTPRINT_MARGIN = 0.8  # cleared space around the PCB outline

# Envelope model of the board, in board coords: (u0, u1, v0, v1, h0, h1) with h measured from the
# PCB top surface. Positions read off the Makerbase "Installation Dimensions" drawing (+-1 mm).
DRIVER_SOCKETS_U = {"X": 17.0, "Y": 32.8, "Z": 49.4, "A": 63.6}   # socket centres along u
USED_SOCKETS = ("Y", "Z")
STEPSTICK = dict(w=15.24, v0=8.0, v1=28.3, socket_h=8.5, pcb_t=1.6, heatsink=9.0, heatsink_h=10.0)

ENVELOPE_BLOCKS = {
    "low components (blanket)": (1, 99, 1, 69, 0, 5.5),
    "electrolytic caps": (8, 46, 33, 52, 0, 11.0),
    "DC jack J1": (15.0, 29.6, 55, 72.0, 0, 11.0),
    "USB J8": (29.4, 41.5, 56, 72.5, 0, 11.0),
    "SD slot": (54.6, 69.4, 57, 71.0, 0, 3.0),
    "RJ11 screen J19": (79.9, 90.1, 54, 73.0, 0, 16.0),
    "J17 (left edge, top)": (0, 10, 4, 10, 0, 10.0),
    "J4 laser 1 XH2.54-3P": (2.0, 7.5, 10.5, 28.0, 0, 8.0),
    "J5 laser 2 MR30": (1.0, 11.0, 29.0, 42.0, 0, 10.0),
    "J7 spindle screw terminal": (0, 12.0, 42.0, 55.0, 0, 12.0),
    "J3 switch": (0, 9.0, 54.0, 62.0, 0, 8.0),
    "motor/air/beeper XH headers": (8, 95, 0, 7.5, 0, 8.0),
    "endstop/door/probe headers": (78, 100, 8, 42, 0, 8.0),
    "ESP32-S3 module": (72, 100, 36, 56, 0, 4.0),
    # cable keep-outs (plugs + bend room)
    "Y1 motor cable": (24, 36, 0, 7.5, 8.0, 17.0),
    "Z motor cable": (43, 55, 0, 7.5, 8.0, 17.0),
    "servo signal plug (laser 1, J4)": (2.0, 7.5, 10.5, 28.0, 8.0, 18.0),
    "servo 5V plug (endstop header)": (88, 98, 8, 20, 8.0, 17.0),
    "PD power plug into DC jack": (15.0, 29.6, 70, 80.0, 0, 12.0),
}

# Upstream assembly components to check against, keyed by bbox (x0, y0, z0) +- 0.3 mm.
# Anything inside the original controller's volume is dropped (replaced by the DLC32 MAX).
ORIGINAL_CONTROLLER_BOX = (8.0, -2.0, -75.0, 81.0, 12.5, 2.0)


# ----------------------------------------------------------------------------------------------
# OCP helpers
# ----------------------------------------------------------------------------------------------
def box(x0, y0, z0, x1, y1, z1) -> TopoDS_Shape:
    return BRepPrimAPI_MakeBox(gp_Pnt(x0, y0, z0), gp_Pnt(x1, y1, z1)).Shape()


def ycyl(x, z, r, y0, y1) -> TopoDS_Shape:
    return BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(x, y0, z), gp_Dir(0, 1, 0)), r, y1 - y0).Shape()


def translate(s, dx=0.0, dy=0.0, dz=0.0) -> TopoDS_Shape:
    t = gp_Trsf()
    t.SetTranslation(gp_Vec(dx, dy, dz))
    return BRepBuilderAPI_Transform(s, t, True).Shape()


def _bool(op, a, b):
    o = op(a, b)
    o.SetFuzzyValue(1e-5)
    o.Build()
    if not o.IsDone():
        raise RuntimeError(f"{op.__name__} failed")
    return o.Shape()


def common(a, b):
    return _bool(BRepAlgoAPI_Common, a, b)


def cut(a, b):
    return _bool(BRepAlgoAPI_Cut, a, b)


def fuse_many(shapes):
    args, tools = TopTools_ListOfShape(), TopTools_ListOfShape()
    args.Append(shapes[0])
    for s in shapes[1:]:
        tools.Append(s)
    f = BRepAlgoAPI_Fuse()
    f.SetArguments(args)
    f.SetTools(tools)
    f.SetFuzzyValue(1e-5)
    f.Build()
    if not f.IsDone():
        raise RuntimeError("fuse failed")
    return unify(f.Shape())


def unify(s):
    u = ShapeUpgrade_UnifySameDomain(s, True, True, False)
    u.Build()
    return u.Shape()


def compound(shapes):
    c = TopoDS_Compound()
    b = BRep_Builder()
    b.MakeCompound(c)
    for s in shapes:
        b.Add(c, s)
    return c


def bbox(s):
    b = Bnd_Box()
    BRepBndLib.Add_s(s, b)
    return b.Get()


def volume(s):
    p = GProp_GProps()
    BRepGProp.VolumeProperties_s(s, p)
    return p.Mass()


def solids(s):
    ex = TopExp_Explorer(s, TopAbs_SOLID)
    while ex.More():
        yield ex.Current()
        ex.Next()


def write_stl(s, path, deflection=0.05):
    BRepMesh_IncrementalMesh(s, deflection, False, 0.3, True)
    w = StlAPI_Writer()
    w.ASCIIMode = False
    if not w.Write(s, path):
        raise RuntimeError(f"STL write failed: {path}")


def mesh_triangles(s, deflection=0.2):
    """Return (n,3,3) triangle array of a shape."""
    BRepMesh_IncrementalMesh(s, deflection, False, 0.5, True)
    tris = []
    ex = TopExp_Explorer(s, TopAbs_FACE)
    while ex.More():
        face = TopoDS.Face_s(ex.Current())
        loc = TopLoc_Location()
        tri = BRep_Tool.Triangulation_s(face, loc)
        if tri is not None:
            trsf = loc.Transformation()
            nodes = [tri.Node(i).Transformed(trsf) for i in range(1, tri.NbNodes() + 1)]
            pts = np.array([[p.X(), p.Y(), p.Z()] for p in nodes])
            for i in range(1, tri.NbTriangles() + 1):
                a, b, c = tri.Triangle(i).Get()
                tris.append(pts[[a - 1, b - 1, c - 1]])
        ex.Next()
    return np.array(tris) if tris else np.zeros((0, 3, 3))


# ----------------------------------------------------------------------------------------------
# Source geometry
# ----------------------------------------------------------------------------------------------
def _matches(bb, size, tol=0.5):
    x0, y0, z0, x1, y1, z1 = bb
    return all(abs(a - b) < tol for a, b in zip((x1 - x0, y1 - y0, z1 - z0), size))


def load_sources():
    os.makedirs(CACHE, exist_ok=True)
    paths = {k: os.path.join(CACHE, f"{k}.brep") for k in ("bottom", "cover", "others")}
    if not all(os.path.exists(p) for p in paths.values()):
        print("Parsing upstream STEP (one-off, ~2 min)...")
        r = STEPControl_Reader()
        if r.ReadFile(STEP_PATH) != 1:
            raise SystemExit(f"cannot read {STEP_PATH}")
        r.TransferRoots()
        bottom = cover = None
        others = []
        for s in solids(r.OneShape()):
            bb = bbox(s)
            if bottom is None and _matches(bb, (170.0, 36.3, 145.7)):
                bottom = s
            elif cover is None and _matches(bb, (103.8, 33.3, 112.2)):
                cover = s
            else:
                others.append(s)
        if bottom is None or cover is None:
            raise SystemExit("could not identify bottom/cover solids in STEP")
        BRepTools.Write_s(bottom, paths["bottom"])
        BRepTools.Write_s(cover, paths["cover"])
        BRepTools.Write_s(compound(others), paths["others"])
    out = {}
    for k, p in paths.items():
        s = TopoDS_Shape()
        BRepTools.Read_s(s, p, BRep_Builder())
        out[k] = s
    return out["bottom"], out["cover"], list(solids(out["others"]))


# ----------------------------------------------------------------------------------------------
# Operations
# ----------------------------------------------------------------------------------------------
BIG = 1000.0


def stretch_z(shape, z0, dz):
    """Split at plane z=z0, move the upper part by dz, fill the gap with the extruded section."""
    lower = common(shape, box(-BIG, -BIG, -BIG, BIG, BIG, z0))
    upper = common(shape, box(-BIG, -BIG, z0, BIG, BIG, BIG))
    prisms = []
    ex = TopExp_Explorer(lower, TopAbs_FACE)
    while ex.More():
        f = TopoDS.Face_s(ex.Current())
        a = BRepAdaptor_Surface(f)
        if a.GetType() == GeomAbs_Plane:
            n = a.Plane().Axis().Direction()
            fb = bbox(f)
            if abs(abs(n.Z()) - 1) < 1e-6 and abs(fb[2] - z0) < 1e-4 and abs(fb[5] - z0) < 1e-4:
                prisms.append(BRepPrimAPI_MakePrism(f, gp_Vec(0, 0, dz)).Shape())
        ex.Next()
    if not prisms:
        raise RuntimeError("stretch: no section faces found at z0")
    return fuse_many([lower, translate(upper, dz=dz)] + prisms)


def board_to_xyz(u, v):
    return BOARD_X0 + v, BOARD_Z0 + u


def board_holes():
    return [board_to_xyz(u, v) for u in (HOLE_INSET, BOARD_L - HOLE_INSET)
            for v in (HOLE_INSET, BOARD_W - HOLE_INSET)]


PCB_TOP_Y = BOSS_TOP_Y + BOARD_T


def block_uvh(u0, u1, v0, v1, h0, h1):
    x0, z0 = board_to_xyz(u0, v0)
    x1, z1 = board_to_xyz(u1, v1)
    return box(min(x0, x1), PCB_TOP_Y + h0, min(z0, z1), max(x0, x1), PCB_TOP_Y + h1, max(z0, z1))


def make_envelope():
    parts = {}
    pcb = block_uvh(0, BOARD_L, 0, BOARD_W, -BOARD_T, 0)
    under = box(BOARD_X0, PLATE_TOP_Y + 0.5, BOARD_Z0, BOARD_X0 + BOARD_W, BOSS_TOP_Y, BOARD_Z0 + BOARD_L)
    for x, z in board_holes():   # bosses live here
        # PCBs keep a component-free ring around mounting holes (~8 mm dia); boss is 7 mm
        hole = ycyl(x, z, BOSS_D / 2 + CLEARANCE_MIN + 0.1, -BIG, BIG)
        pcb = cut(pcb, ycyl(x, z, 1.6, -BIG, BIG))
        under = cut(under, hole)
    parts["PCB"] = pcb
    parts["through-hole pins under PCB"] = under
    for name, (u0, u1, v0, v1, h0, h1) in ENVELOPE_BLOCKS.items():
        parts[name] = block_uvh(u0, u1, v0, v1, h0, h1)
    st = STEPSTICK
    for sock, uc in DRIVER_SOCKETS_U.items():
        top = st["socket_h"] + st["pcb_t"] if sock in USED_SOCKETS else st["socket_h"]
        parts[f"{sock} socket{' + TMC2209' if sock in USED_SOCKETS else ' (empty)'}"] = block_uvh(
            uc - st["w"] / 2, uc + st["w"] / 2, st["v0"], st["v1"], 0, top)
        if sock in USED_SOCKETS:
            vc = (st["v0"] + st["v1"]) / 2
            parts[f"{sock} heatsink"] = block_uvh(uc - st["heatsink"] / 2, uc + st["heatsink"] / 2,
                                                  vc - st["heatsink"] / 2, vc + st["heatsink"] / 2,
                                                  top, top + st["heatsink_h"])
    return parts


def modify_bottom(bottom):
    b = stretch_z(bottom, STRETCH_Z0, STRETCH_DZ)
    # 1) clear everything above the plate within the new board footprint (+margin): the original
    #    controller's standoffs and edge supports, and the inner end of the old left port block
    m = FOOTPRINT_MARGIN
    b = cut(b, box(BOARD_X0 - m, PLATE_TOP_Y, BOARD_Z0 - m, BOARD_X0 + BOARD_W + m, 25,
                   BOARD_Z0 + BOARD_L + m))
    # 2) fill the old standoffs' insert holes back up to the plate surface
    for x, z in OLD_STANDOFFS:
        zz = z + (STRETCH_DZ if z > STRETCH_Z0 else 0)
        b = fuse_many([b, ycyl(x, zz, INSERT_D / 2 + 0.3, INSERT_BOTTOM_Y - 0.5, PLATE_TOP_Y)])
    # 3) new bosses with heat-set insert holes (upstream dimensions)
    for x, z in board_holes():
        b = fuse_many([b, ycyl(x, z, BOSS_D / 2, PLATE_TOP_Y - 0.5, BOSS_TOP_Y)])
        b = cut(b, ycyl(x, z, INSERT_D / 2, INSERT_BOTTOM_Y, BOSS_TOP_Y + 1))
    return b


def modify_cover(cover):
    c = stretch_z(cover, STRETCH_Z0, STRETCH_DZ)
    # trim the inner face of the ledge above the old left port panel (beside the motor connectors)
    m = FOOTPRINT_MARGIN
    c = cut(c, box(BOARD_X0 - m, PLATE_TOP_Y, -64.5, BOARD_X0 + 2, 25, -29.5))
    # right-wall notch for the DC barrel plug + USB (open at the bottom edge: prints cleanly and
    # lets the cover drop over plugged cables)
    _, z0 = board_to_xyz(POWER_USB_OPENING_U[0], 0)
    _, z1 = board_to_xyz(POWER_USB_OPENING_U[1], 0)
    c = cut(c, box(RIGHT_WALL_X[0], PLATE_TOP_Y - 1, z0, RIGHT_WALL_X[1], POWER_USB_OPENING_TOP, z1))
    return c


def place_others(others):
    """Upstream components to clear, moved like the part they're mounted on."""
    keep = []
    X0, Y0, Z0, X1, Y1, Z1 = ORIGINAL_CONTROLLER_BOX
    for s in others:
        x0, y0, z0, x1, y1, z1 = bbox(s)
        if x0 >= X0 and y0 >= Y0 and z0 >= Z0 and x1 <= X1 and y1 <= Y1 and z1 <= Z1:
            continue  # original controller board: replaced
        if x1 < -5 or x0 > 95 or y0 > 32 or z1 < -80 or z0 > 13:
            continue  # not near the board bay
        if z0 > STRETCH_Z0:
            s = translate(s, dz=STRETCH_DZ)
        keep.append(s)
    # OLED wiring: soldered or right-angle leads off the OLED's downward pin header
    # (header at x27.4-29.9, z-56.9..-46.8, bottom y15.8). Straight dupont housings don't fit.
    keep.append(box(27.0, 12.0, -57.5, 30.5, 16.0, -46.3))
    return keep


# ----------------------------------------------------------------------------------------------
# Checks + renders
# ----------------------------------------------------------------------------------------------
@dataclass
class Clash:
    env: str
    other: str
    overlap_mm3: float
    gap_mm: float
    where: str


# Intentional contacts (e.g. the PCB resting on its bosses): (envelope name, target name).
ALLOWED_CONTACT = {("PCB", "bottom (new)")}


def check(envelope, targets):
    clashes = []
    for ename, eshape in envelope.items():
        eb = bbox(eshape)
        for tname, tshape in targets.items():
            tb = bbox(tshape)
            if any(eb[i] > tb[i + 3] + CLEARANCE_MIN or tb[i] > eb[i + 3] + CLEARANCE_MIN for i in range(3)):
                continue
            inter = common(eshape, tshape)
            ov = volume(inter)
            d = BRepExtrema_DistShapeShape(eshape, tshape)
            gap = d.Value() if d.IsDone() else float("nan")
            if (ename, tname) in ALLOWED_CONTACT and ov < 1e-3:
                continue
            if ov > 1e-3 or gap < CLEARANCE_MIN:
                if ov > 1e-3:
                    ib = bbox(inter)
                    where = "overlap x[%.1f,%.1f] y[%.1f,%.1f] z[%.1f,%.1f]" % (
                        ib[0], ib[3], ib[1], ib[4], ib[2], ib[5])
                else:
                    p1, p2 = d.PointOnShape1(1), d.PointOnShape2(1)
                    where = "closest env(%.1f,%.1f,%.1f) part(%.1f,%.1f,%.1f)" % (
                        p1.X(), p1.Y(), p1.Z(), p2.X(), p2.Y(), p2.Z())
                clashes.append(Clash(ename, tname, ov, gap, where))
    return clashes


def render(shapes_colors, path, view, title):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.collections import PolyCollection

    views = {
        "top": (np.array([1, 0, 0]), np.array([0, 0, 1]), np.array([0, 1, 0])),     # look down -y
        "iso": None,
        "right": (np.array([0, 0, 1]), np.array([0, 1, 0]), np.array([1, 0, 0])),   # from +x
    }
    if view == "iso":
        d = np.array([0.55, 0.65, -0.52]); d /= np.linalg.norm(d)
        up0 = np.array([0, 0, 1.0])
        r = np.cross(up0, d); r /= np.linalg.norm(r); up = np.cross(d, r)
        ax_r, ax_u, ax_d = r, up, d
    else:
        ax_r, ax_u, ax_d = views[view]
    light = ax_d * 0.6 + ax_u * 0.5 + ax_r * 0.3
    light /= np.linalg.norm(light)
    polys, cols, depth = [], [], []
    for shape, rgb, alpha in shapes_colors:
        t = mesh_triangles(shape)
        if not len(t):
            continue
        n = np.cross(t[:, 1] - t[:, 0], t[:, 2] - t[:, 0])
        n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-12
        shade = 0.35 + 0.65 * np.abs(n @ light)
        polys.append(np.stack([t @ ax_r, t @ ax_u], axis=-1))
        c = np.clip(np.outer(shade, rgb), 0, 1)
        cols.append(np.c_[c, np.full(len(c), alpha)])
        depth.append((t @ ax_d).mean(1))
    P, C, D = np.concatenate(polys), np.concatenate(cols), np.concatenate(depth)
    o = np.argsort(D)
    fig, ax = plt.subplots(figsize=(11, 10), dpi=100)
    ax.add_collection(PolyCollection(P[o], facecolors=C[o], edgecolors="none"))
    ax.autoscale()
    ax.set_aspect("equal")
    ax.set_title(title)
    ax.axis("off")
    fig.savefig(path, bbox_inches="tight")
    plt.close(fig)


# ----------------------------------------------------------------------------------------------
def main():
    os.makedirs(OUT, exist_ok=True)
    bottom, cover, others = load_sources()
    print("Stretching + modifying bottom...")
    new_bottom = modify_bottom(bottom)
    print("Stretching + modifying cover...")
    new_cover = modify_cover(cover)
    print("Placing upstream components + building board envelope...")
    comps = place_others(others)
    env = make_envelope()

    targets = {"bottom (new)": new_bottom, "cover (new)": new_cover}
    for i, s in enumerate(comps):
        x0, y0, z0, x1, y1, z1 = bbox(s)
        targets[f"component#{i} x[{x0:.0f},{x1:.0f}] y[{y0:.0f},{y1:.0f}] z[{z0:.0f},{z1:.0f}]"] = s
    print(f"Checking {len(env)} envelope blocks against {len(targets)} parts...")
    clashes = check(env, targets)

    write_stl(new_bottom, os.path.join(OUT, "bottom_dlc32max.stl"))
    write_stl(new_cover, os.path.join(OUT, "cover_dlc32max.stl"))
    env_all = compound(list(env.values()))
    write_stl(env_all, os.path.join(OUT, "dlc32max_envelope.stl"), 0.2)

    grey, blue, green, orange = (0.85, 0.85, 0.85), (0.45, 0.6, 0.95), (0.3, 0.75, 0.35), (0.95, 0.6, 0.25)
    comp_all = compound(comps)
    render([(new_bottom, grey, 1), (env_all, green, 1), (comp_all, orange, 1)],
           os.path.join(OUT, "preview_top_no_cover.png"), "top", "Bottom + DLC32 MAX envelope (green) + components (orange), top view")
    render([(new_bottom, grey, 1), (env_all, green, 1), (comp_all, orange, 1)],
           os.path.join(OUT, "preview_iso_no_cover.png"), "iso", "Bottom + DLC32 MAX envelope, iso")
    render([(new_bottom, grey, 1), (new_cover, blue, 0.55), (env_all, green, 1)],
           os.path.join(OUT, "preview_iso_with_cover.png"), "iso", "With cover (translucent)")
    render([(new_cover, blue, 1), (new_bottom, grey, 1)],
           os.path.join(OUT, "preview_right_side.png"), "right", "Right side: DC/USB opening")

    eb = bbox(new_bottom); cb = bbox(new_cover)
    lines = [
        "MKS DLC32 MAX enclosure report",
        f"stretch: +{STRETCH_DZ} mm at z={STRETCH_Z0}",
        f"bottom bbox size: {eb[3]-eb[0]:.1f} x {eb[4]-eb[1]:.1f} x {eb[5]-eb[2]:.1f} mm, volume {volume(new_bottom)/1000:.1f} cm3 (upstream {volume(bottom)/1000:.1f})",
        f"cover  bbox size: {cb[3]-cb[0]:.1f} x {cb[4]-cb[1]:.1f} x {cb[5]-cb[2]:.1f} mm, volume {volume(new_cover)/1000:.1f} cm3 (upstream {volume(cover)/1000:.1f})",
        f"board holes (x,z): {[(round(x,1), round(z,1)) for x, z in board_holes()]}",
        f"drivers in sockets: {', '.join(USED_SOCKETS)}",
        f"clearance required: {CLEARANCE_MIN} mm",
        "",
    ]
    if clashes:
        lines.append(f"FAIL: {len(clashes)} clash(es)")
        for c in clashes:
            lines.append(f"  {c.env:38s} vs {c.other:55s} overlap={c.overlap_mm3:8.2f} mm3 "
                         f"gap={c.gap_mm:.2f} mm  {c.where}")
    else:
        lines.append("PASS: no interference, all clearances >= %.1f mm" % CLEARANCE_MIN)
    report = "\n".join(lines)
    with open(os.path.join(OUT, "report.txt"), "w", encoding="utf-8") as f:
        f.write(report + "\n")
    print(report)
    return 1 if clashes else 0


if __name__ == "__main__":
    sys.exit(main())
