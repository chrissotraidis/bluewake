#ifndef BLUEWAKE_DESKTOP_THEME_H
#define BLUEWAKE_DESKTOP_THEME_H
#include <imgui.h>

// Shared BlueWake desktop styling. Scoped to the menu, never the game/HUD.
namespace bluewake_ui {
inline void begin_theme(float scale = 1.f) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24 * scale, 22 * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10 * scale, 7 * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12 * scale, 10 * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18 * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8 * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 8 * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 10 * scale);
    const ImVec4 ink(0.91f, 0.96f, 1.f, 1.f), muted(0.57f, 0.68f, 0.77f, 1.f);
    const ImVec4 base(0.025f, 0.06f, 0.11f, 0.98f), panel(0.06f, 0.12f, 0.19f, 1.f);
    const ImVec4 hover(0.08f, 0.23f, 0.32f, 1.f), active(0.06f, 0.32f, 0.43f, 1.f);
    const ImVec4 accent(0.18f, 0.8f, 0.94f, 1.f);
    const struct { ImGuiCol slot; ImVec4 value; } colors[] = {
        {ImGuiCol_Text, ink}, {ImGuiCol_TextDisabled, muted},
        {ImGuiCol_WindowBg, base}, {ImGuiCol_PopupBg, base},
        {ImGuiCol_Border, ImVec4(0.15f, 0.28f, 0.37f, 0.8f)},
        {ImGuiCol_TitleBg, base}, {ImGuiCol_TitleBgActive, base},
        {ImGuiCol_FrameBg, panel}, {ImGuiCol_FrameBgHovered, hover}, {ImGuiCol_FrameBgActive, active},
        {ImGuiCol_Button, panel}, {ImGuiCol_ButtonHovered, hover}, {ImGuiCol_ButtonActive, active},
        {ImGuiCol_Header, panel}, {ImGuiCol_HeaderHovered, hover}, {ImGuiCol_HeaderActive, active},
        {ImGuiCol_Tab, panel}, {ImGuiCol_TabHovered, hover}, {ImGuiCol_TabActive, active},
        {ImGuiCol_CheckMark, accent}, {ImGuiCol_SliderGrab, accent}, {ImGuiCol_SliderGrabActive, accent},
        {ImGuiCol_Separator, ImVec4(0.15f, 0.28f, 0.37f, 0.8f)},
    };
    for (const auto& color : colors) ImGui::PushStyleColor(color.slot, color.value);
}
inline void end_theme() {
    ImGui::PopStyleColor(23);
    ImGui::PopStyleVar(7);
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
