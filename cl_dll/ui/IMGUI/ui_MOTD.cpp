#include "ui_MOTD.h"
#include "imgui.h"
#include "hud.h"
#include "keydefs.h"
#include "cl_util.h"
#include "imgui_viewport.h"

bool CImGuiMOTD::m_ShowMOTD = false;

void CImGuiMOTD::Initialize()
{
}

void CImGuiMOTD::VidInitialize()
{
}

void CImGuiMOTD::Terminate()
{
}

void CImGuiMOTD::Think()
{
}

void CImGuiMOTD::Draw()
{
    if (!m_ShowMOTD)
        return;

    const char* motdText = g_ImGuiViewport.m_szMOTD;
    const char* buttonText = CHudTextMessage::BufferedLocaliseTextString("#Menu_OK");

    ImVec2 padding(20, 20);
    float spacing = 15.0f;
    float buttonPaddingX = 30.0f;
    float buttonPaddingY = 10.0f;

    ImVec2 buttonTextSize = ImGui::CalcTextSize(buttonText);
    float btnWidth = buttonTextSize.x + buttonPaddingX * 2;
    float btnHeight = buttonTextSize.y + buttonPaddingY * 2;

    float minBtnWidth = 80.0f;
    float minBtnHeight = 30.0f;
    if (btnWidth < minBtnWidth) btnWidth = minBtnWidth;
    if (btnHeight < minBtnHeight) btnHeight = minBtnHeight;

    ImVec2 textSize = ImGui::CalcTextSize(motdText, nullptr, false, 0.0f);

    float windowWidth = textSize.x + padding.x * 2;
    float windowHeight = textSize.y + padding.y * 2 + btnHeight + spacing;

    float minWindowWidth = btnWidth + padding.x * 2;
    if (windowWidth < minWindowWidth)
        windowWidth = minWindowWidth;

    float maxWidth = g_ImGuiViewport.scrWidth() * 0.8f;
    float maxHeight = g_ImGuiViewport.scrHeight() * 0.8f;

    if (windowWidth > maxWidth)
        windowWidth = maxWidth;
    if (windowHeight > maxHeight)
        windowHeight = maxHeight;

    float minWidth = 200.0f;
    float minHeight = 100.0f;
    if (windowWidth < minWidth) windowWidth = minWidth;
    if (windowHeight < minHeight) windowHeight = minHeight;

    ImVec2 center(g_ImGuiViewport.scrWidth() * 0.5f, g_ImGuiViewport.scrHeight() * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));

    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 150));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 160, 0, 100));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 160, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 160, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 160, 0, 255));

    ImGui::Begin("##motdwin", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoMove);

    m_ImguiUtils.TextWithColorCodes(motdText);

    float winWidth = ImGui::GetWindowSize().x;
    float cursorX = (winWidth - btnWidth) * 0.5f;

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - btnHeight - padding.y);
    ImGui::SetCursorPosX(cursorX);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));

    if (ImGui::Button(buttonText, ImVec2(btnWidth, btnHeight)))
        m_ShowMOTD = false;

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(7);
}

bool CImGuiMOTD::Active()
{
    return m_ShowMOTD;
}

bool CImGuiMOTD::CursorRequired()
{
    return true;
}

bool CImGuiMOTD::HandleKey(bool keyDown, int keyNumber, const char *bindName)
{
    if (keyNumber == K_ESCAPE || keyNumber == K_ENTER)
    {
        m_ShowMOTD = false;
    }
    return false;
}
