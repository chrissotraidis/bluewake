"""compare_systems.py A.json CYCLES_A B.json CYCLES_B: game-thread cycles per frame by game system."""
import json, re, sys
RULES = [
    ("Background collision", r"(__4cBgW|__4dBgW|__4dBgS|__4cBgS|cBgS_|dBgS_|cBgW|dBgW|MakeBlckMinMax|cM3d_Cross|RwgGround|RwgWall|RwgRoof)"),
    ("Actor collision", r"(cCcD|dCcS|dCcMass|cCcS)"),
    ("J3D animation and matrices", r"(J3DAnm|J3DMtxCalc|J3DGetKeyFrame|calcWeightEnvelope|recursiveCalc|J3DJoint|J3DTransform|J3DModel|calcAnmMtx|J3DMtxBuffer)"),
    ("Drawing (J3D/J2D shapes, packets, GX setup)", r"(J3DShape|J3DMaterial|J3DDraw|J3DPacket|J3DGD|J3DTev|J3DSys|J2D|J3DUClipper|J3DModelData|load__|draw__|Draw|draw|GDSet|GXSet|GXLoad|GXBegin|GXCall|_GX|JPADraw|JPA)"),
    ("Sea, sky, weather", r"(wave|vrkumo|Vrkumo|dKy|kankyo|dScnKy|sea|Sea)"),
    ("Sound", r"(JASystem|JAI|JAS|Jaudio|mDoAud)"),
    ("Actors' own update", r"(fopAc|daNpc|daPy|dAct|fpc|daObj|execute|Execute)"),
    ("Maths", r"(cM_|cXyz|cSPolar|cSGlobe|PSMTX|PSVEC|C_MTX|C_VEC|__kernel_|__ieee754|sqrt|atan|sin|cos)"),
]
def group(d):
    g = {k: 0 for k, _ in RULES}; g["Other game code"] = 0
    for name, c in d["functions"].items():
        for k, rx in RULES:
            if re.search(rx, name): g[k] += c; break
        else: g["Other game code"] += c
    g["Native helpers (math, vec, skin)"] = sum(c for n, c in d["module_other"].items() if n.startswith("bluewake_native"))
    g["Other module code (dispatch, memory, helpers)"] = sum(c for n, c in d["module_other"].items() if not n.startswith("bluewake_native"))
    g["Host (GX writes, runtime)"] = sum(d["host"].values())
    return g
a, ca, b, cb = json.load(open(sys.argv[1])), float(sys.argv[2]), json.load(open(sys.argv[3])), float(sys.argv[4])
ga, gb = group(a), group(b)
print(f"{'System':48} {'light Mcyc':>10} {'busiest Mcyc':>12} {'change':>8}")
for k in ga:
    x = ga[k] / a["total"] * ca / 1e6; y = gb[k] / b["total"] * cb / 1e6
    print(f"{k:48} {x:10.2f} {y:12.2f} {y-x:+8.2f}")
print(f"{'Total':48} {ca/1e6:10.2f} {cb/1e6:12.2f} {(cb-ca)/1e6:+8.2f}")
