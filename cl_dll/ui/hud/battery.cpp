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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#if USE_IMGUI
#include "imgui_manager.h"
#include "imgui_viewport.h"
#include "imgui_utils.h"
#include "imgui_internal.h"
#endif

cvar_t *vis_battery100;
cvar_t *vis_battery80;
cvar_t *vis_battery60;
cvar_t *vis_battery40;
cvar_t *vis_battery20;

DECLARE_MESSAGE( m_Battery, Battery )

int CHudBattery::Init( void )
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;

	HOOK_MESSAGE( Battery );

	vis_battery100 = CVAR_CREATE("vis_battery100", "0 250 0", FCVAR_ARCHIVE);
	vis_battery80 = CVAR_CREATE("vis_battery80", "0 250 0", FCVAR_ARCHIVE);
	vis_battery60 = CVAR_CREATE("vis_battery60", "250 250 0", FCVAR_ARCHIVE);
	vis_battery40 = CVAR_CREATE("vis_battery40", "250 100 0", FCVAR_ARCHIVE);
	vis_battery20 = CVAR_CREATE("vis_battery20", "250 0 0", FCVAR_ARCHIVE);

	gHUD.AddHudElem( this );

	return 1;
}

int CHudBattery::VidInit( void )
{
	int HUD_suit_empty = gHUD.GetSpriteIndex( "suit_empty" );
	int HUD_suit_full = gHUD.GetSpriteIndex( "suit_full" );

	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = 0;
	return 1;
}

int CHudBattery::MsgFunc_Battery( const char *pszName,  int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

	if( x != m_iBat )
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
	}

	return 1;
}

void CHudBattery::GetPainColor( int &r, int &g, int &b )
{
	if ( CVAR_GET_FLOAT("hud_vis") )
	{
		int iBattery = m_iBat;

		if( m_iBat > 79 )
		{
			const char *color = vis_battery100->string;
			if (sscanf( color, "%d %d %d", &r, &g, &b) == 3) {
				r = Q_max(r, 0);
				g = Q_max(g, 0);
				b = Q_max(b, 0);

				r = Q_min(r, 255);
				g = Q_min(g, 255);
				b = Q_min(b, 255);
			} else {
				UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
			}
		}
		else if( m_iBat > 59 )
		{
			const char *color = vis_battery80->string;
			if (sscanf( color, "%d %d %d", &r, &g, &b) == 3) {
				r = Q_max(r, 0);
				g = Q_max(g, 0);
				b = Q_max(b, 0);

				r = Q_min(r, 255);
				g = Q_min(g, 255);
				b = Q_min(b, 255);
			} else {
				UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
			}
		}
		else if( m_iBat > 39 )
		{
			const char *color = vis_battery60->string;
			if (sscanf( color, "%d %d %d", &r, &g, &b) == 3) {
				r = Q_max(r, 0);
				g = Q_max(g, 0);
				b = Q_max(b, 0);

				r = Q_min(r, 255);
				g = Q_min(g, 255);
				b = Q_min(b, 255);
			} else {
				UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
			}
		}
		else if( m_iBat > 19 )
		{
			const char *color = vis_battery40->string;
			if (sscanf( color, "%d %d %d", &r, &g, &b) == 3) {
				r = Q_max(r, 0);
				g = Q_max(g, 0);
				b = Q_max(b, 0);

				r = Q_min(r, 255);
				g = Q_min(g, 255);
				b = Q_min(b, 255);
			} else {
				UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
			}
		}
		else if( m_iBat > -1 )
		{
			const char *color = vis_battery20->string;
			if (sscanf( color, "%d %d %d", &r, &g, &b) == 3) {
				r = Q_max(r, 0);
				g = Q_max(g, 0);
				b = Q_max(b, 0);

				r = Q_min(r, 255);
				g = Q_min(g, 255);
				b = Q_min(b, 255);
			} else {
				UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
			}
		}
	}
	else
		UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
}

