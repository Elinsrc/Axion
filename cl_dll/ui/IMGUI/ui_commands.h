#include "imgui_window.h"
#include "imgui_utils.h"

class CImGuiCommands : public IImGuiWindow
{
    CImguiUtils m_CImguiUtils;
public:
    void Initialize();
    void VidInitialize();
    void Terminate();
    void Think();
    void Draw();
    bool Active();
    bool CursorRequired();
    bool HandleKey(bool keyDown, int keyNumber, const char *bindName);

private:
    static void CmdShowCommandsWindow();
    static bool m_ShowCommands;
};
