// Pixel work for the built-in Wind Waker HD importer (imaging.h). The BC1-BC5
// decoder follows Pillow's BcnDecode.c (public domain, CC0); the Lanczos
// resampler, the premultiplied-alpha conversions and the 255 multiply follow
// Pillow's Resample.c, Convert.c, Chops.c and ImagingUtils.h (HPND license),
// version 12.2.0, which scripts/wwhd pins.
#include "cancel.h"
#include "imaging.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wwhd {
namespace {

// --- BCn (Pillow BcnDecode.c) ---------------------------------------------

struct Rgba {
    uint8_t r, g, b, a;
};

Rgba decode_565(uint16_t x) {
    Rgba c;
    int r = (x & 0xf800) >> 8;
    r |= r >> 5;
    int g = (x & 0x7e0) >> 3;
    g |= g >> 6;
    int b = (x & 0x1f) << 3;
    b |= b >> 5;
    c.r = uint8_t(r);
    c.g = uint8_t(g);
    c.b = uint8_t(b);
    c.a = 0xff;
    return c;
}

void decode_bc1_color(Rgba* dst, const uint8_t* src, bool separate_alpha) {
    const uint16_t c0 = uint16_t(src[0] | src[1] << 8), c1 = uint16_t(src[2] | src[3] << 8);
    Rgba p[4];
    p[0] = decode_565(c0);
    p[1] = decode_565(c1);
    const unsigned r0 = p[0].r, g0 = p[0].g, b0 = p[0].b, r1 = p[1].r, g1 = p[1].g, b1 = p[1].b;
    if (c0 > c1 || separate_alpha) {
        p[2] = {uint8_t((2 * r0 + r1) / 3), uint8_t((2 * g0 + g1) / 3), uint8_t((2 * b0 + b1) / 3), 0xff};
        p[3] = {uint8_t((r0 + 2 * r1) / 3), uint8_t((g0 + 2 * g1) / 3), uint8_t((b0 + 2 * b1) / 3), 0xff};
    } else {
        p[2] = {uint8_t((r0 + r1) / 2), uint8_t((g0 + g1) / 2), uint8_t((b0 + b1) / 2), 0xff};
        p[3] = {0, 0, 0, 0};
    }
    for (int n = 0; n < 4; n++)
        for (int o = 0; o < 4; o++)
            dst[n * 4 + o] = p[3 & (src[4 + n] >> (2 * o))];
}

// Eight-value alpha (BC3 alpha, BC4, BC5 channels), written to byte o of each
// of the 16 pixels, stride bytes apart.
void decode_bc3_alpha(uint8_t* dst, const uint8_t* src, int stride, int o) {
    const unsigned a0 = src[0], a1 = src[1];
    const int lut1 = src[2] | src[3] << 8 | src[4] << 16;
    const int lut2 = src[5] | src[6] << 8 | src[7] << 16;
    uint8_t a[8];
    a[0] = uint8_t(a0);
    a[1] = uint8_t(a1);
    if (a0 > a1) {
        a[2] = uint8_t((6 * a0 + 1 * a1) / 7);
        a[3] = uint8_t((5 * a0 + 2 * a1) / 7);
        a[4] = uint8_t((4 * a0 + 3 * a1) / 7);
        a[5] = uint8_t((3 * a0 + 4 * a1) / 7);
        a[6] = uint8_t((2 * a0 + 5 * a1) / 7);
        a[7] = uint8_t((1 * a0 + 6 * a1) / 7);
    } else {
        a[2] = uint8_t((4 * a0 + 1 * a1) / 5);
        a[3] = uint8_t((3 * a0 + 2 * a1) / 5);
        a[4] = uint8_t((2 * a0 + 3 * a1) / 5);
        a[5] = uint8_t((1 * a0 + 4 * a1) / 5);
        a[6] = 0;
        a[7] = 0xff;
    }
    for (int n = 0; n < 8; n++)
        dst[stride * n + o] = a[7 & (lut1 >> (3 * n))];
    for (int n = 0; n < 8; n++)
        dst[stride * (8 + n) + o] = a[7 & (lut2 >> (3 * n))];
}

// Decodes blocks into RGBA as Pillow's DDS reader followed by convert("RGBA"):
// BC1-BC3 are RGBA, BC4 is L (grey, opaque), BC5 RGB (blue 0, opaque).
Image decode_bcn(int base_format, const uint8_t* src, size_t size, int width, int height) {
    Image out(width, height);
    const int block_bytes = (base_format == 0x31 || base_format == 0x34) ? 8 : 16;
    const int bw = (width + 3) / 4, bh = (height + 3) / 4;
    if (size < size_t(bw) * bh * block_bytes)
        throw FormatError("truncated BCn data");
    for (int by = 0; by < bh; by++) {
        check_cancel();
        for (int bx = 0; bx < bw; bx++) {
            const uint8_t* block = src + (size_t(by) * bw + bx) * block_bytes;
            Rgba col[16];
            std::memset(col, 0, sizeof col);
            switch (base_format) {
            case 0x31:
                decode_bc1_color(col, block, false);
                break;
            case 0x32:
                decode_bc1_color(col, block + 8, true);
                for (int n = 0; n < 16; n++) {
                    const int bit = n * 4;
                    const int av = 0xf & (block[bit >> 3] >> (bit & 7));
                    col[n].a = uint8_t(av << 4 | av);
                }
                break;
            case 0x33:
                decode_bc1_color(col, block + 8, true);
                decode_bc3_alpha(reinterpret_cast<uint8_t*>(col), block, 4, 3);
                break;
            case 0x34: {
                uint8_t lum[16] = {};
                decode_bc3_alpha(lum, block, 1, 0);
                for (int n = 0; n < 16; n++)
                    col[n] = {lum[n], lum[n], lum[n], 0xff};
                break;
            }
            case 0x35:
                decode_bc3_alpha(reinterpret_cast<uint8_t*>(col), block, 4, 0);
                decode_bc3_alpha(reinterpret_cast<uint8_t*>(col), block + 8, 4, 1);
                for (int n = 0; n < 16; n++) {
                    col[n].b = 0;
                    col[n].a = 0xff;
                }
                break;
            default:
                throw FormatError("unsupported BCn format");
            }
            for (int n = 0; n < 4; n++) {
                const int y = by * 4 + n;
                if (y >= height)
                    break;
                for (int o = 0; o < 4; o++) {
                    const int x = bx * 4 + o;
                    if (x >= width)
                        break;
                    std::memcpy(out.at(x, y), &col[n * 4 + o], 4);
                }
            }
        }
    }
    return out;
}

// --- Resample (Pillow Resample.c, LANCZOS, 8 bits per channel) --------------

constexpr int kPrecisionBits = 32 - 8 - 2;
constexpr double kPi = 3.14159265358979323846; // M_PI, which MSVC hides

double sinc(double x) {
    if (x == 0.0)
        return 1.0;
    x = x * kPi;
    return std::sin(x) / x;
}

double lanczos(double x) {
    if (-3.0 <= x && x < 3.0)
        return sinc(x) * sinc(x / 3);
    return 0.0;
}

uint8_t clip8(int in) {
    const int v = in >> kPrecisionBits;
    return uint8_t(v < 0 ? 0 : v > 255 ? 255 : v);
}

struct Coeffs {
    int ksize = 0;
    std::vector<int> bounds;   // xmin, xmax per output
    std::vector<int32_t> kk;   // fixed point
};

Coeffs precompute(int in_size, float in0, float in1, int out_size) {
    Coeffs c;
    const double scale = double(in1 - in0) / out_size;
    const double filterscale = scale < 1.0 ? 1.0 : scale;
    const double support = 3.0 * filterscale;
    c.ksize = int(std::ceil(support)) * 2 + 1;
    std::vector<double> kk(size_t(out_size) * c.ksize, 0.0);
    c.bounds.resize(size_t(out_size) * 2);
    for (int xx = 0; xx < out_size; xx++) {
        check_cancel();
        const double center = in0 + (xx + 0.5) * scale;
        double ww = 0.0;
        const double ss = 1.0 / filterscale;
        int xmin = int(center - support + 0.5);
        if (xmin < 0)
            xmin = 0;
        int xmax = int(center + support + 0.5);
        if (xmax > in_size)
            xmax = in_size;
        xmax -= xmin;
        double* k = &kk[size_t(xx) * c.ksize];
        int x = 0;
        for (; x < xmax; x++) {
            const double w = lanczos((x + xmin - center + 0.5) * ss);
            k[x] = w;
            ww += w;
        }
        for (x = 0; x < xmax; x++)
            if (ww != 0.0)
                k[x] /= ww;
        for (; x < c.ksize; x++)
            k[x] = 0;
        c.bounds[size_t(xx) * 2] = xmin;
        c.bounds[size_t(xx) * 2 + 1] = xmax;
    }
    c.kk.resize(kk.size());
    for (size_t i = 0; i < kk.size(); i++)
        c.kk[i] = kk[i] < 0 ? int32_t(-0.5 + kk[i] * (1 << kPrecisionBits))
                            : int32_t(0.5 + kk[i] * (1 << kPrecisionBits));
    return c;
}

// RGBa (premultiplied) resample, as ImagingResampleInner with box (0, 0, w, h).
Image resample(const Image& in, int xsize, int ysize) {
    const bool need_h = xsize != in.width;
    const bool need_v = ysize != in.height;
    const Coeffs h = precompute(in.width, 0.0f, float(in.width), xsize);
    Coeffs v = precompute(in.height, 0.0f, float(in.height), ysize);
    const int ybox_first = v.bounds[0];
    const int ybox_last = v.bounds[size_t(ysize) * 2 - 2] + v.bounds[size_t(ysize) * 2 - 1];
    Image cur = in;
    if (need_h) {
        for (int i = 0; i < ysize; i++)
            v.bounds[size_t(i) * 2] -= ybox_first;
        Image tmp(xsize, ybox_last - ybox_first);
        for (int yy = 0; yy < tmp.height; yy++) {
            check_cancel();
            for (int xx = 0; xx < xsize; xx++) {
                const int xmin = h.bounds[size_t(xx) * 2], xmax = h.bounds[size_t(xx) * 2 + 1];
                const int32_t* k = &h.kk[size_t(xx) * h.ksize];
                int ss[4];
                for (int c = 0; c < 4; c++)
                    ss[c] = 1 << (kPrecisionBits - 1);
                for (int x = 0; x < xmax; x++) {
                    const uint8_t* p = in.at(x + xmin, yy + ybox_first);
                    for (int c = 0; c < 4; c++)
                        ss[c] += p[c] * k[x];
                }
                uint8_t* o = tmp.at(xx, yy);
                for (int c = 0; c < 4; c++)
                    o[c] = clip8(ss[c]);
            }
        }
        cur = std::move(tmp);
    }
    if (need_v) {
        Image out(cur.width, ysize);
        for (int yy = 0; yy < ysize; yy++) {
            check_cancel();
            const int32_t* k = &v.kk[size_t(yy) * v.ksize];
            const int ymin = v.bounds[size_t(yy) * 2], ymax = v.bounds[size_t(yy) * 2 + 1];
            for (int xx = 0; xx < cur.width; xx++) {
                int ss[4];
                for (int c = 0; c < 4; c++)
                    ss[c] = 1 << (kPrecisionBits - 1);
                for (int y = 0; y < ymax; y++) {
                    const uint8_t* p = cur.at(xx, y + ymin);
                    for (int c = 0; c < 4; c++)
                        ss[c] += p[c] * k[y];
                }
                uint8_t* o = out.at(xx, yy);
                for (int c = 0; c < 4; c++)
                    o[c] = clip8(ss[c]);
            }
        }
        cur = std::move(out);
    }
    return cur;
}

uint8_t muldiv255(unsigned a, unsigned b) {
    const unsigned tmp = a * b + 128;
    return uint8_t(((tmp >> 8) + tmp) >> 8);
}

uint8_t clip(int v) { return uint8_t(v <= 0 ? 0 : v < 256 ? v : 255); }

} // namespace

