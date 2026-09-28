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

#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "stb_image.h"
#include "noavatar.h"

#define AVATAR_TARGET_SIZE 64
#define DEFAULT_GIF_DELAY_MS 40.0f
#define FRAMES_PER_TICK 1

ImGuiImage m_pNoAvatar;

CAvatarCache g_AvatarCache;

static void AvatarCacheInfo_f()
{
    g_AvatarCache.PrintCacheInfo();
}

static std::string ProfileUrl(SteamID64 id)
{
    return "https://steamcommunity.com/profiles/" + std::to_string(id);
}

static std::string ExtractXmlTag(const std::string& xml, const std::string& tag)
{
    const std::string openTag = "<" + tag + ">";
    const std::string closeTag = "</" + tag + ">";

    size_t start = xml.find(openTag);
    if (start == std::string::npos)
        return "";

    start += openTag.length();
    size_t end = xml.find(closeTag, start);
    if (end == std::string::npos)
        return "";

    std::string val = xml.substr(start, end - start);

    size_t cdataStart = val.find("<![CDATA[");
    if (cdataStart != std::string::npos)
    {
        cdataStart += 9;
        size_t cdataEnd = val.find("]]>", cdataStart);
        if (cdataEnd != std::string::npos)
            return val.substr(cdataStart, cdataEnd - cdataStart);
    }

    return val;
}

static std::string ExtractAnimatedAvatarUrl(const std::string& html)
{
    size_t avatarBlock = html.find("playerAvatar");
    if (avatarBlock == std::string::npos)
        return "";

    size_t endBlock = html.find("profile_header_centered_col", avatarBlock);
    if (endBlock == std::string::npos)
        endBlock = avatarBlock + 4096;
    if (endBlock > html.size())
        endBlock = html.size();

    size_t gifPos = html.find(".gif", avatarBlock);
    if (gifPos == std::string::npos || gifPos >= endBlock)
        return "";

    size_t urlStart = html.rfind("http", gifPos);
    if (urlStart == std::string::npos || urlStart < avatarBlock)
        return "";

    return html.substr(urlStart, (gifPos + 4) - urlStart);
}

static HttpRequest MakeRequest(const std::string& url, long timeoutSec, const std::atomic<bool>& cancel)
{
    HttpRequest req;
    req.url = url;
    req.timeoutSec = timeoutSec;
    req.cancel = &cancel;
    return req;
}

static bool FetchStaticAvatar(SteamID64 id, const std::atomic<bool>& cancel, std::vector<uint8_t>& out, std::string& error)
{
    HttpResponse xml = g_WebClient.Get(MakeRequest(ProfileUrl(id) + "?xml=1", 5, cancel));
    if (!xml.ok)
    {
        error = "can't get profile info: " + xml.error;
        return false;
    }

    std::string avatarUrl = ExtractXmlTag(xml.Text(), "avatarMedium");
    if (avatarUrl.empty())
    {
        error = "avatar url not found (invalid profile?)";
        return false;
    }

    HttpResponse img = g_WebClient.Get(MakeRequest(avatarUrl, 5, cancel));
    if (!img.ok || img.body.empty())
    {
        error = "image download failed: " + (img.ok ? std::string("empty response") : img.error);
        return false;
    }

    out = std::move(img.body);
    return true;
}

enum class GifFetch 
{ 
    Failed, 
    NoAnimation, 
    Ok 
};

static GifFetch FetchAnimatedGif(SteamID64 id, const std::atomic<bool>& cancel, std::vector<uint8_t>& out)
{
    HttpResponse page = g_WebClient.Get(MakeRequest(ProfileUrl(id), 7, cancel));
    if (!page.ok || page.body.empty())
        return GifFetch::Failed;

    std::string gifUrl = ExtractAnimatedAvatarUrl(page.Text());
    if (gifUrl.empty())
        return GifFetch::NoAnimation;

    HttpResponse gif = g_WebClient.Get(MakeRequest(gifUrl, 7, cancel));
    if (!gif.ok || gif.body.empty())
        return GifFetch::Failed;

    out = std::move(gif.body);
    return GifFetch::Ok;
}

static void ResizeBilinearRGBA(const uint8_t* src, int srcW, int srcH, uint8_t* dst, int dstSize)
{
    const float scaleX = (float)srcW / (float)dstSize;
    const float scaleY = (float)srcH / (float)dstSize;

    for (int y = 0; y < dstSize; y++)
    {
        for (int x = 0; x < dstSize; x++)
        {
            const float sx = (x + 0.5f) * scaleX - 0.5f;
            const float sy = (y + 0.5f) * scaleY - 0.5f;

            const int x0 = Q_max((int)sx, 0);
            const int y0 = Q_max((int)sy, 0);
            const int x1 = Q_min(x0 + 1, srcW - 1);
            const int y1 = Q_min(y0 + 1, srcH - 1);

            const float fx = sx - (float)x0;
            const float fy = sy - (float)y0;

            for (int c = 0; c < 4; c++)
            {
                const float p00 = src[(y0 * srcW + x0) * 4 + c];
                const float p10 = src[(y0 * srcW + x1) * 4 + c];
                const float p01 = src[(y1 * srcW + x0) * 4 + c];
                const float p11 = src[(y1 * srcW + x1) * 4 + c];

                const float val = p00 * (1 - fx) * (1 - fy) + p10 * fx * (1 - fy) + p01 * (1 - fx) * fy + p11 * fx * fy;

                dst[(y * dstSize + x) * 4 + c] = (uint8_t)(val + 0.5f);
            }
        }
    }
}

