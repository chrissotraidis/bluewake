// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Relaunch is consumed by win_entry only after the host has returned.
struct RestartRequest {
    bool pending = false;
    template<class PushQuit> bool request(bool saved, PushQuit push_quit) {
        if (!saved || !push_quit()) return false;
        pending = true;
        return true;
    }
    bool take() { bool result = pending; pending = false; return result; }
};
