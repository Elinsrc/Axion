#include "web_client.h"
#include "build.h"
#include "hud.h"

#include "mHTTP.h"

#include <algorithm>
#include <chrono>
#include <climits>
#include <ctime>
#include <filesystem>

WebClient g_WebClient;

namespace fs = std::filesystem;

using Clock = std::chrono::steady_clock;

static const char* const CACERT_SUBDIR = "certs";
static const char* const CACERT_FILENAME = "cacert.pem";
static const char* const USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64)";

static constexpr long long CACERT_MAX_AGE_HOURS = 24 * 30;
static constexpr long long PROGRESS_INTERVAL_MS = 100;

struct TransferCtx
{
    const HttpRequest* req = nullptr;
    std::vector<uint8_t>* body = nullptr;

    Clock::time_point start;
    Clock::time_point lastProgress;
    Clock::time_point windowStart;

    long long bytes = 0;
    long long windowBytes = 0;

    size_t maxBytes = 0;

    bool timedOut = false;
    bool tooSlow = false;
    bool tooBig = false;
};

static std::string FormatMhttpError(mhttp_error code, const char* msg)
{
    std::string s = mhttp_strerror(code);
    if (msg && msg[0])
    {
        s += ": ";
        s += msg;
    }
    return s;
}

static int SecToMs(long sec)
{
    const long long ms = (long long)sec * 1000;
    return ms > INT_MAX ? INT_MAX : (int)ms;
}

static size_t WriteBodyCallback(const void* data, size_t len, void* user)
{
    auto* ctx = static_cast<TransferCtx*>(user);

    if (ctx->tooBig)
        return len;

    if (ctx->maxBytes && ctx->body->size() + len > ctx->maxBytes)
    {
        ctx->tooBig = true;
        ctx->body->clear();
        ctx->body->shrink_to_fit();
        return len;
    }

    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    ctx->body->insert(ctx->body->end(), bytes, bytes + len);
    return len;
}

static int ProgressCallback(int64_t now, int64_t total, void* user)
{
    auto* ctx = static_cast<TransferCtx*>(user);
    ctx->bytes = now;

    if (ctx->req->onProgress)
    {
        const auto t = Clock::now();
        const bool final = total > 0 && now >= total;

        if (final || t - ctx->lastProgress >= std::chrono::milliseconds(PROGRESS_INTERVAL_MS))
        {
            ctx->lastProgress = t;
            ctx->req->onProgress((long long)now, (long long)total);
        }
    }
    return 0;
}

static int IsCancelledCallback(void* user)
{
    auto* ctx = static_cast<TransferCtx*>(user);
    const HttpRequest* req = ctx->req;

    if (ctx->tooBig)
        return 1;

    if (req->cancel && req->cancel->load())
        return 1;

    const auto now = Clock::now();

    if (req->timeoutSec > 0 && now - ctx->start >= std::chrono::seconds(req->timeoutSec))
    {
        ctx->timedOut = true;
        return 1;
    }

    if (req->lowSpeedLimit > 0 && req->lowSpeedTimeSec > 0)
    {
        const double window = std::chrono::duration<double>(now - ctx->windowStart).count();

        if (window >= (double)req->lowSpeedTimeSec)
        {
            const double speed = (double)(ctx->bytes - ctx->windowBytes) / window;

            if (speed < (double)req->lowSpeedLimit)
            {
                ctx->tooSlow = true;
                return 1;
            }

            ctx->windowStart = now;
            ctx->windowBytes = ctx->bytes;
        }
    }
    return 0;
}

static long long CaFileAgeSec(const std::string& path)
{
    std::error_code ec;

    const uintmax_t size = fs::file_size(path, ec);
    if (ec || size == 0)
        return -1;

    const auto ft = fs::last_write_time(path, ec);
    if (ec)
        return -1;

    const long long age = (long long)std::chrono::duration_cast<std::chrono::seconds>(fs::file_time_type::clock::now() - ft).count();
    return age > 0 ? age : 0;
}

WebClient::WebClient()
{
    mhttp_global_init();
}

