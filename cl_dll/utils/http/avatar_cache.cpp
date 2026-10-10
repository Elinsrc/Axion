#include "avatar_cache.h"

#include "build.h"
#include "hud.h"
#include "cl_util.h"

#ifdef XASH_WIN32
#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#endif

#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <initializer_list>

#include "noavatar.h"

#include "custom_utils.h"

namespace fs = std::filesystem;

ImGuiImage m_pNoAvatar;
CAvatarCache g_AvatarCache;

namespace
{
    uint64_t Fnv1a64(const std::vector<uint8_t>& d)
    {
        uint64_t h = 0xcbf29ce484222325ull;
        for (uint8_t b : d)
        {
            h ^= b;
            h *= 1099511628211ull;
        }
        return h;
    }

    std::vector<uint8_t> ReadWholeFile(const fs::path& p)
    {
        std::ifstream f(p, std::ios::binary | std::ios::ate);
        if (!f)
            return {};

        const std::streamsize n = f.tellg();
        if (n <= 0 || (size_t)n > CUSTOM_AVATAR_MAX_BYTES)
            return {};

        std::vector<uint8_t> buf((size_t)n);
        f.seekg(0);
        f.read(reinterpret_cast<char*>(buf.data()), n);

        if (!f)
            return {};

        return buf;
    }

    std::string BuildMultipart(const std::string& boundary, const std::string& filename, const std::string& data)
    {
        std::string body;
        body.reserve(data.size() + 512);

        body += "--" + boundary + "\r\nContent-Disposition: form-data; name=\"reqtype\"\r\n\r\nfileupload\r\n";
        body += "--" + boundary + "\r\nContent-Disposition: form-data; name=\"fileToUpload\"; filename=\"" + filename + "\"\r\nContent-Type: application/octet-stream\r\n\r\n";
        body += data;
        body += "\r\n--" + boundary + "--\r\n";

        return body;
    }

    bool ValidateCustomImage(const std::vector<uint8_t>& d, bool& isGif)
    {
        isGif = false;

        if (d.size() < 16 || d.size() > CUSTOM_AVATAR_MAX_BYTES)
            return false;

        int w = 0, h = 0;

        if (memcmp(d.data(), "GIF8", 4) == 0)
        {
            isGif = true;

            if (!CustomUtils::GifSize(d.data(), d.size(), w, h))
                return false;

            const int frames = CustomUtils::CountGifFrames(d.data(), d.size());
            if (frames < 1 || frames > CUSTOM_AVATAR_MAX_FRAMES)
                return false;
        }
        else if (!CustomUtils::PngSize(d.data(), d.size(), w, h) && !CustomUtils::JpegSize(d.data(), d.size(), w, h))
        {
            return false;
        }

        return w > 0 && h > 0 && w <= CUSTOM_AVATAR_MAX_SIZE && h <= CUSTOM_AVATAR_MAX_SIZE;
    }
}

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

bool CAvatarCache::WriteAvcStream(std::ostream& f, bool isAnimated, const std::string& hash, const DownloadedAvatar& data)
{
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

    return (bool)f;
}

bool CAvatarCache::WriteAvcFile(const fs::path& path, bool isAnimated, const std::string& hash, const DownloadedAvatar& data)
{
    fs::path tmpPath = path;
    tmpPath += ".tmp";

    std::ofstream f(tmpPath, std::ios::binary | std::ios::trunc);
    if (!f)
        return false;

    const bool ok = WriteAvcStream(f, isAnimated, hash, data);
    f.close();

    std::error_code ec;
    if (!ok || !f)
    {
        fs::remove(tmpPath, ec);
        return false;
    }

    fs::rename(tmpPath, path, ec);
    if (ec)
    {
        fs::remove(tmpPath, ec);
        return false;
    }

    return true;
}

