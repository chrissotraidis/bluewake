#ifndef BLUEWAKE_DESKTOP_THEME_H
#define BLUEWAKE_DESKTOP_THEME_H
#include <imgui.h>
#include <algorithm>
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

inline void begin_theme(float scale = 1.f) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24 * scale, 22 * scale));
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
    const float k = contrast();
    const ImVec4 ink(0.91f, 0.96f, 1.f, 1.f);
    const ImVec4 base(0.025f, 0.06f, 0.11f, 0.98f);
    // Quiet text was too dark against the base (Dale, 2026-10-14); it starts
    // brighter now and the contrast setting lifts it further.
    const ImVec4 muted = lift(ImVec4(0.68f, 0.78f, 0.86f, 1.f), base, k);
    const ImVec4 panel = lift(ImVec4(0.07f, 0.14f, 0.22f, 1.f), base, k);
    const ImVec4 hover = lift(ImVec4(0.10f, 0.28f, 0.38f, 1.f), base, k);
    const ImVec4 active = lift(ImVec4(0.08f, 0.38f, 0.50f, 1.f), base, k);
    // The edge of a control: this is what has to stand off the background, so
    // it is lifted hardest and never sinks below a line you can see.
    const ImVec4 edge = lift(ImVec4(0.36f, 0.53f, 0.63f, 0.9f), base, k);
    const ImVec4 accent(0.18f, 0.8f, 0.94f, 1.f);
    const struct { ImGuiCol slot; ImVec4 value; } colors[] = {
        {ImGuiCol_Text, ink}, {ImGuiCol_TextDisabled, muted},
        {ImGuiCol_WindowBg, base}, {ImGuiCol_PopupBg, base},
        {ImGuiCol_Border, edge},
        {ImGuiCol_TitleBg, base}, {ImGuiCol_TitleBgActive, base},
        {ImGuiCol_FrameBg, panel}, {ImGuiCol_FrameBgHovered, hover}, {ImGuiCol_FrameBgActive, active},
        {ImGuiCol_Button, panel}, {ImGuiCol_ButtonHovered, hover}, {ImGuiCol_ButtonActive, active},
        {ImGuiCol_Header, panel}, {ImGuiCol_HeaderHovered, hover}, {ImGuiCol_HeaderActive, active},
        {ImGuiCol_Tab, panel}, {ImGuiCol_TabHovered, hover}, {ImGuiCol_TabActive, active},
        {ImGuiCol_CheckMark, accent}, {ImGuiCol_SliderGrab, accent}, {ImGuiCol_SliderGrabActive, accent},
        // No outline round what the pad's cursor is on: it lights up instead.
        {ImGuiCol_NavCursor, ImVec4(0.f, 0.f, 0.f, 0.f)},
        {ImGuiCol_Separator, edge},
    };
    for (const auto& color : colors) ImGui::PushStyleColor(color.slot, color.value);
}
inline void end_theme() {
    ImGui::PopStyleColor(24);
    ImGui::PopStyleVar(8); // the seven roundings and paddings, and the frame's border
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
