#include "ui_MOTD.h"
#include "imgui.h"
#include "hud.h"
#include "keydefs.h"
#include "cl_util.h"
#include "imgui_viewport.h"

CImGuiMOTD g_iMOTD;

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

void CImGuiMOTD::Show( const char *text, const char *title )
{
    strncpy(m_szText, text ? text : "", sizeof(m_szText)  - 1 );
    strncpy(m_szTitle, title ? title : "Half-Life", sizeof(m_szTitle) - 1);

    m_szText[sizeof(m_szText) - 1] = '\0';
    m_szTitle[sizeof(m_szTitle) - 1] = '\0';

    m_ShowMOTD = true;
}

void CImGuiMOTD::Draw()
{
    if (!m_ShowMOTD)
        return;

    const char *motdText  = m_szText;
    const char *titleText = m_szTitle;
    const char *buttonText = CHudTextMessage::BufferedLocaliseTextString("#Menu_OK");

    float scrWidth = g_ImGuiViewport.scrWidth();
    float scrHeight = g_ImGuiViewport.scrHeight();
    
    float scrollbarWidth = 12.0f;
    ImVec2 windowPadding(10, 10);
    float btnHeight = 35.0f;
    float btnWidth = 120.0f;
    float spacing = 10.0f;
    float titleLineH = ImGui::GetTextLineHeight();
    float titleAreaH = titleLineH + 8.0f + spacing;

    float maxW = scrWidth * 0.8f;
    float maxH = scrHeight * 0.8f;

    ImVec2 textSize = ImGui::CalcTextSize(motdText, nullptr, false, maxW - (windowPadding.x * 2.0f) - scrollbarWidth);
    float winW = fminf(fmaxf(textSize.x + (windowPadding.x * 2.0f) + scrollbarWidth, 300.0f), maxW);
    
    ImVec2 wrappedSize = ImGui::CalcTextSize(motdText, nullptr, false, winW - (windowPadding.x * 2.0f) - scrollbarWidth);
    float totalNeededH = wrappedSize.y + btnHeight + (windowPadding.y * 2.0f) + spacing + titleAreaH + 20.0f;
    float winH = fminf(totalNeededH, maxH);

    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(scrWidth * 0.5f, scrHeight * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 150));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPadding);

    if (ImGui::Begin("##motdwin", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
    {
        float innerW = ImGui::GetContentRegionAvail().x;

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 160, 0, 255));
        m_ImguiUtils.TextWithColorCodesCentered(titleText);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(255, 160, 0, 80));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        float currentAvailH = ImGui::GetContentRegionAvail().y;
        float scrollZoneH = currentAvailH - (btnHeight + spacing);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(0, 0, 0, 80));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(255, 160, 0, 100));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(255, 160, 0, 180));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(255, 160, 0, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, scrollbarWidth);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 4.0f);
        
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 160, 0, 255));

        if (ImGui::BeginChild("##MOTDBody", ImVec2(0, scrollZoneH), false, ImGuiWindowFlags_None))
        {
            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - scrollbarWidth);
            m_ImguiUtils.TextWithColorCodes(motdText);
            ImGui::PopTextWrapPos();
        }
        ImGui::EndChild();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - btnHeight - windowPadding.y);
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnWidth) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 160, 0, 100));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 160, 0, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 160, 0, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

        if (ImGui::Button(buttonText, ImVec2(btnWidth, btnHeight)))
            m_ShowMOTD = false;

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
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
