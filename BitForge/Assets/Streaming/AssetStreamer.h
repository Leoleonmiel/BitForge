#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>
#include <atomic>

namespace Assets
{
    class AssetStreamer
    {
    public:
        ~AssetStreamer() { Stop(); }

        void Start(unsigned threadCount);
        void Stop();
        void Enqueue(std::function<void()> job);

        size_t Queued() const;

    private:
        std::vector<std::thread>          m_workers;
        std::queue<std::function<void()>> m_jobs;
        mutable std::mutex                m_mutex;
        std::condition_variable           m_cv;
        bool                              m_stop = false;
    };
}
