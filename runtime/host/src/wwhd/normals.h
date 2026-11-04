#pragma once

// Wind Waker HD's normals and vertex colours, keyed by the GameCube's exact vertex positions, for the HD look's sun
// shadows, as scripts/wwhd/normals.py makes them (with j3d.py, the GameCube's J3D models, and fmdl.py, HD's BFRES
// FMDL models): the importer feeds HdNormals every HD archive member, flushes after each HD archive, and writes
// tables()' files: kHdNormalsFolder/<stage>.bin, kHdNormalsFolder/kHdObjectsFile, kHdColoursFolder/<stage>.bin and
// kHdColoursFolder/kHdObjectsFile.
// Both importers write the same bytes: the same order everywhere, the same double arithmetic in the same order
// (no fused multiply-adds here), Python's round() (halves to even), sum() (compensated) and x ** 2 (the C library's
// pow). Header-only, so no platform's build lists another file.

#include "wwhd.h"
#include "cancel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__clang__)
#pragma float_control(push)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC push_options
#pragma GCC optimize("fp-contract=off")
#elif defined(_MSC_VER)
#pragma float_control(precise, on, push)
#endif

namespace wwhd {

inline constexpr const char* kHdNormalsFolder = "optional/hd-normals";
inline constexpr const char* kHdColoursFolder = "optional/hd-colours";
inline constexpr const char* kHdObjectsFile = "objects.bin";

namespace normals_detail {

inline constexpr double kTol = 0.1;          // the match radius once a joint's offset is voted
inline constexpr double kTolFallback = 1.0;  // a stage model that matches nothing at kTol tries this
inline constexpr double kStageShare = 0.5;   // a stage model under this share of its positions matched is left out
inline constexpr double kObjectShare = 0.95; // an object model needs this share
inline constexpr double kLarge = 1.5;        // an offset longer than this moves a whole piece
inline constexpr double kPi = 3.141592653589793;
inline constexpr uint32_t kGxPosition = 9, kGxNormal = 10;

using Mat = std::array<double, 12>; // 3x4, row-major
using Vec3 = std::array<double, 3>;
using Key = std::array<uint8_t, 12>; // a GameCube position's three little-endian float32s
using Rows = std::map<Key, std::vector<Vec3>>;

inline constexpr Mat kIdentity{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};

// Python's struct.error and IndexError: a read past the data. read_model() and read_models() make it a FormatError;
// a FormatError the readers raise themselves passes through as it is, as in Python.
struct Truncated : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Big-endian reads at signed offsets.
struct View {
    const uint8_t* p = nullptr;
    int64_t n = 0;

    void need(int64_t at, int64_t size) const {
        if (at < 0 || size < 0 || at > n || size > n - at) {
            char text[96];
            std::snprintf(text, sizeof text, "truncated data at 0x%llx (%lld bytes)", static_cast<long long>(at),
                          static_cast<long long>(size));
            throw Truncated(text);
        }
    }
    uint8_t u8(int64_t at) const {
        need(at, 1);
        return p[at];
    }
    uint16_t u16(int64_t at) const {
        need(at, 2);
        return uint16_t(p[at] << 8 | p[at + 1]);
    }
    int16_t i16(int64_t at) const { return static_cast<int16_t>(u16(at)); }
    uint32_t u32(int64_t at) const {
        need(at, 4);
        return uint32_t(p[at]) << 24 | uint32_t(p[at + 1]) << 16 | uint32_t(p[at + 2]) << 8 | p[at + 3];
    }
    int32_t i32(int64_t at) const { return static_cast<int32_t>(u32(at)); }
    float f32(int64_t at) const {
        const uint32_t bits = u32(at);
        float f;
        std::memcpy(&f, &bits, 4);
        return f;
    }
    // d[at:at + size] == tag, which is never an error.
    bool is(int64_t at, const char* tag, int64_t size = 4) const {
        return at >= 0 && at <= n && size <= n - at && std::memcmp(p + at, tag, size_t(size)) == 0;
    }
};

// Python's float arithmetic that C++ doesn't spell the same.
inline double py_square(double x) { // x ** 2: the C library's pow, which isn't always x * x
    const volatile double two = 2.0;
    return std::pow(std::fabs(x), two);
}
inline int64_t py_round(double x) { return static_cast<int64_t>(std::nearbyint(x)); } // halves to even
inline double py_max(double a, double b) { return b > a ? b : a; }

// sum() of Python numbers (3.12 and later): ints exactly until the first float, then Neumaier's compensated sum.
struct PySum {
    int64_t whole = 0;
    double hi = 0.0, lo = 0.0;
    bool floating = false;

    void add(double x, bool is_int) {
        if (!floating) {
            if (is_int) {
                whole += static_cast<int64_t>(x);
                return;
            }
            hi = static_cast<double>(whole) + x;
            floating = true;
            return;
        }
        const double t = hi + x;
        if (std::fabs(hi) >= std::fabs(x))
            lo += (hi - t) + x;
        else
            lo += (x - t) + hi;
        hi = t;
    }
    double value() const {
        if (!floating)
            return static_cast<double>(whole);
        return lo != 0.0 && std::isfinite(lo) ? hi + lo : hi;
    }
};

inline double half(uint16_t h) {
    const int exponent = (h >> 10) & 0x1F, mantissa = h & 0x3FF;
    double v;
    if (exponent == 0)
        v = std::ldexp(double(mantissa), -24);
    else if (exponent == 31)
        v = mantissa ? std::numeric_limits<double>::quiet_NaN() : std::numeric_limits<double>::infinity();
    else
        v = std::ldexp(double(mantissa | 0x400), exponent - 25);
    return (h & 0x8000) ? -v : v;
}

inline Key key_of(const std::array<float, 3>& p) {
    Key k;
    for (int c = 0; c < 3; ++c) {
        uint32_t bits;
        std::memcpy(&bits, &p[c], 4);
        for (int b = 0; b < 4; ++b)
            k[4 * c + b] = uint8_t(bits >> (8 * b));
    }
    return k;
}

inline void put32(Bytes& out, uint32_t v) {
    for (int b = 0; b < 4; ++b)
        out.push_back(uint8_t(v >> (8 * b)));
}

inline void put_float(Bytes& out, double v) {
    const float f = static_cast<float>(v);
    uint32_t bits;
    std::memcpy(&bits, &f, 4);
    put32(out, bits);
}

// ---- Matrices (j3d.py) ----

inline Mat mat_mul(const Mat& a, const Mat& b) {
    Mat out;
    for (int r = 0; r < 3; ++r) {
        const double a0 = a[4 * r], a1 = a[4 * r + 1], a2 = a[4 * r + 2], a3 = a[4 * r + 3];
        for (int c = 0; c < 4; ++c) {
            double v = a0 * b[c] + a1 * b[4 + c] + a2 * b[8 + c];
            if (c == 3)
                v += a3;
            out[4 * r + c] = v;
        }
    }
    return out;
}

inline Vec3 apply(const Mat& m, double x, double y, double z) {
    return {m[0] * x + m[1] * y + m[2] * z + m[3], m[4] * x + m[5] * y + m[6] * z + m[7],
            m[8] * x + m[9] * y + m[10] * z + m[11]};
}

// X, then Y, then Z; scale and translation.
inline Mat euler_matrix(double x, double y, double z, const double* s, const double* t) {
    const double cx = std::cos(x), sx = std::sin(x), cy = std::cos(y), sy = std::sin(y), cz = std::cos(z),
                 sz = std::sin(z);
    const double r[3][3] = {{cy * cz, sx * sy * cz - cx * sz, cx * sy * cz + sx * sz},
                            {cy * sz, sx * sy * sz + cx * cz, cx * sy * sz - sx * cz},
                            {-sy, sx * cy, cx * cy}};
    return {r[0][0] * s[0], r[0][1] * s[1], r[0][2] * s[2], t[0], r[1][0] * s[0], r[1][1] * s[1],
            r[1][2] * s[2], t[1], r[2][0] * s[0], r[2][1] * s[1], r[2][2] * s[2], t[2]};
}

// ---- GameCube J3D models (j3d.py) ----

struct J3DModel {
    std::vector<std::array<float, 3>> positions; // raw VTX1 float32 positions (empty unless they are float32)
    std::vector<Mat> joints;                     // each joint's bind-pose world matrix
    std::vector<std::pair<bool, int>> draw;      // per draw matrix: (weighted, joint or envelope index)
    std::map<int, std::set<int>> uses;           // position index -> draw matrices (-1 for none)
    bool normals = false;                        // a shape carries GX_NORMAL

    const Mat& world(int j) const { return j >= 0 && size_t(j) < joints.size() ? joints[size_t(j)] : kIdentity; }

