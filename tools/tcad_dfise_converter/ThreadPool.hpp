#ifndef DFISE_THREADPOOL_H
#define DFISE_THREADPOOL_H

// NOTE: class is added here temporarily as the allpix ThreadPool cannot be used and threading will be redesigned

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "core/module/ThreadPool.hpp"
#include "core/utils/log.h"

class ThreadPool {
private:
    class ThreadWorker {
    private:
        ThreadPool* pool_;

    public:
        ThreadWorker(ThreadPool* pool, allpix::LogLevel log_level) : pool_(pool) {
            // Set logging level
            allpix::Log::setReportingLevel(log_level);
        }
        void operator()() {
            std::function<void()> func;
            bool dequeued;
            while(!pool_->shutdown_) {
                {
                    std::unique_lock<std::mutex> lock(pool_->conditional_mutex_);
                    if(pool_->queue_.empty()) {
                        pool_->conditional_lock_.wait(lock);
                    }
                    dequeued = pool_->queue_.pop(func);
                }
                if(dequeued) {
                    func();
                }
            }
        }
    };

    bool shutdown_;
    allpix::ThreadPool::SafeQueue<std::function<void()>> queue_;
    std::vector<std::thread> threads_;
    std::mutex conditional_mutex_;
    std::condition_variable conditional_lock_;

public:
    ThreadPool(const unsigned int n_threads, allpix::LogLevel log_level)
        : shutdown_(false), threads_(std::vector<std::thread>(n_threads)) {
        for(auto& thread : threads_) {
            thread = std::thread(ThreadWorker(this, log_level));
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // Waits until threads finish their current task and shutdowns the pool
    void shutdown() {
        shutdown_ = true;
        conditional_lock_.notify_all();
        queue_.invalidate();

        for(auto& thrd : threads_) {
            if(thrd.joinable()) {
                thrd.join();
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
        queue_.push(wrapper_func);

        // Wake up one thread if its waiting
        conditional_lock_.notify_one();

        // Return future from promise
        return task_ptr->get_future();
    }
};

#endif
