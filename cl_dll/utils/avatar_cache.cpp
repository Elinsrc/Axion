#include "avatar_cache.h"
#include "hud.h"
#include "cl_util.h"
#include "ui_ScorePanel.h"
#include "custom_utils.h"
#include "avatar_downloader.h"

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
    while (g_AvatarDownloader.PopCompleted(downloaded))
    {
        if (!IsValidPlayerIndex(downloaded.playerIndex))
            continue;

        AvatarEntry& entry = m_avatars[downloaded.playerIndex];

        if (entry.steamId == downloaded.steam64 && downloaded.success && !downloaded.imageData.empty())
        {
            if (entry.texture)
            {
                DeleteTexture(entry.texture);
                entry.texture = 0;
            }

            entry.texture = CreateTextureFromMemory(downloaded.imageData.data(), downloaded.imageData.size());
            entry.loaded = (entry.texture != 0);
        }
        
        entry.requested = false;
    }
}

bool CAvatarCache::LoadAvatar(int playerIndex, SteamID64 steam64)
{
    if (steam64 == 0)
        return false;

    g_AvatarDownloader.FetchAvatarAsync(playerIndex, steam64);
    return true;
}

ImTextureID CAvatarCache::GetAvatar(int playerIndex)
{
    ProcessDownloadedAvatars();

    if (!IsValidPlayerIndex(playerIndex))
        return m_pNoAvatar.texture;

    if (g_PlayerIsBot[playerIndex])
        return m_pNoAvatar.texture;

    const SteamID64 steam64 = g_PlayerSteamID64[playerIndex];

    if (steam64 == 0)
        return m_pNoAvatar.texture;

    AvatarEntry& entry = m_avatars[playerIndex];

    if (entry.steamId != steam64)
    {
        ClearAvatar(playerIndex);
        entry.steamId = steam64;
    }

    if (entry.loaded && entry.texture)
        return entry.texture;

    const float now = gHUD.m_flTime;
    if (entry.requested && (now - entry.lastRequestTime) < AVATAR_REQUEST_COOLDOWN)
        return m_pNoAvatar.texture;

    entry.requested       = true;
    entry.lastRequestTime = now;

    LoadAvatar(playerIndex, steam64);

    return m_pNoAvatar.texture;
}