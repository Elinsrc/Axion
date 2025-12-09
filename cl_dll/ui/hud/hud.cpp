/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <ctime>
#include "parsemsg.h"
#if USE_IMGUI
#include "imgui_viewport.h"
#include "ui_ScorePanel.h"
#endif

#include "demo.h"
#include "demo_api.h"

// Team Colors
int iNumberOfTeamColors = 5;
int iTeamColors[5][3] =
{
    { 255, 170, 0 },	// HL orange (default)
    { 125, 165, 210 },	// Blue
    { 200, 90, 70 },	// Red
    { 225, 205, 45 },	// Yellow
    { 145, 215, 140 },	// Green
};

hud_player_info_t	 g_PlayerInfoList[MAX_PLAYERS+1];	   // player info from the engine
extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];   // additional player info sent directly to the client dll
team_info_t		g_TeamInfo[MAX_TEAMS + 1];
int		g_IsSpectator[MAX_PLAYERS+1];
int g_iPlayerClass;
int g_iTeamNumber;
int g_iUser1 = 0;
int g_iUser2 = 0;
int g_iUser3 = 0;

extern client_sprite_t *GetSpriteList( client_sprite_t *pList, const char *psz, int iRes, int iCount );

extern cvar_t *sensitivity;
cvar_t *cl_lw = NULL;
cvar_t *cl_viewbob = NULL;
cvar_t *cl_rollspeed;
cvar_t *cl_rollangle;

void ShutdownInput( void );

//DECLARE_MESSAGE( m_Logo, Logo )
int __MsgFunc_Logo( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_Logo( pszName, iSize, pbuf );
}

//DECLARE_MESSAGE( m_Logo, Logo )
int __MsgFunc_ResetHUD( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_ResetHUD( pszName, iSize, pbuf );
}

int __MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	gHUD.MsgFunc_InitHUD( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	gHUD.MsgFunc_ViewMode( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_SetFOV( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_SetFOV( pszName, iSize, pbuf );
}

int __MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_Concuss( pszName, iSize, pbuf );
}

int __MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_GameMode( pszName, iSize, pbuf );
}

