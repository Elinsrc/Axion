#include "build.h"
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

#if !XASH_MOBILE_PLATFORM && !XASH_64BIT

#include "noavatar.h"
#include "avatar_cache.h"

ImGuiImage m_pNoAvatar;

cvar_t* hud_scoreboard_showavatars;

#endif

void CImGuiScoreboard::Initialize()
{
#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
	hud_scoreboard_showavatars = CVAR_CREATE("hud_scoreboard_showavatars", "1", FCVAR_ARCHIVE);
	g_AvatarCache.Initialize();
#endif	
	m_bMouseMode = false;

	InitHUDData();
}

void CImGuiScoreboard::InitHUDData()
{
	m_iLastKilledBy = 0;
	m_fLastKillTime = 0;
	m_iPlayerNum = 0;
	m_iNumTeams = 0;
	m_iSelectedPlayer = 0;
	m_bShowPlayerMenu = false;
	
	memset(g_PlayerIsBot, 0, sizeof(g_PlayerIsBot));
	
#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
	memset(g_PlayerSteamId, 0, sizeof(g_PlayerSteamId));
	memset(g_PlayerSteamID64, 0, sizeof(g_PlayerSteamID64));
#endif
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

#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
	m_pNoAvatar = m_ImguiUtils.LoadImageFromMemory(noavatar, noavatar_len);
#endif
}

void CImGuiScoreboard::Terminate()
{
	m_ImguiUtils.FreeImage(m_pScoreboardVoiceBanned);
	m_ImguiUtils.FreeImage(m_pscoreboardVoiceSpeaking);
	m_ImguiUtils.FreeImage(m_pscoreboardVoiceSpeaking2);
	m_ImguiUtils.FreeImage(m_pscoreboardVoiceSpeaking3);
	m_ImguiUtils.FreeImage(m_pscoreboardVoiceSpeaking4);

#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
	m_ImguiUtils.FreeImage(m_pNoAvatar);
	g_AvatarCache.Shutdown();
#endif
}

