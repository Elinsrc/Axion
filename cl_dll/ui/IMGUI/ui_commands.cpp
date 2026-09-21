#include "ui_commands.h"
#include "imgui_internal.h"
#include "hud.h"
#include "cl_util.h"
#include "keydefs.h"
#include "imgui_viewport.h"
#include "icons.h"

#include <cstring>
#include <cmath>
#include <algorithm>

CImGuiCommands g_ImGuiCommands;
bool CImGuiCommands::m_ShowCommands = false;
int CImGuiCommands::m_ActiveTab = 0;

void CImGuiCommands::HelpTooltip(const char* pCvarName, const char* pDesc, const char* pDefault, const char* pRange)
{
    ImGui::SameLine();
    ImGui::TextDisabled("( ? )");
    
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        return;

    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(300.0f);
    ImGui::TextColored(ImVec4(0.50f, 0.80f, 1.00f, 1.00f), "%s", pCvarName);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped("%s", pDesc);

    if (pDefault || pRange)
    {
        ImGui::Spacing();
        ImGui::Separator();
        if (pDefault)
        {
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "Default:");
            ImGui::SameLine();
            ImGui::Text("%s", pDefault);
        }
        if (pRange)
        {
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "Range:");
            ImGui::SameLine();
            ImGui::Text("%s", pRange);
        }
    }

    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void CImGuiCommands::DrawCheckbox(const char* pLabel, const char* pCvar, const char* pDesc, const char* pDef)
{
    bool val = gEngfuncs.pfnGetCvarFloat(pCvar) != 0.0f;
    if (ImGui::Checkbox(pLabel, &val))
        m_ImguiUtils.SetCvarFloat(pCvar, val ? 1.0f : 0.0f);

    HelpTooltip(pCvar, pDesc, pDef, "0 – 1");
}

void CImGuiCommands::DrawSliderFloatWithReset(const char* pLabel, const char* pCvar, float minVal, float maxVal, float defaultVal, const char* pDesc, const char* pFmt)
{
    float val = gEngfuncs.pfnGetCvarFloat(pCvar);
    if (ImGui::SliderFloat(pLabel, &val, minVal, maxVal, pFmt))
        m_ImguiUtils.SetCvarFloat(pCvar, val);

    char defBuf[32], rangeBuf[32];
    std::snprintf(defBuf, sizeof(defBuf), pFmt, defaultVal);
    std::snprintf(rangeBuf, sizeof(rangeBuf), "%.1f – %.1f", minVal, maxVal);
    HelpTooltip(pCvar, pDesc, defBuf, rangeBuf);

    ImGui::SameLine();
    char resetId[64];
    std::snprintf(resetId, sizeof(resetId), "Reset##%s", pCvar);
    if (ImGui::SmallButton(resetId))
        m_ImguiUtils.SetCvarFloat(pCvar, defaultVal);
}

void CImGuiCommands::DrawSliderIntWithReset(const char* pLabel, const char* pCvar, int minVal, int maxVal, int defaultVal, const char* pDesc)
{
    int val = static_cast<int>(gEngfuncs.pfnGetCvarFloat(pCvar));
    if (ImGui::SliderInt(pLabel, &val, minVal, maxVal))
        m_ImguiUtils.SetCvarFloat(pCvar, static_cast<float>(val));

    char defBuf[32], rangeBuf[32];
    std::snprintf(defBuf, sizeof(defBuf), "%d", defaultVal);
    std::snprintf(rangeBuf, sizeof(rangeBuf), "%d – %d", minVal, maxVal);
    HelpTooltip(pCvar, pDesc, defBuf, rangeBuf);

    ImGui::SameLine();
    char resetId[64];
    std::snprintf(resetId, sizeof(resetId), "Reset##%s", pCvar);
    if (ImGui::SmallButton(resetId))
        m_ImguiUtils.SetCvarFloat(pCvar, static_cast<float>(defaultVal));
}

void CImGuiCommands::Initialize()
{
    gEngfuncs.pfnAddCommand("ui_commands", CmdShowCommandsWindow);
}

void CImGuiCommands::VidInitialize()
{
    m_AppIcon = m_ImguiUtils.LoadImageFromMemory(app_icon, app_icon_len);
    m_CLSettingsIcon = m_ImguiUtils.LoadImageFromMemory(cl_settings_icon_png, cl_settings_icon_png_len);
    m_CrosshairIcon = m_ImguiUtils.LoadImageFromMemory(crosshair_icon_png, crosshair_icon_png_len);
    m_DrawIcon = m_ImguiUtils.LoadImageFromMemory(draw_icon_png, draw_icon_png_len);
    m_SettingsIcon = m_ImguiUtils.LoadImageFromMemory(settings_icon_png, settings_icon_png_len);
}

void CImGuiCommands::Terminate()
{
    m_ImguiUtils.FreeImage(m_AppIcon);
    m_ImguiUtils.FreeImage(m_CLSettingsIcon);
    m_ImguiUtils.FreeImage(m_CrosshairIcon);
    m_ImguiUtils.FreeImage(m_DrawIcon);
    m_ImguiUtils.FreeImage(m_SettingsIcon);
}

void CImGuiCommands::Think() {}

bool CImGuiCommands::Active()         
{ 
    return m_ShowCommands; 
}

bool CImGuiCommands::CursorRequired() 
{ 
    return m_ShowCommands; 
}

bool CImGuiCommands::HandleKey(bool keyDown, int keyNumber, const char *bindName)
{
    if (keyNumber == K_ESCAPE)
    {
        m_ShowCommands = false;
        return true;
    }
    return false;
}

void CImGuiCommands::CmdShowCommandsWindow()
{
    m_ShowCommands = !m_ShowCommands;
}

