#include "hud.h"
#include "cl_util.h"
#include "imgui.h"
#include "imgui_debug.h"
#include "imgui_viewport.h"

cvar_t* cl_debug;
cvar_t* cl_debug_showfps;

CImGuiDebug g_ImGuiDebug;

void CImGuiDebug::Init()
{
    cl_debug = CVAR_CREATE("cl_debug", "0", FCVAR_ARCHIVE);
    cl_debug_showfps = CVAR_CREATE("cl_debug_showfps", "1", FCVAR_ARCHIVE);
}

static const unsigned char colors[4][3] = {
    {255, 0, 0},
    {255, 255, 0},
    {0, 255, 0},
    {255, 255, 255}
};


const unsigned char* getColorForFPS(int fps)
{
    if (fps > 59)
        return colors[2];
    else if (fps > 29)
        return colors[1];
    else
        return colors[0];
}

void CImGuiDebug::Draw()
{
    int xpos = (g_ImGuiViewport.scrWidth() / 1.4f);
    int ypos = g_ImGuiViewport.scrHeight() / 50;

    int DebugMode = cl_debug->value;
    float fps = m_CustomUtils.GetFrameTime();

    const unsigned char* color = getColorForFPS(static_cast<int>(fps));
    ImU32 col = IM_COL32(color[0], color[1], color[2], 255);

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    char str[256];

    if (DebugMode > 0.0f || cl_debug_showfps->value > 0.0f)
    {
        sprintf(str, "FPS: %.0f", fps);
        draw->AddText(ImVec2(xpos, ypos),col,str);

        sprintf(str, "Frame Time: %.0f ms", 1000.f / fps);
        draw->AddText(ImVec2(xpos, ypos * 2), col, str);
    }

    col = IM_COL32(colors[3][0], colors[3][1], colors[3][2], 255);

    if (DebugMode > 0.0f)
    {
        const char* vendor = (const char *)(glGetString(GL_VENDOR));
        const char* renderer = (const char *)(glGetString(GL_RENDERER));
        const char* version = (const char *)(glGetString(GL_VERSION));

        sprintf(str, "GL_VENDOR: %s", vendor);
        draw->AddText(ImVec2(xpos, ypos * 4), col, str);

        sprintf(str, "GL_RENDERER: %s", renderer);
        draw->AddText(ImVec2(xpos, ypos * 5), col, str);

        sprintf(str, "GL_VERSION: %s", version);
        draw->AddText(ImVec2(xpos, ypos * 6), col, str);

        sprintf(str, "MODE: %dx%d", g_ImGuiViewport.scrWidth(), g_ImGuiViewport.scrHeight());
        draw->AddText(ImVec2(xpos, ypos * 9), col, str);
    }
}


