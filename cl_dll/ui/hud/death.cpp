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
// death notice
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#if USE_IMGUI
#include "imgui_manager.h"
#include "imgui_viewport.h"
#include "ui_utils.h"
#endif

DECLARE_MESSAGE( m_DeathNotice, DeathMsg )

struct DeathNoticeItem {
	char szKiller[MAX_PLAYER_NAME_LENGTH * 2];
	char szVictim[MAX_PLAYER_NAME_LENGTH * 2];
	int iId;	// the index number of the associated sprite
	int iSuicide;
	int iTeamKill;
	int iNonPlayerKill;
	float flDisplayTime;
	float *KillerColor;
	float *VictimColor;

#if USE_IMGUI
	int iKillerKilledLocal;
#endif
};

#define MAX_DEATHNOTICES	4
static int DEATHNOTICE_DISPLAY_TIME = 6;

#define DEATHNOTICE_TOP		32

DeathNoticeItem rgDeathNoticeList[MAX_DEATHNOTICES + 1];

float g_ColorBlue[3]	= { 0.6, 0.8, 1.0 };
float g_ColorRed[3]	= { 1.0, 0.25, 0.25 };
float g_ColorGreen[3]	= { 0.6, 1.0, 0.6 };
float g_ColorYellow[3]	= { 1.0, 0.7, 0.0 };
float g_ColorGrey[3]	= { 0.8, 0.8, 0.8 };

cvar_t *m_pCvarKillSnd;
cvar_t *m_pCvarKillSndPath;

cvar_t *m_pCvarDeathBg;

float *GetClientColor( int clientIndex )
{
	switch( g_PlayerExtraInfo[clientIndex].teamnumber )
	{
	case 1:	return g_ColorBlue;
	case 2: return g_ColorRed;
	case 3: return g_ColorYellow;
	case 4: return g_ColorGreen;
	case 0: return g_ColorYellow;
	default: return g_ColorGrey;
	}

	return NULL;
}

int CHudDeathNotice::Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( DeathMsg );

	CVAR_CREATE( "hud_deathnotice_time", "6", FCVAR_ARCHIVE );

	m_pCvarDeathBg = CVAR_CREATE( "hud_deathnotice_bg", "1", FCVAR_ARCHIVE );

	// OpenAG
	m_pCvarKillSnd = CVAR_CREATE( "cl_killsound", "1", FCVAR_ARCHIVE );
	m_pCvarKillSndPath = CVAR_CREATE( "cl_killsound_path", "buttons/bell1.wav", FCVAR_ARCHIVE );

	return 1;
}

void CHudDeathNotice::InitHUDData( void )
{
	memset( rgDeathNoticeList, 0, sizeof(rgDeathNoticeList) );
}

int CHudDeathNotice::VidInit( void )
{
	m_HUD_d_skull = gHUD.GetSpriteIndex( "d_skull" );

	return 1;
}

#if USE_IMGUI

#define DN_BG_ROUNDING 5.0f
#define DN_TEXT_PADDING_Y 4.0f
#define DN_ROW_SPACING 5.0f
#define DN_ICON_TEXT_SPACING 5.0f
#define DN_RIGHT_MARGIN 10.0f
#define DN_Y_OFFSET 2.0f
#define DN_BG_Y_OFFSET -2.0f
#define DN_BG_PADDING 5.0f

#define DN_BG_ALPHA_BASE          0.6f
#define DN_HIGHLIGHT_ALPHA        70.0f

#define DN_MAX_ICON_SCALE 1.0f

