#include "imgui.h"
#include "keydefs.h"
#include "imgui_viewport.h"
#include "ui_ScorePanel.h"
#include "voice_status.h"
#include "cl_util.h"
#include "imgui_utils.h"

extern int blue_flag_player_index;
extern int red_flag_player_index;

ImGuiImage m_pScoreboardVoiceBanned;
ImGuiImage m_pscoreboardVoiceSpeaking;
ImGuiImage m_pscoreboardVoiceSpeaking2;
ImGuiImage m_pscoreboardVoiceSpeaking3;
ImGuiImage m_pscoreboardVoiceSpeaking4;

CImGuiScoreboard g_iScoreboard;

bool CImGuiScoreboard::m_ShowScore = false;

void CImGuiScoreboard::Initialize()
{
	m_bMouseMode = false;

	InitHUDData();
}

void CImGuiScoreboard::InitHUDData()
{
	m_iLastKilledBy = 0;
	m_fLastKillTime = 0;
	m_iPlayerNum = 0;
	m_iNumTeams = 0;
	memset(g_PlayerExtraInfo, 0, sizeof(g_PlayerExtraInfo));
	memset(g_TeamInfo, 0, sizeof(g_TeamInfo));
}

void CImGuiScoreboard::VidInitialize()
{
	int iSprite = 0;
	iSprite = gHUD.GetSpriteIndex( "icon_ctf_score" );
	m_IconFlagScore.spr = gHUD.GetSprite( iSprite );
	m_IconFlagScore.rc = gHUD.GetSpriteRect( iSprite );

	m_pScoreboardVoiceBanned = m_ImguiUtils.LoadImageFromFile("gfx/vgui/640_voiceblocked.tga");
	m_pscoreboardVoiceSpeaking = m_ImguiUtils.LoadImageFromFile("gfx/vgui/640_speaker1.tga");
    m_pscoreboardVoiceSpeaking2 = m_ImguiUtils.LoadImageFromFile("gfx/vgui/640_speaker2.tga");
    m_pscoreboardVoiceSpeaking3 = m_ImguiUtils.LoadImageFromFile("gfx/vgui/640_speaker3.tga");
    m_pscoreboardVoiceSpeaking4 = m_ImguiUtils.LoadImageFromFile("gfx/vgui/640_speaker4.tga");
}

void CImGuiScoreboard::Terminate()
{
	m_ImguiUtils.FreeImage(m_pScoreboardVoiceBanned);
	m_ImguiUtils.FreeImage(m_pscoreboardVoiceSpeaking);
	m_ImguiUtils.FreeImage(m_pscoreboardVoiceSpeaking2);
	m_ImguiUtils.FreeImage(m_pscoreboardVoiceSpeaking3);
	m_ImguiUtils.FreeImage(m_pscoreboardVoiceSpeaking4);
}

void CImGuiScoreboard::Think()
{
	if (!m_ShowScore)
	{
		m_bMouseMode = false;
	}
}

void CImGuiScoreboard::RebuildTeams()
{
	// clear out player counts from teams
	int i;
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		g_TeamInfo[i].players = 0;
	}

	// rebuild the team list
	g_ImGuiViewport.GetAllPlayersInfo();
	m_iNumTeams = 0;
	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( g_PlayerInfoList[i].name == NULL )
			continue;

		if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
			continue; // skip over players who are not in a team

			// is this player in an existing team?
			int j;
		for ( j = 1; j <= m_iNumTeams; j++ )
		{
			if ( g_TeamInfo[j].name[0] == '\0' )
				break;

			if ( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
				break;
		}

		if ( j > m_iNumTeams )
		{ // they aren't in a listed team, so make a new one
			// search through for an empty team slot
			for ( j = 1; j <= m_iNumTeams; j++ )
			{
				if ( g_TeamInfo[j].name[0] == '\0' )
					break;
			}
			m_iNumTeams = Q_max( j, m_iNumTeams );

			strncpy( g_TeamInfo[j].name, g_PlayerExtraInfo[i].teamname, MAX_TEAM_NAME - 1 );
			g_TeamInfo[j].players = 0;
		}

		g_TeamInfo[j].players++;
	}

	// clear out any empty teams
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		if ( g_TeamInfo[i].players < 1 )
			memset( &g_TeamInfo[i], 0, sizeof(team_info_t) );
	}
}