    // position index -> joints it is drawn through rigidly (envelopes left out), in index order.
    std::vector<std::pair<int, std::vector<int>>> rigid() const {
        std::vector<std::pair<int, std::vector<int>>> out;
        for (const auto& [i, draws] : uses) {
            if (size_t(i) >= positions.size())
                continue;
            std::vector<int> js;
            for (const int d : draws) {
                int j;
                if (d != -1 && size_t(d) < draw.size()) {
                    if (draw[size_t(d)].first)
                        continue;
                    j = draw[size_t(d)].second;
                } else {
                    j = -1;
                }
                // A model with joints: a position drawn through none of them is left out; without joints, as is.
                if (!joints.empty() && !(j >= 0 && size_t(j) < joints.size()))
                    continue;
                if (joints.empty())
                    j = -1;
                if (std::find(js.begin(), js.end(), j) == js.end())
                    js.push_back(j);
            }
            if (!js.empty())
                out.emplace_back(i, std::move(js));
        }
        return out;
    }
};

inline std::vector<std::array<float, 3>> j3d_positions(const View& d, int64_t at, int64_t size) {
    const int64_t fmt_off = d.u32(at + 8);
    d.need(at + 12, 52);
    std::array<int64_t, 13> offs;
    for (int k = 0; k < 13; ++k)
        offs[size_t(k)] = d.u32(at + 12 + 4 * k);
    int64_t p = at + fmt_off;
    uint32_t cnt, typ;
    while (true) {
        d.need(p, 13);
        const uint32_t attr = d.u32(p);
        cnt = d.u32(p + 4);
        typ = d.u32(p + 8);
        if (attr == 0xFF)
            return {};
        if (attr == kGxPosition)
            break;
        p += 16;
    }
    if (cnt != 1 || typ != 4 || !offs[0])
        return {};
    const int64_t start = offs[0];
    int64_t end = -1;
    for (const int64_t o : offs)
        if (o > start && (end < 0 || o < end))
            end = o;
    if (end < 0)
        end = size;
    const int64_t diff = end - start;
    const int64_t count = diff >= 0 ? diff / 12 : -((-diff + 11) / 12); // floor division
    // d[at + start:at + start + 12 * count], a slice, then unpacked in threes.
    const int64_t lo = std::min(at + start, d.n), hi = std::min(at + start + 12 * count, d.n);
    const int64_t length = std::max<int64_t>(0, hi - lo);
    if (length % 12)
        throw Truncated("iter_unpack requires a buffer of a multiple of 12 bytes");
    std::vector<std::array<float, 3>> out(size_t(length / 12));
    for (size_t i = 0; i < out.size(); ++i)
        for (int c = 0; c < 3; ++c)
            out[i][size_t(c)] = d.f32(lo + 12 * int64_t(i) + 4 * c);
    return out;
}

inline std::vector<Mat> j3d_joints(const View& d, int64_t jnt_at, std::optional<int64_t> inf_at) {
    const int n = d.u16(jnt_at + 8);
    d.need(jnt_at + 0xC, 12);
    const int64_t entries = d.u32(jnt_at + 0xC), remap = d.u32(jnt_at + 0x10);
    std::vector<int64_t> order(static_cast<size_t>(n));
    if (remap) {
        d.need(jnt_at + remap, 2 * int64_t(n));
        for (int i = 0; i < n; ++i)
            order[size_t(i)] = d.u16(jnt_at + remap + 2 * i);
    } else {
        for (int i = 0; i < n; ++i)
            order[size_t(i)] = i;
    }
    std::vector<Mat> local;
    for (int i = 0; i < n; ++i) {
        const int64_t e = jnt_at + entries + 0x40 * order[size_t(i)];
        double s[3], t[3], r[3];
        d.need(e + 4, 12);
        for (int c = 0; c < 3; ++c)
            s[c] = d.f32(e + 4 + 4 * c);
        d.need(e + 0x10, 6);
        for (int c = 0; c < 3; ++c)
            r[c] = double(d.i16(e + 0x10 + 2 * c)) * kPi / 32768.0;
        d.need(e + 0x18, 12);
        for (int c = 0; c < 3; ++c)
            t[c] = d.f32(e + 0x18 + 4 * c);
        local.push_back(euler_matrix(r[0], r[1], r[2], s, t));
    }
    std::vector<int> parents(size_t(n), -1);
    if (inf_at) {
        int64_t p = *inf_at + d.u32(*inf_at + 0x14);
        std::vector<int> stack;
        int last = -1;
        while (true) {
            d.need(p, 4);
            const uint16_t type = d.u16(p), index = d.u16(p + 2);
            p += 4;
            if (type == 0x00)
                break;
            if (type == 0x01) {
                stack.push_back(last);
            } else if (type == 0x02) {
                if (stack.empty()) {
                    last = -1;
                } else {
                    last = stack.back();
                    stack.pop_back();
                }
            } else if (type == 0x10) {
                if (index < n) {
                    int parent = -1;
                    for (auto it = stack.rbegin(); it != stack.rend(); ++it)
                        if (*it >= 0) {
                            parent = *it;
                            break;
                        }
                    parents[index] = parent;
                }
                last = index;
            }
        }
    }
    std::vector<Mat> world;
    for (int i = 0; i < n; ++i) { // INF1 lists parents before children
        const int parent = parents[size_t(i)];
        world.push_back(0 <= parent && parent < i ? mat_mul(world[size_t(parent)], local[size_t(i)]) : local[size_t(i)]);
    }
    return world;
}

inline std::vector<std::pair<bool, int>> j3d_draws(const View& d, int64_t at) {
    const int n = d.u16(at + 8);
    d.need(at + 0xC, 8);
    const int64_t w_at = d.u32(at + 0xC), i_at = d.u32(at + 0x10);
    std::vector<std::pair<bool, int>> out;
    for (int i = 0; i < n; ++i) {
        const bool weighted = d.u8(at + w_at + i) != 0;
        out.emplace_back(weighted, d.u16(at + i_at + 2 * i));
    }
    return out;
}

inline void j3d_uses(const View& d, int64_t at, J3DModel& model) {
    const int n = d.u16(at + 8);
    d.need(at + 0xC, 32);
    const int64_t init_at = d.u32(at + 0xC), desc_at = d.u32(at + 0x18), mtx_tab_at = d.u32(at + 0x1C),
                  dl_at = d.u32(at + 0x20), mtx_init_at = d.u32(at + 0x24), draw_init_at = d.u32(at + 0x28);
    for (int s = 0; s < n; ++s) {
        const int64_t e = at + init_at + 0x28 * s;
        const uint8_t mtx_type = d.u8(e);
        d.need(e + 2, 8);
        const int64_t groups = d.u16(e + 2), desc_off = d.u16(e + 4), mtx_first = d.u16(e + 6),
                      draw_first = d.u16(e + 8);
        std::vector<std::pair<uint32_t, uint32_t>> desc;
        int64_t p = at + desc_at + desc_off;
        while (true) {
            d.need(p, 8);
            const uint32_t attr = d.u32(p), kind = d.u32(p + 4);
            p += 8;
            if (attr == 0xFF)
                break;
            desc.emplace_back(attr, kind);
            if (attr == kGxNormal)
                model.normals = true;
        }
        std::vector<int> slots; // a multi-matrix shape's draw matrix table (0xFFFF keeps the previous entry)
        for (int64_t g = 0; g < groups; ++g) {
            const int64_t mi = at + mtx_init_at + 8 * (mtx_first + g);
            d.need(mi, 8);
            const int use = d.u16(mi);
            const int64_t count = d.u16(mi + 2), first = d.u32(mi + 4);
            if (count) {
                const int64_t tab = at + mtx_tab_at + 2 * first;
                d.need(tab, 2 * count);
                for (int64_t k = 0; k < count; ++k) {
                    const uint16_t v = d.u16(tab + 2 * k);
                    if (size_t(k) >= slots.size())
                        slots.push_back(-1);
                    if (v != 0xFFFF)
                        slots[size_t(k)] = v;
                }
            }
            const int64_t di = at + draw_init_at + 8 * (draw_first + g);
            d.need(di, 8);
            const int64_t dl_size = d.u32(di), dl_off = d.u32(di + 4);
            int64_t q = at + dl_at + dl_off;
            const int64_t end = q + dl_size;
            while (q < end) {
                const uint8_t op = d.u8(q);
                q += 1;
                if (op == 0)
                    continue;
                const int top = op & 0xF8;
                if (top != 0x80 && top != 0x90 && top != 0x98 && top != 0xA0 && top != 0xA8 && top != 0xB0 &&
                    top != 0xB8)
                    break;
                const int nv = d.u16(q);
                q += 2;
                for (int v = 0; v < nv; ++v) {
                    int mtx = -1, pos = -1;
                    for (const auto& [attr, kind] : desc) {
                        if (kind == 1) { // direct
                            if (attr <= 8) {
                                if (attr == 0)
                                    mtx = d.u8(q);
                                q += 1;
                            } else if (attr == 11 || attr == 12) {
                                q += 4;
                            } else {
                                throw FormatError("direct vertex attribute " + std::to_string(attr) + " not supported");
                            }
                        } else if (kind == 2) {
                            if (attr == kGxPosition)
                                pos = d.u8(q);
                            q += 1;
                        } else if (kind == 3) {
                            if (attr == kGxPosition)
                                pos = d.u16(q);
                            q += 2;
                        }
                    }
                    if (pos == -1)
                        continue;
                    int draw;
                    if (mtx_type == 3) {
                        const size_t k = mtx != -1 ? size_t(mtx / 3) : 0;
                        draw = k < slots.size() ? slots[k] : (mtx == -1 ? use : -1);
                    } else {
                        draw = use;
                    }
                    model.uses[pos].insert(draw);
                }
            }
        }
    }
}

inline J3DModel read_j3d(const Bytes& data) {
    const View d{data.data(), int64_t(data.size())};
    if (!(d.is(0, "J3D1") || d.is(0, "J3D2")) || !(d.is(4, "bdl4") || d.is(4, "bmd3") || d.is(4, "bmd2")))
        throw FormatError("not a J3D model");
    std::map<std::string, std::pair<int64_t, int64_t>> found;
    for (int64_t at = 0x20; at + 8 <= d.n;) {
        const int64_t size = d.u32(at + 4);
        if (size < 8)
            break;
        found[std::string(reinterpret_cast<const char*>(d.p + at), 4)] = {at, size};
        at += size;
    }
    if (!found.count("VTX1") || !found.count("SHP1"))
        throw FormatError("no VTX1 or SHP1");
    J3DModel model;
    try {
        if (found.count("JNT1")) {
            std::optional<int64_t> inf;
            if (found.count("INF1"))
                inf = found["INF1"].first;
            model.joints = j3d_joints(d, found["JNT1"].first, inf);
        }
        model.positions = j3d_positions(d, found["VTX1"].first, found["VTX1"].second);
        if (found.count("DRW1"))
            model.draw = j3d_draws(d, found["DRW1"].first);
        j3d_uses(d, found["SHP1"].first, model);
    } catch (const Truncated& ex) {
        throw FormatError(std::string("J3D model: ") + ex.what());
    }
    return model;
}

// ---- Wii U BFRES v3 FMDL models (fmdl.py) ----

// One HD model's vertices in model space (bind pose), one entry per vertex.
struct Placed {
    std::vector<Vec3> positions;
    std::vector<uint8_t> has_normal;
    std::vector<Vec3> normals;
    std::vector<uint8_t> colour_kind; // 0: none; else 0x10 | the components Python holds as ints (bit per component)
    std::vector<std::array<double, 4>> colours;
    std::vector<std::string> materials; // each vertex's shape's material name
};

// A vertex attribute's values: count rows of n components; `ints` when Python holds them as ints.
struct Attr {
    int n = 0;
    bool ints = false;
    size_t count = 0;
    std::vector<double> v;
    double at(size_t row, int c) const { return v[row * size_t(n) + size_t(c)]; }
};

inline int64_t rel(const View& d, int64_t at) {
    const int32_t v = d.i32(at);
    return v ? at + v : 0;
}

inline std::string fres_name(const View& d, int64_t at) {
    if (!at)
        return "";
    const int64_t limit = std::min(at + 1024, d.n);
    int64_t end = -1;
    for (int64_t i = std::max<int64_t>(at, 0); at <= d.n && i < limit; ++i)
        if (d.p[i] == 0) {
            end = i;
            break;
        }
    if (end < 0) {
        char text[64];
        std::snprintf(text, sizeof text, "unterminated name at 0x%llx", static_cast<long long>(at));
        throw FormatError(text);
    }
    std::string out;
    for (int64_t i = at; i < end; ++i) {
        if (d.p[i] < 0x80)
            out += char(d.p[i]);
        else
            out += "\xEF\xBF\xBD"; // decoded as ASCII, errors replaced
    }
    return out;
}

// A BFRES v3 index group's (name, data offset) entries (entry 0 is the tree's root).
inline std::vector<std::pair<std::string, int64_t>> fres_group(const View& d, int64_t at) {
    std::vector<std::pair<std::string, int64_t>> out;
    if (!at)
        return out;
    const int32_t count = d.i32(at + 4);
    for (int64_t i = 1; i <= count; ++i) {
        std::string name = fres_name(d, rel(d, at + 16 + 16 * i));
        out.emplace_back(std::move(name), rel(d, at + 20 + 16 * i));
    }
    return out;
}

// A GX2 attribute format's values; nullopt where fmdl.py raises FormatError (not supported, or past the file).
// The low byte is the layout; 0x100 integer, 0x200 signed, 0x800 integer to float.
inline std::optional<Attr> gx2_attribute(const View& d, uint32_t fmt, uint32_t count, int64_t stride, int64_t start) {
    const uint32_t kind = fmt >> 8, type = fmt & 0xFF;
    const bool is_signed = kind & 0x2, integer = kind & 0x1;
    Attr a;
    a.count = count;
    if (type == 0x0B) { // 10_10_10_2, x in the low bits
        a.n = 3;
        a.ints = integer || (kind & 0x8);
        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t w = d.u32(start + int64_t(i) * stride);
            for (int s = 0; s <= 20; s += 10) {
                int v = int((w >> s) & 0x3FF);
                if (is_signed && (v & 0x200))
                    v -= 0x400;
                a.v.push_back(a.ints ? double(v) : (is_signed ? py_max(v / 511.0, -1.0) : v / 1023.0));
            }
        }
        return a;
    }
    struct Layout {
        uint32_t type;
        int n;
        char code;
        int size;
    };
    static constexpr Layout kLayouts[] = {{0x00, 1, 'B', 1}, {0x02, 1, 'H', 2}, {0x03, 1, 'e', 2}, {0x04, 2, 'B', 1},
                                          {0x05, 1, 'I', 4}, {0x06, 1, 'f', 4}, {0x07, 2, 'H', 2}, {0x08, 2, 'e', 2},
                                          {0x0A, 4, 'B', 1}, {0x0C, 2, 'I', 4}, {0x0D, 2, 'f', 4}, {0x0E, 4, 'H', 2},
                                          {0x0F, 4, 'e', 2}, {0x10, 3, 'I', 4}, {0x11, 3, 'f', 4}, {0x12, 4, 'I', 4},
                                          {0x13, 4, 'f', 4}};
    const Layout* layout = nullptr;
    for (const Layout& l : kLayouts)
        if (l.type == type)
            layout = &l;
    if (!layout)
        return std::nullopt;
    const bool floats = layout->code == 'e' || layout->code == 'f';
    const int n = layout->n, size = layout->size;
    if (count && start + int64_t(count - 1) * stride + int64_t(n) * size > d.n)
        return std::nullopt;
    a.n = n;
    const bool raw = floats || integer || (kind & 0x8);
    a.ints = !floats && raw;
    const double top = double((uint64_t(1) << (8 * size - (is_signed ? 1 : 0))) - 1);
    a.v.reserve(size_t(count) * size_t(n));
    for (uint32_t i = 0; i < count; ++i) {
        const int64_t row = start + int64_t(i) * stride;
        d.need(row, int64_t(n) * size);
        for (int c = 0; c < n; ++c) {
            const int64_t at = row + int64_t(c) * size;
            double v;
            switch (layout->code) {
            case 'B': v = is_signed ? double(static_cast<int8_t>(d.u8(at))) : double(d.u8(at)); break;
            case 'H': v = is_signed ? double(d.i16(at)) : double(d.u16(at)); break;
            case 'I': v = is_signed ? double(d.i32(at)) : double(d.u32(at)); break;
            case 'e': v = half(d.u16(at)); break;
            default: v = double(d.f32(at)); break;
            }
            if (!raw)
                v = is_signed ? py_max(v / top, -1.0) : v / top;
            a.v.push_back(v);
        }
    }
    return a;
}

inline Mat fres_local(const double* scale, const double* rotation, const double* translation, bool euler) {
    if (euler)
        return euler_matrix(rotation[0], rotation[1], rotation[2], scale, translation);
    const double x = rotation[0], y = rotation[1], z = rotation[2], w = rotation[3];
    const double r[3][3] = {{1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)},
                            {2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)},
                            {2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)}};
    const double sx = scale[0], sy = scale[1], sz = scale[2];
    return {r[0][0] * sx, r[0][1] * sy, r[0][2] * sz, translation[0], r[1][0] * sx, r[1][1] * sy,
            r[1][2] * sz, translation[1], r[2][0] * sx, r[2][1] * sy, r[2][2] * sz, translation[2]};
}

