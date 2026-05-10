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
// Health.cpp
//
// implementation of CHudHealth class
//

#include <stdio.h>
#include <stdlib.h>
#include <cmath>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>

#include "mobility_int.h"

#if USE_IMGUI
#include "imgui_manager.h"
#include "imgui_viewport.h"
#include "imgui_utils.h"
#include "imgui_internal.h"
#endif

cvar_t *vis_health100;
cvar_t *vis_health80;
cvar_t *vis_health60;
cvar_t *vis_health40;
cvar_t *vis_health20;

cvar_t *hud_health_divider;

DECLARE_MESSAGE( m_Health, Health )
DECLARE_MESSAGE( m_Health, Damage )

#define PAIN_NAME "sprites/%d_pain.spr"
#define DAMAGE_NAME "sprites/%d_dmg.spr"

int giDmgHeight, giDmgWidth;

int giDmgFlags[NUM_DMG_TYPES] =
{
	DMG_POISON,
	DMG_ACID,
	DMG_FREEZE|DMG_SLOWFREEZE,
	DMG_DROWN,
	DMG_BURN|DMG_SLOWBURN,
	DMG_NERVEGAS, 
	DMG_RADIATION,
	DMG_SHOCK,
	DMG_CALTROP,
	DMG_TRANQ,
	DMG_CONCUSS,
	DMG_HALLUC
};

int CHudHealth::Init( void )
{
	HOOK_MESSAGE( Health );
	HOOK_MESSAGE( Damage );
	m_iHealth = 100;
	m_fFade = 0;
	m_iFlags = 0;
	m_bitsDamage = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;

	memset( m_dmg, 0, sizeof(DAMAGE_IMAGE) * NUM_DMG_TYPES );

	vis_health100 = CVAR_CREATE("vis_health100", "0 250 0", FCVAR_ARCHIVE);
	vis_health80 = CVAR_CREATE("vis_health80", "0 250 0", FCVAR_ARCHIVE);
	vis_health60 = CVAR_CREATE("vis_health60", "250 250 0", FCVAR_ARCHIVE);
	vis_health40 = CVAR_CREATE("vis_health40", "250 100 0", FCVAR_ARCHIVE);
	vis_health20 = CVAR_CREATE("vis_health20", "250 0 0", FCVAR_ARCHIVE);

	hud_health_divider = CVAR_CREATE("hud_health_divider", "1", FCVAR_ARCHIVE);

	gHUD.AddHudElem( this );
	return 1;
}

void CHudHealth::Reset( void )
{
	// make sure the pain compass is cleared when the player respawns
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;

	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	for( int i = 0; i < NUM_DMG_TYPES; i++ )
	{
		m_dmg[i].fExpire = 0;
	}
}

int CHudHealth::VidInit( void )
{
	m_hSprite = 0;

	m_HUD_dmg_bio = gHUD.GetSpriteIndex( "dmg_bio" ) + 1;
	m_HUD_cross = gHUD.GetSpriteIndex( "cross" );

	giDmgHeight = gHUD.GetSpriteRect( m_HUD_dmg_bio ).right - gHUD.GetSpriteRect( m_HUD_dmg_bio ).left;
	giDmgWidth = gHUD.GetSpriteRect( m_HUD_dmg_bio ).bottom - gHUD.GetSpriteRect( m_HUD_dmg_bio ).top;

	return 1;
}

int CHudHealth::MsgFunc_Health( const char *pszName, int iSize, void *pbuf )
{
	// TODO: update local health data
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if( x != m_iHealth )
	{
		m_fFade = FADE_TIME;
		m_iHealth = x;
	}

	return 1;
}

int CHudHealth::MsgFunc_Damage( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int armor = READ_BYTE();	// armor
	int damageTaken = READ_BYTE();	// health
	long bitsDamage = READ_LONG(); // damage bits

	vec3_t vecFrom;

	for( int i = 0; i < 3; i++ )
		vecFrom[i] = READ_COORD();

	UpdateTiles( gHUD.m_flTime, bitsDamage );

	// Actually took damage?
	if( damageTaken > 0 || armor > 0 )
	{
		CalcDamageDirection( vecFrom );

                if( gMobileEngfuncs && damageTaken > 0 )
                {
			float time = damageTaken * 4.0f;

			if( time > 200.0f )
				time = 200.0f;
			gMobileEngfuncs->pfnVibrate( time, 0 );
                }
	}

	return 1;
}

