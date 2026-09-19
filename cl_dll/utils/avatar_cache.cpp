#include "avatar_cache.h"
#include "web_client.h"

#include "build.h"

#include "hud.h"
#include "cl_util.h"
#include "imgui_utils.h"

#ifdef XASH_WIN32
#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "noavatar.h"

#define AVATAR_TARGET_SIZE 64
#define FRAMES_PER_TICK 1

ImGuiImage m_pNoAvatar;

CAvatarCache g_AvatarCache;

static void AvatarCacheInfo_f()
{
    g_AvatarCache.PrintCacheInfo();
}

void CAvatarCache::Initialize()
{
    for (int i = 0; i < MAX_AVATAR_PLAYERS; i++)
        m_avatars[i] = AvatarEntry{};

    gEngfuncs.pfnAddCommand("avatar_cache_info", AvatarCacheInfo_f);
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
    ProcessPendingTextures();

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

        if (!entry.loaded && !entry.requested && !entry.isPending)
        {
            entry.requested = true;
            LoadAvatar(i, steam64);
        }
    }
}

void CAvatarCache::PrintCacheInfo()
{
    int staticCount = 0;
    int animatedCount = 0;
    int pendingCount = 0;
    int loadingCount = 0;
    int totalFrames = 0;
    size_t totalMem = 0;

    for (int i = 1; i < MAX_AVATAR_PLAYERS; i++)
    {
        const AvatarEntry& entry = m_avatars[i];
        if (entry.steamId == 0) continue;

        if (entry.isPending)
        {
            pendingCount++;
        }
        else if (entry.isAnimated && !entry.frames.empty())
        {
            animatedCount++;
            totalFrames += (int)entry.frames.size();
            totalMem += AVATAR_TARGET_SIZE * AVATAR_TARGET_SIZE * 4 * entry.frames.size();
        }
        else if (entry.loaded && entry.texture)
        {
            staticCount++;
            totalMem += AVATAR_TARGET_SIZE * AVATAR_TARGET_SIZE * 4;
        }
        else if (entry.requested)
        {
            loadingCount++;
        }
    }

    gEngfuncs.Con_Printf("AvatarCache:\n");
    gEngfuncs.Con_Printf("  static: %d\n", staticCount);
    gEngfuncs.Con_Printf("  animated: %d\n", animatedCount);
    gEngfuncs.Con_Printf("  loading: %d\n", loadingCount);
    gEngfuncs.Con_Printf("  pending: %d\n", pendingCount);
    gEngfuncs.Con_Printf("  frames: %d\n", totalFrames);
    gEngfuncs.Con_Printf("  memory: %.2f MB\n", totalMem / (1024.0 * 1024.0));
}

void CAvatarCache::ClearAll()
{
    for(int i = 0; i < MAX_AVATAR_PLAYERS; i++)
        ClearAvatar(i);
}

void CAvatarCache::ClearAvatar(int playerIndex)
{
    if(!IsValidPlayerIndex(playerIndex))
        return;

    AvatarEntry& entry = m_avatars[playerIndex];

    if (entry.steamId != 0)
        g_WebClient.InvalidateAvatar(entry.steamId);

    if (entry.texture)
    {
        DeleteTexture(entry.texture);
    }

    for (auto& f : entry.frames)
    {
        if (f.texture)
            DeleteTexture(f.texture);
    }

    entry = AvatarEntry{};
}

ImTextureID CAvatarCache::CreateTextureFromMemory(const uint8_t* buffer, size_t bufSize)
{
    if(!buffer || bufSize == 0)
        return 0;

    ImGuiImage img = m_ImguiUtils.LoadImageFromMemory(buffer, static_cast<int>(bufSize));
    return img.texture;
}

ImTextureID CAvatarCache::CreateTextureFromRGBA(const uint8_t* rgba, int w, int h)
{
    if(!rgba || w <= 0 || h <= 0)
        return 0;

    ImGuiImage img = m_ImguiUtils.LoadImageFromRGBA(rgba, w, h);
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

        if (entry.steamId != downloaded.steam64)
            continue;

        if (!downloaded.success)
            continue;

        if (!downloaded.isAnimated)
        {
            if (entry.texture)
            {
                DeleteTexture(entry.texture);
                entry.texture = 0;
            }

            if (!downloaded.imageData.empty())
            {
                entry.texture = CreateTextureFromMemory(
                    downloaded.imageData.data(),
                    downloaded.imageData.size()
                );
                entry.loaded = (entry.texture != 0);
            }
            continue;
        }

        if (downloaded.gifFrames.empty())
            continue;

        for (auto& f : entry.frames)
        {
            if (f.texture)
                DeleteTexture(f.texture);
        }
        entry.frames.clear();

        entry.pendingFrameData  = std::move(downloaded.gifFrames);
        entry.pendingDelays = std::move(downloaded.gifDelays);
        entry.pendingWidth = downloaded.gifWidth;
        entry.pendingHeight = downloaded.gifHeight;
        entry.pendingFrameCount = downloaded.gifFrameCount;
        entry.pendingFrameIndex = 0;
        entry.totalDurationMs = 0.0f;
        entry.isAnimated = false;
        entry.isPending = true;
    }
}

void CAvatarCache::ProcessPendingTextures()
{
    for (int i = 0; i < MAX_AVATAR_PLAYERS; i++)
    {
        AvatarEntry& entry = m_avatars[i];

        if (!entry.isPending)
            continue;

        int uploaded = 0;
        while (entry.pendingFrameIndex < entry.pendingFrameCount && uploaded < FRAMES_PER_TICK)
        {
            int fi = entry.pendingFrameIndex;

            AvatarFrame af;
            af.delayMs = entry.pendingDelays[fi];
            af.texture = CreateTextureFromRGBA(
                entry.pendingFrameData[fi].data(),
                entry.pendingWidth,
                entry.pendingHeight
            );

            if (af.texture)
                entry.frames.push_back(af);

            entry.pendingFrameIndex++;
            uploaded++;
        }

        if (entry.pendingFrameIndex >= entry.pendingFrameCount)
        {
            entry.pendingFrameData.clear();
            entry.pendingDelays.clear();
            entry.isPending = false;

            if ((int)entry.frames.size() > 1)
            {
                for (auto& f : entry.frames)
                    entry.totalDurationMs += f.delayMs;

                entry.isAnimated = true;
                entry.loaded = true;
                entry.animFrame = 0;
                entry.animAccumMs = 0.0f;
                entry.animLastTime = m_CustomUtils.GetCurrentSysTime();
            }
        }

        break;
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

    if (!entry.loaded)
        return m_pNoAvatar.texture;

    if (entry.isAnimated && !entry.frames.empty() && entry.totalDurationMs > 0.0f)
    {
        double now = m_CustomUtils.GetCurrentSysTime();
        float delta = static_cast<float>((now - entry.animLastTime) * 1000.0);
        entry.animLastTime = now;

        if (delta > 0.0f && delta < entry.frames[entry.animFrame].delayMs * 2.0f)
        {
            entry.animAccumMs += delta;

            while (entry.animAccumMs >= entry.frames[entry.animFrame].delayMs)
            {
                entry.animAccumMs -= entry.frames[entry.animFrame].delayMs;
                entry.animFrame    = (entry.animFrame + 1) % (int)entry.frames.size();
            }
        }
        else
        {
            entry.animLastTime = now;
        }

        return entry.frames[entry.animFrame].texture;
    }

    if (entry.texture)
        return entry.texture;

    return m_pNoAvatar.texture;
}