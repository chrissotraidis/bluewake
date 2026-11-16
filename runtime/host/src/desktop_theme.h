#ifndef BLUEWAKE_DESKTOP_THEME_H
#define BLUEWAKE_DESKTOP_THEME_H
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

// Shared BlueWake desktop styling. Scoped to the menu, never the game/HUD.
namespace bluewake_ui {

// How far the menu's surfaces and quiet text stand off its background, as the
// player set it (BLUEWAKE_MENU_CONTRAST, 1 is the built-in look). Read every
// frame, so the slider in the menu moves the menu while it is open.
inline float contrast() {
    const char* set = std::getenv("BLUEWAKE_MENU_CONTRAST");
    const float value = set != nullptr && set[0] != '\0' ? std::strtof(set, nullptr) : 1.f;
    return value > 0.f ? std::clamp(value, 0.6f, 1.8f) : 1.f;
}

// The menu's text size as the player set it (BLUEWAKE_MENU_TEXT_SIZE, 1 is the
// display's own size for it), read every frame like the contrast. Each
// platform draws the menu's font kTextLargest times larger than it shows it,
// so the largest size is drawn at the font's own and stays sharp.
inline constexpr float kTextLargest = 1.5f;
inline float text_size() {
    const char* set = std::getenv("BLUEWAKE_MENU_TEXT_SIZE");
    const float value = set != nullptr && set[0] != '\0' ? std::strtof(set, nullptr) : 1.f;
    return value > 0.f ? std::clamp(value, 0.75f, kTextLargest) : 1.f;
}

// A colour pushed away from (or pulled towards) the background behind it.
inline ImVec4 lift(const ImVec4& colour, const ImVec4& behind, float k) {
    return ImVec4(std::clamp(behind.x + (colour.x - behind.x) * k, 0.f, 1.f),
                  std::clamp(behind.y + (colour.y - behind.y) * k, 0.f, 1.f),
                  std::clamp(behind.z + (colour.z - behind.z) * k, 0.f, 1.f), colour.w);
}

// The menu's colours, after Wind Waker's own menus but drawn here, in code:
// green leaf panels, a wooden plaque, gold titles outlined in dark brown, and
// a cream paper slip. Nothing in them comes from the game.
namespace palette {
inline constexpr ImVec4 kCream(0.97f, 0.93f, 0.82f, 1.f);      // text on the leaf
inline constexpr ImVec4 kGold(0.95f, 0.79f, 0.29f, 1.f);       // what is chosen or lit
inline constexpr ImVec4 kInk(0.17f, 0.10f, 0.05f, 1.f);        // outlines, text on gold
inline constexpr ImVec4 kWood(0.45f, 0.29f, 0.16f, 1.f);       // the tab plaque, buttons
inline constexpr ImVec4 kWoodDark(0.25f, 0.15f, 0.07f, 1.f);   // the plaque's shadow
inline constexpr ImVec4 kLeaf(0.17f, 0.38f, 0.20f, 1.f);       // the panel
inline constexpr ImVec4 kLeafDeep(0.10f, 0.25f, 0.13f, 1.f);   // wells: tracks, pills
inline constexpr ImVec4 kSlip(0.95f, 0.91f, 0.79f, 1.f);       // the help slip
inline constexpr ImVec4 kSlipEdge(0.79f, 0.72f, 0.56f, 1.f);   // its shadow
inline constexpr ImVec4 kSlipInk(0.33f, 0.22f, 0.13f, 1.f);    // its text
inline constexpr ImVec4 kButtonA(0.25f, 0.68f, 0.42f, 1.f);    // the A glyph
inline constexpr ImVec4 kButtonB(0.85f, 0.27f, 0.23f, 1.f);    // the B glyph
inline constexpr ImVec4 kBumper(0.49f, 0.54f, 0.58f, 1.f);     // the LB and RB glyphs
} // namespace palette

inline ImU32 u32(const ImVec4& colour, float alpha = 1.f) {
    return ImGui::GetColorU32(ImVec4(colour.x, colour.y, colour.z, colour.w * alpha));
}

inline void begin_theme(float scale = 1.f) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22 * scale, 18 * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10 * scale, 7 * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12 * scale, 10 * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18 * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8 * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 8 * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 10 * scale);
    // Every frame carries a line around it: a fill a shade off the background
    // leaves a control with no edge to find (1.15:1 measured), which the quiet
    // outline fixes without lightening the menu.
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    using namespace palette;
    const float k = contrast();
    const ImVec4 base(kLeaf.x, kLeaf.y, kLeaf.z, 0.97f);
    // Quiet text was too dark against the base (Dale, 2026-10-14): it is a
    // softer cream, and the contrast setting lifts it further.
    const ImVec4 muted = lift(ImVec4(0.84f, 0.84f, 0.72f, 1.f), base, k);
    const ImVec4 well = lift(kLeafDeep, base, k);
    const ImVec4 hover = lift(ImVec4(0.24f, 0.47f, 0.26f, 1.f), base, k);
    const ImVec4 active = lift(ImVec4(0.30f, 0.53f, 0.28f, 1.f), base, k);
    // The edge of a control: this is what has to stand off the background, so
    // it is lifted hardest and never sinks below a line you can see.
    const ImVec4 edge = lift(ImVec4(0.62f, 0.68f, 0.52f, 0.9f), base, k);
    const ImVec4 wood_hover(0.55f, 0.36f, 0.20f, 1.f), wood_active(0.62f, 0.41f, 0.22f, 1.f);
    const struct { ImGuiCol slot; ImVec4 value; } colors[] = {
        {ImGuiCol_Text, kCream}, {ImGuiCol_TextDisabled, muted},
        {ImGuiCol_WindowBg, base}, {ImGuiCol_PopupBg, ImVec4(kLeafDeep.x, kLeafDeep.y, kLeafDeep.z, 0.98f)},
        {ImGuiCol_Border, edge},
        {ImGuiCol_TitleBg, base}, {ImGuiCol_TitleBgActive, base},
        {ImGuiCol_FrameBg, well}, {ImGuiCol_FrameBgHovered, hover}, {ImGuiCol_FrameBgActive, active},
        {ImGuiCol_Button, kWood}, {ImGuiCol_ButtonHovered, wood_hover}, {ImGuiCol_ButtonActive, wood_active},
        {ImGuiCol_Header, kWood}, {ImGuiCol_HeaderHovered, wood_hover}, {ImGuiCol_HeaderActive, wood_active},
        {ImGuiCol_Tab, kWood}, {ImGuiCol_TabHovered, wood_hover}, {ImGuiCol_TabActive, wood_active},
        {ImGuiCol_CheckMark, kGold}, {ImGuiCol_SliderGrab, kGold}, {ImGuiCol_SliderGrabActive, kGold},
        // No outline round what the pad's cursor is on: the row's gold light
        // (bw_menu_rows_begin) shows it instead.
        {ImGuiCol_NavCursor, ImVec4(0.f, 0.f, 0.f, 0.f)},
        {ImGuiCol_Separator, ImVec4(edge.x, edge.y, edge.z, 0.55f)},
        {ImGuiCol_ScrollbarBg, ImVec4(0.f, 0.f, 0.f, 0.f)},
        {ImGuiCol_ScrollbarGrab, ImVec4(kCream.x, kCream.y, kCream.z, 0.35f)},
        {ImGuiCol_ScrollbarGrabHovered, ImVec4(kCream.x, kCream.y, kCream.z, 0.55f)},
        {ImGuiCol_ScrollbarGrabActive, kGold},
        {ImGuiCol_TextSelectedBg, ImVec4(kGold.x, kGold.y, kGold.z, 0.35f)},
        {ImGuiCol_ResizeGrip, ImVec4(kCream.x, kCream.y, kCream.z, 0.18f)},
        {ImGuiCol_ResizeGripHovered, ImVec4(kCream.x, kCream.y, kCream.z, 0.4f)},
        {ImGuiCol_ResizeGripActive, kGold},
    };
    for (const auto& color : colors) ImGui::PushStyleColor(color.slot, color.value);
}
inline constexpr int kThemeColors = 32;
inline void end_theme() {
    ImGui::PopStyleColor(kThemeColors);
    ImGui::PopStyleVar(8); // the seven roundings and paddings, and the frame's border
}

