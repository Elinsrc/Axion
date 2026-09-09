#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "pm_shared.h"
#include "imgui.h"
#include "imgui_manager.h"
#include "imgui_viewport.h"
#include "imgui_utils.h"
#include "ui_SpectatorPanel.h"
#include "ui_ScorePanel.h"

#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
#include "avatar_cache.h"
#endif

CImGuiSpectatorPanel m_iSpectatorPanel;

const char *GetSpectatorLabel( int iMode )
{
	switch( iMode )
	{
		case OBS_CHASE_LOCKED:
			return "#OBS_CHASE_LOCKED";

		case OBS_CHASE_FREE:
			return "#OBS_CHASE_FREE";

		case OBS_ROAMING:
			return "#OBS_ROAMING";
		
		case OBS_IN_EYE:
			return "#OBS_IN_EYE";

		case OBS_MAP_FREE:
			return "#OBS_MAP_FREE";

		case OBS_MAP_CHASE:
			return "#OBS_MAP_CHASE";

		case OBS_NONE:
		default:
			return "#OBS_NONE";
	}

	return "";
}

void CImGuiSpectatorPanel::Init()
{
    m_bVisible = false;
    m_szTargetName[0] = '\0';
    m_szCamMode[0] = '\0';
    m_TargetColor = ImVec4( 0.88f, 0.75f, 0.22f, 1.0f );
}

void CImGuiSpectatorPanel::setBottomText( const char *text )
{
    strncpy( m_szTargetName, text ? text : "", sizeof(m_szTargetName) - 1 );
    m_szTargetName[sizeof(m_szTargetName) - 1] = '\0';
}

void CImGuiSpectatorPanel::setCamMode( const char *text )
{
    strncpy( m_szCamMode, text ? text : "", sizeof(m_szCamMode) - 1 );
    m_szCamMode[sizeof(m_szCamMode) - 1] = '\0';
}

void CImGuiSpectatorPanel::setTargetColor( float r, float g, float b )
{
    m_TargetColor = ImVec4( r, g, b, 1.0f );
}

void CImGuiSpectatorPanel::Draw()
{
    if( !m_bVisible )
        return;

    DrawPlayerCard();
}

void CImGuiSpectatorPanel::DrawPlayerCard()
{
    ImGui::PushFont(g_ImGuiManager.GetHudFont());

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    float lineHeight = ImGui::GetTextLineHeight();
    const float padding = 8.0f;
    const float gap = 4.0f;

    const char* modeText = m_szCamMode[0] ? m_szCamMode : CHudTextMessage::BufferedLocaliseTextString(GetSpectatorLabel(g_iUser1));
    const char* nameText = m_szTargetName[0] ? m_szTargetName : nullptr;

    float modeWidth = m_ImguiUtils.CalcTextWidthWithColorCodes(modeText);
    float nameWidth = nameText ? m_ImguiUtils.CalcTextWidthWithColorCodes(nameText) : 0.0f;

#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
    int targetIndex = g_iUser2;
    bool bHasValidTarget = (targetIndex >= 1 && targetIndex < MAX_PLAYERS && g_PlayerInfoList[targetIndex].name && g_PlayerInfoList[targetIndex].name[0]);

    bool bShowAvatar = bHasValidTarget && (g_iUser1 == OBS_CHASE_LOCKED || g_iUser1 == OBS_CHASE_FREE || g_iUser1 == OBS_IN_EYE || g_iUser1 == OBS_MAP_CHASE);

    float avatarSize   = 48.0f;
    float avatarOverlap = bShowAvatar ? (avatarSize * 0.5f) : 0.0f;
    float topMargin    = bShowAvatar ? (avatarOverlap + 6.0f) : padding;

    ImU32 playerColor = IM_COL32((int)(m_TargetColor.x * 255), (int)(m_TargetColor.y * 255), (int)(m_TargetColor.z * 255), 255);

    if (bHasValidTarget)
    {
        extra_player_info_t* ex = &g_PlayerExtraInfo[targetIndex];
        int teamColorIdx = ex->teamnumber % iNumberOfTeamColors;
        playerColor = IM_COL32(iTeamColors[teamColorIdx][0], iTeamColors[teamColorIdx][1], iTeamColors[teamColorIdx][2], 255);
    }
#else
    bool bShowAvatar = false;
    float topMargin  = padding;
#endif

    float bgWidth = fmaxf(nameWidth, modeWidth) + padding * 4.0f;

#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
    if (bShowAvatar && bgWidth < avatarSize + 60.0f)
        bgWidth = avatarSize + 60.0f;
#endif

    float contentHeight = nameText ? (lineHeight * 2.0f + gap) : lineHeight;
    float bgHeight = topMargin + contentHeight + padding;

    float bgPosX = (g_ImGuiViewport.scrWidth() - bgWidth) * 0.5f;
    float bgPosY = g_ImGuiViewport.scrHeight() - bgHeight - 20.0f;

    ImU32 bgColor = gHUD.m_Teamplay ? IM_COL32((int)(m_TargetColor.x * 255), (int)(m_TargetColor.y * 255), (int)(m_TargetColor.z * 255), 140) : IM_COL32(0, 0, 0, 140);

    ImVec4 nameColor = gHUD.m_Teamplay ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(1.0f, 0.85f, 0.2f, 1.0f);

    ImVec4 modeColor = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);

    drawList->AddRectFilled(ImVec2(bgPosX, bgPosY), ImVec2(bgPosX + bgWidth, bgPosY + bgHeight), bgColor, 6.0f);
    drawList->AddRect(ImVec2(bgPosX, bgPosY), ImVec2(bgPosX + bgWidth, bgPosY + bgHeight), IM_COL32(255, 255, 255, 40), 6.0f, 0, 1.0f);

    if (nameText)
    {
        float nameY = bgPosY + topMargin;
        m_ImguiUtils.DrawTextWithColorCodesAt(ImVec2(bgPosX + (bgWidth - nameWidth) * 0.5f, nameY), nameText, nameColor);

        float separatorY = nameY + lineHeight + gap * 0.5f;
        drawList->AddLine(ImVec2(bgPosX + padding, separatorY), ImVec2(bgPosX + bgWidth - padding, separatorY), IM_COL32(255, 255, 255, 30));
    }

    float modePosY = nameText ? (bgPosY + topMargin + lineHeight + gap) : (bgPosY + topMargin);

    m_ImguiUtils.DrawTextWithColorCodesAt(ImVec2(bgPosX + (bgWidth - modeWidth) * 0.5f, modePosY), modeText, modeColor);

#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
    if (bShowAvatar)
    {
        float avatarX = (g_ImGuiViewport.scrWidth() - avatarSize) * 0.5f;
        float avatarY = bgPosY - (avatarSize * 0.5f);

        ImVec2 avMin(avatarX, avatarY);
        ImVec2 avMax(avatarX + avatarSize, avatarY + avatarSize);

        drawList->AddRectFilled(avMin, avMax, IM_COL32(0, 0, 0, 255));

        g_AvatarCache.UpdatePlayer(targetIndex);
        ImTextureID avatarTex = g_AvatarCache.GetAvatar(targetIndex);
        drawList->AddImage(avatarTex, avMin, avMax);

        drawList->AddRect(ImVec2(avMin.x - 1.0f, avMin.y - 1.0f), ImVec2(avMax.x + 1.0f, avMax.y + 1.0f), playerColor, 2.0f, 0, 1.5f);
    }
#endif

    ImGui::PopFont();
}