Image resize_lanczos(const Image& in, int width, int height) {
    if (width == in.width && height == in.height)
        return in;
    Image pre = in;
    for (size_t i = 0; i < pre.rgba.size(); i += 4) {
        if (i % (64 * 1024) == 0)
            check_cancel();
        const unsigned a = pre.rgba[i + 3];
        pre.rgba[i] = muldiv255(pre.rgba[i], a);
        pre.rgba[i + 1] = muldiv255(pre.rgba[i + 1], a);
        pre.rgba[i + 2] = muldiv255(pre.rgba[i + 2], a);
    }
    Image out = resample(pre, width, height);
    for (size_t i = 0; i < out.rgba.size(); i += 4) {
        if (i % (64 * 1024) == 0)
            check_cancel();
        const int a = out.rgba[i + 3];
        if (a != 255 && a != 0) {
            out.rgba[i] = clip(255 * out.rgba[i] / a);
            out.rgba[i + 1] = clip(255 * out.rgba[i + 1] / a);
            out.rgba[i + 2] = clip(255 * out.rgba[i + 2] / a);
        }
    }
    return out;
}

void complete_mips(std::vector<Image>& images) {
    while (!(images.back().width == 1 && images.back().height == 1)) {
        check_cancel();
        const Image& last = images.back();
        Image next = resize_lanczos(last, std::max(1, last.width / 2), std::max(1, last.height / 2));
        images.push_back(std::move(next));
    }
}

