#include "web_client.h"
#include "build.h"
#include "hud.h"

#include <curl/curl.h>

#if XASH_MOBILE_PLATFORM
#include <psa/crypto.h>
#endif

WebClient g_WebClient;

static std::string FormatCurlError(CURLcode code, const char* errBuf)
{
    std::string s = "curl error " + std::to_string((int)code) + " (" + curl_easy_strerror(code) + ")";
    if (errBuf && errBuf[0])
    {
        s += ": ";
        s += errBuf;
    }
    return s;
}

static size_t WriteBodyCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    const size_t total = size * nmemb;
    auto* body = static_cast<std::vector<uint8_t>*>(userp);
    const uint8_t* bytes = static_cast<const uint8_t*>(contents);
    body->insert(body->end(), bytes, bytes + total);
    return total;
}

static int XferInfoCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t)
{
    const HttpRequest* req = static_cast<const HttpRequest*>(clientp);

    if (req->onProgress)
        req->onProgress((long long)dlnow, (long long)dltotal);

    return (req->cancel && req->cancel->load()) ? 1 : 0;
}

WebClient::WebClient()
{
#if XASH_MOBILE_PLATFORM
    psa_crypto_init();
#endif
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

WebClient::~WebClient()
{
    curl_global_cleanup();
}

HttpResponse WebClient::Get(const std::string& url, long timeoutSec) const
{
    HttpRequest req;
    req.url = url;
    req.timeoutSec = timeoutSec;
    return Get(req);
}

HttpResponse WebClient::Get(const HttpRequest& req) const
{
    HttpResponse res;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        res.error = "curl_easy_init failed";
        return res;
    }

    char errBuf[CURL_ERROR_SIZE] = {0};

    curl_easy_setopt(curl, CURLOPT_URL, req.url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errBuf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBodyCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, XferInfoCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &req);

    if (req.timeoutSec > 0)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, req.timeoutSec);
    if (req.connectTimeoutSec > 0)
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, req.connectTimeoutSec);
    if (req.lowSpeedLimit > 0)
    {
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, req.lowSpeedLimit);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, req.lowSpeedTimeSec);
    }
    if (req.followRedirects)
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    if (req.http11)
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    CURLcode code = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK)
    {
        res.error = FormatCurlError(code, errBuf);
        res.body.clear();
        return res;
    }

    if (res.status != 200)
    {
        res.error = "HTTP " + std::to_string(res.status);
        res.body.clear();
        return res;
    }

    res.ok = true;
    return res;
}

void WebClient::LogErrorOnce(const std::string& key, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (!m_loggedKeys.insert(key).second)
        return;
    m_pendingLogs.push_back(msg);
}

void WebClient::FlushLogs()
{
    std::vector<std::string> logs;
    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        logs.swap(m_pendingLogs);
    }

    for (const auto& s : logs)
        gEngfuncs.Con_Printf("%s\n", s.c_str());
}