#ifndef BLUEWAKE_STATUS_LINE_OVERLAY_H
#define BLUEWAKE_STATUS_LINE_OVERLAY_H
#include <cstdio>
#include <imgui.h>

#include "status_line.h"

// The status line ("State saved", "Fullscreen on"...), drawn by both
// desktop menus' overlays: the bottom-left corner for two and a half seconds,
// then a half-second fade. now_ms is the overlay's own clock (SDL_GetTicks).
namespace bluewake_ui {
inline void draw_status_line(unsigned long long now_ms, float scale = 1.f) {
    static unsigned shown;
    static unsigned long long since;
    static char text[64];
    char latest[64];
    const unsigned number = bluewake_status_read(latest, sizeof latest);
    if (number != 0 && number != shown) {
        shown = number;
        since = now_ms;
        std::snprintf(text, sizeof text, "%s", latest);
    }
    const unsigned long long age = now_ms - since;
    if (shown == 0 || age >= 3000)
        return;
    const float fade = age > 2500 ? (3000 - age) / 500.f : 1.f;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(12.f * scale, display.y - 12.f * scale), ImGuiCond_Always, ImVec2(0.f, 1.f));
    ImGui::SetNextWindowBgAlpha(0.75f * fade);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);
    if (ImGui::Begin("##bluewake-status-line", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav))
        ImGui::TextUnformatted(text);
    ImGui::End();
    ImGui::PopStyleVar();
}
} // namespace bluewake_ui
#endif
