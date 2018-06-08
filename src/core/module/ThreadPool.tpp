namespace allpix {
    template <typename T> ThreadPool::SafeQueue<T>::~SafeQueue() { invalidate(); }

    /*
     * Block until a value is available if the wait parameter is set to true. The wait exits when the queue is invalidated.
     */
    template <typename T> bool ThreadPool::SafeQueue<T>::pop(T& out, bool wait, const std::function<void()>& func) {
        // Lock the mutex
        std::unique_lock<std::mutex> lock{mutex_};
        if(queue_.empty() && wait) {
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