bool CImGuiCommands::SidebarButton(const char* pLabel, ImTextureID iconTex, bool isSelected, ImVec2 btnSize, float uiScale, float xPos)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems) 
        return false;

    ImGui::SetCursorPosX(xPos);

    ImGuiContext& ctx = *GImGui;
    const ImGuiStyle& style = ctx.Style;
    const ImGuiID id = pWin->GetID(pLabel);
    const ImVec2 labelSz = ImGui::CalcTextSize(pLabel, nullptr, true);

    const ImVec2 cursorPos = pWin->DC.CursorPos;
    const ImVec2 size = ImGui::CalcItemSize(btnSize, labelSz.x + style.FramePadding.x * 2.0f, labelSz.y + style.FramePadding.y * 2.0f);

    const ImRect bb(cursorPos, ImVec2(cursorPos.x + size.x, cursorPos.y + size.y));
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) 
        return false;

    bool isHovered, isHeld;
    const bool wasPressed = ImGui::ButtonBehavior(bb, id, &isHovered, &isHeld);

    ImU32 bgCol = IM_COL32(0,   0,   0,   0);
    ImU32 textCol = IM_COL32(140, 140, 150, 255);

    if (isSelected) 
    { 
        bgCol = IM_COL32(45, 45, 58, 255); 
        textCol = IM_COL32(255, 255, 255, 255); 
    }
    else if (isHovered)  
    { 
        bgCol = IM_COL32(32, 32, 42, 255); 
        textCol = IM_COL32(210, 210, 220, 255); 
    }

    if (bgCol)
        pWin->DrawList->AddRectFilled(bb.Min, bb.Max, bgCol, 7.0f * uiScale);

    if (isSelected)
    {
        pWin->DrawList->AddRectFilled(ImVec2(bb.Min.x, bb.Min.y + 8.0f * uiScale), ImVec2(bb.Min.x + 3.0f * uiScale, bb.Max.y - 8.0f * uiScale), IM_COL32(80, 160, 255, 255), 4.0f * uiScale);
    }

    const float iconSz = 18.0f * uiScale;
    const ImVec2 iconPos = ImVec2(bb.Min.x + 14.0f * uiScale, bb.Min.y + (size.y - iconSz) * 0.5f);
    const ImU32  iconTint = isSelected ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 160, 200);

    pWin->DrawList->AddImage(iconTex, iconPos, ImVec2(iconPos.x + iconSz, iconPos.y + iconSz), ImVec2(0, 0), ImVec2(1, 1), iconTint);

    if (labelSz.x > 0.0f)
    {
        const ImVec2 textPos = ImVec2(bb.Min.x + 42.0f * uiScale, bb.Min.y + (size.y - labelSz.y) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, textCol);
        ImGui::RenderText(textPos, pLabel);
        ImGui::PopStyleColor();
    }

    return wasPressed;
}

