#include "web_worker.h"

void WebWorker::Start()
{
    if (m_thread.joinable())
        return;

    m_stop.store(false);
    m_thread = std::thread(&WebWorker::Loop, this);
}

void WebWorker::Stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop.store(true);
    }
    m_cv.notify_all();

    if (m_thread.joinable())
        m_thread.join();

    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<Job>().swap(m_jobs);
}

void WebWorker::Post(Job job)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobs.push(std::move(job));
    }
    m_cv.notify_one();
}

void WebWorker::Loop()
{
    for (;;)
    {
        Job job;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return m_stop.load() || !m_jobs.empty(); });

            if (m_stop.load())
                return;

            job = std::move(m_jobs.front());
            m_jobs.pop();
        }

        if (job)
            job();
    }
}