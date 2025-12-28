#include "ui_custom_menu.h"

#include "hud.h"
#include "keydefs.h"
#include "cl_util.h"
#include "imgui_utils.h"
#include "imgui_viewport.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include <unordered_map>

CImGuiCustomMenu g_iCustomMenu;

bool CImGuiCustomMenu::m_ShowCustomMenu = false;

static void TrimRight(char* s)
{
    size_t len = std::strlen(s);
    while (len > 0 &&
        (s[len - 1] == '\n' || s[len - 1] == '\r' ||
        s[len - 1] == ' '  || s[len - 1] == '\t'))
    {
        s[--len] = '\0';
    }
}

static void TrimLeft(std::string& s)
{
    size_t pos = s.find_first_not_of(" \t");
    if (pos == std::string::npos)
        s.clear();
    else if (pos > 0)
        s.erase(0, pos);
}

static bool StartsWith(const std::string& s, const char* what)
{
    size_t n = std::strlen(what);
    return s.size() >= n && s.compare(0, n, what) == 0;
}

static void ExecMenuCmd_f()
{
    g_iCustomMenu.ExecMenu_f();
}

ImGuiStyleVar CImGuiCustomMenu::GetStyleVarIndex(const std::string& name)
{
    static const std::unordered_map<std::string, ImGuiStyleVar> vars = {
        {"Alpha", ImGuiStyleVar_Alpha},
        {"DisabledAlpha", ImGuiStyleVar_DisabledAlpha},
        {"WindowRounding", ImGuiStyleVar_WindowRounding},
        {"WindowBorderSize", ImGuiStyleVar_WindowBorderSize},
        {"ChildRounding", ImGuiStyleVar_ChildRounding},
        {"ChildBorderSize", ImGuiStyleVar_ChildBorderSize},
        {"PopupRounding", ImGuiStyleVar_PopupRounding},
        {"PopupBorderSize", ImGuiStyleVar_PopupBorderSize},
        {"FrameRounding", ImGuiStyleVar_FrameRounding},
        {"FrameBorderSize", ImGuiStyleVar_FrameBorderSize},
        {"IndentSpacing", ImGuiStyleVar_IndentSpacing},
        {"ScrollbarSize", ImGuiStyleVar_ScrollbarSize},
        {"ScrollbarRounding", ImGuiStyleVar_ScrollbarRounding},
        {"ScrollbarPadding", ImGuiStyleVar_ScrollbarPadding},
        {"GrabMinSize", ImGuiStyleVar_GrabMinSize},
        {"GrabRounding", ImGuiStyleVar_GrabRounding},
        {"ImageBorderSize", ImGuiStyleVar_ImageBorderSize},
        {"TabRounding", ImGuiStyleVar_TabRounding},
        {"TabBorderSize", ImGuiStyleVar_TabBorderSize},
        {"TabMinWidthBase", ImGuiStyleVar_TabMinWidthBase},
        {"TabMinWidthShrink", ImGuiStyleVar_TabMinWidthShrink},
        {"TabBarBorderSize", ImGuiStyleVar_TabBarBorderSize},
        {"TabBarOverlineSize", ImGuiStyleVar_TabBarOverlineSize},
        {"TableAngledHeadersAngle", ImGuiStyleVar_TableAngledHeadersAngle},
        {"TreeLinesSize", ImGuiStyleVar_TreeLinesSize},
        {"TreeLinesRounding", ImGuiStyleVar_TreeLinesRounding},
        {"SeparatorTextBorderSize", ImGuiStyleVar_SeparatorTextBorderSize},
        {"WindowPadding", ImGuiStyleVar_WindowPadding},
        {"WindowMinSize", ImGuiStyleVar_WindowMinSize},
        {"WindowTitleAlign", ImGuiStyleVar_WindowTitleAlign},
        {"FramePadding", ImGuiStyleVar_FramePadding},
        {"ItemSpacing", ImGuiStyleVar_ItemSpacing},
        {"ItemInnerSpacing", ImGuiStyleVar_ItemInnerSpacing},
        {"CellPadding", ImGuiStyleVar_CellPadding},
        {"TableAngledHeadersTextAlign", ImGuiStyleVar_TableAngledHeadersTextAlign},
        {"ButtonTextAlign", ImGuiStyleVar_ButtonTextAlign},
        {"SelectableTextAlign", ImGuiStyleVar_SelectableTextAlign},
        {"SeparatorTextAlign", ImGuiStyleVar_SeparatorTextAlign},
        {"SeparatorTextPadding", ImGuiStyleVar_SeparatorTextPadding},
    };

    auto it = vars.find(name);
    return (it != vars.end()) ? it->second : (ImGuiStyleVar)-1;
}