void CImGuiCommands::DrawCrosshairPreview(float previewSz)
{
    const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
    const ImVec2 canvasMax = ImVec2(canvasMin.x + previewSz, canvasMin.y + previewSz);

    ImDrawList* draw = ImGui::GetWindowDrawList();

    const float rounding = 16.0f;

    draw->PushClipRect(canvasMin, canvasMax, true);

    const int numTiles = 14; 
    const float tileSize = previewSz / static_cast<float>(numTiles);

    const ImU32 colDark  = IM_COL32(20, 20, 26, 255);
    const ImU32 colLight = IM_COL32(32, 32, 42, 255);

    for (int y = 0; y < numTiles; ++y)
    {
        for (int x = 0; x < numTiles; ++x)
        {
            ImU32 tileCol = ((x + y) % 2 == 0) ? colDark : colLight;

            ImVec2 tileMin(canvasMin.x + x * tileSize, canvasMin.y + y * tileSize);
            ImVec2 tileMax(canvasMin.x + (x + 1) * tileSize, canvasMin.y + (y + 1) * tileSize);

            draw->AddRectFilled(tileMin, tileMax, tileCol);
        }
    }

    float xPos = canvasMin.x + previewSz * 0.5f;
    float yPos = canvasMin.y + previewSz * 0.5f;

    float alpha = gEngfuncs.pfnGetCvarFloat("cl_cross_alpha");

    int r = 0, g = 255, b = 0;
    const char* pColorStr = gEngfuncs.pfnGetCvarString("cl_cross_color");
    if (pColorStr && *pColorStr)
        std::sscanf(pColorStr, "%d %d %d", &r, &g, &b);
    ImU32 crossColor = IM_COL32(r, g, b, (int)alpha);

    int dot_r = r, dot_g = g, dot_b = b;
    const char* pDotStr = gEngfuncs.pfnGetCvarString("cl_cross_dot_color");
    if (pDotStr && *pDotStr)
        std::sscanf(pDotStr, "%d %d %d", &dot_r, &dot_g, &dot_b);
    ImU32 dotColor = IM_COL32(dot_r, dot_g, dot_b, (int)alpha);

    int circle_r = r, circle_g = g, circle_b = b;
    const char* pCircleStr = gEngfuncs.pfnGetCvarString("cl_cross_circle_color");
    if (pCircleStr && *pCircleStr)
        std::sscanf(pCircleStr, "%d %d %d", &circle_r, &circle_g, &circle_b);
    ImU32 circleColor = IM_COL32(circle_r, circle_g, circle_b, (int)alpha);

    float size = gEngfuncs.pfnGetCvarFloat("cl_cross_size");
    float thickness = gEngfuncs.pfnGetCvarFloat("cl_cross_thickness");
    float gap = gEngfuncs.pfnGetCvarFloat("cl_cross_gap");
    float outline = gEngfuncs.pfnGetCvarFloat("cl_cross_outline");
    float dotsize = gEngfuncs.pfnGetCvarFloat("cl_cross_dot_size");

    float circlegap = gEngfuncs.pfnGetCvarFloat("cl_cross_circle_gap");
    float radius = gEngfuncs.pfnGetCvarFloat("cl_cross_circle_radius");

    ImU32 black = IM_COL32(0, 0, 0, (int)alpha);

    auto drawRect = [&](float x1, float y1, float x2, float y2, ImU32 color)
    {
        draw->AddRectFilled(ImVec2(roundf(x1), roundf(y1)), ImVec2(roundf(x2), roundf(y2)), color);
    };

    if (outline > 0.0f)
    {
        if (gEngfuncs.pfnGetCvarFloat("cl_cross_top_line") != 0.0f)
            drawRect(xPos - gap/2 - outline, yPos - size - thickness - outline, xPos + gap/2 + outline, yPos - size + outline, black);

        if (gEngfuncs.pfnGetCvarFloat("cl_cross_bottom_line") != 0.0f)
            drawRect(xPos - gap/2 - outline, yPos + size - outline, xPos + gap/2 + outline, yPos + size + thickness + outline, black);

        if (gEngfuncs.pfnGetCvarFloat("cl_cross_left_line") != 0.0f)
            drawRect(xPos - size - thickness - outline, yPos - gap/2 - outline, xPos - size + outline, yPos + gap/2 + outline, black);

        if (gEngfuncs.pfnGetCvarFloat("cl_cross_right_line") != 0.0f)
            drawRect(xPos + size - outline, yPos - gap/2 - outline, xPos + size + thickness + outline, yPos + gap/2 + outline, black);

        if (dotsize > 0.0f)
            draw->AddCircleFilled(ImVec2(xPos, yPos), dotsize/2 + outline, black);

        if (radius > 0.0f)
            draw->AddCircle(ImVec2(xPos, yPos), radius, black, 128, circlegap + outline);
    }

    if (gEngfuncs.pfnGetCvarFloat("cl_cross_top_line") != 0.0f)
        drawRect(xPos - gap/2, yPos - size - thickness, xPos + gap/2, yPos - size, crossColor);

    if (gEngfuncs.pfnGetCvarFloat("cl_cross_bottom_line") != 0.0f)
        drawRect(xPos - gap/2, yPos + size, xPos + gap/2, yPos + size + thickness, crossColor);

    if (gEngfuncs.pfnGetCvarFloat("cl_cross_left_line") != 0.0f)
        drawRect(xPos - size - thickness, yPos - gap/2, xPos - size, yPos + gap/2, crossColor);

    if (gEngfuncs.pfnGetCvarFloat("cl_cross_right_line") != 0.0f)
        drawRect(xPos + size, yPos - gap/2, xPos + size + thickness, yPos + gap/2, crossColor);

    if (dotsize > 0.0f)
        draw->AddCircleFilled(ImVec2(xPos, yPos), dotsize/2, dotColor);

    if (radius > 0.0f)
        draw->AddCircle(ImVec2(xPos, yPos), radius, circleColor, 128, circlegap);

    draw->PopClipRect();

    ImU32 bgCol = ImGui::GetColorU32(ImGuiCol_ChildBg);
    if ((bgCol & IM_COL32_A_MASK) == 0)
        bgCol = ImGui::GetColorU32(ImGuiCol_WindowBg);

    auto drawCornerMask = [&](ImVec2 cornerPos, ImVec2 centerPos, float startAngle, float endAngle)
    {
        const int numSegments = 12;
        for (int i = 0; i < numSegments; ++i)
        {
            float a0 = startAngle + (endAngle - startAngle) * (float)i / (float)numSegments;
            float a1 = startAngle + (endAngle - startAngle) * (float)(i + 1) / (float)numSegments;

            ImVec2 p0(centerPos.x + rounding * std::cos(a0), centerPos.y + rounding * std::sin(a0));
            ImVec2 p1(centerPos.x + rounding * std::cos(a1), centerPos.y + rounding * std::sin(a1));

            draw->AddTriangleFilled(cornerPos, p0, p1, bgCol);
        }
    };
    
    drawCornerMask(canvasMin, ImVec2(canvasMin.x + rounding, canvasMin.y + rounding), IM_PI, 1.5f * IM_PI);
    drawCornerMask(ImVec2(canvasMax.x, canvasMin.y), ImVec2(canvasMax.x - rounding, canvasMin.y + rounding), 1.5f * IM_PI, 2.0f * IM_PI);
    drawCornerMask(canvasMax, ImVec2(canvasMax.x - rounding, canvasMax.y - rounding), 0.0f, 0.5f * IM_PI);
    drawCornerMask(ImVec2(canvasMin.x, canvasMax.y), ImVec2(canvasMin.x + rounding, canvasMax.y - rounding), 0.5f * IM_PI, IM_PI);

    draw->AddRect(canvasMin, canvasMax, IM_COL32(65, 65, 80, 255), rounding, 0, 1.5f);

    ImGui::Dummy(ImVec2(previewSz, previewSz));
}