int CHudBattery::Draw( float flTime )
{
#if USE_IMGUI
	if ( CVAR_GET_FLOAT("hud_new") )
		return 1;
#endif

	if( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	int r, g, b, x, y, a;
	wrect_t rc;

	rc = *m_prc2;
	rc.top  += m_iHeight * ( (float)( 100 - ( Q_min( 100, m_iBat ) ) ) * 0.01f );	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	//UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

	if( !( gHUD.m_iWeaponBits & ( 1 << ( WEAPON_SUIT ) ) ) )
		return 1;

	// Has health changed? Flash the health #
	if( m_fFade )
	{
		if( m_fFade > FADE_TIME )
			m_fFade = FADE_TIME;

		m_fFade -= ( (float)gHUD.m_flTimeDelta * 20.0f );
		if( m_fFade <= 0 )
		{
			a = 128;
			m_fFade = 0;
		}

		// Fade the health number back to dim
		a = MIN_ALPHA + ( m_fFade / FADE_TIME ) * 128;
	}
	else
		a = MIN_ALPHA;

	GetPainColor( r, g, b );
	ScaleColors( r, g, b, a );

	int iOffset = ( m_prc1->bottom - m_prc1->top ) / 6;

	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

	if( gHUD.IsHL25( )) // a1ba: HL25 style
		x = ( m_prc1->right - m_prc1->left ) * 3;
	else
		x = ScreenWidth / 5;

	// make sure we have the right sprite handles
	if( !m_hSprite1 )
		m_hSprite1 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_empty" ) );
	if( !m_hSprite2 )
		m_hSprite2 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_full" ) );

	SPR_Set( m_hSprite1, r, g, b );
	SPR_DrawAdditive( 0,  x, y - iOffset, m_prc1 );

	if( rc.bottom > rc.top )
	{
		SPR_Set( m_hSprite2, r, g, b );
		SPR_DrawAdditive( 0, x, y - iOffset + ( rc.top - m_prc2->top ), &rc );
	}

	x += ( m_prc1->right - m_prc1->left );
	x = gHUD.DrawHudNumber( x, y + gHUD.m_iHudNumbersYOffset, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b );

	return 1;
}

#if USE_IMGUI
void CHudBattery::ImGui_BatteryBar()
{
	if (!(CVAR_GET_FLOAT("hud_new"))) 
		return;
	
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) || gEngfuncs.IsSpectateOnly()) 
		return;
	
	if (!(gHUD.m_iWeaponBits & (1 << (WEAPON_SUIT)))) 
		return;

	float scale = CVAR_GET_FLOAT("hud_new_scale");
	
	if (scale <= 1.0f) 
		scale = 1.0f;

	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

	int bat = m_iBat;
	static int s_maxBat = 100;
	if (bat > s_maxBat) 
		s_maxBat = bat;
	if (gHUD.m_Health.m_iHealth <= 0) 
		s_maxBat = 100;

	float frac = ImClamp((float)bat / (float)s_maxBat, 0.0f, 1.0f);
	static float smoothBat = 0.0f;
	smoothBat = ImLerp(smoothBat, frac, 0.1f);

	float radius = 85.0f * scale; 
	float thickness = 4.0f * scale;
	float startAngle = -IM_PI * 0.28f; 
	float endAngle = IM_PI * 0.28f;
	float currentAngle = endAngle - (endAngle - startAngle) * smoothBat;

	dl->PathArcTo(center, radius, startAngle, endAngle, 40);
	dl->PathStroke(IM_COL32(0, 0, 0, 80), 0, thickness + (2.0f * scale));

	if (smoothBat > 0.001f) 
	{
		dl->PathArcTo(center, radius, currentAngle, endAngle, 40);
		dl->PathStroke(IM_COL32(255, 180, 0, 220), 0, thickness);
	}

	char buf[16]; 
	sprintf(buf, "%d", bat);
	float fontSize = 22.0f * scale;
	ImFont* font = ImGui::GetFont();
	ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, buf);

	int iSuitFull = gHUD.GetSpriteIndex("suit_full");
	wrect_t rc = gHUD.GetSpriteRect(iSuitFull);
	float sw = (float)(rc.right - rc.left);
	float sh = (float)(rc.bottom - rc.top);
	float iconH = 20.0f * scale; 
	float iconW = (sh > 0) ? sw * (iconH / sh) : (20.0f * scale);

	float blockCenterX = center.x + radius + (25.0f * scale); 

	float iconY = center.y - iconH - (2.0f * scale);
	float textY = center.y + (2.0f * scale);

	m_ImguiUtils.ImGuiSpriteIcon(gHUD.GetSprite(iSuitFull), rc, blockCenterX - (iconW * 0.5f), iconY, iconW, iconH, iconH, 255, 180, 0, 220);
	m_ImguiUtils.DrawTextShadow(fontSize, ImVec2(blockCenterX - (textSize.x * 0.5f), textY), buf, IM_COL32(255, 180, 0, 220));
}
#endif