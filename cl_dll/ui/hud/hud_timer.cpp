/***
*
* Copyright (c) 2012, AGHL.RU. All rights reserved.
*
****/
//
// hud_timer.cpp
//
// implementation of CHudTimer class
//

#include <time.h>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "net_api.h"
#include "net.h"
#include "demo.h"
#include "demo_api.h"

#define NET_API gEngfuncs.pNetAPI

enum RulesRequestStatus
{
	SOCKET_NONE = 0,
	SOCKET_IDLE = 1,
	SOCKET_AWAITING_CODE = 2,
	SOCKET_AWAITING_ANSWER = 3,
};
RulesRequestStatus g_eRulesRequestStatus = SOCKET_NONE;
NetSocket g_timerSocket = 0;

DECLARE_MESSAGE(m_Timer, Timer);

int CHudTimer::Init()
{
	HOOK_MESSAGE(Timer);

	gHUD.AddHudElem(this);

	m_iFlags |= HUD_ACTIVE;

	m_pCvarHudTimer = CVAR_CREATE("hud_timer", "1", FCVAR_ARCHIVE);
	m_pCvarHudTimerSync = CVAR_CREATE("hud_timer_sync", "1", FCVAR_ARCHIVE);
	m_pCvarHudNextmap = CVAR_CREATE("hud_nextmap", "1", FCVAR_ARCHIVE);

	return 1;
};

int CHudTimer::VidInit()
{
	m_pCvarMpTimelimit = CVAR_GET_POINTER("mp_timelimit");
	m_pCvarMpTimeleft = CVAR_GET_POINTER("mp_timeleft");
	m_pCvarSvAgVersion = CVAR_GET_POINTER("sv_ag_version");
	m_pCvarAmxNextmap = CVAR_GET_POINTER("amx_nextmap");

	m_flDemoSyncTime = 0;
	m_bDemoSyncTimeValid = false;
	m_flNextSyncTime = 0;
	m_flSynced = false;
	m_flEndTime = 0;
	m_flEffectiveTime = 0;
	m_bDelayTimeleftReading = true;
	m_eAgVersion = SV_AG_UNKNOWN;

	m_bNeedWriteTimer = true;
	m_bNeedWriteNextmap = true;

	m_iReceivedSize = 0;
	if (g_timerSocket != NULL)
	{
		NetCloseSocket(g_timerSocket);
		g_timerSocket = NULL;
		g_eRulesRequestStatus = SOCKET_NONE;
	}

	return 1;
};

int CHudTimer::MsgFunc_Timer(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int timelimit = READ_LONG();
	int effectiveTime = READ_LONG();

	if (!m_flSynced)
	{
		m_flEndTime = timelimit;
		m_flEffectiveTime = effectiveTime;
	}

	return 1;
}

void CHudTimer::DoResync(void)
{
	m_bDelayTimeleftReading = true;
	m_flNextSyncTime = 0;
}

void CHudTimer::SyncTimer(float fTime)
{
	if (gEngfuncs.pDemoAPI->IsPlayingback()) return;

	if ((int)m_pCvarHudTimerSync->value == 0)
	{
		m_flSynced = false;
		return;
	}

	// Make sure networking system has started.
	NET_API->InitNetworking();
	// Get net status
	net_status_t status;
	NET_API->Status(&status);
	if (status.connected)
	{
		if (status.remote_address.type == NA_IP)
		{
			SyncTimerRemote(*(unsigned int*)status.remote_address.ip, status.remote_address.port, fTime, status.latency);
			if (g_eRulesRequestStatus == SOCKET_AWAITING_CODE || g_eRulesRequestStatus == SOCKET_AWAITING_ANSWER)
				return;
		}
		else if (status.remote_address.type == NA_LOOPBACK)
		{
			SyncTimerLocal(fTime);
			m_flNextSyncTime = fTime + 5;
		}
		else
		{
			m_flNextSyncTime = fTime + 1;
		}

		if (m_bDelayTimeleftReading)
		{
			m_bDelayTimeleftReading = false;
			// We are not synced via timeleft because it has a delay when server set it after mp_timelimit changed
			// So do an update soon
			m_flNextSyncTime = fTime + 1.5;
		}
	}
	else
	{
		// Close socket if we are not connected anymore
		if (g_timerSocket != NULL)
		{
			NetCloseSocket(g_timerSocket);
			g_timerSocket = NULL;
			g_eRulesRequestStatus = SOCKET_NONE;
		}

		m_flNextSyncTime = fTime + 1;
	}
};

