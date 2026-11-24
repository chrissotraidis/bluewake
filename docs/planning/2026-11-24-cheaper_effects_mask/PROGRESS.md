# Progress — A cheaper effects mask for the HD look's shading

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

- 2026-11-24: measured on the owner's Deck from a save state (the Big Octo fight, sea room 17): 16 runs, in the PRD. The effects mask found to be 2 to 3 ms of a 24.6 ms frame; its second pass alone 2.1 ms. The plan written. No code yet.
- 2026-11-24: the code, first candidate: RecompCore #68 (the mask and the pass over white at half size, against a half-size depth; `DOL_AURORA_EFFECT_MASK_FAST`), pinned here; the Config switch "Faster effects mask" and its test entries. This branch now sits on #655's (both add a switch at the same place in the table). Settings tests pass locally; not yet built or measured.
