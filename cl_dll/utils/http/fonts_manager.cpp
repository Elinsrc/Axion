#include "fonts_manager.h"
#include "web_client.h"
#include "hud.h"
#include "cl_util.h"

#include <filesystem>
#include <fstream>
#include <cstdio>

namespace fs = std::filesystem;

#define FONT_BASE_URL "https://raw.githubusercontent.com/openmaptiles/fonts/master/noto-sans/"
#define MIN_FONT_SIZE 1024
#define ATLAS_REBUILD_INTERVAL_SEC 2

CFontsManager g_FontsManager;

static fs::path CacheDir()
{
    return fs::path(gEngfuncs.pfnGetGameDirectory()) / "fonts_cache";
}

static std::string FormatSize(long long bytes)
{
    char buf[32];
    if (bytes >= 1024 * 1024)
        snprintf(buf, sizeof(buf), "%.1f MB", (double)bytes / (1024.0 * 1024.0));
    else
        snprintf(buf, sizeof(buf), "%lld KB", bytes / 1024);
    return buf;
}

void CFontsManager::Init()
{
    m_entries = {
        {"NotoSans-Regular.ttf"},
        {"NotoSansThai-Regular.ttf"},
        {"NotoSansCJKtc-Regular.otf"},
        {"NotoNaskhArabic-Regular.ttf"},
    };

    std::error_code ec;
    fs::create_directories(CacheDir(), ec);
    if (ec)
        gEngfuncs.Con_Printf("[Fonts] Warning: can't create %s: %s\n", CacheDir().string().c_str(), ec.message().c_str());

    m_worker.Start();

    for (auto &e : m_entries)
    {
        if (IsValidFile(e.file))
        {
            e.ready = true;
            continue;
        }

        fs::remove(CacheDir() / e.file, ec);
        StartDownload(e);
    }
}

void CFontsManager::Shutdown()
{
    m_worker.Stop();
}

bool CFontsManager::Update()
{
    PrintProgress();

    std::vector<DownloadResult> done;
    {
        std::lock_guard<std::mutex> lock(m_doneMutex);
        done.swap(m_done);
    }

    for (auto &d : done)
        ApplyResult(d);

    auto now = std::chrono::steady_clock::now();
    if (m_dirty && now - m_lastRebuild > std::chrono::seconds(ATLAS_REBUILD_INTERVAL_SEC))
    {
        m_dirty = false;
        m_lastRebuild = now;
        gEngfuncs.Con_Printf("[Fonts] Rebuilding font atlas\n");
        return true;
    }
    return false;
}

std::string CFontsManager::PathOf(const std::string &file) const
{
    return (CacheDir() / file).string();
}

bool CFontsManager::IsValidFile(const std::string &file) const
{
    std::error_code ec;
    fs::path p = CacheDir() / file;

    auto size = fs::file_size(p, ec);
    if (ec || size < MIN_FONT_SIZE)
        return false;

    std::ifstream f(p, std::ios::binary);
    uint8_t h[4] = {0};
    if (!f.read((char *)h, 4))
        return false;

    uint32_t tag = (uint32_t(h[0]) << 24) | (uint32_t(h[1]) << 16) | (uint32_t(h[2]) << 8) | h[3];
    return tag == 0x00010000 || tag == 0x4F54544F || tag == 0x74727565;
}

bool CFontsManager::SaveFile(const std::string &file, const std::vector<uint8_t> &data)
{
    std::error_code ec;
    fs::path dst = CacheDir() / file;
    fs::path tmp = CacheDir() / (file + ".tmp");

    bool written;
    {
        std::ofstream f(tmp, std::ios::binary);
        written = (bool)f.write((const char *)data.data(), (std::streamsize)data.size());
    }

    if (written)
        fs::rename(tmp, dst, ec);

    if (written && !ec && IsValidFile(file))
        return true;

    fs::remove(tmp, ec);
    fs::remove(dst, ec);
    return false;
}

CFontsManager::Entry *CFontsManager::FindEntry(const std::string &file)
{
    for (auto &e : m_entries)
        if (e.file == file)
            return &e;
    return nullptr;
}

