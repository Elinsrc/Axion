#include "avatar_cache.h"

#include "build.h"
#include "hud.h"
#include "cl_util.h"

#ifdef XASH_WIN32
#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#endif

#include <cstring>
#include <fstream>
#include <ctime>
#include <chrono>

#include "noavatar.h"

namespace fs = std::filesystem;

ImGuiImage m_pNoAvatar;
CAvatarCache g_AvatarCache;

std::string CAvatarCache::ProfileUrl(SteamID64 id)
{
    return "https://steamcommunity.com/profiles/" + std::to_string(id);
}

std::string CAvatarCache::ExtractXmlTag(const std::string& xml, const std::string& tag)
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

std::string CAvatarCache::ExtractAnimatedAvatarUrl(const std::string& html)
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

std::string CAvatarCache::ExtractAvatarHash(const std::string& url)
{
    size_t slash = url.find_last_of('/');
    std::string file = (slash == std::string::npos) ? url : url.substr(slash + 1);

    size_t dot = file.find_last_of('.');
    if (dot != std::string::npos)
        file = file.substr(0, dot);

    size_t us = file.find_last_of('_');
    if (us != std::string::npos)
        file = file.substr(0, us);

    return file;
}

HttpRequest CAvatarCache::MakeRequest(const std::string& url, long timeoutSec, const std::atomic<bool>& cancel)
{
    HttpRequest req;
    req.url = url;
    req.timeoutSec = timeoutSec;
    req.cancel = &cancel;
    return req;
}

CAvatarCache::GifFetch CAvatarCache::FetchAnimatedGif(SteamID64 id, const std::atomic<bool>& cancel, std::vector<uint8_t>& out)
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

bool CAvatarCache::WriteAvcFile(const fs::path& path, bool isAnimated, const std::string& hash, const DownloadedAvatar& data)
{
    fs::path tmpPath = path;
    tmpPath += ".tmp";

    std::ofstream f(tmpPath, std::ios::binary | std::ios::trunc);
    if (!f)
        return false;

    AvcFileHeader hdr{};
    memcpy(hdr.magic, AVATAR_CACHE_MAGIC, 4);
    hdr.version = AVATAR_CACHE_VERSION;
    hdr.isAnimated = isAnimated ? 1 : 0;
    hdr.hashLen = (uint16_t)hash.size();

    f.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    f.write(hash.data(), (std::streamsize)hash.size());

    if (!isAnimated)
    {
        uint32_t size = (uint32_t)data.imageData.size();
        f.write(reinterpret_cast<const char*>(&size), sizeof(size));
        f.write(reinterpret_cast<const char*>(data.imageData.data()), size);
    }
    else
    {
        uint32_t w = (uint32_t)data.gifWidth;
        uint32_t h = (uint32_t)data.gifHeight;
        uint32_t frameCount = (uint32_t)data.gifFrames.size();
        f.write(reinterpret_cast<const char*>(&w), sizeof(w));
        f.write(reinterpret_cast<const char*>(&h), sizeof(h));
        f.write(reinterpret_cast<const char*>(&frameCount), sizeof(frameCount));

        for (size_t i = 0; i < data.gifFrames.size(); i++)
        {
            float delay = data.gifDelays[i];
            uint32_t dataSize = (uint32_t)data.gifFrames[i].size();
            f.write(reinterpret_cast<const char*>(&delay), sizeof(delay));
            f.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
            f.write(reinterpret_cast<const char*>(data.gifFrames[i].data()), dataSize);
        }
    }

    f.close();
    if (!f)
    {
        std::error_code ec;
        fs::remove(tmpPath, ec);
        return false;
    }

    std::error_code ec;
    fs::rename(tmpPath, path, ec);
    if (ec)
    {
        fs::remove(tmpPath, ec);
        return false;
    }

    return true;
}