WebClient::~WebClient()
{
    mhttp_global_cleanup();
}

static void WebClientInfo_f()
{
    g_WebClient.PrintInfo();
}

void WebClient::RegisterCommands()
{
    gEngfuncs.pfnAddCommand("webclient_info", WebClientInfo_f);
}

static std::string FormatDuration(long long totalSec)
{
    if (totalSec < 0)
        totalSec = 0;

    const long long d = totalSec / 86400;
    const long long h = (totalSec % 86400) / 3600;
    const long long m = (totalSec % 3600) / 60;
    const long long s = totalSec % 60;

    char buf[64];

    if (d > 0)
        snprintf(buf, sizeof(buf), "%lld d %lld h %lld min %lld s", d, h, m, s);
    else if (h > 0)
        snprintf(buf, sizeof(buf), "%lld h %lld min %lld s", h, m, s);
    else if (m > 0)
        snprintf(buf, sizeof(buf), "%lld min %lld s", m, s);
    else
        snprintf(buf, sizeof(buf), "%lld s", s);

    return buf;
}

void WebClient::PrintInfo() const
{
    std::string caPath;
    {
        std::lock_guard<std::mutex> lock(m_caCertMutex);
        caPath = m_caCertPath.empty() ? GetCaCertPath() : m_caCertPath;
    }

    const std::string systemSource = mhttp_ca_system_source();
    const bool usingSystemCa = !systemSource.empty();

    gEngfuncs.Con_Printf("WebClient:\n");
    gEngfuncs.Con_Printf("  mHTTP version: %s\n", MHTTP_VERSION);
    gEngfuncs.Con_Printf("  tls version: %s\n", mhttp_tls_version());
    gEngfuncs.Con_Printf("  ca verify: %s\n", m_caCertReady.load(std::memory_order_relaxed) ? "ON" : "OFF");
    gEngfuncs.Con_Printf("  ca certs loaded: %d\n", mhttp_ca_cert_count());

    if (usingSystemCa)
    {
        gEngfuncs.Con_Printf("  ca source: system (%s)\n", systemSource.c_str());
        gEngfuncs.Con_Printf("  ca next check: never (using system store)\n");
        return;
    }

    std::error_code ec;
    const long long ageSec = CaFileAgeSec(caPath);
    const uintmax_t fileSize = ageSec >= 0 ? fs::file_size(caPath, ec) : 0;

    const long long now = (long long)time(nullptr);
    const long long nextCheckSec = m_nextCaCheck.load(std::memory_order_relaxed) - now;

    gEngfuncs.Con_Printf("  ca source: downloaded bundle\n");
    gEngfuncs.Con_Printf("  ca url: %s\n", mhttp_ca_download_url());
    gEngfuncs.Con_Printf("  ca path: %s\n", caPath.c_str());

    if (ageSec >= 0)
        gEngfuncs.Con_Printf("  ca age: %s (%zu bytes)\n", FormatDuration(ageSec).c_str(), (size_t)fileSize);
    else
        gEngfuncs.Con_Printf("  ca file: not found\n");

    if (nextCheckSec > 0)
        gEngfuncs.Con_Printf("  ca next check: in %s\n", FormatDuration(nextCheckSec).c_str());
    else
        gEngfuncs.Con_Printf("  ca next check: pending\n");
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

    gEngfuncs.Con_Printf("[WebClient] Checking CA certificate...\n");

    if (mhttp_load_system_ca() == MHTTP_OK)
    {
        m_caCertReady.store(true, std::memory_order_relaxed);

        gEngfuncs.Con_Printf("[WebClient] Using system CA certificates from: %s\n", mhttp_ca_system_source());
        gEngfuncs.Con_Printf("[WebClient] SSL certificate verification is ENABLED\n");

        m_nextCaCheck.store(LLONG_MAX, std::memory_order_relaxed);
        return;
    }

    gEngfuncs.Con_Printf("[WebClient] System CA certificate store is unavailable, using downloaded CA bundle at '%s'\n", m_caCertPath.c_str());

    const long long maxAgeSec = CACERT_MAX_AGE_HOURS * 3600;

    const mhttp_error err = mhttp_ca_update(m_caCertPath.c_str(), (int)maxAgeSec);
    const bool ready = (err == MHTTP_OK) && mhttp_ca_ready();

    m_caCertReady.store(ready, std::memory_order_relaxed);

    if (ready)
        gEngfuncs.Con_Printf("[WebClient] SSL certificate verification is ENABLED using '%s'\n", m_caCertPath.c_str());
    else
        gEngfuncs.Con_Printf("[WebClient] SSL certificate verification is DISABLED (no valid CA bundle)\n");

    const long long ageSec = CaFileAgeSec(m_caCertPath);
    const long long secondsUntilNextCheck = (ageSec >= 0 && ageSec < maxAgeSec) ? (maxAgeSec - ageSec) : maxAgeSec;

    m_nextCaCheck.store(now + secondsUntilNextCheck, std::memory_order_relaxed);
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
    return Perform(req);
}

