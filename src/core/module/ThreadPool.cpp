/**
 * @file
 * @brief Definition of thread pool for concurrent events
 * @copyright MIT License
 */

#include "ThreadPool.hpp"
#include "Module.hpp"

using namespace allpix;

/**
 * The threads are created in an exception-safe way and all of them will be destroyed when creation of one fails
 */
ThreadPool::ThreadPool(unsigned int num_threads,
                       std::function<void()> worker_init_function,
                       std::function<void()> worker_finalize_function,
                       std::condition_variable& master_condition)
    : master_condition_(master_condition), worker_finalize_function_(worker_finalize_function) {
    // Create threads
    try {
        for(unsigned int i = 0u; i < num_threads; ++i) {
            threads_.emplace_back(&ThreadPool::worker, this, worker_init_function);
        }
    } catch(...) {
        destroy();
        throw;
    }

    // No threads are currently working
    run_cnt_ = 0;
}

ThreadPool::~ThreadPool() {
    destroy();
}

void ThreadPool::submit_event_function(std::function<void()> event_function) {
    if(threads_.empty()) {
        event_function();
    } else {
        event_queue_.push(std::make_unique<std::packaged_task<void()>>(std::move(event_function)));
    }
}

size_t ThreadPool::queue_size() const {
    return event_queue_.size();
}

void ThreadPool::check_exception() {
    // If exception has been thrown, destroy pool and propagate it
    if(exception_ptr_) {
        destroy();
        Log::setSection("");
        std::rethrow_exception(exception_ptr_);
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock{run_mutex_};
    run_condition_.wait(lock, [this]() { return exception_ptr_ || (event_queue_.empty() && run_cnt_ == 0); });
}

/**
 * If an exception is thrown by a module, the first exception is saved to propagate in the main thread
 */
void ThreadPool::worker(const std::function<void()>& init_function) {
    // Initialize the worker
    init_function();

    // Increase the atomic run count and notify the master thread that we popped an event
    auto increase_run_cnt_func = [this]() {
        ++run_cnt_;
        master_condition_.notify_one();
    };

    while(!done_) {
        Task task{nullptr};

        master_condition_.notify_one();
        if(event_queue_.pop(task, true, increase_run_cnt_func)) {
            // Try to run the task
            try {
                // Execute task
                (*task)();
                // Fetch the future to propagate exceptions
                task->get_future().get();
            } catch(...) {
                // Check if the first exception thrown
                if(has_exception_.test_and_set()) {
                    // Save the first exceptin
                    exception_ptr_ = std::current_exception();
                    // Invalidate the queue to terminate other threads
                    event_queue_.invalidate();
                    // Notify master thread that an exception has thrown
                    master_condition_.notify_one();
                }
            }
        }

        // Propagate that the task has been finished
        std::lock_guard<std::mutex> lock{run_mutex_};
        --run_cnt_;
        run_condition_.notify_all();
    }

    // Execute the cleanup function at the end of loop
    if(worker_finalize_function_) {
        worker_finalize_function_();
    }
}

void ThreadPool::destroy() {
    done_ = true;
    event_queue_.invalidate();

    for(auto& thread : threads_) {
        if(thread.joinable()) {
            thread.join();
        }
    }
}