std::vector<Image> hd_images(const HDTexture& t) {
    std::vector<Image> out;
    const int64_t base = t.format & 0xFF;
    if (t.components.size() != 4)
        throw FormatError("unsupported GX2 component selection");
    for (size_t level = 0; level < t.levels.size(); level++) {
        check_cancel();
        const Bytes& raw = t.levels[level];
        const int w = std::max(1, t.width >> level), h = std::max(1, t.height >> level);
        const size_t pixels = size_t(w) * h;
        Image image(w, h);
        if (base >= 0x31 && base <= 0x35) {
            if (t.format & 0x200)
                throw FormatError("signed BC surfaces are not replacement art");
            image = decode_bcn(int(base), raw.data(), raw.size(), w, h);
        } else if (base == 0x1A) {
            if (raw.size() < pixels * 4)
                throw FormatError("truncated RGBA surface");
            std::memcpy(image.rgba.data(), raw.data(), pixels * 4);
        } else if (base == 1 || base == 7) {
            // GX2 R8/RG8: red and green, blue 0, alpha 1 before the selection.
            const size_t stride = base == 1 ? 1 : 2;
            if (raw.size() < pixels * stride)
                throw FormatError("truncated surface");
            for (size_t i = 0; i < pixels; i++) {
                if (i % (16 * 1024) == 0)
                    check_cancel();
                uint8_t* o = &image.rgba[i * 4];
                o[0] = raw[i * stride];
                o[1] = stride == 2 ? raw[i * stride + 1] : 0;
                o[2] = 0;
                o[3] = 255;
            }
        } else if (base == 2 || base == 8 || base == 10 || base == 11 || base == 25) {
            int masks[4];
            switch (base) {
            case 2: masks[0] = 4; masks[1] = 4; masks[2] = 0; masks[3] = 0; break;
            case 8: masks[0] = 5; masks[1] = 6; masks[2] = 5; masks[3] = 0; break;
            case 10: masks[0] = 5; masks[1] = 5; masks[2] = 5; masks[3] = 1; break;
            case 11: masks[0] = 4; masks[1] = 4; masks[2] = 4; masks[3] = 4; break;
            default: masks[0] = 10; masks[1] = 10; masks[2] = 10; masks[3] = 2; break;
            }
            const int stride = (masks[0] + masks[1] + masks[2] + masks[3]) / 8;
            if (raw.size() < pixels * stride)
                throw FormatError("truncated surface");
            for (size_t i = 0; i < pixels; i++) {
                if (i % (16 * 1024) == 0)
                    check_cancel();
                uint32_t value = 0;
                for (int b = 0; b < stride; b++)
                    value |= uint32_t(raw[i * stride + b]) << (8 * b);
                for (int c = 0; c < 4; c++) {
                    const int bits = masks[c];
                    uint8_t v;
                    if (bits)
                        v = uint8_t((value & ((1u << bits) - 1)) * 255 / ((1u << bits) - 1));
                    else
                        v = c == 3 ? 255 : 0;
                    image.rgba[i * 4 + c] = v;
                    value >>= bits;
                }
            }
        } else {
            char text[64];
            std::snprintf(text, sizeof text, "unsupported Wii U pixel format 0x%llx", (long long)t.format);
            throw FormatError(text);
        }
        // GX2 component selection: 0-3 a channel, 4 zero, 5 one.
        for (uint8_t c : t.components)
            if (c > 5)
                throw FormatError("unsupported GX2 component selection");
        Image selected(w, h);
        for (size_t i = 0; i < pixels; i++) {
            if (i % (16 * 1024) == 0)
                check_cancel();
            const uint8_t* p = &image.rgba[i * 4];
            const uint8_t choices[6] = {p[0], p[1], p[2], p[3], 0, 255};
            for (int c = 0; c < 4; c++)
                selected.rgba[i * 4 + c] = choices[t.components[c]];
        }
        out.push_back(std::move(selected));
    }
    return out;
}

