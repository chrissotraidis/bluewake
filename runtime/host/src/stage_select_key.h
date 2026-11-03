#ifndef BLUEWAKE_STAGE_SELECT_KEY_H
#define BLUEWAKE_STAGE_SELECT_KEY_H

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
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
// True once this session has used the stage select: saving is off until the next launch.
bool bluewake_stage_select_saving_off(void);
// True with --stage-select-saves (BLUEWAKE_STAGE_SELECT_SAVES=1): saving stays on.
bool bluewake_stage_select_saving_kept(void);
// The question a press in play asks before saving turns off: true once (the
// menu then shows it until it is answered); answering with go opens it.
bool bluewake_stage_select_take_question(void);
bool bluewake_stage_select_asking_now(void);
void bluewake_stage_select_answer(bool go);

#ifdef __cplusplus
}
#endif

#ifdef IMGUI_VERSION
// The question's window contents: 1 go, -1 cancel, 0 not answered yet.
// Cancel has the focus, so a stray A press keeps saving on.
inline int bw_stage_select_question() {
    if (!bluewake_stage_select_asking_now())
        return 1; // answered already (the hotkey)
    ImGui::TextUnformatted("Go to the stage select?");
    ImGui::Spacing();
    ImGui::TextUnformatted("Saving turns off until BlueWake restarts, so warping can't damage your file.");
    ImGui::TextUnformatted("Anything you haven't saved yet can't be saved after this.");
    ImGui::Spacing();
    int answer = 0;
    // The menu has the keyboard while it asks, so F7 is read here, not by the
    // hotkey; but only once the press that asked has been let go, or it would
    // answer its own question before it is seen.
    static bool f7_let_go;
    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
        f7_let_go = false;
    }
    const bool f7 = f7_let_go && ImGui::IsKeyPressed(ImGuiKey_F7, false);
    if (!ImGui::IsKeyDown(ImGuiKey_F7))
        f7_let_go = true;
    if (ImGui::Button("Cancel   (Esc)"))
        answer = -1;
    ImGui::SameLine();
    if (ImGui::Button("Go to the stage select   (F7)") || f7)
        answer = 1;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false))
        answer = -1;
    if (answer != 0)
        bluewake_stage_select_answer(answer > 0);
    return answer;
}

// The settings menus' warning under the stage select's toggle.
inline void bw_stage_select_save_warning() {
    if (bluewake_stage_select_saving_kept()) {
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f),
                           "    Saving stays on (--stage-select-saves): a save after warping can leave");
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f),
                           "    your file in a state the story never reaches. Back up your card first.");
        return;
    }
    if (bluewake_stage_select_saving_off()) {
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f),
                           "    Saving is off: the stage select was used. Restart BlueWake to save again.");
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f),
                           "    Until then the game's save screen says so; your card is fine.");
        return;
    }
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f),
                       "    Once you open the stage select, the game can't save until BlueWake restarts,");
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f),
                       "    so warping can't damage your file. Restarting brings it back as it was.");
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f),
                       "    Until then the game's save screen says why; your card is fine.");
}

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