bool CAvatarCache::ReadAvcStream(std::istream& f, DownloadedAvatar& out, std::string& hashOut, bool& isAnimatedOut)
{
    AvcFileHeader hdr{};
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (!f || memcmp(hdr.magic, AVATAR_CACHE_MAGIC, 4) != 0 || hdr.version != AVATAR_CACHE_VERSION)
        return false;

    hashOut.resize(hdr.hashLen);
    if (hdr.hashLen)
    {
        f.read(hashOut.data(), hdr.hashLen);
        if (!f)
            return false;
    }

    isAnimatedOut = hdr.isAnimated != 0;

    if (!isAnimatedOut)
    {
        uint32_t size = 0;
        f.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!f || size == 0 || size > 16 * 1024 * 1024)
            return false;

        out.imageData.resize(size);
        f.read(reinterpret_cast<char*>(out.imageData.data()), size);
        if (!f)
            return false;
    }
    else
    {
        uint32_t w = 0, h = 0, frameCount = 0;
        f.read(reinterpret_cast<char*>(&w), sizeof(w));
        f.read(reinterpret_cast<char*>(&h), sizeof(h));
        f.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));

        if (!f || frameCount == 0 || frameCount > 4096 || w == 0 || h == 0 || w > 256 || h > 256)
            return false;

        if ((uint64_t)w * h * 4 * frameCount > 32ull * 1024 * 1024)
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

            if (!f || (uint64_t)dataSize != (uint64_t)w * h * 4)
                return false;

            out.gifFrames[i].resize(dataSize);
            f.read(reinterpret_cast<char*>(out.gifFrames[i].data()), dataSize);
            if (!f)
                return false;

            if (!(delay >= 10.0f))
                delay = 10.0f;
            if (delay > 10000.0f)
                delay = 10000.0f;

            out.gifDelays[i] = delay;
        }
    }

    out.isAnimated = isAnimatedOut;
    return true;
}

bool CAvatarCache::ReadAvcFile(const fs::path& path, DownloadedAvatar& out, std::string& hashOut, bool& isAnimatedOut)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;

    return ReadAvcStream(f, out, hashOut, isAnimatedOut);
}

bool CAvatarCache::ValidateCustomAvatar(const DownloadedAvatar& d)
{
    if (d.isAnimated)
    {
        return d.gifWidth > 0 && d.gifHeight > 0
            && d.gifWidth <= AVATAR_TARGET_SIZE && d.gifHeight <= AVATAR_TARGET_SIZE
            && !d.gifFrames.empty()
            && (int)d.gifFrames.size() <= CUSTOM_AVATAR_MAX_FRAMES
            && d.gifDelays.size() == d.gifFrames.size();
    }

    const auto& b = d.imageData;
    if (b.size() < 16 || b.size() > 256 * 1024)
        return false;

    int w = 0, h = 0;
    if (!CustomUtils::PngSize(b.data(), b.size(), w, h) && !CustomUtils::JpegSize(b.data(), b.size(), w, h))
        return false;

    return w > 0 && h > 0 && w <= AVATAR_TARGET_SIZE && h <= AVATAR_TARGET_SIZE;
}

void CAvatarCache::AvatarCacheInfo_f()
{
    g_AvatarCache.PrintCacheInfo();
}

void CAvatarCache::AvatarUpload_f()
{
    g_AvatarCache.SyncCustomAvatar();
}

void CAvatarCache::Initialize()
{
    for (int i = 0; i < MAX_AVATAR_PLAYERS; i++)
        m_avatars[i] = AvatarEntry{};

    InitDiskCache();

    CVAR_CREATE("avatar_key", "", FCVAR_USERINFO);
    CVAR_CREATE("cl_custom_avatars", "1", FCVAR_ARCHIVE);

    m_staticWorker.Start();
    m_animatedWorker.Start();
    m_customWorker.Start();
    m_uploadWorker.Start();

    m_staticWorker.Post([this]() { RunStartupCleanup(); });

    gEngfuncs.pfnAddCommand("avatarcache_info", AvatarCacheInfo_f);
    gEngfuncs.pfnAddCommand("avatar_upload", AvatarUpload_f);

    SyncCustomAvatar();
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
    m_customWorker.Stop();
    m_uploadWorker.Stop();

    ClearAll();
    m_ImguiUtils.FreeImage(m_pNoAvatar);
}

fs::path CAvatarCache::AvatarsCacheDir()
{
    return fs::path(gEngfuncs.pfnGetGameDirectory()) / AVATAR_CACHE_DIRNAME;
}

fs::path CAvatarCache::CustomAvatarDir()
{
    return fs::path(gEngfuncs.pfnGetGameDirectory()) / CUSTOM_AVATAR_DIRNAME;
}

fs::path CAvatarCache::GetAvcPath(SteamID64 steam64) const
{
    return AvatarsCacheDir() / (std::to_string(steam64) + ".avc");
}

fs::path CAvatarCache::GetCustomAvcPath(const std::string& name) const
{
    return AvatarsCacheDir() / name;
}

bool CAvatarCache::IsValidCustomName(const std::string& s)
{
    const size_t dot = s.find('.');
    if (dot == std::string::npos || dot < 4 || dot > 12)
        return false;

    for (size_t i = 0; i < dot; i++)
    {
        const char c = s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')))
            return false;
    }

    return s.substr(dot + 1) == "avc";
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
    if (entry.steamId != steam64 || entry.customActive)
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

void CAvatarCache::SyncCustomAvatar()
{
    if (m_uploading.exchange(true))
        return;

    m_uploadWorker.Post([this]()
    {
        ++m_uploadSeq;
        UploadCustomAvatar();
        m_uploading = false;
    });
}

void CAvatarCache::QueuePublish(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_publishMutex);
    m_publishName = name;
    m_publishPending = true;
}

