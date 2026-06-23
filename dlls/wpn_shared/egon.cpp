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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"

#define	EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"

#define EGON_SWITCH_NARROW_TIME			0.75f			// Time it takes to switch fire modes
#define EGON_SWITCH_WIDE_TIME			1.5f

enum egon_e {
	EGON_IDLE1 = 0,
	EGON_FIDGET1,
	EGON_ALTFIREON,
	EGON_ALTFIRECYCLE,
	EGON_ALTFIREOFF,
	EGON_FIRE1,
	EGON_FIRE2,
	EGON_FIRE3,
	EGON_FIRE4,
	EGON_DRAW,
	EGON_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_egon, CEgon )

void CEgon::Spawn()
{
	Precache();
	m_iId = WEAPON_EGON;
	SET_MODEL( ENT( pev ), "models/w_egon.mdl" );

	m_iDefaultAmmo = EGON_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CEgon::Precache( void )
{
	PRECACHE_MODEL( "models/w_egon.mdl" );
	PRECACHE_MODEL( "models/v_egon.mdl" );
	PRECACHE_MODEL( "models/p_egon.mdl" );

	PRECACHE_MODEL( "models/w_9mmclip.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( EGON_SOUND_OFF );
	PRECACHE_SOUND( EGON_SOUND_RUN );
	PRECACHE_SOUND( EGON_SOUND_STARTUP );

	PRECACHE_MODEL( EGON_BEAM_SPRITE );
	PRECACHE_MODEL( EGON_FLARE_SPRITE );

	PRECACHE_SOUND( "weapons/357_cock1.wav" );

	m_usEgonFire = PRECACHE_EVENT( 1, "events/egon_fire.sc" );
	m_usEgonStop = PRECACHE_EVENT( 1, "events/egon_stop.sc" );
}

BOOL CEgon::Deploy( void )
{
	m_deployed = FALSE;
	m_fireState = FIRE_OFF;
	return DefaultDeploy( "models/v_egon.mdl", "models/p_egon.mdl", EGON_DRAW, "egon" );
}

int CEgon::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CEgon::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( EGON_HOLSTER );

	EndAttack();
}

int CEgon::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_EGON;
	p->iFlags = 0;
	p->iWeight = EGON_WEIGHT;

	return 1;
}

#define EGON_PULSE_INTERVAL		0.1
#define EGON_DISCHARGE_INTERVAL		0.1

float CEgon::GetPulseInterval( void )
{
	return EGON_PULSE_INTERVAL;
}

float CEgon::GetDischargeInterval( void )
{
	return EGON_DISCHARGE_INTERVAL;
}

BOOL CEgon::HasAmmo( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return FALSE;

	return TRUE;
}

void CEgon::UseAmmo( int count )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= count )
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= count;
	else
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
}

void CEgon::Attack( void )
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		if( m_fireState != FIRE_OFF || m_pBeam )
		{
			EndAttack();
		}
		else
		{
			PlayEmptySound();
		}
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition();

	int flags = FEV_NOTHOST;

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if( !HasAmmo() )
			{
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25f;
				PlayEmptySound( );
				return;
			}

			m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.

			PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usEgonFire, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, m_fireMode, 1, 0 );
		
			m_shakeTime = 0;

			m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;
			pev->fuser1 = UTIL_WeaponTimeBase() + 2.0f;

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
			m_fireState = FIRE_CHARGE;
			break;
		}
		case FIRE_CHARGE:
		{
			Fire( vecSrc, vecAiming );
			m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;

			if( pev->fuser1 <= UTIL_WeaponTimeBase() )
			{
				PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usEgonFire, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, m_fireMode, 0, 0 );
				pev->fuser1 = 1000;
			}

			if( !HasAmmo() )
			{
				EndAttack();
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
			}
			break;
		}
	}
}

void CEgon::PrimaryAttack( void )
{
	m_fireMode = FIRE_WIDE;
	Attack();
}

void CEgon::Fire( const Vector &vecOrigSrc, const Vector &vecDir )
{
	Vector vecDest = vecOrigSrc + vecDir * 2048.0f;
	edict_t *pentIgnore;
	TraceResult tr;

	pentIgnore = m_pPlayer->edict();
	Vector tmpSrc = vecOrigSrc + gpGlobals->v_up * -8.0f + gpGlobals->v_right * 3.0f;

	// ALERT( at_console, "." );
	
	UTIL_TraceLine( vecOrigSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

	if( tr.fAllSolid )
		return;

	float timedist = 0.0f;

	switch( m_fireMode )
	{
	case FIRE_NARROW:
		timedist = ( pev->dmgtime - gpGlobals->time ) / GetPulseInterval();
		break;
	case FIRE_WIDE:
		timedist = ( pev->dmgtime - gpGlobals->time ) / GetDischargeInterval();
		break;
	}

	if( timedist < 0 )
		timedist = 0;
	else if( timedist > 1 )
		timedist = 1;
	timedist = 1 - timedist;

	UpdateEffect( tmpSrc, tr.vecEndPos, timedist );
}

void CEgon::UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend )
{
}

void CEgon::CreateEffect( void )
{
}

void CEgon::DestroyEffect( void )
{
}

void CEgon::WeaponIdle( void )
{
	ResetEmptySound();

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if( m_fireState != FIRE_OFF )
		 EndAttack();

	int iAnim;

	float flRand = RANDOM_FLOAT( 0.0f, 1.0f );

	if( flRand <= 0.5f )
	{
		iAnim = EGON_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
	}
	else 
	{
		iAnim = EGON_FIDGET1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	}

	SendWeaponAnim( iAnim );
	m_deployed = TRUE;
}

void CEgon::EndAttack( void )
{
	bool bMakeNoise = false;

	if( m_fireState != FIRE_OFF ) //Checking the button just in case!.
		 bMakeNoise = true;

	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, m_pPlayer->edict(), m_usEgonStop, 0.0f, m_pPlayer->pev->origin, m_pPlayer->pev->angles, 0.0f, 0.0f, bMakeNoise, 0, 0, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;

	m_fireState = FIRE_OFF;

	DestroyEffect();
}

class CEgonAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_chainammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_chainammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if( pOther->GiveAmmo( AMMO_URANIUMBOX_GIVE, "uranium", URANIUM_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_egonclip, CEgonAmmo )
