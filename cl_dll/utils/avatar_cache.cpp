#include "avatar_cache.h"
#include "steam_api.h"
#include "hud.h"
#include "cl_util.h"
#include "ui_ScorePanel.h"
#include "custom_utils.h"

#include "build.h"

#ifdef XASH_WIN32
#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#endif
#include <GL/gl.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

CAvatarCache g_AvatarCache;

void CAvatarCache::Initialize()
{
    memset(m_avatars, 0, sizeof(m_avatars));
}

void CAvatarCache::Shutdown()
{
    for (int i = 0; i < MAX_AVATAR_PLAYERS; i++)
    {
        if (m_avatars[i].texture)
        {
            DeleteTexture(m_avatars[i].texture);
        }
    }
    
    memset(m_avatars, 0, sizeof(m_avatars));
}

void CAvatarCache::ClearAll()
{
    for(int i = 0; i < MAX_AVATAR_PLAYERS; i++)
    {
        ClearAvatar(i);
    }
}

void CAvatarCache::ClearAvatar(int playerIndex)
{
    if(!IsValidPlayerIndex(playerIndex))
        return;

    AvatarEntry &entry = m_avatars[playerIndex];

    if( entry.texture )
    {
        DeleteTexture(entry.texture);
        entry.texture = 0;
    }

    memset(&entry, 0, sizeof(AvatarEntry));
}

SteamID64 CAvatarCache::SteamIdToSteam64(const char *steamId)
{
    if(!steamId || !*steamId)
        return 0;

    unsigned int authServer = 0;
    unsigned int authBit = 0;
    unsigned int accountId  = 0;


    if( sscanf(steamId, "STEAM_%u:%u:%u",&authServer, &authBit, &accountId) == 3 )
    {
        return 76561197960265728ULL + (static_cast<uint64_t>(accountId) << 1) + authBit;
    }

    if( sscanf(steamId, "%u:%u:%u", &authServer, &authBit, &accountId) == 3)
    {
        return 76561197960265728ULL + (static_cast<uint64_t>(accountId) << 1) + authBit;
    }

    char *endPtr = nullptr;
    uint64_t numericId = strtoull(steamId, &endPtr, 10);

    if(endPtr != steamId && numericId > 76561197960265728ULL)
    {
        return static_cast<SteamID64>(numericId);
    }

    return 0;
}

ImTextureID CAvatarCache::CreateTextureFromRGBA(uint8_t *data, int width, int height)
{
    if(!data || width <= 0 || height <= 0)
        return 0;

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return (ImTextureID)(intptr_t)texId;
}

void CAvatarCache::DeleteTexture(ImTextureID tex)
{
    if (!tex) 
        return;
    
    GLuint texId = (GLuint)(intptr_t)tex;
    glDeleteTextures(1, &texId);
}

bool CAvatarCache::LoadAvatar(int playerIndex, SteamID64 steam64)
{
    if (!g_SteamAPI.IsInitialized() || steam64 == 0)
        return false;

    int imageHandle = g_SteamAPI.GetMediumFriendAvatar(steam64);

    if (imageHandle == 0 || imageHandle == -1)
    {
        return false;
    }

    uint32_t width = 0, height = 0;
    if (!g_SteamAPI.GetImageSize(imageHandle, &width, &height))
        return false;

    if (width == 0 || height == 0)
        return false;

    int bufSize = width * height * 4;
    uint8_t *rgbaData = (uint8_t *)malloc(bufSize);
    if (!rgbaData)
        return false;

    if (!g_SteamAPI.GetImageRGBA(imageHandle, rgbaData, bufSize))
    {
        free(rgbaData);
        return false;
    }

    if (m_avatars[playerIndex].texture)
    {
        DeleteTexture(m_avatars[playerIndex].texture);
        m_avatars[playerIndex].texture = 0;
    }

    m_avatars[playerIndex].texture = CreateTextureFromRGBA(rgbaData, width, height);
    m_avatars[playerIndex].steamId = steam64;
    m_avatars[playerIndex].loaded = true;

    return true;
}

ImTextureID CAvatarCache::GetAvatar(int playerIndex)
{
    if(!IsValidPlayerIndex(playerIndex))
        return 0;

    const SteamID64 steam64 = g_PlayerSteamID64[playerIndex];
    
    if (steam64 == 0)
        return 0;

    AvatarEntry &entry = m_avatars[playerIndex];

    if (entry.loaded && entry.steamId == steam64)
        return entry.texture;

    if (entry.steamId != steam64)
    {
        ClearAvatar(playerIndex);
        entry.steamId = steam64;
    }

    const float now = gHUD.m_flTime;
    if(entry.requested && (now - entry.lastRequestTime) < AVATAR_REQUEST_COOLDOWN)
    {
        return 0;
    }

    entry.requested = true;
    entry.lastRequestTime = now;

    if (LoadAvatar(playerIndex, steam64))
        return entry.texture;

    return 0;
}
