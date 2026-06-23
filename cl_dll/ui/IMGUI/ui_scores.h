#pragma once
#include "hud.h"

struct UIScoresData
{
	char szScore[64];
	int r, g, b;
};

class CImGuiScores
{
public:
    void Init();
    void Draw();
private:
    UIScoresData m_ScoresData[MAX_PLAYERS];
	int m_iLines = 0;
};

extern CImGuiScores g_ImGuiScores;