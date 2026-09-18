#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <cstdint>

struct DownloadedAvatar
{
    int playerIndex = 0;
    uint64_t steam64 = 0;
    std::vector<uint8_t> imageData;
    bool success = false;
};

class AvatarDownloader
{
public:
    AvatarDownloader() = default;
    ~AvatarDownloader();

    void FetchAvatarAsync(int playerIndex, uint64_t steam64);

    bool PopCompleted(DownloadedAvatar& outData);

private:
    void WorkerThreadFunc(int playerIndex, uint64_t steam64);

    static size_t WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t WriteVectorCallback(void* contents, size_t size, size_t nmemb, void* userp);
    std::string ExtractXmlTag(const std::string& xml, const std::string& tag);

    std::mutex m_mutex;
    std::vector<DownloadedAvatar> m_completedQueue;
};

extern AvatarDownloader g_AvatarDownloader;