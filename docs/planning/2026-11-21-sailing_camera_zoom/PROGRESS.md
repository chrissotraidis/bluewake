# PROGRESS — Controller camera zoom while sailing

Status: **written, not built yet.** Waiting for CI and a hand check on the boat.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md) · Extends [#213](https://github.com/chrissotraidis/bluewake/pull/213)

## Story

**What we tried.** The owner reported that #213's gesture doesn't zoom while sailing, and asked for the same gesture to work there.

**What we found.** The gesture is limited to the follow camera on purpose. On the boat the camera is the game's `rideCamera` (engine 8), whose distance is parameter 10 of its style: seven styles in the DOL's data.

**Decision.** Scale the active boat style's distance from the game's own value every frame, with a boat zoom separate from the one on foot. Keep the rest of the boat's right stick as the game's.

## Log

- 2026-11-21: boat styles read from the disc's DOL; boat zoom, gesture routing, controls table and setting note written. Not built yet (owner asked to hold builds).

## Next

- CI.
- The hand check in the technical plan.
