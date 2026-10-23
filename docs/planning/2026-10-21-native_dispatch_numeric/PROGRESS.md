# PROGRESS — Native dispatch and numeric-path evaluation

Status: Complete; M3 rejected without a runtime change

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Parent M3](https://github.com/chrissotraidis/bluewake/pull/421)

## Milestones

### M0: Attribute and select one M3 surface

Architecture: [native routines](../../architecture/NativeRoutines.md) · [compiled component tests](../../architecture/Testing.md#compiled-component-tests).

- [x] M0-T01: Identify the source-matched Deck control and record its provenance.
- [x] M0-T02: Capture the fixed-route profile and normal-exit dispatch/native reports.
- [x] M0-T03: Reject M3: the dispatch cache already implements the measured reduction; numeric and pool telemetry select no distinct change.

## Disposition

No implementation, worker-default, focused test, checkpoint, or A/B run is
pending. Those gates apply only after a concrete candidate is selected. The
existing worker and guard tests remain the baseline for any future candidate.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
| --- | --- | --- | --- | --- |
| M0-T01 | Deck AppImage SHA-256 `6b9dbccaf1fe…968c29b2`; state SHA-256 `847139a13390…efe6ddcb` | `sha256sum` | Diagnostic, source-matched control; fixed `quick-23874.bwstate` | Provenance recorded; not a current-source qualification artifact. |
| M0-T02 | Deck `m3-dispatch-numeric-census/run.log`; `m3-dispatch-numeric-perf-60s/perf.data` | Normal exit with `BLUEWAKE_DIRECT_CALL_TRACE=1`; 60 s, 99 Hz headless `perf` capture without tracing | `BLUEWAKE_NATIVE_WORKERS=2`; 0 lost samples | Direct calls active (23,222,258 / 23,306,700 allowed); native paths active; `array_max=8`, `parallel=0`; `chassis_dispatch=2.01%`, `selected_dispatch=1.34%`. |
| M0-T03 | `scripts/generate_composite.py`; `tests/test_composite.py`; `docs/archive/status/CURRENT.md` | Source/annotation review | Same generated dispatch path as the capture | Reject: `selected_dispatch` samples are in the existing exact PC-keyed cache and its hit path; no new equivalent reduction is identified. |
