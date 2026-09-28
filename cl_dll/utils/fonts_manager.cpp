#include "fonts_manager.h"
#include "web_client.h"
#include "hud.h"
#include "cl_util.h"

#include <filesystem>
#include <fstream>
#include <cstdio>

namespace fs = std::filesystem;

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

std::string CFontsManager::PathOf(const std::string &file) const
{
    return (CacheDir() / file).string();
}

bool CFontsManager::IsValidFile(const std::string &file) const
{
    std::error_code ec;
    fs::path p = CacheDir() / file;

    auto size = fs::file_size(p, ec);
    if (ec || size < 1024)
        return false;

    std::ifstream f(p, std::ios::binary);
    uint8_t h[4] = {0};
    if (!f.read((char *)h, 4))
        return false;

    uint32_t tag = (uint32_t(h[0]) << 24) | (uint32_t(h[1]) << 16) | (uint32_t(h[2]) << 8) | h[3];
    return tag == 0x00010000 || tag == 0x4F54544F || tag == 0x74727565;
}

void CFontsManager::StartDownload(Entry &e)
{
    e.downloading = true;
    gEngfuncs.Con_Printf("[Fonts] Queued download: %s\n", e.file.c_str());
    g_WebClient.QueueFontDownload(e.file);
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

    for (auto &e : m_entries)
    {
        if (IsValidFile(e.file))
        {
            e.ready = true;
        }
        else
        {
            fs::remove(CacheDir() / e.file, ec);
            StartDownload(e);
        }
    }
}

void CFontsManager::PrintProgress()
{
    std::string file;
    long long now = 0, total = 0;
    if (!g_WebClient.GetFontProgress(file, now, total))
    {
        m_progressFile.clear();
        return;
    }

    auto t = std::chrono::steady_clock::now();
    if (file == m_progressFile && t - m_lastProgress < std::chrono::seconds(1))
        return;

    m_progressFile = file;
    m_lastProgress = t;

    if (total > 0)
        gEngfuncs.Con_Printf("[Fonts] Downloading %s: %d%% (%s / %s)\n", file.c_str(), (int)(now * 100 / total), FormatSize(now).c_str(), FormatSize(total).c_str());
    else
        gEngfuncs.Con_Printf("[Fonts] Downloading %s: %s\n", file.c_str(), FormatSize(now).c_str());
}

bool CFontsManager::Update()
{
    PrintProgress();

    DownloadedFont d;
    while (g_WebClient.PopCompletedFont(d))
    {
        Entry *e = nullptr;
        for (auto &it : m_entries)
            if (it.file == d.file) { e = &it; break; }
        if (!e)
            continue;

        e->downloading = false;

        if (!d.error.empty() || d.data.size() < 1024)
        {
            gEngfuncs.Con_Printf("[Fonts] Download failed: %s (%s)\n", d.file.c_str(), d.error.empty() ? "empty response" : d.error.c_str());
            continue;
        }

        std::error_code ec;
        fs::path tmp = CacheDir() / (d.file + ".tmp");
        bool written;
        {
            std::ofstream f(tmp, std::ios::binary);
            written = (bool)f.write((const char *)d.data.data(), (std::streamsize)d.data.size());
        }

        if (written)
            fs::rename(tmp, CacheDir() / d.file, ec);

        if (!written || ec || !IsValidFile(d.file))
        {
            fs::remove(tmp, ec);
            fs::remove(CacheDir() / d.file, ec);
            gEngfuncs.Con_Printf("[Fonts] Failed to save/validate %s\n", d.file.c_str());
            continue;
        }

        e->ready = true;
        m_dirty = true;
        gEngfuncs.Con_Printf("[Fonts] Downloaded %s (%s), saved to fonts_cache\n", d.file.c_str(), FormatSize((long long)d.data.size()).c_str());
    }

    auto now = std::chrono::steady_clock::now();
    if (m_dirty && now - m_lastRebuild > std::chrono::seconds(2))
    {
        m_dirty = false;
        m_lastRebuild = now;
        gEngfuncs.Con_Printf("[Fonts] Rebuilding font atlas\n");
        return true;
    }
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

bool CFontsManager::IsReady(const std::string &file) const
{
    for (auto &e : m_entries)
    {
        if (e.file == file)
            return e.ready;
    }
    return false;
}
