# BlueWake - Minimum Supported Platforms & Device Classes

**Recorded:** 2026-08-21 (P0)
**Policy change rule:** Changing these marks affected platform/performance evidence stale per PRD section 13/P0.

## macOS

| | Minimum supported | Current development |
|---|---|---|
| OS version | macOS 14.0 (Sonoma) | macOS 26.5 |
| Architecture | Apple Silicon (arm64) only | Apple Silicon M-series |
| Device class | M1 Mac mini or better | M1 Pro+ for development |

## iOS / iPadOS

| | Minimum supported | Current development |
|---|---|---|
| OS version | iOS 17.0 / iPadOS 17.0 | Xcode 26.6 SDK |
| Minimum iPhone | iPhone SE (3rd gen) / iPhone 12s | Representative current iPhone |
| Minimum iPad | iPad (9th gen) / iPad mini (6th gen) | Representative current iPad |

## Toolchain

| Tool | Version |
|------|---------|
| CMake | >= 3.27 |
| Ninja | >= 1.11 |
| Python | >= 3.10 |
| Xcode | >= 16 (for iOS targets) |
| Rust | stable (for nod-on-iOS later) |

## Evidence boundary

Simulator coverage cannot certify physical performance, thermals, real controller lifecycle, audio interruption, haptics, sustained speed, or physical touch ergonomics (PRD section 14.6).
