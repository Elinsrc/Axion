#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <unordered_set>
#include <curl/curl.h>

static const char* FONT_BASE_URL = "https://raw.githubusercontent.com/openmaptiles/fonts/master/noto-sans/";

struct DownloadedAvatar
{
    int playerIndex;
    uint64_t steam64;
    bool success;
    bool isAnimated;
    std::vector<uint8_t> imageData;
    std::vector<uint8_t> animatedData;

    std::vector<std::vector<uint8_t>> gifFrames;
    std::vector<float> gifDelays;
    int gifWidth;
    int gifHeight;
    int gifFrameCount;
};

struct AvatarTask
{
    int playerIndex;
    uint64_t steam64;
};

struct DownloadedFont
{
    std::string file;
    std::vector<uint8_t> data;
    std::string error;
};

template<typename T>
class WorkQueue
{
public:
    void push(T item)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_items.push(std::move(item));
        m_cv.notify_one();
    }

    bool pop(T &item, std::atomic<bool> &running)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&]() { return !m_items.empty() || !running.load(); });
        if (m_items.empty())
            return false;
        item = std::move(m_items.front());
        m_items.pop();
        return true;
    }

private:
    std::queue<T> m_items;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

class WebClient
{
public:
    WebClient();
    ~WebClient();

    void CheckUpdateAsync();
    bool IsUpdateChecked() const { return m_updateFinished.load(); }
    bool HasUpdate() const { return m_hasUpdate.load(); }
    std::string GetRemoteHash() const { return m_remoteHash; }
    std::string GetCommitMessage() const { return m_commitMessage; }

    void QueueAvatarDownload(int playerIndex, uint64_t steam64);
    bool PopCompletedAvatar(DownloadedAvatar& outData);
    void InvalidateAvatar(uint64_t steam64);

    void QueueFontDownload(const std::string& file);
    bool PopCompletedFont(DownloadedFont& out);
    bool GetFontProgress(std::string& file, long long& downloaded, long long& total);

private:
    void PerformUpdateCheck();
    void StaticAvatarWorkerLoop();
    void AnimatedAvatarWorkerLoop();
    void PushCompleted(const DownloadedAvatar& data);
    void LogErrorOnce(const std::string& key, const std::string& msg);
    void FlushLogs();

    void FontWorkerLoop();
    static int FontXferInfo(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

    static size_t WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t WriteVectorCallback(void* contents, size_t size, size_t nmemb, void* userp);
    void SetupCurlEasy(CURL* curl, const std::string& url, long timeoutSec);
    std::string PerformHttpGetString(const std::string& url, long timeoutSec, std::string* err = nullptr);
    std::vector<uint8_t> PerformHttpGetBytes(const std::string& url, long timeoutSec, std::string* err = nullptr, curl_xferinfo_callback xferCb = nullptr, void* xferData = nullptr, bool followRedirects = false, bool failOnError = false, long connectTimeoutSec = 0, long lowSpeedLimit = 0, long lowSpeedTime = 0);
    std::string ExtractXmlTag(const std::string& xml, const std::string& tag);
    std::string ExtractAnimatedAvatarUrl(const std::string& html);
    std::string CleanHash(const std::string& rawHash) const;

private:
    std::atomic<bool> m_updateFinished{false};
    std::atomic<bool> m_hasUpdate{false};
    std::string m_remoteHash;
    std::string m_commitMessage;
    std::thread m_updateThread;

    std::queue<AvatarTask> m_staticQueue;
    std::mutex m_staticMutex;
    std::condition_variable m_staticCv;
    std::thread m_staticThread;

    std::queue<AvatarTask> m_animatedQueue;
    std::mutex m_animatedMutex;
    std::condition_variable m_animatedCv;
    std::thread m_animatedThread;

    std::vector<DownloadedAvatar> m_completedAvatars;
    std::mutex m_completedMutex;

    std::unordered_set<uint64_t> m_downloadedStatic;
    std::unordered_set<uint64_t> m_downloadedAnimated;
    std::mutex m_downloadedMutex;

    std::atomic<bool> m_running{true};

    std::vector<std::string> m_pendingLogs;
    std::unordered_set<std::string> m_loggedKeys;
    std::mutex m_logMutex;

    std::thread m_fontThread;
    WorkQueue<std::string> m_fontQueue;

    std::vector<DownloadedFont> m_fontDone;
    std::mutex m_fontDoneMutex;

    struct FontProgress
    {
        std::string file;
        long long now = 0;
        long long total = 0;
        bool active = false;
    };
    FontProgress m_fontProgress;
    std::mutex m_fontProgressMutex;
};

extern WebClient g_WebClient;