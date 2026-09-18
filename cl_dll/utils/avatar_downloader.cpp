#include "avatar_downloader.h"
#include <curl/curl.h>

AvatarDownloader g_AvatarDownloader;

AvatarDownloader::~AvatarDownloader()
{
}

size_t AvatarDownloader::WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t totalSize = size * nmemb;
    if (userp)
    {
        auto* str = static_cast<std::string*>(userp);
        str->append(static_cast<char*>(contents), totalSize);
    }
    return totalSize;
}

size_t AvatarDownloader::WriteVectorCallback(void* contents, size_t size, size_t nmemb, void* userp)
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

std::string AvatarDownloader::ExtractXmlTag(const std::string& xml, const std::string& tag)
{
    std::string openTag = "<" + tag + ">";
    std::string closeTag = "</" + tag + ">";

    size_t start = xml.find(openTag);
    if (start == std::string::npos) return "";

    start += openTag.length();
    size_t end = xml.find(closeTag, start);
    if (end == std::string::npos) return "";

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

void AvatarDownloader::FetchAvatarAsync(int playerIndex, uint64_t steam64)
{
    std::thread t(&AvatarDownloader::WorkerThreadFunc, this, playerIndex, steam64);
    t.detach();
}

void AvatarDownloader::WorkerThreadFunc(int playerIndex, uint64_t steam64)
{
    CURL* curl = curl_easy_init();
    DownloadedAvatar result;
    result.playerIndex = playerIndex;
    result.steam64 = steam64;
    result.success = false;

    if (!curl)
        return;

    std::string xmlUrl = "https://steamcommunity.com/profiles/" + std::to_string(steam64) + "?xml=1";
    std::string xmlBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, xmlUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &xmlBuffer);

    CURLcode res = curl_easy_perform(curl);
    std::string avatarUrl;

    if (res == CURLE_OK)
    {
        avatarUrl = ExtractXmlTag(xmlBuffer, "avatarMedium");
        if (avatarUrl.empty())
        {
            avatarUrl = ExtractXmlTag(xmlBuffer, "avatarFull");
        }
        if (avatarUrl.empty())
        {
            avatarUrl = ExtractXmlTag(xmlBuffer, "avatarIcon");
        }
    }

    if (!avatarUrl.empty())
    {
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_URL, avatarUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteVectorCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.imageData);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK && !result.imageData.empty())
        {
            result.success = true;
        }
    }

    curl_easy_cleanup(curl);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_completedQueue.push_back(result);
}

bool AvatarDownloader::PopCompleted(DownloadedAvatar& outData)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_completedQueue.empty())
        return false;

    outData = m_completedQueue.back();
    m_completedQueue.pop_back();
    return true;
}