void CAvatarCache::ApplyPublish()
{
    std::string name;

    {
        std::lock_guard<std::mutex> lock(m_publishMutex);
        if (!m_publishPending)
            return;

        name = std::move(m_publishName);
        m_publishPending = false;
    }

    gEngfuncs.PlayerInfo_SetValueForKey("avatar_key", name.c_str());
}

bool CAvatarCache::DecodeCustom(const std::vector<uint8_t>& data, bool isGif, DownloadedAvatar& out)
{
    ImGuiGifImage gif;

    const bool ok = isGif
        ? m_ImguiUtils.LoadGifFromMemory(data.data(), (int)data.size(), AVATAR_TARGET_SIZE, gif)
        : m_ImguiUtils.LoadStaticFromMemory(data.data(), (int)data.size(), AVATAR_TARGET_SIZE, gif);

    if (!ok || gif.frames.empty() || (int)gif.frames.size() > CUSTOM_AVATAR_MAX_FRAMES)
        return false;

    out.isAnimated = true;
    out.gifWidth = gif.width;
    out.gifHeight = gif.height;
    out.gifFrames.reserve(gif.frames.size());
    out.gifDelays.reserve(gif.frames.size());

    for (auto& f : gif.frames)
    {
        out.gifFrames.push_back(std::move(f.pixels));
        out.gifDelays.push_back(f.delayMs);
    }

    return true;
}

void CAvatarCache::QueueLocal(DownloadedAvatar&& data, bool has)
{
    std::lock_guard<std::mutex> lock(m_localMutex);
    m_localData = std::move(data);
    m_hasLocal = has;
    ++m_localVersion;
}

void CAvatarCache::ApplyLocal(AvatarEntry& entry, bool allowCustom)
{
    if (!allowCustom)
    {
        if (entry.localActive)
        {
            ResetVisuals(entry);
            entry.localActive = false;
            entry.customActive = false;
            entry.localVersion = 0;

            entry.diskAttempted = false;
            entry.requested = false;
        }
        return;
    }

    DownloadedAvatar data;
    bool has = false;

    {
        std::lock_guard<std::mutex> lock(m_localMutex);

        if (entry.localVersion == m_localVersion)
            return;

        entry.localVersion = m_localVersion;
        has = m_hasLocal;

        if (has)
            data = m_localData;
    }

    ResetVisuals(entry);

    entry.localActive = false;
    entry.customActive = false;
    entry.customName.clear();
    entry.customRequested = false;

    entry.diskAttempted = false;
    entry.requested = false;

    if (!has)
        return;

    entry.customActive = true;

    if (data.isAnimated)
        ApplyAnimated(entry, data);
    else
        ApplyStatic(entry, data);

    if (!entry.loaded && !entry.isPending)
    {
        entry.customActive = false;
        return;
    }

    entry.localActive = true;
}