void CHudTimer::SyncTimerLocal(float fTime)
{
	float prevEndtime = m_flEndTime;
	int prevAgVersion = m_eAgVersion;

	// Get timer settings directly from cvars
	if (m_pCvarMpTimelimit && m_pCvarMpTimeleft)
	{
		m_flEndTime = m_pCvarMpTimelimit->value * 60;
		if (!m_bDelayTimeleftReading)
		{
			float timeleft = m_pCvarMpTimeleft->value;
			if (timeleft > 0)
			{
				float endtime = timeleft + fTime;
				if (fabs(m_flEndTime - endtime) > 1.5)
					m_flEndTime = endtime;
			}
		}
		if (m_flEndTime != prevEndtime) m_bNeedWriteTimer = true;
	}

	// Get AG version
	if (m_eAgVersion == SV_AG_UNKNOWN)
	{
		if (m_pCvarSvAgVersion && m_pCvarSvAgVersion->string[0])
		{
			if (!strcmp(m_pCvarSvAgVersion->string, "6.6") || !strcmp(m_pCvarSvAgVersion->string, "6.3"))
			{
				m_eAgVersion = SV_AG_FULL;
			}
			else // We will assume its miniAG server, which will be true in almost all cases
			{
				m_eAgVersion = SV_AG_MINI;
			}
		}
		else
		{
			m_eAgVersion = SV_AG_NONE;
		}

		if (m_eAgVersion != prevAgVersion) m_bNeedWriteTimer = true;
	}

	// Get nextmap
	if (m_pCvarAmxNextmap && m_pCvarAmxNextmap->string[0])
	{
		if (strcmp(m_szNextmap, m_pCvarAmxNextmap->string))
		{
			m_bNeedWriteNextmap = true;
			strncpy(m_szNextmap, m_pCvarAmxNextmap->string, sizeof(m_szNextmap) - 1);
			m_szNextmap[sizeof(m_szNextmap) - 1] = 0;
		}
	}
}

