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
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//
#include <stdio.h>

#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
#include "parsemsg.h"

#include "build.h"

#include "VGUI_Panel.h"
#include "VGUI_App.h"

using namespace vgui;

#if USE_IMGUI
#include "imgui_manager.h"
#include "imgui_viewport.h"
int g_ImGuiMouse = 0;
#if XASH_MOBILE_PLATFORM || XASH_64BIT
#include "gl_export.h"
#include "render_api.h"
render_api_t gRenderAPI;
#endif
#endif


extern "C"
{
#include "pm_shared.h"
}

#include <string.h>

cl_enginefunc_t gEngfuncs;
CHud gHUD;
mobile_engfuncs_t *gMobileEngfuncs = NULL;

void InitInput( void );
void EV_HookEvents( void );
void IN_Commands( void );

/*
========================== 
    Initialize

Called when the DLL is first loaded.
==========================
*/
extern "C" 
{
int		DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion );
int		DLLEXPORT HUD_VidInit( void );
void	DLLEXPORT HUD_Init( void );
int		DLLEXPORT HUD_Redraw( float flTime, int intermission );
int		DLLEXPORT HUD_UpdateClientData( client_data_t *cdata, float flTime );
void	DLLEXPORT HUD_Reset ( void );
void	DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server );
void	DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove );
char	DLLEXPORT HUD_PlayerMoveTexture( char *name );
int		DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
int		DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void	DLLEXPORT HUD_Frame( double time );
void	DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking);
void	DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf );
int		DLLEXPORT HUD_MobilityInterface( mobile_engfuncs_t *gpMobileEngfuncs );
#if USE_IMGUI && (XASH_MOBILE_PLATFORM || XASH_64BIT)
int 	DLLEXPORT HUD_GetRenderInterface( int version, render_api_t *renderfuncs, render_interface_t *callback );
#endif
#if USE_IMGUI && XASH_MOBILE_PLATFORM
int		DLLEXPORT IN_ClientTouchEvent ( int fingerID, float x, float y, float dx, float dy );
#endif
}

/*
================================
HUD_GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;

	switch( hullnumber )
	{
	case 0:				// Normal player
		Vector( -16, -16, -36 ).CopyToArray(mins);
		Vector( 16, 16, 36 ).CopyToArray(maxs);
		iret = 1;
		break;
	case 1:				// Crouched player
		Vector( -16, -16, -18 ).CopyToArray(mins);
		Vector( 16, 16, 18 ).CopyToArray(maxs);
		iret = 1;
		break;
	case 2:				// Point based hull
		Vector( 0, 0, 0 ).CopyToArray(mins);
		Vector( 0, 0, 0 ).CopyToArray(maxs);
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
HUD_ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	// Parse stuff from args
	// int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

void DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove )
{
	PM_Init( ppmove );
}

char DLLEXPORT HUD_PlayerMoveTexture( char *name )
{
	return PM_FindTextureType( name );
}

void DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server )
{
	PM_Move( ppmove, server );
}

int DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion )
{
	gEngfuncs = *pEnginefuncs;

	if( iVersion != CLDLL_INTERFACE_VERSION )
		return 0;

	// for now filterstuffcmd is last in the engine interface
	memcpy( &gEngfuncs, pEnginefuncs, sizeof(cl_enginefunc_t) - sizeof( void * ) );


	ConsolePrint( "\n\nAxion\n" );
	ConsolePrint( "GitHub page: https://github.com/Elinsrc/Axion/\n\n" );
	char filePath[256];
	sprintf(filePath, "%s/customconfig.cfg", gEngfuncs.pfnGetGameDirectory());

	FILE* file = fopen(filePath, "r");
	if (file)
	{
		fclose(file);
		ClientCmd("exec customconfig.cfg");
	}

	if( gEngfuncs.pfnGetCvarPointer( "cl_filterstuffcmd" ) == 0 )
	{
		gEngfuncs.pfnFilteredClientCmd = gEngfuncs.pfnClientCmd;
	}
	else
	{
		gEngfuncs.pfnFilteredClientCmd = pEnginefuncs->pfnFilteredClientCmd;
	}

	EV_HookEvents();

	return 1;
}

/*
=================
HUD_GetRect

VGui stub
=================
*/
int *HUD_GetRect( void )
{
	static int extent[4];

	extent[0] = gEngfuncs.GetWindowCenterX() - ScreenWidth / 2;
	extent[1] = gEngfuncs.GetWindowCenterY() - ScreenHeight / 2;
	extent[2] = gEngfuncs.GetWindowCenterX() + ScreenWidth / 2;
	extent[3] = gEngfuncs.GetWindowCenterY() + ScreenHeight / 2;

	return extent;
}

class TeamFortressViewport : public Panel
{
public:
	TeamFortressViewport(int x,int y,int wide,int tall);
	void Initialize( void );

	virtual void paintBackground();
	void *operator new( size_t stAllocateBlock );
};

static TeamFortressViewport* gViewPort = NULL;

TeamFortressViewport::TeamFortressViewport(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	gViewPort = this;
	Initialize();
}

