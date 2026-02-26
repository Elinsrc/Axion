#include <cmath>
#include <cstdio>
#include <cstring>

#include "hud.h"
#include "cl_util.h"
#include "cl_entity.h"
#include "pm_defs.h"
#include "event_api.h"
#include "triangleapi.h"

#include "custom_utils.h"

#include "build.h"

#if XASH_WIN32
#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#else
#include <time.h>
#endif


float CustomUtils::GetCurrentSysTime()
{
#if XASH_WIN32
    static LARGE_INTEGER perfFreq;
    static LARGE_INTEGER clockStart;
    LARGE_INTEGER currentTime;
    LONGLONG timeDiff;

    if (!perfFreq.QuadPart)
    {
        QueryPerformanceFrequency(&perfFreq);
        QueryPerformanceCounter(&clockStart);
    }

    QueryPerformanceCounter(&currentTime);
    timeDiff = currentTime.QuadPart - clockStart.QuadPart;
    return static_cast<float>(timeDiff) / static_cast<float>(perfFreq.QuadPart);
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec / 1000000000.0;
#endif
}


float CustomUtils::GetFrameTime()
{
    float calc;
    double newtime = GetCurrentSysTime();
    static double nexttime = 0, lasttime = 0;
    static double framerate = 0;
    static int framecount = 0;

    if( newtime >= nexttime )
    {
        framerate = framecount / (newtime - lasttime);
        lasttime = newtime;
        nexttime = Q_max( nexttime + 1.0, lasttime - 1.0 );
        framecount = 0;
    }

    calc = framerate;
    framecount++;

    return calc;
}


const char* CustomUtils::FormatTime(float totalSeconds)
{
    int seconds = static_cast<int>(totalSeconds);
    int days = seconds / (24 * 3600);
    seconds %= (24 * 3600);
    int hours = seconds / 3600;
    seconds %= 3600;
    int minutes = seconds / 60;
    seconds %= 60;

    static char buffer[64];
    size_t len = 0;
    buffer[0] = '\0';

    if (days > 0)
        len += snprintf(buffer + len, sizeof(buffer) - len, "%dd ", days);
    if (hours > 0)
        len += snprintf(buffer + len, sizeof(buffer) - len, "%dh ", hours);
    if (minutes > 0)
        len += snprintf(buffer + len, sizeof(buffer) - len, "%dm ", minutes);

    snprintf(buffer + len, sizeof(buffer) - len, "%ds", seconds);

    return buffer;
}


bool CustomUtils::CalcScreen(float *Origin, float *VecScreen) {
    int iResult = gEngfuncs.pTriAPI->WorldToScreen(Origin, VecScreen);

    if (VecScreen[0] < 1 && VecScreen[1] < 1 && VecScreen[0] > -1 && VecScreen[1] > -1 && !iResult)
    {
        VecScreen[0] = VecScreen[0] * (ScreenWidth / 2) + (ScreenWidth / 2);
        VecScreen[1] = -VecScreen[1] * (ScreenHeight / 2) + (ScreenHeight / 2);

        return true;
    }

    return false;
}


bool CustomUtils::CheckForPlayer(cl_entity_s *pEnt) {
    return pEnt && pEnt->model && pEnt->model->name && pEnt->player;
}


vec3_t CustomUtils::GetEntityVelocityApprox(cl_entity_t *entity, int approxStep)
{
    if (entity)
    {
        const int currIndex = entity->current_position;
        position_history_t &currState = entity->ph[currIndex & HISTORY_MASK];
        position_history_t &prevState = entity->ph[(currIndex - approxStep) & HISTORY_MASK];
        float timeDelta = currState.animtime - prevState.animtime;

        if (fabs(timeDelta) > 0.0f) {
            vec3_t originDelta = currState.origin - prevState.origin;
            return originDelta / timeDelta;
        }
    }
    return vec3_t(0, 0, 0);
}


void CustomUtils::TraceLine(vec3_t &origin, vec3_t &dir, float lineLen, pmtrace_t *traceData)
{
    vec3_t lineStart;
    vec3_t lineEnd;
    cl_entity_t *localPlayer;

    lineStart   = origin;
    lineEnd     = lineStart + (dir * lineLen);
    localPlayer = gEngfuncs.GetLocalPlayer();

    gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetSolidPlayers(localPlayer->index - 1);
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(
        lineStart, lineEnd, PM_NORMAL,
        -1, traceData
    );
    gEngfuncs.pEventAPI->EV_PopPMStates();
}


int CustomUtils::TraceEntity(vec3_t origin, vec3_t dir, float distance, vec3_t &intersect)
{
    pmtrace_t traceData;

    TraceLine(origin, dir, distance, &traceData);
    intersect = origin + dir * distance * traceData.fraction;
    if (traceData.fraction < 1.f)
    {
        if (traceData.ent > 0)
            return gEngfuncs.pEventAPI->EV_IndexFromTrace(&traceData);
    }
    return 0;
}


