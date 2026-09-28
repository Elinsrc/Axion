#include "update_checker.h"
#include "web_client.h"

#include "build_info.h"

#include <algorithm>
#include <cctype>

#define UPDATE_URL "https://api.github.com/repos/Elinsrc/Axion/commits?per_page=100"

CUpdateChecker g_UpdateChecker;

static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static std::string CleanHash(const std::string& rawHash)
{
    size_t dirty = rawHash.find("-dirty");
    return ToLower(dirty == std::string::npos ? rawHash : rawHash.substr(0, dirty));
}

static bool FindJsonString(const std::string& json, const std::string& key, size_t from, std::string& value, size_t& valueStart)
{
    static const char* separators[] = { "\":\"", "\" : \"" };

    for (const char* sep : separators)
    {
        std::string needle = "\"" + key + sep;
        size_t pos = json.find(needle, from);
        if (pos == std::string::npos)
            continue;

        size_t start = pos + needle.size();
        size_t end = json.find('"', start);
        if (end == std::string::npos)
            return false;

        value = json.substr(start, end - start);
        valueStart = start;
        return true;
    }
    return false;
}

void CUpdateChecker::CheckAsync()
{
    if (m_started.exchange(true))
        return;

    m_finished.store(false);
    m_hasUpdate.store(false);

    m_worker.Start();
    m_worker.Post([this]() { Run(); });
}

void CUpdateChecker::Shutdown()
{
    m_worker.Stop();
}

void CUpdateChecker::Run()
{
    Check();
    m_finished.store(true);
}

void CUpdateChecker::Check()
{
    const std::string rawLocal = BuildInfo::GetCommitHash();
    if (rawLocal == "notset")
        return;

    HttpRequest req;
    req.url = UPDATE_URL;
    req.timeoutSec = 7;
    req.cancel = &m_worker.StopFlag();

    HttpResponse res = g_WebClient.Get(req);
    if (!res.ok || res.body.empty())
    {
        g_WebClient.LogErrorOnce("update_check", "[UpdateChecker] Check failed: " + res.error);
        return;
    }

    const std::string json = res.Text();

    std::string sha;
    size_t shaStart = 0;
    if (!FindJsonString(json, "sha", 0, sha, shaStart))
    {
        g_WebClient.LogErrorOnce("update_parse", "[UpdateChecker] Check failed: unexpected GitHub response");
        return;
    }

    m_remoteHash = ToLower(sha);
    if (CleanHash(rawLocal) != m_remoteHash)
        m_hasUpdate.store(true);

    std::string message;
    size_t msgStart = 0;
    if (FindJsonString(json, "message", shaStart, message, msgStart))
    {
        size_t pos;
        while ((pos = message.find("\\n")) != std::string::npos)
            message.replace(pos, 2, " ");
        m_commitMessage = message;
    }
}