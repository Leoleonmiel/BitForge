#include "AssetStreamer.h"

namespace Assets
{
    void AssetStreamer::Start(unsigned threadCount)
    {
        if (threadCount == 0)
        {
            unsigned hw = std::thread::hardware_concurrency();
            threadCount = (hw > 2) ? hw - 1 : 2;
        }

        m_stop = false;
        for (unsigned i = 0; i < threadCount; i++)
        {
            m_workers.emplace_back([this]
            {
                for (;;)
                {
                    std::function<void()> job;
                    {
                        std::unique_lock<std::mutex> lk(m_mutex);
                        m_cv.wait(lk, [this] { return m_stop || !m_jobs.empty(); });
                        if (m_stop && m_jobs.empty()) { return; }
                        job = std::move(m_jobs.front());
                        m_jobs.pop();
                    }
                    job();
                }
            });
        }
    }

    void AssetStreamer::Stop()
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (m_stop && m_workers.empty()) { return; }
            m_stop = true;
        }
        m_cv.notify_all();
        for (auto& worker : m_workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
        m_workers.clear();
    }

    void AssetStreamer::Enqueue(std::function<void()> job)
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_jobs.push(std::move(job));
        }
        m_cv.notify_one();
    }

    size_t AssetStreamer::Queued() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_jobs.size();
    }
}