static bool DecodeGif(const std::vector<uint8_t>& bytes, DownloadedAvatar& out)
{
    int width = 0, height = 0, frameCount = 0, channels = 0;
    int* delays = nullptr;

    uint8_t* raw = stbi_load_gif_from_memory(bytes.data(), (int)bytes.size(), &delays, &width, &height, &frameCount, &channels, 4);

    if (!raw || frameCount <= 1 || width <= 0 || height <= 0)
    {
        if (raw)
            stbi_image_free(raw);
        if (delays)
            stbi_image_free(delays);
        return false;
    }

    const size_t srcBytes = (size_t)width * height * 4;
    const size_t dstBytes = (size_t)AVATAR_TARGET_SIZE * AVATAR_TARGET_SIZE * 4;

    out.gifWidth = AVATAR_TARGET_SIZE;
    out.gifHeight = AVATAR_TARGET_SIZE;
    out.gifFrames.resize(frameCount);
    out.gifDelays.resize(frameCount);

    for (int f = 0; f < frameCount; f++)
    {
        out.gifFrames[f].resize(dstBytes);
        ResizeBilinearRGBA(raw + f * srcBytes, width, height, out.gifFrames[f].data(), AVATAR_TARGET_SIZE);

        float d = delays ? (float)delays[f] : DEFAULT_GIF_DELAY_MS;
        out.gifDelays[f] = (d > 0.0f) ? d : DEFAULT_GIF_DELAY_MS;
    }

    stbi_image_free(raw);
    stbi_image_free(delays);
    return true;
}

void CAvatarCache::Initialize()
{
    for (int i = 0; i < MAX_AVATAR_PLAYERS; i++)
        m_avatars[i] = AvatarEntry{};

    m_staticWorker.Start();
    m_animatedWorker.Start();

    gEngfuncs.pfnAddCommand("avatar_cache_info", AvatarCacheInfo_f);
}

void CAvatarCache::VidInitialize()
{
    ClearAll();
    m_pNoAvatar = m_ImguiUtils.LoadImageFromMemory(noavatar, noavatar_len);
}

void CAvatarCache::Shutdown()
{
    m_staticWorker.Stop();
    m_animatedWorker.Stop();

    ClearAll();
    m_ImguiUtils.FreeImage(m_pNoAvatar);
}

