/**
 * @file
 * @brief Statistics of a simulation: stage times, event counters and performance histograms
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_RUN_STATISTICS_H
#define ALLPIX_RUN_STATISTICS_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

#include <TH1D.h>

#include "core/histograms/HistogramManager.hpp"
#include "core/utils/progress_tracker.h"

namespace allpix {
    class Module;

    /*
     * @brief Statistics of the event loop: event counters, times, module execution times and performance histograms
     *
     * Owned by the \ref ModuleManager, worker threads update event counters, per-module execution times, and histograms, all
     * as lock-free operations. Getter functions can be called from any thread at any time and may lock requested resources.
     */
    class RunStatistics {
        class StageTimer;

    public:
        /* Default constructor */
        RunStatistics() = default;

        /// @{
        /* @brief Disable copying and moving */
        RunStatistics(const RunStatistics&) = delete;
        RunStatistics& operator=(const RunStatistics&) = delete;
        RunStatistics(RunStatistics&&) = delete;
        RunStatistics& operator=(RunStatistics&&) = delete;
        /// @}

        /**
         * @brief Currently active stage of the simulation run, stage is ended when this object is destructed
         */
        class Stage {
        public:
            ~Stage();

            /// @{
            /* @brief Disable copying and moving */
            Stage(const Stage&) = delete;
            Stage& operator=(const Stage&) = delete;
            Stage(Stage&&) = delete;
            Stage& operator=(Stage&&) = delete;
            /// @}

        private:
            friend class RunStatistics;
            Stage(StageTimer& timer, ProgressTracker* progress);

            StageTimer& timer_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
            ProgressTracker* progress_;
        };

        /**
         * @name Stages
         * Called by the \ref ModuleManager from the main thread at the begin of the respective stages. The object is held
         * for the duration of the function. Calling a function again restarts the timing of its stage.
         * @return Guard ending the stage on destruction
         */
        ///@{
        /** @brief Start the initialization stage */
        [[nodiscard]] Stage initialize();

        /**
         * @brief Start the run stage, i.e. the event loop, and reset all event counters
         * @param total_events Number of events to process
         * @warning To be called before the worker threads are started
         */
        [[nodiscard]] Stage run(uint64_t total_events);

        /** @brief Start the finalization stage */
        [[nodiscard]] Stage finalize();
        ///@}

        /**
         * @name Event counters
         * Lock-free, to be called from any number of worker threads
         */
        ///@{
        /** @brief Report successfully finished event, counts towards the progress of the run */
        void eventFinished(uint64_t count = 1) {
            finished_.fetch_add(count, std::memory_order_relaxed);
            progress_->increment();
        }
        /** @brief Report aborted event, counts towards the progress of the run but not as finished */
        void eventAborted(uint64_t count = 1) {
            aborted_.fetch_add(count, std::memory_order_relaxed);
            progress_->increment();
        }
        /** @brief Report the current number of buffered events */
        void setBufferedEvents(uint64_t buffered) { buffered_.store(buffered, std::memory_order_relaxed); }

        uint64_t getTotalEvents() const { return progress_->total(); }
        uint64_t getFinishedEvents() const { return finished_.load(std::memory_order_relaxed); }
        uint64_t getBufferedEvents() const { return buffered_.load(std::memory_order_relaxed); }
        uint64_t getAbortedEvents() const { return aborted_.load(std::memory_order_relaxed); }

        /**
         * @brief Retrieve all event counts at once
         * @return Tuple with the number of total, finished, buffered and aborted events, in this order
         */
        std::tuple<uint64_t, uint64_t, uint64_t, uint64_t> getEventCounts() const {
            return {getTotalEvents(), getFinishedEvents(), getBufferedEvents(), getAbortedEvents()};
        }
        ///@}

        /**
         * @name Times of the stages
         * Running while the stage is active and frozen once it ended, zero before it started
         */
        ///@{
        /** @brief Time spent in the initialization stage */
        std::chrono::nanoseconds getInitializeTime() const { return initialize_timer_.elapsed(); }

        /** @brief Time spent in the finalization stage */
        std::chrono::nanoseconds getFinalizeTime() const { return finalize_timer_.elapsed(); }

        /**
         * @brief Time spent in all stages together, not including the time between stages
         */
        std::chrono::nanoseconds getTotalTime() const { return getInitializeTime() + getElapsedTime() + getFinalizeTime(); }
        ///@}

        /**
         * @name Times and rates of the event loop (run stage)
         */
        ///@{
        /**
         * @brief Wall clock time at which the event loop started, or nothing if it has not started
         */
        std::optional<std::chrono::system_clock::time_point> getStartTime() const { return run_timer_.wallStart(); }

        /**
         * @brief Wall clock time at which the event loop ended, or nothing while it has not ended
         */
        std::optional<std::chrono::system_clock::time_point> getEndTime() const { return run_timer_.wallEnd(); }

        /**
         * @brief Expected end time of the event loop
         * @return The actual end time once ended, else now plus the remaining time, or nothing if no estimate is available
         */
        std::optional<std::chrono::system_clock::time_point> getEstimatedEndTime() const;

        /**
         * @brief Time elapsed in the event loop, running while the loop runs and frozen once it ended; zero before
         */
        std::chrono::nanoseconds getElapsedTime() const { return run_timer_.elapsed(); }

        /**
         * @brief Estimated time until the event loop is finished
         * @details Calculated from the processed (finished and aborted) events over a sliding window of the last seconds,
         * independently of whether progress bars or logging are enabled. The estimate grows while no event is processed.
         * @return Remaining time, zero when finished, or nothing if no estimate is available (yet)
         */
        std::optional<std::chrono::nanoseconds> getRemainingTime() const;

        /**
         * @brief Current event rate over the sliding window of the last seconds
         * @return Processed events per second, or nothing if no event was processed in the window
         */
        std::optional<double> getEventRate() const { return progress_->rate(); }

        /**
         * @brief Event rate averaged over the whole event loop so far
         * @return Processed events per second, or nothing if no event was processed
         */
        std::optional<double> getAverageEventRate() const;

        /**
         * @brief Processing time per event averaged over the whole event loop so far (all workers combined)
         * @return Average wall clock time per processed event, or nothing if no event was processed
         */
        std::optional<std::chrono::nanoseconds> getAverageEventTime() const;
        ///@}

        /**
         * @name Per-module execution times
         * Accumulated over construction, initialization, events (all threads summed) and finalization
         */
        ///@{
        /**
         * @brief Add execution time of a module
         * @param module Module that has been executing
         * @param duration Time spent
         * @note Lock-free for a module that has been added before; adding a new module is only possible outside of the run
         */
        void addModuleTime(const Module* module, std::chrono::nanoseconds duration);

        /**
         * @brief Drop a module, e.g. because it has been replaced by an instance with higher priority
         */
        void removeModule(const Module* module);

        /**
         * @brief Accumulated execution time of a module, zero for unknown modules
         */
        std::chrono::nanoseconds getModuleTime(const Module* module) const;

        /**
         * @brief Sum of the execution times of all modules
         */
        std::chrono::nanoseconds getTotalModuleTime() const;

        /**
         * @brief The module with the largest accumulated execution time
         * @return Module and its time, or a null module and zero time if there are no modules
         */
        std::pair<const Module*, std::chrono::nanoseconds> getSlowestModule() const;
        ///@}

        /**
         * @name Performance histograms
         * Only booked if enabled, all fill methods do nothing otherwise
         */
        ///@{
        /**
         * @brief Book the global performance histograms (buffer fill level, event processing time)
         * @param histogram_manager Manager to register the histograms with
         * @param max_buffer_size Maximum size of the event buffer, defines the range of the buffer histogram
         */
        void enablePerformancePlots(HistogramManager& histogram_manager, size_t max_buffer_size);

        /**
         * @brief Book the event processing time histogram of a module
         * @param module Module to book the histogram for
         * @param histogram_manager Manager to register the histogram with
         * @param path Path of the histogram in the output file
         * @param name Name of the histogram
         * @param title Title of the histogram
         */
        void bookModuleHistogram(const Module* module,
                                 HistogramManager& histogram_manager,
                                 const std::string& path,
                                 const std::string& name,
                                 const std::string& title);

        /** @brief Record the current buffer fill level */
        void fillBufferLevel() const;
        /** @brief Record the total processing time of one event, summed over its modules */
        void fillEventTime(std::chrono::nanoseconds duration) const;
        /** @brief Record the processing time of one event in a module */
        void fillModuleEventTime(const Module* module, std::chrono::nanoseconds duration) const;
        ///@}

        /**
         * @brief The tracker calculating event rate and remaining time, shareable with consumers displaying the progress
         */
        const std::shared_ptr<ProgressTracker>& getProgress() const { return progress_; }

    private:
        /**
         * @brief Timing of a single stage: monotonic clock for durations, system clock for the start point in time
         * @details Written by the main thread only, readable from any thread. Zero means processed (yet).
         */
        class StageTimer {
        public:
            void start();
            void stop();

            bool isStarted() const { return start_.load(std::memory_order_acquire) != 0; }
            bool isStopped() const { return end_.load(std::memory_order_acquire) != 0; }
            bool isActive() const { return isStarted() && !isStopped(); }

            std::chrono::nanoseconds elapsed() const;
            std::optional<std::chrono::system_clock::time_point> wallStart() const;
            std::optional<std::chrono::system_clock::time_point> wallEnd() const;

        private:
            std::atomic<std::chrono::steady_clock::rep> start_{0};
            std::atomic<std::chrono::steady_clock::rep> end_{0};
            std::atomic<std::chrono::system_clock::rep> wall_start_{0};
        };

        bool isRunning() const { return run_timer_.isActive(); }

        std::shared_ptr<ProgressTracker> progress_{std::make_shared<ProgressTracker>()};
        std::atomic<uint64_t> buffered_{0};
        std::atomic<uint64_t> finished_{0};
        std::atomic<uint64_t> aborted_{0};

        StageTimer initialize_timer_;
        StageTimer run_timer_;
        StageTimer finalize_timer_;

        // The map is not altered while the run is active, the values are atomic. durations in ns
        std::map<const Module*, std::atomic<int64_t>> module_time_;

        Histogram<TH1D> event_time_;
        Histogram<TH1D> buffer_fill_level_;
        std::map<const Module*, Histogram<TH1D>> module_event_time_;
    };
} // namespace allpix

#endif /* ALLPIX_RUN_STATISTICS_H */
