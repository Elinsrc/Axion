#include "web_client.h"
#include "build.h"
#include "build_info.h"
#include <algorithm>
#include <cstring>

#if XASH_MOBILE_PLATFORM
#include <psa/crypto.h>
#endif

#include "stb_image.h"

WebClient g_WebClient;

WebClient::WebClient()
{
#if XASH_MOBILE_PLATFORM
    psa_crypto_init();
#endif
    curl_global_init(CURL_GLOBAL_DEFAULT);
    m_staticThread   = std::thread(&WebClient::StaticAvatarWorkerLoop, this);
    m_animatedThread = std::thread(&WebClient::AnimatedAvatarWorkerLoop, this);
}

WebClient::~WebClient()
{
    m_running.store(false);
    m_staticCv.notify_all();
    m_animatedCv.notify_all();

    if (m_staticThread.joinable())
        m_staticThread.join();

    if (m_animatedThread.joinable())
        m_animatedThread.join();

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
            return val.substr(cdataStart, cdataEnd - cdataStart);
    }

    return val;
}

std::string WebClient::ExtractAnimatedAvatarUrl(const std::string& html)
{
    size_t avatarBlock = html.find("playerAvatar");
    if (avatarBlock == std::string::npos)
        return "";

    size_t endBlock = html.find("profile_header_centered_col", avatarBlock);
    if (endBlock == std::string::npos)
        endBlock = avatarBlock + 4096;
    if (endBlock > html.size())
        endBlock = html.size();

    size_t gifPos = html.find(".gif", avatarBlock);
    if (gifPos == std::string::npos || gifPos >= endBlock)
        return "";

    size_t urlStart = html.rfind("http", gifPos);
    if (urlStart == std::string::npos || urlStart < avatarBlock)
        return "";

    return html.substr(urlStart, (gifPos + 4) - urlStart);
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
                m_hasUpdate.store(true);

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
                        m_commitMessage.replace(nPos, 2, " ");
                }
            }
        }
    }

    m_updateFinished.store(true);
}

void WebClient::QueueAvatarDownload(int playerIndex, uint64_t steam64)
{
    {
        std::lock_guard<std::mutex> lock(m_downloadedMutex);
        if (m_downloadedStatic.count(steam64))
            return;
    }

    std::lock_guard<std::mutex> lock(m_staticMutex);
    m_staticQueue.push({playerIndex, steam64});
    m_staticCv.notify_one();
}

void WebClient::InvalidateAvatar(uint64_t steam64)
{
    std::lock_guard<std::mutex> lock(m_downloadedMutex);
    m_downloadedStatic.erase(steam64);
    m_downloadedAnimated.erase(steam64);
}

void WebClient::PushCompleted(const DownloadedAvatar& data)
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    m_completedAvatars.push_back(data);
}

void WebClient::StaticAvatarWorkerLoop()
{
    while (m_running.load())
    {
        AvatarTask task;

        {
            std::unique_lock<std::mutex> lock(m_staticMutex);
            m_staticCv.wait(lock, [this]() {
                return !m_staticQueue.empty() || !m_running.load();
            });

            if (!m_running.load())
                break;

            task = m_staticQueue.front();
            m_staticQueue.pop();
        }

        {
            std::lock_guard<std::mutex> lock(m_downloadedMutex);
            if (m_downloadedStatic.count(task.steam64))
                continue;
        }

        DownloadedAvatar result;
        result.playerIndex = task.playerIndex;
        result.steam64 = task.steam64;
        result.success = false;
        result.isAnimated = false;

        std::string xmlUrl = "https://steamcommunity.com/profiles/" + std::to_string(task.steam64) + "?xml=1";
        std::string xmlBuffer = PerformHttpGetString(xmlUrl, 5);

        if (!xmlBuffer.empty())
        {
            std::string avatarUrl = ExtractXmlTag(xmlBuffer, "avatarMedium");

            if (!avatarUrl.empty())
            {
                result.imageData = PerformHttpGetBytes(avatarUrl, 5);
                if (!result.imageData.empty())
                    result.success = true;
            }
        }

        if (result.success)
        {
            {
                std::lock_guard<std::mutex> lock(m_downloadedMutex);
                m_downloadedStatic.insert(task.steam64);
            }

            PushCompleted(result);

            {
                std::lock_guard<std::mutex> lock(m_downloadedMutex);
                if (!m_downloadedAnimated.count(task.steam64))
                {
                    std::lock_guard<std::mutex> alock(m_animatedMutex);
                    m_animatedQueue.push(task);
                    m_animatedCv.notify_one();
                }
            }
        }
        else
        {
            PushCompleted(result);
        }
    }
}

