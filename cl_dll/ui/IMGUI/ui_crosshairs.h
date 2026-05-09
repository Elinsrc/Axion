#pragma once

#include "imgui_window.h"

class CImGuiCrosshairs
{
public:
    static bool m_ShowCrosshairs;
    void Init();
    void Draw();
};

extern CImGuiCrosshairs g_ImGuiCrosshairs;