// The inverse transpose of a 3x4's 3x3 part.
inline Mat normal_matrix(const Mat& m) {
    const double a = m[0], b = m[1], c = m[2], d_ = m[4], e = m[5], f = m[6], g = m[8], h = m[9], i = m[10];
    const double det = a * (e * i - f * h) - b * (d_ * i - f * g) + c * (d_ * h - e * g);
    if (std::fabs(det) < 1e-12)
        return kIdentity;
    const double k = 1.0 / det;
    return {(e * i - f * h) * k, (f * g - d_ * i) * k, (d_ * h - e * g) * k, 0.0,
            (c * h - b * i) * k, (a * i - c * g) * k, (b * g - a * h) * k, 0.0,
            (b * f - c * e) * k, (c * d_ - a * f) * k, (a * e - b * d_) * k, 0.0};
}

inline Vec3 turn(const Mat& nm, double x, double y, double z) {
    const double tx = nm[0] * x + nm[1] * y + nm[2] * z, ty = nm[4] * x + nm[5] * y + nm[6] * z,
                 tz = nm[8] * x + nm[9] * y + nm[10] * z;
    double ln = std::sqrt(tx * tx + ty * ty + tz * tz);
    if (ln == 0.0)
        ln = 1.0;
    return {tx / ln, ty / ln, tz / ln};
}

inline std::pair<std::vector<Mat>, std::vector<int>> fres_skeleton(const View& d, int64_t at) {
    if (!d.is(at, "FSKL")) {
        char text[48];
        std::snprintf(text, sizeof text, "no FSKL at 0x%llx", static_cast<long long>(at));
        throw FormatError(text);
    }
    const int nbone = d.u16(at + 8), nsmooth = d.u16(at + 0xA), nrigid = d.u16(at + 0xC);
    const int64_t bones_at = rel(d, at + 0x14), m2b_at = rel(d, at + 0x18);
    std::vector<int> m2b;
    if (m2b_at && nsmooth + nrigid) {
        d.need(m2b_at, 2 * int64_t(nsmooth + nrigid));
        for (int i = 0; i < nsmooth + nrigid; ++i)
            m2b.push_back(d.u16(m2b_at + 2 * i));
    }
    std::vector<Mat> local;
    std::vector<int> parent;
    for (int i = 0; i < nbone; ++i) {
        const int64_t b = bones_at + 0x40 * int64_t(i);
        parent.push_back(d.i16(b + 6));
        double s[3], r[4], t[3];
        d.need(b + 0x14, 12);
        for (int c = 0; c < 3; ++c)
            s[c] = d.f32(b + 0x14 + 4 * c);
        d.need(b + 0x20, 16);
        for (int c = 0; c < 4; ++c)
            r[c] = d.f32(b + 0x20 + 4 * c);
        d.need(b + 0x30, 12);
        for (int c = 0; c < 3; ++c)
            t[c] = d.f32(b + 0x30 + 4 * c);
        local.push_back(fres_local(s, r, t, (d.u32(b + 0x10) & 0x7000) == 0x1000));
    }
    std::vector<std::optional<Mat>> world(static_cast<size_t>(nbone));
    const std::function<const Mat&(int, int)> place = [&](int i, int depth) -> const Mat& {
        if (!world[size_t(i)]) {
            if (depth > 256)
                throw FormatError("bone parent loop");
            const int p = parent[size_t(i)];
            if (p < 0 || p >= nbone)
                world[size_t(i)] = local[size_t(i)];
            else {
                const Mat& above = place(p, depth + 1);
                world[size_t(i)] = mat_mul(above, local[size_t(i)]);
            }
        }
        return *world[size_t(i)];
    };
    std::vector<Mat> out;
    for (int i = 0; i < nbone; ++i)
        place(i, 0);
    for (int i = 0; i < nbone; ++i)
        out.push_back(*world[size_t(i)]);
    return {std::move(out), std::move(m2b)};
}

struct VertexBuffer {
    std::optional<Attr> p0, n0, i0, c0; // nullopt: missing or unreadable
};

