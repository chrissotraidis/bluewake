// The generated HUD and title art of the built-in Wind Waker HD importer: the
// HD bitmap fonts drawn into GameCube sprites, layout sprites fitted to the
// GameCube UVs, hearts, the opening's panels and the title logo. A port of
// scripts/wwhd/ui.py, layout.py, title.py, font.py and matches.py.
#pragma once

#include "imaging.h"

#include <map>
#include <optional>

namespace wwhd {

// Wii U FFNT v3 bitmap font (font.py BitmapFont).
class BitmapFont {
  public:
    explicit BitmapFont(const Bytes& data);
    Image glyph(int index);
    Image render(const std::u32string& text);
    std::map<uint32_t, int> mapping; // character -> glyph index

  private:
    struct Width {
        int left, glyph_width, advance;
    };
    Bytes data_;
    int cell_w_ = 0, cell_h_ = 0, sheet_count_ = 0, cols_ = 0, rows_ = 0, width_ = 0, height_ = 0;
    size_t sheet_size_ = 0, offset_ = 0;
    int64_t surface_ = 0;
    std::vector<uint8_t> components_;
    int64_t tile_mode_ = 0, pitch_ = 0, surf_height_ = 0, bpp_ = 0;
    std::map<int, Width> widths_;
    std::map<int, Image> sheets_;
};

// A sprite the font draws: (resource, GameCube texture name, text or glyph).
struct FontSprite {
    std::string resource;
    std::string name;
    std::u32string text; // empty when glyph is set
    int glyph = -1;      // a private-use character drawn as one glyph
};
std::vector<FontSprite> font_sprites(const std::string& font_name);

// The original sprite's alpha bounds (ui.py alpha_bounds).
Box gc_alpha_bounds(const GCTexture& gc);
Image fit_sprite(const Image& image, const GCTexture& gc, int scale = 4);
Image opening_sprite(const Image& image, const Box& box, const GCTexture& gc);
Image heart_image(const Image& base, const Image& fill);
Image title_image(int w, int h, const Image& artwork, const Image* badge);

struct LayoutSprite {
    const char* archive;
    const char* filename;
    const char* resource;
    const char* name;
};
extern const LayoutSprite kLayoutSprites[4];

struct OpeningPanel {
    const char* picture;
    const char* name;
    Box box;
};
extern const OpeningPanel kOpeningPanels[7];

// The GameCube names a Wind Waker HD inventory icon stands for (layout.py), or none.
std::vector<std::string> inventory_names(const std::string& path);
const char* swim_icon(const std::string& filename);
const char* heart_name(const std::string& filename);

// matches.py: 1 when this HD texture is the audited choice for its name.
int candidate_priority(const std::string& hd_name, const std::string& hd_resource);

} // namespace wwhd
