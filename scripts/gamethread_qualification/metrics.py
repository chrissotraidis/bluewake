import math
import os
from collections import Counter
from pathlib import Path


def percentile(values, percent):
    """Nearest-rank percentile used by qualification reports."""
    if not values or not 0 < percent <= 100:
        raise ValueError("values must be nonempty and percent must be in (0, 100]")
    ordered = sorted(float(value) for value in values)
    return ordered[max(0, math.ceil(percent / 100 * len(ordered)) - 1)]


def timing_summary(samples_ns, *, hitch_ns, dropped_events=0):
    if any(value < 0 for value in samples_ns) or dropped_events < 0:
        raise ValueError("timings and dropped event counts cannot be negative")
    if not samples_ns:
        return {"available": False, "reason": "no verified update-completion samples"}
    return {
        "available": dropped_events == 0,
        "unit": "ns",
        "sample_count": len(samples_ns),
        "p50": percentile(samples_ns, 50),
        "p95": percentile(samples_ns, 95),
        "p99": percentile(samples_ns, 99),
        "hitches": sum(value > hitch_ns for value in samples_ns),
        "dropped_events": dropped_events,
        "reason": "event loss invalidates timing claims" if dropped_events else "",
    }


def join_wait_summary(caller_complete_ns, workers_complete_ns):
    if len(caller_complete_ns) != len(workers_complete_ns):
        raise ValueError("caller and worker completion grids differ")
    waits = []
    for caller, workers in zip(caller_complete_ns, workers_complete_ns):
        if workers < caller:
            raise ValueError("worker completion precedes caller completion")
        waits.append(workers - caller)
    return timing_summary(waits, hitch_ns=1_000_000)


def job_histogram(job_counts):
    if any(value < 0 for value in job_counts):
        raise ValueError("job counts cannot be negative")
    return {str(key): value for key, value in sorted(Counter(job_counts).items())}


def linux_topology(sys_cpu=Path("/sys/devices/system/cpu"), allowed=None):
    """Return allowed logical CPUs grouped by physical core; no affinity changes."""
    if allowed is None:
        try:
            allowed = sorted(os.sched_getaffinity(0))
        except AttributeError:
            return {"available": False, "reason": "process affinity is unavailable"}
    cores = {}
    for cpu in sorted(allowed):
        topology = Path(sys_cpu) / f"cpu{cpu}" / "topology"
        try:
            package = int((topology / "physical_package_id").read_text().strip())
            core = int((topology / "core_id").read_text().strip())
        except (OSError, ValueError):
            return {"available": False, "reason": f"topology unavailable for CPU {cpu}"}
        cores.setdefault(f"{package}:{core}", []).append(cpu)
    return {
        "available": True,
        "allowed_cpus": sorted(allowed),
        "physical_cores": cores,
        "smt_present": any(len(siblings) > 1 for siblings in cores.values()),
    }