inline VertexBuffer fres_buffer(const View& d, int64_t at) {
    if (!d.is(at, "FVTX")) {
        char text[48];
        std::snprintf(text, sizeof text, "no FVTX at 0x%llx", static_cast<long long>(at));
        throw FormatError(text);
    }
    const int nattr = d.u8(at + 4), nbuf = d.u8(at + 5);
    const uint32_t count = d.u32(at + 8);
    const int64_t attr_at = rel(d, at + 0x10), buf_at = rel(d, at + 0x18);
    std::vector<std::pair<int64_t, int64_t>> bufs;
    for (int b = 0; b < nbuf; ++b) {
        const int64_t data = rel(d, buf_at + 0x18 * b + 0x14);
        bufs.emplace_back(data, d.u16(buf_at + 0x18 * b + 0xC));
    }
    VertexBuffer out;
    for (int a = 0; a < nattr; ++a) {
        const int64_t aa = attr_at + 0xC * a;
        const std::string name = fres_name(d, rel(d, aa));
        std::optional<Attr>* slot = name == "_p0" ? &out.p0 : name == "_n0" ? &out.n0 : name == "_i0" ? &out.i0
                                  : name == "_c0" ? &out.c0 : nullptr;
        if (!slot)
            continue;
        const size_t which = d.u8(aa + 4);
        if (which >= bufs.size())
            throw Truncated("list index out of range");
        const auto [start, stride] = bufs[which];
        const uint32_t fmt = d.u32(aa + 8);
        *slot = gx2_attribute(d, fmt, count, stride, start + d.u16(aa + 6));
    }
    return out;
}

// Every FMDL of a Wii U BFRES v3, by name (the first of a name), in file order. A shape that can't be placed is
// left out.
inline std::vector<std::pair<std::string, Placed>> read_models(const Bytes& data) {
    const View d{data.data(), int64_t(data.size())};
    if (!d.is(0, "FRES"))
        throw FormatError("not a BFRES file");
    std::vector<std::pair<std::string, Placed>> out;
    try {
        if (d.u32(4) >> 24 != 3)
            throw FormatError("BFRES version not supported (v3 only)");
        for (const auto& entry : fres_group(d, rel(d, 0x20))) {
            const std::string& name = entry.first;
            const int64_t fmdl = entry.second;
            if (!d.is(fmdl, "FMDL")) {
                char text[48];
                std::snprintf(text, sizeof text, "no FMDL at 0x%llx", static_cast<long long>(fmdl));
                throw FormatError(text);
            }
            const auto skeleton = fres_skeleton(d, rel(d, fmdl + 0xC));
            const std::vector<Mat>& world = skeleton.first;
            const std::vector<int>& m2b = skeleton.second;
            const int64_t vtx_at = rel(d, fmdl + 0x10);
            std::vector<VertexBuffer> buffers;
            const int nbuffers = d.u16(fmdl + 0x20);
            for (int v = 0; v < nbuffers; ++v)
                buffers.push_back(fres_buffer(d, vtx_at + 0x20 * v));
            std::vector<std::string> materials;
            for (const auto& material : fres_group(d, rel(d, fmdl + 0x18)))
                materials.push_back(material.first);
            Placed placed;
            for (const auto& [shape_name, at] : fres_group(d, rel(d, fmdl + 0x14))) {
                (void)shape_name;
                if (!d.is(at, "FSHP")) {
                    char text[48];
                    std::snprintf(text, sizeof text, "no FSHP at 0x%llx", static_cast<long long>(at));
                    throw FormatError(text);
                }
                const size_t bone = d.u16(at + 0x10), buf = d.u16(at + 0x12);
                const size_t mi = d.u16(at + 0xE);
                const std::string material = mi < materials.size() ? materials[mi] : std::string();
                const uint8_t skin = d.u8(at + 0x16);
                if (buf >= buffers.size())
                    continue;
                const VertexBuffer& attrs = buffers[buf];
                const std::optional<Attr>&pos = attrs.p0, &nrm = attrs.n0, &col = attrs.c0;
                if (!pos || !pos->count)
                    continue;
                std::vector<const Mat*> mats;
                if (skin == 0) {
                    if (bone >= world.size())
                        continue;
                    mats.assign(pos->count, &world[bone]);
                } else if (skin == 1) {
                    if (!attrs.i0)
                        continue;
                    const Attr& idx = *attrs.i0;
                    std::vector<int64_t> picks;
                    for (size_t k = 0; k < idx.count; ++k) {
                        const double v = idx.at(k, 0);
                        if (!std::isfinite(v))
                            throw FormatError("skin index is not a number");
                        picks.push_back(static_cast<int64_t>(std::trunc(v)));
                    }
                    // Python's list indexing: negative indices count from the end.
                    const auto index = [&](int64_t j) -> size_t {
                        const int64_t size = int64_t(m2b.size());
                        if (j < -size || j >= size)
                            throw Truncated("list index out of range");
                        return size_t(j < 0 ? size + j : j);
                    };
                    bool skip = false;
                    for (const int64_t j : picks)
                        if (j >= int64_t(m2b.size()) || size_t(m2b[index(j)]) >= world.size()) {
                            skip = true;
                            break;
                        }
                    if (skip)
                        continue;
                    for (const int64_t j : picks)
                        mats.push_back(&world[size_t(m2b[index(j)])]);
                } else {
                    mats.assign(pos->count, &kIdentity);
                }
                std::map<const Mat*, Mat> normal_mats;
                const bool has_normals = nrm && nrm->count, has_colours = col && col->count;
                for (size_t k = 0; k < pos->count; ++k) {
                    if (k >= mats.size())
                        throw Truncated("list index out of range");
                    const Mat* m = mats[k];
                    if (pos->n < 3)
                        throw Truncated("tuple index out of range");
                    placed.positions.push_back(apply(*m, pos->at(k, 0), pos->at(k, 1), pos->at(k, 2)));
                    if (has_normals) {
                        auto nm = normal_mats.find(m);
                        if (nm == normal_mats.end())
                            nm = normal_mats.emplace(m, m != &kIdentity ? normal_matrix(*m) : kIdentity).first;
                        if (nrm->n < 3)
                            throw FormatError("a normal with fewer than three components");
                        placed.has_normal.push_back(1);
                        placed.normals.push_back(turn(nm->second, nrm->at(k, 0), nrm->at(k, 1), nrm->at(k, 2)));
                    } else {
                        placed.has_normal.push_back(0);
                        placed.normals.push_back({0.0, 0.0, 0.0});
                    }
                    if (has_colours) {
                        std::array<double, 4> c{1.0, 1.0, 1.0, 1.0};
                        uint8_t kinds = 0x10;
                        for (int ci = 0; ci < std::min(col->n, 4); ++ci) {
                            c[size_t(ci)] = col->at(k, ci);
                            if (col->ints)
                                kinds |= uint8_t(1 << ci);
                        }
                        placed.colour_kind.push_back(kinds);
                        placed.colours.push_back(c);
                    } else {
                        placed.colour_kind.push_back(0);
                        placed.colours.push_back({0.0, 0.0, 0.0, 0.0});
                    }
                    placed.materials.push_back(material);
                }
            }
            if (std::none_of(out.begin(), out.end(), [&](const auto& e) { return e.first == name; }))
                out.emplace_back(name, std::move(placed));
        }
    } catch (const Truncated& ex) {
        throw FormatError(std::string("BFRES model: ") + ex.what());
    }
    return out;
}

// ---- The materials' konst2 colour animations (fmdl.py glow_curves) ----

// A konst2 animation at frames 0 .. count - 1, linear r, g, b.
using Curve = std::vector<std::array<double, 3>>;

struct Segment {
    double start, end;
    std::vector<double> c; // cubic: 4, linear: 2
};

// One animation curve's segments, or nullopt for a kind the glow doesn't use (steps and baked curves).
inline std::optional<std::vector<Segment>> anim_curve(const View& d, int64_t at) {
    const uint16_t flags = d.u16(at), count = d.u16(at + 2);
    const double scale = d.f32(at + 0x10), offset = d.f32(at + 0x14);
    const int frame_kind = flags & 3, key_kind = (flags >> 2) & 3, curve_kind = (flags >> 4) & 7;
    if (curve_kind > 1 || key_kind > 2 || frame_kind > 2 || count == 0)
        return std::nullopt;
    const int per = curve_kind == 0 ? 4 : 2;
    const int64_t frames_at = rel(d, at + 0x1C), keys_at = rel(d, at + 0x20);
    std::vector<double> frames;
    for (int i = 0; i < count; ++i)
        frames.push_back(frame_kind == 0   ? double(d.f32(frames_at + 4 * i))
                         : frame_kind == 1 ? double(d.i16(frames_at + 2 * i)) / 32.0
                                           : double(d.u8(frames_at + i)));
    const int n = count * per;
    std::vector<double> raw;
    for (int k = 0; k < n; ++k)
        raw.push_back(key_kind == 0   ? double(d.f32(keys_at + 4 * k))
                      : key_kind == 1 ? double(d.i16(keys_at + 2 * k))
                                      : double(static_cast<int8_t>(d.u8(keys_at + k))));
    std::vector<Segment> segments;
    for (int i = 0; i < count; ++i) {
        Segment s{frames[size_t(i)], i + 1 < count ? frames[size_t(i) + 1] : frames[size_t(i)], {}};
        for (int j = 0; j < per; ++j)
            s.c.push_back(raw[size_t(i * per + j)] * scale);
        s.c[0] += offset;
        segments.push_back(std::move(s));
    }
    return segments;
}

// A curve at frame f: its segment's polynomial in t (the last key after its end).
inline double anim_value(const std::vector<Segment>& segments, double f) {
    for (const Segment& s : segments)
        if (s.start <= f && f <= s.end && s.end > s.start) {
            const double t = (f - s.start) / (s.end - s.start);
            double v = s.c[0] + s.c[1] * t;
            if (s.c.size() == 4)
                v = v + s.c[2] * t * t + s.c[3] * t * t * t;
            return v;
        }
    return segments.back().c[0];
}

