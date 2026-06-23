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

    void Show( const char *text, const char *title );

    static bool m_ShowMOTD;
private:
    char m_szText[4096];
    char m_szTitle[64];
};

extern CImGuiMOTD g_iMOTD;
#endif // IMGUI_MOTD_H
