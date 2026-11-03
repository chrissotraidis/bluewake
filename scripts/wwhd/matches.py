"""Audited choices for art shared by several GameCube resources.

HD debug rooms and figurines sometimes reuse a name with different artwork.
For these visually checked aliases, choose the variant preserving the original
UV composition. Equal-priority disagreements still remain unresolved.
"""

PREFERRED = {
    "dk_kamen1": ("object", "demo_dk"),
    "test_grass_ktga": ("stage", "a_mori", "room0"),
    "txa_nami_ia": ("stage", "sea", "room11"),
    "txa_nami_01": ("stage", "sea", "room11"),
    "door_08": ("stage", "ebesso", "stage"),
    "txa_doom_a": ("stage", "sea", "room44"),
    "txa_hune_a": ("stage", "sea", "room44"),
    "txa_ita3mai_a": ("stage", "sea", "room44"),
    "txa_iwa2_a": ("stage", "linkug", "room0"),
    "bm_head01": ("object", "demo10"),
    # Some HD skies changed to photographic cloud layers. These retained
    # cartoon variants match the silhouette and alpha of the GameCube draw.
    "ws_naka06": ("stage", "cave08", "stage"),
    "ws_mae08": ("stage", "cave08", "stage"),
    "ws_mae09": ("stage", "cave08", "stage"),
    "cloudtx_01": ("stage", "a_nami", "stage"),
    "cloudtx_03": ("stage", "a_nami", "stage"),
    "txo_stone_mori_old": ("stage", "sea", "room42"),
}


# HD's own sky clouds, the sea's, kept apart from the pack in optional/hd-clouds
# for the HD look's "HD's clouds in the sky" switch. Two are GameCube I4/I8
# bands that HD draws from colour with the cloud in alpha, so all of them keep
# HD's own colour and alpha rather than the GameCube's intensity.
HD_CLOUDS = {"ws_naka06", "ws_mae08", "ws_mae09", "cloudtx_01", "cloudtx_02", "cloudtx_03"}
HD_CLOUDS_SOURCE = ("stage", "sea", "stage")
HD_CLOUDS_FOLDER = "optional/hd-clouds"


# Conflicts chosen by eye (runtime/host/src/wwhd/picked.inc): {texture file: HD source from Common/ on}.
def _picked():
    import re
    from pathlib import Path
    inc = Path(__file__).resolve().parents[2] / "runtime/host/src/wwhd/picked.inc"
    return {f: s[s.find("Common/"):] for f, s in re.findall(r'\{"([^"]+)", "([^"]+)"\}', inc.read_text())}


PICKED = _picked()


def picked_priority(key, source):
    """2 for the HD texture chosen for this texture file, above PREFERRED's 1."""
    chosen = PICKED.get(key)
    return 2 if chosen is not None and source[source.find("Common/"):] == chosen else 0


def candidate_priority(hd, identity):
    return int(PREFERRED.get(hd.name.lower()) == identity)