void CHudTimer::SyncTimerRemote(unsigned int ip, unsigned short port, float fTime, double latency)
{
	float prevEndtime = m_flEndTime;
	int prevAgVersion = m_eAgVersion;
	char buffer[2048];
	int len = 0;

	// Check for query timeout and just do a resend
	if (fTime - m_flNextSyncTime > 3 && (g_eRulesRequestStatus == SOCKET_AWAITING_CODE || g_eRulesRequestStatus == SOCKET_AWAITING_ANSWER))
		g_eRulesRequestStatus = SOCKET_IDLE;

	// Retrieve settings from the server
	switch(g_eRulesRequestStatus)
	{
		case SOCKET_NONE:
		case SOCKET_IDLE:
			m_iResponceID = 0;
			m_iReceivedSize = 0;
			m_iReceivedPackets = 0;
			m_iReceivedPacketsCount = 0;
			NetClearSocket(g_timerSocket);
			NetSendUdp(ip, port, "\xFF\xFF\xFF\xFFV\xFF\xFF\xFF\xFF", 9, &g_timerSocket);
			g_eRulesRequestStatus = SOCKET_AWAITING_CODE;
			m_flNextSyncTime = fTime;	// set time for timeout checking
			return;
		case SOCKET_AWAITING_CODE:
			len = NetReceiveUdp(ip, port, buffer, sizeof(buffer), g_timerSocket);
			if (len < 5) return;
			if (*(int*)buffer == -1 /*0xFFFFFFFF*/ && buffer[4] == 'A' && len == 9)
			{
				// Answer is challenge response, send request again with the code
				buffer[4] = 'V';
				NetSendUdp(ip, port, buffer, 9, &g_timerSocket);
				g_eRulesRequestStatus = SOCKET_AWAITING_ANSWER;
				m_flNextSyncTime = fTime;	// set time for timeout checking
				return;
			}
			// Answer should be rules response
			g_eRulesRequestStatus = SOCKET_AWAITING_ANSWER;
			break;
		case SOCKET_AWAITING_ANSWER:
			len = NetReceiveUdp(ip, port, buffer, sizeof(buffer), g_timerSocket);
			if (len < 5) return;
			break;
	}

	// Check for split packet
	if (*(int*)buffer == -2 /*0xFEFFFFFF*/)
	{
		if (len < 9) return;
		int currentPacket = *((unsigned char *)buffer + 8) >> 4;
		int totalPackets = *((unsigned char *)buffer + 8) & 0x0F;
		if (currentPacket >= totalPackets)
			return;	// broken split packet
			if (m_iReceivedPackets == 0)
				m_iResponceID = *(int*)(buffer + 4);
		else if (*(int*)(buffer + 4) != m_iResponceID)
			return;	// packet is from different responce
			if (m_iReceivedPackets & (1 << currentPacket))
				return;	// already has this packet
				if (currentPacket < totalPackets - 1 && len != 1400)
					return;	// strange split packet
					// Copy into merge buffer
					int pos = (1400 - 9) * currentPacket;
				memcpy(m_szPacketBuffer + pos, buffer + 9, len - 9);
			m_iReceivedSize += len - 9;
		m_iReceivedPackets |= (1 << currentPacket);
		m_iReceivedPacketsCount++;
		// Check for completion
		if (m_iReceivedPacketsCount < totalPackets)
			return;
	}
	else if (*(int*)buffer == -1 /*0xFFFFFFFF*/)
	{
		memcpy(m_szPacketBuffer, buffer, len);
		m_iReceivedSize += len;
	}
	else
	{
		return;
	}

	// Check that this is actually rules responce
	if (*(int*)m_szPacketBuffer != -1 /*0xFFFFFFFF*/ || m_szPacketBuffer[4] != 'E')
	{
		return;
	}

	m_flSynced = true;
	g_eRulesRequestStatus = SOCKET_IDLE;
	m_flNextSyncTime = fTime + 10;	// Don't sync offten, we get update notifications via svc_print

	// Parse rules
	// Get map end time
	char *value = NetGetRuleValueFromBuffer(m_szPacketBuffer, m_iReceivedSize, "mp_timelimit");
	if (value && value[0])
	{
		m_flEndTime = atof(value) * 60;
	}
	else
	{
		m_flEndTime = 0;
	}
	value = NetGetRuleValueFromBuffer(m_szPacketBuffer, m_iReceivedSize, "mp_timeleft");
	if (value && value[0] && !gHUD.m_iIntermission && !m_bDelayTimeleftReading)
	{
		float timeleft = atof(value);
		if (timeleft > 0)
		{
			float endtime = timeleft + (int)(fTime - latency + 0.5);
			if (fabs(m_flEndTime - endtime) > 1.5)
				m_flEndTime = endtime;
		}
	}
	if (m_flEndTime != prevEndtime) m_bNeedWriteTimer = true;

	// Get AG version
	if (m_eAgVersion == SV_AG_UNKNOWN)
	{
		value = NetGetRuleValueFromBuffer(m_szPacketBuffer, m_iReceivedSize, "sv_ag_version");
		if (value && value[0])
		{
			if (!strcmp(value, "6.6") || !strcmp(value, "6.3"))
			{
				m_eAgVersion = SV_AG_FULL;
			}
			else // We will assume its miniAG server, which will be true in almost all cases
			{
				m_eAgVersion = SV_AG_MINI;
			}
		}
		else
		{
			m_eAgVersion = SV_AG_NONE;
		}

		if (m_eAgVersion != prevAgVersion) m_bNeedWriteTimer = true;
	}

	// Get nextmap
	value = NetGetRuleValueFromBuffer(m_szPacketBuffer, m_iReceivedSize, "amx_nextmap");
	if (value && value[0])
	{
		if (strcmp(m_szNextmap, value))
		{
			m_bNeedWriteNextmap = true;
			strncpy(m_szNextmap, value, sizeof(m_szNextmap) - 1);
			m_szNextmap[sizeof(m_szNextmap) - 1] = 0;
		}
	}
}

void CHudTimer::Think(void)
{
	float flTime = gEngfuncs.GetClientTime();

	// Check for time reset (can it happen?)
	if (m_flNextSyncTime - flTime > 60)
		m_flNextSyncTime = flTime;

	// Do sync. We do it always, so message hud can hide miniAG timer, and timer could work just as it is enabled
	if (m_flNextSyncTime <= flTime)
		SyncTimer(flTime);

	// If we are recording write changes to the demo
	if (gEngfuncs.pDemoAPI->IsRecording())
	{
		int i = 0;
		unsigned char buffer[100];
		// Current game time
		if ((int)m_flDemoSyncTime != (int)flTime)
		{
			i = 0;
			*(float *)&buffer[i] = flTime;
			i += sizeof(float);
			Demo_WriteBuffer(TYPE_TIME, i, buffer);
			m_flDemoSyncTime = flTime;
		}
		// End time and AG version
		if (m_bNeedWriteTimer)
		{
			i = 0;
			*(float *)&buffer[i] = m_flEndTime;
			i += sizeof(float);
			*(int *)&buffer[i] = m_eAgVersion;
			i += sizeof(int);
			Demo_WriteBuffer(TYPE_TIMER, i, buffer);
			m_bNeedWriteTimer = false;
		}
		if (m_bNeedWriteNextmap)
		{
			Demo_WriteBuffer(TYPE_NEXTMAP, sizeof(m_szNextmap) - 1, (unsigned char *)m_szNextmap);
			m_bNeedWriteNextmap = false;
		}
	}
}

