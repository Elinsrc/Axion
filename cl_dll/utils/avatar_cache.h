#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include "imgui.h"

#include "custom_utils.h"

#define MAX_AVATAR_PLAYERS 33

typedef uint64_t SteamID64;

struct AvatarFrame
{
    ImTextureID texture;
    float delayMs;
};

struct AvatarEntry
{
    ImTextureID texture = 0;
    SteamID64 steamId = 0;
    float lastRequestTime  = 0.0f;
    bool loaded = false;
    bool requested = false;
    bool isAnimated = false;

    std::vector<AvatarFrame> frames;
    float totalDurationMs = 0.0f;
    int animFrame = 0;
    float animAccumMs = 0.0f;
    double animLastTime = 0.0;

    std::vector<std::vector<uint8_t>> pendingFrameData;
    std::vector<float> pendingDelays;
    int pendingWidth = 0;
    int pendingHeight = 0;
    int pendingFrameCount = 0;
    int pendingFrameIndex = 0;
    bool isPending = false;
};

class CAvatarCache
{
    CustomUtils m_CustomUtils;
public:
    void Initialize();
    void VidInitialize();
    void Shutdown();
    
    void Update();

    void PrintCacheInfo();

    ImTextureID GetAvatar(int playerIndex);
    void ClearAvatar(int playerIndex);
    void ClearAll();

    static SteamID64 SteamIdToSteam64(const char* steamId);

private:
    AvatarEntry m_avatars[MAX_AVATAR_PLAYERS];

    ImTextureID CreateTextureFromMemory(const uint8_t* buffer, size_t bufSize);
    ImTextureID CreateTextureFromRGBA(const uint8_t* rgba, int w, int h);
    void DeleteTexture(ImTextureID tex);

    void ProcessDownloadedAvatars();
    void ProcessPendingTextures();
    bool LoadAvatar(int playerIndex, SteamID64 steam64);

    inline bool IsValidPlayerIndex(int playerIndex) const
    {
        return playerIndex >= 1 && playerIndex < MAX_AVATAR_PLAYERS;
    }
};

extern CAvatarCache g_AvatarCache;