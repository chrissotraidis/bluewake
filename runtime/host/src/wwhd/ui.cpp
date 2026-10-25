// The generated HUD and title art of the built-in Wind Waker HD importer (ui.h).
// FFNT section layouts as documented by 3dstools (bffnt.py) and used by
// scripts/wwhd/font.py.
#include "cancel.h"
#include "ui.h"

#include "addrlib.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>

namespace wwhd {

namespace {

int py_round(double v) { return int(std::nearbyint(v)); } // Python round(): ties to even
int floordiv(int a, int b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }

} // namespace

BitmapFont::BitmapFont(const Bytes& data) : data_(data) {
    const Reader r(data_);
    if (data_.size() < 24 || std::memcmp(r.take(0, 4), "FFNT", 4) != 0 || r.u16(4) != 0xFEFF || r.u16(6) != 20 ||
        r.u32(8) != 0x03000000 || r.u32(12) != data_.size() || data_.size() > kMaxFile)
        throw FormatError("unsupported Wii U bitmap font");
    if (std::memcmp(r.take(20, 4), "FINF", 4) != 0 || r.u32(24) != 32)
        throw FormatError("invalid font info section");
    const size_t glyphs = r.u32(40), widths = r.u32(44), mapping_at = r.u32(48);
    if (glyphs < 8)
        throw FormatError("invalid font glyph surface");
    const size_t at = glyphs - 8;
    const bool tglp = std::memcmp(r.take(at, 4), "TGLP", 4) == 0;
    const uint32_t size = r.u32(at + 4);
    cell_w_ = r.u8(at + 8);
    cell_h_ = r.u8(at + 9);
    sheet_count_ = r.u8(at + 10);
    sheet_size_ = r.u32(at + 12);
    const int fmt = r.u16(at + 18);
    cols_ = r.u16(at + 20);
    rows_ = r.u16(at + 22);
    width_ = r.u16(at + 24);
    height_ = r.u16(at + 26);
    offset_ = r.u32(at + 28);
    if (!tglp || !cell_w_ || !cell_h_ || !(sheet_count_ > 0 && sheet_count_ <= 16) || !cols_ || !rows_ ||
        !(width_ > 0 && width_ <= 4096) || !(height_ > 0 && height_ <= 4096) || cols_ * (cell_w_ + 1) > width_ ||
        rows_ * (cell_h_ + 1) > height_ || (fmt != 12 && fmt != 14))
        throw FormatError("invalid font glyph surface");
    r.take(at, size);
    r.take(offset_, size_t(sheet_count_) * sheet_size_);
    surface_ = fmt == 12 ? 0x34 : 0x1A;
    components_ = fmt == 12 ? std::vector<uint8_t>{5, 5, 5, 0} : std::vector<uint8_t>{0, 1, 2, 3};
    const addrlib::SurfaceInfo info = addrlib::getSurfaceInfo(surface_, width_, height_, 1, 1, 4, 0, 0);
    if (int64_t(sheet_size_) != info.surfSize)
        throw FormatError("unexpected font sheet stride");
    tile_mode_ = info.tileMode;
    pitch_ = info.pitch;
    surf_height_ = info.height;
    bpp_ = info.bpp;

    const auto chain = [&](size_t pointer, const char* magic, size_t link_offset, const auto& visit) {
        std::set<size_t> seen;
        while (pointer) {
            check_cancel();
            if (pointer < 8)
                throw FormatError("invalid font section pointer");
            const size_t section = pointer - 8;
            if (seen.count(section) || seen.size() >= 256)
                throw FormatError("cyclic font section chain");
            seen.insert(section);
            if (std::memcmp(r.take(section, 4), magic, 4) != 0)
                throw FormatError("invalid font section pointer");
            const size_t length = r.u32(section + 4);
            if (length < link_offset + 4)
                throw FormatError("invalid font section size");
            r.take(section, length);
            visit(section, length);
            pointer = r.u32(section + link_offset);
        }
    };
    chain(widths, "CWDH", 12, [&](size_t s, size_t length) {
        const int first = r.u16(s + 8), last = r.u16(s + 10);
        if (last < first || 16 + size_t(last - first + 1) * 3 > length)
            throw FormatError("invalid font width range");
        for (int i = first; i <= last; i++) {
            const size_t w = s + 16 + size_t(i - first) * 3;
            widths_[i] = {int8_t(r.u8(w)), r.u8(w + 1), r.u8(w + 2)};
        }
    });
    chain(mapping_at, "CMAP", 16, [&](size_t s, size_t length) {
        const uint32_t first = r.u16(s + 8), last = r.u16(s + 10), method = r.u16(s + 12);
        if (last < first)
            throw FormatError("invalid font character range");
        if (method == 0) {
            if (length < 22)
                throw FormatError("truncated direct font mapping");
            const int start = r.u16(s + 20);
            for (uint32_t c = first; c <= last; c++)
                mapping[c] = int(start + c - first);
        } else if (method == 1) {
            if (20 + size_t(last - first + 1) * 2 > length)
                throw FormatError("truncated table font mapping");
            for (uint32_t c = first; c <= last; c++) {
                const int i = r.u16(s + 20 + size_t(c - first) * 2);
                if (i != 65535)
                    mapping[c] = i;
            }
        } else if (method == 2) {
            const size_t count = r.u16(s + 20);
            if (22 + count * 4 > length)
                throw FormatError("truncated sparse font mapping");
            for (size_t i = 0; i < count; i++)
                mapping[r.u16(s + 22 + i * 4)] = r.u16(s + 24 + i * 4);
        } else {
            throw FormatError("unsupported font mapping method");
        }
    });
}

Image BitmapFont::glyph(int index) {
    const int per_page = cols_ * rows_;
    const int page = index / per_page, cell = index % per_page;
    if (index < 0 || page >= sheet_count_)
        throw FormatError("font glyph outside atlas");
    auto sheet = sheets_.find(page);
    if (sheet == sheets_.end()) {
        const uint8_t* tiled = Reader(data_).take(offset_ + size_t(page) * sheet_size_, sheet_size_);
        // GX2 thin 2D array slices rotate the bank selection by two per slice.
        const Bytes raw = addrlib::deswizzle(width_, height_, surf_height_, surface_, tile_mode_, int64_t(2 * page) << 8,
                                             pitch_, bpp_, tiled, sheet_size_);
        HDTexture t;
        t.source = t.name = "font";
        t.width = width_;
        t.height = height_;
        t.format = surface_;
        t.components = components_;
        t.levels.push_back(raw);
        HDTexture first = t;
        first.levels.resize(1);
        sheet = sheets_.emplace(page, flip_top_bottom(hd_images(first).front())).first;
    }
    const int row = cell / cols_, col = cell % cols_;
    const int x = col * (cell_w_ + 1) + 1, y = row * (cell_h_ + 1) + 1;
    return crop(sheet->second, x, y, x + cell_w_, y + cell_h_);
}

Image BitmapFont::render(const std::u32string& text) {
    if (text.empty() || text.size() > 256)
        throw FormatError("invalid sprite text length");
    std::vector<int> indices;
    std::vector<Width> metrics;
    for (char32_t c : text) {
        const auto m = mapping.find(uint32_t(c));
        if (m == mapping.end())
            throw FormatError("font lacks requested English glyph");
        const auto w = widths_.find(m->second);
        if (w == widths_.end())
            throw FormatError("font lacks requested English glyph");
        indices.push_back(m->second);
        metrics.push_back(w->second);
    }
    int width = cell_w_ * 2;
    for (const Width& m : metrics)
        width += m.advance;
    if (!(width > 0 && width <= 16384))
        throw FormatError("font sprite size limit exceeded");
    Image out(width, cell_h_);
    int x = cell_w_;
    for (size_t i = 0; i < indices.size(); i++) {
        const Width& m = metrics[i];
        if (m.glyph_width > cell_w_)
            throw FormatError("font glyph width exceeds cell");
        const Image g = crop(glyph(indices[i]), 0, 0, m.glyph_width, cell_h_);
        alpha_composite_at(out, g, x + m.left, 0);
        x += m.advance;
    }
    Box box;
    if (!alpha_bbox(out, box))
        throw FormatError("empty font sprite");
    return crop(out, box.left, box.top, box.right, box.bottom);
}

std::vector<FontSprite> font_sprites(const std::string& font_name) {
    static const std::pair<const char*, const char*> kActions[] = {
        {"ba_attack", "Attack"},       {"ba_chekku", "Check"},      {"ba_hanasu", "Let Go"},
        {"ba_harituku", "Slide"},      {"ba_hiraku", "Open"},       {"ba_huru", "Swing"},
        {"ba_husegu", "Defend"},       {"ba_jump", "Jump"},         {"ba_kaizuhe", "Sea Chart"},
        {"ba_kettei", "Choose"},       {"ba_modoru", "Return"},     {"ba_motu", "Lift"},
        {"ba_motu_buki", "Pick Up"},   {"ba_nageru", "Throw"},      {"ba_noru", "Climb"},
        {"ba_noru_hune", "Get In"},    {"ba_oku", "Drop"},          {"ba_oriru", "Let Go"},
        {"ba_oriru_hune", "Get Out"},  {"ba_setumei", "Info"},      {"ba_shagamu", "Crouch"},
        {"ba_shimau", "Put Away"},     {"ba_shiraberu", "Charts"},  {"ba_susumu", "Cruise"},
        {"ba_syaberu", "Speak"},       {"ba_tomeru", "Stop"},       {"ba_tugihe", "Next"},
        {"ba_tukamu", "Grab"},         {"ba_yameru", "Cancel"},     {"ba_yomu", "Read"},
        {"ba_zoom01", "Zoom1"},        {"ba_zoom02", "Zoom2"},
    };
    const auto u32 = [](const std::string& s) { return std::u32string(s.begin(), s.end()); };
    const auto action = [&](const char* name) {
        for (const auto& a : kActions)
            if (std::strcmp(a.first, name) == 0)
                return u32(a.second);
        return std::u32string();
    };
    std::vector<FontSprite> out;
    char name[32];
    if (font_name == "CKingMsg") {
        for (const auto& a : kActions)
            out.push_back({"object|acticon", a.first, u32(a.second)});
        out.push_back({"object|menures", "ba_modoru", u32("Return")});
        for (const char* n : {"ba_kettei", "ba_modoru"})
            out.push_back({"stage|name|stage", n, action(n)});
        for (int n = 0; n < 10; n++) {
            std::snprintf(name, sizeof name, "count_num_%d", n);
            out.push_back({"object|menures", name, u32(std::to_string(n))});
        }
        out.push_back({"object|menures", "rupy_num_cross", std::u32string(1, char32_t(0xD7))});
        for (int n = 1; n < 5; n++) {
            std::snprintf(name, sizeof name, "hierarchy_num_%d", n);
            out.push_back({"object|dmapres", name, u32(std::to_string(n))});
        }
        for (int n = 1; n < 4; n++) {
            std::snprintf(name, sizeof name, "hierarchy_num_b%d", n);
            out.push_back({"object|dmapres", name, u32("B" + std::to_string(n))});
        }
    } else if (font_name == "CKingMainL") {
        for (int n = 0; n < 10; n++) {
            std::snprintf(name, sizeof name, "rupy_num_%02d", n);
            out.push_back({"object|menures", name, u32(std::to_string(n))});
        }
        out.push_back({"object|menures", "rupy_num_dot", u32(".")});
        out.push_back({"object|menures", "rupy_num_slash", u32("/")});
        const char* const kWords[][3] = {{"clctres", "title_collect", "QUEST STATUS"}, {"clctres", "word_option", "OPTIONS"},
                                         {"clctres", "word_save", "SAVE"},             {"itemres", "title_item", "ITEMS"},
                                         {"itemres", "word_save2", "SAVE"},            {"saveres", "title_save", "SAVE"}};
        for (const auto& w : kWords)
            out.push_back({std::string("object|") + w[0], w[1], u32(w[2])});
        for (int n = 1; n < 4; n++) {
            std::snprintf(name, sizeof name, "file_data_%02d", n);
            out.push_back({"stage|name|stage", name, u32("Quest Log " + std::to_string(n))});
        }
    } else if (font_name == "CKingPic") {
        for (int n = 0; n < 4; n++) {
            std::snprintf(name, sizeof name, "font_%02d", n);
            out.push_back({"object|menures", name, {}, 0xE000 + n});
        }
    }
    return out;
}

Box gc_alpha_bounds(const GCTexture& gc) {
    const int fmt = gc.format;
    int bw, bh, bs;
    switch (fmt) {
    case 0: case 8: bw = 8; bh = 8; bs = 32; break;
    case 1: case 2: case 9: bw = 8; bh = 4; bs = 32; break;
    case 3: bw = 4; bh = 4; bs = 32; break;
    default: throw FormatError("unsupported sprite mask format");
    }
    std::vector<int> palette;
    if (fmt == 8 || fmt == 9) {
        for (size_t i = 0; i + 1 < gc.palette.size() + 1 && i + 2 <= gc.palette.size(); i += 2) {
            const int v = gc.palette[i] << 8 | gc.palette[i + 1];
            if (gc.palette_format == 0)
                palette.push_back(v >> 8);
            else if (gc.palette_format == 1)
                palette.push_back(255);
            else if (gc.palette_format == 2)
                palette.push_back((v & 0x8000) ? 255 : ((v >> 12) & 7) * 255 / 7);
            else
                throw FormatError("invalid sprite palette format");
        }
    }
    Image mask(gc.width, gc.height);
    size_t offset = 0;
    for (int by = 0; by < gc.height; by += bh) {
        check_cancel();
        for (int bx = 0; bx < gc.width; bx += bw) {
            for (int p = 0; p < bw * bh; p++) {
                const int x = bx + p % bw, y = by + p / bw;
                if (x >= gc.width || y >= gc.height)
                    continue;
                int v;
                const auto byte = [&](size_t at) {
                    if (at >= gc.pixels.size())
                        throw FormatError("truncated sprite");
                    return int(gc.pixels[at]);
                };
                if (fmt == 0 || fmt == 8)
                    v = (byte(offset + p / 2) >> (p % 2 == 0 ? 4 : 0)) & 15;
                else if (fmt == 3)
                    v = byte(offset + size_t(p) * 2);
                else
                    v = byte(offset + p);
                if (fmt == 8 || fmt == 9) {
                    if (v >= int(palette.size()))
                        throw FormatError("sprite mask references missing palette");
                    v = palette[v];
                } else if (fmt == 0) {
                    v *= 17;
                } else if (fmt == 2) {
                    v = (v >> 4) * 17;
                }
                mask.at(x, y)[3] = uint8_t(v);
            }
            offset += bs;
        }
    }
    Box box;
    if (!alpha_bbox(mask, box))
        throw FormatError("empty original sprite mask");
    return box;
}

Image fit_sprite(const Image& image, const GCTexture& gc, int scale) {
    Box box;
    if (!alpha_bbox(image, box))
        throw FormatError("empty replacement sprite");
    const Box gcb = gc_alpha_bounds(gc);
    const Image sprite = resize_lanczos(crop(image, box.left, box.top, box.right, box.bottom),
                                        (gcb.right - gcb.left) * scale, (gcb.bottom - gcb.top) * scale);
    Image result(gc.width * scale, gc.height * scale);
    paste(result, sprite, gcb.left * scale, gcb.top * scale);
    return result;
}

Image opening_sprite(const Image& image, const Box& box, const GCTexture& gc) {
    if (!(0 <= box.left && box.left < box.right && box.right <= image.width && 0 <= box.top && box.top < box.bottom &&
          box.bottom <= image.height))
        throw FormatError("opening panel crop outside HD picture");
    return resize_lanczos(crop(image, box.left, box.top, box.right, box.bottom), gc.width * 2, gc.height * 2);
}

Image heart_image(const Image& base, const Image& fill) {
    const Image image = resize_lanczos(crop(alpha_composite(base, fill), 5, 7, 57, 45), 88, 68);
    Image result(96, 96);
    paste(result, image, 4, 12);
    return result;
}

Image title_image(int w, int h, const Image& artwork, const Image* badge) {
    if (badge == nullptr)
        return resize_lanczos(artwork, w, h);
    Image canvas(w, h);
    double word_w = py_round(w * 0.86);
    double word_h = py_round(word_w * artwork.height / artwork.width);
    double badge_w = py_round(w * 0.264);
    double badge_h = py_round(badge_w * badge->height / badge->width);
    double gap = std::max(1, py_round(h * 0.04));
    const double scale = std::min(1.0, h / (word_h + gap + badge_h));
    const auto fit = [&](double v) { return std::max(1, py_round(v * scale)); };
    const int ww = fit(word_w), wh = fit(word_h), bw = fit(badge_w), bh = fit(badge_h), g = fit(gap);
    const int y = floordiv(h - wh - g - bh, 2);
    alpha_composite_at(canvas, resize_lanczos(artwork, ww, wh), floordiv(w - ww, 2), y);
    alpha_composite_at(canvas, resize_lanczos(*badge, bw, bh), floordiv(w - bw, 2), y + wh + g);
    return canvas;
}

const LayoutSprite kLayoutSprites[4] = {
    {"Cursor_00", "Crsor_00^t.bflim", "object|menures", "cursor_00_02"},
    {"Cursor_00", "Crsor_01^t.bflim", "object|menures", "cursor_00_01"},
    {"DungeonKey_00", "DungeonKey_00^t.bflim", "object|menures", "key"},
    {"BtnDungeonItemIcon_00", "MapItemIcon_01^l.bflim", "object|itemicon", "boss_key"},
};

const OpeningPanel kOpeningPanels[7] = {
    {"OpeningPic_00^o.bflim", "demo_1", {100, 0, 1178, 1000}}, {"OpeningPic_01^o.bflim", "demo_2", {104, 0, 1175, 500}},
    {"OpeningPic_02^o.bflim", "demo_3", {102, 0, 1174, 500}},  {"OpeningPic_03^o.bflim", "demo_4", {102, 0, 1174, 500}},
    {"OpeningPic_03^o.bflim", "demo_4_2", {1174, 0, 2221, 500}}, {"OpeningPic_04^o.bflim", "demo_5", {102, 0, 1174, 500}},
    {"OpeningPic_05^o.bflim", "demo_6", {104, 0, 1175, 500}},
};

std::vector<std::string> inventory_names(const std::string& path) {
    struct Icon {
        int number;
        std::vector<std::string> names;
    };
    static const std::vector<Icon> kItems = {
        {0, {"bottle_00"}}, {1, {"bottle_05", "bottle_08"}}, {3, {"bottle_06"}}, {4, {"bottle_07"}},
        {5, {"bottle_01"}}, {6, {"bottle_02"}}, {7, {"bottle_03"}}, {8, {"bottle_04"}}, {9, {"bottle_09"}},
        {10, {"telescope"}}, {11, {"rope"}}, {12, {"camera"}}, {13, {"camera_2"}}, {15, {"boots_00"}},
        {17, {"boomerang"}}, {18, {"bow_01"}}, {19, {"hookshot"}}, {20, {"hammer_01"}}, {21, {"bomb_00"}},
        {22, {"fan"}}, {23, {"coverofbeast"}}, {24, {"coverofbait"}}, {25, {"delivery"}}, {26, {"bait_02"}},
        {27, {"bait_01"}}, {28, {"beast_08"}}, {29, {"beast_02"}}, {30, {"beast_04"}}, {31, {"beast_03"}},
        {32, {"beast_01"}}, {33, {"beast_05"}}, {34, {"beast_06"}}, {35, {"beast_07"}}, {36, {"delivery_17"}},
        {37, {"delivery_13"}}, {38, {"delivery_14"}}, {39, {"delivery_15"}}, {40, {"delivery_16"}},
        {41, {"delivery_18"}}, {42, {"delivery_19"}}, {43, {"delivery_01"}}, {44, {"delivery_02"}},
        {45, {"delivery_03"}}, {46, {"delivery_06"}}, {47, {"delivery_08"}}, {48, {"delivery_05"}},
        {49, {"delivery_04"}}, {50, {"delivery_07"}}, {51, {"delivery_09"}}, {52, {"delivery_10"}},
        {53, {"delivery_11"}}, {54, {"delivery_12"}},
    };
    static const std::vector<Icon> kCollect = {
        {4, {"shield_00"}}, {5, {"shield_01"}}, {6, {"sail_00", "sail_01", "sail_02"}}, {8, {"big_purse"}},
        {9, {"gloves_00", "gloves_01"}}, {11, {"sword_00"}}, {12, {"sword_01"}}, {13, {"sword_02"}},
        {14, {"sword_03"}}, {15, {"baton"}},
    };
    static const std::vector<Icon> kDungeon = {{0, {"dungeon_map"}}, {2, {"compass"}}};
    static const std::vector<Icon> kMap = {{0, {"cmap_treasure2"}}, {1, {"cmap_phantomship2"}}, {2, {"cmap_tingle2"}},
                                           {3, {"cmap_hint2"}}, {4, {"cmap_tri2"}}};
    struct Group {
        const char* archive;
        const char* prefix;
        const std::vector<Icon>* icons;
    };
    const Group groups[] = {{"BtnItemIcon_00", "Icon128", &kItems},
                            {"BtnCollectIcon_00", "CollectIcon118", &kCollect},
                            {"BtnDungeonItemIcon_00", "MapItemIcon", &kDungeon},
                            {"BtnMapIcon_01", "MapBtn", &kMap}};
    const std::string name = base_name(path);
    for (const Group& g : groups) {
        if (path.find(std::string("/") + g.archive + ".szs/timg/") == std::string::npos)
            continue;
        for (const Icon& icon : *g.icons) {
            char expected[64];
            std::snprintf(expected, sizeof expected, "%s_%02d^l.bflim", g.prefix, icon.number);
            if (name == expected)
                return icon.names;
        }
    }
    return {};
}

const char* swim_icon(const std::string& filename) {
    static const std::pair<const char*, const char*> kSwim[] = {
        {"SwimTimeLight_00^t.bflim", "light"},          {"SwimTimeBase_00^t.bflim", "swimtime_meter"},
        {"SwimTimeAlpha_00^t.bflim", "swimtime_meter_mask"}, {"SwimTimeDeco_00^t.bflim", "swimtime_soul"},
        {"SwimTimeFrame_00^t.bflim", "swimtime_waku"},  {"SwimTimeSpec_00^t.bflim", "tekari"},
    };
    for (const auto& s : kSwim)
        if (filename == s.first)
            return s.second;
    return nullptr;
}

const char* heart_name(const std::string& filename) {
    static const std::pair<const char*, const char*> kHearts[] = {
        {"HeartFull_00^l.bflim", "heart"}, {"HeartDamage_00^l.bflim", "heart_01"},
        {"HeartDamage_01^l.bflim", "heart_02"}, {"HeartDamage_02^l.bflim", "heart_03"},
    };
    for (const auto& h : kHearts)
        if (filename == h.first)
            return h.second;
    return nullptr;
}

int candidate_priority(const std::string& hd_name, const std::string& hd_resource) {
    static const std::pair<const char*, const char*> kPreferred[] = {
        {"dk_kamen1", "object|demo_dk"},          {"test_grass_ktga", "stage|a_mori|room0"},
        {"txa_nami_ia", "stage|sea|room11"},      {"txa_nami_01", "stage|sea|room11"},
        {"door_08", "stage|ebesso|stage"},        {"txa_doom_a", "stage|sea|room44"},
        {"txa_hune_a", "stage|sea|room44"},       {"txa_ita3mai_a", "stage|sea|room44"},
        {"txa_iwa2_a", "stage|linkug|room0"},     {"bm_head01", "object|demo10"},
        {"ws_naka06", "stage|cave08|stage"},      {"ws_mae08", "stage|cave08|stage"},
        {"ws_mae09", "stage|cave08|stage"},       {"cloudtx_01", "stage|a_nami|stage"},
        {"cloudtx_03", "stage|a_nami|stage"},     {"txo_stone_mori_old", "stage|sea|room42"},
    };
    const std::string name = lower(hd_name);
    for (const auto& p : kPreferred)
        if (name == p.first)
            return hd_resource == p.second ? 1 : 0;
    return 0;
}

} // namespace wwhd
