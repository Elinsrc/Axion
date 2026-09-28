#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

struct HttpRequest
{
    std::string url;
    long timeoutSec = 10;
    long connectTimeoutSec = 0;
    long lowSpeedLimit = 0;
    long lowSpeedTimeSec = 0;
    bool followRedirects = false;
    bool http11 = true;

    const std::atomic<bool>* cancel = nullptr;
    std::function<void(long long now, long long total)> onProgress;
};

struct HttpResponse
{
    bool ok = false;
    long status = 0;
    std::string error;
    std::vector<uint8_t> body;

    std::string Text() const { return std::string(body.begin(), body.end()); }
};

class WebClient
{
public:
    WebClient();
    ~WebClient();

    WebClient(const WebClient&) = delete;
    WebClient& operator=(const WebClient&) = delete;

    HttpResponse Get(const HttpRequest& req) const;
    HttpResponse Get(const std::string& url, long timeoutSec = 10) const;

    void LogErrorOnce(const std::string& key, const std::string& msg);
    void FlushLogs();

private:
    std::vector<std::string> m_pendingLogs;
    std::unordered_set<std::string> m_loggedKeys;
    std::mutex m_logMutex;
};

extern WebClient g_WebClient;