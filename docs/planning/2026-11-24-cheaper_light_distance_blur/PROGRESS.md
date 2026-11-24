# Progress — A cheaper "Light" distance blur

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

- 2026-11-24: measured on the owner's Deck from a save state (rainy Windfall at night, sea room 11). Distance blur: Light found to cost 4–5 retraces a second at 3x; the plan written. No code yet.
- 2026-11-24: reruns with ambient occlusion off (59.6, 58.7) and the HD look off (60.0) ran to the end; the two crashes first read as startup crashes were near the end of their runs and did not repeat. Reported apart.
- 2026-11-24: the code: RecompCore #67 (the half-size blur, `DOL_AURORA_GAME_DOF_FAST`), pinned here; the Config switch "Faster Light distance blur" and its test entries. Settings tests pass locally; not yet built or measured.
