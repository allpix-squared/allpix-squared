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
    using Task = std::unique_ptr<std::packaged_task<void()>>;

    std::atomic_bool done_{false};

    allpix::ThreadPool::SafeQueue<Task> queue_;

    std::atomic<unsigned int> run_cnt_;
    std::mutex run_mutex_;
    std::condition_variable run_condition_;
    std::vector<std::thread> threads_;

    std::atomic_flag has_exception_;
    std::exception_ptr exception_ptr_{nullptr};

    /**
     * @brief Constantly running internal function each thread uses to acquire work items from the queue.
     * @param init_function Function to initialize the relevant thread_local variables
     */
    void worker(const std::function<void()>& init_function) {
        // Initialize the worker
        init_function();

        // Safe lambda to increase the atomic run count
        auto increase_run_cnt_func = [this]() noexcept { ++run_cnt_; };

        // Continue running until the thread pool is finished
        while(!done_) {
            Task task{nullptr};
            if(queue_.pop(task, true, increase_run_cnt_func)) {
                // Try to run task
                try {
                    // Execute task
                    (*task)();
                    // Fetch the future to propagate exceptions
                    task->get_future().get();
                } catch(...) {
                    // Check if the first exception thrown
                    if(has_exception_.test_and_set()) {
                        // Save first exception
                        exception_ptr_ = std::current_exception();
                        // Invalidate the queue to terminate other threads
                        queue_.invalidate();
                    }
                }

                // Propagate that the task has been finished
                std::lock_guard<std::mutex> lock{run_mutex_};
                --run_cnt_;
                run_condition_.notify_all();
            }
        }
    }

public:
    /**
     * @brief Construct thread pool with provided number of threads
     * @param num_threads Number of threads in the pool
     * @param worker_init_function Function run by all the workers to initialize
     */
    explicit ThreadPool(const unsigned int num_threads, const std::function<void()>& worker_init_function) {
        // Create threads
        try {
            for(unsigned int i = 0u; i < num_threads; ++i) {
                threads_.emplace_back(&ThreadPool::worker, this, worker_init_function);
            }
        } catch(...) {
            destroy();
            throw;
        }

        run_cnt_ = 0;
    }

    /**
     * @brief Destroy and wait for all threads to finish on destruction
     */
    ~ThreadPool() { destroy(); }

    /// @{
    /**
     * @brief Copying the thread pool is not allowed
     */
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    /// @}

    /**
     * @brief Invalidate all queues and joins all running threads when the pool is destroyed.
     */
    void destroy() {
        done_ = true;

        queue_.invalidate();
        for(auto& thrd : threads_) {
            if(thrd.joinable()) {
                thrd.join();
            }
        }
    }

    /**
     * @brief Submit a job to be run by the thread pool
     * @param func Function to execute by the pool
     * @param args Parameters to pass to the function
     * @warning The thread submitting task should always call the \ref ThreadPool::execute method to prevent a lock when
     *          there are no threads available
     */
    template <typename Func, typename... Args> auto submit(Func&& func, Args&&... args) {
        // Bind the arguments to the tasks
        auto bound_task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

        // Construct packaged task with correct return type
        using PackagedTask = std::packaged_task<decltype(bound_task())()>;
        PackagedTask task(bound_task);

        // Get future and wrapper to add to vector
        auto future = task.get_future();
        auto task_function = [task = std::move(task)]() mutable { task(); };
        queue_.push(std::make_unique<std::packaged_task<void()>>(std::move(task_function)));
        return future;
    }
};

#endif
