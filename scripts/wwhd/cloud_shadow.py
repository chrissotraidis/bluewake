"""Wind Waker HD's cloud shadow texture (ww_shadow_clouds) for the HD look's cloud shadows on HD's rule.

HD projects it onto the scenery from above, a repeat every 15,000 units, drifting along x (RecompCore's sun shadows
read it; their notes have the projection). It is the sea stage's: first_szs_permanent.pack's sea_Stage.szs, its
Stage.bfres, 4096 x 4096 BC4 with 13 mips. The importer keeps its blocks as they are, untiled, in a DDS
(optional/hd-cloud-shadow/ww_shadow_clouds.dds: FourCC BC4U, every mip). Only from the player's own disc.
"""
import struct

HD_CLOUD_SHADOW_FOLDER = "optional/hd-cloud-shadow"
HD_CLOUD_SHADOW_FILE = "ww_shadow_clouds.dds"
HD_CLOUD_SHADOW_NAME = "ww_shadow_clouds"
GX2_BC4_UNORM = 0x34


def is_source(path: str) -> bool:
    """The archive member that holds it: the sea stage's Stage.bfres."""
    return path.endswith("/sea_Stage.szs/Stage.bfres")


def dds_bc4(width: int, height: int, levels: list[bytes]) -> bytes:
    """A BC4 DDS of untiled mip levels (each as GX2 stores BC4: 8-byte blocks, rows of blocks)."""
    flags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x20000 | 0x80000  # caps, height, width, pixel format, mip count, linear size
    caps = 0x1000 | 0x8 | 0x400000  # texture, complex, mipmap
    header = struct.pack("<7I", 124, flags, height, width, len(levels[0]), 0, len(levels)) + bytes(44)
    header += struct.pack("<2I4s5I", 32, 0x4, b"BC4U", 0, 0, 0, 0, 0)
    header += struct.pack("<5I", caps, 0, 0, 0, 0)
    return b"DDS " + header + b"".join(levels)
