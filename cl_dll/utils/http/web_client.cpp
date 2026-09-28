#include "web_client.h"
#include "build.h"
#include "hud.h"

#include <curl/curl.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>

#include <psa/crypto.h>

WebClient g_WebClient;

static const char* const CACERT_URL = "https://curl.se/ca/cacert.pem";
static const char* const CACERT_SUBDIR = "certs";
static const char* const CACERT_FILENAME = "cacert.pem";

static constexpr long long CACERT_MAX_AGE_HOURS = 24 * 30;
static constexpr long long CACERT_RECHECK_INTERVAL_SEC = 24 * 3600;

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
    psa_crypto_init();
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

WebClient::~WebClient()
{
    curl_global_cleanup();
}

std::string WebClient::GetCaCertDir() const
{
    std::string base = ".";
    if (gEngfuncs.pfnGetGameDirectory)
    {
        const char* dir = gEngfuncs.pfnGetGameDirectory();
        if (dir && dir[0])
            base = dir;
    }
    return base + "/" + CACERT_SUBDIR;
}

std::string WebClient::GetCaCertPath() const
{
    return GetCaCertDir() + "/" + CACERT_FILENAME;
}

bool WebClient::DownloadCaCertificate(const std::string& path) const
{
    namespace fs = std::filesystem;

    gEngfuncs.Con_Printf("[WebClient] Downloading CA certificate bundle from %s ...\n", CACERT_URL);

    std::error_code dirEc;
    fs::path parentDir = fs::path(path).parent_path();
    if (!parentDir.empty())
    {
        fs::create_directories(parentDir, dirEc);
        if (dirEc)
        {
            gEngfuncs.Con_Printf("[WebClient] Failed to create directory '%s': %s\n", parentDir.string().c_str(), dirEc.message().c_str());
        }
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        gEngfuncs.Con_Printf("[WebClient] curl_easy_init failed while downloading CA certificate\n");
        return false;
    }

    std::vector<uint8_t> body;
    char errBuf[CURL_ERROR_SIZE] = {0};

    curl_easy_setopt(curl, CURLOPT_URL, CACERT_URL);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errBuf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBodyCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode code = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK || status != 200 || body.empty())
    {
        gEngfuncs.Con_Printf("[WebClient] Failed to download CA certificate: %s (HTTP %ld)\n", FormatCurlError(code, errBuf).c_str(), status);
        return false;
    }

    const std::string tmpPath = path + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            gEngfuncs.Con_Printf("[WebClient] Failed to open temp file '%s' for writing\n", tmpPath.c_str());
            return false;
        }

        out.write(reinterpret_cast<const char*>(body.data()), (std::streamsize)body.size());
        if (!out.good())
        {
            out.close();
            std::error_code rmEc;
            fs::remove(tmpPath, rmEc);
            gEngfuncs.Con_Printf("[WebClient] Failed to write CA certificate data to disk\n");
            return false;
        }
    }

    std::error_code ec;
    fs::rename(tmpPath, path, ec);
    if (ec)
    {
        ec.clear();
        fs::copy_file(tmpPath, path, fs::copy_options::overwrite_existing, ec);
        std::error_code rmEc;
        fs::remove(tmpPath, rmEc);
        if (ec)
        {
            gEngfuncs.Con_Printf("[WebClient] Failed to move CA certificate into place: %s\n", ec.message().c_str());
            return false;
        }
    }

    gEngfuncs.Con_Printf("[WebClient] CA certificate bundle saved to '%s' (%zu bytes)\n", path.c_str(), body.size());
    return true;
}

void WebClient::EnsureCaCertificate() const
{
    const long long now = (long long)time(nullptr);
    if (now < m_nextCaCheck.load(std::memory_order_relaxed))
        return;

    std::lock_guard<std::mutex> lock(m_caCertMutex);

    if (now < m_nextCaCheck.load(std::memory_order_relaxed))
        return;

    if (m_caCertPath.empty())
        m_caCertPath = GetCaCertPath();

    namespace fs = std::filesystem;
    std::error_code ec;

    const std::string certDir = GetCaCertDir();

    gEngfuncs.Con_Printf("[WebClient] Checking CA certificate at '%s'...\n", m_caCertPath.c_str());

    if (!fs::exists(certDir, ec) || ec)
    {
        ec.clear();
        gEngfuncs.Con_Printf("[WebClient] Certificate directory '%s' does not exist, creating...\n", certDir.c_str());
        if (!fs::create_directories(certDir, ec) && ec)
        {
            gEngfuncs.Con_Printf("[WebClient] Failed to create directory '%s': %s\n", certDir.c_str(), ec.message().c_str());
        }
    }

    bool needDownload = false;
    bool fileExists = false;

    ec.clear();
    fileExists = fs::exists(m_caCertPath, ec) && !ec;

    if (!fileExists)
    {
        gEngfuncs.Con_Printf("[WebClient] CA certificate not found, will download a fresh copy\n");
        needDownload = true;
    }
    else
    {
        ec.clear();
        const uintmax_t sz = fs::file_size(m_caCertPath, ec);
        if (ec || sz == 0)
        {
            gEngfuncs.Con_Printf("[WebClient] CA certificate file is empty or unreadable, will re-download\n");
            needDownload = true;
        }
        else
        {
            ec.clear();
            const auto ftime = fs::last_write_time(m_caCertPath, ec);
            if (ec)
            {
                gEngfuncs.Con_Printf("[WebClient] Failed to read CA certificate timestamp, will re-download\n");
                needDownload = true;
            }
            else
            {
                const auto ageHours = std::chrono::duration_cast<std::chrono::hours>(fs::file_time_type::clock::now() - ftime).count();
                const long long ageDays = ageHours / 24;

                if (ageHours >= CACERT_MAX_AGE_HOURS)
                {
                    gEngfuncs.Con_Printf("[WebClient] CA certificate is %lld days old (limit %lld days), re-downloading...\n", ageDays, (long long)(CACERT_MAX_AGE_HOURS / 24));
                    needDownload = true;
                }
                else
                {
                    gEngfuncs.Con_Printf("[WebClient] CA certificate is up to date (%lld days old)\n", ageDays);
                }
            }
        }
    }

    if (needDownload)
    {
        if (DownloadCaCertificate(m_caCertPath))
        {
            gEngfuncs.Con_Printf("[WebClient] CA certificate updated successfully\n");
        }
        else
        {
            gEngfuncs.Con_Printf("[WebClient] Could not obtain CA certificate bundle, SSL verification will be relaxed\n");
        }
    }

    ec.clear();
    const bool ready = fs::exists(m_caCertPath, ec) && !ec && fs::file_size(m_caCertPath, ec) > 0 && !ec;
    m_caCertReady.store(ready, std::memory_order_relaxed);

    if (ready)
        gEngfuncs.Con_Printf("[WebClient] SSL certificate verification is ENABLED using '%s'\n", m_caCertPath.c_str());
    else
        gEngfuncs.Con_Printf("[WebClient] SSL certificate verification is DISABLED (no valid CA bundle)\n");

    m_nextCaCheck.store(now + CACERT_RECHECK_INTERVAL_SEC, std::memory_order_relaxed);
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

    EnsureCaCertificate();

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

    if (m_caCertReady.load(std::memory_order_relaxed) && !m_caCertPath.empty())
    {
        curl_easy_setopt(curl, CURLOPT_CAINFO, m_caCertPath.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    }
    else
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

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