void TeamFortressViewport::Initialize()
{
#if USE_IMGUI
	App::getInstance()->setCursorOveride( App::getInstance()->getScheme()->getCursor(Scheme::scu_none) );
#endif
}

void TeamFortressViewport::paintBackground()
{
//	int wide, tall;
//	getParent()->getSize( wide, tall );
//	setSize( wide, tall );
	int extents[4];
	getParent()->getAbsExtents(extents[0],extents[1],extents[2],extents[3]);
	gEngfuncs.VGui_ViewportPaintBackground(extents);
}

void *TeamFortressViewport::operator new( size_t stAllocateBlock )
{
	void *mem = ::operator new( stAllocateBlock );
	memset( mem, 0, stAllocateBlock );
	return mem;
}

/*
==========================
	HUD_VidInit

Called when the game initializes
and whenever the vid_mode is changed
so the HUD can reinitialize itself.
==========================
*/

int DLLEXPORT HUD_VidInit( void )
{
	gHUD.VidInit();
	Panel* root=(vgui::Panel*)gEngfuncs.VGui_GetPanel();
	if (root) {
		gEngfuncs.Con_Printf( "Root VGUI panel exists\n" );
		root->setBgColor(128,128,0,0);

		if (gViewPort != NULL)
		{
			gViewPort->Initialize();
		}
		else
		{
			gViewPort = new TeamFortressViewport(0,0,root->getWide(),root->getTall());
			gViewPort->setParent(root);
		}
	} else {
		gEngfuncs.Con_Printf( "Root VGUI panel does not exist\n" );
	}

#if USE_IMGUI
	g_ImGuiManager.VidInitialize();
#endif
	return 1;
}

/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/

void DLLEXPORT HUD_Init( void )
{
	InitInput();
	gHUD.Init();

#if USE_IMGUI
	g_ImGuiViewport.Initialize();
#if XASH_MOBILE_PLATFORM || XASH_64BIT
	GL_Init();
#else
	g_ImGuiManager.Initialize();
#endif
#endif

}

/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/

int DLLEXPORT HUD_Redraw( float time, int intermission )
{
	gHUD.Redraw( time, intermission );

#if USE_IMGUI
	g_ImGuiManager.NewFrame();
#endif

	return 1;
}

/*
==========================
	HUD_UpdateClientData

called every time shared client
dll/engine data gets changed,
and gives the cdll a chance
to modify the data.

returns 1 if anything has been changed, 0 otherwise.
==========================
*/

int DLLEXPORT HUD_UpdateClientData( client_data_t *pcldata, float flTime )
{
	IN_Commands();

	return gHUD.UpdateClientData( pcldata, flTime );
}

/*
==========================
	HUD_Reset

Called at start and end of demos to restore to "non"HUD state.
==========================
*/

void DLLEXPORT HUD_Reset( void )
{
	gHUD.VidInit();
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/

void DLLEXPORT HUD_Frame( double time )
{
	GetClientVoiceMgr()->Frame(time);

	if (!gViewPort)
		gEngfuncs.VGui_ViewportPaintBackground(HUD_GetRect());
}

/*
==========================
HUD_VoiceStatus

Called when a player starts or stops talking.
==========================
*/

void DLLEXPORT HUD_VoiceStatus( int entindex, qboolean bTalking )
{
	GetClientVoiceMgr()->UpdateSpeakerStatus(entindex, bTalking);
}

/*
==========================
HUD_DirectorEvent

Called when a director event message was received
==========================
*/

void DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf )
{
	 gHUD.m_Spectator.DirectorMessage( iSize, pbuf );
}

int DLLEXPORT HUD_MobilityInterface( mobile_engfuncs_t *gpMobileEngfuncs )
{
	if( gpMobileEngfuncs->version != MOBILITY_API_VERSION )
		return 1;

	gMobileEngfuncs = gpMobileEngfuncs;

	return 0;
}

bool HUD_MessageBox( const char *msg )
{
	gEngfuncs.Con_Printf( msg ); // just in case

	if( IsXashFWGS() )
	{
		gMobileEngfuncs->pfnSys_Warn( msg );
		return true;
	}

	// TODO: Load SDL2 and call ShowSimpleMessageBox

	return false;
}

bool IsXashFWGS()
{
	return gMobileEngfuncs != NULL;
}

/*
==========================
HUD_GetRenderInterface

Called when Xash3D sends render api to us
==========================
*/
#if USE_IMGUI && (XASH_MOBILE_PLATFORM || XASH_64BIT)
int DLLEXPORT HUD_GetRenderInterface( int version, render_api_t *renderfuncs, render_interface_t *callback )
{
	if( version != CL_RENDER_INTERFACE_VERSION )
	{
		return false;
	}

	gRenderAPI = *renderfuncs;

	// we didn't send callbacks to engine, because we don't use it
	// *callback = renderInterface;

	return true;
}
#endif

#if USE_IMGUI && XASH_MOBILE_PLATFORM
int DLLEXPORT IN_ClientTouchEvent(int fingerID, float x, float y, float dx, float dy)
{
	g_ImGuiManager.TouchEvent(fingerID, x, y, dx, dy);
	return g_ImGuiManager.IsCursorRequired() ? 1 : 0;
}
#endif