// Material name -> its konst2 colour animation (HD's lava glow colour), as fmdl.py glow_curves: only materials whose
// r, g and b all animate; the first animation that has a material wins; nothing if the file can't be read.
inline std::map<std::string, Curve> glow_curves(const Bytes& data) {
    std::map<std::string, Curve> out;
    const View d{data.data(), int64_t(data.size())};
    if (!d.is(0, "FRES"))
        return out;
    try {
        for (int group = 3; group <= 4; ++group)
            for (const auto& entry : fres_group(d, rel(d, 0x20 + 4 * group))) {
                const int64_t at = entry.second;
                if (!d.is(at, "FSHU"))
                    continue;
                const uint32_t frames = d.u32(at + 0x10);
                const int count = d.u16(at + 0x14);
                const int64_t mats = rel(d, at + 0x2C);
                if (!(0 < frames && frames <= 4096))
                    continue;
                for (int m = 0; m < count; ++m) {
                    const int64_t e = mats + 0x20 * m;
                    const int nparam = d.u16(e);
                    const std::string material = fres_name(d, rel(d, e + 0x10));
                    const int64_t info = rel(d, e + 0x14), curves = rel(d, e + 0x18);
                    if (out.count(material))
                        continue;
                    for (int p = 0; p < nparam; ++p) {
                        const int64_t pe = info + 0x10 * p;
                        const int begin = d.u16(pe), floats = d.u16(pe + 2);
                        if (fres_name(d, rel(d, pe + 0xC)) != "konst2")
                            continue;
                        std::map<uint32_t, std::optional<std::vector<Segment>>> rgb;
                        for (int c = 0; c < floats; ++c) {
                            const int64_t ca = curves + 0x24 * (begin + c);
                            const uint32_t target = d.u32(ca + 4);
                            if ((target == 0 || target == 4 || target == 8) && !rgb.count(target / 4))
                                rgb[target / 4] = anim_curve(d, ca);
                        }
                        if (rgb.size() == 3 && rgb[0] && rgb[1] && rgb[2]) {
                            Curve curve;
                            for (uint32_t f = 0; f < frames; ++f)
                                curve.push_back({anim_value(*rgb[0], double(f)), anim_value(*rgb[1], double(f)),
                                                 anim_value(*rgb[2], double(f))});
                            out.emplace(material, std::move(curve));
                        }
                        break;
                    }
                }
            }
    } catch (const Truncated&) {
        return {};
    } catch (const FormatError&) {
        return {};
    }
    return out;
}

// ---- Matching (normals.py) ----

// At most n items of seq, evenly spaced (the same in both importers).
template <class T> std::vector<T> spaced(const std::vector<T>& seq, size_t n) {
    if (seq.size() <= n)
        return seq;
    const double step = double(seq.size()) / double(n);
    std::vector<T> out;
    out.reserve(n);
    for (size_t k = 0; k < n; ++k)
        out.push_back(seq[size_t(double(k) * step)]);
    return out;
}

// Points in cubes `cell` wide, for radius queries.
class Grid {
  public:
    Grid(const std::vector<Vec3>& points, double cell) : points_(points), inv_(1.0 / cell) {
        for (size_t i = 0; i < points.size(); ++i) {
            const Vec3& p = points[i];
            cells_[{floor_of(p[0] * inv_), floor_of(p[1] * inv_), floor_of(p[2] * inv_)}].push_back(i);
        }
    }

    // Calls f(i) for each point within tol of q, cell by cell; f returns false to stop.
    template <class F> void each_near(const Vec3& q, double tol, F f) const {
        const double t2 = tol * tol;
        const int64_t x0 = floor_of((q[0] - tol) * inv_), x1 = floor_of((q[0] + tol) * inv_);
        const int64_t y0 = floor_of((q[1] - tol) * inv_), y1 = floor_of((q[1] + tol) * inv_);
        const int64_t z0 = floor_of((q[2] - tol) * inv_), z1 = floor_of((q[2] + tol) * inv_);
        for (int64_t cx = x0; cx <= x1; ++cx)
            for (int64_t cy = y0; cy <= y1; ++cy)
                for (int64_t cz = z0; cz <= z1; ++cz) {
                    const auto cell = cells_.find({cx, cy, cz});
                    if (cell == cells_.end())
                        continue;
                    for (const size_t i : cell->second) {
                        const Vec3& p = points_[i];
                        const double dx = p[0] - q[0], dy = p[1] - q[1], dz = p[2] - q[2];
                        if (dx * dx + dy * dy + dz * dz <= t2)
                            if (!f(i))
                                return;
                    }
                }
    }

    std::vector<size_t> near(const Vec3& q, double tol) const {
        std::vector<size_t> out;
        each_near(q, tol, [&](size_t i) {
            out.push_back(i);
            return true;
        });
        std::sort(out.begin(), out.end());
        return out;
    }

    bool any_near(const Vec3& q, double tol) const {
        bool found = false;
        each_near(q, tol, [&](size_t) {
            found = true;
            return false;
        });
        return found;
    }

    // The nearest point within tol's offset from q (point - q); of equally near points, the last by index.
    std::optional<Vec3> nearest(const Vec3& q, double tol) const {
        std::optional<size_t> best;
        double bd = tol * tol;
        for (const size_t i : near(q, tol)) {
            const Vec3& p = points_[i];
            const double d = py_square(p[0] - q[0]) + py_square(p[1] - q[1]) + py_square(p[2] - q[2]);
            if (d <= bd) {
                best = i;
                bd = d;
            }
        }
        if (!best)
            return std::nullopt;
        const Vec3& p = points_[*best];
        return Vec3{p[0] - q[0], p[1] - q[1], p[2] - q[2]};
    }

    const std::vector<Vec3>& points() const { return points_; }

  private:
    using Cell = std::array<int64_t, 3>;
    struct CellHash {
        size_t operator()(const Cell& c) const {
            uint64_t h = uint64_t(c[0]) * 0x9E3779B97F4A7C15ull;
            h ^= uint64_t(c[1]) + 0x7F4A7C159E3779B9ull + (h << 6) + (h >> 2);
            h ^= uint64_t(c[2]) + 0x94D049BB133111EBull + (h << 6) + (h >> 2);
            return size_t(h);
        }
    };
    static int64_t floor_of(double v) { return static_cast<int64_t>(std::floor(v)); }

    const std::vector<Vec3>& points_;
    double inv_;
    std::unordered_map<Cell, std::vector<size_t>, CellHash> cells_;
};

// v to 1/10,000, halves to even, and never -0.
inline double quantize(double v) { return std::nearbyint(v * 10000.0) / 10000.0 + 0.0; }

// The mean of the most common bin of deltas (ties: the first bin to reach the count).
inline Vec3 vote(const std::vector<Vec3>& deltas, double step = 0.1) {
    std::map<std::array<int64_t, 3>, size_t> index;
    std::vector<std::vector<size_t>> bins;
    for (size_t i = 0; i < deltas.size(); ++i) {
        const Vec3& d = deltas[i];
        const std::array<int64_t, 3> key{py_round(d[0] / step), py_round(d[1] / step), py_round(d[2] / step)};
        const auto [it, added] = index.emplace(key, bins.size());
        if (added)
            bins.emplace_back();
        bins[it->second].push_back(i);
    }
    const std::vector<size_t>* best = &bins[0];
    for (const auto& bin : bins)
        if (bin.size() > best->size())
            best = &bin;
    PySum sx, sy, sz;
    for (const size_t i : *best) {
        sx.add(deltas[i][0], false);
        sy.add(deltas[i][1], false);
        sz.add(deltas[i][2], false);
    }
    const double n = double(best->size());
    return {sx.value() / n, sy.value() / n, sz.value() / n};
}

inline std::vector<size_t> hits_of(const Grid& grid, const std::vector<Vec3>& pts, const Vec3& off, double tol) {
    std::vector<size_t> out;
    for (size_t k = 0; k < pts.size(); ++k)
        if (grid.any_near({pts[k][0] + off[0], pts[k][1] + off[1], pts[k][2] + off[2]}, tol))
            out.push_back(k);
    return out;
}

// One joint's offsets (HD - GameCube) in the order tried, each with the points it rescues: round 0 looks for a
// small one (usually none); later rounds vote a large one from the points still missing (a piece HD moved).
inline std::vector<std::pair<Vec3, std::vector<size_t>>> offsets(const std::vector<Vec3>& pts, const Grid& fine,
                                                                 const Grid& coarse, const std::vector<Vec3>& pool,
                                                                 double tol) {
    std::vector<size_t> remaining(pts.size());
    for (size_t k = 0; k < pts.size(); ++k)
        remaining[k] = k;
    std::vector<std::pair<Vec3, std::vector<size_t>>> found;
    for (int round_no = 0; round_no < 4; ++round_no) {
        if (remaining.size() < (round_no == 0 ? 1u : 4u))
            break;
        check_cancel();
        std::vector<Vec3> sub;
        sub.reserve(remaining.size());
        for (const size_t k : remaining)
            sub.push_back(pts[k]);
        const std::vector<Vec3> sample = spaced(sub, 400);
        std::vector<Vec3> cands, deltas;
        for (const Vec3& q : sample)
            if (const auto d = coarse.nearest(q, 1.0))
                deltas.push_back(*d);
        if (!deltas.empty())
            cands.push_back(vote(deltas));
        if (round_no == 0)
            cands.push_back({0.0, 0.0, 0.0});
        size_t best_small = 0;
        for (const Vec3& c : cands)
            best_small = std::max(best_small, hits_of(fine, sub, c, tol).size());
        if (sub.size() - best_small >= 4) {
            std::map<std::array<int64_t, 3>, size_t> votes;
            for (const Vec3& g : spaced(sub, 16))
                for (const Vec3& h : pool)
                    votes[{py_round(h[0] - g[0]), py_round(h[1] - g[1]), py_round(h[2] - g[2])}]++;
            std::vector<std::pair<std::array<int64_t, 3>, size_t>> ranked(votes.begin(), votes.end());
            std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
                return a.second != b.second ? a.second > b.second : a.first < b.first;
            });
            for (size_t r = 0; r < ranked.size() && r < 3; ++r) {
                const auto& [off, n] = ranked[r];
                if (n < 3)
                    break;
                const double ox = double(off[0]), oy = double(off[1]), oz = double(off[2]);
                std::vector<Vec3> refine;
                for (const Vec3& q : sample)
                    if (const auto d = coarse.nearest({q[0] + ox, q[1] + oy, q[2] + oz}, 1.5))
                        refine.push_back(*d);
                if (!refine.empty()) {
                    const Vec3 v = vote(refine);
                    cands.push_back({v[0] + ox, v[1] + oy, v[2] + oz});
                }
            }
        }
        std::optional<Vec3> best;
        std::vector<size_t> best_hits;
        const double nsub = double(sub.size());
        for (const Vec3& c : cands) {
            std::vector<size_t> hits = hits_of(fine, sub, c, tol);
            const double h = double(hits.size());
            bool ok;
            if (std::sqrt(py_square(c[0]) + py_square(c[1]) + py_square(c[2])) > kLarge)
                ok = h >= py_max(8.0, 0.05 * nsub) || h >= py_max(4.0, 0.5 * nsub);
            else
                ok = h >= (round_no == 0 ? 1.0 : py_max(4.0, 0.05 * nsub));
            if (ok && hits.size() > best_hits.size()) {
                best = c;
                best_hits = std::move(hits);
            }
        }
        if (!best)
            break;
        std::vector<size_t> rescued;
        for (const size_t k : best_hits)
            rescued.push_back(remaining[k]);
        const std::set<size_t> taken(rescued.begin(), rescued.end());
        std::vector<size_t> rest;
        for (const size_t k : remaining)
            if (!taken.count(k))
                rest.push_back(k);
        remaining = std::move(rest);
        found.emplace_back(*best, std::move(rescued));
    }
    return found;
}

