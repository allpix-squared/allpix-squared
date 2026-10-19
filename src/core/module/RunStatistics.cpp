/**
 * @file
 * @brief Implementation of the run statistics
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "RunStatistics.hpp"

#include <algorithm>

using namespace allpix;

void RunStatistics::StageTimer::start() {
    end_.store(0, std::memory_order_relaxed);
    wall_start_.store(std::chrono::system_clock::now().time_since_epoch().count(), std::memory_order_relaxed);
    start_.store(std::chrono::steady_clock::now().time_since_epoch().count(), std::memory_order_release);
}

void RunStatistics::StageTimer::stop() {
    if(isActive()) {
        end_.store(std::chrono::steady_clock::now().time_since_epoch().count(), std::memory_order_release);
    }
}

std::chrono::nanoseconds RunStatistics::StageTimer::elapsed() const {
    const auto start = start_.load(std::memory_order_acquire);
    if(start == 0) {
        return std::chrono::nanoseconds::zero();
    }
    const auto end = end_.load(std::memory_order_acquire);
    const auto end_time = (end != 0) ? std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration(end))
                                     : std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration(start)));
}

std::optional<std::chrono::system_clock::time_point> RunStatistics::StageTimer::wallStart() const {
    if(!isStarted()) {
        return std::nullopt;
    }
    return std::chrono::system_clock::time_point(
        std::chrono::system_clock::duration(wall_start_.load(std::memory_order_relaxed)));
}

std::optional<std::chrono::system_clock::time_point> RunStatistics::StageTimer::wallEnd() const {
    const auto start = wallStart();
    if(!start.has_value() || !isStopped()) {
        return std::nullopt;
    }
    return *start + std::chrono::duration_cast<std::chrono::system_clock::duration>(elapsed());
}

RunStatistics::Stage::Stage(StageTimer& timer, ProgressTracker* progress) : timer_(timer), progress_(progress) {
    timer_.start();
}

RunStatistics::Stage::~Stage() {
    if(progress_ != nullptr) {
        progress_->stop();
    }
    timer_.stop();
}

RunStatistics::Stage RunStatistics::initialize() { return {initialize_timer_, nullptr}; }

RunStatistics::Stage RunStatistics::run(uint64_t total_events) {
    finished_.store(0, std::memory_order_relaxed);
    aborted_.store(0, std::memory_order_relaxed);
    buffered_.store(0, std::memory_order_relaxed);
    progress_->start(total_events);
    return {run_timer_, progress_.get()};
}

RunStatistics::Stage RunStatistics::finalize() { return {finalize_timer_, nullptr}; }

std::optional<std::chrono::system_clock::time_point> RunStatistics::getEstimatedEndTime() const {
    if(const auto end = getEndTime()) {
        return end;
    }
    const auto remaining = getRemainingTime();
    if(!remaining.has_value()) {
        return std::nullopt;
    }
    return std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(*remaining);
}

std::optional<std::chrono::nanoseconds> RunStatistics::getRemainingTime() const {
    const auto remaining = progress_->remaining();
    if(!remaining.has_value()) {
        return std::nullopt;
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(*remaining);
}

std::optional<double> RunStatistics::getAverageEventRate() const {
    const auto finished = progress_->done();
    const auto elapsed = std::chrono::duration<double>(getElapsedTime()).count();
    if(finished == 0 || elapsed <= 0) {
        return std::nullopt;
    }
    return static_cast<double>(finished) / elapsed;
}

std::optional<std::chrono::nanoseconds> RunStatistics::getAverageEventTime() const {
    const auto finished = progress_->done();
    if(finished == 0) {
        return std::nullopt;
    }
    return getElapsedTime() / static_cast<std::chrono::nanoseconds::rep>(finished);
}

void RunStatistics::addModuleTime(const Module* module, std::chrono::nanoseconds duration) {
    auto it = module_time_.find(module);
    if(it == module_time_.end()) {
        // Adding entries is not thread-safe, only possible while no worker threads are running
        if(run_timer_.isActive()) {
            return;
        }
        it = module_time_.try_emplace(module, 0).first;
    }
    it->second.fetch_add(duration.count(), std::memory_order_relaxed);
}

void RunStatistics::removeModule(const Module* module) {
    module_time_.erase(module);
    module_event_time_.erase(module);
}

std::chrono::nanoseconds RunStatistics::getModuleTime(const Module* module) const {
    const auto it = module_time_.find(module);
    return (it == module_time_.end()) ? std::chrono::nanoseconds::zero()
                                      : std::chrono::nanoseconds(it->second.load(std::memory_order_relaxed));
}

std::chrono::nanoseconds RunStatistics::getTotalModuleTime() const {
    auto total = std::chrono::nanoseconds::zero();
    for(const auto& [module, time] : module_time_) {
        total += std::chrono::nanoseconds(time.load(std::memory_order_relaxed));
    }
    return total;
}

std::pair<const Module*, std::chrono::nanoseconds> RunStatistics::getSlowestModule() const {
    std::pair<const Module*, std::chrono::nanoseconds> slowest{nullptr, std::chrono::nanoseconds::zero()};
    for(const auto& [module, time] : module_time_) {
        const std::chrono::nanoseconds module_time(time.load(std::memory_order_relaxed));
        if(module_time > slowest.second) {
            slowest = {module, module_time};
        }
    }
    return slowest;
}

void RunStatistics::enablePerformancePlots(HistogramManager& histogram_manager, size_t max_buffer_size) {
    buffer_fill_level_ = histogram_manager.registerHistogram<TH1D>("Performance",
                                                                   "buffer_fill_level",
                                                                   "Buffer fill level;# buffered events;# events",
                                                                   static_cast<int>(max_buffer_size),
                                                                   0,
                                                                   static_cast<double>(max_buffer_size));
    event_time_ = histogram_manager.registerHistogram<TH1D>(
        "Performance", "event_time", "processing time per event;time [s];# events", 1000, 0, 10);
}

void RunStatistics::bookModuleHistogram(const Module* module,
                                        HistogramManager& histogram_manager,
                                        const std::string& path,
                                        const std::string& name,
                                        const std::string& title) {
    module_event_time_[module] = histogram_manager.registerHistogram<TH1D>(path, name.c_str(), title.c_str(), 1000, 0, 1);
}

void RunStatistics::fillBufferLevel() const {
    if(buffer_fill_level_) {
        buffer_fill_level_->Fill(static_cast<double>(getBufferedEvents()));
    }
}

void RunStatistics::fillEventTime(std::chrono::nanoseconds duration) const {
    if(event_time_) {
        event_time_->Fill(std::chrono::duration<double>(duration).count());
    }
}

void RunStatistics::fillModuleEventTime(const Module* module, std::chrono::nanoseconds duration) const {
    const auto it = module_event_time_.find(module);
    if(it != module_event_time_.end() && it->second) {
        it->second->Fill(std::chrono::duration<double>(duration).count());
    }
}
