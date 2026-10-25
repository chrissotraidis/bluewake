// Pixel work for the built-in Wind Waker HD importer: decoding Wii U texture
// data to RGBA and resizing it, matching Pillow 12.2 (which scripts/wwhd uses)
// byte for byte, so the pack's images are the ones the script makes.
#pragma once

#include "hd.h"

namespace wwhd {

struct Image {
    int width = 0;
    int height = 0;
    Bytes rgba; // width * height * 4

    Image() = default;
    Image(int w, int h) : width(w), height(h), rgba(size_t(w) * h * 4, 0) {}
    uint8_t* at(int x, int y) { return rgba.data() + (size_t(y) * width + x) * 4; }
    const uint8_t* at(int x, int y) const { return rgba.data() + (size_t(y) * width + x) * 4; }
};

// HDTexture.images(): each stored level as RGBA, with the GX2 component selection.
std::vector<Image> hd_images(const HDTexture& t);
// Image.resize(size, LANCZOS) for an RGBA image (premultiplied, as Pillow does).
Image resize_lanczos(const Image& in, int width, int height);
// Halve with Lanczos until 1x1, as the importer completes a mip chain.
void complete_mips(std::vector<Image>& images);
// for_gc_format(): GameCube I4/I8 (formats 0 and 1) sample intensity everywhere.
std::vector<Image> for_gc_format(const std::vector<Image>& images, int gc_format, bool layout_mask);

// Pillow operations the generated HUD and title art use, with Pillow's results.
// crop(): pixels outside the image are transparent black.
Image crop(const Image& in, int left, int top, int right, int bottom);
// paste(src, (x, y)) without a mask: src replaces what it covers (clipped).
void paste(Image& dst, const Image& src, int x, int y);
// Image.alpha_composite(dst, src) for two images of one size.
Image alpha_composite(const Image& dst, const Image& src);
// dst.alpha_composite(src, (x, y)), in place, as Pillow's method.
void alpha_composite_at(Image& dst, const Image& src, int x, int y);
// getbbox() of one 8-bit channel: false when every value is 0.
struct Box {
    int left = 0, top = 0, right = 0, bottom = 0;
};
bool alpha_bbox(const Image& in, Box& box);
Image flip_top_bottom(const Image& in);

} // namespace wwhd
