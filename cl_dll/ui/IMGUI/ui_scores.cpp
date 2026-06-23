#include "hud.h"
#include "cl_util.h"
#include "imgui.h"
#include "ui_scores.h"
#include "ui_ScorePanel.h"
#include "imgui_utils.h"

cvar_t* m_pCvarUIScores;
cvar_t* m_pCvarUIScoresPos;
cvar_t* m_pCvarUIScoresAlpha;

CImGuiScores g_ImGuiScores;

void CImGuiScores::Init()
{
    m_pCvarUIScores = CVAR_CREATE("ui_scores", "0", FCVAR_ARCHIVE);
	m_pCvarUIScoresPos = CVAR_CREATE("ui_scores_pos", "30 50", FCVAR_ARCHIVE);
    m_pCvarUIScoresAlpha = CVAR_CREATE("ui_scores_alpha", "20", FCVAR_ARCHIVE);
}

void CImGuiScores::Draw()
{
    if (g_iScoreboard.m_ShowScore == true)
        return;
    
    if (m_pCvarUIScores->value < 1)
		return;

	if (gEngfuncs.GetMaxClients() <= 1)
		return;

    g_iScoreboard.Update();

    m_iLines = 0;
	for (int iRow = 0; iRow < g_iScoreboard.m_iRows && m_iLines < m_pCvarUIScores->value; iRow++)
    {
        if (g_iScoreboard.m_iIsATeam[iRow] == 1 && gHUD.m_Teamplay)
        {
            team_info_t* team_info = &g_TeamInfo[g_iScoreboard.m_iSortedRows[iRow]];
            UIScoresData* data = &m_ScoresData[m_iLines];
            sprintf(data->szScore, "%-5i %s", team_info->frags, team_info->name);
            
            data->r = iTeamColors[team_info->teamnumber % iNumberOfTeamColors][0];
            data->g = iTeamColors[team_info->teamnumber % iNumberOfTeamColors][1];
            data->b = iTeamColors[team_info->teamnumber % iNumberOfTeamColors][2];
            
            m_iLines++;
        }
        else if (g_iScoreboard.m_iIsATeam[iRow] == 0 && !gHUD.m_Teamplay)
		{
            hud_player_info_t* pl_info = &g_PlayerInfoList[g_iScoreboard.m_iSortedRows[iRow]];
			extra_player_info_t* pl_info_extra = &g_PlayerExtraInfo[g_iScoreboard.m_iSortedRows[iRow]];
			if (pl_info->spectator && pl_info_extra->frags == 0)
				continue;
			UIScoresData* data = &m_ScoresData[m_iLines];
			sprintf(data->szScore, "%-5i %s", pl_info_extra->frags, pl_info->name);

			data->r = iTeamColors[pl_info_extra->teamnumber % iNumberOfTeamColors][0];
			data->g = iTeamColors[pl_info_extra->teamnumber % iNumberOfTeamColors][1];
			data->b = iTeamColors[pl_info_extra->teamnumber % iNumberOfTeamColors][2];
    
			m_iLines++;
        }
    }

	int xpos = 30;
	int ypos = 50;
	sscanf(m_pCvarUIScoresPos->string, "%i %i", &xpos, &ypos);

    int alpha = Q_min(255, Q_max(0, m_pCvarUIScoresAlpha->value));

    float fontSize = CVAR_GET_FLOAT("ui_imgui_font_scale");
    ImFont* font = ImGui::GetFont();
    
    float textHeight = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, "A").y;
    
    float paddingY = 3.0f; 
    float rowHeight = textHeight + (paddingY * 2.0f); 

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    float currentY = static_cast<float>(ypos);

    float maxStringWidth = 0.0f;
    for (int iLine = 0; iLine < m_iLines && iLine < m_pCvarUIScores->value; iLine++)
    {
        float stringWidth = m_ImguiUtils.CalcTextWidthWithColorCodes(m_ScoresData[iLine].szScore, fontSize);
        if (stringWidth > maxStringWidth)
            maxStringWidth = stringWidth;
    }

    float padLeft = 10.0f;
    float padRight = 15.0f;
    float fullBgWidth = maxStringWidth + padLeft + padRight;

    for (int iLine = 0; iLine < m_iLines && iLine < m_pCvarUIScores->value; iLine++)
    {
        UIScoresData* data = &m_ScoresData[iLine];

        ImVec2 bgMin = ImVec2(static_cast<float>(xpos) - padLeft, currentY - paddingY);
        ImVec2 bgMax = ImVec2(bgMin.x + fullBgWidth, currentY + textHeight + paddingY);

        ImU32 rowBgColor = IM_COL32(data->r, data->g, data->b, alpha);
        
        dl->AddRectFilled(bgMin, bgMax, rowBgColor);

        m_ImguiUtils.DrawText(fontSize, ImVec2(static_cast<float>(xpos), currentY), data->szScore, IM_COL32(data->r, data->g, data->b, 255), true);
        
        currentY += rowHeight + 2.0f;
    }

}