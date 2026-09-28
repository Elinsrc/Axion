#pragma once

#include <atomic>
#include <string>

#include "web_worker.h"

class CUpdateChecker
{
public:
    void CheckAsync();
    void Shutdown();

    bool IsChecked() const { return m_finished.load(); }
    bool HasUpdate() const { return m_hasUpdate.load(); }
    std::string GetRemoteHash() const { return m_remoteHash; }
    std::string GetCommitMessage() const { return m_commitMessage; }

private:
    void Run();
    void Check();

    WebWorker m_worker;
    std::atomic<bool> m_started{false};
    std::atomic<bool> m_finished{false};
    std::atomic<bool> m_hasUpdate{false};

    std::string m_remoteHash;
    std::string m_commitMessage;
};

extern CUpdateChecker g_UpdateChecker;