// Text with a dark edge all round, as the game writes its titles.
// `size` is the font size to draw at (0: the current one); the edge is drawn
// all round and a little more below, as a shadow.
inline void outlined_text(ImDrawList* paint, ImVec2 at, ImU32 fill, ImU32 edge, const char* text,
                          const char* end = nullptr, float width = 1.5f, float size = 0.f) {
    static const ImVec2 kAround[] = {{-1.f, 0.f}, {1.f, 0.f}, {0.f, -1.f}, {0.f, 1.f}, {-0.7f, -0.7f},
                                     {0.7f, -0.7f}, {-0.7f, 0.7f}, {0.7f, 0.7f}, {0.f, 1.4f}};
    ImFont* font = ImGui::GetFont();
    if (size <= 0.f)
        size = ImGui::GetFontSize();
    for (const ImVec2& d : kAround)
        paint->AddText(font, size, ImVec2(at.x + d.x * width, at.y + d.y * width), edge, text, end);
    paint->AddText(font, size, at, fill, text, end);
}

// A stitched line round a rectangle, dashes along its rounded edge, as the
// game's file screen sews round the chosen file.
inline void stitch(ImDrawList* paint, ImVec2 a, ImVec2 b, float rounding, ImU32 colour, float dash,
                   float thickness = 1.5f) {
    dash = std::max(dash, 1.f);
    paint->PathRect(a, b, rounding);
    if (paint->_Path.Size < 2) {
        paint->PathClear();
        return;
    }
    ImVector<ImVec2> path = paint->_Path;
    paint->PathClear();
    // A copy of the first point, not a reference into the vector: push_back
    // may move the vector before it reads it.
    const ImVec2 first = path[0];
    path.push_back(first);
    double along = 0.0; // how far round the walk is, for where in the dash-gap cycle it is
    for (int i = 1; i < path.Size; ++i) {
        const ImVec2 p = path[i - 1], q = path[i];
        const double length = std::sqrt(double(q.x - p.x) * (q.x - p.x) + double(q.y - p.y) * (q.y - p.y));
        double t = 0.0;
        // Each step goes at least a thousandth of a pixel, so rounding can't stall it.
        while (length - t > 1e-3) {
            const double phase = std::fmod(along, double(dash) * 2.0);
            const bool drawn = phase < dash;
            const double step = std::min(length - t, std::max(1e-3, (drawn ? dash : dash * 2.0) - phase));
            if (drawn) {
                const float t0 = float(t / length), t1 = float((t + step) / length);
                paint->AddLine(ImVec2(p.x + (q.x - p.x) * t0, p.y + (q.y - p.y) * t0),
                               ImVec2(p.x + (q.x - p.x) * t1, p.y + (q.y - p.y) * t1), colour, thickness);
            }
            t += step;
            along += step;
        }
    }
}

