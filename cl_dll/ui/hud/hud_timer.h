#pragma once

#define MAX_MAP_NAME 64
#define HLARRAYSIZE(p) (sizeof(p)/sizeof(p[0]))

class CHudTimer: public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	void Think(void);
	int Draw(float flTime);

	int MsgFunc_Timer(const char *pszName, int iSize, void *pbuf);

	void DoResync(void);
	void ReadDemoTimerBuffer(int type, const unsigned char *buffer);

	enum {
		SV_AG_NONE = -1,
		SV_AG_UNKNOWN = 0,
		SV_AG_MINI = 1,
		SV_AG_FULL = 2,
	};

	int GetAgVersion(void) { return m_eAgVersion; }
	float GetHudNextmap(void) { return m_pCvarHudNextmap->value; }
	const char* GetNextmap(void) { return m_szNextmap; }
	void SetNextmap(const char *nextmap)
	{
		strncpy(m_szNextmap, nextmap, HLARRAYSIZE(m_szNextmap) - 1);
		m_szNextmap[HLARRAYSIZE(m_szNextmap) - 1] = 0;
	}

private:
	void SyncTimer(float fTime);
	void SyncTimerLocal(float fTime);
	void SyncTimerRemote(unsigned int ip, unsigned short port, float fTime, double latency);
	void DrawTimerInternal(int time, int r, int g, int b, bool redOnLow);

	float	m_flDemoSyncTime;
	bool	m_bDemoSyncTimeValid;
	float	m_flNextSyncTime;
	bool	m_flSynced;
	float	m_flEndTime;
	float	m_flEffectiveTime;
	bool	m_bDelayTimeleftReading;
	int		m_eAgVersion;
	char	m_szNextmap[MAX_MAP_NAME];
	bool	m_bNeedWriteTimer;
	bool	m_bNeedWriteNextmap;

	cvar_t *m_pCvarHudTimer;
	cvar_t *m_pCvarHudTimerSync;
	cvar_t *m_pCvarHudNextmap;
	cvar_t *m_pCvarMpTimelimit;
	cvar_t *m_pCvarMpTimeleft;
	cvar_t *m_pCvarSvAgVersion;
	cvar_t *m_pCvarAmxNextmap;

	char	m_szPacketBuffer[22400];	// 16x1400 split packets
	int		m_iResponceID;
	int		m_iReceivedSize;
	int		m_iReceivedPackets;
	int		m_iReceivedPacketsCount;
};