void CAvatarCache::Update()
{
    g_WebClient.FlushLogs();

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
    int failedCount = 0;
    int totalFrames = 0;
    size_t totalMem = 0;

    for (int i = 1; i < MAX_AVATAR_PLAYERS; i++)
    {
        const AvatarEntry& entry = m_avatars[i];
        if (entry.steamId == 0) 
            continue;

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
        else if (entry.failed)
        {
            failedCount++;
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
    gEngfuncs.Con_Printf("  failed: %d\n", failedCount);
    gEngfuncs.Con_Printf("  pending: %d\n", pendingCount);
    gEngfuncs.Con_Printf("  frames: %d\n", totalFrames);
    gEngfuncs.Con_Printf("  memory: %.2f MB\n", totalMem / (1024.0 * 1024.0));
}

void CAvatarCache::ClearAll()
{
    for (int i = 0; i < MAX_AVATAR_PLAYERS; i++)
        ClearAvatar(i);
}

void CAvatarCache::ClearAvatar(int playerIndex)
{
    if (!IsValidPlayerIndex(playerIndex))
        return;

    AvatarEntry& entry = m_avatars[playerIndex];

    if (entry.steamId != 0)
        InvalidateDownloaded(entry.steamId);

    DeleteTexture(entry.texture);

    for (auto& f : entry.frames)
        DeleteTexture(f.texture);

    entry = AvatarEntry{};
}

ImTextureID CAvatarCache::CreateTextureFromMemory(const uint8_t* buffer, size_t bufSize)
{
    if (!buffer || bufSize == 0)
        return 0;

    ImGuiImage img = m_ImguiUtils.LoadImageFromMemory(buffer, static_cast<int>(bufSize));
    return img.texture;
}

ImTextureID CAvatarCache::CreateTextureFromRGBA(const uint8_t* rgba, int w, int h)
{
    if (!rgba || w <= 0 || h <= 0)
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
    std::vector<DownloadedAvatar> done;
    {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        done.swap(m_completed);
    }

    for (auto& data : done)
    {
        if (!IsValidPlayerIndex(data.playerIndex))
            continue;

        AvatarEntry& entry = m_avatars[data.playerIndex];
        if (entry.steamId != data.steam64)
            continue;

        if (data.failed)
        {
            entry.failed = true;
            continue;
        }

        if (data.isAnimated)
            ApplyAnimated(entry, data);
        else
            ApplyStatic(entry, data);
    }
}

void CAvatarCache::ApplyStatic(AvatarEntry& entry, DownloadedAvatar& data)
{
    if (data.imageData.empty())
        return;

    DeleteTexture(entry.texture);
    entry.texture = CreateTextureFromMemory(data.imageData.data(), data.imageData.size());
    entry.loaded = (entry.texture != 0);
}

void CAvatarCache::ApplyAnimated(AvatarEntry& entry, DownloadedAvatar& data)
{
    if (data.gifFrames.empty())
        return;

    for (auto& f : entry.frames)
        DeleteTexture(f.texture);
    entry.frames.clear();

    entry.pendingFrameData = std::move(data.gifFrames);
    entry.pendingDelays = std::move(data.gifDelays);
    entry.pendingWidth = data.gifWidth;
    entry.pendingHeight = data.gifHeight;
    entry.pendingFrameCount = (int)entry.pendingFrameData.size();
    entry.pendingFrameIndex = 0;
    entry.totalDurationMs = 0.0f;
    entry.isAnimated = false;
    entry.isPending = true;
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
            const int fi = entry.pendingFrameIndex;

            AvatarFrame af;
            af.delayMs = entry.pendingDelays[fi];
            af.texture = CreateTextureFromRGBA(entry.pendingFrameData[fi].data(), entry.pendingWidth, entry.pendingHeight);

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

    QueueStatic({playerIndex, steam64});
    return true;
}

void CAvatarCache::QueueStatic(const AvatarTask& task)
{
    if (IsDownloaded(false, task.steam64))
        return;

    m_staticWorker.Post([this, task]() { DownloadStatic(task); });
}

void CAvatarCache::DownloadStatic(const AvatarTask& task)
{
    if (IsDownloaded(false, task.steam64))
        return;

    DownloadedAvatar result;
    result.playerIndex = task.playerIndex;
    result.steam64 = task.steam64;

    std::string error;
    if (!FetchStaticAvatar(task.steam64, m_staticWorker.StopFlag(), result.imageData, error))
    {
        g_WebClient.LogErrorOnce("avatar_" + std::to_string(task.steam64), "[Avatars] " + std::to_string(task.steam64) + ": " + error);

        result.failed = true;
        result.imageData.clear();
        PushCompleted(std::move(result));
        return;
    }

    MarkDownloaded(false, task.steam64);
    PushCompleted(std::move(result));

    if (!IsDownloaded(true, task.steam64))
        m_animatedWorker.Post([this, task]() { DownloadAnimated(task); });
}

void CAvatarCache::DownloadAnimated(const AvatarTask& task)
{
    if (IsDownloaded(true, task.steam64))
        return;

    std::vector<uint8_t> gif;
    GifFetch fetch = FetchAnimatedGif(task.steam64, m_animatedWorker.StopFlag(), gif);

    if (fetch == GifFetch::Failed)
        return;

    if (fetch == GifFetch::NoAnimation)
    {
        MarkDownloaded(true, task.steam64);
        return;
    }

    DownloadedAvatar result;
    result.playerIndex = task.playerIndex;
    result.steam64 = task.steam64;
    result.isAnimated = true;

    if (!DecodeGif(gif, result))
        return;

    MarkDownloaded(true, task.steam64);
    PushCompleted(std::move(result));
}

void CAvatarCache::PushCompleted(DownloadedAvatar&& data)
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    m_completed.push_back(std::move(data));
}

bool CAvatarCache::IsDownloaded(bool animated, SteamID64 id)
{
    std::lock_guard<std::mutex> lock(m_downloadedMutex);
    return (animated ? m_downloadedAnimated : m_downloadedStatic).count(id) != 0;
}

void CAvatarCache::MarkDownloaded(bool animated, SteamID64 id)
{
    std::lock_guard<std::mutex> lock(m_downloadedMutex);
    (animated ? m_downloadedAnimated : m_downloadedStatic).insert(id);
}

void CAvatarCache::InvalidateDownloaded(SteamID64 id)
{
    std::lock_guard<std::mutex> lock(m_downloadedMutex);
    m_downloadedStatic.erase(id);
    m_downloadedAnimated.erase(id);
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
                entry.animFrame = (entry.animFrame + 1) % (int)entry.frames.size();
            }
        }

        return entry.frames[entry.animFrame].texture;
    }

    if (entry.texture)
        return entry.texture;

    return m_pNoAvatar.texture;
}