void CImGuiScoreboard::Think()
{
	if (!m_ShowScore)
	{
		m_bMouseMode = false;
		m_bShowPlayerMenu = false;
		m_iSelectedPlayer = 0;
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
	if (!m_ShowScore)
		return;

	g_ImGuiViewport.GetAllPlayersInfo();

	bool bShowAvatars = false;
#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
	if (hud_scoreboard_showavatars->value > 0)
		bShowAvatars = true;
#endif

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

	float avatarSize = 32.0f;
	float textHeight = ImGui::GetTextLineHeight();
	
	float rowHeight = bShowAvatars ? (avatarSize + 8.0f) : (textHeight + 6.0f);
	float textVertOffset = (rowHeight - textHeight) * 0.5f;

	float colModel = ImGui::CalcTextSize(modelText).x + 24.f;
	float colScore = ImGui::CalcTextSize(scoreText).x + 24.f;
	float colDeath = ImGui::CalcTextSize(deathText).x + 24.f;
	float colPing = ImGui::CalcTextSize(latencyText).x + 24.f;
	float colVoice = ImGui::CalcTextSize(voiceText).x + 24.f;

	float scrollbarWidth = 12.0f;

	float minNameWidth = 200.f;
	float fixedColumnsWidth = colModel + colScore + colDeath + colPing + colVoice;
	float minWindowWidth = fixedColumnsWidth + minNameWidth + 40.f + scrollbarWidth;

	float padding_y = scrHeight * 0.05f;
	float sb_width = scrWidth * 0.6f;

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
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;

	if (!m_bMouseMode) 
		flags |= ImGuiWindowFlags_NoInputs;
	else 
		flags |= ImGuiWindowFlags_NoNav;

	// FLAG FOR DEFERRED POPUP OPENING
	bool bNeedOpenPopup = false;

	if (ImGui::Begin("##Scoreboard", &m_ShowScore, flags))
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 win_pos = ImGui::GetWindowPos();
		ImVec2 win_size = ImGui::GetWindowSize();

		float padding = ImGui::GetStyle().WindowPadding.x;
		float lineStartX = win_pos.x + padding;
		float lineEndX = win_pos.x + win_size.x - padding;

		float line_y = ImGui::GetCursorPosY();

		// SERVER NAME
		m_ImguiUtils.TextWithColorCodes(g_ImGuiViewport.m_szServerName);

		// PLAYER COUNT
		char player_count[256];
		sprintf(player_count, "%d/%d", get_player_count(), gEngfuncs.GetMaxClients());
		float text_width = ImGui::CalcTextSize(player_count).x;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - text_width - padding);
		ImGui::SetCursorPosY(line_y);
		ImGui::Text("%s", player_count);
		
		ImGui::Spacing();

		// MAP NAME
		char map_name[64];
		get_map_name(map_name, ARRAYSIZE(map_name));
		ImGui::Text("%s", map_name);
		ImGui::Spacing();

		float contentAvail = ImGui::GetContentRegionAvail().x;
		float commonTableWidth = contentAvail - scrollbarWidth;

		// HEADER TABLE
		if (ImGui::BeginTable("##SBHeader", 6, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings, ImVec2(commonTableWidth, 0)))
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
			ImGui::EndTable();
		}

		// HEADER SEPARATOR LINE
		{
			float y = ImGui::GetCursorScreenPos().y - 2.0f;
			draw_list->AddLine(ImVec2(lineStartX, y), ImVec2(lineEndX, y), IM_COL32(100, 100, 100, 255), 1.0f);
		}

		float scrollHeight = sb_height - ImGui::GetCursorPosY() - 10.f;

		// SCROLLABLE BODY STYLES
		ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(0, 0, 0, 80));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(255, 160, 0, 100));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(255, 160, 0, 180));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(255, 160, 0, 255));
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, scrollbarWidth);
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 4.0f);

		ImGuiWindowFlags childFlags = ImGuiWindowFlags_None;
		if (!m_bMouseMode) 
			childFlags |= ImGuiWindowFlags_NoScrollWithMouse;

		// SCROLLABLE BODY
		if (ImGui::BeginChild("##SBBody", ImVec2(0, scrollHeight), false, childFlags))
		{
			ImVec2 childMin = ImGui::GetWindowPos();
			ImVec2 childMax = ImVec2(childMin.x + ImGui::GetWindowSize().x, childMin.y + ImGui::GetWindowSize().y);
			float contentMaxX = childMax.x - scrollbarWidth;

			// PLAYERS TABLE
			if (ImGui::BeginTable("##SBPlayers", 6, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings, ImVec2(commonTableWidth, 0)))
			{
				if (!gHUD.m_Teamplay) 
				{
					ImGui::TableSetupColumn("##p1", ImGuiTableColumnFlags_WidthStretch, 0.f);
					ImGui::TableSetupColumn("##p2", ImGuiTableColumnFlags_WidthFixed, colModel);
				} 
				else 
				{
					ImGui::TableSetupColumn("##p1", ImGuiTableColumnFlags_WidthStretch, 0.f);
					ImGui::TableSetupColumn("##p2", ImGuiTableColumnFlags_WidthFixed, colModel);
				}
				ImGui::TableSetupColumn("##p3", ImGuiTableColumnFlags_WidthFixed, colScore);
				ImGui::TableSetupColumn("##p4", ImGuiTableColumnFlags_WidthFixed, colDeath);
				ImGui::TableSetupColumn("##p5", ImGuiTableColumnFlags_WidthFixed, colPing);
				ImGui::TableSetupColumn("##p6", ImGuiTableColumnFlags_WidthFixed, colVoice);

				for (int row = 0; row < m_iRows; row++)
				{
					if (m_iIsATeam[row] == TEAM_YES)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);

						team_info_t* team = &g_TeamInfo[m_iSortedRows[row]];
						int teamColorIdx = team->teamnumber % iNumberOfTeamColors;
						ImU32 teamColor = IM_COL32(iTeamColors[teamColorIdx][0], iTeamColors[teamColorIdx][1], iTeamColors[teamColorIdx][2], 255);

						float teamRowH = ImGui::GetTextLineHeightWithSpacing();
						float teamOffY = (teamRowH - textHeight) * 0.5f;

						// TEAM NAME
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + teamOffY);
						ImGui::PushStyleColor(ImGuiCol_Text, teamColor);
						ImGui::Text("%s - %d %s", team->name, team->players, CHudTextMessage::BufferedLocaliseTextString(team->players == 1 ? "#Player" : "#Player_plural"));
						ImGui::PopStyleColor();

						// TEAM SCORE
						ImGui::TableSetColumnIndex(2); 
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + teamOffY);
						ImGui::PushStyleColor(ImGuiCol_Text, teamColor); 
						ImGui::Text("%d", team->frags); 
						ImGui::PopStyleColor();

						// TEAM DEATHS
						ImGui::TableSetColumnIndex(3); 
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + teamOffY);
						ImGui::PushStyleColor(ImGuiCol_Text, teamColor); 
						ImGui::Text("%d", team->deaths); 
						ImGui::PopStyleColor();
						
						// TEAM PING
						ImGui::TableSetColumnIndex(4); 
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + teamOffY);
						ImGui::PushStyleColor(ImGuiCol_Text, teamColor); 
						ImGui::Text("%d", team->ping); 
						ImGui::PopStyleColor();

						// TEAM SEPARATOR LINE
						ImGui::TableSetColumnIndex(0);
						float y = ImGui::GetCursorScreenPos().y + teamRowH - 2.0f;
						draw_list->PushClipRect(childMin, childMax, true);
						draw_list->AddLine(ImVec2(lineStartX, y), ImVec2(lineEndX, y), teamColor, 1.0f);
						draw_list->PopClipRect();
					}
					else if (m_iIsATeam[row] == TEAM_SPECTATORS)
					{
						ImGui::TableNextRow(); 
						ImGui::TableSetColumnIndex(0);
						float teamOffY = (ImGui::GetTextLineHeightWithSpacing() - textHeight) * 0.5f;
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + teamOffY);

						// TEXT
						ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 160, 0, 255));
						ImGui::Text("%s", CHudTextMessage::BufferedLocaliseTextString("#Spectators"));
						ImGui::PopStyleColor();
						
						// SEPARATOR LINE
						float y = ImGui::GetCursorScreenPos().y + textHeight + 2.0f;
						draw_list->PushClipRect(childMin, childMax, true);
						draw_list->AddLine(ImVec2(lineStartX, y), ImVec2(lineEndX, y), IM_COL32(255, 160, 0, 255), 1.0f);
						draw_list->PopClipRect();
					}
					else if (m_iIsATeam[row] == TEAM_BLANK) 
					{
						ImGui::TableNextRow(); 
						ImGui::TableSetColumnIndex(0); 
						ImGui::Text(" ");
					}
					else
					{
						ImGui::TableNextRow(0, rowHeight);
						ImGui::TableSetColumnIndex(0);

						int iPlayerIndex = m_iSortedRows[row];
						hud_player_info_t* pl = &g_PlayerInfoList[iPlayerIndex];
						extra_player_info_t* ex = &g_PlayerExtraInfo[iPlayerIndex];

						int teamColorIdx = ex->teamnumber % iNumberOfTeamColors;
						ImU32 playerColor = IM_COL32(iTeamColors[teamColorIdx][0], iTeamColors[teamColorIdx][1], iTeamColors[teamColorIdx][2], 255);

						float rowYScreen = ImGui::GetCursorScreenPos().y;

						m_CustomUtils.UpdatePlayerInfo(iPlayerIndex);
						
						// PLAYER BACKGROUND
						ImVec2 row_min = ImVec2(lineStartX, rowYScreen);
						ImVec2 row_max = ImVec2(contentMaxX, rowYScreen + rowHeight);

						ImU32 bgColor = 0;
						if (pl->thisplayer) 
							bgColor = IM_COL32(0, 0, 255, 70);
						else if (m_iSortedRows[row] == m_iLastKilledBy && m_fLastKillTime && m_fLastKillTime > gHUD.m_flTime)
							bgColor = IM_COL32(255, 0, 0, (int)(70.0f * (m_fLastKillTime - gHUD.m_flTime) / 10.0f));

						bool isHovered = false;
						if (m_bMouseMode) 
						{
							ImVec2 mPos = ImGui::GetMousePos();
							if (mPos.x >= row_min.x && mPos.x <= row_max.x && mPos.y >= row_min.y && mPos.y <= row_max.y)
								isHovered = true;
						}

						// HOVER HIGHLIGHT (only when popup is NOT open)
						if (isHovered && pl && !pl->thisplayer && !m_bShowPlayerMenu) 
							bgColor = IM_COL32(255, 255, 255, 30);

						// SELECTED PLAYER BG HIGHLIGHT (no border/outline)
						if (m_bShowPlayerMenu && m_iSelectedPlayer == iPlayerIndex)
						{
							bgColor = IM_COL32(255, 255, 255, 30);
						}

						// DRAW PLAYER BACKGROUND
						if (bgColor != 0) 
						{
							draw_list->PushClipRect(childMin, childMax, true);
							draw_list->AddRectFilled(row_min, row_max, bgColor);
							draw_list->PopClipRect();
						}

						// CLICK TO OPEN CONTEXT MENU (deferred)
						if (m_bMouseMode && isHovered && ImGui::IsMouseClicked(0) && pl && !pl->thisplayer && pl->name && pl->name[0] && !m_bShowPlayerMenu)
						{
							m_iSelectedPlayer = iPlayerIndex;
							m_bShowPlayerMenu = true;
							bNeedOpenPopup = true;
						}

						ImGui::PushID(iPlayerIndex);

						float startContentX = win_pos.x + padding;

						// AVATAR
						if (bShowAvatars)
						{
							float avatarY = rowYScreen + (rowHeight - avatarSize) * 0.5f;
							
							ImGui::SetCursorScreenPos(ImVec2(startContentX + 4.0f, avatarY));
							ImVec2 p = ImGui::GetCursorScreenPos();

							// AVATAR BORDER
							draw_list->PushClipRect(childMin, childMax, true);
							draw_list->AddRect(ImVec2(p.x - 1, p.y - 1), ImVec2(p.x + avatarSize + 1, p.y + avatarSize + 1), playerColor, 2.0f, 0, 1.5f);
							draw_list->PopClipRect();

#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
							// AVATAR IMAGE
							if (g_PlayerIsBot[iPlayerIndex])
							{
								ImGui::Image(m_pNoAvatar.texture, ImVec2(avatarSize, avatarSize));
							}
							else
							{
								ImTextureID avatarTex = g_AvatarCache.GetAvatar(iPlayerIndex);
								if (avatarTex) 
									ImGui::Image(avatarTex, ImVec2(avatarSize, avatarSize));
								else 
									ImGui::Image(m_pNoAvatar.texture, ImVec2(avatarSize, avatarSize));
							}
#endif
							ImGui::SameLine();
							
							startContentX += (avatarSize + 20.0f);
						}

						float nameY = rowYScreen + textVertOffset;
						ImGui::SetCursorScreenPos(ImVec2(startContentX, nameY));
						
						// PLAYER NAME
						ImGui::PushStyleColor(ImGuiCol_Text, playerColor);
						m_ImguiUtils.TextWithColorCodes(pl->name);
						ImGui::PopStyleColor();

						// CTF FLAG ICON
						if (blue_flag_player_index == iPlayerIndex || red_flag_player_index == iPlayerIndex) 
						{
							float sprW = (float)(m_IconFlagScore.rc.right - m_IconFlagScore.rc.left);
							float sprH = (float)(m_IconFlagScore.rc.bottom - m_IconFlagScore.rc.top);
							float iconH = textHeight; 
							float iconW = (sprH > 0) ? iconH * (sprW / sprH) : iconH;
							int r = (blue_flag_player_index == iPlayerIndex) ? 80 : 255;
							int g = (blue_flag_player_index == iPlayerIndex) ? 160 : 80;
							int b = (blue_flag_player_index == iPlayerIndex) ? 255 : 80;
							float col0Width = ImGui::GetColumnWidth(0);
							m_ImguiUtils.ImGuiSpriteIcon(m_IconFlagScore.spr, m_IconFlagScore.rc, win_pos.x + padding + col0Width - iconW - 5.0f, nameY, iconW, iconH, textHeight, r, g, b, 255);
						}

						// MODEL (non-teamplay only)
						if (!gHUD.m_Teamplay) {
							ImGui::TableSetColumnIndex(1); 
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textVertOffset);
							m_ImguiUtils.DrawModelName(pl->topcolor, pl->bottomcolor, pl->model);
						}

						// PLAYER SCORE
						ImGui::TableSetColumnIndex(2); 
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textVertOffset);
						ImGui::PushStyleColor(ImGuiCol_Text, playerColor); 
						ImGui::Text("%d", ex->frags); 
						ImGui::PopStyleColor();

						// PLAYER DEATHS
						ImGui::TableSetColumnIndex(3); 
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textVertOffset);
						ImGui::PushStyleColor(ImGuiCol_Text, playerColor); 
						ImGui::Text("%d", ex->deaths); 
						ImGui::PopStyleColor();

						// PLAYER PING
						ImGui::TableSetColumnIndex(4); 
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textVertOffset);
						ImGui::PushStyleColor(ImGuiCol_Text, playerColor); 
						if (g_PlayerIsBot[iPlayerIndex])
							ImGui::Text("BOT");
						else
							ImGui::Text("%d", pl->ping);
						ImGui::PopStyleColor();

						// VOICE ICON
						ImGui::TableSetColumnIndex(5);
						if (pl && pl->name && pl->name[0]) 
						{
							float iconSize = textHeight; 
							float iconOffset = (rowHeight - iconSize) * 0.5f;
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + iconOffset);
							double frameDuration = 0.5; 
							int currentFrame = (int)(ImGui::GetTime() / frameDuration) % 3;
							
							ImGuiImage* pImg = nullptr; 
							ImVec4 tint = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
							
							// VOICE BLOCKED
							if (GetClientVoiceMgr()->IsPlayerBlocked(iPlayerIndex)) 
							{ 
								pImg = &m_pScoreboardVoiceBanned; 
								tint = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); 
							}
							// VOICE SPEAKING (animated)
							else if (GetClientVoiceMgr()->IsPlayerSpeaking(iPlayerIndex)) 
							{
								if (currentFrame == 0) 
									pImg = &m_pscoreboardVoiceSpeaking2; 
								else if (currentFrame == 1) 
									pImg = &m_pscoreboardVoiceSpeaking3; 
								else 
									pImg = &m_pscoreboardVoiceSpeaking4;
								tint = ImVec4(1.0f, 0.66f, 0.0f, 1.0f);
							} 
							// VOICE IDLE
							else 
							{ 
								pImg = &m_pscoreboardVoiceSpeaking; 
							}
							ImGui::Image(pImg->texture, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), tint, ImVec4(0, 0, 0, 0));
						}

						ImGui::PopID();
					}
				}

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(5);

		// PLAYER CONTEXT MENU (at main window level)
		if (bNeedOpenPopup)
		{
			ImGui::OpenPopup("##PlayerContextMenu");
		}

		if (m_bShowPlayerMenu)
		{
			// CONTEXT MENU STYLES
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
			ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 2.0f);
			
			// NO BORDER FOR POPUP
			ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f); 

			ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(20, 20, 20, 200));
			ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0)); // Transparent border
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 160, 0, 255));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(255, 255, 255, 80));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(255, 255, 255, 120));

			if (ImGui::BeginPopup("##PlayerContextMenu"))
			{
				hud_player_info_t* selPl = &g_PlayerInfoList[m_iSelectedPlayer];
				extra_player_info_t* selEx = &g_PlayerExtraInfo[m_iSelectedPlayer];
				
				int selTeamColorIdx = selEx->teamnumber % iNumberOfTeamColors;
				ImU32 selPlayerColor = IM_COL32(iTeamColors[selTeamColorIdx][0], iTeamColors[selTeamColorIdx][1], iTeamColors[selTeamColorIdx][2], 255);

				// CONTEXT MENU HEADER - PLAYER NAME
				ImGui::PushStyleColor(ImGuiCol_Text, selPlayerColor);
				m_ImguiUtils.TextWithColorCodes(selPl->name);
				ImGui::PopStyleColor();
				ImGui::Separator();

				// MUTE / UNMUTE OPTION
				bool isMuted = GetClientVoiceMgr()->IsPlayerBlocked(m_iSelectedPlayer);
				const char* muteLabel = isMuted ? "Unmute" : "Mute";
				
				if (ImGui::Selectable(muteLabel))
				{
					char string[256];
					if (isMuted) 
					{
						// UNMUTE PLAYER
						char string1[1024]; 
						GetClientVoiceMgr()->SetPlayerBlockedState(m_iSelectedPlayer, false);
						sprintf(string1, CHudTextMessage::BufferedLocaliseTextString("#Unmuted"), selPl->name);
						sprintf(string, "%c** %s\n", HUD_PRINTTALK, string1);
					} 
					else 
					{
						// MUTE PLAYER
						char string1[1024];
						char string2[1024]; 
						GetClientVoiceMgr()->SetPlayerBlockedState(m_iSelectedPlayer, true);
						sprintf(string1, CHudTextMessage::BufferedLocaliseTextString("#Muted"), selPl->name);
						sprintf(string2, "%s", CHudTextMessage::BufferedLocaliseTextString("#No_longer_hear_that_player"));
						sprintf(string, "%c** %s %s\n", HUD_PRINTTALK, string1, string2);
					}
					gHUD.m_TextMessage.MsgFunc_TextMsg(NULL, strlen(string) + 1, string);
					
					m_bShowPlayerMenu = false;
					m_iSelectedPlayer = 0;
				}

				// STEAM PROFILE OPTION
#if !XASH_MOBILE_PLATFORM && !XASH_64BIT
				if (!g_PlayerIsBot[m_iSelectedPlayer] && g_PlayerSteamID64[m_iSelectedPlayer])
				{
					if (ImGui::Selectable("Steam Profile"))
					{
						// OPEN STEAM OVERLAY TO PLAYER PROFILE
						g_SteamAPI.ActivateGameOverlayToUser("steamid", g_PlayerSteamID64[m_iSelectedPlayer]);
						m_bShowPlayerMenu = false;
						m_iSelectedPlayer = 0;
					}
				}
#endif
				ImGui::EndPopup();
			}
			else
			{
				// CONTEXT MENU WAS CLOSED (clicked outside)
				m_bShowPlayerMenu = false;
				m_iSelectedPlayer = 0;
			}

			ImGui::PopStyleColor(5);
			ImGui::PopStyleVar(4);
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