ImGuiCol CImGuiCustomMenu::GetColorIndex(const std::string& name)
{
    static const std::unordered_map<std::string, ImGuiCol> colors = {
        {"Text", ImGuiCol_Text},
        {"TextDisabled", ImGuiCol_TextDisabled},
        {"WindowBg", ImGuiCol_WindowBg},
        {"ChildBg", ImGuiCol_ChildBg},
        {"PopupBg", ImGuiCol_PopupBg},
        {"Border", ImGuiCol_Border},
        {"BorderShadow", ImGuiCol_BorderShadow},
        {"FrameBg", ImGuiCol_FrameBg},
        {"FrameBgHovered", ImGuiCol_FrameBgHovered},
        {"FrameBgActive", ImGuiCol_FrameBgActive},
        {"TitleBg", ImGuiCol_TitleBg},
        {"TitleBgActive", ImGuiCol_TitleBgActive},
        {"TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
        {"MenuBarBg", ImGuiCol_MenuBarBg},
        {"ScrollbarBg", ImGuiCol_ScrollbarBg},
        {"ScrollbarGrab", ImGuiCol_ScrollbarGrab},
        {"ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
        {"ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
        {"CheckMark", ImGuiCol_CheckMark},
        {"SliderGrab", ImGuiCol_SliderGrab},
        {"SliderGrabActive", ImGuiCol_SliderGrabActive},
        {"Button", ImGuiCol_Button},
        {"ButtonHovered", ImGuiCol_ButtonHovered},
        {"ButtonActive", ImGuiCol_ButtonActive},
        {"Header", ImGuiCol_Header},
        {"HeaderHovered", ImGuiCol_HeaderHovered},
        {"HeaderActive", ImGuiCol_HeaderActive},
        {"Separator", ImGuiCol_Separator},
        {"SeparatorHovered", ImGuiCol_SeparatorHovered},
        {"SeparatorActive", ImGuiCol_SeparatorActive},
        {"ResizeGrip", ImGuiCol_ResizeGrip},
        {"ResizeGripHovered", ImGuiCol_ResizeGripHovered},
        {"ResizeGripActive", ImGuiCol_ResizeGripActive},
        {"InputTextCursor", ImGuiCol_InputTextCursor},
        {"TabHovered", ImGuiCol_TabHovered},
        {"Tab", ImGuiCol_Tab},
        {"TabSelected", ImGuiCol_TabSelected},
        {"TabSelectedOverline", ImGuiCol_TabSelectedOverline},
        {"TabDimmed", ImGuiCol_TabDimmed},
        {"TabDimmedSelected", ImGuiCol_TabDimmedSelected},
        {"TabDimmedSelectedOverline", ImGuiCol_TabDimmedSelectedOverline},
        {"PlotLines", ImGuiCol_PlotLines},
        {"PlotLinesHovered", ImGuiCol_PlotLinesHovered},
        {"PlotHistogram", ImGuiCol_PlotHistogram},
        {"PlotHistogramHovered", ImGuiCol_PlotHistogramHovered},
        {"TableHeaderBg", ImGuiCol_TableHeaderBg},
        {"TableBorderStrong", ImGuiCol_TableBorderStrong},
        {"TableBorderLight", ImGuiCol_TableBorderLight},
        {"TableRowBg", ImGuiCol_TableRowBg},
        {"TableRowBgAlt", ImGuiCol_TableRowBgAlt},
        {"TextLink", ImGuiCol_TextLink},
        {"TextSelectedBg", ImGuiCol_TextSelectedBg},
        {"TreeLines", ImGuiCol_TreeLines},
        {"DragDropTarget", ImGuiCol_DragDropTarget},
        {"UnsavedMarker", ImGuiCol_UnsavedMarker},
        {"NavCursor", ImGuiCol_NavCursor},
        {"NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
        {"NavWindowingDimBg", ImGuiCol_NavWindowingDimBg},
        {"ModalWindowDimBg", ImGuiCol_ModalWindowDimBg},
    };

    auto it = colors.find(name);
    return (it != colors.end()) ? it->second : (ImGuiCol)-1;
}


void CImGuiCustomMenu::SetupDefaultMenuStyle()
{
    auto addColor = [&](ImGuiCol idx, float r, float g, float b, float a)
    {
        MenuStyleColor sc;
        sc.idx   = idx;
        sc.color = ImVec4(r, g, b, a);
        m_StyleColors.push_back(sc);
    };

    auto addVar = [&](ImGuiStyleVar idx, float v)
    {
        MenuStyleVar sv;
        sv.idx    = idx;
        sv.isVec2 = false;
        sv.value  = ImVec2(v, 0.0f);
        m_StyleVars.push_back(sv);
    };

    auto addVar2 = [&](ImGuiStyleVar idx, float x, float y)
    {
        MenuStyleVar sv;
        sv.idx    = idx;
        sv.isVec2 = true;
        sv.value  = ImVec2(x, y);
        m_StyleVars.push_back(sv);
    };

    // ===============================
    // IMGUI STYLE (COLORS)
    // ===============================

    addColor(ImGuiCol_Text,                     1.00f, 1.00f, 1.00f, 1.00f);
    addColor(ImGuiCol_TextDisabled,             0.50f, 0.50f, 0.50f, 1.00f);

    addColor(ImGuiCol_WindowBg,                 0.06f, 0.06f, 0.06f, 0.94f);
    addColor(ImGuiCol_ChildBg,                  0.00f, 0.00f, 0.00f, 0.00f);
    addColor(ImGuiCol_PopupBg,                  0.08f, 0.08f, 0.08f, 0.94f);

    addColor(ImGuiCol_Border,                   0.43f, 0.43f, 0.50f, 0.50f);
    addColor(ImGuiCol_BorderShadow,             0.00f, 0.00f, 0.00f, 0.00f);

    addColor(ImGuiCol_FrameBg,                  0.16f, 0.29f, 0.48f, 0.54f);
    addColor(ImGuiCol_FrameBgHovered,           0.26f, 0.59f, 0.98f, 0.40f);
    addColor(ImGuiCol_FrameBgActive,            0.26f, 0.59f, 0.98f, 0.67f);

    addColor(ImGuiCol_TitleBg,                  0.04f, 0.04f, 0.04f, 1.00f);
    addColor(ImGuiCol_TitleBgActive,            0.16f, 0.29f, 0.48f, 1.00f);
    addColor(ImGuiCol_TitleBgCollapsed,         0.00f, 0.00f, 0.00f, 0.51f);

    addColor(ImGuiCol_MenuBarBg,                0.14f, 0.14f, 0.14f, 1.00f);

    addColor(ImGuiCol_ScrollbarBg,              0.02f, 0.02f, 0.02f, 0.53f);
    addColor(ImGuiCol_ScrollbarGrab,            0.31f, 0.31f, 0.31f, 1.00f);
    addColor(ImGuiCol_ScrollbarGrabHovered,     0.41f, 0.41f, 0.41f, 1.00f);
    addColor(ImGuiCol_ScrollbarGrabActive,      0.51f, 0.51f, 0.51f, 1.00f);

    addColor(ImGuiCol_CheckMark,                0.26f, 0.59f, 0.98f, 1.00f);
    addColor(ImGuiCol_SliderGrab,               0.24f, 0.52f, 0.88f, 1.00f);
    addColor(ImGuiCol_SliderGrabActive,         0.26f, 0.59f, 0.98f, 1.00f);

    addColor(ImGuiCol_Button,                   0.26f, 0.59f, 0.98f, 0.40f);
    addColor(ImGuiCol_ButtonHovered,            0.26f, 0.59f, 0.98f, 1.00f);
    addColor(ImGuiCol_ButtonActive,             0.06f, 0.53f, 0.98f, 1.00f);

    addColor(ImGuiCol_Header,                   0.26f, 0.59f, 0.98f, 0.31f);
    addColor(ImGuiCol_HeaderHovered,            0.26f, 0.59f, 0.98f, 0.80f);
    addColor(ImGuiCol_HeaderActive,             0.26f, 0.59f, 0.98f, 1.00f);

    addColor(ImGuiCol_Separator,                0.43f, 0.43f, 0.50f, 0.50f);
    addColor(ImGuiCol_SeparatorHovered,         0.10f, 0.40f, 0.75f, 0.78f);
    addColor(ImGuiCol_SeparatorActive,          0.10f, 0.40f, 0.75f, 1.00f);

    addColor(ImGuiCol_ResizeGrip,               0.26f, 0.59f, 0.98f, 0.20f);
    addColor(ImGuiCol_ResizeGripHovered,        0.26f, 0.59f, 0.98f, 0.67f);
    addColor(ImGuiCol_ResizeGripActive,         0.26f, 0.59f, 0.98f, 0.95f);

    addColor(ImGuiCol_InputTextCursor,          1.00f, 1.00f, 1.00f, 1.00f);

    addColor(ImGuiCol_TabHovered,               0.26f, 0.59f, 0.98f, 0.80f);
    addColor(ImGuiCol_Tab,                      0.18f, 0.35f, 0.58f, 0.86f);
    addColor(ImGuiCol_TabSelected,              0.20f, 0.41f, 0.68f, 1.00f);
    addColor(ImGuiCol_TabSelectedOverline,      0.26f, 0.59f, 0.98f, 1.00f);
    addColor(ImGuiCol_TabDimmed,                0.07f, 0.10f, 0.15f, 0.97f);
    addColor(ImGuiCol_TabDimmedSelected,        0.14f, 0.26f, 0.42f, 1.00f);
    addColor(ImGuiCol_TabDimmedSelectedOverline,0.50f, 0.50f, 0.50f, 0.00f);

    addColor(ImGuiCol_PlotLines,                0.61f, 0.61f, 0.61f, 1.00f);
    addColor(ImGuiCol_PlotLinesHovered,         1.00f, 0.43f, 0.35f, 1.00f);
    addColor(ImGuiCol_PlotHistogram,            0.90f, 0.70f, 0.00f, 1.00f);
    addColor(ImGuiCol_PlotHistogramHovered,     1.00f, 0.60f, 0.00f, 1.00f);

    addColor(ImGuiCol_TableHeaderBg,            0.19f, 0.19f, 0.20f, 1.00f);
    addColor(ImGuiCol_TableBorderStrong,        0.31f, 0.31f, 0.35f, 1.00f);
    addColor(ImGuiCol_TableBorderLight,         0.23f, 0.23f, 0.25f, 1.00f);
    addColor(ImGuiCol_TableRowBg,               0.00f, 0.00f, 0.00f, 0.00f);
    addColor(ImGuiCol_TableRowBgAlt,            1.00f, 1.00f, 1.00f, 0.06f);

    addColor(ImGuiCol_TextLink,                 0.26f, 0.59f, 0.98f, 1.00f);
    addColor(ImGuiCol_TextSelectedBg,           0.26f, 0.59f, 0.98f, 0.35f);

    addColor(ImGuiCol_TreeLines,                0.43f, 0.43f, 0.50f, 0.50f);
    addColor(ImGuiCol_DragDropTarget,           1.00f, 1.00f, 0.00f, 0.90f);
    addColor(ImGuiCol_UnsavedMarker,            1.00f, 1.00f, 1.00f, 1.00f);

    addColor(ImGuiCol_NavCursor,                0.26f, 0.59f, 0.98f, 1.00f);
    addColor(ImGuiCol_NavWindowingHighlight,    1.00f, 1.00f, 1.00f, 0.70f);
    addColor(ImGuiCol_NavWindowingDimBg,        0.80f, 0.80f, 0.80f, 0.20f);
    addColor(ImGuiCol_ModalWindowDimBg,         0.80f, 0.80f, 0.80f, 0.35f);

    // ===============================
    // IMGUI STYLE (VARS)
    // ===============================

    addVar(ImGuiStyleVar_Alpha,                     1.0f);
    addVar(ImGuiStyleVar_DisabledAlpha,             0.60f);

    addVar(ImGuiStyleVar_WindowRounding,            0.0f);
    addVar(ImGuiStyleVar_WindowBorderSize,          1.0f);

    addVar(ImGuiStyleVar_ChildRounding,             0.0f);
    addVar(ImGuiStyleVar_ChildBorderSize,           1.0f);

    addVar(ImGuiStyleVar_PopupRounding,             0.0f);
    addVar(ImGuiStyleVar_PopupBorderSize,           1.0f);

    addVar(ImGuiStyleVar_FrameRounding,             0.0f);
    addVar(ImGuiStyleVar_FrameBorderSize,           0.0f);

    addVar(ImGuiStyleVar_IndentSpacing,             21.0f);

    addVar(ImGuiStyleVar_ScrollbarSize,             14.0f);
    addVar(ImGuiStyleVar_ScrollbarRounding,         9.0f);
    addVar(ImGuiStyleVar_ScrollbarPadding,          2.0f);

    addVar(ImGuiStyleVar_GrabMinSize,               12.0f);
    addVar(ImGuiStyleVar_GrabRounding,              0.0f);

    addVar(ImGuiStyleVar_ImageBorderSize,           0.0f);

    addVar(ImGuiStyleVar_TabRounding,               5.0f);
    addVar(ImGuiStyleVar_TabBorderSize,             0.0f);
    addVar(ImGuiStyleVar_TabMinWidthBase,           1.0f);
    addVar(ImGuiStyleVar_TabMinWidthShrink,         80.0f);

    addVar(ImGuiStyleVar_TabBarBorderSize,          1.0f);
    addVar(ImGuiStyleVar_TabBarOverlineSize,        1.0f);

    addVar(ImGuiStyleVar_TableAngledHeadersAngle,   0.610865f);
    addVar(ImGuiStyleVar_TreeLinesSize,             1.0f);
    addVar(ImGuiStyleVar_TreeLinesRounding,         0.0f);
    addVar(ImGuiStyleVar_SeparatorTextBorderSize,   3.0f);

    addVar2(ImGuiStyleVar_WindowPadding,            8.0f, 8.0f);
    addVar2(ImGuiStyleVar_WindowMinSize,            32.0f, 32.0f);
    addVar2(ImGuiStyleVar_WindowTitleAlign,         0.0f, 0.5f);

    addVar2(ImGuiStyleVar_FramePadding,             4.0f, 3.0f);
    addVar2(ImGuiStyleVar_ItemSpacing,              8.0f, 4.0f);
    addVar2(ImGuiStyleVar_ItemInnerSpacing,         4.0f, 4.0f);
    addVar2(ImGuiStyleVar_CellPadding,              4.0f, 2.0f);

    addVar2(ImGuiStyleVar_TableAngledHeadersTextAlign, 0.5f, 0.0f);

    addVar2(ImGuiStyleVar_ButtonTextAlign,          0.5f, 0.5f);
    addVar2(ImGuiStyleVar_SelectableTextAlign,      0.0f, 0.0f);

    addVar2(ImGuiStyleVar_SeparatorTextAlign,       0.0f, 0.5f);
    addVar2(ImGuiStyleVar_SeparatorTextPadding,     20.0f, 3.0f);
}

void CImGuiCustomMenu::FreeImagesRecursive(std::vector<MenuItem>& items)
{
    for (auto& it : items)
    {
        if ((it.type == MenuItem::IMAGE || it.type == MenuItem::IMAGE_BUTTON) && it.image.texture != (ImTextureID)0)
        {
            m_ImguiUtils.FreeImage(it.image);
            it.imageLoaded = false;
        }

        if (!it.children.empty())
            FreeImagesRecursive(it.children);
    }
}

void CImGuiCustomMenu::FreeAllImages()
{
    for (auto& win : m_Windows)
        FreeImagesRecursive(win.items);
}

void CImGuiCustomMenu::Initialize()
{
    gEngfuncs.pfnAddCommand("exec_menu", ExecMenuCmd_f);
}

void CImGuiCustomMenu::VidInitialize() {}
void CImGuiCustomMenu::Terminate()
{
    FreeAllImages();
    m_Windows.clear();
    m_StyleVars.clear();
    m_StyleColors.clear();
}
void CImGuiCustomMenu::Think() {}

bool CImGuiCustomMenu::Active()
{
    return m_ShowCustomMenu;
}

bool CImGuiCustomMenu::CursorRequired()
{
    return m_ShowCustomMenu;
}

bool CImGuiCustomMenu::HandleKey(bool keyDown, int keyNumber, const char * /*bindName*/)
{
    if (keyNumber == K_ESCAPE && keyDown && m_ShowCustomMenu)
    {
        m_ShowCustomMenu = false;
        return true;
    }
    return false;
}

void CImGuiCustomMenu::Draw()
{
    if (!m_ShowCustomMenu)
        return;

    for (const auto& sv : m_StyleVars)
    {
        if (sv.idx == (ImGuiStyleVar)-1)
            continue;
        if (sv.isVec2)
            ImGui::PushStyleVar(sv.idx, sv.value);
        else
            ImGui::PushStyleVar(sv.idx, sv.value.x);
    }

    for (const auto& sc : m_StyleColors)
    {
        if (sc.idx == (ImGuiCol)-1)
            continue;
        ImGui::PushStyleColor(sc.idx, sc.color);
    }

    bool anyOpen = false;
    for (auto& win : m_Windows)
    {
        if (!win.open)
            continue;

        DrawWindow(win);
        if (win.open)
            anyOpen = true;
    }

    if (!m_StyleColors.empty())
        ImGui::PopStyleColor((int)m_StyleColors.size());
    if (!m_StyleVars.empty())
        ImGui::PopStyleVar((int)m_StyleVars.size());

    if (!anyOpen)
        m_ShowCustomMenu = false;
}

void CImGuiCustomMenu::DrawWindow(MenuWindow& win)
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;

    bool openFlag = win.open;
    const char* title = win.title.empty() ? m_DefaultTitle.c_str() : win.title.c_str();

    if (!ImGui::Begin(title, &openFlag, flags))
    {
        ImGui::End();
        win.open = openFlag;
        return;
    }

    float scrW = (float)g_ImGuiViewport.scrWidth();
    float scrH = (float)g_ImGuiViewport.scrHeight();

    ImVec2 size = ImGui::GetWindowSize();
    if (size.x > scrW)
        size.x = scrW;
    if (size.y > scrH)
        size.y = scrH;

    ImGui::SetWindowSize(size);

    ImVec2 pos = ImGui::GetWindowPos();
    if (pos.x < 0.0f)
        pos.x = 0.0f;
    if (pos.y < 0.0f)
        pos.y = 0.0f;

    if (pos.x + size.x > scrW)
        pos.x = scrW - size.x;
    if (pos.y + size.y > scrH)
        pos.y = scrH - size.y;

    ImGui::SetWindowPos(pos);

    DrawItemsRecursive(win.items);

    ImGui::End();
    win.open = openFlag;
}

void CImGuiCustomMenu::DrawItemsRecursive(std::vector<MenuItem>& items)
{
    for (auto& item : items)
    {
        switch (item.type)
        {
            case MenuItem::BUTTON:
            {
                if (item.label.empty() || item.command.empty())
                    break;

                if (ImGui::Button(item.label.c_str()))
                    gEngfuncs.pfnClientCmd(item.command.c_str());
                break;
            }

            case MenuItem::CHECKBOX:
            {
                if (item.label.empty() || item.cvarName.empty())
                    break;

                float v = gEngfuncs.pfnGetCvarFloat(item.cvarName.c_str());
                bool  state = (v != 0.0f);

                if (ImGui::Checkbox(item.label.c_str(), &state))
                {
                    m_ImguiUtils.SetCvarFloat(item.cvarName.c_str(), state ? 1.0f : 0.0f);
                }
                break;
            }

            case MenuItem::TEXT:
            {
                if (item.label.empty())
                    break;

                ImGui::TextUnformatted(item.label.c_str());
                break;
            }

            case MenuItem::SEPARATOR:
                ImGui::Separator();
                break;

            case MenuItem::SAME_LINE:
                ImGui::SameLine();
                break;

            case MenuItem::CLOSE_MENU:
            {
                if (item.label.empty())
                    break;

                if (ImGui::Button(item.label.c_str()))
                    m_ShowCustomMenu = false;
                break;
            }

            case MenuItem::SUBMENU:
            {
                if (item.label.empty())
                    break;

                if (ImGui::CollapsingHeader(item.label.c_str()))
                {
                    ImGui::Indent(10.0f);
                    DrawItemsRecursive(item.children);
                    ImGui::Unindent(10.0f);
                }
                break;
            }

            case MenuItem::TABBAR:
            {
                if (item.label.empty())
                    break;

                std::string id = item.label + "##tabbar";
                if (ImGui::BeginTabBar(id.c_str()))
                {
                    for (auto& child : item.children)
                    {
                        if (child.type == MenuItem::CONDITION)
                        {
                            float val = gEngfuncs.pfnGetCvarFloat(child.conditionCvar.c_str());
                            if (val == child.conditionValue)
                            {
                                for (auto& tab : child.children)
                                {
                                    if (tab.type != MenuItem::TAB || tab.label.empty())
                                        continue;

                                    if (ImGui::BeginTabItem(tab.label.c_str()))
                                    {
                                        ImGui::PushID(&tab);
                                        DrawItemsRecursive(tab.children);
                                        ImGui::PopID();
                                        ImGui::EndTabItem();
                                    }
                                }
                            }
                            continue;
                        }

                        if (child.type != MenuItem::TAB || child.label.empty())
                            continue;

                        if (ImGui::BeginTabItem(child.label.c_str()))
                        {
                            ImGui::PushID(&child);
                            DrawItemsRecursive(child.children);
                            ImGui::PopID();
                            ImGui::EndTabItem();
                        }
                    }

                    ImGui::EndTabBar();
                }
                break;
            }

            case MenuItem::TAB:
                break;

            case MenuItem::SLIDER_INT:
            {
                if (item.label.empty() || item.cvarName.empty())
                    continue;

                int v = (int)gEngfuncs.pfnGetCvarFloat(item.cvarName.c_str());
                if (ImGui::SliderInt(item.label.c_str(), &v, item.minInt, item.maxInt))
                {
                    char cmd[256];
                    std::snprintf(cmd, sizeof(cmd), "%s %d", item.cvarName.c_str(), v);
                    gEngfuncs.pfnClientCmd(cmd);
                }
                break;
            }

            case MenuItem::SLIDER_FLOAT:
            {
                if (item.label.empty() || item.cvarName.empty())
                    continue;

                float v = gEngfuncs.pfnGetCvarFloat(item.cvarName.c_str());

                const float step = 0.1f;

                if (ImGui::SliderFloat(item.label.c_str(), &v, item.minValue, item.maxValue, "%.1f"))
                {
                    v = std::roundf(v / step) * step;

                    char cmd[256];
                    std::snprintf(cmd, sizeof(cmd), "%s %.1f", item.cvarName.c_str(), v);
                    gEngfuncs.pfnClientCmd(cmd);
                }
                break;
            }

            case MenuItem::COLOR_CVAR:
            {
                if (item.label.empty() || item.cvarName.empty())
                    continue;

                const char* str = gEngfuncs.pfnGetCvarString(item.cvarName.c_str());
                int r = 0, g = 0, b = 0, a = 255;
                int count = 0;

                if (str && *str)
                    count = std::sscanf(str, "%d %d %d %d", &r, &g, &b, &a);

                if (count < 3)
                {
                    r = g = b = 0;
                    a = 255;
                }

                if (count < 4)
                    a = 255;

                float col[4];
                col[0] = r / 255.0f;
                col[1] = g / 255.0f;
                col[2] = b / 255.0f;
                col[3] = a / 255.0f;

                const char* label = item.label.c_str();
                bool hasAlpha = (count >= 4);

                bool changed = false;
                if (hasAlpha)
                    changed = ImGui::ColorEdit4(label, col);
                else
                    changed = ImGui::ColorEdit3(label, col);

                if (changed)
                {
                    r = (int)(col[0] * 255.0f + 0.5f);
                    g = (int)(col[1] * 255.0f + 0.5f);
                    b = (int)(col[2] * 255.0f + 0.5f);
                    a = (int)(col[3] * 255.0f + 0.5f);

                    char cmd[128];
                    if (hasAlpha)
                        std::snprintf(cmd, sizeof(cmd), "%s \"%d %d %d %d\"", item.cvarName.c_str(), r, g, b, a);
                    else
                        std::snprintf(cmd, sizeof(cmd), "%s \"%d %d %d\"", item.cvarName.c_str(), r, g, b);

                    gEngfuncs.pfnClientCmd(cmd);
                }
                break;
            }

            case MenuItem::SPACE:
            {
                ImGui::Spacing();
                break;
            }

            case MenuItem::CONDITION:
            {
                float v = gEngfuncs.pfnGetCvarFloat(item.conditionCvar.c_str());
                bool pass = false;

                switch (item.conditionOp)
                {
                    case MenuItem::EQ:
                        pass = (v == item.conditionValue);
                        break;
                    case MenuItem::NE:
                        pass = (v != item.conditionValue);
                        break;
                    case MenuItem::GT:
                        pass = (v > item.conditionValue);
                        break;
                    case MenuItem::GE:
                        pass = (v >= item.conditionValue);
                        break;
                    case MenuItem::LT:
                        pass = (v < item.conditionValue);
                        break;
                    case MenuItem::LE:
                        pass = (v <= item.conditionValue);
                        break;
                }

                if (pass)
                    DrawItemsRecursive(item.children);

                break;
            }

            case MenuItem::IMAGE:
            {
                if (item.imagePath.empty())
                    break;

                if (!item.imageLoaded)
                {
                    item.image = m_ImguiUtils.LoadImageFromFile(item.imagePath.c_str());
                    item.imageLoaded = (item.image.texture != (ImTextureID)0);

                    if (!item.imageLoaded)
                    {
                        gEngfuncs.Con_Printf("Failed to load image '%s'\n", item.imagePath.c_str());
                        break;
                    }
                }

                float w = (item.imageWidth  > 0.0f) ? item.imageWidth  : (float)item.image.width;
                float h = (item.imageHeight > 0.0f) ? item.imageHeight : (float)item.image.height;

                ImVec2 size(w, h);

                ImGui::Image(item.image.texture, size);
                break;
            }

            case MenuItem::IMAGE_BUTTON:
            {
                if (item.imagePath.empty())
                    break;

                if (!item.imageLoaded)
                {
                    item.image = m_ImguiUtils.LoadImageFromFile(item.imagePath.c_str());
                    item.imageLoaded = (item.image.texture != (ImTextureID)0);

                    if (!item.imageLoaded)
                    {
                        gEngfuncs.Con_Printf("Failed to load image '%s'\n", item.imagePath.c_str());
                        break;
                    }
                }

                float w = (item.imageWidth  > 0.0f) ? item.imageWidth  : (float)item.image.width;
                float h = (item.imageHeight > 0.0f) ? item.imageHeight : (float)item.image.height;

                ImVec2 size(w, h);

                ImGui::PushID(&item);
                bool pressed = ImGui::ImageButton("##imgbtn", item.image.texture, size);
                ImGui::PopID();

                if (pressed && !item.command.empty())
                    gEngfuncs.pfnClientCmd(item.command.c_str());

                if (!item.label.empty() && ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", item.label.c_str());

                break;
            }

            case MenuItem::CVAR_CHECK:
            {
                cvar_t* cvar = gEngfuncs.pfnGetCvarPointer(item.cvarName.c_str());

                if (!cvar)
                {
                    ImGui::Text("%s: <not found>", item.label.c_str());
                    break;
                }

                char* end = nullptr;
                std::strtof(cvar->string, &end);
                bool isNumber = (end && *end == '\0');

                if (!isNumber)
                {
                    ImGui::Text("%s: %s", item.label.c_str(), cvar->string);
                    break;
                }

                float v = cvar->value;
                int iv = (int)v;

                if (fabsf(v - (float)iv) < 0.0001f)
                {
                    // int
                    ImGui::Text("%s: %d", item.label.c_str(), iv);
                }
                else
                {
                    // float
                    ImGui::Text("%s: %.3f", item.label.c_str(), v);
                }

                break;
            }

        }
    }
}

void CImGuiCustomMenu::ExecMenu_f()
{
    int argc = gEngfuncs.Cmd_Argc();

    if (argc < 2)
    {
        gEngfuncs.Con_Printf("Usage: exec_menu <file>\n");
        return;
    }

    const char* arg = gEngfuncs.Cmd_Argv(1);

    std::string file = arg;
    if (file.size() < 4 || file.compare(file.size() - 4, 4, ".cfg") != 0)
        file += ".cfg";

    if (!LoadFromCfg(file.c_str()))
        gEngfuncs.Con_Printf("Failed to load menu '%s'\n", file.c_str());
}

bool CImGuiCustomMenu::LoadFromCfg(const char* filename)
{
    FreeAllImages();

    m_ShowCustomMenu = false;

    char path[512];
    std::snprintf(path, sizeof(path), "%s/%s", gEngfuncs.pfnGetGameDirectory(), filename);

    FILE* f = std::fopen(path, "r");
    if (!f)
    {
        gEngfuncs.Con_Printf("Cannot open menu config: %s\n", path);
        return false;
    }

    m_Windows.clear();
    m_StyleVars.clear();
    m_StyleColors.clear();

    SetupDefaultMenuStyle();

    MenuWindow* curWin  = nullptr;
    std::vector<MenuItem>* curList = nullptr;
    std::vector<std::vector<MenuItem>*> stack;
    MenuItem* lastContainer = nullptr;

    bool hadError = false;
    int line = 0;

    auto PrintError = [&](const char* fmt, ...)
    {
        hadError = true;

        char msg[1024];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);

        gEngfuncs.Con_Printf("Menu cfg error in %s:%d: %s\n", path, line, msg);
    };

    char buf[512];
    while (std::fgets(buf, sizeof(buf), f))
    {
        ++line;

        TrimRight(buf);
        if (buf[0] == '\0')
            continue;

        std::string str(buf);
        TrimLeft(str);
        if (str.empty())
            continue;

        if (str[0] == '#' || str[0] == ';' ||
            (str.size() > 1 && str[0] == '/' && str[1] == '/'))
            continue;

        if (!curWin)
        {
            if (StartsWith(str, "stylevar2"))
            {
                char name[64];
                float a, b;
                if (std::sscanf(str.c_str(), "stylevar2 %63s %f %f", name, &a, &b) != 3)
                {
                    PrintError("Invalid stylevar2 syntax. Expected: stylevar2 <Name> <x> <y>");
                    continue;
                }

                MenuStyleVar sv;
                sv.idx = GetStyleVarIndex(name);
                sv.isVec2 = true;
                sv.value = ImVec2(a, b);

                if (sv.idx == (ImGuiStyleVar)-1)
                    PrintError("Unknown stylevar2 '%s'", name);
                else
                    m_StyleVars.push_back(sv);

                continue;
            }

            if (StartsWith(str, "stylevar"))
            {
                char name[64];
                float a;
                if (std::sscanf(str.c_str(), "stylevar %63s %f", name, &a) != 2)
                {
                    PrintError("Invalid stylevar syntax. Expected: stylevar <Name> <value>");
                    continue;
                }

                MenuStyleVar sv;
                sv.idx = GetStyleVarIndex(name);
                sv.isVec2 = false;
                sv.value = ImVec2(a, 0.0f);

                if (sv.idx == (ImGuiStyleVar)-1)
                    PrintError("Unknown stylevar '%s'", name);
                else
                    m_StyleVars.push_back(sv);

                continue;
            }

            if (StartsWith(str, "stylecolor"))
            {
                char  name[64];
                float r, g, b, a;
                if (std::sscanf(str.c_str(), "stylecolor %63s %f %f %f %f", name, &r, &g, &b, &a) != 5)
                {
                    PrintError("Invalid stylecolor syntax. Expected: stylecolor <Name> r g b a");
                    continue;
                }

                MenuStyleColor sc;
                sc.idx = GetColorIndex(name);
                sc.color = ImVec4(r, g, b, a);

                if (sc.idx == (ImGuiCol)-1)
                    PrintError("Unknown stylecolor '%s'", name);
                else
                    m_StyleColors.push_back(sc);

                continue;
            }

            if (StartsWith(str, "window"))
            {
                size_t first = str.find('"');
                size_t second = str.find('"', first + 1);

                MenuWindow win;
                if (first != std::string::npos && second != std::string::npos && second > first)
                {
                    win.title = str.substr(first + 1, second - first - 1);
                }
                else
                {
                    PrintError("Window title is missing. Using default title.");
                }

                m_Windows.push_back(win);
                curWin = &m_Windows.back();
                curList = &curWin->items;
                lastContainer = nullptr;
                stack.clear();
                continue;
            }

            if (str == "{" || str == "}")
            {
                PrintError("Unexpected '%s' before any window", str.c_str());
                continue;
            }

            PrintError("Unknown directive before any window: '%s'", str.c_str());
            continue;
        }

        if (str == "}")
        {
            if (!stack.empty())
            {
                curList = stack.back();
                stack.pop_back();
                lastContainer = nullptr;
            }
            else
            {
                curWin = nullptr;
                curList = nullptr;
                lastContainer = nullptr;
            }
            continue;
        }

        if (str == "{")
        {
            if (lastContainer && curList)
            {
                stack.push_back(curList);
                curList = &lastContainer->children;
                lastContainer = nullptr;
            }
            continue;
        }

        if (!curList)
        {
            PrintError("Internal parser error: curList is null");
            continue;
        }

        if (StartsWith(str, "text"))
        {
            size_t first = str.find('"');
            size_t last = str.rfind('"');
            if (first == std::string::npos || last  == std::string::npos || last <= first)
            {
                PrintError("Invalid text syntax. Expected: text \"Label\"");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::TEXT;
            it.label = str.substr(first + 1, last - first - 1);
            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "separator"))
        {
            MenuItem it;
            it.type = MenuItem::SEPARATOR;
            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "same_line"))
        {
            MenuItem it;
            it.type = MenuItem::SAME_LINE;
            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "button"))
        {
            size_t first = str.find('"');
            size_t second = str.find('"', first + 1);
            size_t third = str.find('"', second + 1);
            size_t fourth = str.find('"', third + 1);

            if (first == std::string::npos || second == std::string::npos || third  == std::string::npos || fourth == std::string::npos || !(second > first && fourth > third))
            {
                PrintError("Invalid button syntax. Expected: button \"Label\" \"command\"");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::BUTTON;
            it.label = str.substr(first  + 1, second - first  - 1);
            it.command = str.substr(third  + 1, fourth - third - 1);
            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "checkbox"))
        {
            size_t first = str.find('"');
            size_t second = str.find('"', first + 1);
            size_t third = str.find('"', second + 1);
            size_t fourth = str.find('"', third + 1);

            if (first == std::string::npos || second == std::string::npos || third  == std::string::npos || fourth == std::string::npos || !(second > first && fourth > third))
            {
                PrintError("Invalid checkbox syntax. Expected: checkbox \"Label\" \"cvar\"");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::CHECKBOX;
            it.label = str.substr(first  + 1, second - first  - 1);
            it.cvarName = str.substr(third  + 1, fourth - third - 1);
            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "close_menu"))
        {
            size_t first = str.find('"');
            size_t last = str.rfind('"');
            if (first == std::string::npos || last  == std::string::npos || last <= first)
            {
                PrintError("Invalid close_menu syntax. Expected: close_menu \"Label\"");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::CLOSE_MENU;
            it.label = str.substr(first + 1, last - first - 1);
            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "submenu"))
        {
            size_t first = str.find('"');
            size_t second = str.find('"', first + 1);

            if (first == std::string::npos || second == std::string::npos || second <= first)
            {
                PrintError("Invalid submenu syntax. Expected: submenu \"Label\" { ... }");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::SUBMENU;
            it.label = str.substr(first + 1, second - first - 1);
            curList->push_back(it);
            lastContainer = &curList->back();

            if (str.find('{', second) != std::string::npos)
            {
                stack.push_back(curList);
                curList       = &lastContainer->children;
                lastContainer = nullptr;
            }
            continue;
        }

        if (StartsWith(str, "tabbar"))
        {
            size_t first = str.find('"');
            size_t second = str.find('"', first + 1);

            if (first == std::string::npos || second == std::string::npos || second <= first)
            {
                PrintError("Invalid tabbar syntax. Expected: tabbar \"Label\" { ... }");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::TABBAR;
            it.label = str.substr(first + 1, second - first - 1);
            curList->push_back(it);
            lastContainer = &curList->back();

            if (str.find('{', second) != std::string::npos)
            {
                stack.push_back(curList);
                curList = &lastContainer->children;
                lastContainer = nullptr;
            }
            continue;
        }

        if (StartsWith(str, "tab"))
        {
            size_t first = str.find('"');
            size_t second = str.find('"', first + 1);

            if (first == std::string::npos || second == std::string::npos || second <= first)
            {
                PrintError("Invalid tab syntax. Expected: tab \"Label\" { ... }");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::TAB;
            it.label = str.substr(first + 1, second - first - 1);
            curList->push_back(it);
            lastContainer = &curList->back();

            if (str.find('{', second) != std::string::npos)
            {
                stack.push_back(curList);
                curList = &lastContainer->children;
                lastContainer = nullptr;
            }
            continue;
        }

        if (StartsWith(str, "slider_int"))
        {
            size_t first = str.find('"');
            size_t second = str.find('"', first + 1);
            size_t third = str.find('"', second + 1);
            size_t fourth = str.find('"', third + 1);

            if (first == std::string::npos || second == std::string::npos || second <= first || third  == std::string::npos || fourth == std::string::npos || fourth <= third)
            {
                PrintError("Invalid slider_int syntax. Expected: slider_int \"Label\" \"cvar\" min max");
                continue;
            }

            std::string label = str.substr(first  + 1, second - first  - 1);
            std::string cvar = str.substr(third + 1, fourth - third - 1);

            const char* rest = str.c_str() + fourth + 1;

            int minVal = 0, maxVal = 0;
            if (std::sscanf(rest, "%d %d", &minVal, &maxVal) != 2)
            {
                PrintError("Invalid slider_int syntax. Expected: slider_int \"Label\" \"cvar\" min max");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::SLIDER_INT;
            it.label = label;
            it.cvarName = cvar;
            it.minInt = minVal;
            it.maxInt = maxVal;

            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "slider_float"))
        {
            size_t first = str.find('"');
            size_t second = str.find('"', first + 1);
            size_t third = str.find('"', second + 1);
            size_t fourth = str.find('"', third + 1);

            if (first == std::string::npos || second == std::string::npos || second <= first || third  == std::string::npos || fourth == std::string::npos || fourth <= third)
            {
                PrintError("Invalid slider_float syntax. Expected: slider_float \"Label\" \"cvar\" min max");
                continue;
            }

            std::string label = str.substr(first  + 1, second - first  - 1);
            std::string cvar = str.substr(third + 1, fourth - third - 1);

            const char* rest = str.c_str() + fourth + 1;

            float minVal = 0.0f, maxVal = 0.0f;
            if (std::sscanf(rest, "%f %f", &minVal, &maxVal) != 2)
            {
                PrintError("Invalid slider_float syntax. Expected: slider_float \"Label\" \"cvar\" min max");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::SLIDER_FLOAT;
            it.label = label;
            it.cvarName = cvar;
            it.minValue = minVal;
            it.maxValue = maxVal;

            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "color_cvar"))
        {
            size_t first  = str.find('"');
            size_t second = str.find('"', first + 1);
            size_t third  = str.find('"', second + 1);
            size_t fourth = str.find('"', third + 1);

            if (first  == std::string::npos || second == std::string::npos || second <= first || third  == std::string::npos || fourth == std::string::npos || fourth <= third)
            {
                PrintError("Invalid color_cvar syntax. Expected: color_cvar \"Label\" \"cvar\"");
                continue;
            }

            std::string label = str.substr(first  + 1, second - first  - 1);
            std::string cvar = str.substr(third + 1, fourth - third - 1);

            MenuItem it;
            it.type = MenuItem::COLOR_CVAR;
            it.label = label;
            it.cvarName = cvar;

            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "space"))
        {
            MenuItem it;
            it.type = MenuItem::SPACE;
            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "condition"))
        {
            size_t first = str.find('"');
            size_t second = str.find('"', first + 1);

            if (first == std::string::npos || second == std::string::npos || second <= first)
            {
                PrintError("Invalid condition syntax. Expected: condition \"cvar\" <operator> value { ... }");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::CONDITION;
            it.conditionCvar = str.substr(first + 1, second - first - 1);

            std::string rest = str.substr(second + 1);

            if (rest.find(">=") != std::string::npos)
                it.conditionOp = MenuItem::GE;
            else if (rest.find("<=") != std::string::npos)
                it.conditionOp = MenuItem::LE;
            else if (rest.find("!=") != std::string::npos)
                it.conditionOp = MenuItem::NE;
            else if (rest.find(">")  != std::string::npos)
                it.conditionOp = MenuItem::GT;
            else if (rest.find("<")  != std::string::npos)
                it.conditionOp = MenuItem::LT;
            else
                it.conditionOp = MenuItem::EQ;

            float value = 0.0f;
            if (std::sscanf(rest.c_str(), "%*[^0-9.-]%f", &value) != 1)
            {
                PrintError("Invalid condition value");
                continue;
            }

            it.conditionValue = value;

            curList->push_back(it);
            lastContainer = &curList->back();

            if (str.find('{', second) != std::string::npos)
            {
                stack.push_back(curList);
                curList = &lastContainer->children;
            }
            continue;
        }

        if (StartsWith(str, "image "))
        {
            size_t first = str.find('"');
            size_t second = str.find('"', first + 1);

            if (first == std::string::npos || second == std::string::npos || second <= first)
            {
                PrintError("Invalid image syntax. Expected: image \"path\" width height");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::IMAGE;
            it.imagePath = str.substr(first + 1, second - first - 1);

            float w = 32.0f, h = 32.0f;
            const char* rest = str.c_str() + second + 1;

            if (std::sscanf(rest, "%f %f", &w, &h) != 2)
            {
                PrintError("Invalid image size. Expected: image \"path\" width height");
                continue;
            }

            it.imageWidth = w;
            it.imageHeight = h;
            it.imageLoaded = false;

            curList->push_back(it);
            continue;
        }


        if (StartsWith(str, "image_button"))
        {
            size_t first  = str.find('"');
            size_t second = str.find('"', first  + 1);
            size_t third  = str.find('"', second + 1);
            size_t fourth = str.find('"', third  + 1);
            size_t fifth  = str.find('"', fourth + 1);
            size_t sixth  = str.find('"', fifth  + 1);

            if (first  == std::string::npos || second == std::string::npos || second <= first ||
                third  == std::string::npos || fourth == std::string::npos || fourth <= third ||
                fifth  == std::string::npos || sixth  == std::string::npos || sixth  <= fifth)
            {
                PrintError("Invalid image_button syntax. Expected: image_button \"Label\" \"command\" \"image\" width height");
                continue;
            }

            std::string label = str.substr(first  + 1, second - first - 1);
            std::string command = str.substr(third  + 1, fourth - third - 1);
            std::string image = str.substr(fifth  + 1, sixth  - fifth - 1);

            float w = 32.0f, h = 32.0f;
            const char* rest = str.c_str() + sixth + 1;
            std::sscanf(rest, "%f %f", &w, &h);

            MenuItem it;
            it.type = MenuItem::IMAGE_BUTTON;
            it.label = label;
            it.command = command;
            it.imagePath = image;
            it.imageWidth = w;
            it.imageHeight = h;
            it.imageLoaded = false;

            curList->push_back(it);
            continue;
        }

        if (StartsWith(str, "cvar_check"))
        {
            size_t first = str.find('"');
            size_t second = str.find('"', first + 1);
            size_t third = str.find('"', second + 1);
            size_t fourth = str.find('"', third + 1);

            if (first == std::string::npos || second == std::string::npos || third == std::string::npos || fourth == std::string::npos)
            {
                PrintError("Invalid cvar_check syntax. Expected: cvar_check \"text\" \"cvar\"");
                continue;
            }

            MenuItem it;
            it.type = MenuItem::CVAR_CHECK;
            it.label = str.substr(first + 1, second - first - 1);
            it.cvarName = str.substr(third + 1, fourth - third - 1);

            curList->push_back(it);
            continue;
        }


        PrintError("Unknown directive: '%s'", str.c_str());
    }

    std::fclose(f);

    if (!stack.empty())
    {
        PrintError("Unexpected end of file: %d unclosed '{' block(s)", (int)stack.size());
    }

    bool success = !hadError && stack.empty() && !m_Windows.empty();

    m_ShowCustomMenu = success;

    return success;
}
