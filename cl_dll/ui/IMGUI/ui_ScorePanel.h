#ifndef IMGUI_SCOREBOARD_H
#define IMGUI_SCOREBOARD_H

#include "imgui_window.h"
#include "imgui_utils.h"
#include "hud.h"
#include "cl_util.h"

#define MAX_SCOREBOARD_TEAMS 5
#define NUM_ROWS (MAX_PLAYERS + (MAX_SCOREBOARD_TEAMS * 2))
#define TEAM_NO 0
#define TEAM_YES 1
#define TEAM_SPECTATORS 2
#define TEAM_BLANK 3


class CImGuiScoreboard : public IImGuiWindow
{
public:
    static bool m_ShowWindow;

    void Initialize();
    void VidInitialize();
    void Terminate();
    void Think();

    void DrawScoreboard();
    void Draw();
    bool Active();
    bool CursorRequired();
    bool HandleKey(bool keyDown, int keyNumber, const char *bindName);

    void SortTeams();
    void SortPlayers(int iTeam, char *team);
    void RebuildTeams();
    void DeathMsg( int killer, int victim );

    int m_iRows;
    int m_iNumTeams;

    int m_iSortedRows[NUM_ROWS];
    int m_iIsATeam[NUM_ROWS];
    bool m_bHasBeenSorted[MAX_PLAYERS];

    int m_iPlayerNum;
    int m_iLastKilledBy;
    int m_fLastKillTime;

private:
    typedef struct
    {
        HSPRITE spr;
        wrect_t rc;
    }icon_flagstatus_t;

    icon_flagstatus_t m_IconFlagScore;
};

extern CImGuiScoreboard m_iScoreboard;

#endif // IMGUI_SCOREBOARD_H