void CImGuiScoreboard::SortTeams()
{
	// clear out team scores
	int i;
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		if( !g_TeamInfo[i].scores_overriden )
			g_TeamInfo[i].frags = g_TeamInfo[i].deaths = 0;
		g_TeamInfo[i].ping = g_TeamInfo[i].packetloss = 0;
	}

	// recalc the team scores, then draw them
	for( i = 1; i < MAX_PLAYERS; i++ )
	{
		if( g_PlayerInfoList[i].name == 0 )
			continue; // empty player slot, skip

			if( g_PlayerExtraInfo[i].teamname[0] == 0 )
				continue; // skip over players who are not in a team

				// find what team this player is in
				int j;
			for ( j = 1; j <= m_iNumTeams; j++ )
			{
				if( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
					break;
			}
			if( j > m_iNumTeams )  // player is not in a team, skip to the next guy
				continue;

		if( !g_TeamInfo[j].scores_overriden )
		{
			g_TeamInfo[j].frags += g_PlayerExtraInfo[i].frags;
			g_TeamInfo[j].deaths += g_PlayerExtraInfo[i].deaths;
		}

		g_TeamInfo[j].ping += g_PlayerInfoList[i].ping;
		g_TeamInfo[j].packetloss += g_PlayerInfoList[i].packetloss;

		if( g_PlayerInfoList[i].thisplayer )
			g_TeamInfo[j].ownteam = TRUE;
		else
			g_TeamInfo[j].ownteam = FALSE;

		// Set the team's number (used for team colors)
		g_TeamInfo[j].teamnumber = g_PlayerExtraInfo[i].teamnumber;
	}

	// find team ping/packetloss averages
	for( i = 1; i <= m_iNumTeams; i++ )
	{
		g_TeamInfo[i].already_drawn = FALSE;

		if( g_TeamInfo[i].players > 0 )
		{
			g_TeamInfo[i].ping /= g_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
			g_TeamInfo[i].packetloss /= g_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
		}
	}

	// Draw the teams
	while( 1 )
	{
		int highest_frags = -99999; int lowest_deaths = 99999;
		int best_team = 0;

		for( i = 1; i <= m_iNumTeams; i++ )
		{
			if( g_TeamInfo[i].players < 1 )
				continue;

			if( !g_TeamInfo[i].already_drawn && g_TeamInfo[i].frags >= highest_frags )
			{
				if( g_TeamInfo[i].frags > highest_frags || g_TeamInfo[i].deaths < lowest_deaths )
				{
					best_team = i;
					lowest_deaths = g_TeamInfo[i].deaths;
					highest_frags = g_TeamInfo[i].frags;
				}
			}
		}

		// draw the best team on the scoreboard
		if( !best_team )
			break;

		// Put this team in the sorted list
		m_iSortedRows[m_iRows] = best_team;
		m_iIsATeam[m_iRows] = TEAM_YES;
		g_TeamInfo[best_team].already_drawn = TRUE;  // set the already_drawn to be TRUE, so this team won't get sorted again
		m_iRows++;

		// Now sort all the players on this team
		SortPlayers( 0, g_TeamInfo[best_team].name );
	}

	// Add all the players who aren't in a team yet into spectators
	SortPlayers( TEAM_SPECTATORS, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Sort a list of players
//-----------------------------------------------------------------------------
void CImGuiScoreboard::SortPlayers( int iTeam, char *team )
{
	bool bCreatedTeam = false;

	// draw the players, in order,  and restricted to team if set
	while ( 1 )
	{
		// Find the top ranking player
		int highest_frags = -99999;	int lowest_deaths = 99999;
		int best_player;
		best_player = 0;

		for ( int i = 1; i < MAX_PLAYERS; i++ )
		{
			if ( m_bHasBeenSorted[i] == false && g_PlayerInfoList[i].name && g_PlayerExtraInfo[i].frags >= highest_frags )
			{
				cl_entity_t *ent = gEngfuncs.GetEntityByIndex( i );

				if ( ent && !(team && stricmp(g_PlayerExtraInfo[i].teamname, team)) )
				{
					extra_player_info_t *pl_info = &g_PlayerExtraInfo[i];
					if ( pl_info->frags > highest_frags || pl_info->deaths < lowest_deaths )
					{
						best_player = i;
						lowest_deaths = pl_info->deaths;
						highest_frags = pl_info->frags;
					}
				}
			}
		}

		if ( !best_player )
			break;

		// If we haven't created the Team yet, do it first
		if (!bCreatedTeam && iTeam)
		{
			m_iIsATeam[ m_iRows ] = iTeam;
			m_iRows++;

			bCreatedTeam = true;
		}

		// Put this player in the sorted list
		m_iSortedRows[ m_iRows ] = best_player;
		m_bHasBeenSorted[ best_player ] = true;
		m_iRows++;
	}

	if (team)
	{
		m_iIsATeam[m_iRows++] = TEAM_BLANK;
	}
}

void CImGuiScoreboard::DrawScoreboard()
{
	g_ImGuiViewport.GetAllPlayersInfo();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

	float scrWidth = g_ImGuiViewport.scrWidth();
	float scrHeight = g_ImGuiViewport.scrHeight();

	char playersText[64], teamsText[64], scoreText[64], deathText[64], latencyText[64], modelText[64], voiceText[64];
	strncpy(playersText, CHudTextMessage::BufferedLocaliseTextString("#PLAYERS"), sizeof(playersText) - 1);
	strncpy(teamsText, CHudTextMessage::BufferedLocaliseTextString("#TEAMS"), sizeof(teamsText) - 1);
	strncpy(scoreText, CHudTextMessage::BufferedLocaliseTextString("#SCORE"), sizeof(scoreText) - 1);
	strncpy(deathText, CHudTextMessage::BufferedLocaliseTextString("#DEATHS"), sizeof(deathText) - 1);
	strncpy(latencyText, CHudTextMessage::BufferedLocaliseTextString("#LATENCY"), sizeof(latencyText) - 1);
	strncpy(modelText, "MODEL", sizeof(modelText) - 1);
	strncpy(voiceText, CHudTextMessage::BufferedLocaliseTextString("#VOICE"), sizeof(voiceText) - 1);

	float colModel = ImGui::CalcTextSize(modelText).x + 24.f;
	float colScore = ImGui::CalcTextSize(scoreText).x + 24.f;
	float colDeath = ImGui::CalcTextSize(deathText).x + 24.f;
	float colPing = ImGui::CalcTextSize(latencyText).x + 24.f;
	float colVoice = ImGui::CalcTextSize(voiceText).x + 24.f;

	float minNameWidth = 150.f;

	float fixedColumnsWidth = colModel + colScore + colDeath + colPing + colVoice;
	float minWindowWidth = fixedColumnsWidth + minNameWidth + 40.f;

	float padding_y = scrHeight * 0.05f;
	float sb_width = scrWidth * 0.55f;

	if (sb_width < minWindowWidth)
		sb_width = minWindowWidth;

	if (sb_width > scrWidth - 20.f)
		sb_width = scrWidth - 20.f;

	float sb_height = scrHeight - 2 * padding_y;
	float sb_x = (scrWidth - sb_width) / 2;
	float sb_y = padding_y;

	if (sb_x < 10.f)
		sb_x = 10.f;

	ImGui::SetNextWindowPos(ImVec2(sb_x, sb_y));
	ImGui::SetNextWindowSize(ImVec2(sb_width, sb_height));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 150));
	ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_TableRowBg, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 160, 0, 255));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));      
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0)); 
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));  
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0,0,0,0));

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoCollapse;


	if (!m_bMouseMode)
		flags |= ImGuiWindowFlags_NoInputs;
	else
		flags |= ImGuiWindowFlags_NoNav;

	if (ImGui::Begin("##Scoreboard", &m_ShowScore, flags))
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 win_pos = ImGui::GetWindowPos();
		ImVec2 win_size = ImGui::GetWindowSize();

		float padding = ImGui::GetStyle().WindowPadding.x;
		float lineStartX = win_pos.x + padding;
		float lineEndX = win_pos.x + win_size.x - padding;

		float line_y = ImGui::GetCursorPosY();

		m_ImguiUtils.TextWithColorCodes(g_ImGuiViewport.m_szServerName);

		char player_count[256];
		sprintf(player_count, "%d/%d", get_player_count(), gEngfuncs.GetMaxClients());
		float text_width = ImGui::CalcTextSize(player_count).x;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - text_width - padding);
		ImGui::SetCursorPosY(line_y);
		ImGui::Text("%s", player_count);

		ImGui::Spacing();
		char map_name[64];
		get_map_name(map_name, ARRAYSIZE(map_name));
		ImGui::Text("%s", map_name);

		float tableHeight = sb_height - ImGui::GetCursorPosY() - 20.f;

		if (ImGui::BeginTable("ScoreboardTableInvisible", 6, ImGuiTableFlags_SizingFixedFit, ImVec2(0, tableHeight)))
		{
			if (!gHUD.m_Teamplay)
			{
				ImGui::TableSetupColumn(playersText, ImGuiTableColumnFlags_WidthStretch, 0.f);
				ImGui::TableSetupColumn(modelText, ImGuiTableColumnFlags_WidthFixed, colModel);
			}
			else
			{
				ImGui::TableSetupColumn(teamsText, ImGuiTableColumnFlags_WidthStretch, 0.f);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, colModel);
			}

			ImGui::TableSetupColumn(scoreText, ImGuiTableColumnFlags_WidthFixed, colScore);
			ImGui::TableSetupColumn(deathText, ImGuiTableColumnFlags_WidthFixed, colDeath);
			ImGui::TableSetupColumn(latencyText, ImGuiTableColumnFlags_WidthFixed, colPing);
			ImGui::TableSetupColumn(voiceText, ImGuiTableColumnFlags_WidthFixed, colVoice);
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			ImVec2 line_start = ImGui::GetCursorScreenPos();
			line_start.x = lineStartX;
			line_start.y -= 2.0f;
			ImVec2 line_end = ImVec2(lineEndX, line_start.y);
			draw_list->AddLine(line_start, line_end, IM_COL32(100, 100, 100, 255), 1.0f);

			ImGui::Spacing();

			for (int row = 0; row < m_iRows; row++)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				if (m_iIsATeam[row] == TEAM_YES)
				{
					team_info_t* team = &g_TeamInfo[m_iSortedRows[row]];
					int teamColorIdx = team->teamnumber % iNumberOfTeamColors;
					ImU32 teamColor = IM_COL32(iTeamColors[teamColorIdx][0], iTeamColors[teamColorIdx][1], iTeamColors[teamColorIdx][2], 255);

					// TEAMS
					ImGui::PushStyleColor(ImGuiCol_Text, teamColor);
					ImGui::Text("%s - %d %s", team->name, team->players, CHudTextMessage::BufferedLocaliseTextString(team->players == 1 ? "#Player" : "#Player_plural"));
					ImGui::PopStyleColor();

					// SCORE
					ImGui::TableSetColumnIndex(2);
					ImGui::PushStyleColor(ImGuiCol_Text, teamColor);
					ImGui::Text("%d", team->frags);
					ImGui::PopStyleColor();

					// DEATHS
					ImGui::TableSetColumnIndex(3);
					ImGui::PushStyleColor(ImGuiCol_Text, teamColor);
					ImGui::Text("%d", team->deaths);
					ImGui::PopStyleColor();

					// PING
					ImGui::TableSetColumnIndex(4);
					ImGui::PushStyleColor(ImGuiCol_Text, teamColor);
					ImGui::Text("%d", team->ping);
					ImGui::PopStyleColor();

					// LINE
					ImVec2 team_line_start = ImGui::GetCursorScreenPos();
					team_line_start.x = lineStartX;
					team_line_start.y -= 2.0f;
					ImVec2 team_line_end = ImVec2(lineEndX, team_line_start.y);
					draw_list->AddLine(team_line_start, team_line_end, teamColor, 1.0f);
				}
				else if (m_iIsATeam[row] == TEAM_SPECTATORS)
				{
					// TEXT
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 160, 0, 255));
					ImGui::Text("%s", CHudTextMessage::BufferedLocaliseTextString("#Spectators"));
					ImGui::PopStyleColor();

					// LINE
					ImVec2 spec_line_start = ImGui::GetCursorScreenPos();
					spec_line_start.x = lineStartX;
					spec_line_start.y -= 2.0f;
					ImVec2 spec_line_end = ImVec2(lineEndX, spec_line_start.y);
					draw_list->AddLine(spec_line_start, spec_line_end, IM_COL32(255, 160, 0, 255), 1.0f);
				}
				else if (m_iIsATeam[row] == TEAM_BLANK)
				{
					ImGui::Text(" ");
				}
				else
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);

					int iPlayerIndex = m_iSortedRows[row]; 
					hud_player_info_t* pl = &g_PlayerInfoList[iPlayerIndex];
					extra_player_info_t* ex = &g_PlayerExtraInfo[iPlayerIndex];

					int teamColorIdx = ex->teamnumber % iNumberOfTeamColors;
					ImU32 playerColor = IM_COL32(iTeamColors[teamColorIdx][0], iTeamColors[teamColorIdx][1], iTeamColors[teamColorIdx][2], 255);

					float textH = ImGui::GetTextLineHeight();

					// PLAYER BG
					ImVec2 row_min = ImGui::GetCursorScreenPos();
					row_min.x = lineStartX;
					ImVec2 row_max = ImVec2(lineEndX, row_min.y + ImGui::GetTextLineHeightWithSpacing());

					if (pl->thisplayer)
					{
						draw_list->AddRectFilled(row_min, row_max, IM_COL32(0, 0, 255, 70), 4.0f);
					}
					else if (m_iSortedRows[row] == m_iLastKilledBy && m_fLastKillTime && m_fLastKillTime > gHUD.m_flTime)
					{
						draw_list->AddRectFilled(row_min, row_max, IM_COL32(255, 0, 0, (int)(70.0f * (m_fLastKillTime - gHUD.m_flTime) / 10.0f)), 4.0f);
					}

					// FLAG
					if (blue_flag_player_index == m_iSortedRows[row] || red_flag_player_index == m_iSortedRows[row])
					{
						float sprW = (float)(m_IconFlagScore.rc.right - m_IconFlagScore.rc.left);
						float sprH = (float)(m_IconFlagScore.rc.bottom - m_IconFlagScore.rc.top);

						float iconH = textH;
						float iconW = (sprH > 0) ? iconH * (sprW / sprH) : iconH;

						int r, g, b;

						if (blue_flag_player_index == m_iSortedRows[row])
						{
							r = 80;  g = 160; b = 255;
						}
						else
						{
							r = 255; g = 80;  b = 80;
						}

						m_ImguiUtils.ImGuiSpriteIcon(m_IconFlagScore.spr, m_IconFlagScore.rc, win_pos.x - iconW - 6.0f, row_min.y, iconW, iconH, textH, r, g, b, 255);
					}

					// NAME
					ImGui::PushStyleColor(ImGuiCol_Text, playerColor);
					m_ImguiUtils.TextWithColorCodes(pl->name);
					ImGui::PopStyleColor();

					// MODEL
					if (!gHUD.m_Teamplay)
					{
						ImGui::TableSetColumnIndex(1);
						m_ImguiUtils.DrawModelName(pl->topcolor, pl->bottomcolor, pl->model);
					}

					// SCORE
					ImGui::TableSetColumnIndex(2);
					ImGui::PushStyleColor(ImGuiCol_Text, playerColor);
					ImGui::Text("%d", ex->frags);
					ImGui::PopStyleColor();

					// DEATHS
					ImGui::TableSetColumnIndex(3);
					ImGui::PushStyleColor(ImGuiCol_Text, playerColor);
					ImGui::Text("%d", ex->deaths);
					ImGui::PopStyleColor();

					// PING
					ImGui::TableSetColumnIndex(4);
					ImGui::PushStyleColor(ImGuiCol_Text, playerColor);
					ImGui::Text("%d", pl->ping);
					ImGui::PopStyleColor();

					// VOICE	
					ImGui::TableSetColumnIndex(5);
					if (pl->name && pl->name[0]) 
					{
						char string[256];

						CVoiceStatus* pVoiceMgr = GetClientVoiceMgr();

						bool isMuted = pVoiceMgr->IsPlayerBlocked(iPlayerIndex);
						bool isSpeaking = pVoiceMgr->IsPlayerSpeaking(iPlayerIndex);

						if (pl->thisplayer) 
							isMuted = false;
					
						double frameDuration = 0.5; // seconds per frame
						int currentFrame = (int)(ImGui::GetTime() / frameDuration) % 3;

						ImGuiImage* pCurrentImg = nullptr;
						ImVec4 tint = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

						if (isMuted)
						{
							pCurrentImg = &m_pScoreboardVoiceBanned;
							tint = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
						}
						else if (isSpeaking)
						{
							switch (currentFrame)
							{
							case 0:
								pCurrentImg = &m_pscoreboardVoiceSpeaking2;    
								break;
							case 1:
								pCurrentImg = &m_pscoreboardVoiceSpeaking3;
								break;
							case 2:
								pCurrentImg = &m_pscoreboardVoiceSpeaking4; 
								break;
							}
							tint = ImVec4(1.0f, 0.66f, 0.0f, 1.0f);
						}
						else
						{
							pCurrentImg = &m_pscoreboardVoiceSpeaking;
						}

						if (pCurrentImg)
						{
							float iconSize = ImGui::GetTextLineHeight();
							
							ImGui::PushID(iPlayerIndex);

							if (m_bMouseMode && !pl->thisplayer)
							{
								if (ImGui::ImageButton("##vbtn", pCurrentImg->texture, ImVec2(iconSize, iconSize), ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), tint))
								{
									if (isMuted)
									{
										char string1[1024];

										// remove mute
										GetClientVoiceMgr()->SetPlayerBlockedState(iPlayerIndex, false);

										sprintf(string1, CHudTextMessage::BufferedLocaliseTextString("#Unmuted"), pl->name);
										sprintf(string, "%c** %s\n", HUD_PRINTTALK, string1);

										gHUD.m_TextMessage.MsgFunc_TextMsg(NULL, strlen(string) + 1, string);
									}
									else
									{
										char string1[1024];
										char string2[1024];

										// mute the player
										GetClientVoiceMgr()->SetPlayerBlockedState(iPlayerIndex, true);

										sprintf(string1, CHudTextMessage::BufferedLocaliseTextString("#Muted"), pl->name);
										sprintf(string2, "%s", CHudTextMessage::BufferedLocaliseTextString("#No_longer_hear_that_player"));
										sprintf(string, "%c** %s %s\n", HUD_PRINTTALK, string1, string2);

										gHUD.m_TextMessage.MsgFunc_TextMsg(NULL, strlen(string) + 1, string);
									}

								}
							}
							else
							{
								ImGui::Image(pCurrentImg->texture, ImVec2(iconSize, iconSize), ImVec2(0,0), ImVec2(1,1), tint, ImVec4(0,0,0,0));
							}
							
							ImGui::PopID();
						}
					}
				}
			}
			ImGui::EndTable();
		}

		ImGui::End();
	}

	ImGui::PopStyleColor(11);
	ImGui::PopStyleVar(7);
}