bool CFontsManager::IsReady(const std::string &file) const
{
    for (auto &e : m_entries)
        if (e.file == file)
            return e.ready;
    return false;
}

void CFontsManager::MergeInto(float sizePixels, const std::string &skipFile)
{
    ImGuiIO &io = ImGui::GetIO();

    for (auto &e : m_entries)
    {
        if (!e.ready || e.file == skipFile)
            continue;

        ImFontConfig cfg;
        cfg.MergeMode = true;
        cfg.PixelSnapH = true;

        ImFont *f = io.Fonts->AddFontFromFileTTF(PathOf(e.file).c_str(), sizePixels, &cfg);
        if (!f)
            gEngfuncs.Con_Printf("[Fonts] Failed to load %s from fonts_cache\n", e.file.c_str());
    }
}

void CFontsManager::StartDownload(Entry &e)
{
    e.downloading = true;
    gEngfuncs.Con_Printf("[Fonts] Queued download: %s\n", e.file.c_str());

    const std::string file = e.file;
    m_worker.Post([this, file]() { DownloadFont(file); });
}

void CFontsManager::DownloadFont(const std::string &file)
{
    {
        std::lock_guard<std::mutex> lock(m_progressMutex);
        m_progress.file = file;
        m_progress.now = 0;
        m_progress.total = 0;
        m_progress.active = true;
    }

    HttpRequest req;
    req.url = std::string(FONT_BASE_URL) + file;
    req.timeoutSec = 0;
    req.connectTimeoutSec = 10;
    req.lowSpeedLimit = 1024;
    req.lowSpeedTimeSec = 20;
    req.followRedirects = true;
    req.cancel = &m_worker.StopFlag();
    req.onProgress = [this](long long now, long long total)
    {
        std::lock_guard<std::mutex> lock(m_progressMutex);
        m_progress.now = now;
        m_progress.total = total;
    };

    HttpResponse res = g_WebClient.Get(req);

    {
        std::lock_guard<std::mutex> lock(m_progressMutex);
        m_progress.active = false;
    }

    DownloadResult out;
    out.file = file;
    out.data = std::move(res.body);
    out.error = res.error;

    std::lock_guard<std::mutex> lock(m_doneMutex);
    m_done.push_back(std::move(out));
}

void CFontsManager::ApplyResult(DownloadResult &d)
{
    Entry *e = FindEntry(d.file);
    if (!e)
        return;

    e->downloading = false;

    if (!d.error.empty() || d.data.size() < MIN_FONT_SIZE)
    {
        gEngfuncs.Con_Printf("[Fonts] Download failed: %s (%s)\n", d.file.c_str(), d.error.empty() ? "empty response" : d.error.c_str());
        return;
    }

    if (!SaveFile(d.file, d.data))
    {
        gEngfuncs.Con_Printf("[Fonts] Failed to save/validate %s\n", d.file.c_str());
        return;
    }

    e->ready = true;
    m_dirty = true;
    gEngfuncs.Con_Printf("[Fonts] Downloaded %s (%s), saved to fonts_cache\n", d.file.c_str(), FormatSize((long long)d.data.size()).c_str());
}

void CFontsManager::PrintProgress()
{
    Progress p;
    {
        std::lock_guard<std::mutex> lock(m_progressMutex);
        p = m_progress;
    }

    if (!p.active)
    {
        m_progressFile.clear();
        return;
    }

    auto t = std::chrono::steady_clock::now();
    if (p.file == m_progressFile && t - m_lastProgress < std::chrono::seconds(1))
        return;

    m_progressFile = p.file;
    m_lastProgress = t;

    if (p.total > 0)
        gEngfuncs.Con_Printf("[Fonts] Downloading %s: %d%% (%s / %s)\n", p.file.c_str(), (int)(p.now * 100 / p.total), FormatSize(p.now).c_str(), FormatSize(p.total).c_str());
    else
        gEngfuncs.Con_Printf("[Fonts] Downloading %s: %s\n", p.file.c_str(), FormatSize(p.now).c_str());
}