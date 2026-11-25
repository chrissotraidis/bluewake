# PRD — A steadier boat with Swift Sail

## Why

With Swift Sail (Better Wind Waker's option, on by default) King of Red Lions sails twice as fast. The owner sees the boat bob up and down too strongly at that speed, far enough that the sea shows through the deck: no effect on play, but jarring to look at.

## What changes for players

- With Swift Sail on, the boat no longer sinks into the swells deeper than 20 units (the game allows 60). The sea stays below the deck.
- Nothing else: speed, steering, the wind, braking and collisions are as before. With Swift Sail off the game is unchanged.

## Not in scope

- The boat's tilt on the waves (pitch and roll), unless the owner still finds it too strong once the sinking is gone.
- A separate switch: this belongs to Swift Sail, which causes it.

## Success

- On the owner's Deck, sailing with Swift Sail at full speed: no sea showing through the deck.
- The boat still rides the swells visibly (it rises and falls, only not as deep).
