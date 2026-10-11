#include "build.h"
#include "imgui.h"
#include "keydefs.h"
#include "imgui_viewport.h"
#include "ui_ScorePanel.h"
#include "voice_status.h"
#include "cl_util.h"
#include "imgui_utils.h"
#include "avatar_cache.h"

extern mobile_engfuncs_t *gMobileEngfuncs;

extern int blue_flag_player_index;
extern int red_flag_player_index;

ImGuiImage m_pScoreboardVoiceBanned;
ImGuiImage m_pscoreboardVoiceSpeaking;
ImGuiImage m_pscoreboardVoiceSpeaking2;
ImGuiImage m_pscoreboardVoiceSpeaking3;
ImGuiImage m_pscoreboardVoiceSpeaking4;

CImGuiScoreboard g_iScoreboard;

bool CImGuiScoreboard::m_ShowScore = false;

cvar_t* hud_scoreboard_showavatars;

void CImGuiScoreboard::Initialize()
{
	hud_scoreboard_showavatars = CVAR_CREATE("hud_scoreboard_showavatars", "1", FCVAR_ARCHIVE);

	gEngfuncs.pfnAddCommand("+sb_selprev", []() { g_iScoreboard.SelectPlayerStep(-1); });
	gEngfuncs.pfnAddCommand("-sb_selprev", []() {});
	gEngfuncs.pfnAddCommand("+sb_selnext", []() { g_iScoreboard.SelectPlayerStep(1); });
	gEngfuncs.pfnAddCommand("-sb_selnext", []() {});
	gEngfuncs.pfnAddCommand("+sb_mute", []() { g_iScoreboard.OnMuteButton(); });
	gEngfuncs.pfnAddCommand("-sb_mute", []() {});

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
	m_bScrollToSelected = false;

	memset(g_PlayerIsBot, 0, sizeof(g_PlayerIsBot));
	memset(g_PlayerSteamID64, 0, sizeof(g_PlayerSteamID64));
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

	if (gMobileEngfuncs)
	{
		gMobileEngfuncs->pfnTouchRemoveButton("sb_up");
		gMobileEngfuncs->pfnTouchRemoveButton("sb_mute");
		gMobileEngfuncs->pfnTouchRemoveButton("sb_down");
	}
}