bool CAvatarCache::ReadAvcFile(const fs::path& path, DownloadedAvatar& out, std::string& hashOut, bool& isAnimatedOut)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;

    AvcFileHeader hdr{};
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (!f || memcmp(hdr.magic, AVATAR_CACHE_MAGIC, 4) != 0 || hdr.version != AVATAR_CACHE_VERSION)
        return false;

    hashOut.resize(hdr.hashLen);
    if (hdr.hashLen)
        f.read(hashOut.data(), hdr.hashLen);

    isAnimatedOut = hdr.isAnimated != 0;

    if (!isAnimatedOut)
    {
        uint32_t size = 0;
        f.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!f || size == 0 || size > 16 * 1024 * 1024)
            return false;

        out.imageData.resize(size);
        f.read(reinterpret_cast<char*>(out.imageData.data()), size);
    }
    else
    {
        uint32_t w = 0, h = 0, frameCount = 0;
        f.read(reinterpret_cast<char*>(&w), sizeof(w));
        f.read(reinterpret_cast<char*>(&h), sizeof(h));
        f.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));

        if (!f || frameCount == 0 || frameCount > 4096 || w == 0 || h == 0)
            return false;

        out.gifWidth = (int)w;
        out.gifHeight = (int)h;
        out.gifFrames.resize(frameCount);
        out.gifDelays.resize(frameCount);

        for (uint32_t i = 0; i < frameCount; i++)
        {
            float delay = 0.0f;
            uint32_t dataSize = 0;
            f.read(reinterpret_cast<char*>(&delay), sizeof(delay));
            f.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

            if (!f || dataSize != w * h * 4)
                return false;

            out.gifFrames[i].resize(dataSize);
            f.read(reinterpret_cast<char*>(out.gifFrames[i].data()), dataSize);
            out.gifDelays[i] = delay;
        }
    }

    out.isAnimated = isAnimatedOut;
    return true;
}

void CAvatarCache::AvatarCacheInfo_f()
{
    g_AvatarCache.PrintCacheInfo();
}

void CAvatarCache::Initialize()
{
    for (int i = 0; i < MAX_AVATAR_PLAYERS; i++)
        m_avatars[i] = AvatarEntry{};

    InitDiskCache();

    m_staticWorker.Start();
    m_animatedWorker.Start();

    m_staticWorker.Post([this]() { RunStartupCleanup(); });

    gEngfuncs.pfnAddCommand("avatarcache_info", AvatarCacheInfo_f);
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

fs::path CAvatarCache::AvatarsCacheDir()
{
    return fs::path(gEngfuncs.pfnGetGameDirectory()) / AVATAR_CACHE_DIRNAME;
}

fs::path CAvatarCache::GetAvcPath(SteamID64 steam64) const
{
    return AvatarsCacheDir() / (std::to_string(steam64) + ".avc");
}

void CAvatarCache::InitDiskCache()
{
    std::error_code ec;
    fs::create_directories(AvatarsCacheDir(), ec);
}

void CAvatarCache::RunStartupCleanup()
{
    std::lock_guard<std::mutex> lock(m_diskMutex);

    std::error_code ec;
    const fs::path dir = AvatarsCacheDir();

    if (!fs::exists(dir, ec))
        return;

    const auto fsNow = fs::file_time_type::clock::now();
    int removed = 0;

    for (auto it = fs::directory_iterator(dir, ec); !ec && it != fs::directory_iterator(); it.increment(ec))
    {
        const auto& path = it->path();

        if (!it->is_regular_file() || path.extension() != ".avc")
            continue;

        std::error_code wtEc;
        const auto mtime = fs::last_write_time(path, wtEc);
        if (wtEc)
            continue;

        const int64_t unusedDays = std::chrono::duration_cast<std::chrono::hours>(fsNow - mtime).count() / 24;

        if (unusedDays >= AVATAR_CACHE_MAX_UNUSED_DAYS)
        {
            std::error_code rmEc;
            fs::remove(path, rmEc);
            if (!rmEc)
                removed++;
        }
    }

    if (removed > 0)
        gEngfuncs.Con_DPrintf("[AvatarCache] startup cleanup: removed %d unused cache files\n", removed);
}

bool CAvatarCache::TryLoadFromDisk(int playerIndex, SteamID64 steam64)
{
    const fs::path path = GetAvcPath(steam64);

    DownloadedAvatar data;
    std::string hash;
    bool isAnimated = false;

    {
        std::lock_guard<std::mutex> lock(m_diskMutex);

        std::error_code ec;
        if (!fs::exists(path, ec))
            return false;

        if (!ReadAvcFile(path, data, hash, isAnimated))
        {
            fs::remove(path, ec);
            return false;
        }
    }

    AvatarEntry& entry = m_avatars[playerIndex];
    if (entry.steamId != steam64)
        return false;

    data.playerIndex = playerIndex;
    data.steam64 = steam64;

    if (isAnimated)
        ApplyAnimated(entry, data);
    else
        ApplyStatic(entry, data);

    entry.diskAvatarHash = hash;

    TouchAvcFile(steam64);
    return true;
}

void CAvatarCache::TouchAvcFile(SteamID64 steam64)
{
    std::lock_guard<std::mutex> lock(m_diskMutex);

    std::error_code ec;
    fs::last_write_time(GetAvcPath(steam64), fs::file_time_type::clock::now(), ec);
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

        if (!entry.diskAttempted)
        {
            entry.diskAttempted = true;
            TryLoadFromDisk(i, steam64);
        }

        if (!entry.requested)
        {
            entry.requested = true;
            QueueStatic({i, steam64, entry.diskAvatarHash});
        }
    }
}

