/**
 * @file
 * @brief Implementation of thread pool for module multithreading
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
                       const std::vector<Module*>& modules,
                       const std::function<void()>& worker_init_function) {
    // Create threads
    try {
        for(unsigned int i = 0u; i < num_threads; ++i) {
            threads_.emplace_back(&ThreadPool::worker, this, worker_init_function);
        }
    } catch(...) {
        destroy();
        throw;
    }

    // Add module module queues
    for(auto& module : modules) {
        // Default constructs a queue for all modules
        // NOTE: this is the only valid method due to SafeTask not being movable
        task_queues_[module];
    }
    run_cnt_ = 0;
}

void ThreadPool::submit_module_function(std::function<void()> module_function) {
    module_queue_.push(std::make_unique<std::packaged_task<void()>>(std::move(module_function)));
    all_queue_.push(&module_queue_);
}

ThreadPool::~ThreadPool() {
    destroy();
}

/**
 * @warning This function does not wait for the all the running tasks to finish
 * @warning The module running this function is responsible for handling exceptions in the function called
 *
 * Should always be run by the thread spawning tasks, to ensure the task can be completed when there are no other threads
 * available to execute them.
 */
bool ThreadPool::execute(Module* module) {
    // Run tasks until the queue is empty
    Task task{nullptr};
    while(task_queues_.at(module).pop(task, false)) {
        // Execute task
        (*task)();
        // Fetch the future to propagate exceptions
        task->get_future().get();
    }
    return task_queues_.at(module).isValid();
}

/**
 * Run by the \ref ModuleManager to ensure all tasks and modules are completed before moving to the next instantiations.
 * Besides waiting for the queue to empty this will also wait for all the tasks to be completed. If an exception is
 * thrown by another thread, the exception will be propagated to the main thread by this function.
 */
bool ThreadPool::execute_all() {
    while(true) {
        // Run tasks until the queue is empty
        SafeQueue<Task>* queue_ptr;
        while(all_queue_.pop(queue_ptr, false)) {
            Task task{nullptr};
            if(queue_ptr->pop(task, false)) {
                try {
                    // Execute task
                    (*task)();
                    // Fetch the future to propagate exceptions
                    task->get_future().get();
                } catch(...) {
                    // Check if the first exception thrown
                    if(has_exception_.test_and_set()) {
                        // Check if the first exception thrown
                        exception_ptr_ = std::current_exception();
                        // Invalidate the queue to terminate other threads
                        all_queue_.invalidate();
                    }
                }
            }
        }

        // Wait for the threads to complete their task, continue helping if a new task was pushed
        std::unique_lock<std::mutex> lock{run_mutex_};
        run_condition_.wait(lock, [this]() { return !all_queue_.empty() || run_cnt_ == 0; });

        // Only stop when both the queue is empty and the run count is zero
        if(all_queue_.empty() && run_cnt_ == 0) {
            break;
        }
    }

    // If exception has been thrown, destroy pool and propagate it
    if(exception_ptr_) {
        destroy();
        Log::setSection("");
        std::rethrow_exception(exception_ptr_);
    }

    return all_queue_.isValid();
}

/**
 * If an exception is thrown by a module, the first exception is saved to propagate in the main thread
 */
void ThreadPool::worker(const std::function<void()>& init_function) {
    // Initialize the worker
    init_function();

    // Safe lambda to increase the atomic run count
    auto increase_run_cnt_func = [this]() { ++run_cnt_; };

    // Continue running until the thread pool is finished
    while(!done_) {
        SafeQueue<Task>* queue_ptr;
        if(all_queue_.pop(queue_ptr, true, increase_run_cnt_func)) {
            Task task{nullptr};
            if(queue_ptr->pop(task, false)) {
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
                        all_queue_.invalidate();
                    }
                }
            }

            // Propagate that the task has been finished
            std::lock_guard<std::mutex> lock{run_mutex_};
            --run_cnt_;
            run_condition_.notify_all();
        }
    }
}

void ThreadPool::destroy() {
    done_ = true;

    all_queue_.invalidate();
    module_queue_.invalidate();
    for(auto& queue : task_queues_) {
        queue.second.invalidate();
    }

    for(auto& thread : threads_) {
        if(thread.joinable()) {
            thread.join();
        }
    }
}
