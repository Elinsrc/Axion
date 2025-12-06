#include "imgui.h"
#include "keydefs.h"
#include "imgui_backend.h"
#include "imgui_viewport.h"
#include "imgui_ScorePanel.h"

extern int blue_flag_player_index;
extern int red_flag_player_index;

CImGuiScoreboard m_iScoreboard;

bool CImGuiScoreboard::m_ShowWindow = false;

void CImGuiScoreboard::Initialize()
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
}

void CImGuiScoreboard::Terminate()
{
}
void CImGuiScoreboard::Think()
{
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

	float padding_y = 20.f;
	float sb_width = 700.f;
	float sb_height = g_ImGuiViewport.scrHeight() - 2 * padding_y;
	float sb_x = (g_ImGuiViewport.scrWidth() - sb_width) / 2;
	float sb_y = padding_y;

	ImGui::SetNextWindowPos(ImVec2(sb_x, sb_y));
	ImGui::SetNextWindowSize(ImVec2(sb_width, sb_height));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.02f, 0.02f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0,0,0,0));
	ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.05f,0.05f,0.05f,0.5f));
	ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.08f,0.08f,0.08f,0.5f));
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,160,0,255));

	if (ImGui::Begin("##Scoreboard", &m_ShowWindow,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoInputs))
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 win_pos = ImGui::GetWindowPos();
		ImVec2 win_size = ImGui::GetWindowSize();

		float line_y = ImGui::GetCursorPosY();

		m_ImguiUtils.TextWithColorCodes(g_ImGuiViewport.m_szServerName);

		char player_count[256];
		sprintf(player_count, "%d/%d", get_player_count(), gEngfuncs.GetMaxClients());
		float text_width = ImGui::CalcTextSize(player_count).x;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - text_width - ImGui::GetStyle().WindowPadding.x);
		ImGui::SetCursorPosY(line_y);
		ImGui::Text("%s", player_count);

		ImGui::Spacing();
		char map_name[64]; get_map_name(map_name, ARRAYSIZE(map_name));
		ImGui::Text("%s", map_name);

		if (ImGui::BeginTable("ScoreboardTableInvisible", 5, ImGuiTableFlags_SizingFixedFit, ImVec2(0, 480)))
		{
			if (!gHUD.m_Teamplay)
			{
				ImGui::TableSetupColumn(CHudTextMessage::BufferedLocaliseTextString("#PLAYERS"), ImGuiTableColumnFlags_WidthStretch, 0.f);
				ImGui::TableSetupColumn("MODEL", ImGuiTableColumnFlags_WidthFixed, 80.f);
			}
			else
			{
				ImGui::TableSetupColumn(CHudTextMessage::BufferedLocaliseTextString("#TEAMS"), ImGuiTableColumnFlags_WidthStretch, 0.f);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.f);
			}
			ImGui::TableSetupColumn(CHudTextMessage::BufferedLocaliseTextString("#SCORE"), ImGuiTableColumnFlags_WidthFixed, 80.f);
			ImGui::TableSetupColumn(CHudTextMessage::BufferedLocaliseTextString("#DEATHS"), ImGuiTableColumnFlags_WidthFixed, 80.f);
			ImGui::TableSetupColumn(CHudTextMessage::BufferedLocaliseTextString("#LATENCY"), ImGuiTableColumnFlags_WidthFixed, 80.f);
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			ImVec2 line_start = ImGui::GetCursorScreenPos();
			line_start.x = win_pos.x + ImGui::GetStyle().WindowPadding.x;
			line_start.y -= 2.0f;
			ImVec2 line_end = ImVec2(win_pos.x + win_size.x - ImGui::GetStyle().WindowPadding.x, line_start.y );
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
					ImVec4 teamColor = ImVec4(
						iTeamColors[teamColorIdx][0] / 255.f,
						iTeamColors[teamColorIdx][1] / 255.f,
						iTeamColors[teamColorIdx][2] / 255.f,
						1.0f
					);

					// TEAMS
					ImGui::PushStyleColor(ImGuiCol_Text, teamColor);
					ImGui::Text("%s (%d)", team->name, team->players);
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
					ImVec2 line_start = ImGui::GetCursorScreenPos();
					line_start.x = win_pos.x + ImGui::GetStyle().WindowPadding.x;
					line_start.y -= 2.0f;
					ImVec2 line_end = ImVec2(win_pos.x + win_size.x - ImGui::GetStyle().WindowPadding.x, line_start.y);
					ImU32 lineColor = IM_COL32((int)(teamColor.x * 255), (int)(teamColor.y * 255), (int)(teamColor.z * 255), 255);
					draw_list->AddLine(line_start, line_end, lineColor, 1.0f);
				}
				else if (m_iIsATeam[row] == TEAM_SPECTATORS)
				{
					// TEXT
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.63f, 0.0f, 1.0f));
					ImGui::Text("Spectators");
					ImGui::PopStyleColor();

					// LINE
					ImVec2 line_start = ImGui::GetCursorScreenPos();
					line_start.x = win_pos.x + ImGui::GetStyle().WindowPadding.x;
					line_start.y -= 2.0f;
					ImVec2 line_end = ImVec2(win_pos.x + win_size.x - ImGui::GetStyle().WindowPadding.x, line_start.y);
					ImU32 lineColor = IM_COL32(255, 160, 0, 255);
					draw_list->AddLine(line_start, line_end, lineColor, 1.0f);
				}

				else if (m_iIsATeam[row] == TEAM_BLANK)
				{
					ImGui::Text(" ");
				}
				else
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);

					hud_player_info_t* pl = &g_PlayerInfoList[m_iSortedRows[row]];
					extra_player_info_t* ex = &g_PlayerExtraInfo[m_iSortedRows[row]];

					int teamColorIdx = ex->teamnumber % iNumberOfTeamColors;
					ImVec4 playerColor = ImVec4(
						iTeamColors[teamColorIdx][0] / 255.f,
						iTeamColors[teamColorIdx][1] / 255.f,
						iTeamColors[teamColorIdx][2] / 255.f,
						1.0f
					);

					float textH = ImGui::GetTextLineHeight();

					// PLAYER BG
					ImVec2 row_min = ImGui::GetCursorScreenPos();
					row_min.x = win_pos.x + ImGui::GetStyle().WindowPadding.x;
					ImVec2 row_max = ImVec2(win_pos.x + win_size.x - ImGui::GetStyle().WindowPadding.x, row_min.y + ImGui::GetTextLineHeightWithSpacing());

					if (pl->thisplayer)
					{
						draw_list->AddRectFilled(row_min, row_max, IM_COL32(0, 0, 255, 70), 4.0f);
					}
					else if ( m_iSortedRows[row] == m_iLastKilledBy && m_fLastKillTime && m_fLastKillTime > gHUD.m_flTime )
					{
						draw_list->AddRectFilled(row_min, row_max,IM_COL32(255, 0, 0,(int)(70.0f * (m_fLastKillTime - gHUD.m_flTime) / 10.0f)),4.0f);
					}

					// FLAG
					if (blue_flag_player_index == m_iSortedRows[row] || red_flag_player_index == m_iSortedRows[row])
					{
						float sprW = (float)(m_IconFlagScore.rc.right  - m_IconFlagScore.rc.left);
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
				}

			}
			ImGui::EndTable();
		}

		ImGui::End();
	}

	ImGui::PopStyleColor(5);
	ImGui::PopStyleVar(6);
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
	if (!m_ShowWindow)
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
	return m_ShowWindow;
}
bool CImGuiScoreboard::CursorRequired()
{
	return false;

}
bool CImGuiScoreboard::HandleKey(bool keyDown, int keyNumber, const char* bindName)
{
	return true;
}