void CAvatarCache::UploadCustomAvatar()
{
    const std::string logKey = "avatar_up_" + std::to_string(m_uploadSeq.load());

    const fs::path dir = CustomAvatarDir();
    const fs::path recPath = dir / ".uploaded";

    std::error_code ec;
    fs::create_directories(dir, ec);

    fs::path file;
    for (const char* n : { "avatar.gif", "avatar.png", "avatar.jpg", "avatar.jpeg" })
    {
        if (fs::is_regular_file(dir / n, ec))
        {
            file = dir / n;
            break;
        }
    }

    if (file.empty())
    {
        m_localHash.clear();
        QueueLocal(DownloadedAvatar{}, false);

        if (fs::exists(recPath, ec))
        {
            fs::remove(recPath, ec);
            QueuePublish("");
        }
        return;
    }

    const std::vector<uint8_t> original = ReadWholeFile(file);

    bool isGif = false;
    if (!ValidateCustomImage(original, isGif))
    {
        g_WebClient.LogErrorOnce(logKey, "[AvatarCache] avatar file is invalid: png/jpg/gif, up to 2 MB, up to 512x512" + std::string(isGif ? ", up to 200 frames" : ""));
        return;
    }

    DownloadedAvatar local;

    if (!DecodeCustom(original, isGif, local))
    {
        g_WebClient.LogErrorOnce(logKey, "[AvatarCache] failed to decode avatar");
        return;
    }

    char hashBuf[17];
    snprintf(hashBuf, sizeof(hashBuf), "%016llx", (unsigned long long)Fnv1a64(original));
    const std::string hash = hashBuf;

    std::ostringstream ss(std::ios::binary);
    if (!WriteAvcStream(ss, local.isAnimated, hash, local))
    {
        g_WebClient.LogErrorOnce(logKey, "[AvatarCache] failed to build .avc");
        return;
    }

    const std::string avc = ss.str();
    if (avc.size() > CUSTOM_AVC_MAX_BYTES)
    {
        g_WebClient.LogErrorOnce(logKey, "[AvatarCache] avatar is too large after conversion (reduce frame count)");
        return;
    }

    if (hash != m_localHash)
    {
        m_localHash = hash;
        QueueLocal(std::move(local), true);
    }

    std::string recHash, recName;
    {
        std::ifstream r(recPath);
        r >> recHash >> recName;
    }

    if (recHash == hash && IsValidCustomName(recName))
    {
        QueuePublish(recName);
        return;
    }

    const std::string boundary = "----avatar" + hash;

    HttpRequest req = MakeRequest(CUSTOM_AVATAR_UPLOAD_URL, 120, m_uploadWorker.StopFlag());
    req.method = "POST";
    req.headers.push_back("Content-Type: multipart/form-data; boundary=" + boundary);
    req.postBody = BuildMultipart(boundary, "avatar.avc", avc);
    req.maxBytes = 4096;
    req.followRedirects = false;

    HttpResponse resp = g_WebClient.Post(req);

    std::string url = resp.Text();
    while (!url.empty() && isspace((unsigned char)url.back()))
        url.pop_back();

    if (!resp.ok || url.rfind(CUSTOM_AVATAR_HOST, 0) != 0)
    {
        g_WebClient.LogErrorOnce(logKey, "[AvatarCache] avatar upload failed: " + (resp.ok ? url : resp.error));
        return;
    }

    const std::string name = url.substr(strlen(CUSTOM_AVATAR_HOST));
    if (!IsValidCustomName(name))
    {
        g_WebClient.LogErrorOnce(logKey, "[AvatarCache] avatar upload returned unexpected name: " + name);
        return;
    }

    {
        std::ofstream w(recPath, std::ios::trunc);
        w << hash << ' ' << name;
    }

    g_WebClient.LogErrorOnce(logKey, "[AvatarCache] avatar uploaded (public link): " + url);
    QueuePublish(name);
}

void CAvatarCache::UpdateCustomState(int playerIndex, AvatarEntry& entry, SteamID64 steam64, bool allowCustom, int localIndex)
{
    if (playerIndex == localIndex && entry.localActive)
        return;

    const char* raw = (playerIndex == localIndex) ? gEngfuncs.pfnGetCvarString("avatar_key") : gEngfuncs.PlayerInfo_ValueForKey(playerIndex, "avatar_key");

    std::string want;
    if (allowCustom && raw && IsValidCustomName(raw))
        want = raw;

    if (want != entry.customName)
    {
        if (entry.customActive)
            ResetVisuals(entry);

        entry.customName = want;
        entry.customActive = false;
        entry.customRequested = false;
    }

    const bool steamUnavailable = steam64 == 0 || entry.failed;

    if (!entry.customName.empty() && !entry.customRequested && steamUnavailable)
    {
        entry.customRequested = true;
        QueueCustom(playerIndex, entry.customName);
    }
}

