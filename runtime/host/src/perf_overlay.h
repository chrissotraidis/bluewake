// SPDX-License-Identifier: GPL-3.0-or-later
// The performance panel (F9's full mode): the frame rate and frame times with a
// graph of them, the machine's CPU, memory and graphics card load, temperatures,
// power and clocks, and how the picture is being made. It is drawn in the top
// right corner of the game's window, over the game and under the settings menu,
// as the TotK port's overlay is (runtime/perfmon.c and runtime/ui/menu.cpp
// there). The numbers are perf_monitor.c's; the picture's details are worded by
// the menu and handed in, so this file needs nothing but ImGui.
#pragma once
#include <imgui.h>

#include <cstdio>

#include "perf_monitor.h"

namespace bluewake_ui {

// What the menu knows about the picture: its window, the resolution the game is
// drawn at, what scales it to the window, the frame limit and the present mode.
struct BwPerfPicture {
    const char* window;
    const char* image;
    const char* scaling;
    const char* limit;
    const char* present;
};

namespace perf_detail {

// A number in the panel's right-aligned column, or "-" for one the machine or
// the renderer does not give (a negative, as perf_monitor.h keeps them).
inline void number(const char* format, float value) {
    ImGui::TableNextColumn();
    if (value < 0.f) {
        ImGui::TextDisabled("%s", "-");
        return;
    }
    char text[48];
    std::snprintf(text, sizeof text, format, value);
    const float room = ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(text).x;
    if (room > 0.f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + room);
    ImGui::TextUnformatted(text);
}

inline void label(const char* text, const ImVec4& color) {
    ImGui::TableNextColumn();
    ImGui::TextColored(color, "%s", text);
}

// A column as wide as the widest text it can hold, so a number changing never
// moves the panel (the digits of the menu's font share one width).
inline float column_width(const char* const* samples, int count) {
    float width = 0.f;
    for (int i = 0; i < count; ++i) {
        char text[48];
        std::snprintf(text, sizeof text, "%s", samples[i]);
        const float w = ImGui::CalcTextSize(text).x;
        if (w > width)
            width = w;
    }
    return width;
}

} // namespace perf_detail

// The whole panel. font and font_scale are the menu's (bw_platform::look's), so the
// panel is set in the same text as the menu, follows the player's text size, and is
// drawn from the menu's font atlas — rasterised at the display's scale, so it is sharp
// at every size rather than a small font stretched up. The spacing, the margins and
// the graph are all in font sizes, so a different resolution or text size re-lays the
// panel out instead of scaling a picture of it.
inline void draw_perf_panel(const BwPerfStats& stats, const BwPerfPicture& picture, ImFont* font, float font_scale) {
    using namespace perf_detail;
    if (font != nullptr)
        ImGui::PushFont(font);
    // The text is half again the menu's: the panel is read at a glance, not sat in
    // front of. Never above the font's own rasterised size, though — drawn larger
    // than its atlas is what looks stretched. Everything below is in font sizes, so
    // a new resolution or text size re-lays the panel out instead of scaling it.
    const float base_size = font != nullptr ? font->FontSize : ImGui::GetFontSize();
    const float panel_scale = font_scale * 1.5f < 1.f ? font_scale * 1.5f : 1.f;
    const float font_size = base_size * panel_scale;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float margin = 0.7f * font_size;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.6f * font_size, 0.5f * font_size));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.4f * font_size, 0.25f * font_size));
    ImGui::SetNextWindowPos(ImVec2(display.x - margin, margin), ImGuiCond_Always, ImVec2(1.f, 0.f));
    ImGui::SetNextWindowBgAlpha(0.62f);
    ImGui::Begin("##bluewake-perf", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoMove);
    ImGui::SetWindowFontScale(panel_scale);   // inside the window: this frame's text, this window only
    const ImVec4 gpu_color(0.55f, 0.83f, 0.31f, 1.f), cpu_color(0.18f, 0.59f, 0.80f, 1.f),
        ram_color(0.80f, 0.45f, 0.70f, 1.f), rate_color(0.95f, 0.95f, 0.95f, 1.f),
        picture_color(0.62f, 0.70f, 0.95f, 1.f);

    if (ImGui::BeginTable("##bluewake-perf-numbers", 4, ImGuiTableFlags_SizingFixedFit)) {
        static const char* const kFirst[] = {"GPU", "VRAM", "CPU", "RAM", "60 FPS"};
        static const char* const kValue[] = {"100%", "888.8 GB", "888.8 ms"};
        static const char* const kOf[] = {"188 C", "888.8 GB", "8888.8 ms"};
        static const char* const kShare[] = {"888 W", "8888 MHz", "100%", "888.8 GB"};
        ImGui::TableSetupColumn("##labels", ImGuiTableColumnFlags_WidthFixed, column_width(kFirst, 5));
        ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthFixed, column_width(kValue, 3));
        ImGui::TableSetupColumn("##of", ImGuiTableColumnFlags_WidthFixed, column_width(kOf, 3));
        ImGui::TableSetupColumn("##share", ImGuiTableColumnFlags_WidthFixed, column_width(kShare, 4));
        label("GPU", gpu_color);
        number("%.0f%%", stats.gpu_pct);
        number("%.0f C", stats.gpu_temp_c);
        number("%.0f W", stats.gpu_power_w);
        label("VRAM", gpu_color);
        number("%.1f GB", stats.vram_used_mb / 1024.f);
        ImGui::TableNextColumn();
        if (stats.vram_total_mb < 0.f)
            ImGui::TextDisabled("%s", "-");
        else {
            char text[48];
            std::snprintf(text, sizeof text, "of %.1f GB", stats.vram_total_mb / 1024.f);
            ImGui::TextUnformatted(text);
        }
        number("%.0f MHz", stats.gpu_clock_mhz);
        label("CPU", cpu_color);
        number("%.0f%%", stats.cpu_total_pct);
        number("%.0f C", stats.cpu_temp_c);
        number("%.0f%%", stats.cpu_game_pct);
        label("RAM", ram_color);
        number("%.1f GB", stats.ram_used_mb / 1024.f);
        ImGui::TableNextColumn();
        if (stats.ram_total_mb < 0.f)
            ImGui::TextDisabled("%s", "-");
        else {
            char text[48];
            std::snprintf(text, sizeof text, "of %.1f GB", stats.ram_total_mb / 1024.f);
            ImGui::TextUnformatted(text);
        }
        number("%.1f GB", stats.ram_game_mb / 1024.f);
        ImGui::TableNextColumn();   // the frame row: the rate where the labels are
        if (stats.fps <= 0.f)
            ImGui::TextDisabled("%s", "-");
        else {
            char text[48];
            std::snprintf(text, sizeof text, "%.0f FPS", stats.fps);
            ImGui::TextColored(rate_color, "%s", text);
        }
        number("%.1f ms", stats.frame_ms_avg);
        number("%.1f ms", stats.frame_ms_max);
        ImGui::EndTable();
    }

    // The frame times as a graph: the worst frame sets its top, and 50 ms (20
    // FPS) is the least height it shows, so a good run still reads as a line.
    const float top = stats.frame_ms_max > 50.f ? stats.frame_ms_max : 50.f;
    // The graph does not count toward the panel's width in the same way the
    // tables do; -FLT_MIN fills the columns the two tables sized.
    // The line in the waits' amber over a darker trough, so it reads against the
    // game behind the panel rather than as a faint smudge in the default grey.
    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.93f, 0.76f, 0.33f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.35f));
    ImGui::PlotLines("##bluewake-perf-graph", stats.frame_ms, BW_PERF_HISTORY, 0, nullptr, 0.f, top,
                     ImVec2(-1.f, 3.5f * font_size));
    ImGui::PopStyleColor(2);

    static const char* const kLabels[] = {"Window", "Game image", "Scaling", "Frame limit", "Presentation"};
    const char* const values[] = {picture.window, picture.image, picture.scaling, picture.limit, picture.present};
    float value_width = 0.f;
    for (int i = 0; i < 5; ++i)
        if (values[i] != nullptr) {
            const float w = ImGui::CalcTextSize(values[i]).x;
            if (w > value_width)
                value_width = w;
        }
    if (ImGui::BeginTable("##bluewake-perf-picture", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("##picture-labels", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##picture-values", ImGuiTableColumnFlags_WidthFixed, value_width);
        for (int i = 0; i < 5; ++i) {
            label(kLabels[i], picture_color);
            ImGui::TableNextColumn();
            if (values[i] == nullptr || values[i][0] == '\0')
                ImGui::TextDisabled("%s", "-");
            else
                ImGui::TextUnformatted(values[i]);
        }
        ImGui::EndTable();
    }
    if (stats.gpu_name[0] != '\0')
        ImGui::TextDisabled("%s", stats.gpu_name);
    else
        ImGui::TextDisabled("%s", "-");
    if (stats.cpu_name[0] != '\0')
        ImGui::TextDisabled("%s", stats.cpu_name);
    ImGui::End();
    ImGui::PopStyleVar(2);
    if (font != nullptr)
        ImGui::PopFont();
}

} // namespace bluewake_ui
