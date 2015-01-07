// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/common.h"

#include "video_core/video_core.h"

#include "core/settings.h"

#include "emu_window/emu_window_android.h"

EmuWindow_Android* g_emuWindow_Android;

EmuWindow_Android* EmuWindow_Android::GetEmuWindow() {
    return g_emuWindow_Android;
}

/// EmuWindow_Android constructor
EmuWindow_Android::EmuWindow_Android() {
    g_emuWindow_Android = this;
/*    keyboard_id = KeyMap::NewDeviceId();

    ReloadSetKeymaps();

    glfwSetErrorCallback([](int error, const char *desc){
        LOG_ERROR(Frontend, "GLFW 0x%08x: %s", error, desc);
    });

    // Initialize the window
    if(glfwInit() != GL_TRUE) {
        LOG_CRITICAL(Frontend, "Failed to initialize GLFW! Exiting...");
        exit(1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    // GLFW on OSX requires these window hints to be set to create a 3.2+ GL context.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::string window_title = Common::StringFromFormat("Citra | %s-%s", Common::g_scm_branch, Common::g_scm_desc);
    m_render_window = glfwCreateWindow(VideoCore::kScreenTopWidth,
        (VideoCore::kScreenTopHeight + VideoCore::kScreenBottomHeight),
        window_title.c_str(), nullptr, nullptr);

    if (m_render_window == nullptr) {
        LOG_CRITICAL(Frontend, "Failed to create GLFW window! Exiting...");
        exit(1);
    }

    glfwSetWindowUserPointer(m_render_window, this);

    // Notify base interface about window state
    int width, height;
    glfwGetFramebufferSize(m_render_window, &width, &height);
    OnFramebufferResizeEvent(m_render_window, width, height);

    glfwGetWindowSize(m_render_window, &width, &height);
    OnClientAreaResizeEvent(m_render_window, width, height);

    // Setup callbacks
    glfwSetKeyCallback(m_render_window, OnKeyEvent);
    glfwSetFramebufferSizeCallback(m_render_window, OnFramebufferResizeEvent);
    glfwSetWindowSizeCallback(m_render_window, OnClientAreaResizeEvent);

    DoneCurrent();
*/
}

/// EmuWindow_Android destructor
EmuWindow_Android::~EmuWindow_Android() {
//    glfwTerminate();
}

/// Swap buffers to display the next frame
void EmuWindow_Android::SwapBuffers() {
    eglSwapBuffers(egl_dpy, egl_surf);
}

/// Polls window events
void EmuWindow_Android::PollEvents() {
}

/// Makes the GLFW OpenGL context current for the caller thread
void EmuWindow_Android::MakeCurrent() {
    eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx);
}

/// Releases (dunno if this is the "right" word) the GLFW context from the caller thread
void EmuWindow_Android::DoneCurrent() {
    eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void EmuWindow_Android::ReloadSetKeymaps() {
}
