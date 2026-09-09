#pragma once

#include "imgui.h"
#include "imgui_utils.h"

class CImGuiSpectatorPanel
{
public:
    void Init();
    void Draw();

    bool isVisible() const { return m_bVisible; }
    void setVisible( bool vis ) { m_bVisible = vis; }

    void setBottomText( const char *text );
    void setCamMode( const char *text );
    void setTargetColor( float r, float g, float b );

    bool m_bVisible;
    char m_szTargetName[128];
    char m_szCamMode[64];
    ImVec4 m_TargetColor;

private:
    void DrawPlayerCard();
};

const char *GetSpectatorLabel( int iMode );

extern CImGuiSpectatorPanel m_iSpectatorPanel;