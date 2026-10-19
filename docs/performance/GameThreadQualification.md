# Game-thread qualification

[Performance overview](Overview.md) · [Architecture overview](../architecture/Overview.md)

This flow qualifies native game-thread work without publishing a disc, memory
card, translated module, or run log. Run it only with local owned-game inputs.

## 1. Preflight

Create a local JSON manifest with `artifacts.app`, `candidate_module`,
`reference_module`, and `source_card`, plus `device.class: steam-deck`. Then run:

```bash
python3 scripts/qualify_gamethread.py preflight \
  --manifest local-manifest.json --out qualification/report.json
```

The report stores hashes and sizes by role, not private paths. Preflight exits
3 because provenance alone is intentionally incomplete evidence.

## 2. Run the sequence

Use `python3 scripts/qualify_gamethread.py matrix` to inspect the staged matrix.
Run source-only and private-oracle checks first, then interleaved serial/candidate
gameplay, copied-card save/reload, and paired 30-minute rendered soaks. Retain raw
logs locally; add only pass/failure outcomes and non-sensitive aggregates.

## 3. Finalize

```bash
python3 scripts/qualify_gamethread.py finalize qualification/report.json
```

Finalization requires passing source-only, private-oracle, repeatability,
rendered, save-reload, and soak outcomes on a declared Steam Deck. Exit 3 means
required evidence is absent; it must not be reported as a performance result.
