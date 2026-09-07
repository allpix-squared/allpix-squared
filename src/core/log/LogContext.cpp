/**
 * @file
 * @brief Implementation of the thread-local logging context
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "LogContext.hpp"

#include "LoggerManager.hpp"

using namespace allpix;

namespace {
    thread_local Logger* g_active_logger = nullptr; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local char g_stage = '\0';               // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local uint64_t g_event_num = 0;          // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local bool g_is_progress = false;        // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
} // namespace

Logger& log_context::active() {
    if(g_active_logger == nullptr) {
        g_active_logger = &LoggerManager::getInstance().getDefault();
    }
    return *g_active_logger;
}

char log_context::stage() { return g_stage; }

uint64_t log_context::event_num() { return g_event_num; }

bool log_context::is_progress() { return g_is_progress; }

void log_context::set_progress(bool progress) { g_is_progress = progress; }

void log_context::reset() {
    g_active_logger = &LoggerManager::getInstance().getDefault();
    g_stage = '\0';
    g_event_num = 0;
    g_is_progress = false;
}

LogContext::LogContext(Logger& logger, char stage, uint64_t event_num)
    : prev_logger_(&log_context::active()), prev_stage_(g_stage), prev_event_(g_event_num) {
    g_active_logger = &logger;
    g_stage = stage;
    g_event_num = event_num;
}

LogContext::~LogContext() {
    g_active_logger = prev_logger_;
    g_stage = prev_stage_;
    g_event_num = prev_event_;
}
