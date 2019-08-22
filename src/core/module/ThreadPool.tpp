namespace allpix {
    template <typename T> ThreadPool::SafeQueue<T>::SafeQueue(unsigned int max_size) : max_size_(max_size) {}

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

        // Notify possible producers waiting to fill the queue
        condition_.notify_all();

        return true;
    }

    template <typename T> void ThreadPool::SafeQueue<T>::push(T value) {
        std::unique_lock<std::mutex> lock{mutex_};

        // Check if the queue reached its full size
        if(queue_.size() >= max_size_) {
            // Wait until the queue is below the max size or it was invalidated(shutdown)
            condition_.wait(lock, [this]() { return queue_.size() < max_size_ || !valid_; });
        }

        // Abort the push operation if conditions not met
        if(queue_.size() >= max_size_ || !valid_) {
            return;
        }

        // Push a new element to the queue and notify consumers
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

    template <typename T> size_t ThreadPool::SafeQueue<T>::size() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return queue_.size();
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
