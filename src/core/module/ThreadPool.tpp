/**
 * @file
 * @brief Template implementation of thread pool for module multithreading
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

namespace allpix {
    template <typename Func, typename... Args> auto ThreadPool::submit(Module* module, Func&& func, Args&&... args) {
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
        task_queues_.at(module).push(std::make_unique<std::packaged_task<void()>>(std::move(task_function)));
        all_queue_.push(&task_queues_.at(module));
        return future;
    }

    template <typename T> ThreadPool::SafeQueue<T>::~SafeQueue() { invalidate(); }

    /*
     * Block until a value is available if the wait parameter is set to true. The wait exits when the queue is invalidated.
     */
    template <typename T> bool ThreadPool::SafeQueue<T>::pop(T& out, bool wait, const std::function<void()>& func) {
        // Lock the mutex
        std::unique_lock<std::mutex> lock{mutex_};
        if(wait) {
            // Wait for new item in the queue (unlocks the mutex while waiting)
            condition_.wait(lock, [this]() { return !queue_.empty() || !valid_; });
        }
        // Check for empty and valid queue
        if(queue_.empty() || !valid_) {
            return false;
        }

        // Pop the queue and optionally execute the mutex protected function
        out = std::move(queue_.front());
        queue_.pop();
        if(func != nullptr) {
            func();
        }
        return true;
    }

    template <typename T> void ThreadPool::SafeQueue<T>::push(T value) {
        std::lock_guard<std::mutex> lock{mutex_};
        queue_.push(std::move(value));
        condition_.notify_one();
    }

    template <typename T> bool ThreadPool::SafeQueue<T>::isValid() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return valid_;
    }

    template <typename T> bool ThreadPool::SafeQueue<T>::empty() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return !valid_ || queue_.empty();
    }

    /*
     * Used to ensure no conditions are being waited for in pop when a thread or the application is trying to exit. The queue
     * is invalid after calling this method and it is an error to continue using a queue after this method has been called.
     */
    template <typename T> void ThreadPool::SafeQueue<T>::invalidate() {
        std::lock_guard<std::mutex> lock{mutex_};
        std::queue<T>().swap(queue_);
        valid_ = false;
        condition_.notify_all();
    }
} // namespace allpix
