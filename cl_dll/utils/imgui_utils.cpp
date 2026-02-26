#include "imgui_utils.h"
#include <string>
#include <cmath>
    
#include "build.h"

#if XASH_MOBILE_PLATFORM || XASH_64BIT
#include "gl_export.h"
#else
#if XASH_WIN32
#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#include <GL/gl.h>
#elif XASH_LINUX
#include <GL/gl.h>
#endif
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

CImguiUtils m_ImguiUtils;

ImVec4 CImguiUtils::ColorFromCode(char code)
{
    switch(code)
    {
        case '0': return ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // Black
        case '1': return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
        case '2': return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
        case '3': return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
        case '4': return ImVec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
        case '5': return ImVec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
        case '6': return ImVec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta
        case '7': return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
        case '8': return ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // Black (same as 0)
        case '9': return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red (same as 1)
        default: return ImVec4(1.0f, 0.65f, 0.0f, 1.0f); // default orange
    }
}

void CImguiUtils::TextWithColorCodes(const char* text)
{
    bool hasColorCodes = false;
    for (const char* c = text; *c; c++)
    {
        if (*c == '^' && *(c + 1))
        {
            hasColorCodes = true;
            break;
        }
    }

    if (!hasColorCodes)
    {
        ImGui::TextUnformatted(text);
        return;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 cursor = pos;

    float lineHeight = ImGui::GetTextLineHeight();
    ImVec4 defaultColor = ImVec4(1.0f, 0.65f, 0.0f, 1.0f);

    ImVec4 color = defaultColor;
    const char* ptr = text;

    while (*ptr)
    {
        if (*ptr == '\n')
        {
            cursor.x = pos.x;
            cursor.y += lineHeight;
            ptr++;
            color = defaultColor;
            continue;
        }

        if (*ptr == '^' && *(ptr + 1))
        {
            color = ColorFromCode(*(ptr + 1));
            ptr += 2;
            continue;
        }

        const char* start = ptr;
        while (*ptr && *ptr != '\n' && !(*ptr == '^' && *(ptr + 1)))
            ptr++;

        std::string seg(start, ptr - start);

        if (!seg.empty())
        {
            draw->AddText(cursor, ImGui::ColorConvertFloat4ToU32(color), seg.c_str());
            cursor.x += ImGui::CalcTextSize(seg.c_str()).x;
        }
    }

    ImGui::Dummy(ImVec2(0, cursor.y - pos.y + lineHeight));
}

float CImguiUtils::CalcTextWidthWithColorCodes(const char* text)
{
    float width = 0.0f;
    const char* ptr = text;

    while (*ptr)
    {
        if (*ptr == '^' && *(ptr + 1))
        {
            ptr += 2;
            continue;
        }

        const char* start = ptr;
        while (*ptr && !(*ptr == '^' && *(ptr + 1)))
            ++ptr;

        if (ptr > start)
        {
            ImVec2 sz = ImGui::CalcTextSize(start, ptr);
            width += sz.x;
        }
    }

    return width;
}

float CImguiUtils::DrawTextWithColorCodesAt(const ImVec2& pos, const char* text, ImVec4 defaultColor, float alphaMul)
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    ImVec2 cursor = pos;
    ImVec4 color  = defaultColor;

    color.w *= alphaMul;
    const char* ptr = text;

    while (*ptr)
    {
        if (*ptr == '^' && *(ptr + 1))
        {
            color = ColorFromCode(*(ptr + 1));
            color.w *= alphaMul;
            ptr += 2;
            continue;
        }

        const char* start = ptr;
        while (*ptr && !(*ptr == '^' && *(ptr + 1)))
            ++ptr;

        if (ptr > start)
        {
            ImU32 colU32 = ImGui::ColorConvertFloat4ToU32(color);

            dl->AddText(cursor, colU32, start, ptr);

            ImVec2 sz = ImGui::CalcTextSize(start, ptr);
            cursor.x += sz.x;
        }
    }

    return cursor.x - pos.x;
}

void CImguiUtils::DrawCallback_SetAdditive(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}
void CImguiUtils::DrawCallback_SetNormal(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

float CImguiUtils::ImGuiSpriteIcon(HSPRITE hSprite, const wrect_t& rc, float x, float y, float iconWidth, float iconHeight, float textHeight, int r, int g, int b, int alpha)
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    SPR_Set(hSprite, 255, 255, 255);

    GLboolean oldColorMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, oldColorMask);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    SPR_DrawAdditive(0, 0, 0, &rc);

    glColorMask(oldColorMask[0], oldColorMask[1], oldColorMask[2], oldColorMask[3]);

    GLint boundTex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
    if (boundTex == 0)
        return x;

    GLint texW = 0, texH = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &texW);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);
    if (texW <= 0 || texH <= 0)
        return x;

    ImTextureID tex = (ImTextureID)(uintptr_t)boundTex;

    float u1 = (float)rc.left   / (float)texW;
    float v1 = (float)rc.top    / (float)texH;
    float u2 = (float)rc.right  / (float)texW;
    float v2 = (float)rc.bottom / (float)texH;

    float iconY = y + (textHeight - iconHeight) * 0.5f;
    ImVec2 p_min(x, iconY);
    ImVec2 p_max(x + iconWidth, iconY + iconHeight);

    ImU32 col = IM_COL32(r, g, b, alpha);

    dl->AddCallback(DrawCallback_SetAdditive, nullptr);
    dl->AddImage(tex, p_min, p_max, ImVec2(u1, v1), ImVec2(u2, v2), col);
    dl->AddCallback(DrawCallback_SetNormal, nullptr);

    return x + iconWidth;
}

