#include "imgui_window_system.h"
#include "ui_demo_window.h"
#include "ui_commands.h"
#include "ui_ScorePanel.h"
#include "ui_MOTD.h"

void CImGuiWindowSystem::LinkWindows()
{
    static CImGuiDemoWindow demoWindow;
    AddWindow(&demoWindow);

    static CImGuiCommands commandsWindow;
    AddWindow(&commandsWindow);

    AddWindow(&m_iScoreboard);

    static CImGuiMOTD motdWindow;
    AddWindow(&motdWindow);
}
