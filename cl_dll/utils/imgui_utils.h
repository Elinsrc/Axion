#pragma once

#include "imgui.h"
#include "rgb_color.h"
#include "hud.h"
#include "cl_util.h"

#include <stdint.h>
#include <vector>

static constexpr float DEFAULT_GIF_FRAME_DELAY_MS = 40.0f;

struct ImGuiImage
{
    ImTextureID texture = 0;
    int width = 0;
    int height = 0;
};

struct ImGuiGifFrame
{
    std::vector<uint8_t> pixels;
    float delayMs = 0.0f;
};

struct ImGuiGifImage
{
    int width = 0;
    int height = 0;
    std::vector<ImGuiGifFrame> frames;
};

class CImguiUtils
{
public:
    ImVec4 ColorFromCode(char code);
    void TextWithColorCodes(const char* text);
    void TextWithColorCodesCentered(const char *text);
    float CalcTextWidthWithColorCodes(const char* text, float fontSize = 0.0f);
    float DrawTextWithColorCodesAt(const ImVec2& pos, const char* text, ImVec4 defaultColor, float alphaMul = 1.0f);
    static void DrawCallback_SetAdditive(const ImDrawList* parent_list, const ImDrawCmd* cmd);
    static void DrawCallback_SetNormal(const ImDrawList* parent_list, const ImDrawCmd* cmd);
    static float ImGuiSpriteIcon(HLSPRITE hSprite, const wrect_t& rc, float x, float y, float iconWidth, float iconHeight, float textHeight, int r, int g, int b, int alpha);
    void HUEtoRGB(float hue, RGBColor &color);
    void DrawModelName(float topcolor, float bottomcolor, const char* model);
    void SetCvarFloat(const char* name, float value);
    void GetCvarColor(const char* name, float outColor[3]);
    void SetCvarColor(const char* name, const float color[3]);

    ImGuiImage LoadImageFromFile(const char* filename);
    ImGuiImage LoadImageFromMemory(const unsigned char* buffer, int bufferSize);
    ImGuiImage LoadImageFromRGBA(const unsigned char* rgba, int width, int height);
    bool LoadGifFromMemory(const unsigned char* buffer, int bufferSize, int targetSize, ImGuiGifImage& out, float defaultDelayMs = DEFAULT_GIF_FRAME_DELAY_MS);
    bool LoadStaticFromMemory(const unsigned char* buffer, int bufferSize, int targetSize, ImGuiGifImage& out);

    float DrawImage(const ImGuiImage& image, float x, float y, float rowHeight, float width, float height, int r = 255, int g = 255, int b = 255, int alpha = 255);
    void FreeImage(ImGuiImage& image);
    void FreeTexture(ImTextureID tex);

    void RenderColorCodeText(float fontSize, const ImVec2& pos, const char* text, ImVec4 color, bool shadow);
    void RenderText(float fontSize, const ImVec2& pos, const char* text, ImU32 color, bool shadow = false);
    void RenderTextCenter(float fontSize, const ImVec2& pos, const char* text, ImU32 color, bool shadow = false);

private:
    static void ResizeBilinearRGBA(const uint8_t* src, int srcW, int srcH, uint8_t* dst, int dstSize);
    static uint32_t CreateGLTexture(const unsigned char* rgba, int width, int height);
};

extern CImguiUtils m_ImguiUtils;