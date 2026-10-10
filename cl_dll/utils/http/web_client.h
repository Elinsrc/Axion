#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

struct HttpRequest
{
    std::string url;
    long timeoutSec = 20;
    long connectTimeoutSec = 0;
    long lowSpeedLimit = 0;
    long lowSpeedTimeSec = 0;
    bool followRedirects = false;
    bool http11 = true;

    std::string method;
    std::vector<std::string> headers;
    std::string postBody;
    size_t maxBytes = 0;

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

    void RegisterCommands();
    void PrintInfo() const;

    HttpResponse Get(const HttpRequest& req) const;
    HttpResponse Get(const std::string& url, long timeoutSec = 10) const;
    HttpResponse Post(const HttpRequest& req) const;

    void LogErrorOnce(const std::string& key, const std::string& msg);
    void FlushLogs();

private:
    HttpResponse Perform(const HttpRequest& req) const;

    void EnsureCaCertificate() const;
    std::string GetCaCertDir() const;
    std::string GetCaCertPath() const;

    mutable std::mutex m_caCertMutex;
    mutable std::string m_caCertPath;
    mutable std::atomic<bool> m_caCertReady{false};
    mutable std::atomic<long long> m_nextCaCheck{0};

    std::vector<std::string> m_pendingLogs;
    std::unordered_set<std::string> m_loggedKeys;
    std::mutex m_logMutex;
};

extern WebClient g_WebClient;