// (position index, joint) -> the offset that matched it, where longer than kTol (HD moved the piece).
using Moved = std::map<std::pair<int, int>, Vec3>;

// (rows: key -> normals, how many position indices matched) for one GameCube model against one HD model; moved
// (optional) gets the offsets longer than kTol, as normals.py match().
inline std::pair<Rows, size_t> match(const J3DModel& gm, const std::vector<std::pair<int, std::vector<int>>>& rigid,
                                     const Placed& hd, double tol, Moved* moved = nullptr) {
    const Grid fine(hd.positions, tol <= 0.25 ? 0.25 : 1.0);
    const Grid coarse(hd.positions, 1.0);
    const std::vector<Vec3> pool = spaced(hd.positions, 6000);
    std::map<int, std::vector<std::pair<int, Vec3>>> groups; // joint -> (position index, model-space point)
    for (const auto& [i, joints] : rigid)
        for (const int j : joints) {
            const auto& p = gm.positions[size_t(i)];
            groups[j].emplace_back(i, apply(gm.world(j), p[0], p[1], p[2]));
        }
    Rows rows;
    std::set<int> matched;
    for (const auto& [j, items] : groups) {
        const Mat& m = gm.world(j);
        std::vector<Vec3> pts;
        for (const auto& item : items)
            pts.push_back(item.second);
        for (const auto& [off, rescued] : offsets(pts, fine, coarse, pool, tol)) {
            for (const size_t k : rescued) {
                const auto& [i, q] = items[k];
                const std::vector<size_t> hits = fine.near({q[0] + off[0], q[1] + off[1], q[2] + off[2]}, tol);
                if (hits.empty())
                    continue;
                matched.insert(i);
                if (moved && std::sqrt(py_square(off[0]) + py_square(off[1]) + py_square(off[2])) > kTol)
                    moved->emplace(std::make_pair(i, j), off);
                const Key key = key_of(gm.positions[size_t(i)]);
                for (const size_t h : hits) {
                    if (!hd.has_normal[h])
                        continue;
                    const Vec3& n = hd.normals[h];
                    // model space -> joint-local: the transpose of the joint's 3x3
                    const double lx = m[0] * n[0] + m[4] * n[1] + m[8] * n[2];
                    const double ly = m[1] * n[0] + m[5] * n[1] + m[9] * n[2];
                    const double lz = m[2] * n[0] + m[6] * n[1] + m[10] * n[2];
                    const double ln = std::sqrt(lx * lx + ly * ly + lz * lz);
                    if (ln < 1e-9)
                        continue;
                    const Vec3 t{quantize(lx / ln), quantize(ly / ln), quantize(lz / ln)};
                    std::vector<Vec3>& list = rows[key];
                    if (std::find(list.begin(), list.end(), t) == list.end())
                        list.push_back(t);
                }
            }
        }
    }
    return {std::move(rows), matched.size()};
}

// key -> HD's colour averaged over its vertices within kTol of the position (no offset).
inline std::map<Key, std::array<double, 4>> colours(const J3DModel& gm,
                                                    const std::vector<std::pair<int, std::vector<int>>>& rigid,
                                                    const Placed& hd) {
    const Grid grid(hd.positions, 0.25);
    std::map<Key, std::array<double, 4>> out;
    for (const auto& [i, joints] : rigid) {
        const auto& p = gm.positions[size_t(i)];
        const Vec3 q = apply(gm.world(joints[0]), p[0], p[1], p[2]);
        std::vector<size_t> hits;
        for (const size_t h : grid.near(q, kTol))
            if (hd.colour_kind[h])
                hits.push_back(h);
        if (hits.empty())
            continue;
        std::array<double, 4> c;
        for (int ci = 0; ci < 4; ++ci) {
            PySum sum;
            for (const size_t h : hits)
                sum.add(hd.colours[h][size_t(ci)], (hd.colour_kind[h] >> ci) & 1);
            c[size_t(ci)] = sum.value() / double(hits.size());
        }
        out[key_of(p)] = c;
    }
    return out;
}

// A stage's colour at a position (normals.py colours() with curves): r, g, b, a, glow, and the glow's curve.
struct StageColour {
    std::array<double, 5> c{};
    std::shared_ptr<const Curve> curve; // null: none
};
using CurveMap = std::map<std::string, std::shared_ptr<const Curve>>;
using StageColours = std::map<Key, StageColour>;

// The materials whose vertices all have colours with green 0: HD's lava rooms' (glow, 0, shade) masks.
inline std::set<std::string> blue_shades(const Placed& hd) {
    std::set<std::string> seen, other;
    for (size_t v = 0; v < hd.materials.size(); ++v) {
        if (hd.colour_kind[v] && hd.colours[v][1] == 0.0)
            seen.insert(hd.materials[v]);
        else
            other.insert(hd.materials[v]);
    }
    for (const std::string& m : other)
        seen.erase(m);
    return seen;
}

// The materials with a vertex whose green is 0.1 or more: shades, not (glow, 0, shade) masks (normals.py
// grey_shades()).
inline std::set<std::string> grey_shades(const Placed& hd) {
    std::set<std::string> out;
    for (size_t v = 0; v < hd.materials.size(); ++v)
        if (hd.colour_kind[v] && hd.colours[v][1] >= 0.1)
            out.insert(hd.materials[v]);
    return out;
}

// key -> a stage's HD colour as its room shaders take it (normals.py colours() with curves): a mask material's
// vertex (b, b, b, a), with its glow (red squared) where the material has a konst2 animation; another material with
// the animation and grey shades (Dragon Roost Cavern's, its mask in alpha) its colour with (1 - a) squared as its
// glow; averaged over the vertices within kTol, the curve the hits' (none if none, or if they differ).
inline StageColours stage_colours(const J3DModel& gm, const std::vector<std::pair<int, std::vector<int>>>& rigid,
                                  const Placed& hd, const CurveMap& curves, const Moved& moved) {
    const Grid grid(hd.positions, 0.25);
    const std::set<std::string> blue = blue_shades(hd);
    const std::set<std::string> grey = curves.empty() ? std::set<std::string>{} : grey_shades(hd);
    StageColours out;
    for (const auto& [i, joints] : rigid) {
        const auto& p = gm.positions[size_t(i)];
        Vec3 q = apply(gm.world(joints[0]), p[0], p[1], p[2]);
        if (const auto off = moved.find({i, joints[0]}); off != moved.end())
            q = {q[0] + off->second[0], q[1] + off->second[1], q[2] + off->second[2]};
        std::vector<size_t> hits;
        for (const size_t h : grid.near(q, kTol))
            if (hd.colour_kind[h])
                hits.push_back(h);
        if (hits.empty())
            continue;
        std::vector<std::array<double, 5>> cols;
        std::vector<uint8_t> kinds; // bit per component: Python holds it as an int
        std::vector<std::shared_ptr<const Curve>> glows;
        for (const size_t h : hits) {
            const auto& c = hd.colours[h];
            const uint8_t k = hd.colour_kind[h];
            if (blue.count(hd.materials[h])) {
                const auto it = curves.find(hd.materials[h]);
                const std::shared_ptr<const Curve> curve = it != curves.end() ? it->second : nullptr;
                const uint8_t b = (k >> 2) & 1, a = (k >> 3) & 1, r = k & 1;
                if (curve) {
                    cols.push_back({c[2], c[2], c[2], c[3], c[0] * c[0]});
                    kinds.push_back(uint8_t(b | b << 1 | b << 2 | a << 3 | r << 4));
                } else {
                    cols.push_back({c[2], c[2], c[2], c[3], 0.0});
                    kinds.push_back(uint8_t(b | b << 1 | b << 2 | a << 3 | 1 << 4));
                }
                glows.push_back(curve);
            } else if (const auto it = curves.find(hd.materials[h]);
                       it != curves.end() && it->second && grey.count(hd.materials[h])) {
                // Another konst2 material (Dragon Roost Cavern's): its colour, and (1 - a) squared as its glow.
                const uint8_t a = (k >> 3) & 1;
                cols.push_back({c[0], c[1], c[2], c[3], (1.0 - c[3]) * (1.0 - c[3])});
                kinds.push_back(uint8_t((k & 0xF) | a << 4));
                glows.push_back(it->second);
            } else {
                cols.push_back({c[0], c[1], c[2], c[3], 0.0});
                kinds.push_back(uint8_t((k & 0xF) | 1 << 4));
                glows.push_back(nullptr);
            }
        }
        std::shared_ptr<const Curve> curve;
        bool same = true;
        for (const auto& g : glows)
            if (g) {
                if (!curve)
                    curve = g;
                else if (*g != *curve)
                    same = false;
            }
        if (!same)
            curve = nullptr;
        StageColour sc;
        for (int ci = 0; ci < 5; ++ci) {
            PySum sum;
            for (size_t j = 0; j < cols.size(); ++j)
                sum.add(cols[j][size_t(ci)], (kinds[j] >> ci) & 1);
            sc.c[size_t(ci)] = sum.value() / double(cols.size());
        }
        if (curve)
            sc.curve = curve;
        else
            sc.c[4] = 0.0;
        out[key_of(p)] = sc;
    }
    return out;
}