std::vector<Image> for_gc_format(const std::vector<Image>& images, int gc_format, bool layout_mask) {
    if (gc_format != 0 && gc_format != 1)
        return images;
    std::vector<Image> out;
    for (const Image& im : images) {
        Image o(im.width, im.height);
        for (size_t i = 0; i < im.rgba.size(); i += 4) {
            if (i % (64 * 1024) == 0)
                check_cancel();
            const uint8_t r = im.rgba[i], a = im.rgba[i + 3];
            const uint8_t v = layout_mask ? uint8_t(int(r) * int(a) / 255) : r;
            o.rgba[i] = o.rgba[i + 1] = o.rgba[i + 2] = o.rgba[i + 3] = v;
        }
        out.push_back(std::move(o));
    }
    return out;
}

Image crop(const Image& in, int left, int top, int right, int bottom) {
    Image out(std::max(0, right - left), std::max(0, bottom - top));
    for (int y = 0; y < out.height; y++) {
        check_cancel();
        const int sy = y + top;
        if (sy < 0 || sy >= in.height)
            continue;
        for (int x = 0; x < out.width; x++) {
            const int sx = x + left;
            if (sx >= 0 && sx < in.width)
                std::memcpy(out.at(x, y), in.at(sx, sy), 4);
        }
    }
    return out;
}

