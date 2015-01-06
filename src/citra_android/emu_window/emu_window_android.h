// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/emu_window.h"

struct GLFWwindow;

class EmuWindow_Android : public EmuWindow {
public:
    EmuWindow_Android();
    ~EmuWindow_Android();

    /// Swap buffers to display the next frame
    void SwapBuffers() override;

    /// Polls window events
    void PollEvents() override;

    /// Makes the graphics context current for the caller thread
    void MakeCurrent() override;

    /// Releases (dunno if this is the "right" word) the GL context from the caller thread
    void DoneCurrent() override;

    void ReloadSetKeymaps() override;

    static EmuWindow_Android* GetEmuWindow(GLFWwindow* win);
};