void CHudDeathNotice::ImGui_DeathNotice()
{
	ImFont* hudFont = g_ImGuiManager.GetHudFont();
	ImGui::PushFont(hudFont);

	ImDrawList* dl = ImGui::GetBackgroundDrawList();

	for (int i = 0; i < MAX_DEATHNOTICES; ++i)
	{
		if (rgDeathNoticeList[i].iId == 0)
			break;

		if (rgDeathNoticeList[i].flDisplayTime < gHUD.m_flTime)
		{
			memmove(&rgDeathNoticeList[i], &rgDeathNoticeList[i + 1], sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - i));
			--i;
			continue;
		}

		rgDeathNoticeList[i].flDisplayTime = Q_min(rgDeathNoticeList[i].flDisplayTime, gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME);

		float textHeight = ImGui::GetTextLineHeight();
		float bg_h  = textHeight + DN_TEXT_PADDING_Y;
		float rowHeight  = bg_h + DN_ROW_SPACING;

		float killerWidth = m_ImguiUtils.CalcTextWidthWithColorCodes(rgDeathNoticeList[i].szKiller);
		float victimWidth = m_ImguiUtils.CalcTextWidthWithColorCodes(rgDeathNoticeList[i].szVictim);

		float y = (int)(YRES(DEATHNOTICE_TOP) + DN_Y_OFFSET + rowHeight * i);

		int id = (rgDeathNoticeList[i].iId == -1) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;

		wrect_t rc = gHUD.GetSpriteRect(id);
		int spriteWidth  = rc.right  - rc.left;
		int spriteHeight = rc.bottom - rc.top;

		float maxIconHeight = textHeight;
		float scale = DN_MAX_ICON_SCALE;
		if (spriteHeight > 0 && spriteHeight > maxIconHeight)
			scale = maxIconHeight / (float)spriteHeight;

		float iconWidth = spriteWidth  * scale;
		float iconHeight = spriteHeight * scale;

		float timeLeft  = rgDeathNoticeList[i].flDisplayTime - gHUD.m_flTime;
		float totalTime = (float)DEATHNOTICE_DISPLAY_TIME;
		float t = timeLeft / totalTime;

		if (t < 0.0f)
			t = 0.0f;
		if (t > 1.0f)
			t = 1.0f;

		int alphaBg = (int)(255.0f * DN_BG_ALPHA_BASE * t);
		int alphaUI = (int)(255.0f * t);
		float alphaUIF = alphaUI / 255.0f;

		bool highlight = (rgDeathNoticeList[i].iKillerKilledLocal != 0);

		float x = g_ImGuiViewport.scrWidth() - victimWidth - iconWidth - DN_RIGHT_MARGIN;

		if (!rgDeathNoticeList[i].iSuicide)
			x -= DN_ICON_TEXT_SPACING + killerWidth;

		float bg_x = x - DN_BG_PADDING;
		float bg_y = (float)y + DN_BG_Y_OFFSET;
		float bg_w = victimWidth + iconWidth + DN_RIGHT_MARGIN;
		if (!rgDeathNoticeList[i].iSuicide)
			bg_w += DN_ICON_TEXT_SPACING + killerWidth;

		ImU32 bgColor;
		if (highlight)
		{
			int a = (int)(DN_HIGHLIGHT_ALPHA * t);
			bgColor = IM_COL32(255, 0, 0, a);
		}
		else
		{
			bgColor = IM_COL32(0, 0, 0, alphaBg);
		}

		if (m_pCvarDeathBg->value > 0.0f)
			dl->AddRectFilled(ImVec2(bg_x, bg_y), ImVec2(bg_x + bg_w, bg_y + bg_h), bgColor, DN_BG_ROUNDING);

		if (!rgDeathNoticeList[i].iSuicide && rgDeathNoticeList[i].szKiller[0])
		{
			ImVec4 baseColor(1.0f, 1.0f, 1.0f, 1.0f);
			if (rgDeathNoticeList[i].KillerColor)
			{
				baseColor.x = rgDeathNoticeList[i].KillerColor[0];
				baseColor.y = rgDeathNoticeList[i].KillerColor[1];
				baseColor.z = rgDeathNoticeList[i].KillerColor[2];
			}

			m_ImguiUtils.DrawTextWithColorCodesAt(ImVec2(x, y), rgDeathNoticeList[i].szKiller, baseColor, alphaUIF);

			x += killerWidth + DN_ICON_TEXT_SPACING;
		}

		int r = 255;
		int g = 80;
		int b = 0;

		if (rgDeathNoticeList[i].iTeamKill)
		{
			r = 10;
			g = 240;
			b = 10;
		}

		HSPRITE hSpr = gHUD.GetSprite(id);

		x = m_ImguiUtils.ImGuiSpriteIcon(hSpr, rc, x, y, iconWidth, iconHeight, textHeight, r, g, b, alphaUI);

		if (!rgDeathNoticeList[i].iNonPlayerKill && rgDeathNoticeList[i].szVictim[0])
		{
			ImVec4 baseColor(1.0f, 1.0f, 1.0f, 1.0f);
			if (rgDeathNoticeList[i].VictimColor)
			{
				baseColor.x = rgDeathNoticeList[i].VictimColor[0];
				baseColor.y = rgDeathNoticeList[i].VictimColor[1];
				baseColor.z = rgDeathNoticeList[i].VictimColor[2];
			}

			m_ImguiUtils.DrawTextWithColorCodesAt(ImVec2(x, y), rgDeathNoticeList[i].szVictim, baseColor, alphaUIF);
		}
	}

	ImGui::PopFont();
}
#endif