HttpResponse WebClient::Post(const HttpRequest& req) const
{
    return Perform(req);
}

HttpResponse WebClient::Perform(const HttpRequest& req) const
{
    HttpResponse res;

    EnsureCaCertificate();

    TransferCtx ctx;
    ctx.req = &req;
    ctx.body = &res.body;
    ctx.maxBytes = req.maxBytes;
    ctx.start = Clock::now();
    ctx.windowStart = ctx.start;

    mhttp_request mreq;
    mhttp_request_init(&mreq, req.url.c_str());

    mreq.user_agent = USER_AGENT;
    mreq.follow_redirects = req.followRedirects ? 1 : 0;
    mreq.insecure = m_caCertReady.load(std::memory_order_relaxed) ? 0 : 1;
    mreq.max_body_size = req.maxBytes;

    if (req.lowSpeedLimit > 0 && req.lowSpeedTimeSec > 0)
        mreq.timeout_ms = SecToMs(req.lowSpeedTimeSec);
    else if (req.timeoutSec > 0)
        mreq.timeout_ms = SecToMs(req.timeoutSec);
    else
        mreq.timeout_ms = 0;

    if (req.connectTimeoutSec > 0)
        mreq.connect_timeout_ms = SecToMs(req.connectTimeoutSec);

    std::vector<const char*> headerPtrs;

    if (!req.method.empty() || !req.postBody.empty())
    {
        mreq.method = req.method.empty() ? "POST" : req.method.c_str();
        mreq.body = req.postBody.data();
        mreq.body_len = req.postBody.size();
    }

    if (!req.headers.empty())
    {
        for (const auto& h : req.headers)
            headerPtrs.push_back(h.c_str());

        headerPtrs.push_back(nullptr);
        mreq.headers = headerPtrs.data();
    }

    mreq.on_data = WriteBodyCallback;
    mreq.on_data_user = &ctx;
    mreq.on_progress = ProgressCallback;
    mreq.on_progress_user = &ctx;
    mreq.is_cancelled = IsCancelledCallback;
    mreq.is_cancelled_user = &ctx;

    mhttp_response mres;
    const mhttp_error err = mhttp_perform(&mreq, &mres);

    res.status = mres.status;
    const std::string mhttpError = mres.error;
    mhttp_response_free(&mres);

    if (ctx.tooBig || err == MHTTP_ERR_TOO_BIG)
    {
        res.error = "response larger than " + std::to_string(req.maxBytes) + " bytes";
        res.body.clear();
        return res;
    }

    if (err == MHTTP_ERR_HTTP)
    {
        res.error = "HTTP " + std::to_string(res.status);
        res.body.clear();
        return res;
    }

    if (err != MHTTP_OK)
    {
        if (ctx.timedOut)
            res.error = "timeout after " + std::to_string(req.timeoutSec) + " s";
        else if (ctx.tooSlow)
            res.error = "transfer speed below " + std::to_string(req.lowSpeedLimit) + " B/s for " + std::to_string(req.lowSpeedTimeSec) + " s";
        else
            res.error = FormatMhttpError(err, mhttpError.c_str());

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