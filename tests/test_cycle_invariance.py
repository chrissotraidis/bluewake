from pathlib import Path

from scripts.cycle_invariance import compare_logs, invariant_lines


def write_log(path: Path, delivery_hash: str = "A1") -> None:
    path.write_text(
        "\n".join(
            [
                "[clock] cycle-domain cap=256",
                "[clock] summary cycles=100 timebase=8 retraces=1",
                f"[cycle-delivery] summary external=1 hash={delivery_hash}",
                "[cycle-delivery] external[0] ordinal=1 cycle=50 pc=0x80000004",
                "[aram-dma] summary transfers=2 rejected=0",
                "[run] stopped: normal after 123 blocks at pc=0x80000008",
            ]
        )
        + "\n"
    )


def test_cycle_invariance_ignores_cap_and_host_turn_count(tmp_path: Path):
    cap256 = tmp_path / "cap256.log"
    cap1024 = tmp_path / "cap1024.log"
    write_log(cap256)
    write_log(cap1024)
    cap1024.write_text(
        cap1024.read_text()
        .replace("cap=256", "cap=1024")
        .replace("after 123 blocks", "after 45 blocks")
    )

    passed, message = compare_logs([cap256, cap1024])
    assert passed, message
    assert len(invariant_lines(cap256)) == 4


def test_cycle_invariance_reports_first_architectural_difference(tmp_path: Path):
    baseline = tmp_path / "baseline.log"
    drift = tmp_path / "drift.log"
    write_log(baseline)
    write_log(drift, delivery_hash="B2")

    passed, message = compare_logs([baseline, drift])
    assert not passed
    assert "invariant record 2 differs" in message
    assert "hash=A1" in message
    assert "hash=B2" in message
