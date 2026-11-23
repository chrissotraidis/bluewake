# Progress — Boat zoom: leave the game's data alone while it's off

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md) · Follow-up to [#633 review](https://github.com/chrissotraidis/bluewake/pull/633#pullrequestreview-6011551306)

- 2026-11-22: per-style written flags in `boat_zoom_frame`. Draft PR opened; not yet built or tested on the Deck.
- 2026-11-22: tested on a Steam Deck in Game Mode (build main-642-643, 23eb4da30 = main + #642 + #643): no zoom leaves the boat camera as the game has it; zooming is smooth with no camera turn; boarding again returns to the last boat zoom, by design; on foot the zoom and the first-person tap are unchanged. Marked ready for review.
