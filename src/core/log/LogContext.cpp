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

#include <vector>

#include "LoggerManager.hpp"

using namespace allpix;

namespace {
    /**
     * @brief One saved (logger, stage, event number) frame, pushed by acquire() and popped by release()
     */
    struct context_frame {
        Logger* logger;
        char stage;
        uint64_t event_num;
    };

    thread_local Logger* g_active_logger = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local char g_stage = '\0';                // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local uint64_t g_event_num = 0;           // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local bool g_is_progress = false;         // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local std::vector<context_frame> g_stack; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
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
    g_stack.clear();
}

void log_context::acquire(Logger& logger, char stage, uint64_t event_num) {
    g_stack.push_back({&log_context::active(), g_stage, g_event_num});
    g_active_logger = &logger;
    g_stage = stage;
    g_event_num = event_num;
}

void log_context::release() {
    if(g_stack.empty()) {
        // Fall back to the default context
        reset();
        return;
    }
    const auto frame = g_stack.back();
    g_stack.pop_back();
    g_active_logger = frame.logger;
    g_stage = frame.stage;
    g_event_num = frame.event_num;
}
