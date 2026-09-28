#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class WebWorker
{
public:
    using Job = std::function<void()>;

    WebWorker() = default;
    ~WebWorker() { Stop(); }

    WebWorker(const WebWorker&) = delete;
    WebWorker& operator=(const WebWorker&) = delete;

    void Start();
    void Stop();
    void Post(Job job);

    const std::atomic<bool>& StopFlag() const { return m_stop; }

private:
    void Loop();

    std::thread m_thread;
    std::queue<Job> m_jobs;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stop{ false };
};