inline void merge(Rows& into, const Rows& rows) {
    for (const auto& [key, normals] : rows) {
        std::vector<Vec3>& list = into[key];
        for (const Vec3& n : normals)
            if (std::find(list.begin(), list.end(), n) == list.end())
                list.push_back(n);
    }
}

inline Bytes normals_table(const Rows& rows) {
    size_t count = 0;
    for (const auto& entry : rows)
        count += entry.second.size();
    Bytes out{'B', 'W', 'H', 'N'};
    put32(out, 1);
    put32(out, uint32_t(count));
    for (const auto& [key, normals] : rows)
        for (const Vec3& n : normals) {
            out.insert(out.end(), key.begin(), key.end());
            for (const double v : n)
                put_float(out, v);
        }
    return out;
}

inline Bytes colours_table(const std::map<Key, std::array<double, 4>>& cols) {
    Bytes out{'B', 'W', 'H', 'C'};
    put32(out, 1);
    put32(out, uint32_t(cols.size()));
    for (const auto& [key, c] : cols) {
        out.insert(out.end(), key.begin(), key.end());
        for (const double v : c)
            put_float(out, v);
    }
    return out;
}

// A stage's table: version 1 without glow, else version 2 with its curves (normals.py colours_table()).
inline Bytes colours_table(const StageColours& cols) {
    std::vector<const Curve*> curves;
    const auto index_of = [&](const Curve& c) -> int {
        for (size_t i = 0; i < curves.size(); ++i)
            if (*curves[i] == c)
                return int(i);
        return -1;
    };
    for (const auto& entry : cols)
        if (entry.second.curve && index_of(*entry.second.curve) < 0)
            curves.push_back(entry.second.curve.get());
    if (curves.empty()) {
        std::map<Key, std::array<double, 4>> plain;
        for (const auto& [key, sc] : cols)
            plain.emplace(key, std::array<double, 4>{sc.c[0], sc.c[1], sc.c[2], sc.c[3]});
        return colours_table(plain);
    }
    Bytes out{'B', 'W', 'H', 'C'};
    put32(out, 2);
    put32(out, uint32_t(cols.size()));
    put32(out, uint32_t(curves.size()));
    for (const Curve* curve : curves) {
        put32(out, uint32_t(curve->size()));
        for (const auto& rgb : *curve)
            for (const double v : rgb)
                put_float(out, v);
    }
    for (const auto& [key, sc] : cols) {
        out.insert(out.end(), key.begin(), key.end());
        for (const double v : sc.c)
            put_float(out, v);
        put_float(out, sc.curve ? double(index_of(*sc.curve)) : -1.0);
    }
    return out;
}

inline bool ends_with_lower(const std::string& lowered, const char* suffix) {
    const size_t n = std::strlen(suffix);
    return lowered.size() >= n && lowered.compare(lowered.size() - n, n, suffix) == 0;
}

// res/Stage/STAGE/RoomN.arc or Stage.arc (any case): STAGE and "RoomN" (no leading zeros) or "Stage".
inline bool stage_arc(const std::string& path, std::string& stage, std::string& arc) {
    const std::string l = lower(path);
    if (l.rfind("res/stage/", 0) != 0)
        return false;
    const size_t slash = path.find('/', 10);
    if (slash == std::string::npos || slash == 10 || path.find('/', slash + 1) != std::string::npos)
        return false;
    const std::string file = l.substr(slash + 1);
    if (!ends_with_lower(file, ".arc"))
        return false;
    const std::string base = file.substr(0, file.size() - 4);
    if (base == "stage") {
        arc = "Stage";
    } else {
        if (base.size() <= 4 || base.compare(0, 4, "room") != 0)
            return false;
        std::string digits = base.substr(4);
        for (const char c : digits)
            if (c < '0' || c > '9')
                return false;
        const size_t first = digits.find_first_not_of('0');
        arc = "Room" + (first == std::string::npos ? std::string("0") : digits.substr(first));
    }
    stage = path.substr(10, slash - 10);
    return true;
}

// res/Object/NAME.arc (any case): NAME in lower case.
inline bool object_arc(const std::string& path, std::string& name) {
    const std::string l = lower(path);
    if (l.rfind("res/object/", 0) != 0 || path.find('/', 11) != std::string::npos || !ends_with_lower(l, ".arc") ||
        l.size() <= 11 + 4)
        return false;
    name = l.substr(11, l.size() - 11 - 4);
    return true;
}

// The last NAME of a "NAME.szs/" path part, as Python's re.finditer(r"(?:^|/)([^/]+)\.szs/") finds them.
inline std::optional<std::string> container_of(const std::string& path) {
    std::optional<std::string> last;
    const auto at = [&](size_t g, size_t& next) {
        const size_t slash = path.find('/', g);
        if (slash == std::string::npos || slash - g < 5 || path.compare(slash - 4, 4, ".szs") != 0)
            return false;
        last = path.substr(g, slash - 4 - g);
        next = slash + 1;
        return true;
    };
    for (size_t s = 0; s <= path.size();) {
        size_t next = 0;
        if ((s == 0 && at(0, next)) || (s < path.size() && path[s] == '/' && at(s + 1, next)))
            s = next;
        else
            s++;
    }
    return last;
}

} // namespace normals_detail

// Collects the tables while the importer reads HD's archives once: feed() each archive member with its path,
// flush() after each HD archive, then tables(). The GameCube's arcs are read from its disc as their HD files turn up.
class HdNormals {
  public:
    struct Tables {
        std::vector<std::pair<std::string, Bytes>> files; // relative path, bytes
        std::string report;                                // the report's "hd-normals" entry (JSON)
        std::vector<ImportError> errors;                   // for the report's "unsupported" list
        size_t stages = 0, object_models = 0;              // what the tables cover, for the import's summary
    };

    explicit HdNormals(const std::string& iso_path) : disc_(iso_path, std::ios::binary) {
        if (!disc_)
            throw FormatError("cannot open the GameCube disc " + iso_path);
        const Bytes header = read(0, 0x500);
        const Reader h(header);
        const uint32_t fst_offset = h.u32(0x424), fst_size = h.u32(0x428);
        if (fst_size > 16u * 1024 * 1024)
            throw FormatError("invalid GameCube file table");
        const Bytes fst = read(fst_offset, fst_size);
        const Reader f(fst);
        const size_t entries = f.u32(8), strings = entries * 12;
        const std::function<void(size_t, size_t, const std::string&)> walk = [&](size_t i, size_t end,
                                                                                 const std::string& prefix) {
            while (i < end) {
                const size_t e = i * 12;
                const uint8_t flags = f.u8(e);
                const uint32_t name_off = f.u32(e) & 0xFFFFFF, off = f.u32(e + 4), size = f.u32(e + 8);
                const std::string name = f.string(strings + name_off);
                if (flags == 0) {
                    add(Entry{prefix + name, off, size});
                    i++;
                } else if (flags == 1) {
                    if (size <= i || size > end)
                        throw FormatError("invalid GameCube file table");
                    walk(i + 1, size, prefix + name + "/");
                    i = size;
                } else {
                    i++;
                }
            }
        };
        walk(1, entries, "");
    }

    void feed(const std::string& path, const Bytes& data) {
        const std::optional<std::string> name = wanted(path);
        if (!name || data.size() < 4 || std::memcmp(data.data(), "FRES", 4) != 0)
            return;
        if (!container_ || *container_ != *name) {
            flush();
            container_ = *name;
        }
        try {
            auto models = normals_detail::read_models(data);
            normals_detail::CurveMap curves;
            if (std::any_of(models.begin(), models.end(), [&](const auto& m) { return !model_index_.count(m.first); }))
                for (auto& [material, curve] : normals_detail::glow_curves(data))
                    curves.emplace(material, std::make_shared<const normals_detail::Curve>(std::move(curve)));
            for (auto& [model, placed] : models)
                if (!model_index_.count(model)) {
                    model_index_.emplace(model, models_.size());
                    models_.emplace_back(model, std::move(placed));
                    model_curves_.emplace(model, curves);
                }
        } catch (const FormatError& ex) {
            errors_.push_back({path, ex.what()});
        }
    }

    void flush() {
        if (!container_)
            return;
        const std::string name = *container_;
        std::vector<std::pair<std::string, normals_detail::Placed>> models = std::move(models_);
        std::map<std::string, size_t> index = std::move(model_index_);
        curves_ = std::move(model_curves_);
        container_.reset();
        models_.clear();
        model_index_.clear();
        model_curves_.clear();
        done_.insert(name);
        const auto stage = stage_arcs_.find(name);
        const Entry& entry = stage != stage_arcs_.end() ? stage->second.second : object_arcs_.at(lower(name));
        try {
            if (entry.size > kMaxFile)
                throw FormatError("GameCube resource exceeds size limit");
            const Bytes arc = read(entry.offset, entry.size);
            leaves(arc, entry.path, [&](const std::string& path, const Bytes& leaf) {
                const std::string l = lower(path);
                if (!normals_detail::ends_with_lower(l, ".bdl") && !normals_detail::ends_with_lower(l, ".bmd"))
                    return;
                check_cancel();
                const std::string file = base_name(path);
                const size_t dot = file.rfind('.');
                const std::string model = dot == std::string::npos ? file : file.substr(0, dot);
                if (stage != stage_arcs_.end()) {
                    std::optional<normals_detail::Rows> rows = stage_model(path, model, leaf, models, index);
                    if (!rows) {
                        stage_models_skipped_++;
                    } else {
                        stage_models_++;
                        normals_detail::merge(stages_[stage->second.first], *rows);
                        merge_colours(stage_colours_[stage->second.first], stage_clashes_[stage->second.first],
                                      model_colours_, true);
                    }
                } else if (!object_model(path, model, leaf, models)) {
                    object_models_skipped_++;
                }
            });
        } catch (const FormatError& ex) {
            errors_.push_back({entry.path, ex.what()});
        }
    }

    // Models matched so far (the stages' and the objects'), for the import's progress.
    size_t models_matched() const { return stage_models_ + object_models_; }

    // At most how many tables tables() writes (each stage with GameCube arcs a normals and a colours table, and
    // the objects' two), known from the start: the import's progress counts them with the HD files.
    size_t tables_most() const {
        std::set<std::string> stages;
        for (const auto& arc : stage_arcs_)
            stages.insert(arc.second.first);
        return 2 * stages.size() + 2;
    }