void CAvatarCache::PrintCacheInfo()
{
    int staticCount = 0, animatedCount = 0, pendingCount = 0;
    int loadingCount = 0, failedCount = 0, totalFrames = 0;
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

    size_t diskFiles = 0;
    uintmax_t diskBytes = 0;

    {
        std::lock_guard<std::mutex> lock(m_diskMutex);

        std::error_code ec;
        for (auto it = fs::directory_iterator(AvatarsCacheDir(), ec); !ec && it != fs::directory_iterator(); it.increment(ec))
        {
            if (!it->is_regular_file() || it->path().extension() != ".avc")
                continue;

            diskFiles++;
            std::error_code sizeEc;
            diskBytes += fs::file_size(it->path(), sizeEc);
        }
    }

    gEngfuncs.Con_Printf("AvatarCache:\n");
    gEngfuncs.Con_Printf("  Players with static avatar: %d\n", staticCount);
    gEngfuncs.Con_Printf("  Players with animated avatar: %d\n", animatedCount);
    gEngfuncs.Con_Printf("  Total animation frames in memory: %d\n", totalFrames);
    gEngfuncs.Con_Printf("  Avatars waiting for download: %d\n", loadingCount);
    gEngfuncs.Con_Printf("  Avatars uploading to GPU: %d\n", pendingCount);
    gEngfuncs.Con_Printf("  Failed avatar downloads: %d\n", failedCount);
    gEngfuncs.Con_Printf("  Video memory used by avatars: %.2f MB\n", totalMem / (1024.0 * 1024.0));
    gEngfuncs.Con_Printf("  Avatars saved on disk (.avc): %zu\n", diskFiles);
    gEngfuncs.Con_Printf("  Disk cache size: %.2f MB\n", diskBytes / (1024.0 * 1024.0));
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

    m_ImguiUtils.FreeTexture(entry.texture);
    for (auto& f : entry.frames)
        m_ImguiUtils.FreeTexture(f.texture);

    entry = AvatarEntry{};
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
            if (!entry.loaded)
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

    m_ImguiUtils.FreeTexture(entry.texture);

    ImGuiImage img = m_ImguiUtils.LoadImageFromMemory(data.imageData.data(), (int)data.imageData.size());
    entry.texture = img.texture;
    entry.loaded = (entry.texture != 0);
}