// Returns back a color from the
// Green <-> Yellow <-> Red ramp
void CHudHealth::GetPainColor( int &r, int &g, int &b )
{
	int iHealth = m_iHealth;

	if ( CVAR_GET_FLOAT("hud_vis") )
	{
		if( m_iHealth > 79 )
		{
			const char *color = vis_health100->string;
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
		else if( m_iHealth > 59 )
		{
			const char *color = vis_health80->string;
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
		else if( m_iHealth > 39 )
		{
			const char *color = vis_health60->string;
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
		else if( m_iHealth > 19 )
		{
			const char *color = vis_health40->string;
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
		else if( m_iHealth > -1 )
		{
			const char *color = vis_health20->string;
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
	{
	#if 0
		if( iHealth > 25 )
			iHealth -= 25;
		else if( iHealth < 0 )
			iHealth = 0;

		g = iHealth * 255 / 100;
		r = 255 - g;
		b = 0;
    #else
		if( m_iHealth > 25 )
		{
			UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

		}
		else
		{
			r = 250;
			g = 0;
			b = 0;
		}
	#endif
	}
}

int CHudHealth::Draw( float flTime )
{
#if USE_IMGUI
	if ( CVAR_GET_FLOAT("hud_new") )
		return 1;
#endif

	int r, g, b;
	int a = 0, x, y;
	int HealthWidth;

	if( ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH ) || gEngfuncs.IsSpectateOnly() )
		return 1;

	if( !m_hSprite )
		m_hSprite = LoadSprite( PAIN_NAME );

	// Has health changed? Flash the health #
	if( m_fFade )
	{
		m_fFade -= ( (float)gHUD.m_flTimeDelta * 20.0f );
		if( m_fFade <= 0 )
		{
			a = MIN_ALPHA;
			m_fFade = 0;
		}

		// Fade the health number back to dim
		a = MIN_ALPHA + ( m_fFade / FADE_TIME ) * 128;
	}
	else
		a = MIN_ALPHA;

	// If health is getting low, make it bright red
	if( m_iHealth <= 15 )
		a = 255;

	GetPainColor( r, g, b );
	ScaleColors( r, g, b, a );

	// Only draw health if we have the suit.
	if( gHUD.m_iWeaponBits & ( 1 << ( WEAPON_SUIT ) ) )
	{
		HealthWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;
		int CrossWidth = gHUD.GetSpriteRect( m_HUD_cross ).right - gHUD.GetSpriteRect( m_HUD_cross ).left;

		y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
		x = CrossWidth / 2;

		SPR_Set( gHUD.GetSprite( m_HUD_cross ), r, g, b );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( m_HUD_cross ) );

		x = CrossWidth + HealthWidth / 2;

		x = gHUD.DrawHudNumber( x, y + gHUD.m_iHudNumbersYOffset, DHN_3DIGITS | DHN_DRAWZERO, m_iHealth, r, g, b );

		x += HealthWidth / 2;

		int iHeight = gHUD.m_iFontHeight;
		int iWidth = HealthWidth / 10;
		UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

		if (hud_health_divider->value)
			FillRGBA( x, y + gHUD.m_iHudNumbersYOffset, iWidth, iHeight, r, g, b, a );
	}

	DrawDamage( flTime );
	return DrawPain( flTime );
}

#if USE_IMGUI
void CHudHealth::ImGui_HealthBar()
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

	int hp = m_iHealth;
	static int s_maxHp = 100;
	if (hp > s_maxHp) 
		s_maxHp = hp;
	if (hp <= 0) 
		s_maxHp = 100;
	
	float frac = ImClamp((float)hp / (float)s_maxHp, 0.0f, 1.0f);
	static float smoothHp = 0.0f;
	smoothHp = ImLerp(smoothHp, frac, 0.1f);

	float r, g, b;
	if (smoothHp > 0.5f) 
	{
		float t = (smoothHp - 0.5f) * 2.0f;
		r = ImLerp(255.0f, 0.0f, t);
		g = ImLerp(200.0f, 255.0f, t);
		b = 0.0f;
	} else 
	{
		float t = smoothHp * 2.0f;
		r = ImLerp(255.0f, 255.0f, t);
		g = ImLerp(0.0f, 200.0f, t);
		b = 0.0f;
	}
	ImU32 hpColor = IM_COL32((int)r, (int)g, (int)b, 230);

	float radius = 85.0f * scale; 
	float thickness = 4.0f * scale;
	float startAngle = IM_PI * 0.72f;
	float endAngle = IM_PI * 1.28f;
	float currentAngle = startAngle + (endAngle - startAngle) * smoothHp;

	dl->PathArcTo(center, radius, startAngle, endAngle, 40);
	dl->PathStroke(IM_COL32(0, 0, 0, 80), 0, thickness + (2.0f * scale));

	if (smoothHp > 0.001f) 
	{
		dl->PathArcTo(center, radius, startAngle, currentAngle, 40);
		dl->PathStroke(hpColor, 0, thickness);
	}

	char buf[16]; 
	sprintf(buf, "%d", hp);
	float fontSize = 22.0f * scale;
	ImFont* font = ImGui::GetFont();
	float textLineHeight = ImGui::GetTextLineHeight();
	ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, buf);

	wrect_t rc = gHUD.GetSpriteRect(m_HUD_cross);
	float sw = (float)(rc.right - rc.left);
	float sh = (float)(rc.bottom - rc.top);
	float iconH = 20.0f * scale;
	float iconW = sw * (iconH / sh);

	float blockCenterX = center.x - radius - (25.0f * scale); 
	
	float iconY = center.y - iconH - (2.0f * scale);
	float textY = center.y + (2.0f * scale);

	m_ImguiUtils.ImGuiSpriteIcon(gHUD.GetSprite(m_HUD_cross), rc, blockCenterX - (iconW * 0.5f), iconY, iconW, iconH, iconH, (int)r, (int)g, (int)b, 230);
	m_ImguiUtils.DrawTextShadow(fontSize, ImVec2(blockCenterX - (textSize.x * 0.5f), textY), buf, hpColor);
}
#endif

void CHudHealth::CalcDamageDirection( vec3_t vecFrom )
{
	vec3_t forward, right, up;
	float side, front;
	vec3_t vecOrigin, vecAngles;

	if( !vecFrom[0] && !vecFrom[1] && !vecFrom[2] )
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
		return;
	}

	memcpy( vecOrigin, gHUD.m_vecOrigin, sizeof(vec3_t) );
	memcpy( vecAngles, gHUD.m_vecAngles, sizeof(vec3_t) );

	VectorSubtract( vecFrom, vecOrigin, vecFrom );

	float flDistToTarget = vecFrom.Length();

	vecFrom = vecFrom.Normalize();
	AngleVectors( vecAngles, forward, right, up );

	front = DotProduct( vecFrom, right );
	side = DotProduct( vecFrom, forward );

	if( flDistToTarget <= 50 )
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
	}
	else 
	{
		if( side > 0.0f )
		{
			if( side > 0.3f )
				m_fAttackFront = Q_max( m_fAttackFront, side );
		}
		else
		{
			float f = fabs( side );
			if( f > 0.3f )
				m_fAttackRear = Q_max( m_fAttackRear, f );
		}

		if( front > 0.0f )
		{
			if( front > 0.3f )
				m_fAttackRight = Q_max( m_fAttackRight, front );
		}
		else
		{
			float f = fabs( front );
			if( f > 0.3f )
				m_fAttackLeft = Q_max( m_fAttackLeft, f );
		}
	}
}

