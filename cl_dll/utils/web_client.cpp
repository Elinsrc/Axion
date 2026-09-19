#include "web_client.h"
#include "build.h"
#include "build_info.h"
#include <algorithm>

#if XASH_MOBILE_PLATFORM
#include <psa/crypto.h>
#endif

WebClient g_WebClient;

WebClient::WebClient()
{
#if XASH_MOBILE_PLATFORM
    psa_crypto_init();
#endif
    curl_global_init(CURL_GLOBAL_DEFAULT);
    m_avatarThread = std::thread(&WebClient::AvatarWorkerLoop, this);
}

WebClient::~WebClient()
{
    m_running.store(false);
    m_avatarCv.notify_all();

    if (m_avatarThread.joinable())
        m_avatarThread.join();

    if (m_updateThread.joinable())
        m_updateThread.join();

    curl_global_cleanup();
}

size_t WebClient::WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t totalSize = size * nmemb;
    if (userp)
    {
        auto* str = static_cast<std::string*>(userp);
        str->append(static_cast<char*>(contents), totalSize);
    }
    return totalSize;
}

size_t WebClient::WriteVectorCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t totalSize = size * nmemb;
    if (userp)
    {
        auto* vec = static_cast<std::vector<uint8_t>*>(userp);
        const uint8_t* bytes = static_cast<const uint8_t*>(contents);
        vec->insert(vec->end(), bytes, bytes + totalSize);
    }
    return totalSize;
}

void WebClient::SetupCurlEasy(CURL* curl, const std::string& url, long timeoutSec)
{
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
}

std::string WebClient::PerformHttpGetString(const std::string& url, long timeoutSec)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return "";

    std::string buffer;
    SetupCurlEasy(curl, url, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK)
        return buffer;

    return "";
}

std::vector<uint8_t> WebClient::PerformHttpGetBytes(const std::string& url, long timeoutSec)
{
    CURL* curl = curl_easy_init();
    std::vector<uint8_t> buffer;
    if (!curl)
        return buffer;

    SetupCurlEasy(curl, url, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteVectorCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK)
        return buffer;

    return std::vector<uint8_t>();
}

std::string WebClient::ExtractXmlTag(const std::string& xml, const std::string& tag)
{
    std::string openTag = "<" + tag + ">";
    std::string closeTag = "</" + tag + ">";

    size_t start = xml.find(openTag);
    if (start == std::string::npos)
        return "";

    start += openTag.length();
    size_t end = xml.find(closeTag, start);
    if (end == std::string::npos)
        return "";

    std::string val = xml.substr(start, end - start);

    size_t cdataStart = val.find("<![CDATA[");
    if (cdataStart != std::string::npos)
    {
        cdataStart += 9;
        size_t cdataEnd = val.find("]]>", cdataStart);
        if (cdataEnd != std::string::npos)
        {
            return val.substr(cdataStart, cdataEnd - cdataStart);
        }
    }

    return val;
}

std::string WebClient::CleanHash(const std::string& rawHash) const
{
    std::string clean = rawHash;
    size_t dirtyPos = clean.find("-dirty");
    if (dirtyPos != std::string::npos)
        clean = clean.substr(0, dirtyPos);
    std::transform(clean.begin(), clean.end(), clean.begin(), ::tolower);
    return clean;
}

void WebClient::CheckUpdateAsync()
{
    if (m_updateThread.joinable())
        return;
    m_updateFinished.store(false);
    m_hasUpdate.store(false);
    m_updateThread = std::thread(&WebClient::PerformUpdateCheck, this);
}

void WebClient::PerformUpdateCheck()
{
    std::string rawLocalSha = BuildInfo::GetCommitHash();
    if (rawLocalSha == "notset")
    {
        m_updateFinished.store(true);
        return;
    }

    std::string localSha = CleanHash(rawLocalSha);
    std::string url = "https://api.github.com/repos/Elinsrc/Axion/commits?per_page=100";
    std::string readBuffer = PerformHttpGetString(url, 7);

    if (!readBuffer.empty())
    {
        std::string shaKey = "\"sha\":\"";
        size_t pos = readBuffer.find(shaKey);
        if (pos == std::string::npos)
        {
            shaKey = "\"sha\" : \"";
            pos = readBuffer.find(shaKey);
        }

        if (pos != std::string::npos)
        {
            size_t startPos = pos + shaKey.length();
            m_remoteHash = readBuffer.substr(startPos, 40);
            std::transform(m_remoteHash.begin(), m_remoteHash.end(), m_remoteHash.begin(), ::tolower);

            if (localSha != m_remoteHash)
            {
                m_hasUpdate.store(true);
            }

            std::string messageKey = "\"message\":\"";
            size_t msgPos = readBuffer.find(messageKey, startPos);
            if (msgPos == std::string::npos)
            {
                messageKey = "\"message\" : \"";
                msgPos = readBuffer.find(messageKey, startPos);
            }

            if (msgPos != std::string::npos)
            {
                size_t msgStart = msgPos + messageKey.length();
                size_t msgEnd = readBuffer.find("\"", msgStart);

                if (msgEnd != std::string::npos && msgEnd > msgStart)
                {
                    m_commitMessage = readBuffer.substr(msgStart, msgEnd - msgStart);

                    size_t nPos;
                    while ((nPos = m_commitMessage.find("\\n")) != std::string::npos)
                    {
                        m_commitMessage.replace(nPos, 2, " ");
                    }
                }
            }
        }
    }

    m_updateFinished.store(true);
}

void WebClient::QueueAvatarDownload(int playerIndex, uint64_t steam64)
{
    std::lock_guard<std::mutex> lock(m_avatarMutex);
    m_avatarQueue.push({playerIndex, steam64});
    m_avatarCv.notify_one();
}

void WebClient::AvatarWorkerLoop()
{
    while (m_running.load())
    {
        AvatarTask task;

        {
            std::unique_lock<std::mutex> lock(m_avatarMutex);
            m_avatarCv.wait(lock, [this]() {
                return !m_avatarQueue.empty() || !m_running.load();
            });

            if (!m_running.load())
                break;

            task = m_avatarQueue.front();
            m_avatarQueue.pop();
        }

        DownloadedAvatar result;
        result.playerIndex = task.playerIndex;
        result.steam64 = task.steam64;
        result.success = false;

        std::string xmlUrl = "https://steamcommunity.com/profiles/" + std::to_string(task.steam64) + "?xml=1";
        std::string xmlBuffer = PerformHttpGetString(xmlUrl, 5);

        if (!xmlBuffer.empty())
        {
            std::string avatarUrl = ExtractXmlTag(xmlBuffer, "avatarMedium");

            if (!avatarUrl.empty())
            {
                result.imageData = PerformHttpGetBytes(avatarUrl, 5);
                if (!result.imageData.empty())
                {
                    result.success = true;
                }
            }
        }

        std::lock_guard<std::mutex> lock(m_avatarMutex);
        m_completedAvatars.push_back(result);
    }
}

bool WebClient::PopCompletedAvatar(DownloadedAvatar& outData)
{
    std::lock_guard<std::mutex> lock(m_avatarMutex);
    if (m_completedAvatars.empty())
        return false;

    outData = m_completedAvatars.back();
    m_completedAvatars.pop_back();
    return true;
}