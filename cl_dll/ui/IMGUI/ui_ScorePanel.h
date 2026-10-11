#ifndef IMGUI_SCOREBOARD_H
#define IMGUI_SCOREBOARD_H

#include "imgui_window.h"
#include "imgui_utils.h"
#include "hud.h"
#include "cl_util.h"
#include "custom_utils.h"

#define MAX_SCOREBOARD_TEAMS 5
#define NUM_ROWS (MAX_PLAYERS + (MAX_SCOREBOARD_TEAMS * 2))
#define TEAM_NO 0
#define TEAM_YES 1
#define TEAM_SPECTATORS 2
#define TEAM_BLANK 3

class CImGuiScoreboard : public IImGuiWindow
{
    CustomUtils m_CustomUtils;
public:
    static bool m_ShowScore;
    static bool isVisible() { return m_ShowScore; }

    void Initialize();
    void InitHUDData();
    void VidInitialize();
    void Terminate();
    void Think();

    void Update();
    void DrawScoreboard();
    void Draw();
    bool Active();
    bool CursorRequired();
    bool HandleKey(bool keyDown, int keyNumber, const char *bindName);

    void SortTeams();
    void SortPlayers(int iTeam, char *team);
    void RebuildTeams();
    void DeathMsg( int killer, int victim );

    void UpdateMobileButtons(bool show);
    void SelectPlayerStep(int dir);
    void ToggleMuteSelected();
    void OnMuteButton();

    int m_iRows;
    int m_iNumTeams;

    int m_iSortedRows[NUM_ROWS];
    int m_iIsATeam[NUM_ROWS];
    bool m_bHasBeenSorted[MAX_PLAYERS];

    int m_iPlayerNum;
    int m_iLastKilledBy;
    int m_fLastKillTime;

private:
    void LayoutMobileButtons(float sbX, float sbY, float sbW, float sbH, float scrW, float scrH, float uiScale);
    bool m_bBtnLayoutValid = false;
    float m_fBtnX = 0.f, m_fBtnY = 0.f, m_fBtnSize = 0.f;

    typedef struct
    {
        HLSPRITE spr;
        wrect_t rc;
    }icon_flagstatus_t;

    icon_flagstatus_t m_IconFlagScore;

    bool m_bMouseMode;

    int m_iSelectedPlayer = 0;
    bool m_bShowPlayerMenu = false;
    bool m_bScrollToSelected = false;
    bool m_bButtonsShown = false;
};

extern CImGuiScoreboard g_iScoreboard;

#endif // IMGUI_SCOREBOARD_H