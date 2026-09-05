/**
 * @file
 * @brief Implementation of Logger
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Logger.hpp"

#include <string_view>

#include <spdlog/spdlog.h>

using namespace allpix;

Logger::Logger(std::shared_ptr<spdlog::logger> spdlog_logger) : spdlog_logger_(std::move(spdlog_logger)) {}

Logger::~Logger() { flush(); }

void Logger::flush() {
    for(auto& sink : spdlog_logger_->sinks()) {
        sink->flush();
    }
}

void Logger::log(Level level, std::string_view message, std::source_location loc) const {
    spdlog_logger_->log(
        {loc.file_name(), static_cast<int>(loc.line()), loc.function_name()}, to_spdlog_level(level), message);
}