void WebClient::AnimatedAvatarWorkerLoop()
{
    while (m_running.load())
    {
        AvatarTask task;

        {
            std::unique_lock<std::mutex> lock(m_animatedMutex);
            m_animatedCv.wait(lock, [this]() { return !m_animatedQueue.empty() || !m_running.load(); });

            if (!m_running.load())
                break;

            task = m_animatedQueue.front();
            m_animatedQueue.pop();
        }

        {
            std::lock_guard<std::mutex> lock(m_downloadedMutex);
            if (m_downloadedAnimated.count(task.steam64))
                continue;
        }

        std::string profileUrl = "https://steamcommunity.com/profiles/" + std::to_string(task.steam64);
        std::string htmlBuffer = PerformHttpGetString(profileUrl, 7);
        if (htmlBuffer.empty())
            continue;

        std::string gifUrl = ExtractAnimatedAvatarUrl(htmlBuffer);
        if (gifUrl.empty())
        {
            std::lock_guard<std::mutex> lock(m_downloadedMutex);
            m_downloadedAnimated.insert(task.steam64);
            continue;
        }

        std::vector<uint8_t> gifBytes = PerformHttpGetBytes(gifUrl, 7);
        if (gifBytes.empty())
            continue;

        int width = 0;
        int height = 0;
        int frameCount = 0;
        int* delays = nullptr;
        int channels = 0;

        uint8_t* rawData = stbi_load_gif_from_memory(
            gifBytes.data(),
            static_cast<int>(gifBytes.size()),
            &delays,
            &width,
            &height,
            &frameCount,
            &channels,
            4
        );

        if (!rawData || frameCount <= 1 || width <= 0 || height <= 0)
        {
            if (rawData) 
                stbi_image_free(rawData);
            if (delays)  
                stbi_image_free(delays);
            continue;
        }

        const int TARGET = 64;
        size_t srcBytes = static_cast<size_t>(width * height * 4);
        size_t dstBytes = static_cast<size_t>(TARGET * TARGET * 4);

        DownloadedAvatar result;
        result.playerIndex  = task.playerIndex;
        result.steam64 = task.steam64;
        result.success = true;
        result.isAnimated = true;
        result.gifWidth = TARGET;
        result.gifHeight = TARGET;
        result.gifFrameCount = frameCount;
        result.gifFrames.resize(frameCount);
        result.gifDelays.resize(frameCount);

        float scaleX = (float)width / (float)TARGET;
        float scaleY = (float)height / (float)TARGET;

        for (int f = 0; f < frameCount; f++)
        {
            result.gifFrames[f].resize(dstBytes);
            const uint8_t* src = rawData + f * srcBytes;
            uint8_t* dst = result.gifFrames[f].data();

            for (int y = 0; y < TARGET; y++)
            {
                for (int x = 0; x < TARGET; x++)
                {
                    float sx = (x + 0.5f) * scaleX - 0.5f;
                    float sy = (y + 0.5f) * scaleY - 0.5f;

                    int x0 = (int)sx; if (x0 < 0) x0 = 0;
                    int y0 = (int)sy; if (y0 < 0) y0 = 0;
                    int x1 = x0 + 1; if (x1 >= width)  x1 = width  - 1;
                    int y1 = y0 + 1; if (y1 >= height) y1 = height - 1;

                    float fx = sx - (float)x0;
                    float fy = sy - (float)y0;

                    for (int c = 0; c < 4; c++)
                    {
                        float p00 = src[(y0 * width + x0) * 4 + c];
                        float p10 = src[(y0 * width + x1) * 4 + c];
                        float p01 = src[(y1 * width + x0) * 4 + c];
                        float p11 = src[(y1 * width + x1) * 4 + c];

                        float val = p00 * (1-fx) * (1-fy) + p10 * fx * (1-fy) + p01 * (1-fx) * fy + p11 * fx * fy;

                        dst[(y * TARGET + x) * 4 + c] = (uint8_t)(val + 0.5f);
                    }
                }
            }

            float d = delays ? static_cast<float>(delays[f]) : 40.0f;
            if (d <= 0.0f) 
                d = 40.0f;
            result.gifDelays[f] = d;
        }

        stbi_image_free(rawData);
        stbi_image_free(delays);

        {
            std::lock_guard<std::mutex> lock(m_downloadedMutex);
            m_downloadedAnimated.insert(task.steam64);
        }

        PushCompleted(result);
    }
}

bool WebClient::PopCompletedAvatar(DownloadedAvatar& outData)
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    if (m_completedAvatars.empty())
        return false;

    outData = m_completedAvatars.back();
    m_completedAvatars.pop_back();
    return true;
}