void CImGuiScoreboard::Think()
{
	if (!m_ShowScore)
	{
		m_bMouseMode = false;
		m_bShowPlayerMenu = false;
		m_iSelectedPlayer = 0;
		m_bScrollToSelected = false;
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

	static float chromeMem = 0.f;
	static float errMem = 0.f;

	if (!isfinite(chromeMem) || chromeMem < 0.f || chromeMem > 4096.f)
		chromeMem = 0.f;

	if (!isfinite(errMem))
		errMem = 0.f;

	g_ImGuiViewport.GetAllPlayersInfo();

	bool bShowAvatars = false;

	if (hud_scoreboard_showavatars->value > 0)
		bShowAvatars = true;

	bool bTeamplay = gHUD.m_Teamplay ? true : false;

	const float cellPadX = 12.f;
	const float cellPadY = 2.f;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(cellPadX, cellPadY));
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

	float ui_scale = CVAR_GET_FLOAT("ui_imgui_scale");
	if (!isfinite(ui_scale) || ui_scale < 1.5f)
	{
		gEngfuncs.Cvar_SetValue("ui_imgui_scale", 1.5f);
		ui_scale = 1.5f;
	}

	float baseAvatarSize = 32.0f;
	float scaledAvatarSize = baseAvatarSize * ui_scale;
	float textHeight = ImGui::GetTextLineHeight();

	float rowHeight = ceilf(bShowAvatars ? (scaledAvatarSize + 6.0f * ui_scale) : (textHeight + 8.0f));
	float teamRowH = ceilf(textHeight + 10.0f);
	float headerRowH = ceilf(textHeight + 2.0f * cellPadY + 2.0f);
	float spacerH = 6.0f;

	auto getTeamColor = [&](int teamnumber) -> ImU32
	{
		int idx = teamnumber % iNumberOfTeamColors;
		if (idx < 0)
			idx += iNumberOfTeamColors;
		return IM_COL32(iTeamColors[idx][0], iTeamColors[idx][1], iTeamColors[idx][2], 255);
	};

	auto cellText = [&](int col, const char* txt, float rowH, bool center)
	{
		ImGui::TableSetColumnIndex(col);

		ImVec2 p = ImGui::GetCursorScreenPos();
		float availW = ImGui::GetContentRegionAvail().x;
		float tw = ImGui::CalcTextSize(txt).x;

		float x = p.x;
		if (center && tw < availW)
			x += (availW - tw) * 0.5f;

		float y = p.y - cellPadY + (rowH - textHeight) * 0.5f;

		ImGui::GetWindowDrawList()->AddText(ImVec2(floorf(x), floorf(y)), ImGui::GetColorU32(ImGuiCol_Text), txt);
	};

	auto cellInt = [&](int col, int val, float rowH)
	{
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", val);
		cellText(col, buf, rowH, true);
	};

	auto cellOffsetY = [&](float h)
	{
		float off = (rowHeight - 2.0f * cellPadY - h) * 0.5f;
		if (off <= 0.f)
			return;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 0));
		ImGui::Dummy(ImVec2(0, off));
		ImGui::PopStyleVar();
	};

	float colModel = bTeamplay ? 0.f : fmaxf(ImGui::CalcTextSize(modelText).x, ImGui::CalcTextSize("0000000000").x) + 24.f * ui_scale;
	float colScore = ImGui::CalcTextSize(scoreText).x + 24.f * ui_scale;
	float colDeath = ImGui::CalcTextSize(deathText).x + 24.f * ui_scale;
	float colPing = ImGui::CalcTextSize(latencyText).x + 24.f * ui_scale;
	float colVoice = ImGui::CalcTextSize(voiceText).x + 24.f * ui_scale;

	float scrollbarWidth = 12.0f * ui_scale;

	float minNameWidth = 200.f * ui_scale;
	float fixedColumnsWidth = colModel + colScore + colDeath + colPing + colVoice;
	float minWindowWidth = fixedColumnsWidth + minNameWidth + 40.f * ui_scale + scrollbarWidth;

	float padding_y = scrHeight * 0.05f;
	float sb_width = fminf(scrWidth * 0.55f, 820.f * ui_scale);

	if (sb_width < minWindowWidth)
		sb_width = minWindowWidth;

	if (sb_width > scrWidth - 20.f)
		sb_width = scrWidth - 20.f;

	int nPlayerRows = 0, nHeaderRows = 0, nSpacerRows = 0;
	for (int r = 0; r < m_iRows; r++)
	{
		if (m_iIsATeam[r] == TEAM_YES || m_iIsATeam[r] == TEAM_SPECTATORS)
			nHeaderRows++;
		else if (m_iIsATeam[r] == TEAM_BLANK)
			nSpacerRows++;
		else
			nPlayerRows++;
	}

	float maxWinH = scrHeight - 2 * padding_y;
	float chromeH = (chromeMem > 0.f) ? chromeMem : (10.f + 6.f * textHeight + 40.f);
	float bodyMaxH = maxWinH - chromeH - 10.f;

	float bodyPred = nHeaderRows * teamRowH + nSpacerRows * spacerH + nPlayerRows * rowHeight;
	float bodyWanted = bodyPred + errMem + 1.f;
	bool needScroll = bodyWanted > bodyMaxH + 0.5f;

	float bodyH = fminf(bodyWanted, bodyMaxH);
	if (bodyH < rowHeight)
		bodyH = rowHeight;

	float sb_height = fminf(ceilf(chromeH + bodyH + 10.f), maxWinH);
	float sb_x = (scrWidth - sb_width) / 2;
	float sb_y = floorf((scrHeight - sb_height) * 0.5f);

	if (sb_x < 10.f)
		sb_x = 10.f;

	LayoutMobileButtons(sb_x, sb_y, sb_width, sb_height, scrWidth, scrHeight, ui_scale);

	if (m_iSelectedPlayer && (m_iSelectedPlayer < 1 || m_iSelectedPlayer >= MAX_PLAYERS || !g_PlayerInfoList[m_iSelectedPlayer].name))
	{
		m_iSelectedPlayer = 0;
		m_bShowPlayerMenu = false;
	}

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
		char server_name[256];
		snprintf(server_name, sizeof(server_name), "Server: %s", g_ImGuiViewport.m_szServerName);
		m_ImguiUtils.TextWithColorCodes(server_name);

		// PLAYER COUNT
		char player_count[256];
		snprintf(player_count, sizeof(player_count), "%d/%d", get_player_count(), gEngfuncs.GetMaxClients());
		float text_width = ImGui::CalcTextSize(player_count).x;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - text_width - padding);
		ImGui::SetCursorPosY(line_y);
		ImGui::Text("%s", player_count);

		ImGui::Spacing();

		// MAP NAME
		char map_name[64];
		get_map_name(map_name, ARRAYSIZE(map_name));
		ImGui::Text("Map: %s", map_name);

		ImGui::Spacing();

		// NEXT MAP
		const char* nextmap = gHUD.m_Timer.GetNextmap();
		if (nextmap[0])
			ImGui::Text("Next: %s", nextmap);
		else
			ImGui::Text("Next: N/A");
		ImGui::Spacing();

		float contentAvail = ImGui::GetContentRegionAvail().x;
		float commonTableWidth = contentAvail - (needScroll ? scrollbarWidth : 0.f);

		// HEADER TABLE
		if (ImGui::BeginTable("##SBHeader", 6, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings, ImVec2(commonTableWidth, 0)))
		{
			if (!bTeamplay)
			{
				ImGui::TableSetupColumn(playersText, ImGuiTableColumnFlags_WidthStretch, 0.f);
				ImGui::TableSetupColumn(modelText, ImGuiTableColumnFlags_WidthFixed, colModel);
			}
			else
			{
				ImGui::TableSetupColumn(teamsText, ImGuiTableColumnFlags_WidthStretch, 0.f);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_Disabled, 0.f);
			}
			ImGui::TableSetupColumn(scoreText, ImGuiTableColumnFlags_WidthFixed, colScore);
			ImGui::TableSetupColumn(deathText, ImGuiTableColumnFlags_WidthFixed, colDeath);
			ImGui::TableSetupColumn(latencyText, ImGuiTableColumnFlags_WidthFixed, colPing);
			ImGui::TableSetupColumn(voiceText, ImGuiTableColumnFlags_WidthFixed, colVoice);

			const char* heads[6] = { bTeamplay ? teamsText : playersText, modelText, scoreText, deathText, latencyText, voiceText };

			ImGui::TableNextRow(0, headerRowH);
			for (int c = 0; c < 6; c++)
			{
				if (c == 1 && bTeamplay)
					continue;

				cellText(c, heads[c], headerRowH, c != 0);
			}
			ImGui::EndTable();
		}

		// HEADER SEPARATOR LINE
		{
			float y = ImGui::GetCursorScreenPos().y - 2.0f;
			draw_list->AddLine(ImVec2(lineStartX, y), ImVec2(lineEndX, y), IM_COL32(100, 100, 100, 255), 1.0f);
		}

		chromeMem = ImGui::GetCursorPosY();

		float scrollHeight = sb_height - chromeMem - 10.f;
		if (scrollHeight < 1.f)
			scrollHeight = 1.f;

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
			float contentMaxX = childMin.x + commonTableWidth;

			ImVec2 mPos = ImGui::GetMousePos();
			bool bMouseInBody = m_bMouseMode && mPos.x >= childMin.x && mPos.x <= childMax.x && mPos.y >= childMin.y && mPos.y <= childMax.y;

			// PLAYERS TABLE
			if (ImGui::BeginTable("##SBPlayers", 6, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings, ImVec2(commonTableWidth, 0)))
			{
				ImGui::TableSetupColumn("##p1", ImGuiTableColumnFlags_WidthStretch, 0.f);
				if (!bTeamplay)
					ImGui::TableSetupColumn("##p2", ImGuiTableColumnFlags_WidthFixed, colModel);
				else
					ImGui::TableSetupColumn("##p2", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_Disabled, 0.f);
				ImGui::TableSetupColumn("##p3", ImGuiTableColumnFlags_WidthFixed, colScore);
				ImGui::TableSetupColumn("##p4", ImGuiTableColumnFlags_WidthFixed, colDeath);
				ImGui::TableSetupColumn("##p5", ImGuiTableColumnFlags_WidthFixed, colPing);
				ImGui::TableSetupColumn("##p6", ImGuiTableColumnFlags_WidthFixed, colVoice);

				for (int row = 0; row < m_iRows; row++)
				{
					if (m_iIsATeam[row] == TEAM_YES)
					{
						int teamIdx = m_iSortedRows[row];
						if (teamIdx < 1 || teamIdx > m_iNumTeams)
							continue;

						ImGui::TableNextRow(0, teamRowH);
						ImGui::TableSetColumnIndex(0);

						float teamRowTop = ImGui::GetCursorScreenPos().y - cellPadY;

						team_info_t* team = &g_TeamInfo[teamIdx];
						ImU32 teamColor = getTeamColor(team->teamnumber);

						ImGui::PushStyleColor(ImGuiCol_Text, teamColor);

						// TEAM NAME
						char teamText[128];
						snprintf(teamText, sizeof(teamText), "%s - %d %s", team->name, team->players, CHudTextMessage::BufferedLocaliseTextString(team->players == 1 ? "#Player" : "#Player_plural"));
						cellText(0, teamText, teamRowH, false);

						// TEAM SCORE
						cellInt(2, team->frags, teamRowH);

						// TEAM DEATHS
						cellInt(3, team->deaths, teamRowH);

						// TEAM PING
						cellInt(4, team->ping, teamRowH);

						ImGui::PopStyleColor();

						// TEAM SEPARATOR LINE
						float y = teamRowTop + teamRowH - 1.0f;
						draw_list->PushClipRect(childMin, childMax, true);
						draw_list->AddLine(ImVec2(lineStartX, y), ImVec2(contentMaxX, y), teamColor, 1.0f);
						draw_list->PopClipRect();
					}
					else if (m_iIsATeam[row] == TEAM_SPECTATORS)
					{
						ImGui::TableNextRow(0, teamRowH);
						ImGui::TableSetColumnIndex(0);

						float rowStartY = ImGui::GetCursorScreenPos().y - cellPadY;

						// TEXT
						ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 160, 0, 255));
						cellText(0, CHudTextMessage::BufferedLocaliseTextString("#Spectators"), teamRowH, false);
						ImGui::PopStyleColor();

						// SEPARATOR LINE
						float y = rowStartY + teamRowH - 1.0f;
						draw_list->PushClipRect(childMin, childMax, true);
						draw_list->AddLine(ImVec2(lineStartX, y), ImVec2(contentMaxX, y), IM_COL32(255, 160, 0, 255), 1.0f);
						draw_list->PopClipRect();
					}
					else if (m_iIsATeam[row] == TEAM_BLANK)
					{
						ImGui::TableNextRow(0, spacerH);
					}
					else
					{
						int iPlayerIndex = m_iSortedRows[row];
						if (iPlayerIndex < 1 || iPlayerIndex >= MAX_PLAYERS || !g_PlayerInfoList[iPlayerIndex].name)
							continue;

						ImGui::TableNextRow(0, rowHeight);
						ImGui::TableSetColumnIndex(0);

						// AUTO SCROLL TO SELECTED PLAYER (mobile buttons)
						if (m_bScrollToSelected && iPlayerIndex == m_iSelectedPlayer)
						{
							ImGui::SetScrollHereY(0.5f);
							m_bScrollToSelected = false;
						}

						hud_player_info_t* pl = &g_PlayerInfoList[iPlayerIndex];
						extra_player_info_t* ex = &g_PlayerExtraInfo[iPlayerIndex];

						ImU32 playerColor = getTeamColor(ex->teamnumber);

						ImVec2 cell0 = ImGui::GetCursorScreenPos();
						float cell0W = ImGui::GetContentRegionAvail().x;
						float rowYScreen = cell0.y - cellPadY;

						// PLAYER BACKGROUND
						ImVec2 row_min = ImVec2(lineStartX, rowYScreen + 1.0f);
						ImVec2 row_max = ImVec2(contentMaxX, rowYScreen + rowHeight - 1.0f);

						ImU32 bgColor = 0;
						if (pl->thisplayer)
						{
							bgColor = IM_COL32(0, 0, 255, 70);
						}
						else if (iPlayerIndex == m_iLastKilledBy && m_fLastKillTime && m_fLastKillTime > gHUD.m_flTime)
						{
							int a = (int)(70.0f * (m_fLastKillTime - gHUD.m_flTime) / 10.0f);
							if (a < 0) a = 0;
							if (a > 255) a = 255;
							bgColor = IM_COL32(255, 0, 0, a);
						}

						bool isHovered = false;
						if (bMouseInBody)
						{
							if (mPos.x >= row_min.x && mPos.x <= row_max.x && mPos.y >= row_min.y && mPos.y <= row_max.y)
								isHovered = true;
						}

						// HOVER HIGHLIGHT (only when popup is NOT open)
						if (isHovered && !pl->thisplayer && !m_bShowPlayerMenu)
							bgColor = IM_COL32(255, 255, 255, 30);

						// SELECTED PLAYER HIGHLIGHT (buttons or click)
						if (m_iSelectedPlayer == iPlayerIndex)
							bgColor = IM_COL32(255, 255, 255, 50);

						// DRAW PLAYER BACKGROUND
						if (bgColor != 0)
						{
							draw_list->PushClipRect(childMin, childMax, true);
							draw_list->AddRectFilled(row_min, row_max, bgColor);
							draw_list->PopClipRect();
						}

						// CLICK TO OPEN CONTEXT MENU (deferred)
						if (m_bMouseMode && isHovered && ImGui::IsMouseClicked(0) && !pl->thisplayer && pl->name[0] && !m_bShowPlayerMenu)
						{
							m_iSelectedPlayer = iPlayerIndex;
							m_bShowPlayerMenu = true;
							bNeedOpenPopup = true;
						}

						ImGui::PushID(iPlayerIndex);

						float startContentX = cell0.x;

						// AVATAR
						if (bShowAvatars)
						{
							float avatarY = floorf(rowYScreen + (rowHeight - scaledAvatarSize) * 0.5f);
							float avatarX = cell0.x;

							ImVec2 avMin(avatarX, avatarY);
							ImVec2 avMax(avatarX + scaledAvatarSize, avatarY + scaledAvatarSize);
							float avatarRounding = 2.0f * ui_scale;

							draw_list->PushClipRect(childMin, childMax, true);

							// DRAW AVATAR IMAGE
							draw_list->AddImageRounded(
								g_AvatarCache.GetAvatar(iPlayerIndex),
								avMin,
								avMax,
								ImVec2(0, 0),
								ImVec2(1, 1),
								IM_COL32(255, 255, 255, 255),
								avatarRounding
							);

							// DRAW AVATAR BORDER
							draw_list->AddRect(
								avMin,
								avMax,
								playerColor,
								avatarRounding,
								0,
								1.0f * ui_scale
							);

							draw_list->PopClipRect();

							// ADVANCE CONTENT X POSITION
							startContentX = avatarX + scaledAvatarSize + (8.0f * ui_scale);
						}

						float nameY = floorf(rowYScreen + (rowHeight - textHeight) * 0.5f);
						float nameIndent = startContentX - cell0.x;

						if (nameIndent > 0.f)
							ImGui::Indent(nameIndent);

						cellOffsetY(textHeight);

						// PLAYER NAME
						ImGui::PushStyleColor(ImGuiCol_Text, playerColor);
						m_ImguiUtils.TextWithColorCodes(pl->name);
						ImGui::PopStyleColor();

						if (nameIndent > 0.f)
							ImGui::Unindent(nameIndent);

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
							m_ImguiUtils.ImGuiSpriteIcon(m_IconFlagScore.spr, m_IconFlagScore.rc, cell0.x + cell0W - iconW - 2.0f, nameY, iconW, iconH, textHeight, r, g, b, 255);
							ImGui::Dummy(ImVec2(0, 0));
						}

						// MODEL (non-teamplay only)
						if (!bTeamplay)
						{
							ImGui::TableSetColumnIndex(1);

							const char* modelStr = pl->model;
							float mw = ImGui::GetContentRegionAvail().x;
							float tw = ImGui::CalcTextSize(modelStr).x;
							float modelIndent = (tw < mw) ? (mw - tw) * 0.5f : 0.f;

							if (modelIndent > 0.f)
								ImGui::Indent(modelIndent);

							cellOffsetY(textHeight);
							m_ImguiUtils.DrawModelName(pl->topcolor, pl->bottomcolor, pl->model);

							if (modelIndent > 0.f)
								ImGui::Unindent(modelIndent);
						}

						ImGui::PushStyleColor(ImGuiCol_Text, playerColor);

						// PLAYER SCORE
						cellInt(2, ex->frags, rowHeight);

						// PLAYER DEATHS
						cellInt(3, ex->deaths, rowHeight);

						// PLAYER PING
						if (g_PlayerIsBot[iPlayerIndex])
							cellText(4, "BOT", rowHeight, true);
						else
							cellInt(4, pl->ping, rowHeight);

						ImGui::PopStyleColor();

						// VOICE ICON
						ImGui::TableSetColumnIndex(5);
						if (pl->name[0])
						{
							float iconSize = textHeight;

							float vw = ImGui::GetContentRegionAvail().x;
							float voiceIndent = (vw > iconSize) ? (vw - iconSize) * 0.5f : 0.f;

							if (voiceIndent > 0.f)
								ImGui::Indent(voiceIndent);

							cellOffsetY(iconSize);

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

							if (voiceIndent > 0.f)
								ImGui::Unindent(voiceIndent);
						}

						ImGui::PopID();
					}
				}

				ImGui::EndTable();

				float err = ImGui::GetCursorPosY() - bodyPred;
				if (!isfinite(err)) 
					err = 0.f;
				if (err < -20.f) 
					err = -20.f;
				if (err > 20.f) 
					err = 20.f;
				errMem = err;
			}
		}
		ImGui::EndChild();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(5);

		if (m_bShowPlayerMenu && (m_iSelectedPlayer < 1 || m_iSelectedPlayer >= MAX_PLAYERS || !g_PlayerInfoList[m_iSelectedPlayer].name))
		{
			m_bShowPlayerMenu = false;
			m_iSelectedPlayer = 0;
		}

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

				ImU32 selPlayerColor = getTeamColor(selEx->teamnumber);

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
					ToggleMuteSelected();
					m_bShowPlayerMenu = false;
					m_iSelectedPlayer = 0;
				}

				// STEAM PROFILE OPTION
				if (m_iSelectedPlayer > 0 && !g_PlayerIsBot[m_iSelectedPlayer] && g_PlayerSteamID64[m_iSelectedPlayer])
				{
					char profileUrl[128];
					snprintf(profileUrl, sizeof(profileUrl), "https://steamcommunity.com/profiles/%llu", (unsigned long long)g_PlayerSteamID64[m_iSelectedPlayer]);

					if (ImGui::TextLinkOpenURL("Steam Profile", profileUrl))
					{
						m_bShowPlayerMenu = false;
						m_iSelectedPlayer = 0;
					}
				}

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
	}
	ImGui::End();

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

