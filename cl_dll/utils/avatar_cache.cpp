#include "avatar_cache.h"
#include "web_client.h"

#include "build.h"

#ifdef XASH_WIN32
#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "noavatar.h"

ImGuiImage m_pNoAvatar;

CAvatarCache g_AvatarCache;

void CAvatarCache::Initialize()
{
    memset(m_avatars, 0, sizeof(m_avatars));
}

void CAvatarCache::VidInitialize()
{
    ClearAll();
    m_pNoAvatar = m_ImguiUtils.LoadImageFromMemory(noavatar, noavatar_len);
}

void CAvatarCache::Shutdown()
{
    ClearAll();
    m_ImguiUtils.FreeImage(m_pNoAvatar);
}

void CAvatarCache::Update()
{
    ProcessDownloadedAvatars();

    for (int i = 1; i <= gEngfuncs.GetMaxClients(); i++)
    {
        m_CustomUtils.UpdatePlayerInfo(i);
        
        if (g_PlayerIsBot[i])
            continue;

        const SteamID64 steam64 = g_PlayerSteamID64[i];
        if (steam64 == 0)
            continue;

        AvatarEntry& entry = m_avatars[i];

        if (entry.steamId != steam64)
        {
            ClearAvatar(i);
            entry.steamId = steam64;
        }

        if (!entry.loaded && !entry.requested)
        {
            entry.requested = true;
            LoadAvatar(i, steam64);
        }
    }
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
    }

    memset(&entry, 0, sizeof(AvatarEntry));
}

ImTextureID CAvatarCache::CreateTextureFromMemory(const uint8_t *buffer, size_t bufSize)
{
    if(!buffer || bufSize == 0)
        return 0;

    ImGuiImage img = m_ImguiUtils.LoadImageFromMemory(buffer, static_cast<int>(bufSize));
    return img.texture;
}

void CAvatarCache::DeleteTexture(ImTextureID tex)
{
    if (!tex) 
        return;

    ImGuiImage tempImg;
    memset(&tempImg, 0, sizeof(tempImg));
    tempImg.texture = tex;
    
    m_ImguiUtils.FreeImage(tempImg);
}

void CAvatarCache::ProcessDownloadedAvatars()
{
    DownloadedAvatar downloaded;
    while (g_WebClient.PopCompletedAvatar(downloaded))
    {
        if (!IsValidPlayerIndex(downloaded.playerIndex))
            continue;

        AvatarEntry& entry = m_avatars[downloaded.playerIndex];

        if (entry.steamId == downloaded.steam64)
        {
            if (downloaded.success && !downloaded.imageData.empty())
            {
                if (entry.texture)
                {
                    DeleteTexture(entry.texture);
                    entry.texture = 0;
                }

                entry.texture = CreateTextureFromMemory(downloaded.imageData.data(), downloaded.imageData.size());
                entry.loaded = (entry.texture != 0);
            }
        }
    }
}

bool CAvatarCache::LoadAvatar(int playerIndex, SteamID64 steam64)
{
    if (steam64 == 0)
        return false;

    g_WebClient.QueueAvatarDownload(playerIndex, steam64);
    return true;
}

ImTextureID CAvatarCache::GetAvatar(int playerIndex)
{
    if (!IsValidPlayerIndex(playerIndex) || g_PlayerIsBot[playerIndex])
        return m_pNoAvatar.texture;

    AvatarEntry& entry = m_avatars[playerIndex];

    if (entry.loaded && entry.texture)
        return entry.texture;

    return m_pNoAvatar.texture;
}