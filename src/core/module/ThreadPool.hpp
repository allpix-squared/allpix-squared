/**
 * @file
 * @brief Definition of thread pool for concurrent events
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_THREADPOOL_H
#define ALLPIX_THREADPOOL_H

#include <algorithm>
#include <atomic>
#include <exception>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <thread>
#include <utility>

namespace allpix {
    /**
     * @brief Pool of threads where event tasks can be submitted to
     */
    class ThreadPool {
    public:
        /**
         * @brief Internal thread-safe queuing system
         *
         * It internally consists of two separate queues
         * - A standard queue pushed in order of jobs to process
         * - An ordered priority queue for work that need linear processing
         *
         * The priority queue is popped if the top of the queue can be directly processed. Otherwise work is popped from the
         * default queue unless the priority queue size is too large.
         */
        template <typename T> class SafeQueue {
        public:
            /**
             * @brief Default constructor, initializes empty queue
             * @param max_standard_size Max size of the default queue
             * @param max_priority_size Max size of the priority queue
             */
            SafeQueue(unsigned int max_standard_size, unsigned int max_priority_size);

            /**
             * @brief Erases the queue and release waiting threads on destruction
             */
            ~SafeQueue() { invalidate(); };

            /**
             * @brief Get the top value from the appropriate queue
             * @param out Reference where the value at the top of the queue will be written to
             * @param buffer_left Optional number of jobs that should be left in priority buffer without stall on push
             * @return True if a task was acquired or false if pop was exited for another reason
             */
            bool pop(T& out, size_t buffer_left = 0);

            /**
             * @brief Push a new value onto the standard queue, will block if queue is full
             * @param value Value to push to the queue
             * @param wait If the push is allowed to stall if there is no capacity
             * @return If the push was successful
             */
            bool push(T value, bool wait = true);
            /**
             * @brief Push a new value onto the priority queue
             * @param n Ordering identifier for the priority
             * @param value Value to push to the queue
             * @param wait If the push is allowed to stall if there is no capacity
             * @return If the push was successful
             */
            bool push(uint64_t n, T value, bool wait = true);

            /**
             * @brief Mark an identifier as complete
             * @param n Identifier that is complete
             */
            void complete(uint64_t n);

            /**
             * @brief Get current identifier (last uncompleted)
             * @return Current identifier
             */
            uint64_t currentId() const;

            /**
             * @brief Return if the queue system in a valid state
             * @return True if the queue is valid, false if \ref SafeQueue::invalidate has been called
             */
            bool valid() const;

            /**
             * @brief Return if all related queues are empty or not
             * @return True if queues are empty, false otherwise
             */
            bool empty() const;

            /**
             * @brief Return total size of values stored in both queues
             * @return Size of of the internal queues
             */
            size_t size() const;

            /**
             * @brief Return total size of values stored in both queues
             * @return Size of of the internal queues
             */
            size_t prioritySize() const;

            /**
             * @brief Invalidate the queue
             */
            void invalidate();

        private:
            std::atomic_bool valid_{true};
            mutable std::mutex mutex_{};
            std::queue<T> queue_;
            std::set<uint64_t> completed_ids_;
            uint64_t current_id_{0};
            using PQValue = std::pair<uint64_t, T>;
            std::priority_queue<PQValue, std::vector<PQValue>, std::greater<>> priority_queue_;
            std::atomic_size_t priority_queue_size_{0};
            std::condition_variable push_condition_;
            std::condition_variable pop_condition_;
            const size_t max_standard_size_;
            const size_t max_priority_size_;
        };

        /**
         * @brief Construct thread pool with provided number of threads without buffered jobs
         * @param num_threads Number of threads in the pool
         * @param max_queue_size Maximum size of the standard job queue
         * @param worker_init_function Function run by all the workers to initialize
         * @param worker_finalize_function Function run by all the workers to cleanup
         * @warning Total count of threads need to be preregistered via \ref ThreadPool::registerThreadCount
         */
        ThreadPool(unsigned int num_threads,
                   unsigned int max_queue_size,
                   const std::function<void()>& worker_init_function = nullptr,
                   const std::function<void()>& worker_finalize_function = nullptr);

        /**
         * @brief Construct thread pool with provided number of threads with buffered jobs
         * @param num_threads Number of threads in the pool
         * @param max_queue_size Maximum size of the standard job queue
         * @param max_buffered_size Maximum size of the buffered job queue (should be at least number of threads)
         * @param worker_init_function Function run by all the workers to initialize
         * @param worker_finalize_function Function run by all the workers to cleanup
         * @warning Total count of threads need to be preregistered via \ref ThreadPool::registerThreadCount
         */
        ThreadPool(unsigned int num_threads,
                   unsigned int max_queue_size,
                   unsigned int max_buffered_size,
                   const std::function<void()>& worker_init_function = nullptr,
                   const std::function<void()>& worker_finalize_function = nullptr);

        /// @{
        /**
         * @brief Copying the thread pool is not allowed
         */
        ThreadPool(const ThreadPool& rhs) = delete;
        ThreadPool& operator=(const ThreadPool& rhs) = delete;
        /// @}

        /**
         * @brief Submit a standard job to be run by the thread pool. In case no workers are registered, the function will be
         * executed immediately.
         * @param func Function to execute by the pool
         * @param args Parameters to pass to the function
         */
        template <typename Func, typename... Args> auto submit(Func&& func, Args&&... args);
        /**
         * @brief Submit a priority job to be run by the thread pool. In case no workers are registered, the function will be
         * executed immediately.
         * @param n Priority identifier or UINT64_MAX for non-prioritized submission
         * @param func Function to execute by the pool
         * @param args Parameters to pass to the function
         *
         * @warning This function can only be called if thread pool was initialized with buffered jobs
         */
        template <typename Func, typename... Args> auto submit(uint64_t n, Func&& func, Args&&... args);

        /**
         * @brief Mark identifier as completed
         * @param n Identifier that is complete
         */
        void markComplete(uint64_t n);

        /**
         * @brief Get the lowest ID that is not completely processed yet
         * @return n Identifier that is not yet completed
         */
        uint64_t minimumUncompleted() const { return queue_.currentId(); }

        /**
         * @brief Return the total number of enqueued jobs
         * @return The number of enqueued jobs
         */
        size_t queueSize() const { return queue_.size(); }

        /**
         * @brief Return the number of jobs in buffered priority queue
         * @return The number of enqueued jobs in the buffered queue
         */
        size_t bufferedQueueSize() const { return queue_.prioritySize(); }

        /**
         * @brief Check if any worker thread has thrown an exception
         * @throw Exception thrown by worker thread, if any
         */
        void checkException();

        /**
         * @brief Waits for the worker threads to finish
         */
        void wait();

        /**
         * @brief Invalidate all queues and joins all running threads when the pool is destroyed
         */
        void destroy();

        /**
         * @brief Returns if the threadpool is in a valid state (has not been invalidated)
         */
        bool valid();

        /**
         * @brief Destroy and wait for all threads to finish on destruction
         */
        ~ThreadPool();

        /**
         * @brief Get the unique number of the current thread between 0 and the total number of threads
         * @return Number of the current thread
         */
        static unsigned int threadNum();

        /**
         * @brief Get the number of thread including the main thread (thus always at least one)
         * @return Total count of threads
         */
        static unsigned int threadCount();

        /**
         * @brief Add number to the total count of threads that could be used
         * @param cnt Additional number of thread to preregister
         */
        static void registerThreadCount(unsigned int cnt);

    private:
        /**
         * @brief Constantly running internal function each thread uses to acquire work items from the queue.
         * @param min_thread_buffer   Minimum buffer size to keep available without stall on push
         * @param initialize_function Function to initialize the thread
         * @param finalize_function   Function to finalize the thread
         */
        void worker(size_t min_thread_buffer,
                    const std::function<void()>& initialize_function,
                    const std::function<void()>& finalize_function);

        // The queue holds the task functions to be executed by the workers
        using Task = std::unique_ptr<std::packaged_task<void()>>;
        SafeQueue<Task> queue_;
        bool with_buffered_{true};
        std::function<void()> finalize_function_{};

        std::atomic_bool done_{false};

        std::atomic<unsigned int> run_cnt_{0};
        mutable std::mutex run_mutex_{};
        std::condition_variable run_condition_;
        std::vector<std::thread> threads_;

        std::atomic_flag has_exception_{false};
        std::exception_ptr exception_ptr_{nullptr};

        static std::map<std::thread::id, unsigned int> thread_nums_;
        static std::atomic_uint thread_cnt_;
        static std::atomic_uint thread_total_;
    };
} // namespace allpix

// Include template members
#include "ThreadPool.tpp"

#endif /* ALLPIX_THREADPOOL_H */
