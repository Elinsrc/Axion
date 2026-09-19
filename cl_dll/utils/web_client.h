#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <curl/curl.h>

struct DownloadedAvatar
{
    int playerIndex;
    uint64_t steam64;
    bool success;
    std::vector<uint8_t> imageData;
};

struct AvatarTask
{
    int playerIndex;
    uint64_t steam64;
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

private:
    void PerformUpdateCheck();
    void AvatarWorkerLoop();

    static size_t WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t WriteVectorCallback(void* contents, size_t size, size_t nmemb, void* userp);
    void SetupCurlEasy(CURL* curl, const std::string& url, long timeoutSec);
    std::string PerformHttpGetString(const std::string& url, long timeoutSec);
    std::vector<uint8_t> PerformHttpGetBytes(const std::string& url, long timeoutSec);
    std::string ExtractXmlTag(const std::string& xml, const std::string& tag);
    std::string CleanHash(const std::string& rawHash) const;

private:
    std::atomic<bool> m_updateFinished{false};
    std::atomic<bool> m_hasUpdate{false};
    std::string m_remoteHash;
    std::string m_commitMessage;
    std::thread m_updateThread;

    std::queue<AvatarTask> m_avatarQueue;
    std::vector<DownloadedAvatar> m_completedAvatars;
    std::mutex m_avatarMutex;
    std::condition_variable m_avatarCv;
    std::thread m_avatarThread;
    std::atomic<bool> m_running{true};
};

extern WebClient g_WebClient;