void CImGuiCommands::DrawTabClient()
{
    ImGui::TextDisabled("MOVEMENT");
    ImGui::Separator();
    ImGui::Spacing();

    DrawCheckbox("Auto Jump", "cl_autojump", "Automatically jumps when landing while holding +jump", "1");
    ImGui::TextWrapped("+ducktap: Performs a duck tap on landing (1-frame +duck). Bind in controls");

    ImGui::Spacing();
    ImGui::TextDisabled("WEAPON & MODELS");
    ImGui::Separator();
    ImGui::Spacing();

    DrawCheckbox("Weapon Lowering", "cl_weaponlowering", "Lowers weapon based on movement speed", "1");
    DrawCheckbox("Weapon Sway", "cl_weaponsway", "Adds weapon sway while moving", "1");
    DrawCheckbox("Weapon Lag", "cl_weaponlag", "Adds weapon lag when turning", "1");
    DrawCheckbox("Hide Corpses", "cl_hidecorpses", "Hides player corpses", "0");

    ImGui::Spacing();
    ImGui::TextDisabled("VIEWMODEL OFFSETS");
    ImGui::Separator();
    ImGui::Spacing();

    DrawSliderFloatWithReset("Right", "cl_viewmodel_ofs_right", -20.0f, 20.0f, 0.0f, "Viewmodel right offset");
    DrawSliderFloatWithReset("Forward", "cl_viewmodel_ofs_forward", -20.0f, 20.0f, 0.0f, "Viewmodel forward offset");
    DrawSliderFloatWithReset("Up", "cl_viewmodel_ofs_up", -20.0f, 20.0f, 0.0f, "Viewmodel vertical offset");

    ImGui::Spacing();
    ImGui::TextDisabled("GAUSS SPRITES");
    ImGui::Separator();
    ImGui::Spacing();

    DrawCheckbox("Gauss Balls", "cl_gauss_balls", "Enables Gauss impact particles", "1");
    DrawCheckbox("Gauss Hits",  "cl_gauss_hits",  "Enables Gauss impact sprite",    "1");

    ImGui::Spacing();
    ImGui::TextDisabled("CHAT & SOUNDS");
    ImGui::Separator();
    ImGui::Spacing();

    {
        int logChat = static_cast<int>(gEngfuncs.pfnGetCvarFloat("cl_logchat"));
        int newLogChat = logChat;
        ImGui::Text("Chat Logging:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Off", newLogChat == 0)) 
            newLogChat = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton("Players", newLogChat == 1)) 
            newLogChat = 1;
        ImGui::SameLine();
        if (ImGui::RadioButton("All", newLogChat == 2)) 
            newLogChat = 2;
        if (newLogChat != logChat)
            m_ImguiUtils.SetCvarFloat("cl_logchat", static_cast<float>(newLogChat));
        HelpTooltip("cl_logchat", "Chat logging mode\n0 = off\n1 = players only\n2 = system + players", "0", "0 – 2");
    }

    ImGui::Spacing();

    {
        bool chatSound = gEngfuncs.pfnGetCvarFloat("cl_chatsound") != 0.0f;
        if (ImGui::Checkbox("Chat Sound", &chatSound))
            m_ImguiUtils.SetCvarFloat("cl_chatsound", chatSound ? 1.0f : 0.0f);
        HelpTooltip("cl_chatsound", "Plays a sound when a chat message arrives", "1", "0 – 2");

        if (chatSound)
        {
            ImGui::Indent();
            const char* pPath = gEngfuncs.pfnGetCvarString("cl_chatsound_path");
            char pathBuf[256];
            std::strncpy(pathBuf, pPath ? pPath : "", sizeof(pathBuf) - 1);
            pathBuf[sizeof(pathBuf) - 1] = '\0';
            ImGui::InputText("##chatpath", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            ImGui::TextDisabled("cl_chatsound_path");
            ImGui::Unindent();
        }
    }

    {
        bool killSound = gEngfuncs.pfnGetCvarFloat("cl_killsound") != 0.0f;
        if (ImGui::Checkbox("Kill Sound", &killSound))
            m_ImguiUtils.SetCvarFloat("cl_killsound", killSound ? 1.0f : 0.0f);
        HelpTooltip("cl_killsound", "Plays a sound when you kill a player", "1");

        if (killSound)
        {
            ImGui::Indent();
            const char* pPath = gEngfuncs.pfnGetCvarString("cl_killsound_path");
            char pathBuf[256];
            std::strncpy(pathBuf, pPath ? pPath : "", sizeof(pathBuf) - 1);
            pathBuf[sizeof(pathBuf) - 1] = '\0';
            ImGui::InputText("##killpath", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            ImGui::TextDisabled("cl_killsound_path");
            ImGui::Unindent();
        }
    }

    ImGui::Spacing();
    ImGui::TextDisabled("DEBUG & UI TOOLS");
    ImGui::Separator();
    ImGui::Spacing();

    DrawCheckbox("Show FPS", "cl_debug_showfps", "Shows FPS counter", "1");

    ImGui::Spacing();
    if (ImGui::Button("Open ImGui Demo Window"))
        gEngfuncs.pfnClientCmd("ui_imgui_demo");
    HelpTooltip("ui_imgui_demo", "Opens standard ImGui Demo Window for UI testing");
}

void CImGuiCommands::DrawTabHUD()
{
    ImGui::TextDisabled("HUD ELEMENTS");
    ImGui::Separator();
    ImGui::Spacing();

    DrawCheckbox("ImGuiStyle HUD", "hud_new", "New HUD made with ImGui",  "1");
    DrawCheckbox("Weapon Panel", "hud_weapon", "Displays current weapon in HUD", "0");
    DrawCheckbox("Watermark", "hud_watermark", "Displays client watermark", "1");
    DrawCheckbox("Deathnotice BG", "hud_deathnotice_bg", "Enables death notice background", "1");
    DrawCheckbox("Health Divider", "hud_health_divider", "Shows health separator bar", "1");
    DrawCheckbox("HD Sprites", "hud_allow_hd", "Enables HL25-style HUD sprites", "1");
    DrawCheckbox("Next Map", "hud_nextmap", "Displays next map", "1");

    ImGui::Spacing();

    {
        int  timerMode = static_cast<int>(gEngfuncs.pfnGetCvarFloat("hud_timer"));
        int  newTimerMode = timerMode;
        ImGui::Text("Timer Mode:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Off##tmr", newTimerMode == 0)) 
            newTimerMode = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton("Time Left##tmr", newTimerMode == 1)) 
            newTimerMode = 1;
        ImGui::SameLine();
        if (ImGui::RadioButton("Elapsed##tmr", newTimerMode == 2)) 
            newTimerMode = 2;
        ImGui::SameLine();
        if (ImGui::RadioButton("Clock##tmr", newTimerMode == 3)) 
            newTimerMode = 3;
        if (newTimerMode != timerMode)
            m_ImguiUtils.SetCvarFloat("hud_timer", static_cast<float>(newTimerMode));
        HelpTooltip("hud_timer", "HUD timer mode.\n0 = off\n1 = time left\n2 = time passed\n3 = clock", "1", "0 – 3");
    }

    ImGui::Spacing();

    DrawSliderFloatWithReset("HUD Scale", "hud_new_scale", 0.5f, 3.0f, 1.4f, "Adjusts overall scale of new ImGui HUD elements", "%.2f");
    DrawSliderFloatWithReset("UI Scale Factor", "ui_imgui_scale", 0.5f, 3.0f, 1.0f, "UI scaling factor for ImGui elements", "%.2f");

    ImGui::Spacing();
    ImGui::TextDisabled("SPEED & STRAFE");
    ImGui::Separator();
    ImGui::Spacing();

    {
        bool speedometer = gEngfuncs.pfnGetCvarFloat("hud_speedometer") != 0.0f;
        if (ImGui::Checkbox("Speedometer", &speedometer))
            m_ImguiUtils.SetCvarFloat("hud_speedometer", speedometer ? 1.0f : 0.0f);
        HelpTooltip("hud_speedometer", "Displays player movement speed.", "0", "0 – 1");

        if (speedometer)
        {
            ImGui::Indent();
            DrawCheckbox("Below Crosshair##spd", "hud_speedometer_below_cross", "Positions speedometer below crosshair", "0");
            ImGui::Unindent();
        }
    }

    {
        bool jumpSpeed = gEngfuncs.pfnGetCvarFloat("hud_jumpspeed") != 0.0f;
        if (ImGui::Checkbox("Jump Speed", &jumpSpeed))
            m_ImguiUtils.SetCvarFloat("hud_jumpspeed", jumpSpeed ? 1.0f : 0.0f);
        HelpTooltip("hud_jumpspeed", "Displays speed at the jump moment", "0", "0 – 1");

        if (jumpSpeed)
        {
            ImGui::Indent();
            DrawCheckbox("Below Crosshair##jmp", "hud_jumpspeed_below_cross", "Positions jumpspeed below crosshair", "0");
            DrawSliderFloatWithReset("Vertical Offset##jmp", "hud_jumpspeed_height", -200.0f, 200.0f, 0.0f, "Adjusts jumpspeed vertical position", "%.0f");
            ImGui::Unindent();
        }
    }

    {
        bool strafeGuide = gEngfuncs.pfnGetCvarFloat("hud_strafeguide") != 0.0f;
        if (ImGui::Checkbox("Strafe Guide", &strafeGuide))
            m_ImguiUtils.SetCvarFloat("hud_strafeguide", strafeGuide ? 1.0f : 0.0f);
        HelpTooltip("hud_strafeguide", "Displays the strafing guide overlay.", "0", "0 – 1");

        if (strafeGuide)
        {
            ImGui::Indent();
            DrawSliderFloatWithReset("Zoom##sg", "hud_strafeguide_zoom", 0.1f, 5.0f,   1.0f, "Strafeguide zoom level", "%.2f");
            DrawSliderFloatWithReset("Height##sg", "hud_strafeguide_height", -300.0f, 300.0f, 0.0f, "Strafeguide vertical position", "%.0f");
            DrawSliderFloatWithReset("Size##sg", "hud_strafeguide_size", 0.0f, 500.0f, 0.0f, "Strafeguide size", "%.0f");
            ImGui::Unindent();
        }
    }

    ImGui::Spacing();
    ImGui::TextDisabled("MINI SCOREBOARD");
    ImGui::Separator();
    ImGui::Spacing();

    DrawSliderIntWithReset("Players Shown", "ui_scores", 0, 32,  0, "Displays mini-scoreboard. Value = players shown");
    DrawSliderIntWithReset("Alpha", "ui_scores_alpha", 0, 255, 20, "Mini-scoreboard background transparency");

    {
        const char* pPosStr = gEngfuncs.pfnGetCvarString("ui_scores_pos");
        float posX = 30.0f, posY = 50.0f;
        if (pPosStr && *pPosStr)
            std::sscanf(pPosStr, "%f %f", &posX, &posY);

        bool changed = false;
        ImGui::Text("Position (X Y):");
        ImGui::SameLine();
        changed |= ImGui::DragFloat("##scoreX", &posX, 1.0f, 0.0f, 100.0f, "X: %.0f");
        ImGui::SameLine();
        changed |= ImGui::DragFloat("##scoreY", &posY, 1.0f, 0.0f, 100.0f, "Y: %.0f");
        HelpTooltip("ui_scores_pos", "Controls the position of mini-scoreboard (X Y %)", "30 50");

        if (changed)
        {
            char cmdBuf[128];
            std::snprintf(cmdBuf, sizeof(cmdBuf), "ui_scores_pos \"%.0f %.0f\"", posX, posY);
            gEngfuncs.pfnClientCmd(cmdBuf);
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##ui_scores_pos"))
        {
            gEngfuncs.pfnClientCmd("ui_scores_pos \"30 50\"");
        }
    }
}