void CHudTimer::ReadDemoTimerBuffer(int type, const unsigned char *buffer)
{
	int i = 0, count = 0;
	float time = 0;
	switch (type)
	{
		case TYPE_TIME:
			// HACK for a first update, GetClientTime returns "random" value, so we use some magic number
			time = !m_bDemoSyncTimeValid ? 2 : gEngfuncs.GetClientTime();
			m_flDemoSyncTime = *(float *)&buffer[i] - time;
			i += sizeof(float);
			m_bDemoSyncTimeValid = true;
			break;
		case TYPE_TIMER:
			m_flEndTime = *(float *)&buffer[i];
			i += sizeof(float);
			m_eAgVersion = *(int *)&buffer[i];
			i += sizeof(int);
			break;
		case TYPE_NEXTMAP:
			strncpy(m_szNextmap, (char *)buffer, sizeof(m_szNextmap) - 1);
			m_szNextmap[sizeof(m_szNextmap) - 1] = 0;
			break;
	}
}

int CHudTimer::Draw(float fTime)
{
	char text[64];

	if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL)
		return 1;

	// We will take time from demo stream if playingback
	float currentTime;
	if (gEngfuncs.pDemoAPI->IsPlayingback())
	{
		if (m_bDemoSyncTimeValid)
			currentTime = m_flDemoSyncTime + fTime;
		else
			currentTime = 0;
	}
	else
	{
		currentTime = fTime;
	}

	// Get the paint color
	int r, g, b;
	float a = 255 * gHUD.GetHudTransparency();
	UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);
	ScaleColors(r, g, b, a);

	// Draw timer
	float timeleft = m_flSynced ? (int)(m_flEndTime - currentTime) + 1 : (int)(m_flEndTime - m_flEffectiveTime);
	int hud_timer = (int)m_pCvarHudTimer->value;

	switch(hud_timer)
	{
		case 1:	// time left
			if (currentTime > 0 && timeleft > 0)
				DrawTimerInternal((int)timeleft, r, g, b, true);
		break;
		case 2:	// time passed
			if (currentTime > 0)
				DrawTimerInternal((int)currentTime, r, g, b, false);
		break;
		case 3:	// local PC time
			time_t rawtime;
			struct tm *timeinfo;
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			sprintf(text, "Clock %ld:%02ld:%02ld", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

			gHUD.DrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight, text, r, g, b);
			break;
	}

	// Draw next map
	int hud_nextmap = (int)m_pCvarHudNextmap->value;
	if (m_szNextmap[0] && timeleft < 60 && timeleft >= 0 && m_flEndTime > 0 && (hud_nextmap == 2 || (hud_nextmap == 1 && timeleft >= 37)))
	{
		sprintf(text, "Nextmap is %s", m_szNextmap);

		float a = (timeleft >= 40 || hud_nextmap > 1 ? 255.0 : 255.0 / 3 * ((m_flEndTime - currentTime) + 1 - 37)) * gHUD.GetHudTransparency();
		UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);
		ScaleColors(r, g, b, a);

		gHUD.DrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight * 2, text, r, g, b);
	}

	return 1;
}

void CHudTimer::DrawTimerInternal(int time, int r, int g, int b, bool redOnLow)
{
	div_t q;
	char text[64];

	// Calculate time parts and format into a text
	if (time >= 86400)
	{
		q = div(time, 86400);
		int d = q.quot;
		q = div(q.rem, 3600);
		int h = q.quot;
		q = div(q.rem, 60);
		int m = q.quot;
		int s = q.rem;
		sprintf(text, "%ldd %ldh %02ldm %02lds", d, h, m, s);
	}
	else if (time >= 3600)
	{
		q = div(time, 3600);
		int h = q.quot;
		q = div(q.rem, 60);
		int m = q.quot;
		int s = q.rem;
		sprintf(text, "%ldh %02ldm %02lds", h, m, s);
	}
	else if (time >= 60)
	{
		q = div(time, 60);
		int m = q.quot;
		int s = q.rem;
		sprintf(text, "%ld:%02ld", m, s);
	}
	else
	{
		sprintf(text, "%ld", (int)time);
		if (redOnLow)
		{
			float a = 255 * gHUD.GetHudTransparency();
			r = 255, g = 16, b = 16;
			ScaleColors(r, g, b, a);
		}
	}

	gHUD.DrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight, text, r, g, b);
}
