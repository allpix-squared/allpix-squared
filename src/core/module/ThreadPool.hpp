/**
 * @file
 * @brief Definition of thread pool for module multithreading
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_THREADPOOL_H
#define ALLPIX_THREADPOOL_H

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <iostream>

namespace allpix {
    class Module;

    /**
     * @brief Pool of threads where module tasks can be submitted to
     */
    class ThreadPool {
        friend class ModuleManager;

    public:
        /**
         * @brief Internal thread-safe queue
         */
        template <typename T> class SafeQueue {
        public:
            /**
             * @brief Default constructor, initializes empty queue
             */
            SafeQueue() = default;

            /**
             * @brief Erases the queue and release waiting threads on destruction
             */
            ~SafeQueue();

            /**
             * @brief Get the top value in the safe queue
             * @param out Reference where the value at the top of the queue will be written to
             * @param wait If the method should wait for new tasks or should exit (defaults to wait)
             * @param func Optional function to execute before releasing the queue mutex if pop was successful
             * @return True if a task was acquired or false if pop was exited for another reason
             */
            bool pop(T& out, bool wait = true, const std::function<void()>& func = nullptr);

            /**
             * @brief Push a new value onto the safe queue
             * @param value Value to push to the queue
             */
            void push(T value);

            /**
             * @brief Return if the queue is in a valid state
             * @return True if the queue is valid, false if \ref SafeQueue::invalidate has been called
             */
            bool isValid() const;

            /**
             * @brief Return if the queue is empty or not
             * @return True if the queue is empty, false otherwise
             */
            bool empty() const;

            /**
             * @brief Invalidate the queue
             */
            void invalidate();

        private:
            std::atomic_bool valid_{true};
            mutable std::mutex mutex_;
            std::queue<T> queue_;
            std::condition_variable condition_;
        };

        /**
         * @brief Construct thread pool with provided number of threads
         * @param num_threads Number of threads in the pool
         * @param modules List of module instantiations to create a task queue for
         * @param worker_init_function Function run by all the workers to initialize
         * @warning Only module instantiations that are registered in this constructor can spawn tasks
         */
        explicit ThreadPool(unsigned int num_threads,
                            const std::vector<Module*>& modules,
                            const std::function<void()>& worker_init_function);

        /// @{
        /**
         * @brief Copying the thread pool is not allowed
         */
        ThreadPool(const ThreadPool& rhs) = delete;
        ThreadPool& operator=(const ThreadPool& rhs) = delete;
        /// @}

        /**
         * @brief Destroy and wait for all threads to finish on destruction
         */
        ~ThreadPool();

        /**
         * @brief Submit a job from a module to be run by the thread pool
         * @param module Module the task belongs to
         * @param func Function to execute by the pool
         * @param args Parameters to pass to the function
         * @warning The thread submitting task should always call the \ref ThreadPool::execute method to prevent a lock when
         *          there are no threads available
         */
        template <typename Func, typename... Args> auto submit(Module* module, Func&& func, Args&&... args);

        /**
         * @brief Execute jobs from the module queue until all module tasks are finished or an interrupt happened
         * @param module Module to run tasks for
         * @return True if module task queue finished, false if stopped for other reason
         */
        bool execute(Module* module);

    private:
        /**
         * @brief Function to run a single event for a module by the \ref ModuleManager
         * @param module_function Function to execute (should call the run-method of the module)
         * @warning This method can only be called by the \ref ModuleManager
         */
        void submit_module_function(std::function<void()> module_function);

        /**
         * @brief Execute jobs from the queue until all tasks and modules are finished or an interrupt happened
         * @return True if module task queue finished, false if stopped for other reason
         * @warning This method can only be called by the \ref ModuleManager
         */
        bool execute_all();

        /**
         * @brief Constantly running internal function each thread uses to acquire work items from the queue.
         * @param init_function Function to initialize the relevant thread_local variables
         */
        void worker(const std::function<void()>& init_function);

        /**
         * @brief Invalidate all queues and joins all running threads when the pool is destroyed.
         */
        void destroy();

        using Task = std::unique_ptr<std::packaged_task<void()>>;

        std::atomic_bool done_{false};

        SafeQueue<SafeQueue<Task>*> all_queue_;
        SafeQueue<Task> module_queue_;
        std::map<Module*, SafeQueue<Task>> task_queues_;

        std::atomic<unsigned int> run_cnt_;
        mutable std::mutex run_mutex_;
        std::condition_variable run_condition_;
        std::vector<std::thread> threads_;

        std::atomic_flag has_exception_;
        std::exception_ptr exception_ptr_{nullptr};
    };
} // namespace allpix

// Include template members
#include "ThreadPool.tpp"

#endif /* ALLPIX_THREADPOOL_H */