    // With `each`, every table goes to it as soon as it is made, in the order of `files` (left empty), after
    // `counted` has been told how many there are: the importer writes and counts them one by one.
    Tables tables(const std::function<void(size_t)>& counted = {},
                  const std::function<void(const std::string&, const Bytes&)>& each = {}) {
        using namespace normals_detail;
        flush();
        Tables out;
        std::vector<std::pair<std::string, std::function<Bytes()>>> todo; // relative path, its table
        size_t stages = 0;
        for (const auto& [stage, rows] : stages_)
            if (!rows.empty()) {
                stages++;
                todo.emplace_back(std::string(kHdNormalsFolder) + "/" + stage + ".bin",
                                  [&rows = rows] { return normals_table(rows); });
            }
        std::list<StageColours> kept_tables;
        for (const auto& [stage, table] : stage_colours_) {
            StageColours kept;
            const std::set<Key>& clashes = stage_clashes_[stage];
            for (const auto& [key, c] : table)
                if (!clashes.count(key))
                    kept.emplace(key, c);
            const auto rows = stages_.find(stage);
            if (!kept.empty() && rows != stages_.end() && !rows->second.empty()) {
                const StageColours& k = kept_tables.emplace_back(std::move(kept));
                todo.emplace_back(std::string(kHdColoursFolder) + "/" + stage + ".bin",
                                  [&k] { return colours_table(k); });
            }
        }
        std::map<Key, std::array<double, 4>> cols;
        for (const auto& [key, c] : object_colours_)
            if (!clashes_.count(key))
                cols.emplace(key, c);
        if (!objects_.empty())
            todo.emplace_back(std::string(kHdNormalsFolder) + "/" + kHdObjectsFile,
                              [this] { return normals_table(objects_); });
        if (!cols.empty())
            todo.emplace_back(std::string(kHdColoursFolder) + "/" + kHdObjectsFile,
                              [&cols] { return colours_table(cols); });
        if (counted)
            counted(todo.size());
        for (auto& [name, make] : todo) {
            Bytes table = make();
            if (each)
                each(name, table);
            else
                out.files.emplace_back(name, std::move(table));
        }
        out.report = "{\"stage_models\": " + std::to_string(stage_models_) +
                     ", \"stage_models_skipped\": " + std::to_string(stage_models_skipped_) +
                     ", \"object_models\": " + std::to_string(object_models_) +
                     ", \"object_models_skipped\": " + std::to_string(object_models_skipped_) +
                     ", \"stages\": " + std::to_string(stages) +
                     ", \"object_positions\": " + std::to_string(objects_.size()) +
                     ", \"object_colours\": " + std::to_string(cols.size()) +
                     ", \"object_colours_left_out\": " + std::to_string(clashes_.size()) + "}";
        out.errors = errors_;
        out.stages = stages;
        out.object_models = object_models_;
        return out;
    }

  private:
    struct Entry {
        std::string path;
        uint32_t offset, size;
    };
    using Models = std::vector<std::pair<std::string, normals_detail::Placed>>;

    Bytes read(uint64_t offset, size_t size) {
        Bytes out(size);
        disc_.clear();
        disc_.seekg(std::streamoff(offset));
        disc_.read(reinterpret_cast<char*>(out.data()), std::streamsize(size));
        if (size_t(disc_.gcount()) != size)
            throw FormatError("truncated GameCube disc");
        return out;
    }

    void add(const Entry& e) {
        std::string stage, arc, object;
        if (normals_detail::stage_arc(e.path, stage, arc))
            stage_arcs_[stage + "_" + arc] = {stage, e};
        else if (normals_detail::object_arc(e.path, object))
            object_arcs_.emplace(object, e);
    }

    std::optional<std::string> wanted(const std::string& path) const {
        std::optional<std::string> name = normals_detail::container_of(path);
        if (!name || done_.count(*name) || (!stage_arcs_.count(*name) && !object_arcs_.count(lower(*name))))
            return std::nullopt;
        return name;
    }

    std::optional<normals_detail::J3DModel> read_gc(const std::string& path, const Bytes& leaf) {
        try {
            normals_detail::J3DModel gm = normals_detail::read_j3d(leaf);
            if (gm.positions.empty())
                return std::nullopt;
            return gm;
        } catch (const FormatError& ex) {
            errors_.push_back({path, ex.what()});
            return std::nullopt;
        }
    }

    std::optional<normals_detail::Rows> stage_model(const std::string& path, const std::string& model,
                                                    const Bytes& leaf, const Models& models,
                                                    const std::map<std::string, size_t>& index) {
        using namespace normals_detail;
        const auto it = index.find(model);
        if (model.rfind("vr_", 0) == 0 || it == index.end())
            return std::nullopt;
        const Placed& hd = models[it->second].second;
        if (hd.positions.empty() || std::find(hd.has_normal.begin(), hd.has_normal.end(), 1) == hd.has_normal.end())
            return std::nullopt;
        const std::optional<J3DModel> gm = read_gc(path, leaf);
        if (!gm)
            return std::nullopt;
        const auto rigid = gm->rigid();
        if (rigid.empty())
            return std::nullopt;
        Moved moved;
        auto [rows, matched] = match(*gm, rigid, hd, kTol, &moved);
        if (!matched) {
            moved.clear();
            std::tie(rows, matched) = match(*gm, rigid, hd, kTolFallback, &moved);
        }
        if (double(matched) < kStageShare * double(rigid.size()))
            return std::nullopt;
        const auto curves = curves_.find(model);
        model_colours_ = stage_colours(*gm, rigid, hd, curves != curves_.end() ? curves->second : CurveMap{}, moved);
        return std::move(rows);
    }

    // A vertex colour HD uses as a colour: not a mask (no green under 0.02 beside red or blue over 0.1, as the lava
    // rooms' (glow, 0, shade) before stage_colours() turns those to greys) and nothing below 0.
    static bool shade_colour(const std::array<double, 5>& c) {
        return std::min(std::min(c[0], c[1]), c[2]) >= 0.0 &&
               !(c[1] < 0.02 && normals_detail::py_max(c[0], c[2]) > 0.1);
    }

    // A stage model's colours into its table (normals.py _merge_colours(stage=True)): the first kept, only those
    // HD uses as colours (shade_colour), a position whose colours or glow differ by over 0.02, or whose glow curves
    // differ, left out.
    static void merge_colours(normals_detail::StageColours& into, std::set<normals_detail::Key>& clashes,
                              const normals_detail::StageColours& cols, bool) {
        for (const auto& [key, sc] : cols) {
            const auto& c = sc.c;
            if (!shade_colour(c))
                continue;
            const auto old = into.find(key);
            if (old == into.end()) {
                into.emplace(key, sc);
                continue;
            }
            double most = std::fabs(old->second.c[0] - c[0]);
            for (size_t k = 1; k < 5; ++k)
                most = normals_detail::py_max(most, std::fabs(old->second.c[k] - c[k]));
            const bool same_curve = (!old->second.curve && !sc.curve) ||
                                    (old->second.curve && sc.curve && *old->second.curve == *sc.curve);
            if (most > 0.02 || !same_curve)
                clashes.insert(key);
        }
    }

    // A model's colours into a table: the first kept, a position whose colours differ by over 0.02 left out.
    static void merge_colours(std::map<normals_detail::Key, std::array<double, 4>>& into,
                              std::set<normals_detail::Key>& clashes,
                              const std::map<normals_detail::Key, std::array<double, 4>>& cols) {
        for (const auto& [key, c] : cols) {
            const auto old = into.find(key);
            if (old == into.end()) {
                into.emplace(key, c);
                continue;
            }
            double most = std::fabs(old->second[0] - c[0]);
            for (size_t k = 1; k < 4; ++k)
                most = normals_detail::py_max(most, std::fabs(old->second[k] - c[k]));
            if (most > 0.02)
                clashes.insert(key);
        }
    }

    bool object_model(const std::string& path, const std::string& model, const Bytes& leaf, const Models& models) {
        using namespace normals_detail;
        const Placed* hd = nullptr;
        const std::string wanted_name = lower(model);
        for (const auto& [name, placed] : models)
            if (lower(name) == wanted_name) {
                hd = &placed;
                break;
            }
        if (!hd || hd->positions.empty())
            return false;
        const std::optional<J3DModel> gm = read_gc(path, leaf);
        if (!gm || gm->normals)
            return false;
        const auto rigid = gm->rigid();
        if (rigid.empty())
            return false;
        auto [rows, matched] = match(*gm, rigid, *hd, kTol);
        if (double(matched) < kObjectShare * double(rigid.size()))
            return false;
        merge(objects_, rows);
        merge_colours(object_colours_, clashes_, colours(*gm, rigid, *hd));
        object_models_++;
        return true;
    }

    std::ifstream disc_;
    std::map<std::string, std::pair<std::string, Entry>> stage_arcs_; // "<stage>_<RoomN|Stage>" -> (stage, arc)
    std::map<std::string, Entry> object_arcs_;                        // lower-case name -> arc (the first)
    std::set<std::string> done_;
    std::optional<std::string> container_;
    Models models_;
    std::map<std::string, size_t> model_index_;
    std::map<std::string, normals_detail::Rows> stages_; // stage -> key -> normals
    normals_detail::Rows objects_;
    std::map<normals_detail::Key, std::array<double, 4>> object_colours_;
    std::set<normals_detail::Key> clashes_;
    std::map<std::string, normals_detail::StageColours> stage_colours_; // stage -> colours
    std::map<std::string, std::set<normals_detail::Key>> stage_clashes_;
    normals_detail::StageColours model_colours_; // the last stage model's (stage_model)
    std::map<std::string, normals_detail::CurveMap> model_curves_, curves_; // model -> its file's konst2 animations
    size_t stage_models_ = 0, stage_models_skipped_ = 0, object_models_ = 0, object_models_skipped_ = 0;
    std::vector<ImportError> errors_;
};

} // namespace wwhd

#if defined(__clang__)
#pragma float_control(pop)
#elif defined(__GNUC__)
#pragma GCC pop_options
#elif defined(_MSC_VER)
#pragma float_control(pop)
#endif
