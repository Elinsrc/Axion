#pragma once
#include "custom_utils.h"

class CImGuiDebug
{
    CustomUtils m_CustomUtils;

public:
    void Init();
    void Draw();
};

extern CImGuiDebug g_ImGuiDebug;