const char *CustomUtils::GetMovetypeName(int moveType)
{
    switch (moveType)
    {
        case MOVETYPE_BOUNCE:           return "Bounce";
        case MOVETYPE_BOUNCEMISSILE:    return "Bounce-missile";
        case MOVETYPE_FLY:              return "Fly";
        case MOVETYPE_FLYMISSILE:       return "Fly-missile";
        case MOVETYPE_FOLLOW:           return "Follow";
        case MOVETYPE_NOCLIP:           return "Noclip";
        case MOVETYPE_NONE:             return "None";
        case MOVETYPE_PUSH:             return "Push";
        case MOVETYPE_PUSHSTEP:         return "Push-step";
        case MOVETYPE_STEP:             return "Step";
        case MOVETYPE_TOSS:             return "Toss";
        case MOVETYPE_WALK:             return "Walk";
        default:                        return "Unknown";
    }
}


const char *CustomUtils::GetRenderModeName(int renderMode)
{
    switch (renderMode)
    {
        case kRenderNormal:         return "Normal";
        case kRenderTransColor:     return "Trans. color";
        case kRenderTransTexture:   return "Trans. texture";
        case kRenderGlow:           return "Glow";
        case kRenderTransAlpha:     return "Trans. alpha";
        case kRenderTransAdd:       return "Trans. additive";
        default:                    return "Unknown";
    }
}


const char *CustomUtils::GetRenderFxName(int renderFx)
{
    switch (renderFx)
    {
        case kRenderFxNone:             return "None";
        case kRenderFxPulseSlow:        return "Pulse (slow)";
        case kRenderFxPulseFast:        return "Pulse (fast)";
        case kRenderFxPulseSlowWide:    return "Pulse (slow wide)";
        case kRenderFxPulseFastWide:    return "Pulse (fast wide)";
        case kRenderFxFadeSlow:         return "Fade (slow)";
        case kRenderFxFadeFast:         return "Fade (fast)";
        case kRenderFxSolidSlow:        return "Solid (slow)";
        case kRenderFxSolidFast:        return "Solid (fast)";
        case kRenderFxStrobeSlow:       return "Strobe (slow)";
        case kRenderFxStrobeFast:       return "Strobe (fast)";
        case kRenderFxStrobeFaster:     return "Strobe (faster)";
        case kRenderFxFlickerSlow:      return "Flicker (slow)";
        case kRenderFxFlickerFast:      return "Flicker (fast)";
        case kRenderFxNoDissipation:    return "No dissipation";
        case kRenderFxDistort:          return "Distort";
        case kRenderFxHologram:         return "Hologram";
        case kRenderFxDeadPlayer:       return "Dead player";
        case kRenderFxExplode:          return "Explode";
        case kRenderFxGlowShell:        return "Glow shell";
        case kRenderFxClampMinScale:    return "Clamp min. scale";
        case kRenderFxLightMultiplier:  return "Light multiplier";
        default:                        return "Unknown";
    }
}


void CustomUtils::DrawBox(int x, int y, int w, int h, int linewidth, int r, int g, int b, int a)
{
    FillRGBABlend(x, y, w, linewidth, r, g, b, a);
    FillRGBABlend(x + w - linewidth, y + linewidth, linewidth, h - linewidth, r, g, b, a);
    FillRGBABlend(x, y + linewidth, linewidth, h - linewidth, r, g, b, a);
    FillRGBABlend(x + linewidth, y + h - linewidth, w - linewidth * 2, linewidth, r, g, b, a);
}


void CustomUtils::DrawBoxOutline(float x, float y, float w, float h, float linewidth, int r, int g, int b, int a)
{
    DrawBox(x, y, w, h, linewidth, r, g, b, a);
    DrawBox(x - 1, y - 1, w + 2, h + 2, 1, 0, 0, 0, a);
    DrawBox(x + linewidth, y + linewidth, w - linewidth * 2, h - linewidth * 2, 1, 0, 0, 0, a);
}


void CustomUtils::DrawBoxCorner(int x, int y, int w, int h, int linewidth, int r, int g, int b, int a)
{
    FillRGBABlend(x, y, w / 4, linewidth, r, g, b, a);
    FillRGBABlend(x + w - w / 4, y, w / 4, linewidth, r, g, b, a);
    FillRGBABlend(x, y + linewidth, linewidth, h / 4 - linewidth, r, g, b, a);
    FillRGBABlend(x, y + h - h / 4, linewidth, h / 4, r, g, b, a);
    FillRGBABlend(x + w - linewidth, y + linewidth, linewidth, h / 4 - linewidth, r, g, b, a);
    FillRGBABlend(x + w - linewidth, y + h - h / 4, linewidth, h / 4, r, g, b, a);
    FillRGBABlend(x + linewidth, y + h - linewidth, w / 4 - linewidth, linewidth, r, g, b, a);
    FillRGBABlend(x + w - w / 4, y + h - linewidth, w / 4 - linewidth, linewidth, r, g, b, a);
}


void CustomUtils::DrawBoxCornerOutline(int x, int y, int w, int h, int linewidth, int r, int g, int b, int a)
{
    DrawBoxCorner(x - 1, y + 1, w, h, linewidth, 0, 0, 0, a);
    DrawBoxCorner(x - 1, y - 1, w, h, linewidth, 0, 0, 0, a);
    DrawBoxCorner(x + 1, y + 1, w, h, linewidth, 0, 0, 0, a);
    DrawBoxCorner(x + 1, y - 1, w, h, linewidth, 0, 0, 0, a);

    DrawBoxCorner(x, y, w, h, linewidth, r, g, b, a);
}
