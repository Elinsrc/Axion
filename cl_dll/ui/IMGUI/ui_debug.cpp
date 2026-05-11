#include "hud.h"
#include "cl_util.h"
#include "imgui.h"
#include "ui_debug.h"
#include "imgui_viewport.h"
#include "imgui_utils.h"

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
    float font_scale = CVAR_GET_FLOAT("ui_imgui_font_scale");
    const float fixedPadding = 2.0f;
    float lineHeight = font_scale + fixedPadding;

    float xpos = g_ImGuiViewport.scrWidth() / 1.4f;
    float ypos = g_ImGuiViewport.scrHeight() / 50.0f;

    int DebugMode = cl_debug->value;
    float fps = m_CustomUtils.GetFrameTime();

    const unsigned char* fps_color = getColorForFPS(static_cast<int>(fps));
    ImU32 col_fps = IM_COL32(fps_color[0], fps_color[1], fps_color[2], 255);

    char str[256];
    float currentY = ypos;

    if (DebugMode > 0 || cl_debug_showfps->value > 0.0f)
    {
        sprintf(str, "FPS: %.0f", fps);
        m_ImguiUtils.DrawTextShadow(font_scale, ImVec2(xpos, currentY), str, col_fps);
        currentY += lineHeight;

        sprintf(str, "Frame Time: %.0f ms", 1000.f / fps);
        m_ImguiUtils.DrawTextShadow(font_scale, ImVec2(xpos, currentY), str, col_fps);
        currentY += lineHeight * 2.0f;
    }

    ImU32 col_sys = IM_COL32(colors[3][0], colors[3][1], colors[3][2], 255);

    if (DebugMode > 0)
    {
        const char* vendor = (const char *)(glGetString(GL_VENDOR));
        const char* renderer = (const char *)(glGetString(GL_RENDERER));
        const char* version = (const char *)(glGetString(GL_VERSION));

        sprintf(str, "GL_VENDOR: %s", vendor);
        m_ImguiUtils.DrawTextShadow(font_scale, ImVec2(xpos, currentY), str, col_sys);
        currentY += lineHeight;

        sprintf(str, "GL_RENDERER: %s", renderer);
        m_ImguiUtils.DrawTextShadow(font_scale, ImVec2(xpos, currentY), str, col_sys);
        currentY += lineHeight;

        sprintf(str, "GL_VERSION: %s", version);
        m_ImguiUtils.DrawTextShadow(font_scale, ImVec2(xpos, currentY), str, col_sys);
        currentY += lineHeight;

        sprintf(str, "MODE: %dx%d", g_ImGuiViewport.scrWidth(), g_ImGuiViewport.scrHeight());
        m_ImguiUtils.DrawTextShadow(font_scale, ImVec2(xpos, currentY), str, col_sys);
    }
}