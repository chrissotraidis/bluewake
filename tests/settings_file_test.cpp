// The old Windows settings file, converted through settings_table.h
// (runtime/host/src/old_windows_settings.h): a player's every setting carries
// over, into the desktop format's keys and values.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include "../runtime/host/src/old_windows_settings.h"

using bw_settings::OldLines;

static OldLines lines_of(const char* text) {
    OldLines lines;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const size_t eq = line.find('=');
        if (!line.empty() && line[0] != '#' && eq != std::string::npos)
            lines.emplace_back(line.substr(0, eq), line.substr(eq + 1));
    }
    return lines;
}

static std::map<std::string, std::string> values_of(const OldLines& lines) {
    std::map<std::string, std::string> values;
    for (const auto& [key, value] : lines)
        assert(values.emplace(key, value).second);  // each key once
    return values;
}

int main() {
    const std::string pack = "C:\\Users\\p\\AppData\\Roaming\\BlueWake\\Load\\Textures\\GZLE01";

    // An old Windows file, a player's: every setting carries over.
    const char* old_file =
        "# BlueWake settings (the in-game menu, F1, writes this file)\n"
        "fullscreen=1\nwindow=1920x1080\nwindow_position=40,60\nrender_scale=3\nanisotropy=8\nsmooth_motion=1\n"
        "show_fps=1\npause_unfocused=1\nfps_position=top-left\ncompile_shaders_first=1\nsmooth_motion_fps=display\n"
        "mouse_camera=0\nmouse_sensitivity=0.35\nmouse_invert_y=1\ncontroller_swap_ab=1\ncontroller_swap_xy=1\n"
        "controller_swap_lr=1\ndpad_menus=1\nbutton_map=1,0,2,3,10,6\nmouse_buttons=a,b,x,y,z\nkey_map=13,14,24,12,20,8,21,40\n"
        "controller_invert_x=1\ncontroller_invert_y=1\nstick_camera=0\nstick_camera_speed=500\nstick_aim_speed=90\n"
        "stick_zoom=1\nstick_zoom_speed=0.750\naim_invert_y=1\ngyro_aim=1\ngyro_aim_speed=2.50\nclimb=1\n"
        "climb_stamina=20\nforest_water_keep_trees=1\nforest_water_30_minutes=1\nhd_look=1\n"
        "texture_pack=D:\\Packs\\WWHD\nhaptics=classic\nhaptics_strength=35\nhaptics_triggers=0\naspect=21:9\n"
        "keep_aspect=0\nbetterww=1\nhd_textures=1\nlle_audio=1\noption.swift_sail=1\noption.instant_text=0\n"
        "movement_extras=1\nsprint_speed=1.84\nfast_transitions=1\nquick_doors=1\nstage_select=1\n"
        "picto_crop=1\n"
        "unheard_of=1\n";
    OldLines player = lines_of(old_file);
    assert(bw_settings::convert_old_windows(player, pack));
    const auto v = values_of(player);
    const std::map<std::string, std::string> expected = {
        {"DOL_AURORA_FULLSCREEN", "1"}, {"DOL_AURORA_WINDOW", "1920x1080"}, {"BLUEWAKE_WINDOW_POSITION", "40,60"},
        {"DOL_AURORA_RENDER_SCALE", "3"}, {"DOL_AURORA_FORCE_ANISO", "8"}, {"DOL_AURORA_FRAME_INTERP", "1"},
        {"DOL_AURORA_SHOW_FPS", "1"}, {"BLUEWAKE_PAUSE_UNFOCUSED", "1"}, {"DOL_AURORA_FPS_POSITION", "top-left"},
        {"BLUEWAKE_SHADERS_FIRST", "1"}, {"DOL_AURORA_FRAME_INTERP_STEPS", "display"},
        {"BLUEWAKE_MOUSE_CAMERA", "0"}, {"BLUEWAKE_MOUSE_SENSITIVITY", "0.35"}, {"BLUEWAKE_MOUSE_INVERT_Y", "1"},
        {"BLUEWAKE_PAD_SWAP_AB", "1"}, {"BLUEWAKE_PAD_SWAP_XY", "1"}, {"BLUEWAKE_PAD_SWAP_LR", "1"},
        {"BLUEWAKE_DPAD_MENUS", "1"}, {"BLUEWAKE_BUTTON_MAP", "1,0,2,3,10,6"},
        {"BLUEWAKE_MOUSE_BUTTONS", "a,b,x,y,z"}, {"BLUEWAKE_KEY_MAP", "13,14,24,12,20,8,21,40"},
        {"BLUEWAKE_STICK_CAMERA_INVERT_X", "1"}, {"BLUEWAKE_STICK_CAMERA_INVERT_Y", "1"},
        {"BLUEWAKE_STICK_CAMERA", "0"}, {"BLUEWAKE_STICK_CAMERA_SPEED", "500"}, {"BLUEWAKE_STICK_AIM_SPEED", "90"},
        {"BLUEWAKE_STICK_ZOOM", "1"}, {"BLUEWAKE_STICK_ZOOM_SPEED", "0.750"}, {"BLUEWAKE_AIM_INVERT_Y", "1"},
        {"BLUEWAKE_GYRO_AIM", "1"}, {"BLUEWAKE_GYRO_AIM_SPEED", "2.50"}, {"BLUEWAKE_CLIMB", "1"},
        {"BLUEWAKE_CLIMB_STAMINA", "20"}, {"BLUEWAKE_FOREST_WATER_KEEP_TREES", "1"},
        {"BLUEWAKE_FOREST_WATER_30_MINUTES", "1"}, {"BLUEWAKE_HD_LOOK", "1"},
        {"DOL_AURORA_TEXTURE_PACK", "D:\\Packs\\WWHD"}, {"BLUEWAKE_TEXTURE_PACK_FOLDER", ""},
        {"BLUEWAKE_HAPTICS", "classic"}, {"BLUEWAKE_HAPTICS_STRENGTH", "35"}, {"BLUEWAKE_HAPTICS_TRIGGERS", "0"},
        {"BLUEWAKE_ASPECT", "21:9"}, {"DOL_AURORA_ASPECT_FIT", "0"}, {"BLUEWAKE_MODS", "betterww"},
        {"BLUEWAKE_DSP_MODE", "lle"}, {"BLUEWAKE_OPTIONS", "-instant_text,swift_sail"},
        {"BLUEWAKE_JUMP_BUTTON", "1"}, {"BLUEWAKE_SPRINT_SPEED", "1.84"}, {"BLUEWAKE_SPRINT_CHOICE", "1.84"},
        {"BLUEWAKE_FAST_FORWARD", "1"}, {"BLUEWAKE_FADE_FRAMES", "6"}, {"BLUEWAKE_QUICK_DOORS", "1"},
        {"BLUEWAKE_STAGE_SELECT_TOGGLE", "1"}, {"BLUEWAKE_PICTO_CROP", "1"}, {"unheard_of", "1"}};
    for (const auto& [key, value] : expected) {
        const auto found = v.find(key);
        assert(found != v.end() && found->second == value);
    }
    assert(v.size() == expected.size());
    // In the table's order, then the line nobody knew.
    int last = -1;
    for (const auto& [key, value] : player) {
        if (key == "unheard_of")
            break;
        const int at = static_cast<int>(bw_settings::find(key.c_str()) - bw_settings::kSettings);
        assert(at > last);
        last = at;
    }
    assert(player.back().first == "unheard_of");

    // HD textures off keep the folder for when they come back; on without a
    // folder of their own is the default one.
    OldLines off = lines_of("hd_textures=0\ntexture_pack=D:\\Packs\\WWHD\n");
    assert(bw_settings::convert_old_windows(off, pack));
    assert(values_of(off).at("DOL_AURORA_TEXTURE_PACK").empty());
    assert(values_of(off).at("BLUEWAKE_TEXTURE_PACK_FOLDER") == "D:\\Packs\\WWHD");
    OldLines default_folder = lines_of("hd_textures=1\n");
    assert(bw_settings::convert_old_windows(default_folder, pack));
    assert(values_of(default_folder).at("DOL_AURORA_TEXTURE_PACK") == pack);

    // Jump & Sprint off keeps the chosen speed; the host's own key is 1.
    OldLines walking = lines_of("movement_extras=0\nsprint_speed=1.84\n");
    assert(bw_settings::convert_old_windows(walking, pack));
    assert(values_of(walking).at("BLUEWAKE_JUMP_BUTTON") == "0" && values_of(walking).at("BLUEWAKE_SPRINT_SPEED") == "1");
    assert(values_of(walking).at("BLUEWAKE_SPRINT_CHOICE") == "1.84");

    // Values read as the old file read them: a switch's true/yes/on, a number
    // kept in its range, a choice it didn't know as the default one.
    OldLines loose = lines_of("climb=yes\nhd_look=true\nshow_fps=no\nrender_scale=9\nanisotropy=0\n"
                              "aspect=5:4\nhaptics=strong\nsmooth_motion_fps=120\n");
    assert(bw_settings::convert_old_windows(loose, pack));
    const auto l = values_of(loose);
    assert(l.at("BLUEWAKE_CLIMB") == "1" && l.at("BLUEWAKE_HD_LOOK") == "1" && l.at("DOL_AURORA_SHOW_FPS") == "0");
    assert(l.at("DOL_AURORA_RENDER_SCALE") == "4" && l.at("DOL_AURORA_FORCE_ANISO") == "1");
    assert(l.at("BLUEWAKE_ASPECT") == "4:3" && l.at("BLUEWAKE_HAPTICS") == "enhanced");
    assert(l.at("DOL_AURORA_FRAME_INTERP_STEPS") == "3");

    // A file in the desktop format (with a line nobody knows) is not old.
    OldLines current = lines_of("BLUEWAKE_ASPECT=16:9\nBLUEWAKE_OPTIONS=none,instant_text\nunheard_of=1\n");
    const OldLines before = current;
    assert(!bw_settings::convert_old_windows(current, pack));
    assert(current == before);

    // Every old key names a row, once (the two that became two settings aside).
    for (const bw_settings::Setting& s : bw_settings::kSettings) {
        if (s.old_windows == nullptr || !std::strcmp(s.old_windows, "fast_transitions") ||
            !std::strcmp(s.old_windows, "sprint_speed"))
            continue;
        assert(bw_settings::old_row(s.old_windows) == &s);
    }
    return 0;
}