void CImGuiScoreboard::DeathMsg( int killer, int victim )
{
	if ( victim == m_iPlayerNum || killer == 0 )
	{
		m_iLastKilledBy = killer ? killer : m_iPlayerNum;
		m_fLastKillTime = gHUD.m_flTime + 10;

		if ( killer == m_iPlayerNum )
			m_iLastKilledBy = m_iPlayerNum;
	}
}

void CImGuiScoreboard::Draw()
{
	if (!m_ShowScore)
		return;

	int i;

	RebuildTeams();

	m_iRows = 0;
	g_ImGuiViewport.GetAllPlayersInfo();

	// Clear out sorts
	for (i = 0; i < NUM_ROWS; i++)
	{
		m_iSortedRows[i] = 0;
		m_iIsATeam[i] = TEAM_NO;
	}
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		m_bHasBeenSorted[i] = false;
	}

	if( !gHUD.m_Teamplay )
		SortPlayers( 0, NULL );
	else
		SortTeams();

	DrawScoreboard();
}

bool CImGuiScoreboard::Active()
{
	return m_ShowScore;
}

bool CImGuiScoreboard::CursorRequired()
{
	if (m_ShowScore && m_bMouseMode)
		return true;

	return false;
}

bool CImGuiScoreboard::HandleKey(bool keyDown, int keyNumber, const char* bindName)
{
	if (!m_ShowScore)
		return true;

	if (keyDown && keyNumber == K_MOUSE2)
	{
		if (!m_bMouseMode)
		{
			m_bMouseMode = true; 
			gEngfuncs.pfnSetMousePos(g_ImGuiViewport.scrWidth() / 2, g_ImGuiViewport.scrHeight() / 2);
		}
		
		return false; 
	}

	if (m_bMouseMode)
	{
		return false;
	}

	return true;
}
