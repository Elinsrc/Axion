#pragma once
#include "imgui_window_system.h"
#include "cursor_type.h"
#include <map>
#include <memory>
#include "gl_imgui_backend.h"
#include "ui_crosshairs.h"
#include "ui_debug.h"

class CImGuiBackend;

class CImGuiManager
{
public:
    static CImGuiManager &GetInstance();

    void Initialize();
    void VidInitialize();
    void Terminate();
    void NewFrame();
    bool KeyInput(bool keyDown, int keyNumber, const char *bindName);
    bool IsCursorRequired();

#if __ANDROID__
    void TouchEvent(int fingerID, float x, float y, float dx, float dy);
    int m_TouchID = -1;
    float m_TouchX = 0.0f;
    float m_TouchY = 0.0f;
    float m_TouchDX = 0.0f;
    float m_TouchDY = 0.0f;
#endif

    ImFont* GetDefaultFont() const { return m_pDefaultFont; }
    ImFont* GetHudFont() const { return m_pHudFont; }

private:
    struct MouseButtonsState
    {
        bool left = false;
        bool middle = false;
        bool right = false;
    };

    CImGuiManager();
    ~CImGuiManager();
    CImGuiManager(const CImGuiManager &) = delete;
    CImGuiManager &operator=(const CImGuiManager &) = delete;

    void LoadFonts();
    void ApplyStyles();
    void UpdateMouseState();
    void UpdateCursorState();
    void HandleKeyInput(bool keyDown, int keyNumber);
    bool HandleMouseInput(bool keyDown, int keyNumber);
    void SetupConfig();
    void SetupKeyboardMapping();

    bool m_bWasCursorRequired = false;
    MouseButtonsState m_MouseButtonsState;
    std::map<int, int> m_KeysMapping;
    std::unique_ptr<CImGuiBackend> m_pBackend;
    CImGuiWindowSystem m_WindowSystem;
    CImGuiCrosshairs g_ImGuiCrosshairs;
    CImGuiDebug g_ImGuiDebug;

    ImFont* m_pDefaultFont = nullptr;
    ImFont* m_pHudFont     = nullptr;
};
extern CImGuiManager &g_ImGuiManager;
