# Progress — Automatic render scale (opt-in)

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

- 2026-11-24: the plan written, from the two Deck scenes measured for #655 and #658 (full speed at 2x, 92 % and 83 % at 3x). Read in RecompCore: a live scale change exists (`aurora_set_frame_buffer_scale`) and takes the deferred resize path. Not yet measured: what a live change costs, Smooth Motion across it, the GPU signal. No code.