void paste(Image& dst, const Image& src, int x, int y) {
    for (int sy = 0; sy < src.height; sy++) {
        check_cancel();
        const int dy = y + sy;
        if (dy < 0 || dy >= dst.height)
            continue;
        for (int sx = 0; sx < src.width; sx++) {
            const int dx = x + sx;
            if (dx >= 0 && dx < dst.width)
                std::memcpy(dst.at(dx, dy), src.at(sx, sy), 4);
        }
    }
}

Image alpha_composite(const Image& dst, const Image& src) {
    if (dst.width != src.width || dst.height != src.height)
        throw FormatError("alpha_composite size mismatch");
    constexpr unsigned kBits = 7; // AlphaComposite.c PRECISION_BITS
    const auto shift_div255 = [](uint32_t a) { return ((a >> 8) + a) >> 8; };
    Image out(dst.width, dst.height);
    for (size_t i = 0; i < dst.rgba.size(); i += 4) {
        if (i % (64 * 1024) == 0)
            check_cancel();
        const uint8_t* d = &dst.rgba[i];
        const uint8_t* s = &src.rgba[i];
        uint8_t* o = &out.rgba[i];
        if (s[3] == 0) {
            std::memcpy(o, d, 4);
            continue;
        }
        const uint32_t blend = uint32_t(d[3]) * (255 - s[3]);
        const uint32_t outa255 = uint32_t(s[3]) * 255 + blend;
        const uint32_t coef1 = uint32_t(s[3]) * 255 * 255 * (1u << kBits) / outa255;
        const uint32_t coef2 = 255 * (1u << kBits) - coef1;
        for (int c = 0; c < 3; c++) {
            const uint32_t tmp = s[c] * coef1 + d[c] * coef2;
            o[c] = uint8_t(shift_div255(tmp + (0x80u << kBits)) >> kBits);
        }
        o[3] = uint8_t(shift_div255(outa255 + 0x80));
    }
    return out;
}

void alpha_composite_at(Image& dst, const Image& src, int x, int y) {
    const Image background = crop(dst, x, y, x + src.width, y + src.height);
    paste(dst, alpha_composite(background, src), x, y);
}

bool alpha_bbox(const Image& in, Box& box) {
    int left = in.width, top = in.height, right = -1, bottom = -1;
    for (int y = 0; y < in.height; y++) {
        check_cancel();
        for (int x = 0; x < in.width; x++)
            if (in.at(x, y)[3]) {
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = std::max(bottom, y);
            }
    }
    if (right < 0)
        return false;
    box = {left, top, right + 1, bottom + 1};
    return true;
}

Image flip_top_bottom(const Image& in) {
    Image out(in.width, in.height);
    const size_t row = size_t(in.width) * 4;
    for (int y = 0; y < in.height; y++) {
        check_cancel();
        std::memcpy(out.rgba.data() + size_t(in.height - 1 - y) * row, in.rgba.data() + size_t(y) * row, row);
    }
    return out;
}

} // namespace wwhd
