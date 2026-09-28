#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include "imgui.h"

#include "web_worker.h"

class CFontsManager
{
public:
    void Init();
    void Shutdown();
    bool Update();
    void MergeInto(float sizePixels, const std::string &skipFile = "");
    bool IsReady(const std::string &file) const;
    std::string PathOf(const std::string &file) const;

private:
    struct Entry
    {
        std::string file;
        bool ready = false;
        bool downloading = false;
    };

    struct DownloadResult
    {
        std::string file;
        std::vector<uint8_t> data;
        std::string error;
    };

    struct Progress
    {
        std::string file;
        long long now = 0;
        long long total = 0;
        bool active = false;
    };

    bool IsValidFile(const std::string &file) const;
    bool SaveFile(const std::string &file, const std::vector<uint8_t> &data);
    Entry *FindEntry(const std::string &file);
    void StartDownload(Entry &e);
    void ApplyResult(DownloadResult &d);
    void PrintProgress();

    void DownloadFont(const std::string &file);

    std::vector<Entry> m_entries;
    bool m_dirty = false;
    std::chrono::steady_clock::time_point m_lastRebuild{};
    std::chrono::steady_clock::time_point m_lastProgress{};
    std::string m_progressFile;

    WebWorker m_worker;

    std::vector<DownloadResult> m_done;
    std::mutex m_doneMutex;

    Progress m_progress;
    std::mutex m_progressMutex;
};

extern CFontsManager g_FontsManager;