// The wind's swirl, the menu's cursor: a spiral opening out from `centre`.
inline void swirl(ImDrawList* paint, ImVec2 centre, float radius, ImU32 colour, float thickness) {
    constexpr int kPoints = 28;
    for (int i = 0; i <= kPoints; ++i) {
        const float t = static_cast<float>(i) / kPoints;
        const float angle = 3.4f + t * 9.6f;  // a turn and a half, starting at its inner end
        const float r = radius * (0.12f + 0.88f * t);
        paint->PathLineTo(ImVec2(centre.x + std::cos(angle) * r, centre.y + std::sin(angle) * r * 0.9f));
    }
    paint->PathStroke(colour, 0, thickness);
}

// A round button glyph with its letter, as the game shows A and B.
inline void glyph(ImDrawList* paint, ImVec2 centre, float radius, ImU32 fill, const char* letter) {
    const float wide = ImGui::CalcTextSize(letter).x;
    const float half = std::max(radius, wide * 0.5f + radius * 0.45f);
    paint->AddRectFilled(ImVec2(centre.x - half, centre.y - radius), ImVec2(centre.x + half, centre.y + radius), fill,
                         radius);
    paint->AddRectFilled(ImVec2(centre.x - half, centre.y + radius * 0.55f),
                         ImVec2(centre.x + half, centre.y + radius), IM_COL32(0, 0, 0, 50), radius,
                         ImDrawFlags_RoundCornersBottom);
    const ImVec2 size = ImGui::CalcTextSize(letter);
    paint->AddText(ImVec2(centre.x - size.x * 0.5f, centre.y - size.y * 0.5f), IM_COL32(255, 255, 255, 255), letter);
}
// How wide glyph() draws a letter of this height.
inline float glyph_width(const char* letter, float radius) {
    return 2.f * std::max(radius, ImGui::CalcTextSize(letter).x * 0.5f + radius * 0.45f);
}

inline void heading(const char* title, const char* subtitle) {
    ImGui::TextUnformatted(title);
    ImGui::PushTextWrapPos();
    ImGui::TextDisabled("%s", subtitle);
    ImGui::PopTextWrapPos();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
}
#endif
