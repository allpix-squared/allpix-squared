/**
 * @file
 * @brief Implementation of the central logger registry
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "LoggerManager.hpp"

#include <memory>
#include <mutex>
#include <utility>

#include <spdlog/logger.h>

#include "Sinks.hpp"
#include "core/utils/enum.h"

using namespace allpix;

LoggerManager& LoggerManager::getInstance() {
    static LoggerManager instance;
    return instance;
}

Logger& LoggerManager::getLogger(const std::string& topic, Level level) {
    std::scoped_lock const lock(mutex_);
    auto it = loggers_.find(topic);
    if(it != loggers_.end()) {
        // Logger already exists: keeps its level permanently, requested level is ignored
        return *it->second;
    }

    auto backend = std::make_shared<spdlog::logger>(topic, sinks_.begin(), sinks_.end());
    backend->set_level(to_spdlog_level(level));
    // FIXME necessary?
    // backend->flush_on(spdlog::level::trace);

    auto [inserted, _] = loggers_.emplace(topic, std::make_shared<Logger>(std::move(backend)));
    return *inserted->second;
}

Logger& LoggerManager::getLogger(const std::string& topic) {
    return getLogger(topic, global_level_.load(std::memory_order_relaxed));
}

Logger& LoggerManager::getDefault() { return getLogger(""); }

void LoggerManager::addSink(const spdlog::sink_ptr& sink) {
    std::scoped_lock const lock(mutex_);
    sinks_.push_back(sink);
    for(auto& [topic, logger] : loggers_) {
        logger->getBackend()->sinks().push_back(sink);
    }
}

void LoggerManager::addStream(std::ostream& stream) {
    auto sink = std::make_shared<StreamSink>(stream, global_format_.load(std::memory_order_relaxed));
    {
        std::scoped_lock const lock(mutex_);
        stream_sinks_.push_back(sink);
    }
    addSink(sink);
}

void LoggerManager::setGlobalLevel(Level level) {
    global_level_.store(level, std::memory_order_relaxed);
    global_level_explicit_.store(true, std::memory_order_relaxed);

    std::scoped_lock const lock(mutex_);
    auto it = loggers_.find("");
    if(it != loggers_.end()) {
        it->second->setLevel(level);
    }
}

void LoggerManager::setGlobalFormat(Format format) {
    global_format_.store(format, std::memory_order_relaxed);

    std::scoped_lock const lock(mutex_);
    for(auto& sink : stream_sinks_) {
        sink->setFormat(format);
    }
}

Level Log::getLevelFromString(const std::string& level_str) {
    const auto level = enum_cast<Level>(level_str);
    if(!level.has_value()) {
        throw std::invalid_argument("unknown log level");
    }
    return level.value();
}

Format Log::getFormatFromString(const std::string& format_str) {
    const auto format = enum_cast<Format>(format_str);
    if(!format.has_value()) {
        throw std::invalid_argument("unknown format");
    }
    return format.value();
}

void LoggerManager::finish() {
    std::scoped_lock const lock(mutex_);
    for(auto& sink : sinks_) {
        sink->flush();
    }
    // Dropping the last reference to a terminal StreamSink restores the cursor (see ~StreamSink)
    for(auto& [topic, logger] : loggers_) {
        logger->getBackend()->sinks().clear();
    }
    sinks_.clear();
    stream_sinks_.clear();
}