// TFFree Command Menu
void __CmdFunc_OpenCommandMenu( void )
{
#if USE_IMGUI
	//g_ImGuiViewport.ShowCommandMenu( g_ImGuiViewport.m_StandardMenu );
#endif
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial( void )
{
}

void __CmdFunc_CloseCommandMenu( void )
{
#if USE_IMGUI
	//g_ImGuiViewport.InputSignalHideCommandMenu();
#endif
}

void __CmdFunc_ForceCloseCommandMenu( void )
{
#if USE_IMGUI
	//g_ImGuiViewport.HideCommandMenu();
#endif
}

void __CmdFunc_Agrecord()
{
	/*
	 * Yay overcomplicating stuff.
	 * All this code makes sure we can fit as much as possible into cmd.
	 */

	char cmd[256];
	cmd[ARRAYSIZE(cmd) - 1] = '\0';

	std::time_t curtime = std::time(nullptr);

	auto written = std::strftime(cmd, sizeof(cmd), "record %Y%m%d_%H%M%S_", std::localtime(&curtime));
	if (written > 0) {
		char mapname[256];
		auto mapname_len = get_map_name(mapname, ARRAYSIZE(mapname));

		/*
		 * We want to leave at least one more byte for '\0'.
		 * written does not include the '\0'.
		 * written is strictly less than sizeof(cmd).
		 * The maximal value for written is sizeof(cmd) - 1.
		 * So if we wrote ARRAYSIZE(cmd) - 1 bytes, we have no extra bytes left.
		 */
		mapname_len = Q_min(mapname_len, ARRAYSIZE(cmd) - written - 1);
		strncpy(cmd + written, mapname, mapname_len);

		cmd[written + mapname_len] = '\0';

		if (gEngfuncs.Cmd_Argc() >= 2) {
			size_t bytes_left = ARRAYSIZE(cmd) - written - 1 - mapname_len;
			if (bytes_left >= 2) {
				cmd[written + mapname_len] = '_';

				auto arg_len = strlen(gEngfuncs.Cmd_Argv(1));
				auto bytes_to_write = Q_min(arg_len, bytes_left - 1);

				strncpy(cmd + written + mapname_len + 1, gEngfuncs.Cmd_Argv(1), bytes_to_write);

				cmd[written + mapname_len + 1 + bytes_to_write] = '\0';
			}
		}

		gEngfuncs.pfnClientCmd(cmd);
	}
}

void __CmdFunc_Append()
{
	if (gEngfuncs.Cmd_Argc() != 2) {
		if (!gEngfuncs.pDemoAPI->IsPlayingback())
			gEngfuncs.Con_Printf("append <command> - put the command into the end of the command buffer.\n");

		return;
	}

	ClientCmd(gEngfuncs.Cmd_Argv(1));
}

void __CmdFunc_Writemap()
{
	FILE* saved_maps;
	char map_name[64];
	char map_name_to_check[64];

	get_map_name(map_name, ARRAYSIZE(map_name));

	if (map_name[0])
	{
		saved_maps = fopen("saved_maps.txt", "r+");

		if (saved_maps)
		{
			while (fgets(map_name_to_check, ARRAYSIZE(map_name_to_check), saved_maps))
			{
				map_name_to_check[strlen(map_name_to_check) - 1] = '\0';

				if (!strcmp(map_name, map_name_to_check))
				{
					gEngfuncs.Con_Printf("Current map is already in saved_maps.txt\n");
					fclose(saved_maps);
					return;
				}
			}
		}
		else
		{
			saved_maps = fopen("saved_maps.txt", "a");
			if(!saved_maps)
				return;
		}

		fprintf(saved_maps, "%s\n", map_name);
		fclose(saved_maps);
	}
}

// TFFree Command Menu Message Handlers
int __MsgFunc_ValClass( const char *pszName, int iSize, void *pbuf )
{
#if USE_IMGUI
	return g_ImGuiViewport.MsgFunc_ValClass( pszName, iSize, pbuf );
#endif
	return 0;
}

int __MsgFunc_TeamNames( const char *pszName, int iSize, void *pbuf )
{
#if USE_IMGUI
	return g_ImGuiViewport.MsgFunc_TeamNames( pszName, iSize, pbuf );
#endif
	return 0;
}

int __MsgFunc_Feign( const char *pszName, int iSize, void *pbuf )
{
#if USE_IMGUI
	return g_ImGuiViewport.MsgFunc_Feign( pszName, iSize, pbuf );
#endif
	return 0;
}

int __MsgFunc_Detpack( const char *pszName, int iSize, void *pbuf )
{
#if USE_IMGUI
	return g_ImGuiViewport.MsgFunc_Detpack( pszName, iSize, pbuf );
#endif
	return 0;
}

int __MsgFunc_VGUIMenu( const char *pszName, int iSize, void *pbuf )
{
#if USE_IMGUI
	return g_ImGuiViewport.MsgFunc_IMGUIMenu( pszName, iSize, pbuf );
#endif
	return 0;
}

#if USE_IMGUI && !USE_NOIMGUI_MOTD
int __MsgFunc_MOTD(const char *pszName, int iSize, void *pbuf)
{
	return g_ImGuiViewport.MsgFunc_MOTD( pszName, iSize, pbuf );
}
#endif

int __MsgFunc_BuildSt( const char *pszName, int iSize, void *pbuf )
{
#if USE_IMGUI
	return g_ImGuiViewport.MsgFunc_BuildSt( pszName, iSize, pbuf );
#endif
	return 0;
}

int __MsgFunc_RandomPC( const char *pszName, int iSize, void *pbuf )
{
#if USE_IMGUI
	return g_ImGuiViewport.MsgFunc_RandomPC( pszName, iSize, pbuf );
#endif
	return 0;
}
 
int __MsgFunc_ServerName( const char *pszName, int iSize, void *pbuf )
{
#if USE_IMGUI
	return g_ImGuiViewport.MsgFunc_ServerName( pszName, iSize, pbuf );
#endif
	BEGIN_READ( pbuf, iSize );

	strncpy( gHUD.m_szServerName, READ_STRING(), sizeof(gHUD.m_szServerName) );
	gHUD.m_szServerName[sizeof(gHUD.m_szServerName) - 1] = 0;
	return 0;
}

#if USE_IMGUI && !USE_NOIMGUI_SCOREBOARD
int __MsgFunc_ScoreInfo(const char *pszName, int iSize, void *pbuf)
{
	return g_ImGuiViewport.MsgFunc_ScoreInfo( pszName, iSize, pbuf );
}

int __MsgFunc_TeamScore(const char *pszName, int iSize, void *pbuf)
{
	return g_ImGuiViewport.MsgFunc_TeamScore( pszName, iSize, pbuf );
}

int __MsgFunc_TeamInfo(const char *pszName, int iSize, void *pbuf)
{
	return g_ImGuiViewport.MsgFunc_TeamInfo( pszName, iSize, pbuf );
}
#endif

int __MsgFunc_Spectator( const char *pszName, int iSize, void *pbuf )
{
#if USE_IMGUI
	return g_ImGuiViewport.MsgFunc_Spectator( pszName, iSize, pbuf );
#else
	BEGIN_READ( pbuf, iSize );
	short cl = READ_BYTE();
	if( cl > 0 && cl <= MAX_PLAYERS )
	{
		g_IsSpectator[cl] = READ_BYTE();
	}
	return 1;
#endif
}

#if USE_IMGUI
int __MsgFunc_SpecFade(const char *pszName, int iSize, void *pbuf)
{
	return g_ImGuiViewport.MsgFunc_SpecFade( pszName, iSize, pbuf );
}

int __MsgFunc_ResetFade(const char *pszName, int iSize, void *pbuf)
{
	return g_ImGuiViewport.MsgFunc_ResetFade( pszName, iSize, pbuf );
}
#endif

int __MsgFunc_AllowSpec( const char *pszName, int iSize, void *pbuf )
{
#if USE_IMGUI
	return g_ImGuiViewport.MsgFunc_AllowSpec( pszName, iSize, pbuf );
#endif
	return 0;
}
 
int __MsgFunc_Gametype(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_Gametype( pszName, iSize, pbuf );
}

// This is called every time the DLL is loaded
void CHud::Init( void )
{
	HOOK_MESSAGE( Logo );
	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( GameMode );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( ViewMode );
	HOOK_MESSAGE( SetFOV );
	HOOK_MESSAGE( Concuss );
	HOOK_MESSAGE( Gametype );

	// TFFree CommandMenu
	HOOK_COMMAND( "+commandmenu", OpenCommandMenu );
	HOOK_COMMAND( "-commandmenu", CloseCommandMenu );
	HOOK_COMMAND( "ForceCloseCommandMenu", ForceCloseCommandMenu );
	HOOK_COMMAND( "special", InputPlayerSpecial );

	HOOK_COMMAND( "agrecord", Agrecord );
	HOOK_COMMAND( "append", Append );
	HOOK_COMMAND( "writemap", Writemap );
	ClientCmd("alias zpecial \"append _zpecial\"");

	HOOK_MESSAGE( ValClass );
	HOOK_MESSAGE( TeamNames );
	HOOK_MESSAGE( Feign );
	HOOK_MESSAGE( Detpack );
	HOOK_MESSAGE( BuildSt );
	HOOK_MESSAGE( RandomPC );
	HOOK_MESSAGE( ServerName );

#if USE_IMGUI && !USE_NOIMGUI_MOTD
	HOOK_MESSAGE( MOTD );
#endif

#if USE_IMGUI && !USE_NOIMGUI_SCOREBOARD
	HOOK_MESSAGE( ScoreInfo );
	HOOK_MESSAGE( TeamScore );
	HOOK_MESSAGE( TeamInfo );
#endif

	HOOK_MESSAGE( Spectator );
	HOOK_MESSAGE( AllowSpec );

#if USE_IMGUI
	HOOK_MESSAGE( SpecFade );
	HOOK_MESSAGE( ResetFade );
#endif

	// VGUI Menus
	HOOK_MESSAGE( VGUIMenu );

	CVAR_CREATE( "hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );		// controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE( "hud_takesshots", "0", FCVAR_ARCHIVE );		// controls whether or not to automatically take screenshots at the end of a round

	m_iLogo = 0;
	m_iFOV = 0;

	CVAR_CREATE( "zoom_sensitivity_ratio", "1.2", FCVAR_ARCHIVE );
	CVAR_CREATE( "cl_autowepswitch", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );
	default_fov = CVAR_CREATE( "default_fov", "90", FCVAR_ARCHIVE );
	m_pCvarStealMouse = CVAR_CREATE( "hud_capturemouse", "1", FCVAR_ARCHIVE );
	m_pCvarDraw = CVAR_CREATE( "hud_draw", "1", FCVAR_ARCHIVE );
	m_pAllowHD = CVAR_CREATE ( "hud_allow_hd", "1", FCVAR_ARCHIVE );
	cl_lw = gEngfuncs.pfnGetCvarPointer( "cl_lw" );
	cl_viewbob = CVAR_CREATE( "cl_viewbob", "0", FCVAR_ARCHIVE );
	cl_rollangle = gEngfuncs.pfnRegisterVariable( "cl_rollangle", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
	cl_rollspeed = gEngfuncs.pfnRegisterVariable( "cl_rollspeed", "200", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
	m_pSpriteList = NULL;

	// OpenAG
	m_pCvarColor = CVAR_CREATE( "hud_color", "255 160 0", FCVAR_ARCHIVE );
	CVAR_CREATE( "hud_weapon", "0", FCVAR_ARCHIVE );
	m_pCvarAutostop = CVAR_CREATE("cl_autostop", "0", FCVAR_ARCHIVE);
	m_pCvarDrawDeathNoticesAlways = CVAR_CREATE( "cl_draw_deathnotices_always", "0", FCVAR_ARCHIVE );
	m_pCvarDrawMessagesAlways = CVAR_CREATE( "cl_draw_messages_always", "0", FCVAR_ARCHIVE );
	//

	CVAR_CREATE( "hud_vis", "0", FCVAR_ARCHIVE );
	CVAR_CREATE( "cl_gauss_balls", "1", FCVAR_ARCHIVE );
	CVAR_CREATE( "cl_gauss_hits", "1", FCVAR_ARCHIVE );
	CVAR_CREATE( "cl_hidecorpses", "0", FCVAR_ARCHIVE );

	m_szServerName[0] = 0;

	// Clear any old HUD list
	if( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_AmmoSecondary.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	m_Speedometer.Init();
	m_Jumpspeed.Init();
#if !USE_IMGUI
	m_Crosshairs.Init();
	m_Debug.Init();
#endif
	m_Watermark.Init();
	m_StrafeGuide.Init();
	m_Rainbow.Init();
	m_Countdown.Init();
	m_Settings.Init();
	m_Timer.Init();
	m_CTF.Init();
	m_CustomTimer.Init();
	m_Location.Init();
	m_NextMap.Init();
	m_PlayerId.Init();
	m_SuddenDeath.Init();
	m_Timeout.Init();
	m_Vote.Init();
	// m_Splash.Init();
	m_Longjump.Init();
	GetClientVoiceMgr()->Init();

#if !USE_IMGUI || USE_NOIMGUI_MOTD
	m_MOTD.Init();
#endif
#if !USE_IMGUI || USE_NOIMGUI_SCOREBOARD
	m_Scoreboard.Init();
#endif

	m_Menu.Init();

	MsgFunc_ResetHUD( 0, 0, NULL );
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud::~CHud()
{
	delete[] m_rghSprites;
	delete[] m_rgrcRects;
	delete[] m_rgszSpriteNames;

	if( m_pHudList )
	{
		HUDLIST *pList;
		while( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rghSprites[] array
// returns 0 if sprite not found
int CHud::GetSpriteIndex( const char *SpriteName )
{
	// look through the loaded sprite name list for SpriteName
	for( int i = 0; i < m_iSpriteCount; i++ )
	{
		if( strncmp( SpriteName, m_rgszSpriteNames + ( i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH ) == 0 )
			return i;
	}

	return -1; // invalid sprite
}

void CHud::VidInit( void )
{
	int j;
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo( &m_scrinfo );

	// ----------
	// Load Sprites
	// ---------
	//m_hsprFont = LoadSprite("sprites/%d_font.spr");

	m_hsprLogo = 0;	
	m_hsprCursor = 0;

	// a1ba: don't break the loading order here and
	// don't cause memory leak but check
	// maximum HUD sprite resolution we haveAdd commentMore actions
	m_iMaxRes = 640;
	client_sprite_t *pSpriteList = m_pSpriteList ? m_pSpriteList :
	SPR_GetList( "sprites/hud.txt", &m_iSpriteCountAllRes );
	if( pSpriteList )
	{
		for( int i = 0; i < m_iSpriteCountAllRes; i++ )
		{
			if( m_iMaxRes < pSpriteList[i].iRes )
				m_iMaxRes = pSpriteList[i].iRes;
		}
	}

	m_iRes = GetSpriteRes( ScreenWidth, ScreenHeight );

	// Only load this once
	if( !m_pSpriteList )
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = pSpriteList;

		if( m_pSpriteList )
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t *p = m_pSpriteList;
			for( j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if( p->iRes == m_iRes )
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
 			m_rghSprites = new HSPRITE[m_iSpriteCount];
			m_rgrcRects = new wrect_t[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for( j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if( p->iRes == m_iRes )
				{
					char sz[256];
					sprintf( sz, "sprites/%s.spr", p->szSprite );
					m_rghSprites[index] = SPR_Load( sz );
					m_rgrcRects[index] = p->rc;
					strncpy( &m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH - 1 );
					m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH + ( MAX_SPRITE_NAME_LENGTH - 1 )] = '\0';
					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t *p = m_pSpriteList;

		// count the number of sprites of the appropriate res
		m_iSpriteCount = 0;
		for( j = 0; j < m_iSpriteCountAllRes; j++ )
		{
			if( p->iRes == m_iRes )
				m_iSpriteCount++;
			p++;
		}

		delete[] m_rghSprites;
		delete[] m_rgrcRects;
		delete[] m_rgszSpriteNames;

		// allocated memory for sprite handle arrays
 		m_rghSprites = new HSPRITE[m_iSpriteCount];
		m_rgrcRects = new wrect_t[m_iSpriteCount];
		m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

		p = m_pSpriteList;
		int index = 0;
		for( j = 0; j < m_iSpriteCountAllRes; j++ )
		{
			if( p->iRes == m_iRes )
			{
				char sz[256];
				sprintf( sz, "sprites/%s.spr", p->szSprite );
				m_rghSprites[index] = SPR_Load( sz );
				m_rgrcRects[index] = p->rc;
				strncpy( &m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH - 1 );
				m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH + ( MAX_SPRITE_NAME_LENGTH - 1 )] = '\0';
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex( "number_0" );

	if( m_HUD_number_0 == -1 )
	{
		const char *msg = "There is something wrong with your game data! Please, reinstall\n";

		if( HUD_MessageBox( msg ) )
		{
			gEngfuncs.pfnClientCmd( "quit\n" );
		}

		return;
	}

	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;

	m_Ammo.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
	m_Geiger.VidInit();
	m_Train.VidInit();
	m_Battery.VidInit();
	m_Flash.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();
	m_Speedometer.VidInit();
	m_Jumpspeed.VidInit();
#if !USE_IMGUI
	m_Crosshairs.VidInit();
	m_Debug.VidInit();
#endif
	m_Watermark.VidInit();
	m_StrafeGuide.VidInit();
	m_Countdown.VidInit();
	m_Settings.VidInit();
	m_Timer.VidInit();
	m_CTF.VidInit();
	m_CustomTimer.VidInit();
	m_Location.VidInit();
	m_NextMap.VidInit();
	m_PlayerId.VidInit();
	m_SuddenDeath.VidInit();
	m_Timeout.VidInit();
	m_Vote.VidInit();
	// m_Splash.VidInit();
	m_Longjump.VidInit();
	GetClientVoiceMgr()->VidInit();
#if !USE_IMGUI || USE_NOIMGUI_MOTD
	m_MOTD.VidInit();
#endif
#if !USE_IMGUI || USE_NOIMGUI_SCOREBOARD
	m_Scoreboard.VidInit();
#endif
}

int CHud::MsgFunc_Logo( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	// update Train data
	m_iLogo = READ_BYTE();

	return 1;
}

float g_lastFOV = 0.0;

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase ( const char *in, char *out )
{
	int len, start, end;

	len = strlen( in );

	// scan backward for '.'
	end = len - 1;
	while( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;

	if( in[end] != '.' )		// no '.', copy to end
		end = len - 1;
	else 
		end--;					// Found ',', copy to left of '.'

	// Scan backward for '/'
	start = len - 1;
	while( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if( in[start] != '/' && in[start] != '\\' )
		start = 0;
	else 
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy( out, &in[start], len );

	// Terminate it
	out[len] = 0;
}

/*
=================
HUD_IsGame

=================
*/
int HUD_IsGame( const char *game )
{
	const char *gamedir;
	char gd[1024];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if( gamedir && gamedir[0] )
	{
		COM_FileBase( gamedir, gd );
		if( !stricmp( gd, game ) )
			return 1;
	}
	return 0;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV( void )
{
	if( gEngfuncs.pDemoAPI->IsRecording() )
	{
		// Write it
		int i = 0;
		unsigned char buf[100];

		// Active
		*(float *)&buf[i] = g_lastFOV;
		i += sizeof(float);

		Demo_WriteBuffer( TYPE_ZOOM, i, buf );
	}

	if( gEngfuncs.pDemoAPI->IsPlayingback() )
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}

int CHud::MsgFunc_SetFOV( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int newfov = READ_BYTE();
	int def_fov = CVAR_GET_FLOAT( "default_fov" );

	g_lastFOV = newfov;

	if( newfov == 0 )
	{
		m_iFOV = def_fov;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if( m_iFOV == def_fov )
	{  
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{  
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)def_fov) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	return 1;
}

void CHud::AddHudElem( CHudBase *phudelem )
{
	HUDLIST *pdl, *ptemp;

	//phudelem->Think();

	if( !phudelem )
		return;

	pdl = (HUDLIST *)malloc( sizeof(HUDLIST) );
	if( !pdl )
		return;

	memset( pdl, 0, sizeof(HUDLIST) );
	pdl->p = phudelem;

	if( !m_pHudList )
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while( ptemp->pNext )
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity( void )
{
	return m_flMouseSensitivity;
}

void CHud::GetAllPlayersInfo()
{
	for( int i = 1; i < MAX_PLAYERS; i++ )
	{
		GetPlayerInfo( i, &g_PlayerInfoList[i] );

		if( g_PlayerInfoList[i].thisplayer )
		{
#if USE_IMGUI
			m_iScoreboard.m_iPlayerNum = i;
#endif
#if !USE_IMGUI || USE_NOIMGUI_SCOREBOARD
			m_Scoreboard.m_iPlayerNum = i;  // !!!HACK: this should be initialized elsewhere... maybe gotten from the engine
#endif
		}
	}
}
