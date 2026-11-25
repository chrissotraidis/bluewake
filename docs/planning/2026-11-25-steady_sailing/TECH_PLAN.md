# Technical plan — A steadier boat with Swift Sail

## Cause

`daShip_c::setYPos` (d_a_ship.rel, .text 0x23E0; the public decompilation, zeldaret/tww `src/d/actor/d_a_ship.cpp`) moves the boat toward the water's height (`m03F4`) each frame: 10 % of the gap scaled by speed / 55 (capped at 1), plus its own vertical speed `m03D0` (a spring: += 5 % of the gap, ±20 at most, eased toward 0). Then it holds the boat between 40 units above and 60 below the water.

Swift Sail doubles the top speed (55 → 110) but the speed factor is already 1 at 55, so the spring is no stronger while the swells pass twice as fast. It lags, overshoots, and sinks the boat toward the 60-unit floor; at that depth the sea surface cuts the deck.

## Change

- `mods/betterww/options.txt`: a hook for Swift Sail before `lfs f1, 0x10C(r31)` at `d_a_ship.rel+0x2788` (.text 0x2694), the instruction both branches of the hold meet at.
- `runtime/host/src/game_options.c`: hook 7 reads the boat (r30) — `current.pos.y` at 0x1FC, `m03D0` at 0x3D0, `m03F4` at 0x3F4 — and calls `sail_steady`.
- `runtime/host/src/sail_steady.h`: below `water − 20` the boat is held at `water − 20`, and a downward `m03D0` is set to 0 so the spring does not keep pushing into the floor. Above that, nothing changes.
- `tests/sail_steady_test.c` (audit workflow, sanitizers): untouched on, above and at the floor; held below it; a rising boat keeps its rise; negative heights.

Brisk Sail turns Swift Sail's sites on, so it gets the hook too.

## Risks

- **Hook placement:** a hook site is a block of its own in the translation; 0x2694 is a branch target of both arms of the hold, and the hook runs before it either way.
- **Too stiff:** at 20 the boat may look glued to the water in big swells. The depth is one constant; the owner judges on the Deck.
- **Only the sinking:** if the bobbing still looks strong, the tilt (`setWaveAngle`) and the spring's strength are the next places.

## Verification

- `sail_steady_test` passes; `game_options.py check` reads the list (18 options, 41 sites).
- A build, then the owner sails on the Deck with Swift Sail at full speed.
