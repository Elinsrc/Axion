#pragma once
#include "imgui.h"
#include "rgb_color.h"
#include "hud.h"
#include "cl_util.h"

struct ImGuiImage
{
    ImTextureID texture = 0;
    int width = 0;
    int height = 0;
};

class CImguiUtils
{
public:
    ImVec4 ColorFromCode(char code);
    void TextWithColorCodes(const char* text);
    float CalcTextWidthWithColorCodes(const char* text);
    float DrawTextWithColorCodesAt(const ImVec2& pos, const char* text, ImVec4 defaultColor, float alphaMul = 1.0f);
    static void DrawCallback_SetAdditive(const ImDrawList* parent_list, const ImDrawCmd* cmd);
    static void DrawCallback_SetNormal(const ImDrawList* parent_list, const ImDrawCmd* cmd);
    static float ImGuiSpriteIcon(HSPRITE hSprite, const wrect_t& rc, float x, float y, float iconWidth, float iconHeight, float textHeight, int r, int g, int b, int alpha);
    void HUEtoRGB(float hue, RGBColor &color);
    void DrawModelName(float topcolor, float bottomcolor, const char* model);
    void SetCvarFloat(const char* name, float value);
    void GetCvarColor(const char* name, float outColor[3]);
    void SetCvarColor(const char* name, const float color[3]);
    void StyleColorVGUI(void);
    ImGuiImage LoadImageFromFile(const char* filename);
    float DrawImage(const ImGuiImage& image, float x, float y, float rowHeight, float width, float height, int r = 255, int g = 255, int b = 255, int alpha = 255);
    void FreeImage(ImGuiImage& image);
};

extern CImguiUtils m_ImguiUtils;
