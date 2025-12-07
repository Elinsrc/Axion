#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE(m_NextMap, Nextmap);

int CHudNextMap::Init()
{
	HOOK_MESSAGE(Nextmap);

	m_iFlags = 0;
	m_szNextmap[0] = '\0';
	m_flTurnoff = 0;

	gHUD.AddHudElem(this);
	return 0;
}

int CHudNextMap::VidInit()
{
	m_iFlags &= ~HUD_ACTIVE;

	return 1;
}

int CHudNextMap::Draw(float time)
{
	if (gHUD.m_flTime >= m_flTurnoff) {
		m_iFlags &= ~HUD_ACTIVE;
		return 0;
	}

	int r, g, b, a;
	a = 255 * gHUD.GetHudTransparency();
	UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);
	ScaleColors(r, g, b, a);

	char szText[32];
	sprintf(szText, "Nextmap is %s", m_szNextmap);

	gHUD.DrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight * 5, szText, r, g, b);

	return 0;
}

int CHudNextMap::MsgFunc_Nextmap(const char* name, int size, void* buf)
{
	BEGIN_READ(buf, size);
	strcpy(m_szNextmap, READ_STRING());

	gHUD.m_Timer.SetNextmap(m_szNextmap);

	const int hud_nextmap = (int)gHUD.m_Timer.GetHudNextmap();
	if (hud_nextmap != 2 && hud_nextmap != 1)
	{
		m_flTurnoff = gHUD.m_flTime + 10.0f;
		m_iFlags |= HUD_ACTIVE;
	}

	return 1;
}