void CAvatarCache::ApplyAnimated(AvatarEntry& entry, DownloadedAvatar& data)
{
    if (data.gifFrames.empty())
        return;

    for (auto& f : entry.frames)
        m_ImguiUtils.FreeTexture(f.texture);
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

            ImGuiImage img = m_ImguiUtils.LoadImageFromRGBA(entry.pendingFrameData[fi].data(), entry.pendingWidth, entry.pendingHeight);

            if (img.texture)
            {
                AvatarFrame af;
                af.texture = img.texture;
                af.delayMs = entry.pendingDelays[fi];
                entry.frames.push_back(af);
            }

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

void CAvatarCache::QueueStatic(const AvatarTask& task)
{
    m_staticWorker.Post([this, task]() { DownloadStatic(task); });
}

void CAvatarCache::DownloadStatic(const AvatarTask& task)
{
    HttpResponse xml = g_WebClient.Get(MakeRequest(ProfileUrl(task.steam64) + "?xml=1", 5, m_staticWorker.StopFlag()));
    if (!xml.ok)
    {
        g_WebClient.LogErrorOnce("avatar_" + std::to_string(task.steam64), "[AvatarCache] " + std::to_string(task.steam64) + ": can't get profile info: " + xml.error);

        DownloadedAvatar fail;
        fail.playerIndex = task.playerIndex;
        fail.steam64 = task.steam64;
        fail.failed = true;
        PushCompleted(std::move(fail));
        return;
    }

    std::string avatarUrl = ExtractXmlTag(xml.Text(), "avatarMedium");
    if (avatarUrl.empty())
        return;

    const std::string hash = ExtractAvatarHash(avatarUrl);

    if (!hash.empty() && hash == task.knownHash)
    {
        TouchAvcFile(task.steam64);
        return;
    }

    HttpResponse img = g_WebClient.Get(MakeRequest(avatarUrl, 5, m_staticWorker.StopFlag()));
    if (!img.ok || img.body.empty())
    {
        g_WebClient.LogErrorOnce("avatar_" + std::to_string(task.steam64),
            "[AvatarCache] " + std::to_string(task.steam64) + ": image download failed");
        return;
    }

    DownloadedAvatar result;
    result.playerIndex = task.playerIndex;
    result.steam64 = task.steam64;
    result.imageData = std::move(img.body);

    {
        std::lock_guard<std::mutex> lock(m_diskMutex);
        WriteAvcFile(GetAvcPath(task.steam64), false, hash, result);
    }

    PushCompleted(std::move(result));

    m_animatedWorker.Post([this, task, hash]() { DownloadAnimated(task, hash); });
}

void CAvatarCache::DownloadAnimated(const AvatarTask& task, const std::string& staticHash)
{
    std::vector<uint8_t> gif;
    GifFetch fetch = FetchAnimatedGif(task.steam64, m_animatedWorker.StopFlag(), gif);

    if (fetch != GifFetch::Ok)
        return;

    ImGuiGifImage gifImage;
    if (!m_ImguiUtils.LoadGifFromMemory(gif.data(), (int)gif.size(), AVATAR_TARGET_SIZE, gifImage))
        return;

    DownloadedAvatar result;
    result.playerIndex = task.playerIndex;
    result.steam64 = task.steam64;
    result.isAnimated = true;
    result.gifWidth = gifImage.width;
    result.gifHeight = gifImage.height;
    result.gifFrames.reserve(gifImage.frames.size());
    result.gifDelays.reserve(gifImage.frames.size());

    for (auto& f : gifImage.frames)
    {
        result.gifFrames.push_back(std::move(f.pixels));
        result.gifDelays.push_back(f.delayMs);
    }

    {
        std::lock_guard<std::mutex> lock(m_diskMutex);
        WriteAvcFile(GetAvcPath(task.steam64), true, staticHash, result);
    }

    PushCompleted(std::move(result));
}

void CAvatarCache::PushCompleted(DownloadedAvatar&& data)
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    m_completed.push_back(std::move(data));
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