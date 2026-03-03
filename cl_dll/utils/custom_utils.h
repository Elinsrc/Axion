#pragma once
#include "pmtrace.h"
#include <stdint.h> 

class CustomUtils
{
    float m_frameTime;
    float m_lastFrameTime;
    float m_lastSysTime;

public:
    float GetCurrentSysTime();
    float GetFrameTime();
    const char* FormatTime(float totalSeconds);
    bool CalcScreen(float *Origin, float *VecScreen);
    bool CheckForPlayer(cl_entity_s *pEnt);
    vec3_t GetEntityVelocityApprox(cl_entity_t *entity, int approxStep);
    void TraceLine(vec3_t &origin, vec3_t &dir, float lineLen, pmtrace_t *traceData);
    int TraceEntity(vec3_t origin, vec3_t dir, float distance, vec3_t &intersect);
    const char* GetMovetypeName(int moveType);
    const char* GetRenderModeName(int renderMode);
    const char* GetRenderFxName(int renderFx);
    void DrawBox(int x, int y, int w, int h, int linewidth, int r, int g, int b, int a);
    void DrawBoxOutline(float x, float y, float w, float h, float linewidth, int r, int g, int b, int a);
    void DrawBoxCorner(int x, int y, int w, int h, int linewidth, int r, int g, int b, int a);
    void DrawBoxCornerOutline(int x, int y, int w, int h, int linewidth, int r, int g, int b, int a);

    void UpdatePlayerInfo(int iPlayerIndex);
};