void CImGuiCommands::DrawTabVisuals()
{
    ImGui::TextDisabled("HUD COLOR");
    ImGui::Separator();
    ImGui::Spacing();

    {
        static ImVec4 hudColor;
        m_ImguiUtils.GetCvarColor("hud_color", (float*)&hudColor);
        if (ImGui::ColorEdit3("HUD Color", (float*)&hudColor))
            m_ImguiUtils.SetCvarColor("hud_color", (float*)&hudColor);
        HelpTooltip("hud_color", "Sets HUD color (RGB)", "255 160 0");
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##hc"))
        {
            hudColor = ImVec4(1.0f, 160.0f / 255.0f, 0.0f, 1.0f);
            m_ImguiUtils.SetCvarColor("hud_color", (float*)&hudColor);
        }
    }

    ImGui::Spacing();

    {
        bool rainbow = gEngfuncs.pfnGetCvarFloat("hud_rainbow") != 0.0f;
        if (ImGui::Checkbox("Rainbow HUD", &rainbow))
            m_ImguiUtils.SetCvarFloat("hud_rainbow", rainbow ? 1.0f : 0.0f);
        HelpTooltip("hud_rainbow", "Enables rainbow colour cycling on HUD.", "0", "0 – 1");

        if (rainbow)
        {
            ImGui::Indent();
            DrawSliderFloatWithReset("Saturation", "hud_rainbow_sat", 0.0f, 100.0f, 100.0f, "Rainbow saturation", "%.1f");
            DrawSliderFloatWithReset("Brightness", "hud_rainbow_val", 0.0f, 100.0f, 100.0f, "Rainbow brightness", "%.1f");
            DrawSliderFloatWithReset("Speed", "hud_rainbow_speed", 1.0f, 200.0f, 40.0f, "Rainbow animation speed", "%.1f");
            DrawSliderFloatWithReset("X Phase", "hud_rainbow_xphase", 0.0f, 1.0f, 0.4f, "Rainbow X phase offset", "%.2f");
            DrawSliderFloatWithReset("Y Phase", "hud_rainbow_yphase", 0.0f, 1.0f, 0.7f, "Rainbow Y phase offset", "%.2f");
            ImGui::Unindent();
        }
    }

    ImGui::Spacing();
    ImGui::TextDisabled("RELOAD INDICATOR");
    ImGui::Separator();
    ImGui::Spacing();

    {
        bool visReload = gEngfuncs.pfnGetCvarFloat("vis_reload") != 0.0f;
        if (ImGui::Checkbox("Reload Highlight", &visReload))
            m_ImguiUtils.SetCvarFloat("vis_reload", visReload ? 1.0f : 0.0f);
        HelpTooltip("vis_reload", "Highlights weapon sprite while reloading", "1", "0 – 1");

        if (visReload)
        {
            ImGui::Indent();
            static ImVec4 reloadColor;
            m_ImguiUtils.GetCvarColor("vis_reload_color", (float*)&reloadColor);
            if (ImGui::ColorEdit3("Reload Color", (float*)&reloadColor))
                m_ImguiUtils.SetCvarColor("vis_reload_color", (float*)&reloadColor);
            HelpTooltip("vis_reload_color", "Color used for reload highlight", "250 250 250");
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset##rc"))
            {
                reloadColor = ImVec4(250.0f/255.0f, 250.0f/255.0f, 250.0f/255.0f, 1.0f);
                m_ImguiUtils.SetCvarColor("vis_reload_color", (float*)&reloadColor);
            }
            ImGui::Unindent();
        }
    }

    ImGui::Spacing();
    ImGui::TextDisabled("HEALTH & ARMOR INDICATORS");
    ImGui::Separator();
    ImGui::Spacing();

    {
        bool hudVis = gEngfuncs.pfnGetCvarFloat("hud_vis") != 0.0f;
        if (ImGui::Checkbox("Dynamic Health and Armor Color Indicators", &hudVis))
            m_ImguiUtils.SetCvarFloat("hud_vis", hudVis ? 1.0f : 0.0f);
        HelpTooltip("hud_vis", "Enables dynamic health and armor coloring", "0", "0 – 1");

        if (hudVis)
        {
            ImGui::Indent();

            struct ColorEntry
            {
                const char* pLabel;
                const char* pCvar;
                ImVec4 defaultVal;
                const char* pDesc;
            };

            static ImVec4 healthColors[5];
            static ImVec4 batteryColors[5];

            static const ColorEntry kHealthEntries[5] = {
                { "Health 100%", "vis_health100", ImVec4(0.0f, 250.0f/255.0f, 0.0f, 1.0f), "Health color at 100%" },
                { "Health  80%", "vis_health80", ImVec4(0.0f, 250.0f/255.0f, 0.0f, 1.0f), "Health color at 80%"  },
                { "Health  60%", "vis_health60", ImVec4(250.0f/255.0f, 250.0f/255.0f, 0.0f, 1.0f), "Health color at 60%" },
                { "Health  40%", "vis_health40", ImVec4(250.0f/255.0f, 100.0f/255.0f, 0.0f, 1.0f), "Health color at 40%" },
                { "Health  20%", "vis_health20", ImVec4(250.0f/255.0f, 0.0f, 0.0f, 1.0f), "Health color at 20%"  },
            };

            static const ColorEntry kBatteryEntries[5] = {
                { "Battery 100%", "vis_battery100", ImVec4(0.0f, 250.0f/255.0f, 0.0f, 1.0f), "Battery color at 100%" },
                { "Battery  80%", "vis_battery80", ImVec4(0.0f, 250.0f/255.0f, 0.0f, 1.0f), "Battery color at 80%"  },
                { "Battery  60%", "vis_battery60", ImVec4(250.0f/255.0f, 250.0f/255.0f, 0.0f, 1.0f), "Battery color at 60%" },
                { "Battery  40%", "vis_battery40", ImVec4(250.0f/255.0f, 100.0f/255.0f, 0.0f, 1.0f), "Battery color at 40%" },
                { "Battery  20%", "vis_battery20", ImVec4(250.0f/255.0f, 0.0f, 0.0f, 1.0f), "Battery color at 20%"  },
            };

            ImGui::TextDisabled("Health");
            for (int i = 0; i < 5; ++i)
            {
                m_ImguiUtils.GetCvarColor(kHealthEntries[i].pCvar, (float*)&healthColors[i]);
                if (ImGui::ColorEdit3(kHealthEntries[i].pLabel, (float*)&healthColors[i]))
                    m_ImguiUtils.SetCvarColor(kHealthEntries[i].pCvar, (float*)&healthColors[i]);
                HelpTooltip(kHealthEntries[i].pCvar, kHealthEntries[i].pDesc);
                ImGui::SameLine();
                char rId[64]; std::snprintf(rId, sizeof(rId), "Reset##%s", kHealthEntries[i].pCvar);
                if (ImGui::SmallButton(rId))
                {
                    healthColors[i] = kHealthEntries[i].defaultVal;
                    m_ImguiUtils.SetCvarColor(kHealthEntries[i].pCvar, (float*)&healthColors[i]);
                }
            }

            ImGui::Spacing();
            ImGui::TextDisabled("Battery / Armor");
            for (int i = 0; i < 5; ++i)
            {
                m_ImguiUtils.GetCvarColor(kBatteryEntries[i].pCvar, (float*)&batteryColors[i]);
                if (ImGui::ColorEdit3(kBatteryEntries[i].pLabel, (float*)&batteryColors[i]))
                    m_ImguiUtils.SetCvarColor(kBatteryEntries[i].pCvar, (float*)&batteryColors[i]);
                HelpTooltip(kBatteryEntries[i].pCvar, kBatteryEntries[i].pDesc);
                ImGui::SameLine();
                char rId[64]; std::snprintf(rId, sizeof(rId), "Reset##%s", kBatteryEntries[i].pCvar);
                if (ImGui::SmallButton(rId))
                {
                    batteryColors[i] = kBatteryEntries[i].defaultVal;
                    m_ImguiUtils.SetCvarColor(kBatteryEntries[i].pCvar, (float*)&batteryColors[i]);
                }
            }

            ImGui::Unindent();
        }
    }
}

