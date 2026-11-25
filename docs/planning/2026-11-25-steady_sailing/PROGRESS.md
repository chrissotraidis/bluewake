# Progress — A steadier boat with Swift Sail

- 2026-11-25: the owner reported the boat bobbing too strongly with their fast-sailing settings (Swift Sail, Brisk Sail), the sea showing through the deck; cosmetic only. Cause found in `daShip_c::setYPos` (the boat's spring onto the water overshoots at twice the speed and may sink 60 units). The owner approved the fix (hold the sinking to 20 units with Swift Sail on) and a build.
- 2026-11-25: hook 7 in Swift Sail's sites, `sail_steady.h` and its test; `sail_steady_test` passes, the option list checks (18 options, 41 sites).
- 2026-11-25: built (stack `4fdd81f27`); the hook is in the translated `d_a_ship` code behind Swift Sail's switch. On the owner's Deck as BlueWake-sail. The technical plan now has the height code, the cause, the hook site's disassembly, the native code and diagrams, at the owner's request.