int CHudHealth::DrawPain( float flTime )
{
	if( !( m_fAttackFront || m_fAttackRear || m_fAttackLeft || m_fAttackRight) )
		return 1;

	int r, g, b;
	int x, y, a, shade;

	// TODO:  get the shift value of the health
	a = 255;	// max brightness until then

	float fFade = gHUD.m_flTimeDelta * 2;

	// SPR_Draw top
	if( m_fAttackFront > 0.4f )
	{
		GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackFront, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 - SPR_Width( m_hSprite, 0 ) / 2;
		y = ScreenHeight / 2 - SPR_Height( m_hSprite, 0 ) * 3;
		SPR_DrawAdditive( 0, x, y, NULL );
		m_fAttackFront = Q_max( 0, m_fAttackFront - fFade );
	} else
		m_fAttackFront = 0;

	if( m_fAttackRight > 0.4f )
	{
		GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackRight, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 + SPR_Width( m_hSprite, 1 ) * 2;
		y = ScreenHeight / 2 - SPR_Height( m_hSprite,1 ) / 2;
		SPR_DrawAdditive( 1, x, y, NULL );
		m_fAttackRight = Q_max( 0, m_fAttackRight - fFade );
	}
	else
		m_fAttackRight = 0;

	if( m_fAttackRear > 0.4f )
	{
		GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackRear, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 - SPR_Width( m_hSprite, 2 ) / 2;
		y = ScreenHeight / 2 + SPR_Height( m_hSprite, 2 ) * 2;
		SPR_DrawAdditive( 2, x, y, NULL );
		m_fAttackRear = Q_max( 0, m_fAttackRear - fFade );
	}
	else
		m_fAttackRear = 0;

	if( m_fAttackLeft > 0.4f )
	{
		GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackLeft, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 - SPR_Width( m_hSprite, 3 ) * 3;
		y = ScreenHeight / 2 - SPR_Height( m_hSprite,3 ) / 2;
		SPR_DrawAdditive( 3, x, y, NULL );

		m_fAttackLeft = Q_max( 0, m_fAttackLeft - fFade );
	} else
		m_fAttackLeft = 0;

	return 1;
}

