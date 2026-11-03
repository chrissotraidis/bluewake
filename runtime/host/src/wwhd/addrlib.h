// Wii U (GX2) surface addressing for the built-in Wind Waker HD importer, ported
// from AboodXD's addrlib.py (GPL-3.0-or-later); see addrlib.cpp.
#pragma once

#include "wwhd.h"

#include <cstdint>

namespace wwhd::addrlib {

struct SurfaceInfo {
    int64_t size = 0, pitch = 0, height = 0, depth = 0, surfSize = 0, tileMode = 0, baseAlign = 0, pitchAlign = 0,
            heightAlign = 0, depthAlign = 0, bpp = 0, pixelPitch = 0, pixelHeight = 0, pixelBits = 0, sliceSize = 0;
};

int64_t surfaceGetBitsPerPixel(int64_t surfaceFormat);
SurfaceInfo getSurfaceInfo(int64_t surfaceFormat, int64_t surfaceWidth, int64_t surfaceHeight, int64_t surfaceDepth,
                           int64_t surfaceDim, int64_t surfaceTileMode, int64_t surfaceAA, int64_t level);
// Tiled GX2 surface data to rows, as addrlib.deswizzle.
Bytes deswizzle(int64_t width, int64_t height, int64_t height_, int64_t format, int64_t tileMode, int64_t swizzle,
                int64_t pitch, int64_t bitsPerPixel, const uint8_t* data, size_t size);

} // namespace wwhd::addrlib
