/**
 * @file
 * @brief Implementation of thread pool for concurrent events
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "ThreadPool.hpp"
#include "Module.hpp"

using namespace allpix;

/**
 * The threads are created in an exception-safe way and all of them will be destroyed when creation of one fails
 */
ThreadPool::ThreadPool(unsigned int num_threads,
                       unsigned int max_queue_size,
                       const std::function<void()>& worker_init_function,
                       const std::function<void()>& worker_finalize_function)
    : queue_(max_queue_size) {
    // Create threads
    try {
        for(unsigned int i = 0u; i < num_threads; ++i) {
            threads_.emplace_back(&ThreadPool::worker, this, worker_init_function, worker_finalize_function);
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

size_t ThreadPool::queueSize() const {
    return queue_.size();
}

void ThreadPool::checkException() {
    // If exception has been thrown, destroy pool and propagate it
    if(exception_ptr_) {
        destroy();
        Log::setSection("");
        std::rethrow_exception(exception_ptr_);
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock{run_mutex_};
    run_condition_.wait(lock, [this]() { return exception_ptr_ != nullptr || (queue_.empty() && run_cnt_ == 0); });
}

/**
 * If an exception is thrown by a module, the first exception is saved to propagate in the main thread
 */
void ThreadPool::worker(const std::function<void()>& initialize_function, const std::function<void()>& finalize_function) {
    try {
        // Initialize the worker
        if(initialize_function) {
            initialize_function();
        }

        // Increase the atomic run count and notify the master thread that we popped an event
        auto increase_run_cnt_func = [this]() noexcept { ++run_cnt_; };

        while(!done_) {
            Task task{nullptr};

            if(queue_.pop(task, increase_run_cnt_func)) {
                // Execute task
                (*task)();
                // Fetch the future to propagate exceptions
                task->get_future().get();
                // Update the run count and propagate update
                std::lock_guard<std::mutex> lock{run_mutex_};
                --run_cnt_;
                run_condition_.notify_all();
            }
        }

        // Execute the cleanup function at the end of run
        if(finalize_function) {
            finalize_function();
        }
    } catch(...) {
        // Check if the first exception thrown
        if(!has_exception_.test_and_set()) {
            // Save the first exception
            exception_ptr_ = std::current_exception();
            // Invalidate the queue to terminate other threads
            queue_.invalidate();
        }
        // Propagate that the worker terminated
        run_condition_.notify_all();
    }
}

void ThreadPool::destroy() {
    done_ = true;
    queue_.invalidate();

    for(auto& thread : threads_) {
        if(thread.joinable()) {
            thread.join();
        }
    }
}
