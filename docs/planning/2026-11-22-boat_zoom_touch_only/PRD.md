# PRD — Boat zoom: leave the game's data alone while it's off

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md) · Follow-up to [#633 review](https://github.com/chrissotraidis/bluewake/pull/633#pullrequestreview-6011551306)

## Why

#633's boat zoom checked all seven boat camera distances every frame and wrote the table's value back whenever one differed, even with the zoom never used. On the standard GZLE01 disc that writes nothing, but a mod or another disc with other boat distances would have them overwritten for every player.

## What changes for players

- Nothing with the zoom in use: it zooms as before.
- With the zoom unused, BlueWake no longer writes the boat camera's data at all.

## Success

- No write to a boat style unless the zoom has scaled it; after zooming, the style is put back to the game's distance once.
