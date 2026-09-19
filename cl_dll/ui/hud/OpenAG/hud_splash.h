//++ BulliT
#pragma once

class CHudSplash: public CHudBase
{
public:
    int Init( void );
    int VidInit( void );
    int Draw(float flTime);
    void Reset(void);
    int MsgFunc_Splash(const char *pszName, int iSize, void *pbuf);

private:
    float m_flTurnoff;
    HLSPRITE m_hSprite;
};

//-- Martin Webrant
