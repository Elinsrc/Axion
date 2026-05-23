#pragma once

#include <string>
#include <thread>
#include <atomic>

class UpdateChecker
{
public:
    UpdateChecker();
    ~UpdateChecker();

    void CheckAsync();
    bool IsFinished() const { return m_isFinished.load(); }
    bool HasUpdate() const { return m_hasUpdate.load(); }
    std::string GetRemoteHash() const { return m_remoteHash; }
    std::string GetCommitMessage() const { return m_commitMessage; }
private:
    void PerformCheck();
    std::string CleanHash(const std::string& rawHash) const;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

private:
    std::thread m_workerThread;
    std::atomic<bool> m_isFinished{false};
    std::atomic<bool> m_hasUpdate{false};
    std::string m_remoteHash;
    std::string m_commitMessage;
};

extern UpdateChecker* g_pUpdateChecker;
