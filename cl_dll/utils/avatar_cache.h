#ifndef AVATAR_CACHE_H
#define AVATAR_CACHE_H

#include <stdint.h>
#include "imgui.h"
#include "steam_api.h"

#define MAX_AVATAR_PLAYERS 32

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
    void Shutdown();

    ImTextureID GetAvatar(int playerIndex);

    void ClearAvatar(int playerIndex);
    void ClearAll();

private:
    AvatarEntry m_avatars[MAX_AVATAR_PLAYERS];


    SteamID64 SteamIdToSteam64(const char *steamId);


    ImTextureID CreateTextureFromRGBA(uint8_t *data, int width, int height);

    void DeleteTexture(ImTextureID tex);

    bool LoadAvatar(int playerIndex, SteamID64 steam64);
};

extern CAvatarCache g_AvatarCache;

#endif