#pragma once
#include <string>
#include <vector>
#include <chrono>
#include "imgui.h"

class CFontsManager
{
public:
    void Init();
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

    bool IsValidFile(const std::string &file) const;
    void StartDownload(Entry &e);
    void PrintProgress();

    std::vector<Entry> m_entries;
    bool m_dirty = false;
    std::chrono::steady_clock::time_point m_lastRebuild{};
    std::chrono::steady_clock::time_point m_lastProgress{};
    std::string m_progressFile;
};

extern CFontsManager g_FontsManager;