int CHudDeathNotice::Draw( float flTime )
{
#if USE_IMGUI
	return 1;
#endif

	int x, y, r, g, b;

	for( int i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if( rgDeathNoticeList[i].iId == 0 )
			break;  // we've gone through them all

		if( rgDeathNoticeList[i].flDisplayTime < flTime )
		{
			// display time has expired
			// remove the current item from the list
			memmove( &rgDeathNoticeList[i], &rgDeathNoticeList[i + 1], sizeof(DeathNoticeItem) * ( MAX_DEATHNOTICES - i ) );
			i--;  // continue on the next item;  stop the counter getting incremented
			continue;
		}

		rgDeathNoticeList[i].flDisplayTime = Q_min( rgDeathNoticeList[i].flDisplayTime, gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME );

		// Only draw if the viewport will let me
		// vgui dropped out
#if USE_IMGUI
		//if( g_ImGuiViewport.AllowedToPrintText() )
#endif
		{
			// Draw the death notice
			y = YRES( DEATHNOTICE_TOP ) + 2 + ( 25 * i );  //!!!

			int id = ( rgDeathNoticeList[i].iId == -1 ) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;
			x = ScreenWidth - ConsoleStringLen( rgDeathNoticeList[i].szVictim ) - ( gHUD.GetSpriteRect(id).right - gHUD.GetSpriteRect(id).left ) - 10;

			if( !rgDeathNoticeList[i].iSuicide )
			{
				x -= ( 5 + ConsoleStringLen( rgDeathNoticeList[i].szKiller ) );

				if (m_pCvarDeathBg->value > 0.0f && !gHUD.IsHL25( ))
					FillRGBABlend( x - 5, y - 2, ConsoleStringLen( rgDeathNoticeList[i].szVictim ) + ConsoleStringLen( rgDeathNoticeList[i].szKiller ) + ( gHUD.GetSpriteRect(id).right - gHUD.GetSpriteRect(id).left ) + 15, 20 , 0, 0, 0, 255 * 0.6 );

				// Draw killers name
				if( rgDeathNoticeList[i].KillerColor )
						x = 5 + gHUD.DrawConsoleStringWithColorTags(
						x,
						y,
						rgDeathNoticeList[i].szKiller,
						true,
						rgDeathNoticeList[i].KillerColor[0],
						rgDeathNoticeList[i].KillerColor[1],
						rgDeathNoticeList[i].KillerColor[2]
					);
				else
					x = 5 + DrawConsoleString( x, y, rgDeathNoticeList[i].szKiller );
			}
			else
			{
				if (m_pCvarDeathBg->value > 0.0f && !gHUD.IsHL25( ))
					FillRGBABlend( x - 5, y - 2, ConsoleStringLen( rgDeathNoticeList[i].szVictim ) + ( gHUD.GetSpriteRect(id).right - gHUD.GetSpriteRect(id).left ) + 10, 20 , 0, 0, 0, 255 * 0.6 );
			}

			r = 255; g = 80; b = 0;
			if( rgDeathNoticeList[i].iTeamKill )
			{
				r = 10;	g = 240; b = 10;  // display it in sickly green
			}

			// Draw death weapon
			SPR_Set( gHUD.GetSprite(id), r, g, b );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect(id) );

			x += ( gHUD.GetSpriteRect(id).right - gHUD.GetSpriteRect(id).left );

			// Draw victims name (if it was a player that was killed)
			if( rgDeathNoticeList[i].iNonPlayerKill == FALSE )
			{
				if( rgDeathNoticeList[i].VictimColor )
					x = gHUD.DrawConsoleStringWithColorTags(
						x,
						y,
						rgDeathNoticeList[i].szVictim,
						true,
						rgDeathNoticeList[i].VictimColor[0],
						rgDeathNoticeList[i].VictimColor[1],
						rgDeathNoticeList[i].VictimColor[2]
					);
				else
					x = DrawConsoleString( x, y, rgDeathNoticeList[i].szVictim );
			}
		}
	}

	return 1;
}

// This message handler may be better off elsewhere
int CHudDeathNotice::MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf )
{
	int i;
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );

	int killer = READ_BYTE();
	int victim = READ_BYTE();

	char killedwith[32];
	strcpy( killedwith, "d_" );
	strncat( killedwith, READ_STRING(), sizeof(killedwith) - strlen(killedwith) - 1 );
	killedwith[sizeof(killedwith) - 1] = '\0';

#if USE_IMGUI && !USE_NOIMGUI_SCOREBOARD
	g_ImGuiViewport.DeathMsg( killer, victim );
