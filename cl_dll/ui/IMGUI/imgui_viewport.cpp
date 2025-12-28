#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "imgui_viewport.h"
#include <string.h>

CImGuiViewport &g_ImGuiViewport = CImGuiViewport::GetInstance();

CImGuiViewport &CImGuiViewport::GetInstance()
{
    static CImGuiViewport instance;
    return instance;
}

CImGuiViewport::CImGuiViewport()
{
}

CImGuiViewport::~CImGuiViewport()
{
}

void CImGuiViewport::Initialize()
{
    m_iGotAllMOTD = true;
    m_szServerName[0] = '\0';
}

void CImGuiViewport::ShowScoreBoard()
{
    if( gEngfuncs.GetMaxClients() > 1 )
        g_iScoreboard.m_ShowScore = true;
}

bool CImGuiViewport::IsScoreBoardVisible()
{
    return g_iScoreboard.isVisible();
}

void CImGuiViewport::HideScoreBoard()
{
    if( gHUD.m_iIntermission )
        return;

    g_iScoreboard.m_ShowScore = false;
}

void CImGuiViewport::GetAllPlayersInfo()
{
    for( int i = 1; i < MAX_PLAYERS; i++ )
    {
        GetPlayerInfo( i, &g_PlayerInfoList[i] );

        if( g_PlayerInfoList[i].thisplayer )
            g_iScoreboard.m_iPlayerNum = i;  // !!!HACK: this should be initialized elsewhere... maybe gotten from the engine
    }
}

int CImGuiViewport::MsgFunc_ValClass( const char *pszName, int iSize, void *pbuf )
{
    return 1;
}

int CImGuiViewport::MsgFunc_TeamNames( const char *pszName, int iSize, void *pbuf )
{
    return 1;
}

int CImGuiViewport::MsgFunc_Feign( const char *pszName, int iSize, void *pbuf )
{
    return 1;
}

int CImGuiViewport::MsgFunc_Detpack( const char *pszName, int iSize, void *pbuf )
{
    return 1;
}

int CImGuiViewport::MsgFunc_IMGUIMenu( const char *pszName, int iSize, void *pbuf )
{
    return 1;
}

int CImGuiViewport::MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf )
{
    if( m_iGotAllMOTD )
        m_szMOTD[0] = 0;

    BEGIN_READ( pbuf, iSize );

    m_iGotAllMOTD = READ_BYTE();

    int roomInArray = sizeof(m_szMOTD) - strlen( m_szMOTD ) - 1;

    strncat( m_szMOTD, READ_STRING(), roomInArray >= 0 ? roomInArray : 0 );
    m_szMOTD[sizeof(m_szMOTD) - 1] = '\0';

    // don't show MOTD for HLTV spectators
    if( m_iGotAllMOTD && !gEngfuncs.IsSpectateOnly() )
    {
        g_iMOTD.m_ShowMOTD = true;
    }

    return 1;
}

int CImGuiViewport::MsgFunc_BuildSt( const char *pszName, int iSize, void *pbuf )
{
    return 1;
}

int CImGuiViewport::MsgFunc_RandomPC( const char *pszName, int iSize, void *pbuf )
{
    return 1;
}

int CImGuiViewport::MsgFunc_ServerName( const char *pszName, int iSize, void *pbuf )
{
    BEGIN_READ( pbuf, iSize );

    strncpy( m_szServerName, READ_STRING(), sizeof(m_szServerName) - 1 );
    m_szServerName[sizeof(m_szServerName) - 1] = 0;

    return 1;
}

int CImGuiViewport::MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf )
{
    BEGIN_READ( pbuf, iSize );
    short cl = READ_BYTE();
    short frags = READ_SHORT();
    short deaths = READ_SHORT();
    short playerclass = READ_SHORT();
    short teamnumber = READ_SHORT();

    if( cl > 0 && cl <= MAX_PLAYERS )
    {
        g_PlayerExtraInfo[cl].frags = frags;
        g_PlayerExtraInfo[cl].deaths = deaths;
        g_PlayerExtraInfo[cl].playerclass = playerclass;
        g_PlayerExtraInfo[cl].teamnumber = teamnumber;

        //Dont go bellow 0!
        if( g_PlayerExtraInfo[cl].teamnumber < 0 )
            g_PlayerExtraInfo[cl].teamnumber = 0;
    }

    return 1;
}

int CImGuiViewport::MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf )
{
    BEGIN_READ( pbuf, iSize );
    char *TeamName = READ_STRING();

    int i;

    for( i = 1; i <= g_iScoreboard.m_iNumTeams; i++ )
    {
        if( !stricmp( TeamName, g_TeamInfo[i].name ) )
            break;
    }

    if( i > g_iScoreboard.m_iNumTeams )
        return 1;

    g_TeamInfo[i].scores_overriden = TRUE;
    g_TeamInfo[i].frags = READ_SHORT();
    g_TeamInfo[i].deaths = READ_SHORT();

    return 1;
}

int CImGuiViewport::MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf )
{
    BEGIN_READ( pbuf, iSize );
    short cl = READ_BYTE();

    if( cl > 0 && cl <= MAX_PLAYERS )
    {
        strncpy( g_PlayerExtraInfo[cl].teamname, READ_STRING(), MAX_TEAM_NAME - 1 );
        g_PlayerExtraInfo[cl].teamname[MAX_TEAM_NAME - 1] = '\0';
    }

    g_iScoreboard.RebuildTeams();

    return 1;
}

void CImGuiViewport::DeathMsg( int killer, int victim )
{
    g_iScoreboard.DeathMsg( killer, victim );
}

int CImGuiViewport::MsgFunc_Spectator( const char *pszName, int iSize, void *pbuf )
{
    BEGIN_READ( pbuf, iSize );

    short cl = READ_BYTE();
    if( cl > 0 && cl <= MAX_PLAYERS )
    {
        g_IsSpectator[cl] = READ_BYTE();
    }

    return 1;
}

int CImGuiViewport::MsgFunc_AllowSpec( const char *pszName, int iSize, void *pbuf )
{
    BEGIN_READ( pbuf, iSize );

    m_iAllowSpectators = READ_BYTE();

    return 1;
}

int CImGuiViewport::MsgFunc_ResetFade( const char *pszName, int iSize, void *pbuf )
{
    return 1;
}

int CImGuiViewport::MsgFunc_SpecFade( const char *pszName, int iSize, void *pbuf )
{
    return 1;
}
