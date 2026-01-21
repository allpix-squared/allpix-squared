/**
 * @file
 * @brief Template implementation of thread pool for module multithreading
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <cassert>
#include <climits>

namespace allpix {
    template <typename T>
    ThreadPool::SafeQueue<T>::SafeQueue(unsigned int max_standard_size, unsigned max_priority_size)
        : max_standard_size_(max_standard_size), max_priority_size_(max_priority_size) {}

    /*
     * Block until a value is available if the wait parameter is set to true. The wait exits when the queue is invalidated.
     */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
    template <typename T> bool ThreadPool::SafeQueue<T>::pop(T& out, size_t buffer_left) {
        assert(buffer_left <= max_priority_size_);
        // Lock the mutex
        std::unique_lock<std::mutex> lock{mutex_};
        if(!valid_) {
            return false;
        }

        // Wait for one of the queues to be available
        bool pop_priority = !priority_queue_.empty() && priority_queue_.top().first == current_id_;
        bool pop_standard = !queue_.empty() && priority_queue_.size() + buffer_left <= max_priority_size_;
        while(!pop_priority && !pop_standard) {
            // Wait for new item in the queue (unlocks the mutex while waiting)
            pop_condition_.wait(lock);
            if(!valid_) {
                return false;
            }
            pop_priority = !priority_queue_.empty() && priority_queue_.top().first == current_id_;
            pop_standard = !queue_.empty() && priority_queue_.size() + buffer_left <= max_priority_size_;
        }

        // Pop the appropriate queue
        if(pop_priority) {
            // Priority queue is missing a pop returning a non-const reference, so need to apply a const_cast
            out = std::move(const_cast<PQValue&>(priority_queue_.top())).second; // NOLINT
            priority_queue_.pop();
            priority_queue_size_--;
        } else { // pop_standard
            out = std::move(queue_.front());
            queue_.pop();
        }

        // Notify possible pusher waiting to fill the queue
        lock.unlock();
        if(pop_priority) {
            pop_condition_.notify_one();
        }
        push_condition_.notify_one();
        return true;
    }
#pragma GCC diagnostic pop

    template <typename T> bool ThreadPool::SafeQueue<T>::push(T value, bool wait) {
        // Lock the mutex
        std::unique_lock<std::mutex> lock{mutex_};

        // Check if the queue reached its full size
        if(queue_.size() >= max_standard_size_) {
            // Wait until the queue is below the max size or it was invalidated(shutdown)
            if(!wait) {
                return false;
            }
            push_condition_.wait(lock, [this]() { return queue_.size() < max_standard_size_ || !valid_; });
        }

        // Abort the push operation if conditions not met
        if(queue_.size() >= max_standard_size_ || !valid_) {
            return false;
        }

        // Push a new element to the queue and notify possible consumer
        queue_.push(std::move(value));
        lock.unlock();
        pop_condition_.notify_one();
        return true;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
    template <typename T> bool ThreadPool::SafeQueue<T>::push(uint64_t n, T value, bool wait) {
        // Lock the mutex
        std::unique_lock<std::mutex> lock{mutex_};
        assert(n >= current_id_);

        // Check if the queue reached its full size
        if(priority_queue_.size() >= max_priority_size_) {
            // Wait until the queue is below the max size or it was invalidated(shutdown)
            if(!wait) {
                return false;
            }
            push_condition_.wait(lock, [this]() { return priority_queue_.size() < max_priority_size_ || !valid_; });
        }

        // Abort the push operation if conditions not met
        if(priority_queue_.size() >= max_priority_size_ || !valid_) {
            return false;
        }

        // Push a new element to the queue and notify possible consumer
        priority_queue_.emplace(n, std::move(value));
        priority_queue_size_++;
        lock.unlock();
        pop_condition_.notify_one();
        return true;
    }
#pragma GCC diagnostic pop

    template <typename T> void ThreadPool::SafeQueue<T>::complete(uint64_t n) {
        std::unique_lock<std::mutex> lock{mutex_};
        completed_ids_.insert(n);
        auto iter = completed_ids_.begin();
        while(iter != completed_ids_.end()) {
            if(*iter != current_id_) {
                return;
            }
            iter = completed_ids_.erase(iter);
            ++current_id_;
            lock.unlock();
            pop_condition_.notify_one();
            lock.lock();
        }
    }

    template <typename T> uint64_t ThreadPool::SafeQueue<T>::currentId() const {
        const std::lock_guard<std::mutex> lock{mutex_};
        return current_id_;
    }

    template <typename T> bool ThreadPool::SafeQueue<T>::valid() const {
        const std::lock_guard<std::mutex> lock{mutex_};
        return valid_;
    }

    template <typename T> bool ThreadPool::SafeQueue<T>::empty() const {
        const std::lock_guard<std::mutex> lock{mutex_};
        return !valid_ || (queue_.empty() && priority_queue_.empty());
    }

    template <typename T> size_t ThreadPool::SafeQueue<T>::size() const {
        const std::lock_guard<std::mutex> lock{mutex_};
        return queue_.size() + priority_queue_.size();
    }

    template <typename T> size_t ThreadPool::SafeQueue<T>::prioritySize() const { return priority_queue_size_; }

    /*
     * Used to ensure no conditions are being waited for in pop when a thread or the application is trying to exit. The queue
     * is invalid after calling this method and it is an error to continue using a queue after this method has been called.
     */
    template <typename T> void ThreadPool::SafeQueue<T>::invalidate() {
        std::unique_lock<std::mutex> lock{mutex_};
        std::priority_queue<PQValue, std::vector<PQValue>, std::greater<>>().swap(priority_queue_);
        priority_queue_size_ = 0;
        std::queue<T>().swap(queue_);
        valid_ = false;
        lock.unlock();
        push_condition_.notify_all();
        pop_condition_.notify_all();
    }

    template <typename Func, typename... Args> auto ThreadPool::submit(Func&& func, Args&&... args) {
        return submit(UINT64_MAX, std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args> auto ThreadPool::submit(uint64_t n, Func&& func, Args&&... args) {
        assert(n == UINT64_MAX || with_buffered_);
        // Bind the arguments to the tasks
        auto bound_task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

        // Construct packaged task with correct return type
        using PackagedTask = std::packaged_task<decltype(bound_task())()>;
        PackagedTask task(bound_task);

        // Get future and wrapper to add to vector
        auto future = task.get_future().share();
        auto task_function = [task = std::move(task), future = future]() mutable {
            task();
            future.get();
        };
        bool success = true;
        if(threads_.empty()) {
            task_function();
        } else {
            if(n == UINT64_MAX) {
                success = queue_.push(std::make_unique<std::packaged_task<void()>>(std::move(task_function)), true);
            } else {
                success = queue_.push(n, std::make_unique<std::packaged_task<void()>>(std::move(task_function)), false);
            }
            // Increment run count:
            ++run_cnt_;
        }
        if(success) {
            return future;
        }

        return decltype(future)();
    }

} // namespace allpix