void CImguiUtils::HUEtoRGB(float hue, RGBColor &color)
{
    hue = fmax(0, fmin(255, hue));
    float h = hue / 255.0f;
    float r, g, b;

    if (h < 1.0f/6.0f) {
        r = 1.0f;
        g = h * 6.0f;
        b = 0.0f;
    }
    else if (h < 2.0f/6.0f) {
        r = 1.0f - (h - 1.0f/6.0f) * 6.0f;
        g = 1.0f;
        b = 0.0f;
    }
    else if (h < 3.0f/6.0f) {
        r = 0.0f;
        g = 1.0f;
        b = (h - 2.0f/6.0f) * 6.0f;
    }
    else if (h < 4.0f/6.0f) {
        r = 0.0f;
        g = 1.0f - (h - 3.0f/6.0f) * 6.0f;
        b = 1.0f;
    }
    else if (h < 5.0f/6.0f) {
        r = (h - 4.0f/6.0f) * 6.0f;
        g = 0.0f;
        b = 1.0f;
    }
    else {
        r = 1.0f;
        g = 0.0f;
        b = 1.0f - (h - 5.0f/6.0f) * 6.0f;
    }

    color.r = static_cast<int>(r * 255);
    color.g = static_cast<int>(g * 255);
    color.b = static_cast<int>(b * 255);
}

void CImguiUtils::DrawModelName(float topcolor, float bottomcolor, const char* model)
{
    if (!model)
        return;

    size_t modelLength = strlen(model);
    size_t mid = modelLength / 2;

    std::string firstcolor(model, mid);
    std::string secondcolor(model + mid, modelLength - mid);

    RGBColor top, bottom;
    HUEtoRGB(topcolor, top);
    HUEtoRGB(bottomcolor, bottom);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), pos, IM_COL32(top.r, top.g, top.b, 255), firstcolor.c_str());

    float firstWidth = ImGui::CalcTextSize(firstcolor.c_str()).x;
    pos.x += firstWidth;

    draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), pos, IM_COL32(bottom.r, bottom.g, bottom.b, 255), secondcolor.c_str());
}

void CImguiUtils::SetCvarFloat(const char* name, float value)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "%s %f", name, value);
    gEngfuncs.pfnClientCmd(cmd);
}

void CImguiUtils::GetCvarColor(const char* name, float outColor[3])
{
    const char* str = gEngfuncs.pfnGetCvarString(name);
    int r, g, b;

    sscanf(str, "%d %d %d", &r, &g, &b);

    outColor[0] = r / 255.0f;
    outColor[1] = g / 255.0f;
    outColor[2] = b / 255.0f;
}

void CImguiUtils::SetCvarColor(const char* name, const float color[3])
{
    int r = (int)(color[0] * 255.0f + 0.5f);
    int g = (int)(color[1] * 255.0f + 0.5f);
    int b = (int)(color[2] * 255.0f + 0.5f);

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "%s \"%d %d %d\"", name, r, g, b);
    gEngfuncs.pfnClientCmd(cmd);
}

ImGuiImage CImguiUtils::LoadImageFromFile(const char* filename)
{
    ImGuiImage result;

    char Path[512];
    snprintf(Path, sizeof(Path), "%s/%s", gEngfuncs.pfnGetGameDirectory(), filename);

    int channels;
    unsigned char* data = stbi_load(Path, &result.width, &result.height, &channels, 4);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, result.width, result.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    result.texture = (ImTextureID)(intptr_t)texture;
    return result;
}

ImGuiImage CImguiUtils::LoadImageFromMemory(const unsigned char* buffer, int bufferSize)
{
    ImGuiImage result;

    int channels;
    unsigned char* data = stbi_load_from_memory(buffer, bufferSize, &result.width, &result.height, &channels, 4);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, result.width, result.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    result.texture = (ImTextureID)(intptr_t)texture;
    return result;
}

float CImguiUtils::DrawImage(const ImGuiImage& image, float x, float y, float rowHeight, float width, float height, int r, int g, int b, int alpha)
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    float iconY = y + (rowHeight - height) * 0.5f;

    ImVec2 p_min(x, iconY);
    ImVec2 p_max(x + width, iconY + height);

    ImU32 col = IM_COL32(r, g, b, alpha);

    dl->AddImage(image.texture, p_min, p_max, ImVec2(0,0), ImVec2(1,1), col);

    return x + width;
}

void CImguiUtils::FreeImage(ImGuiImage& image)
{
    if (image.texture != (ImTextureID)0)
    {
        GLuint tex = (GLuint)(uintptr_t)image.texture;
        glDeleteTextures(1, &tex);
        image.texture = (ImTextureID)0;
        image.width = image.height = 0;
    }
}