void CAvatarCache::Update()
{
    g_WebClient.FlushLogs();

    ApplyPublish();
    ProcessDownloadedAvatars();
    ProcessPendingTextures();

    const bool allowCustom = gEngfuncs.pfnGetCvarFloat("cl_custom_avatars") != 0.0f;

    cl_entity_t* local = gEngfuncs.GetLocalPlayer();
    const int localIndex = local ? local->index : 0;

    for (int i = 1; i <= gEngfuncs.GetMaxClients(); i++)
    {
        m_CustomUtils.UpdatePlayerInfo(i);

        if (g_PlayerIsBot[i])
            continue;

        const SteamID64 steam64 = g_PlayerSteamID64[i];
        AvatarEntry& entry = m_avatars[i];

        if (entry.steamId != steam64)
        {
            ClearAvatar(i);
            entry.steamId = steam64;
        }

        if (i == localIndex)
            ApplyLocal(entry, allowCustom);

        UpdateCustomState(i, entry, steam64, allowCustom, localIndex);

        if (steam64 == 0)
            continue;

        if (entry.localActive)
            continue;

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
    int customCount = 0;
    size_t totalMem = 0;

    for (int i = 1; i < MAX_AVATAR_PLAYERS; i++)
    {
        const AvatarEntry& entry = m_avatars[i];
        if (entry.steamId == 0 && entry.customName.empty() && !entry.localActive)
            continue;

        if (entry.customActive)
            customCount++;

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
        else if (entry.requested || entry.customRequested)
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
    gEngfuncs.Con_Printf("  Players with custom avatar: %d\n", customCount);
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

void CAvatarCache::ResetVisuals(AvatarEntry& entry)
{
    m_ImguiUtils.FreeTexture(entry.texture);
    entry.texture = 0;

    for (auto& f : entry.frames)
        m_ImguiUtils.FreeTexture(f.texture);
    entry.frames.clear();

    entry.pendingFrameData.clear();
    entry.pendingDelays.clear();
    entry.pendingFrameCount = 0;
    entry.pendingFrameIndex = 0;
    entry.pendingWidth = 0;
    entry.pendingHeight = 0;

    entry.isPending = false;
    entry.isAnimated = false;
    entry.loaded = false;
    entry.totalDurationMs = 0.0f;
    entry.animFrame = 0;
    entry.animAccumMs = 0.0f;
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

        if (data.custom)
        {
            if (entry.customName != data.customName)
                continue;

            if (entry.steamId != 0 && !entry.failed)
                continue;

            ApplyCustom(entry, data);
            continue;
        }

        if (entry.steamId != data.steam64 || entry.customActive)
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

void CAvatarCache::ApplyCustom(AvatarEntry& entry, DownloadedAvatar& data)
{
    ResetVisuals(entry);
    entry.customActive = true;

    if (data.isAnimated)
        ApplyAnimated(entry, data);
    else
        ApplyStatic(entry, data);

    if (!entry.loaded && !entry.isPending)
        entry.customActive = false;
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
            else if (entry.customActive && entry.frames.size() == 1)
            {
                entry.texture = entry.frames[0].texture;
                entry.frames.clear();
                entry.loaded = true;
            }
            else if (entry.customActive)
            {
                entry.customActive = false;
                entry.localActive = false;
            }
        }

        break;
    }
}

void CAvatarCache::QueueStatic(const AvatarTask& task)
{
    m_staticWorker.Post([this, task]() { DownloadStatic(task); });
}

void CAvatarCache::QueueCustom(int playerIndex, const std::string& name)
{
    m_customWorker.Post([this, playerIndex, name]() { DownloadCustom(playerIndex, name); });
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

void CAvatarCache::DownloadCustom(int playerIndex, const std::string& name)
{
    const fs::path avc = GetCustomAvcPath(name);

    DownloadedAvatar cached;
    bool hit = false;

    {
        std::lock_guard<std::mutex> lock(m_diskMutex);

        std::error_code ec;
        if (fs::exists(avc, ec))
        {
            std::string storedName;
            bool storedAnimated = false;

            if (ReadAvcFile(avc, cached, storedName, storedAnimated))
            {
                hit = true;
                fs::last_write_time(avc, fs::file_time_type::clock::now(), ec);
            }
            else
            {
                cached = DownloadedAvatar{};
                fs::remove(avc, ec);
            }
        }
    }

    if (hit)
    {
        cached.playerIndex = playerIndex;
        cached.custom = true;
        cached.customName = name;
        PushCompleted(std::move(cached));
        return;
    }

    const std::string logKey = "avatar_" + name;

    HttpRequest req = MakeRequest(std::string(CUSTOM_AVATAR_HOST) + name, 30, m_customWorker.StopFlag());
    req.maxBytes = CUSTOM_AVC_MAX_BYTES;
    req.followRedirects = false;

    HttpResponse resp = g_WebClient.Get(req);
    if (!resp.ok || resp.body.empty() || resp.body.size() > CUSTOM_AVC_MAX_BYTES)
    {
        g_WebClient.LogErrorOnce(logKey, "[AvatarCache] custom avatar " + name + ": download failed: " + resp.error);
        return;
    }

    DownloadedAvatar result;
    std::string storedHash;
    bool storedAnimated = false;

    std::istringstream in(std::string(resp.body.begin(), resp.body.end()), std::ios::binary);
    if (!ReadAvcStream(in, result, storedHash, storedAnimated) || !ValidateCustomAvatar(result))
    {
        g_WebClient.LogErrorOnce(logKey, "[AvatarCache] custom avatar " + name + ": rejected (invalid .avc)");
        return;
    }

    result.playerIndex = playerIndex;
    result.custom = true;
    result.customName = name;

    {
        std::lock_guard<std::mutex> lock(m_diskMutex);
        WriteAvcFile(avc, result.isAnimated, name, result);
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