void CImGuiCommands::DrawTabCrosshair()
{
    {
        bool crossEnabled = gEngfuncs.pfnGetCvarFloat("cl_cross") != 0.0f;
        if (ImGui::Checkbox("Enable Custom Crosshair", &crossEnabled))
            m_ImguiUtils.SetCvarFloat("cl_cross", crossEnabled ? 1.0f : 0.0f);
        HelpTooltip("cl_cross", "Enables custom ImGui crosshair", "0", "0 – 1");

        if (!crossEnabled) return;
    }

    ImGui::Spacing();

    const float availW = ImGui::GetContentRegionAvail().x;
    const float previewSz = Q_min(220.0f, availW * 0.38f);
    const float settingsW = availW - previewSz - 24.0f;

    ImGui::BeginGroup();
    ImGui::BeginChild("##crossSettings", ImVec2(settingsW, 0.0f), false, ImGuiWindowFlags_NoScrollbar);

    ImGui::TextDisabled("COLOR & ALPHA");
    ImGui::Separator();
    ImGui::Spacing();

    {
        static ImVec4 crossColor;
        m_ImguiUtils.GetCvarColor("cl_cross_color", (float*)&crossColor);
        if (ImGui::ColorEdit3("Color##cross", (float*)&crossColor))
            m_ImguiUtils.SetCvarColor("cl_cross_color", (float*)&crossColor);
        HelpTooltip("cl_cross_color", "Crosshair line color (RGB).", "0 255 0");
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##cc"))
        {
            crossColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            m_ImguiUtils.SetCvarColor("cl_cross_color", (float*)&crossColor);
        }
    }

    DrawSliderIntWithReset("Alpha##cross", "cl_cross_alpha", 0, 255, 255, "Crosshair transparency.");

    ImGui::Spacing();
    ImGui::TextDisabled("SIZE");
    ImGui::Separator();
    ImGui::Spacing();

    DrawSliderIntWithReset("Size##s", "cl_cross_size", 0, 100, 10, "Crosshair arm length");
    DrawSliderIntWithReset("Gap##s", "cl_cross_gap", 0, 100, 3, "Gap between lines and center");
    DrawSliderIntWithReset("Thickness##s", "cl_cross_thickness", 1, 100, 2, "Line thickness in pixels");

    ImGui::Spacing();
    ImGui::TextDisabled("LINES");
    ImGui::Separator();
    ImGui::Spacing();

    DrawCheckbox("Top Line", "cl_cross_top_line", "Enables top crosshair arm", "1");
    DrawCheckbox("Bottom Line", "cl_cross_bottom_line", "Enables bottom crosshair arm", "1");
    DrawCheckbox("Left Line", "cl_cross_left_line", "Enables left crosshair arm", "1");
    DrawCheckbox("Right Line", "cl_cross_right_line", "Enables right crosshair arm", "1");
    DrawCheckbox("Outline", "cl_cross_outline", "Draws black outline", "1");

    ImGui::Spacing();
    ImGui::TextDisabled("DOT");
    ImGui::Separator();
    ImGui::Spacing();

    {
        int dotSz = static_cast<int>(gEngfuncs.pfnGetCvarFloat("cl_cross_dot_size"));
        if (ImGui::SliderInt("Dot Size", &dotSz, 0, 20))
            m_ImguiUtils.SetCvarFloat("cl_cross_dot_size", static_cast<float>(dotSz));
        HelpTooltip("cl_cross_dot_size", "Crosshair center dot radius. 0 = disabled", "1");
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##cl_cross_dot_size"))
            m_ImguiUtils.SetCvarFloat("cl_cross_dot_size", 0.0f);

        if (dotSz > 0)
        {
            ImGui::Indent();
            static ImVec4 dotColor;
            m_ImguiUtils.GetCvarColor("cl_cross_dot_color", (float*)&dotColor);
            if (ImGui::ColorEdit3("Dot Color", (float*)&dotColor))
                m_ImguiUtils.SetCvarColor("cl_cross_dot_color", (float*)&dotColor);
            HelpTooltip("cl_cross_dot_color", "Crosshair center dot color", "0 255 0");
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset##dc"))
            {
                dotColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                m_ImguiUtils.SetCvarColor("cl_cross_dot_color", (float*)&dotColor);
            }
            ImGui::Unindent();
        }
    }

    ImGui::Spacing();
    ImGui::TextDisabled("CIRCLE");
    ImGui::Separator();
    ImGui::Spacing();

    {
        int circleR = static_cast<int>(gEngfuncs.pfnGetCvarFloat("cl_cross_circle_radius"));
        if (ImGui::SliderInt("Circle Radius", &circleR, 0, 100))
            m_ImguiUtils.SetCvarFloat("cl_cross_circle_radius", static_cast<float>(circleR));
        HelpTooltip("cl_cross_circle_radius", "Crosshair circle radius. 0 = disabled", "0");
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##cl_cross_circle_radius"))
            m_ImguiUtils.SetCvarFloat("cl_cross_circle_radius", 0.0f);

        if (circleR > 0)
        {
            ImGui::Indent();
            DrawSliderIntWithReset("Circle Gap", "cl_cross_circle_gap", 0, 100, 3, "Gap between lines and circle");

            static ImVec4 circleColor;
            m_ImguiUtils.GetCvarColor("cl_cross_circle_color", (float*)&circleColor);
            if (ImGui::ColorEdit3("Circle Color", (float*)&circleColor))
                m_ImguiUtils.SetCvarColor("cl_cross_circle_color", (float*)&circleColor);
            HelpTooltip("cl_cross_circle_color", "Crosshair circle color", "0 255 0");
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset##circ"))
            {
                circleColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                m_ImguiUtils.SetCvarColor("cl_cross_circle_color", (float*)&circleColor);
            }
            ImGui::Unindent();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.10f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.05f, 0.05f, 1.0f));

    if (ImGui::Button("Reset All Crosshair Settings", ImVec2(settingsW - 8.0f, 0.0f)))
    {
        ImVec4 defGreen = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        m_ImguiUtils.SetCvarColor("cl_cross_color", (float*)&defGreen);
        m_ImguiUtils.SetCvarColor("cl_cross_dot_color", (float*)&defGreen);
        m_ImguiUtils.SetCvarColor("cl_cross_circle_color", (float*)&defGreen);
        m_ImguiUtils.SetCvarFloat("cl_cross_alpha", 255.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_thickness", 2.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_size", 10.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_gap", 3.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_outline", 1.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_circle_radius", 0.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_circle_gap", 3.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_dot_size", 0.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_top_line", 1.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_bottom_line", 1.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_left_line", 1.0f);
        m_ImguiUtils.SetCvarFloat("cl_cross_right_line", 1.0f);
    }

    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::TextDisabled("PREVIEW");
    ImGui::Separator();
    ImGui::Spacing();
    DrawCrosshairPreview(previewSz);
    ImGui::EndGroup();
}

