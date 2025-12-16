#pragma once

#include "imgui_window.h"
#include "imgui.h"
#include "imgui_utils.h"

#include <string>
#include <vector>

struct MenuItem
{
    enum Type
    {
        BUTTON,
        CHECKBOX,
        TEXT,
        SEPARATOR,
        SUBMENU,
        CLOSE_MENU,
        SAME_LINE,
        TABBAR,
        TAB,
        SLIDER_INT,
        SLIDER_FLOAT,
        COLOR_CVAR,
        SPACE,
        CONDITION,
        IMAGE_BUTTON
    } type;

    std::string label;
    std::string command;
    std::string cvarName;

    float minValue;
    float maxValue;

    int minInt;
    int maxInt;

    enum CompareOp
    {
        EQ, // ==
        NE, // !=
        GT, // >
        GE, // >=
        LT, // <
        LE  // <=
    } conditionOp = EQ;

    std::string conditionCvar;
    float conditionValue;

    std::vector<MenuItem> children;

    std::string imagePath;

    ImGuiImage image;
    bool imageLoaded = false;

    float imageWidth;
    float imageHeight;
};

struct MenuWindow
{
    std::string title;
    bool open = true;
    std::vector<MenuItem> items;
};

struct MenuStyleVar
{
    ImGuiStyleVar idx = (ImGuiStyleVar)-1;
    ImVec2 value;
    bool isVec2 = false;
};

struct MenuStyleColor
{
    ImGuiCol idx = (ImGuiCol)-1;
    ImVec4 color;
};

class CImGuiCustomMenu : public IImGuiWindow
{
public:
    void Initialize();
    void VidInitialize();
    void Terminate();
    void Think();
    void Draw();
    bool Active();
    bool CursorRequired();
    bool HandleKey(bool keyDown, int keyNumber, const char *bindName);

    void ExecMenu_f();
    bool LoadFromCfg(const char* filename);

private:
    void DrawWindow(MenuWindow& win);
    void DrawItemsRecursive(std::vector<MenuItem>& items);
    void SetupDefaultMenuStyle();

    static ImGuiStyleVar GetStyleVarIndex(const std::string& name);
    static ImGuiCol GetColorIndex(const std::string& name);

    void FreeImagesRecursive(std::vector<MenuItem>& items);
    void FreeAllImages();

private:
    static bool m_ShowCustomMenu;

    std::string m_DefaultTitle = "Custom Menu";
    std::vector<MenuWindow> m_Windows;
    std::vector<MenuStyleVar> m_StyleVars;
    std::vector<MenuStyleColor> m_StyleColors;
};

extern CImGuiCustomMenu m_iCustomMenu;
