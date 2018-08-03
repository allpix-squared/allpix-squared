/**
 * @file
 * @brief Definition of thread pool for concurrent events
 * @copyright MIT License
 */

#ifndef ALLPIX_THREADPOOL_H
#define ALLPIX_THREADPOOL_H

#include <algorithm>
#include <atomic>
#include <exception>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <utility>

namespace allpix {
    /**
     * @brief Pool of threads where event tasks can be submitted to
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
             * @brief Return size of internal queue
             * @return Size of the size of the internal queue
             */
            size_t size() const;

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
         * @param worker_init_function Function run by all the workers to initialize
         * @param master_condition Condition to notify when modifying the event queue
         */
        explicit ThreadPool(unsigned int num_threads,
                            std::function<void()> worker_init_function,
                            std::condition_variable& master_condition);

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

    private:
        /**
         * @brief Submit wrapped event function for worker execution
         * @param event_function Function to execute (should call the run-method of the event)
         */
        void submit_event_function(std::function<void()> event_function);

        /**
         * @brief Return the number of enqueued events
         * @return The number of enqueued events
         */
        size_t queue_size() const;

        /**
         * @brief Check if any worker thread has thrown an exception
         * @throw Exception thrown by worker thread, if any
         */
        void check_exception();

        /**
         * @brief Waits for the worker threads to finish
         */
        void wait();

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

        SafeQueue<Task> event_queue_;

        std::atomic<unsigned int> run_cnt_;
        mutable std::mutex run_mutex_;
        std::condition_variable run_condition_;
        std::vector<std::thread> threads_;

        std::atomic_flag has_exception_;
        std::exception_ptr exception_ptr_{nullptr};
        std::condition_variable& master_condition_;
    };
} // namespace allpix

// Include template members
#include "ThreadPool.tpp"

#endif
