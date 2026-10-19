/**
 * @file
 * @brief Lock-free tracking of progress, rate and estimated remaining time of a long-running process
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PROGRESS_TRACKER_H
#define ALLPIX_PROGRESS_TRACKER_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>

namespace allpix {
    /**
     * @brief Tracker of the progress of a process, providing counts, rate and estimated remaining time
     *
     * Reporting (incrementing counts) is non-blocking and a single atomic operation. Computations such as rates or remaining
     * time is performed only when reading. Rates and estimated time is calculated from a sliding window of the last 60s.
     */
    class ProgressTracker {
    public:
        using Clock = std::chrono::steady_clock;

        /* Default constructor and destructor */
        ProgressTracker() = default;
        ~ProgressTracker() = default;

        /// @{
        /* @brief Disable copying and moving */
        ProgressTracker(const ProgressTracker&) = delete;
        ProgressTracker& operator=(const ProgressTracker&) = delete;
        ProgressTracker(ProgressTracker&&) = delete;
        ProgressTracker& operator=(ProgressTracker&&) = delete;
        /// @}

        /**
         * @brief Start or restart tracking
         * @param total Total number of steps until completion, 0 if unknown (no remaining time is available)
         * @warning To be called before progress can be reported
         */
        void start(uint64_t total) {
            const std::scoped_lock lock(sample_mutex_);
            const auto now = Clock::now();
            samples_.clear();
            samples_.push_back({now, 0});
            total_.store(total, std::memory_order_relaxed);
            done_.store(0, std::memory_order_relaxed);
            end_.store(0, std::memory_order_relaxed);
            start_.store(now.time_since_epoch().count(), std::memory_order_release);
        }

        /**
         * @brief Report completed steps, lock-free
         * @param count Number of newly completed steps
         */
        void increment(uint64_t count = 1) { done_.fetch_add(count, std::memory_order_relaxed); }

        /**
         * @brief Report the absolute number of completed steps, lock-free
         * @param done Steps completed so far, ignored if lower than the currently stored count
         */
        void set(uint64_t done) {
            auto current = done_.load(std::memory_order_relaxed);
            while(current < done && !done_.compare_exchange_weak(current, done, std::memory_order_relaxed)) {
            }
        }

        /**
         * @brief Mark the process as over (finished or aborted). Freezes elapsed time and rate.
         */
        void stop() {
            Clock::rep expected = 0;
            end_.compare_exchange_strong(expected, Clock::now().time_since_epoch().count(), std::memory_order_release);
        }

        /// @{
        /** @brief Basic state, lock-free */
        uint64_t done() const { return done_.load(std::memory_order_relaxed); }
        uint64_t total() const { return total_.load(std::memory_order_relaxed); }
        bool isStarted() const { return start_.load(std::memory_order_acquire) != 0; }
        bool isStopped() const { return end_.load(std::memory_order_acquire) != 0; }
        /// Total is known and reached
        bool isComplete() const {
            const auto t = total();
            return t > 0 && done() >= t;
        }
        /// Complete or stopped
        bool isFinished() const { return isStopped() || isComplete(); }
        /// @}

        /**
         * @brief Time since \ref start, frozen by \ref stop; lock-free
         */
        Clock::duration elapsed() const {
            const auto start = start_.load(std::memory_order_acquire);
            if(start == 0) {
                return Clock::duration::zero();
            }
            return end_time() - Clock::time_point(Clock::duration(start));
        }

        /**
         * @brief Current rate in steps per second over the sliding window
         * @note Samples the counter, locks the sample mutex
         * @return Optional with the rate, or nullopt if no step has been completed in the window
         */
        std::optional<double> rate() const {
            const std::scoped_lock lock(sample_mutex_);
            return calculate_rate();
        }

        /**
         * @brief Estimated time until completion
         * @note Samples the counter, locks the sample mutex
         * @return Optional with remaining time, zero once complete, nullopt if the total is unknown or no rate is available
         */
        std::optional<Clock::duration> remaining() const {
            const auto t = total();
            if(!isStarted() || t == 0) {
                return std::nullopt;
            }
            const auto current = done();
            if(current >= t) {
                return Clock::duration::zero();
            }
            const std::scoped_lock lock(sample_mutex_);
            const auto current_rate = calculate_rate();
            if(!current_rate.has_value()) {
                return std::nullopt;
            }
            return std::chrono::duration_cast<Clock::duration>(
                std::chrono::duration<double>(static_cast<double>(t - current) / *current_rate));
        }

    private:
        struct sample {
            Clock::time_point time;
            uint64_t done;
        };

        Clock::time_point end_time() const {
            const auto end = end_.load(std::memory_order_acquire);
            return (end != 0) ? Clock::time_point(Clock::duration(end)) : Clock::now();
        }

        /**
         * @brief Take a new sample if due, clear old ones, and compute the rate over it
         * @note Sample mutex must be held by calling function
         * @return Optional with calculated rate or nullopt if cannot be calculated (yet)
         */
        std::optional<double> calculate_rate() const {
            if(!isStarted() || samples_.empty()) {
                return std::nullopt;
            }
            const auto now = end_time();
            const auto current = done();
            if(!isStopped() && now - samples_.back().time >= SAMPLE_INTERVAL) {
                samples_.push_back({now, current});
            }
            // Drop samples outside of the window but keep one anchor at or before its begin
            while(samples_.size() > 1 && now - samples_[1].time >= WINDOW) {
                samples_.pop_front();
            }

            const auto& anchor = samples_.front();
            const auto span = std::chrono::duration<double>(now - anchor.time).count();
            if(current <= anchor.done || span <= 0) {
                return std::nullopt;
            }
            return static_cast<double>(current - anchor.done) / span;
        }

        static constexpr auto SAMPLE_INTERVAL = std::chrono::seconds(1);
        static constexpr auto WINDOW = std::chrono::seconds(60);

        // Progress counters
        std::atomic<uint64_t> done_{0};
        std::atomic<uint64_t> total_{0};
        std::atomic<Clock::rep> start_{0};
        std::atomic<Clock::rep> end_{0};

        // Mutex only ever locked by reading methods
        mutable std::mutex sample_mutex_;
        mutable std::deque<sample> samples_;
    };
} // namespace allpix

#endif /* ALLPIX_PROGRESS_TRACKER_H */
