//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"

DECLARE_MESSAGE(m_Longjump, Longjump )

int CHudLongjump::Init(void)
{
    HOOK_MESSAGE( Longjump );

    m_flTurnoff = 0;
    m_iFlags = 0;

    gHUD.AddHudElem(this);

    return 1;
}

int CHudLongjump::VidInit(void)
{
    return 1;
}

void CHudLongjump::Reset(void)
{
    m_iFlags &= ~HUD_ACTIVE;
}

int CHudLongjump::Draw(float fTime)
{
    if ( m_flTurnoff < gHUD.m_flTime || gHUD.m_iIntermission )
    {
        Reset();
        return 1;
    }

    char szText[32];

    int r, g, b, a;
    UnpackRGB(r, g, b, RGB_GREENISH);
    a = 255 * gHUD.GetHudTransparency();
    ScaleColors(r, g, b, a);

    sprintf(szText,"Longjump %d", (int)(m_flTurnoff - gHUD.m_flTime));
    gHUD.DrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight * 2, szText, r, g, b );

    return 0;
}


int CHudLongjump::MsgFunc_Longjump(const char *pszName,  int iSize, void *pbuf)
{
    BEGIN_READ( pbuf, iSize );
    int iTime = READ_BYTE();

    m_flTurnoff = gHUD.m_flTime + iTime;
    m_iFlags |= HUD_ACTIVE;

    return 1;
}

//-- Martin Webrant
