#pragma once
#include <cstdint>

class CHudNextMap : public CHudBase
{
public:
	CHudNextMap() : m_flTurnoff(0) {}

	virtual int Init();
	virtual int VidInit();
	virtual int Draw(float time);

	int MsgFunc_Nextmap(const char* name, int size, void* buf);

private:
	float m_flTurnoff;
	char m_szNextmap[32];
};
