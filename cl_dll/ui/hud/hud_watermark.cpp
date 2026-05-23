// OpenAG
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "build_info.h"

#if !XASH_ANDROID
#include "update_checker.h"
#endif

int CHudWatermark::Init()
{
	m_iFlags = 0;

	hud_watermark = CVAR_CREATE("hud_watermark", "1", FCVAR_ARCHIVE);

	gHUD.AddHudElem(this);
	return 0;
}

int CHudWatermark::VidInit()
{
	m_iFlags |= HUD_ACTIVE;
	refresh_draw_until = true;

	return 1;
}

int CHudWatermark::Draw(float time)
{
	if (hud_watermark->value == 0.0f)
		return 0;

	if (refresh_draw_until || (draw_until > gHUD.m_flTime + 15.0f)) {
		refresh_draw_until = false;
		draw_until = gHUD.m_flTime + 15.0f;
	}

	if (gHUD.m_flTime >= draw_until) {
		m_iFlags &= ~HUD_ACTIVE;
		return 0;
	}

	int r, g, b;
	UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);

	char str[256];
	sprintf(str, "^2Axion^7: built ^1%s^7, commit ^2%s^7, architecture ^3%s^7, platform ^5%s\n",
		BuildInfo::GetDate(),
		BuildInfo::GetCommitHash(),
		BuildInfo::GetArchitecture(),
		BuildInfo::GetPlatform()
	);
	gHUD.DrawHudStringWithColorTags(ScreenWidth / 20, gHUD.m_scrinfo.iCharHeight, str, r, g, b);
	gHUD.DrawHudStringWithColorTags(ScreenWidth / 20, gHUD.m_scrinfo.iCharHeight * 2, "t.me/Elinsrc", r, g, b);
	gHUD.DrawHudStringWithColorTags(ScreenWidth / 20, gHUD.m_scrinfo.iCharHeight * 3, "t.me/HalfLifeCollectiveChat", r, g, b);
	gHUD.DrawHudStringWithColorTags(ScreenWidth / 20, gHUD.m_scrinfo.iCharHeight * 4, BuildInfo::GetGitHubLink(), r, g, b);
	sprintf(str, "To disable this message, type in the console ^2hud_watermark 0");
	gHUD.DrawHudStringWithColorTags(ScreenWidth / 20, gHUD.m_scrinfo.iCharHeight * 6, str, r, g, b);

#if !XASH_ANDROID
    if (g_pUpdateChecker && g_pUpdateChecker->IsFinished() && g_pUpdateChecker->HasUpdate())
    {
        gHUD.DrawHudStringWithColorTags(ScreenWidth / 20, gHUD.m_scrinfo.iCharHeight * 8, "^2Axion^7: ^1New update available!", r, g, b);
		sprintf(str, "Commit: %s\n", g_pUpdateChecker->GetRemoteHash().c_str());
		gHUD.DrawHudStringWithColorTags(ScreenWidth / 20, gHUD.m_scrinfo.iCharHeight * 9, str, r, g, b);
		sprintf(str, "%s\n", g_pUpdateChecker->GetCommitMessage().c_str());
		gHUD.DrawHudStringWithColorTags(ScreenWidth / 20, gHUD.m_scrinfo.iCharHeight * 10, str, r, g, b);
    }
#endif

	return 0;
}
