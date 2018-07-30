#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "core/utils/log.h"

class ThreadPool {
private:
    // Thread safe implementation of a Queue using a std::queue
    template <typename T> class SafeQueue {
    private:
        std::queue<T> m_queue;
        std::mutex m_mutex;

    public:
        SafeQueue() {}
        ~SafeQueue() {}

        bool empty() {
            std::unique_lock<std::mutex> lock(m_mutex);
            return m_queue.empty();
        }

        int size() {
            std::unique_lock<std::mutex> lock(m_mutex);
            return m_queue.size();
        }

        void enqueue(T& t) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_queue.push(t);
        }

        bool dequeue(T& t) {
            std::unique_lock<std::mutex> lock(m_mutex);
            if(m_queue.empty()) {
                return false;
            }
            t = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }
    };

    class ThreadWorker {
    private:
        unsigned int m_id;
        ThreadPool* m_pool;

    public:
        ThreadWorker(ThreadPool* pool, const unsigned int id, allpix::LogLevel log_level) : m_id(id), m_pool(pool) {
            // Set logging level
            allpix::Log::setReportingLevel(log_level);
        }
        void operator()() {
            std::function<void()> func;
            bool dequeued;
            while(!m_pool->m_shutdown) {
                {
                    std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
                    if(m_pool->m_queue.empty()) {
                        m_pool->m_conditional_lock.wait(lock);
                    }
                    dequeued = m_pool->m_queue.dequeue(func);
                }
                if(dequeued) {
                    func();
                }
            }
        }
    };

    bool m_shutdown;
    SafeQueue<std::function<void()>> m_queue;
    std::vector<std::thread> m_threads;
    std::mutex m_conditional_mutex;
    std::condition_variable m_conditional_lock;

public:
    ThreadPool(const unsigned int n_threads, allpix::LogLevel log_level)
        : m_shutdown(false), m_threads(std::vector<std::thread>(n_threads)) {
        for(unsigned int i = 0; i < m_threads.size(); ++i) {
            m_threads[i] = std::thread(ThreadWorker(this, i, log_level));
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // Waits until threads finish their current task and shutdowns the pool
    void shutdown() {
        m_shutdown = true;
        m_conditional_lock.notify_all();

        for(unsigned int i = 0; i < m_threads.size(); ++i) {
            if(m_threads[i].joinable()) {
                m_threads[i].join();
            }
        }
    }

    // Submit a function to be executed asynchronously by the pool
    template <typename F, typename... Args> auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        // Create a function with bounded parameters ready to execute
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        // Encapsulate it into a shared ptr in order to be able to copy construct / assign
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

        // Wrap packaged task into void function
        std::function<void()> wrapper_func = [task_ptr]() { (*task_ptr)(); };

        // Enqueue generic wrapper function
        m_queue.enqueue(wrapper_func);

        // Wake up one thread if its waiting
        m_conditional_lock.notify_one();

        // Return future from promise
        return task_ptr->get_future();
    }
};