#else
	gHUD.m_Scoreboard.DeathMsg( killer, victim );
#endif

	gHUD.m_Spectator.DeathMessage( victim );

	for( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if( rgDeathNoticeList[i].iId == 0 )
			break;
	}
	if( i == MAX_DEATHNOTICES )
	{
		// move the rest of the list forward to make room for this item
		memmove( rgDeathNoticeList, rgDeathNoticeList + 1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES );
		i = MAX_DEATHNOTICES - 1;
	}

	gHUD.GetAllPlayersInfo();

	// Get the Killer's name
	const char *killer_name = "";
	killer_name = g_PlayerInfoList[killer].name;
	if( !killer_name )
	{
		killer_name = "";
		rgDeathNoticeList[i].szKiller[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].KillerColor = GetClientColor( killer );
		strncpy( rgDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH - 1 );
		rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH - 1] = 0;
	}

	// Get the Victim's name
	const char *victim_name = "";
	// If victim is -1, the killer killed a specific, non-player object (like a sentrygun)
	if( ( (signed char)victim ) != -1 )
		victim_name = g_PlayerInfoList[victim].name;
	if( !victim_name )
	{
		victim_name = "";
		rgDeathNoticeList[i].szVictim[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].VictimColor = GetClientColor( victim );
		strncpy( rgDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH - 1 );
		rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH - 1] = 0;
	}

	// Is it a non-player object kill?
	if( ( (signed char)victim ) == -1 )
	{
		rgDeathNoticeList[i].iNonPlayerKill = TRUE;

		// Store the object's name in the Victim slot (skip the d_ bit)
		strcpy( rgDeathNoticeList[i].szVictim, killedwith + 2 );
	}
	else
	{
		if( killer == victim || killer == 0 )
			rgDeathNoticeList[i].iSuicide = TRUE;

		if( !strcmp( killedwith, "d_teammate" ) )
			rgDeathNoticeList[i].iTeamKill = TRUE;
	}

#if USE_IMGUI
	rgDeathNoticeList[i].iKillerKilledLocal = FALSE;
	if( !rgDeathNoticeList[i].iNonPlayerKill && !rgDeathNoticeList[i].iSuicide )
	{
		if( g_PlayerInfoList[victim].thisplayer || g_iUser2 == victim )
		{
			rgDeathNoticeList[i].iKillerKilledLocal = TRUE;
		}
	}
#endif

	// Find the sprite in the list
	int spr = gHUD.GetSpriteIndex( killedwith );

	rgDeathNoticeList[i].iId = spr;

	DEATHNOTICE_DISPLAY_TIME = CVAR_GET_FLOAT( "hud_deathnotice_time" );
	rgDeathNoticeList[i].flDisplayTime = gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME;

	// Play kill sound
	if ((g_PlayerInfoList[killer].thisplayer || g_iUser2 == killer) &&
		!rgDeathNoticeList[i].iNonPlayerKill &&
		!rgDeathNoticeList[i].iSuicide &&
		m_pCvarKillSnd->value > 0.0f)
	{
		PlaySound(m_pCvarKillSndPath->string, m_pCvarKillSnd->value);
	}

	if( rgDeathNoticeList[i].iNonPlayerKill )
	{
		ConsolePrint( rgDeathNoticeList[i].szKiller );
		ConsolePrint( " killed a " );
		ConsolePrint( rgDeathNoticeList[i].szVictim );
		ConsolePrint( "\n" );
	}
	else
	{
		// record the death notice in the console
		if( rgDeathNoticeList[i].iSuicide )
		{
			ConsolePrint( rgDeathNoticeList[i].szVictim );

			if( !strcmp( killedwith, "d_world" ) )
			{
				ConsolePrint( " died" );
			}
			else
			{
				ConsolePrint( " killed self" );
			}
		}
		else if( rgDeathNoticeList[i].iTeamKill )
		{
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed his teammate " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}
		else
		{
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}

		if( *killedwith && (*killedwith > 13 ) && strcmp( killedwith, "d_world" ) && !rgDeathNoticeList[i].iTeamKill )
		{
			ConsolePrint( " with " );

			// replace the code names with the 'real' names
			if( !strcmp( killedwith + 2, "egon" ) )
				strcpy( killedwith, "d_gluon gun" );
			if( !strcmp( killedwith + 2, "gauss" ) )
				strcpy( killedwith, "d_tau cannon" );

			ConsolePrint( killedwith + 2 ); // skip over the "d_" part
		}

		ConsolePrint( "\n" );
	}

	return 1;
}
