#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <condition_variable>
#include <iostream>
#include <functional>
#include <future>
#include <cassert>

class ThreadPool {
public:
    /**
     * @brief Construct thread pool with provided number of threads
     * @param num_threads Number of threads in the pool
     * @param worker_init_function Function run by all the workers to initialize
     */
    explicit ThreadPool(unsigned int num_threads, std::function<void()> worker_init_function)
    {
        // Create threads
        try {
            for (unsigned int i = 0; i < num_threads; ++i)
                threads_.emplace_back(&ThreadPool::worker, this, worker_init_function);
        } catch (...) {
            destroy();
            throw;
        }
    }

    /// @{
    /**
     * @brief Copying the thread pool is not allowed
     */
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    /// @}

    /**
     * @brief Destroy and wait for all threads to finish on destruction
     */
    ~ThreadPool()
    {
        destroy();
    }

    /**
     * @brief Submit a task over to a waiting worker thread, blocks if no worker is waiting
     * @param event_function The task to be submitted
     */
    void submit(std::function<void()> task)
    {
        {
            // Wait for the slot to be freed
            std::unique_lock<std::mutex> lock{slot_mutex_};
            if (slot_.get() != nullptr)
                waiting_submitters_.wait(lock, [this]() { return slot_.get() == nullptr; });

            // Fill the slot and notify a waiting worker
            slot_ = std::make_unique<std::packaged_task<void()>>(std::move(task));
            waiting_workers_.notify_one();
        }

        // If exception has been thrown, destroy pool and propagate it
        if (exception_ptr_) {
            destroy();
            std::rethrow_exception(exception_ptr_);
        }
    }

private:
    void destroy()
    {
        done_ = true;

        waiting_workers_.notify_all();
        for (auto &thread : threads_) {
            if (thread.joinable())
                thread.join();
        }
    }

    /**
     * @brief Constantly running internal function each thread uses to acquire work task from submitting threads
     * @param init_function Function to initialize the thread with
     */
    void worker(const std::function<void()> &init_function)
    {
        // Initialize the worker
        init_function();

        // Continue running until the thread pool is finished
        while (true) {
            Task task{nullptr};

            {
                // Wait for the task slot to fill
                std::unique_lock<std::mutex> lock{slot_mutex_};
                if (slot_.get() == nullptr && !done_) {
                    waiting_workers_.wait(lock, [this]() {
                        return (slot_.get() != nullptr || done_);
                    });
                }

                if (done_) return;

                // Aqcuire the task, free the slot and notify a submitter
                task = std::move(slot_);
                slot_ = nullptr;
                waiting_submitters_.notify_one();
            }

            try {
                // Execute task
                assert(task != nullptr);
                (*task)();
                // Fetch the future to propagate exceptions
                task->get_future().get();
            } catch (...) {
                // Check if the first exception thrown
                if (has_exception_.test_and_set()) {
                    // Save first exception
                    exception_ptr_ = std::current_exception();
                }
            }
        }
    }

    using Task = std::unique_ptr<std::packaged_task<void()>>;
    Task slot_;
    std::mutex slot_mutex_;

    std::atomic_bool done_{false};

    std::vector<std::thread> threads_;
    std::condition_variable waiting_workers_;
    std::condition_variable waiting_submitters_;

    std::atomic_flag has_exception_;
    std::exception_ptr exception_ptr_{nullptr};
};

int main()
{
    auto worker_init_function = []() { std::cout << "Thread started\n"; };
    auto thread_pool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency(),
        worker_init_function);

    try {
        for (int i = 0; i < 20; ++i) {
            thread_pool->submit([]() {
                using namespace std::literals;

                std::this_thread::sleep_for(200ms);
                std::cout << "Task finished!\n";
            });
        }
    } catch (...) {
        std::cerr << "Exception caught!\n";
    }

    return 0;
}