void CImGuiScoreboard::Update()
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
}

void CImGuiScoreboard::Draw()
{
	Update();
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

void CImGuiScoreboard::UpdateMobileButtons(bool show)
{
	if (!gMobileEngfuncs || m_bButtonsShown == show)
		return;

	m_bButtonsShown = show;
	m_bBtnLayoutValid = false;

	if (!show)
	{
		gMobileEngfuncs->pfnTouchRemoveButton("sb_up");
		gMobileEngfuncs->pfnTouchRemoveButton("sb_mute");
		gMobileEngfuncs->pfnTouchRemoveButton("sb_down");
	}
}

void CImGuiScoreboard::LayoutMobileButtons(float sbX, float sbY, float sbW, float sbH, float scrW, float scrH, float uiScale)
{
	if (!gMobileEngfuncs || !m_bButtonsShown || scrW < 1.f || scrH < 1.f)
		return;

	const float btn = 56.f * uiScale;
	const float gap = 10.f * uiScale;
	const float colH = btn * 3.f + gap * 2.f;

	float px = sbX - gap - btn;
	if (px < 4.f)
		px = sbX + sbW + gap;
	if (px + btn > scrW - 4.f)
		px = scrW - btn - 4.f;
	px = floorf(px);

	float topY = floorf(sbY + (sbH - colH) * 0.5f);
	if (topY + colH > scrH - 4.f)
		topY = floorf(scrH - 4.f - colH);
	if (topY < 4.f)
		topY = 4.f;

	if (m_bBtnLayoutValid && fabsf(px - m_fBtnX) < 0.5f && fabsf(topY - m_fBtnY) < 0.5f && fabsf(btn - m_fBtnSize) < 0.5f)
		return;

	m_bBtnLayoutValid = true;
	m_fBtnX = px;
	m_fBtnY = topY;
	m_fBtnSize = btn;

	gMobileEngfuncs->pfnTouchRemoveButton("sb_up");
	gMobileEngfuncs->pfnTouchRemoveButton("sb_mute");
	gMobileEngfuncs->pfnTouchRemoveButton("sb_down");

	const float x1 = px / scrW;
	const float x2 = (px + btn) / scrW;

	auto y1 = [&](int i) { return (topY + i * (btn + gap)) / scrH; };
	auto y2 = [&](int i) { return (topY + i * (btn + gap) + btn) / scrH; };

	unsigned char col[4] = { 255, 160, 0, 200 };
	gMobileEngfuncs->pfnTouchAddClientButton("sb_up", "touch_default/prev_weap", "+sb_selprev", x1, y1(0), x2, y2(0), col, 0, 1.0f, TOUCH_FL_NOEDIT);
	gMobileEngfuncs->pfnTouchAddClientButton("sb_mute", "touch_default/microphone", "+sb_mute", x1, y1(1), x2, y2(1), col, 0, 1.0f, TOUCH_FL_NOEDIT);
	gMobileEngfuncs->pfnTouchAddClientButton("sb_down", "touch_default/next_weap", "+sb_selnext", x1, y1(2), x2, y2(2), col, 0, 1.0f, TOUCH_FL_NOEDIT);
}

static bool SbButtonBounce(int slot)
{
	static float last[3] = { -100.f, -100.f, -100.f };
	float t = gEngfuncs.GetClientTime();

	if (t >= last[slot] && t - last[slot] < 0.25f)
		return true;

	last[slot] = t;
	return false;
}

void CImGuiScoreboard::SelectPlayerStep(int dir)
{
	if (!m_ShowScore)
		return;

	if (SbButtonBounce(dir > 0 ? 1 : 0))
		return;

	int list[MAX_PLAYERS];
	int n = 0;
	int cur = -1;

	for (int r = 0; r < m_iRows; r++)
	{
		if (m_iIsATeam[r] != TEAM_NO)
			continue;

		int idx = m_iSortedRows[r];
		if (idx < 1 || idx >= MAX_PLAYERS || !g_PlayerInfoList[idx].name)
			continue;
		if (g_PlayerInfoList[idx].thisplayer)
			continue;

		if (idx == m_iSelectedPlayer)
			cur = n;
		list[n++] = idx;
	}

	if (n == 0)
		return;

	if (cur < 0)
		cur = (dir > 0) ? 0 : n - 1;
	else
		cur = (cur + dir + n) % n;

	m_iSelectedPlayer = list[cur];
	m_bShowPlayerMenu = false;
	m_bScrollToSelected = true;
}

void CImGuiScoreboard::ToggleMuteSelected()
{
	if (m_iSelectedPlayer < 1 || m_iSelectedPlayer >= MAX_PLAYERS || !g_PlayerInfoList[m_iSelectedPlayer].name)
		return;

	hud_player_info_t* selPl = &g_PlayerInfoList[m_iSelectedPlayer];
	bool isMuted = GetClientVoiceMgr()->IsPlayerBlocked(m_iSelectedPlayer);
	char string[256];

	if (isMuted)
	{
		char string1[1024];
		GetClientVoiceMgr()->SetPlayerBlockedState(m_iSelectedPlayer, false);
		snprintf(string1, sizeof(string1), CHudTextMessage::BufferedLocaliseTextString("#Unmuted"), selPl->name);
		snprintf(string, sizeof(string), "%c** %s\n", HUD_PRINTTALK, string1);
	}
	else
	{
		char string1[1024];
		char string2[1024];
		GetClientVoiceMgr()->SetPlayerBlockedState(m_iSelectedPlayer, true);
		snprintf(string1, sizeof(string1), CHudTextMessage::BufferedLocaliseTextString("#Muted"), selPl->name);
		snprintf(string2, sizeof(string2), "%s", CHudTextMessage::BufferedLocaliseTextString("#No_longer_hear_that_player"));
		snprintf(string, sizeof(string), "%c** %s %s\n", HUD_PRINTTALK, string1, string2);
	}
	gHUD.m_TextMessage.MsgFunc_TextMsg(NULL, strlen(string) + 1, string);
}

void CImGuiScoreboard::OnMuteButton()
{
	if (!m_ShowScore)
		return;

	if (SbButtonBounce(2))
		return;

	if (m_iSelectedPlayer < 1)
	{
		for (int r = 0; r < m_iRows; r++)
		{
			if (m_iIsATeam[r] != TEAM_NO)
				continue;

			int idx = m_iSortedRows[r];
			if (idx < 1 || idx >= MAX_PLAYERS || !g_PlayerInfoList[idx].name || g_PlayerInfoList[idx].thisplayer)
				continue;

			m_iSelectedPlayer = idx;
			m_bShowPlayerMenu = false;
			m_bScrollToSelected = true;
			break;
		}
		return;
	}

	ToggleMuteSelected();
}