from pathlib import Path
import sys


sys.path.insert(0, str(Path(__file__).parents[1] / "scripts"))

from route_digest import route_digest, route_records


def write_route(path: Path, *, blocks: int, clock: int, pc: str) -> None:
    path.write_text(
        "\n".join(
            [
                "[trace] ignored host diagnostic",
                f"[clock] summary cycles={clock} retraces=14100",
                "[cycle-delivery] summary external=2 hash=ABC",
                "[collision-provenance] summary valid_seen=1 sentinel_seen=1",
                "[fp] summary invalid=0 fresh=3 restored=9",
                "[cpu-abi] summary deferred_fp=0",
                "[boot-milestone] summary play_scene=1 play_scene_retrace=13910",
                "[scene-draw] summary bg=1/2@13931",
                "[dvd-lifecycle] forwards=3 failures=0",
                "[aram-dma] summary transfers=4 rejected=0",
                "[dsp-lle] summary dmas=5 first_nonzero=1",
                f"[run] stopped: normal after {blocks} blocks at pc={pc}",
                "real 12.34",
            ]
        )
        + "\n"
    )


def test_digest_ignores_host_turns_and_wall_time(tmp_path: Path) -> None:
    first = tmp_path / "first.log"
    second = tmp_path / "second.log"
    card = tmp_path / "card.raw"
    card.write_bytes(b"canonical card")
    write_route(first, blocks=100, clock=123, pc="0x80000004")
    write_route(second, blocks=900, clock=123, pc="0x80000004")

    assert route_digest(first, card) == route_digest(second, card)
    assert route_records(first)[-1] == "[run] stopped: normal pc=0x80000004"


def test_digest_detects_architectural_or_card_drift(tmp_path: Path) -> None:
    first = tmp_path / "first.log"
    second = tmp_path / "second.log"
    card = tmp_path / "card.raw"
    changed_card = tmp_path / "changed.raw"
    card.write_bytes(b"canonical card")
    changed_card.write_bytes(b"changed card")
    write_route(first, blocks=100, clock=123, pc="0x80000004")
    write_route(second, blocks=100, clock=124, pc="0x80000004")

    assert route_digest(first, card) != route_digest(second, card)
    assert route_digest(first, card) != route_digest(first, changed_card)