int CHudHealth::DrawDamage( float flTime )
{
	int i, r, g, b, a;
	DAMAGE_IMAGE *pdmg;

	if( !m_bitsDamage )
		return 1;

	UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

	a = (int)( fabs( sin( flTime * 2.0f ) ) * 256.0f );

	ScaleColors( r, g, b, a );

	// check for bits that should be expired
	for( i = 0; i < NUM_DMG_TYPES; i++ )
	{
		if( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg = &m_dmg[i];

			// Draw all the items
			SPR_Set( gHUD.GetSprite( m_HUD_dmg_bio + i ), r, g, b );
			SPR_DrawAdditive( 0, pdmg->x, pdmg->y, &gHUD.GetSpriteRect( m_HUD_dmg_bio + i ) );

			pdmg->fExpire = Q_min( flTime + DMG_IMAGE_LIFE, pdmg->fExpire );

			if( pdmg->fExpire <= flTime		// when the time has expired
				&& a < 40 )						// and the flash is at the low point of the cycle
			{
				pdmg->fExpire = 0;

				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;

				// move everyone above down
				for( int j = 0; j < NUM_DMG_TYPES; j++ )
				{
					pdmg = &m_dmg[j];
					if( ( pdmg->y ) && ( pdmg->y < y ) )
						pdmg->y += giDmgHeight;

				}

				m_bitsDamage &= ~giDmgFlags[i];  // clear the bits
			}
		}
	}

	return 1;
}

void CHudHealth::UpdateTiles( float flTime, long bitsDamage )
{	
	DAMAGE_IMAGE *pdmg;

	// Which types are new?
	long bitsOn = ~m_bitsDamage & bitsDamage;

	for( int i = 0; i < NUM_DMG_TYPES; i++ )
	{
		pdmg = &m_dmg[i];

		// Is this one already on?
		if( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE; // extend the duration
			if( !pdmg->fBaseline )
				pdmg->fBaseline = flTime;
		}

		// Are we just turning it on?
		if( bitsOn & giDmgFlags[i] )
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth / 8;
			pdmg->y = ScreenHeight - giDmgHeight * 2;
			pdmg->fExpire=flTime + DMG_IMAGE_LIFE;

			// move everyone else up
			for( int j = 0; j < NUM_DMG_TYPES; j++ )
			{
				if( j == i )
					continue;

				pdmg = &m_dmg[j];
				if( pdmg->y )
					pdmg->y -= giDmgHeight;
			}
			// pdmg = &m_dmg[i];
		}
	}

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bitsDamage;
}
