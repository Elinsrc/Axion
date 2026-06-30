#include "update_checker.h"
#include "build_info.h"
#include <iostream>
#include <algorithm>
#include <curl/curl.h>

UpdateChecker* g_pUpdateChecker = nullptr;

UpdateChecker::UpdateChecker() 
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

UpdateChecker::~UpdateChecker()
{
    if (m_workerThread.joinable()) 
        m_workerThread.join();
    curl_global_cleanup();
}

void UpdateChecker::CheckAsync()
{
    if (m_workerThread.joinable())
        return;
    m_isFinished.store(false);
    m_hasUpdate.store(false);
    m_workerThread = std::thread(&UpdateChecker::PerformCheck, this);
}

size_t UpdateChecker::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) 
{
    size_t totalSize = size * nmemb;
    if (userp) 
    {
        std::string* response = static_cast<std::string*>(userp);
        response->append(static_cast<char*>(contents), totalSize);
    }
    return totalSize;
}

std::string UpdateChecker::CleanHash(const std::string& rawHash) const 
{
    std::string clean = rawHash;
    size_t dirtyPos = clean.find("-dirty");
    if (dirtyPos != std::string::npos) 
        clean = clean.substr(0, dirtyPos);
    std::transform(clean.begin(), clean.end(), clean.begin(), ::tolower);
    return clean;
}

void UpdateChecker::PerformCheck() 
{
    std::string rawLocalSha = BuildInfo::GetCommitHash();
    if (rawLocalSha == "notset") 
    {
        m_isFinished.store(true);
        return;
    }

    std::string localSha = CleanHash(rawLocalSha);

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        m_isFinished.store(true);
        return;
    }

    std::string url = "https://api.github.com/repos/Elinsrc/Axion/commits?per_page=100";
    std::string readBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 7L); 
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 4L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) 
    {
        m_isFinished.store(true);
        return;
    }

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

    m_isFinished.store(true);
}
