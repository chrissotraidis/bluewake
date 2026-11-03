// Wii U (GX2) surface addressing for the built-in Wind Waker HD importer: a
// line-by-line C++ port of AboodXD's addrlib.py from BFRES-Tool
// (https://github.com/aboood40091/BFRES-Tool, revision e00e9bcf,
// GPL-3.0-or-later, copyright AboodXD 2018), which scripts/wwhd vendors
// unchanged (scripts/wwhd/vendor/SOURCE.json). Function names follow it so the
// two can be read side by side; Python's integers are 64-bit here.
#include "cancel.h"
#include "addrlib.h"

#include <algorithm>
#include <cstring>

namespace wwhd::addrlib {
namespace {

using i64 = int64_t;

const uint8_t kFormatHwInfo[256] = {
    0x00, 0x00, 0x00, 0x01, 0x08, 0x03, 0x00, 0x01, 0x08, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00, 0x10, 0x03, 0x00, 0x01, 0x10, 0x03, 0x00, 0x01,
    0x10, 0x0B, 0x00, 0x01, 0x10, 0x01, 0x00, 0x01, 0x10, 0x03, 0x00, 0x01, 0x10, 0x03, 0x00, 0x01,
    0x10, 0x03, 0x00, 0x01, 0x20, 0x03, 0x00, 0x00, 0x20, 0x07, 0x00, 0x00, 0x20, 0x03, 0x00, 0x00,
    0x20, 0x03, 0x00, 0x01, 0x20, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x20, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x01, 0x20, 0x0B, 0x00, 0x01, 0x20, 0x0B, 0x00, 0x01, 0x20, 0x0B, 0x00, 0x01,
    0x40, 0x05, 0x00, 0x00, 0x40, 0x03, 0x00, 0x00, 0x40, 0x03, 0x00, 0x00, 0x40, 0x03, 0x00, 0x00,
    0x40, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0x00, 0x80, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x10, 0x01, 0x00, 0x00,
    0x10, 0x01, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x60, 0x01, 0x00, 0x00,
    0x60, 0x01, 0x00, 0x00, 0x40, 0x01, 0x00, 0x01, 0x80, 0x01, 0x00, 0x01, 0x80, 0x01, 0x00, 0x01,
    0x40, 0x01, 0x00, 0x01, 0x80, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t kFormatExInfo[256] = {
    0x00, 0x01, 0x01, 0x03, 0x08, 0x01, 0x01, 0x03, 0x08, 0x01, 0x01, 0x03, 0x08, 0x01, 0x01, 0x03,
    0x00, 0x01, 0x01, 0x03, 0x10, 0x01, 0x01, 0x03, 0x10, 0x01, 0x01, 0x03, 0x10, 0x01, 0x01, 0x03,
    0x10, 0x01, 0x01, 0x03, 0x10, 0x01, 0x01, 0x03, 0x10, 0x01, 0x01, 0x03, 0x10, 0x01, 0x01, 0x03,
    0x10, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03,
    0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03,
    0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03,
    0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03,
    0x40, 0x01, 0x01, 0x03, 0x40, 0x01, 0x01, 0x03, 0x40, 0x01, 0x01, 0x03, 0x40, 0x01, 0x01, 0x03,
    0x40, 0x01, 0x01, 0x03, 0x00, 0x01, 0x01, 0x03, 0x80, 0x01, 0x01, 0x03, 0x80, 0x01, 0x01, 0x03,
    0x00, 0x01, 0x01, 0x03, 0x01, 0x08, 0x01, 0x05, 0x01, 0x08, 0x01, 0x06, 0x10, 0x01, 0x01, 0x07,
    0x10, 0x01, 0x01, 0x08, 0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03, 0x20, 0x01, 0x01, 0x03,
    0x18, 0x03, 0x01, 0x04, 0x30, 0x03, 0x01, 0x04, 0x30, 0x03, 0x01, 0x04, 0x60, 0x03, 0x01, 0x04,
    0x60, 0x03, 0x01, 0x04, 0x40, 0x04, 0x04, 0x09, 0x80, 0x04, 0x04, 0x0A, 0x80, 0x04, 0x04, 0x0B,
    0x40, 0x04, 0x04, 0x0C, 0x40, 0x04, 0x04, 0x0D, 0x40, 0x04, 0x04, 0x0D, 0x40, 0x04, 0x04, 0x0D,
    0x00, 0x01, 0x01, 0x03, 0x00, 0x01, 0x01, 0x03, 0x00, 0x01, 0x01, 0x03, 0x00, 0x01, 0x01, 0x03,
    0x00, 0x01, 0x01, 0x03, 0x00, 0x01, 0x01, 0x03, 0x40, 0x01, 0x01, 0x03, 0x00, 0x01, 0x01, 0x03,
};

bool is_bcn(i64 format) {
    switch (format) {
    case 0x31: case 0x431: case 0x32: case 0x432: case 0x33: case 0x433:
    case 0x34: case 0x234: case 0x35: case 0x235:
        return true;
    default:
        return false;
    }
}

bool in(i64 v, std::initializer_list<i64> set) {
    return std::find(set.begin(), set.end(), v) != set.end();
}

i64 computeSurfaceThickness(i64 tileMode) {
    if (in(tileMode, {3, 7, 11, 13, 15}))
        return 4;
    if (in(tileMode, {16, 17}))
        return 8;
    return 1;
}

i64 computePixelIndexWithinMicroTile(i64 x, i64 y, i64 bpp) {
    if (bpp == 0x08)
        return 32 * ((y & 4) >> 2) | 16 * (y & 1) | 8 * ((y & 2) >> 1) | 4 * ((x & 4) >> 2) | 2 * ((x & 2) >> 1) | (x & 1);
    if (bpp == 0x10)
        return 32 * ((y & 4) >> 2) | 16 * ((y & 2) >> 1) | 8 * (y & 1) | 4 * ((x & 4) >> 2) | 2 * ((x & 2) >> 1) | (x & 1);
    if (bpp == 0x20 || bpp == 0x60)
        return 32 * ((y & 4) >> 2) | 16 * ((y & 2) >> 1) | 8 * ((x & 4) >> 2) | 4 * (y & 1) | 2 * ((x & 2) >> 1) | (x & 1);
    if (bpp == 0x40)
        return 32 * ((y & 4) >> 2) | 16 * ((y & 2) >> 1) | 8 * ((x & 4) >> 2) | 4 * ((x & 2) >> 1) | 2 * (y & 1) | (x & 1);
    if (bpp == 0x80)
        return 32 * ((y & 4) >> 2) | 16 * ((y & 2) >> 1) | 8 * ((x & 4) >> 2) | 4 * ((x & 2) >> 1) | 2 * (x & 1) | (y & 1);
    return 32 * ((y & 4) >> 2) | 16 * ((y & 2) >> 1) | 8 * ((x & 4) >> 2) | 4 * (y & 1) | 2 * ((x & 2) >> 1) | (x & 1);
}

i64 computePipeFromCoordWoRotation(i64 x, i64 y) { return ((y >> 3) ^ (x >> 3)) & 1; }

i64 computeBankFromCoordWoRotation(i64 x, i64 y) {
    return (((y >> 5) ^ (x >> 3)) & 1) | (2 * (((y >> 4) ^ (x >> 4)) & 1));
}

i64 isThickMacroTiled(i64 tileMode) { return in(tileMode, {7, 11, 13, 15}) ? 1 : 0; }
i64 isBankSwappedTileMode(i64 tileMode) { return in(tileMode, {8, 9, 10, 11, 14, 15}) ? 1 : 0; }

i64 computeMacroTileAspectRatio(i64 tileMode) {
    if (in(tileMode, {5, 9}))
        return 2;
    if (in(tileMode, {6, 10}))
        return 4;
    return 1;
}

i64 computeSurfaceBankSwappedWidth(i64 tileMode, i64 bpp, i64 pitch, i64 numSamples = 1) {
    if (isBankSwappedTileMode(tileMode) == 0)
        return 0;
    const i64 bytesPerSample = 8 * bpp;
    i64 slicesPerTile = 1;
    if (bytesPerSample != 0) {
        const i64 samplesPerTile = 2048 / bytesPerSample;
        slicesPerTile = std::max<i64>(1, numSamples / samplesPerTile);
    }
    if (isThickMacroTiled(tileMode) != 0)
        numSamples = 4;
    const i64 bytesPerTileSlice = numSamples * bytesPerSample / slicesPerTile;
    const i64 factor = computeMacroTileAspectRatio(tileMode);
    const i64 swapTiles = std::max<i64>(1, 128 / bpp);
    const i64 swapWidth = swapTiles * 32;
    const i64 heightBytes = numSamples * factor * bpp * 2 / slicesPerTile;
    const i64 swapMax = 0x4000 / heightBytes;
    const i64 swapMin = 256 / bytesPerTileSlice;
    i64 bankSwapWidth = std::min(swapMax, std::max(swapMin, swapWidth));
    while (bankSwapWidth >= 2 * pitch)
        bankSwapWidth >>= 1;
    return bankSwapWidth;
}

i64 computeSurfaceAddrFromCoordMicroTiled(i64 x, i64 y, i64 bpp, i64 pitch, i64 tileMode) {
    const i64 microTileThickness = tileMode == 3 ? 4 : 1;
    const i64 microTileBytes = (64 * microTileThickness * bpp + 7) / 8;
    const i64 microTilesPerRow = pitch >> 3;
    const i64 microTileOffset = microTileBytes * ((x >> 3) + (y >> 3) * microTilesPerRow);
    const i64 pixelOffset = (bpp * computePixelIndexWithinMicroTile(x, y, bpp)) >> 3;
    return pixelOffset + microTileOffset;
}

const i64 kBankSwapOrder[] = {0, 1, 3, 2, 6, 7, 5, 4, 0, 0};

i64 computeSurfaceAddrFromCoordMacroTiled(i64 x, i64 y, i64 bpp, i64 pitch, i64 height, i64 tileMode, i64 pipeSwizzle,
                                          i64 bankSwizzle) {
    const i64 microTileThickness = computeSurfaceThickness(tileMode);
    const i64 microTileBits = bpp * (microTileThickness * 64);
    const i64 microTileBytes = (microTileBits + 7) / 8;
    i64 elemOffset = bpp * computePixelIndexWithinMicroTile(x, y, bpp);
    const i64 bytesPerSample = microTileBytes;
    i64 numSamples, sampleSlice;
    if (microTileBytes <= 2048) {
        numSamples = 1;
        sampleSlice = 0;
    } else {
        const i64 samplesPerSlice = 2048 / bytesPerSample;
        const i64 numSampleSplits = std::max<i64>(1, samplesPerSlice ? 1 / samplesPerSlice : 1);
        numSamples = samplesPerSlice;
        sampleSlice = elemOffset / (microTileBits / numSampleSplits);
        elemOffset %= microTileBits / numSampleSplits;
    }
    elemOffset = (elemOffset + 7) / 8;
    i64 pipe = computePipeFromCoordWoRotation(x, y);
    i64 bank = computeBankFromCoordWoRotation(x, y);
    const i64 swizzle = pipeSwizzle + 2 * bankSwizzle;
    const i64 bankPipe = ((pipe + 2 * bank) ^ (6 * sampleSlice ^ swizzle)) % 8;
    pipe = bankPipe % 2;
    bank = bankPipe / 2;
    const i64 sliceBytes = (height * pitch * microTileThickness * bpp * numSamples + 7) / 8;
    const i64 sliceOffset = sliceBytes * (sampleSlice / microTileThickness);
    i64 macroTilePitch = 32, macroTileHeight = 16;
    if (in(tileMode, {5, 9})) {
        macroTilePitch >>= 1;
        macroTileHeight *= 2;
    } else if (in(tileMode, {6, 10})) {
        macroTilePitch >>= 2;
        macroTileHeight *= 4;
    }
    const i64 macroTilesPerRow = pitch / macroTilePitch;
    const i64 macroTileBytes = (numSamples * microTileThickness * bpp * macroTileHeight * macroTilePitch + 7) / 8;
    const i64 macroTileIndexX = x / macroTilePitch;
    const i64 macroTileIndexY = y / macroTileHeight;
    const i64 macroTileOffset = (macroTileIndexX + macroTilesPerRow * macroTileIndexY) * macroTileBytes;
    if (in(tileMode, {8, 9, 10, 11, 14, 15})) {
        const i64 bankSwapWidth = computeSurfaceBankSwappedWidth(tileMode, bpp, pitch, 1);
        const i64 swapIndex = macroTilePitch * macroTileIndexX / bankSwapWidth;
        bank ^= kBankSwapOrder[swapIndex & 3];
    }
    const i64 totalOffset = elemOffset + ((macroTileOffset + sliceOffset) >> 3);
    return bank << 9 | pipe << 8 | (255 & totalOffset) | (totalOffset & -256) << 3;
}

struct SurfaceIn {
    i64 tileMode = 0, format = 0, bpp = 0, numSamples = 0, width = 0, height = 0, numSlices = 0, slice = 0,
        mipLevel = 0, flags = 0;
};

i64 powTwoAlign(i64 x, i64 align) { return ~(align - 1) & (x + align - 1); }

i64 nextPow2(i64 dim) {
    i64 newDim = 1;
    if (dim <= 0x7FFFFFFF) {
        while (newDim < dim)
            newDim *= 2;
    } else {
        newDim = 0x80000000LL;
    }
    return newDim;
}

i64 convertToNonBankSwappedMode(i64 tileMode) {
    switch (tileMode) {
    case 8: return 4;
    case 9: return 5;
    case 10: return 6;
    case 11: return 7;
    case 14: return 12;
    case 15: return 13;
    default: return tileMode;
    }
}

i64 computeSurfaceTileSlices(i64 tileMode, i64 bpp, i64 numSamples) {
    const i64 bytePerSample = ((bpp << 6) + 7) >> 3;
    i64 tileSlices = 1;
    if (computeSurfaceThickness(tileMode) > 1)
        numSamples = 4;
    if (bytePerSample) {
        const i64 samplePerTile = 2048 / bytePerSample;
        if (samplePerTile)
            tileSlices = std::max<i64>(1, numSamples / samplePerTile);
    }
    return tileSlices;
}

i64 computeSurfaceMipLevelTileMode(i64 baseTileMode, i64 bpp, i64 level, i64 width, i64 height, i64 numSlices,
                                   i64 numSamples, i64 isDepth, i64 noRecursive) {
    i64 widthAlignFactor = 1, macroTileWidth = 32, macroTileHeight = 16;
    const i64 tileSlices = computeSurfaceTileSlices(baseTileMode, bpp, numSamples);
    i64 expTileMode = baseTileMode;
    if (baseTileMode == 7) {
        if (numSamples > 1 || tileSlices > 1 || isDepth)
            expTileMode = 4;
    } else if (baseTileMode == 13) {
        if (numSamples > 1 || tileSlices > 1 || isDepth)
            expTileMode = 12;
    } else if (baseTileMode == 11) {
        if (numSamples > 1 || tileSlices > 1 || isDepth)
            expTileMode = 8;
    } else if (baseTileMode == 15) {
        if (numSamples > 1 || tileSlices > 1 || isDepth)
            expTileMode = 14;
    } else if (baseTileMode == 2) {
        if (numSamples > 1)
            expTileMode = 4;
    } else if (baseTileMode == 3) {
        if (numSamples > 1 || isDepth)
            expTileMode = 2;
        if (numSamples == 2 || numSamples == 4)
            expTileMode = 7;
    }
    if (!noRecursive) {
        if (in(bpp, {24, 48, 96}))
            bpp /= 3;
        const i64 widtha = nextPow2(width), heighta = nextPow2(height), numSlicesa = nextPow2(numSlices);
        if (level) {
            expTileMode = convertToNonBankSwappedMode(expTileMode);
            const i64 thickness = computeSurfaceThickness(expTileMode);
            const i64 microTileBytes = (numSamples * bpp * (thickness << 6) + 7) >> 3;
            if (microTileBytes < 256)
                widthAlignFactor = std::max<i64>(1, 256 / microTileBytes);
            if (expTileMode == 4 || expTileMode == 12) {
                if (widtha < widthAlignFactor * macroTileWidth || heighta < macroTileHeight)
                    expTileMode = 2;
            } else if (expTileMode == 5) {
                macroTileWidth = 16;
                macroTileHeight = 32;
                if (widtha < widthAlignFactor * macroTileWidth || heighta < macroTileHeight)
                    expTileMode = 2;
            } else if (expTileMode == 6) {
                macroTileWidth = 8;
                macroTileHeight = 64;
                if (widtha < widthAlignFactor * macroTileWidth || heighta < macroTileHeight)
                    expTileMode = 2;
            }
            if (expTileMode == 7 || expTileMode == 13) {
                if (widtha < widthAlignFactor * macroTileWidth || heighta < macroTileHeight)
                    expTileMode = 3;
            }
            if (expTileMode == 3) {
                if (numSlicesa < 4)
                    expTileMode = 2;
            } else if (expTileMode == 7) {
                if (numSlicesa < 4)
                    expTileMode = 4;
            } else if (expTileMode == 13 && numSlicesa < 4) {
                expTileMode = 12;
            }
            return computeSurfaceMipLevelTileMode(expTileMode, bpp, level, widtha, heighta, numSlicesa, numSamples,
                                                  isDepth, 1);
        }
    }
    return expTileMode;
}

struct Result {
    i64 valid = 0, pitch = 0, height = 0, numSlices = 0, surfSize = 0, tileMode = 0, baseAlign = 0, pitchAlign = 0,
        heightAlign = 0, depthAlign = 0;
};

// The module-level state of addrlib.py (pIn, pOut, expPitch...), one per call.
struct Addr {
    SurfaceIn pIn;
    SurfaceInfo pOut;
    i64 expPitch = 0, expHeight = 0, expNumSlices = 0;

    void padDimensions(i64 tileMode, i64 padDims, i64 isCube, i64 /*cubeAsArray*/, i64 pitchAlign, i64 heightAlign,
                       i64 sliceAlign) {
        const i64 thickness = computeSurfaceThickness(tileMode);
        if (!padDims)
            padDims = 3;
        if (!(pitchAlign & (pitchAlign - 1))) {
            expPitch = powTwoAlign(expPitch, pitchAlign);
        } else {
            expPitch = pitchAlign + expPitch - 1;
            expPitch /= pitchAlign;
            expPitch *= pitchAlign;
        }
        if (padDims > 1)
            expHeight = powTwoAlign(expHeight, heightAlign);
        if (padDims > 2 || thickness > 1) {
            if (isCube)
                expNumSlices = nextPow2(expNumSlices);
            if (thickness > 1)
                expNumSlices = powTwoAlign(expNumSlices, sliceAlign);
        }
    }

    static i64 adjustPitchAlignment(i64 flags, i64 pitchAlign) {
        if ((flags >> 13) & 1)
            pitchAlign = powTwoAlign(pitchAlign, 0x20);
        return pitchAlign;
    }

    Result computeSurfaceInfoLinear(i64 tileMode, i64 bpp, i64 numSamples, i64 pitch, i64 height, i64 numSlices,
                                    i64 mipLevel, i64 padDims, i64 flags) {
        expPitch = pitch;
        expHeight = height;
        expNumSlices = numSlices;
        const i64 microTileThickness = computeSurfaceThickness(tileMode);
        i64 baseAlign, pitchAlign, heightAlign;
        if (tileMode) {
            if (tileMode == 1) {
                const i64 pixelsPerPipeInterleave = 2048 / bpp;
                baseAlign = 256;
                pitchAlign = std::max<i64>(0x40, pixelsPerPipeInterleave);
                heightAlign = 1;
            } else {
                baseAlign = pitchAlign = heightAlign = 1;
            }
        } else {
            baseAlign = 1;
            pitchAlign = bpp != 1 ? 1 : 8;
            heightAlign = 1;
        }
        pitchAlign = adjustPitchAlignment(flags, pitchAlign);
        if ((flags >> 9) & 1 && !mipLevel) {
            expPitch /= 3;
            expPitch = nextPow2(expPitch);
        }
        if (mipLevel) {
            expPitch = nextPow2(expPitch);
            expHeight = nextPow2(expHeight);
            if ((flags >> 4) & 1) {
                expNumSlices = numSlices;
                padDims = numSlices <= 1 ? 2 : 0;
            } else {
                expNumSlices = nextPow2(numSlices);
            }
        }
        padDimensions(tileMode, padDims, (flags >> 4) & 1, (flags >> 7) & 1, pitchAlign, heightAlign,
                      microTileThickness);
        if ((flags >> 9) & 1 && !mipLevel)
            expPitch *= 3;
        const i64 slices = expNumSlices * numSamples / microTileThickness;
        Result r;
        r.valid = 1;
        r.pitch = expPitch;
        r.height = expHeight;
        r.numSlices = expNumSlices;
        r.surfSize = (expHeight * expPitch * slices * bpp * numSamples + 7) / 8;
        r.baseAlign = baseAlign;
        r.pitchAlign = pitchAlign;
        r.heightAlign = heightAlign;
        r.depthAlign = microTileThickness;
        return r;
    }

    Result computeSurfaceInfoMicroTiled(i64 tileMode, i64 bpp, i64 numSamples, i64 pitch, i64 height, i64 numSlices,
                                        i64 mipLevel, i64 padDims, i64 flags) {
        i64 expTileMode = tileMode;
        expPitch = pitch;
        expHeight = height;
        expNumSlices = numSlices;
        i64 microTileThickness = computeSurfaceThickness(tileMode);
        if (mipLevel) {
            expPitch = nextPow2(pitch);
            expHeight = nextPow2(height);
            if ((flags >> 4) & 1) {
                expNumSlices = numSlices;
                padDims = numSlices <= 1 ? 2 : 0;
            } else {
                expNumSlices = nextPow2(numSlices);
            }
            if (expTileMode == 3 && expNumSlices < 4) {
                expTileMode = 2;
                microTileThickness = 1;
            }
        }
        i64 abpp = bpp;
        if (in(abpp, {24, 48, 96}))
            abpp /= 3;
        const i64 thickness = computeSurfaceThickness(expTileMode);
        const i64 baseAlign = 256;
        i64 pitchAlign = std::max<i64>(8, 256 / abpp / numSamples / thickness);
        const i64 heightAlign = 8;
        pitchAlign = adjustPitchAlignment(flags, pitchAlign);
        padDimensions(expTileMode, padDims, (flags >> 4) & 1, (flags >> 7) & 1, pitchAlign, heightAlign,
                      microTileThickness);
        Result r;
        r.valid = 1;
        r.pitch = expPitch;
        r.height = expHeight;
        r.numSlices = expNumSlices;
        r.surfSize = (expHeight * expPitch * expNumSlices * bpp * numSamples + 7) / 8;
        r.tileMode = expTileMode;
        r.baseAlign = baseAlign;
        r.pitchAlign = pitchAlign;
        r.heightAlign = heightAlign;
        r.depthAlign = microTileThickness;
        return r;
    }

    struct MacroAlign {
        i64 baseAlign, pitchAlign, heightAlign, macroWidth, macroHeight;
    };

    static MacroAlign computeSurfaceAlignmentsMacroTiled(i64 tileMode, i64 bpp, i64 flags, i64 numSamples) {
        const i64 aspectRatio = computeMacroTileAspectRatio(tileMode);
        const i64 thickness = computeSurfaceThickness(tileMode);
        if (in(bpp, {24, 48, 96}))
            bpp /= 3;
        if (bpp == 3)
            bpp = 1;
        const i64 macroTileWidth = 32 / aspectRatio;
        const i64 macroTileHeight = aspectRatio * 16;
        i64 pitchAlign = std::max(macroTileWidth, macroTileWidth * (256 / bpp / (8 * thickness) / numSamples));
        pitchAlign = adjustPitchAlignment(flags, pitchAlign);
        const i64 heightAlign = macroTileHeight;
        const i64 macroTileBytes = numSamples * ((bpp * macroTileHeight * macroTileWidth + 7) >> 3);
        i64 baseAlign;
        if (thickness == 1)
            baseAlign = std::max(macroTileBytes, (numSamples * heightAlign * bpp * pitchAlign + 7) >> 3);
        else
            baseAlign = std::max<i64>(256, (4 * heightAlign * bpp * pitchAlign + 7) >> 3);
        const i64 microTileBytes = (thickness * numSamples * (bpp << 6) + 7) >> 3;
        const i64 numSlicesPerMicroTile = microTileBytes < 2048 ? 1 : microTileBytes / 2048;
        baseAlign /= numSlicesPerMicroTile;
        return {baseAlign, pitchAlign, heightAlign, macroTileWidth, macroTileHeight};
    }

    Result computeSurfaceInfoMacroTiled(i64 tileMode, i64 baseTileMode, i64 bpp, i64 numSamples, i64 pitch,
                                        i64 height, i64 numSlices, i64 mipLevel, i64 padDims, i64 flags) {
        expPitch = pitch;
        expHeight = height;
        expNumSlices = numSlices;
        i64 expTileMode = tileMode;
        i64 microTileThickness = computeSurfaceThickness(tileMode);
        if (mipLevel) {
            expPitch = nextPow2(pitch);
            expHeight = nextPow2(height);
            if ((flags >> 4) & 1) {
                expNumSlices = numSlices;
                padDims = numSlices <= 1 ? 2 : 0;
            } else {
                expNumSlices = nextPow2(numSlices);
            }
            if (expTileMode == 7 && expNumSlices < 4) {
                expTileMode = 4;
                microTileThickness = 1;
            }
        }
        const auto finish = [&](const MacroAlign& a, i64 pitchAlign) {
            padDimensions(tileMode, padDims, (flags >> 4) & 1, (flags >> 7) & 1, pitchAlign, a.heightAlign,
                          microTileThickness);
            Result r;
            r.valid = 1;
            r.pitch = expPitch;
            r.height = expHeight;
            r.numSlices = expNumSlices;
            r.surfSize = (expHeight * expPitch * expNumSlices * bpp * numSamples + 7) / 8;
            r.tileMode = expTileMode;
            r.baseAlign = a.baseAlign;
            r.pitchAlign = pitchAlign;
            r.heightAlign = a.heightAlign;
            r.depthAlign = microTileThickness;
            return r;
        };
        if (tileMode == baseTileMode || !mipLevel || !isThickMacroTiled(baseTileMode) || isThickMacroTiled(tileMode)) {
            const MacroAlign a = computeSurfaceAlignmentsMacroTiled(tileMode, bpp, flags, numSamples);
            i64 pitchAlign = a.pitchAlign;
            const i64 bankSwappedWidth = computeSurfaceBankSwappedWidth(tileMode, bpp, pitch, numSamples);
            if (bankSwappedWidth > pitchAlign)
                pitchAlign = bankSwappedWidth;
            return finish(a, pitchAlign);
        }
        const MacroAlign b = computeSurfaceAlignmentsMacroTiled(baseTileMode, bpp, flags, numSamples);
        const i64 pitchAlignFactor = std::max<i64>(1, 32 / bpp);
        if (expPitch < b.pitchAlign * pitchAlignFactor || expHeight < b.heightAlign) {
            expTileMode = 2;
            return computeSurfaceInfoMicroTiled(2, bpp, numSamples, pitch, height, numSlices, mipLevel, padDims, flags);
        }
        const MacroAlign a = computeSurfaceAlignmentsMacroTiled(tileMode, bpp, flags, numSamples);
        i64 pitchAlign = a.pitchAlign;
        const i64 bankSwappedWidth = computeSurfaceBankSwappedWidth(tileMode, bpp, pitch, numSamples);
        if (bankSwappedWidth > pitchAlign)
            pitchAlign = bankSwappedWidth;
        return finish(a, pitchAlign);
    }

    int ComputeSurfaceInfoEx() {
        i64 tileMode = pIn.tileMode;
        const i64 bpp = pIn.bpp;
        const i64 numSamples = std::max<i64>(1, pIn.numSamples);
        const i64 pitch = pIn.width, height = pIn.height, numSlices = pIn.numSlices, mipLevel = pIn.mipLevel;
        const i64 flags = pIn.flags;
        Result r;
        r.pitch = pOut.pitch;
        r.height = pOut.height;
        r.numSlices = pOut.depth;
        r.tileMode = pOut.tileMode;
        r.surfSize = pOut.surfSize;
        r.baseAlign = pOut.baseAlign;
        r.pitchAlign = pOut.pitchAlign;
        r.heightAlign = pOut.heightAlign;
        r.depthAlign = pOut.depthAlign;
        i64 padDims = 0;
        const i64 baseTileMode = tileMode;
        if ((flags >> 4) & 1 && !mipLevel)
            padDims = 2;
        if ((flags >> 6) & 1)
            tileMode = convertToNonBankSwappedMode(tileMode);
        else
            tileMode = computeSurfaceMipLevelTileMode(tileMode, bpp, mipLevel, pitch, height, numSlices, numSamples,
                                                      (flags >> 1) & 1, 0);
        if (tileMode == 0 || tileMode == 1) {
            r = computeSurfaceInfoLinear(tileMode, bpp, numSamples, pitch, height, numSlices, mipLevel, padDims, flags);
            r.tileMode = tileMode;
        } else if (tileMode == 2 || tileMode == 3) {
            r = computeSurfaceInfoMicroTiled(tileMode, bpp, numSamples, pitch, height, numSlices, mipLevel, padDims,
                                             flags);
        } else if (tileMode >= 4 && tileMode <= 15) {
            r = computeSurfaceInfoMacroTiled(tileMode, baseTileMode, bpp, numSamples, pitch, height, numSlices,
                                             mipLevel, padDims, flags);
        }
        pOut.pitch = r.pitch;
        pOut.height = r.height;
        pOut.depth = r.numSlices;
        pOut.tileMode = r.tileMode;
        pOut.surfSize = r.surfSize;
        pOut.baseAlign = r.baseAlign;
        pOut.pitchAlign = r.pitchAlign;
        pOut.heightAlign = r.heightAlign;
        pOut.depthAlign = r.depthAlign;
        return r.valid ? 0 : 3;
    }

    i64 adjustSurfaceInfo(i64 elemMode, i64 expandX, i64 expandY, i64 bpp, i64 width, i64 height) {
        const bool bcn = bpp && in(elemMode, {9, 10, 11, 12, 13});
        if (width && height && (expandX > 1 || expandY > 1)) {
            i64 widtha, heighta;
            if (elemMode == 4) {
                widtha = expandX * width;
                heighta = expandY * height;
            } else if (bcn) {
                widtha = width / expandX;
                heighta = height / expandY;
            } else {
                widtha = (width + expandX - 1) / expandX;
                heighta = (height + expandY - 1) / expandY;
            }
            pIn.width = std::max<i64>(1, widtha);
            pIn.height = std::max<i64>(1, heighta);
        }
        if (bpp) {
            if (elemMode == 4)
                pIn.bpp = bpp / expandX / expandY;
            else if (elemMode == 5 || elemMode == 6)
                pIn.bpp = expandY * expandX * bpp;
            else if (elemMode == 9 || elemMode == 12)
                pIn.bpp = 64;
            else if (elemMode == 10 || elemMode == 11 || elemMode == 13)
                pIn.bpp = 128;
            else
                pIn.bpp = bpp;
            return pIn.bpp;
        }
        return 0;
    }

    void computeMipLevel() {
        if (pIn.format >= 49 && pIn.format <= 55 && (!pIn.mipLevel || ((pIn.flags >> 12) & 1))) {
            pIn.width = powTwoAlign(pIn.width, 4);
            pIn.height = powTwoAlign(pIn.height, 4);
        }
        bool hwlHandled = false;
        if (pIn.format >= 49 && pIn.format <= 55) {
            if (pIn.mipLevel) {
                i64 width = pIn.width, height = pIn.height, slices = pIn.numSlices;
                if ((pIn.flags >> 12) & 1) {
                    const i64 widtha = width >> pIn.mipLevel, heighta = height >> pIn.mipLevel;
                    if (!((pIn.flags >> 4) & 1))
                        slices >>= pIn.mipLevel;
                    width = std::max<i64>(1, widtha);
                    height = std::max<i64>(1, heighta);
                    slices = std::max<i64>(1, slices);
                }
                pIn.width = nextPow2(width);
                pIn.height = nextPow2(height);
                pIn.numSlices = slices;
            }
            hwlHandled = true;
        }
        if (!hwlHandled && pIn.mipLevel && ((pIn.flags >> 12) & 1)) {
            i64 width = std::max<i64>(1, pIn.width >> pIn.mipLevel);
            i64 height = std::max<i64>(1, pIn.height >> pIn.mipLevel);
            i64 slices = std::max<i64>(1, pIn.numSlices);
            if (!((pIn.flags >> 4) & 1))
                slices = std::max<i64>(1, slices >> pIn.mipLevel);
            if (pIn.format != 47 && pIn.format != 48) {
                width = nextPow2(width);
                height = nextPow2(height);
                slices = nextPow2(slices);
            }
            pIn.width = width;
            pIn.height = height;
            pIn.numSlices = slices;
        }
    }

    i64 restoreSurfaceInfo(i64 elemMode, i64 expandX, i64 expandY, i64 bpp) {
        if (pOut.pixelPitch && pOut.pixelHeight) {
            i64 width = pOut.pixelPitch, height = pOut.pixelHeight;
            if (expandX > 1 || expandY > 1) {
                if (elemMode == 4) {
                    width /= expandX;
                    height /= expandY;
                } else {
                    width *= expandX;
                    height *= expandY;
                }
            }
            pOut.pixelPitch = std::max<i64>(1, width);
            pOut.pixelHeight = std::max<i64>(1, height);
        }
        if (bpp) {
            if (elemMode == 4)
                return expandY * expandX * bpp;
            if (elemMode == 5 || elemMode == 6)
                return bpp / expandX / expandY;
            if (elemMode == 9 || elemMode == 12)
                return 64;
            if (elemMode == 10 || elemMode == 11 || elemMode == 13)
                return 128;
            return bpp;
        }
        return 0;
    }

    void computeSurfaceInfo() {
        int returnCode = pIn.bpp > 0x80 ? 3 : 0;
        if (returnCode != 0)
            return;
        computeMipLevel();
        const i64 width = pIn.width, height = pIn.height;
        i64 bpp = pIn.bpp, expandX = 1, expandY = 1, elemMode = 0;
        pOut.pixelBits = pIn.bpp;
        if (pIn.format) {
            const i64 at = (pIn.format & 0xFF) * 4;
            bpp = kFormatExInfo[at];
            expandX = kFormatExInfo[at + 1];
            expandY = kFormatExInfo[at + 2];
            elemMode = kFormatExInfo[at + 3];
            if (elemMode == 4 && expandX == 3 && pIn.tileMode == 1)
                pIn.flags |= 0x200;
            bpp = adjustSurfaceInfo(elemMode, expandX, expandY, bpp, width, height);
        } else if (pIn.bpp) {
            pIn.width = std::max<i64>(1, pIn.width);
            pIn.height = std::max<i64>(1, pIn.height);
        } else {
            returnCode = 3;
        }
        if (returnCode == 0)
            returnCode = ComputeSurfaceInfoEx();
        if (returnCode == 0) {
            pOut.bpp = pIn.bpp;
            pOut.pixelPitch = pOut.pitch;
            pOut.pixelHeight = pOut.height;
            if (pIn.format && (!((pIn.flags >> 9) & 1) || !pIn.mipLevel))
                bpp = restoreSurfaceInfo(elemMode, expandX, expandY, bpp);
            if ((pIn.flags >> 5) & 1) {
                pOut.sliceSize = pOut.surfSize;
            } else {
                pOut.sliceSize = pOut.depth ? pOut.surfSize / pOut.depth : 0;
                if (pIn.slice == pIn.numSlices - 1 && pIn.numSlices > 1)
                    pOut.sliceSize += pOut.sliceSize * (pOut.depth - pIn.numSlices);
            }
        }
    }
};

} // namespace

int64_t surfaceGetBitsPerPixel(int64_t surfaceFormat) { return kFormatHwInfo[(surfaceFormat & 0x3F) * 4]; }

SurfaceInfo getSurfaceInfo(int64_t surfaceFormat, int64_t surfaceWidth, int64_t surfaceHeight, int64_t surfaceDepth,
                           int64_t surfaceDim, int64_t surfaceTileMode, int64_t surfaceAA, int64_t level) {
    Addr a;
    const i64 hwFormat = surfaceFormat & 0x3F;
    if (surfaceTileMode == 16) {
        // Linear special mode: not used by the HD archives the importer reads.
        throw FormatError("unsupported GX2 tile mode 16");
    }
    a.pIn.tileMode = surfaceTileMode & 0xF;
    a.pIn.format = hwFormat;
    a.pIn.bpp = kFormatHwInfo[hwFormat * 4];
    a.pIn.numSamples = i64(1) << surfaceAA;
    a.pIn.width = std::max<i64>(1, surfaceWidth >> level);
    switch (surfaceDim) {
    case 0: a.pIn.height = 1; a.pIn.numSlices = 1; break;
    case 1: a.pIn.height = std::max<i64>(1, surfaceHeight >> level); a.pIn.numSlices = 1; break;
    case 2: a.pIn.height = std::max<i64>(1, surfaceHeight >> level); a.pIn.numSlices = std::max<i64>(1, surfaceDepth >> level); break;
    case 3: a.pIn.height = std::max<i64>(1, surfaceHeight >> level); a.pIn.numSlices = std::max<i64>(6, surfaceDepth); a.pIn.flags |= 0x10; break;
    case 4: a.pIn.height = 1; a.pIn.numSlices = surfaceDepth; break;
    case 5: a.pIn.height = std::max<i64>(1, surfaceHeight >> level); a.pIn.numSlices = surfaceDepth; break;
    case 6: a.pIn.height = std::max<i64>(1, surfaceHeight >> level); a.pIn.numSlices = 1; break;
    case 7: a.pIn.height = std::max<i64>(1, surfaceHeight >> level); a.pIn.numSlices = surfaceDepth; break;
    default: break;
    }
    a.pIn.slice = 0;
    a.pIn.mipLevel = level;
    if (surfaceDim == 2)
        a.pIn.flags |= 0x20;
    if (level == 0)
        a.pIn.flags = (1 << 12) | (a.pIn.flags & 0xFFFFEFFF);
    else
        a.pIn.flags = a.pIn.flags & 0xFFFFEFFF;
    a.computeSurfaceInfo();
    return a.pOut;
}

Bytes deswizzle(int64_t width, int64_t height, int64_t height_, int64_t format, int64_t tileMode, int64_t swizzle,
                int64_t pitch, int64_t bitsPerPixel, const uint8_t* data, size_t size) {
    const i64 bytesPerPixel = bitsPerPixel / 8;
    Bytes result(size);
    if (is_bcn(format)) {
        width = (width + 3) / 4;
        height = (height + 3) / 4;
    }
    const i64 pipeSwizzle = (swizzle >> 8) & 1;
    const i64 bankSwizzle = (swizzle >> 9) & 3;
    const i64 length = static_cast<i64>(size);
    for (i64 y = 0; y < height; ++y) {
        check_cancel();
        for (i64 x = 0; x < width; ++x) {
            i64 pos;
            if (tileMode == 0 || tileMode == 1)
                pos = (y * pitch + x) * bytesPerPixel;
            else if (tileMode == 2 || tileMode == 3)
                pos = computeSurfaceAddrFromCoordMicroTiled(x, y, bitsPerPixel, pitch, tileMode);
            else
                pos = computeSurfaceAddrFromCoordMacroTiled(x, y, bitsPerPixel, pitch, height_, tileMode, pipeSwizzle,
                                                            bankSwizzle);
            const i64 to = (y * width + x) * bytesPerPixel;
            if (to + bytesPerPixel <= length && pos + bytesPerPixel <= length && pos >= 0)
                std::memcpy(result.data() + to, data + pos, static_cast<size_t>(bytesPerPixel));
        }
    }
    return result;
}

} // namespace wwhd::addrlib
