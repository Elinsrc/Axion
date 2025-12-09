#ifndef IMGUI_MOTD_H
#define IMGUI_MOTD_H

#include "imgui_window.h"

class CImGuiMOTD : public IImGuiWindow
{
public:
    void Initialize();
    void VidInitialize();
    void Terminate();
    void Think();
    void Draw();
    bool Active();
    bool CursorRequired();
    bool HandleKey(bool keyDown, int keyNumber, const char *bindName);

    static bool m_ShowMOTD;
};

extern CImGuiMOTD m_iMOTD;
#endif // IMGUI_MOTD_H
