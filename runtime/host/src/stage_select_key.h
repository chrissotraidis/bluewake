#ifndef BLUEWAKE_STAGE_SELECT_KEY_H
#define BLUEWAKE_STAGE_SELECT_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

// F7 (mouse_camera.c's event handler), or the button in the settings menus:
// opens the developers' stage select from play, or goes back from it to where
// play was (stage_select.h, main.c).
void bluewake_stage_select_hotkey(void);

// What that does right now, for the menus' button.
enum {
    BW_STAGE_SELECT_OFF,     // the setting is off for this session
    BW_STAGE_SELECT_NO_FILE, // the title screen or file select: no file has started yet
    BW_STAGE_SELECT_OPEN,    // in play: opens the stage select
    BW_STAGE_SELECT_BACK,    // in the stage select, opened from play: goes back there
    BW_STAGE_SELECT_PICK,    // in the stage select, opened as a file started: only Start leaves it
};
int bluewake_stage_select_state(void);

#ifdef __cplusplus
}
#endif

#ifdef IMGUI_VERSION
// The settings menus' button (Mac, Linux and Windows), so a controller can do
// what F7 does: Back opens the menu, and the button opens the stage select or
// goes back. True when pressed; the menu then closes so the game runs on.
inline bool bw_stage_select_button() {
    const int state = bluewake_stage_select_state();
    if (state == BW_STAGE_SELECT_OFF)
        return false;
    const bool back = state == BW_STAGE_SELECT_BACK;
    const bool usable = state == BW_STAGE_SELECT_OPEN || back;
    ImGui::BeginDisabled(!usable);
    const bool pressed = ImGui::Button(back ? "Back to where I was (F7)" : "Go to the stage select (F7)");
    ImGui::EndDisabled();
    if (state == BW_STAGE_SELECT_NO_FILE)
        ImGui::TextDisabled("    Works once a file has started; or hold R as you start one.");
    else if (state == BW_STAGE_SELECT_PICK)
        ImGui::TextDisabled("    Opened as the file started: pick a place and press Start.");
    if (pressed)
        bluewake_stage_select_hotkey();
    return pressed;
}
#endif

#endif