void CImGuiCommands::Draw()
{
    if (!m_ShowCommands) return;

    const float scrW = static_cast<float>(g_ImGuiViewport.scrWidth());
    const float scrH = static_cast<float>(g_ImGuiViewport.scrHeight());

    const float uiScale = Q_max(0.5f, gEngfuncs.pfnGetCvarFloat("ui_imgui_scale"));
    const float headerH = 44.0f  * uiScale;
    const float sidebarW = 175.0f * uiScale;
    const float padding  = 12.0f  * uiScale;
    const float rounding = 10.0f  * uiScale;
    const float winW = scrW   * 0.84f;
    const float winH = scrH   * 0.84f;

    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2((scrW - winW) * 0.5f, (scrH - winH) * 0.5f),
        ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * uiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 6.0f * uiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 12.0f * uiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * uiScale, 6.0f * uiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f * uiScale);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.13f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.28f, 0.28f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.38f, 0.38f, 0.46f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.16f, 0.16f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.22f, 0.22f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.26f, 0.26f, 0.34f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.36f, 0.69f, 1.00f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.30f, 0.65f, 1.00f, 1.0f));

    constexpr ImGuiWindowFlags kWinFlags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavInputs |
        ImGuiWindowFlags_NoNavFocus;

    if (!ImGui::Begin("##AxionMenu", &m_ShowCommands, kWinFlags))
    {
        ImGui::End();
        ImGui::PopStyleColor(11);
        ImGui::PopStyleVar(7);
        return;
    }

    ImGui::SetWindowFontScale(uiScale);

    const ImVec2 winPos  = ImGui::GetWindowPos();
    const ImVec2 winSize = ImGui::GetWindowSize();
    ImDrawList*  pDraw   = ImGui::GetWindowDrawList();

    pDraw->AddRectFilled(winPos, ImVec2(winPos.x + winSize.x, winPos.y + headerH), IM_COL32(20, 20, 26, 255), rounding, ImDrawFlags_RoundCornersTop);

    pDraw->AddLine(ImVec2(winPos.x, winPos.y + headerH), ImVec2(winPos.x + winSize.x, winPos.y + headerH), IM_COL32(255, 255, 255, 10));

    const float iconSz = 22.0f * uiScale;
    ImGui::SetCursorPos(ImVec2(padding, (headerH - iconSz) * 0.5f));
    ImGui::Image(m_AppIcon.texture, ImVec2(iconSz, iconSz));

    ImGui::SetCursorPos(ImVec2(padding + iconSz + 8.0f * uiScale, (headerH - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::TextColored(ImVec4(0.90f, 0.90f, 0.92f, 1.0f), "Axion  |  Commands Menu");

    pDraw->AddRectFilled(ImVec2(winPos.x, winPos.y + headerH), ImVec2(winPos.x + sidebarW, winPos.y + winSize.y), IM_COL32(16, 16, 21, 255), rounding, ImDrawFlags_RoundCornersBottomLeft);

    pDraw->AddLine(ImVec2(winPos.x + sidebarW, winPos.y + headerH), ImVec2(winPos.x + sidebarW, winPos.y + winSize.y), IM_COL32(255, 255, 255, 7));

    ImGui::SetCursorPos(ImVec2(0.0f, headerH));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::BeginChild("##sidebar", ImVec2(sidebarW, winSize.y - headerH), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::Dummy(ImVec2(0, padding));

    struct TabEntry 
    { 
        const char* pLabel; 
        ImTextureID icon; 
    };

    const TabEntry kTabs[] = {
        { "Client", m_CLSettingsIcon.texture },
        { "HUD", m_SettingsIcon.texture },
        { "Visuals", m_DrawIcon.texture },
        { "Crosshair", m_CrosshairIcon.texture },
    };

    const float btnH = 44.0f * uiScale;
    const float btnX = padding * 0.5f;
    const ImVec2 btnSz(sidebarW - padding, btnH);

    for (int i = 0; i < 4; ++i)
    {
        if (SidebarButton(kTabs[i].pLabel, kTabs[i].icon, m_ActiveTab == i, btnSz, uiScale, btnX))
            m_ActiveTab = i;
    }

    const float closeBtnH = 36.0f * uiScale;
    const float sidebarContentH  = winSize.y - headerH;
    const float usedH = padding + static_cast<float>(4) * btnH + ImGui::GetStyle().ItemSpacing.y * 4;
    const float spaceLeft = sidebarContentH - usedH - closeBtnH - padding * 2.0f;

    if (spaceLeft > 0.0f)
        ImGui::Dummy(ImVec2(0, spaceLeft));

    ImGui::SetCursorPosX(btnX);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.07f, 0.07f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.10f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.38f, 0.04f, 0.04f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f * uiScale);

    if (ImGui::Button("Close", ImVec2(sidebarW - padding, closeBtnH)))
        m_ShowCommands = false;

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::PopStyleColor();

    const float contentX = sidebarW + padding;
    const float contentW = winSize.x - contentX - padding;
    const float contentH = winSize.y - headerH  - padding * 2.0f;

    ImGui::SetCursorPos(ImVec2(contentX, headerH + padding));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));

    ImGui::BeginChild("##content", ImVec2(contentW, contentH), false, ImGuiWindowFlags_None);

    ImGui::PushItemWidth(Q_min(260.0f * uiScale, contentW * 0.50f));

    switch (m_ActiveTab)
    {
        case 0: 
            DrawTabClient();    
            break;

        case 1: 
            DrawTabHUD();       
            break;
        
        case 2: 
            DrawTabVisuals();   
            break;

        case 3: 
            DrawTabCrosshair(); 
            break;
        
        default: 
            break;
    }

    ImGui::PopItemWidth();
    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    pDraw->AddRect(winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y), IM_COL32(50, 50, 62, 255), rounding, ImDrawFlags_RoundCornersAll, 1.0f);

    ImGui::End();

    ImGui::PopStyleColor(11);
    ImGui::PopStyleVar(7);
}