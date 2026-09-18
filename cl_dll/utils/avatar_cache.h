#pragma once

#include <stdint.h>
#include <stddef.h>
#include "imgui.h"

#define MAX_AVATAR_PLAYERS 32
#define AVATAR_REQUEST_COOLDOWN 2.0f

typedef uint64_t SteamID64;

struct AvatarEntry
{
    ImTextureID texture;
    SteamID64 steamId;
    float lastRequestTime;
    bool loaded;
    bool requested;
};

class CAvatarCache
{
public:
    void Initialize();
    void VidInitialize();
    void Shutdown();

    ImTextureID GetAvatar(int playerIndex);

    void ClearAvatar(int playerIndex);
    void ClearAll();

    static SteamID64 SteamIdToSteam64(const char* steamId);

private:
    AvatarEntry m_avatars[MAX_AVATAR_PLAYERS];

    ImTextureID CreateTextureFromMemory(const uint8_t *buffer, size_t bufSize);
    void DeleteTexture(ImTextureID tex);

    void ProcessDownloadedAvatars();
    bool LoadAvatar(int playerIndex, SteamID64 steam64);

    inline bool IsValidPlayerIndex(int playerIndex) const
    {
        return playerIndex >= 1 && playerIndex < MAX_AVATAR_PLAYERS;
    }
};

extern CAvatarCache g_AvatarCache;