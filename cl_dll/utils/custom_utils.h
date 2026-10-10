#pragma once

#include "hud.h"
#include "cl_util.h"
#include "cl_entity.h"
#include "pm_defs.h"
#include "event_api.h"
#include "triangleapi.h"
#include "pmtrace.h"

#include <cstdint>

extern uint64_t g_PlayerSteamID64[MAX_PLAYERS + 1];
extern bool g_PlayerIsBot[MAX_PLAYERS + 1];

static constexpr int CUSTOM_AVATAR_MAX_FRAMES = 200;

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

    static uint64_t ParseToSteam64(const char* sid);
    void UpdatePlayerInfo(int iPlayerIndex);

    static bool PngSize(const uint8_t* d, size_t n, int& w, int& h);
    static bool JpegSize(const uint8_t* d, size_t n, int& w, int& h);
    static bool GifSize(const uint8_t* d, size_t n, int& w, int& h);
    static int CountGifFrames(const uint8_t* d, size_t n);
};