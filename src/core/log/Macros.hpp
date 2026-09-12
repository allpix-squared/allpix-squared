/**
 * @file
 * @brief Macros for logging
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LOG_MACROS_H
#define ALLPIX_LOG_MACROS_H

#include <atomic>
#include <ios>
#include <ostream>
#include <source_location>

#include "Level.hpp"
#include "LogContext.hpp"

namespace allpix {
    // NOLINTBEGIN(cppcoreguidelines-macro-usage)

    /**
     * @brief Execute a block only if the reporting level of the currently active logger is high enough
     * @param level The minimum log level
     */
#define IFLOG(level) if(::allpix::log_context::active().shouldLog(::allpix::Level::level))

    /**
     * @brief Create a logging stream if the reporting level of the currently active logger is high enough
     * @param level The log level of the stream
     *
     * Logs to the currently active logger
     */
#define LOG(level)                                                                                                          \
    if(!::allpix::log_context::active().shouldLog(::allpix::Level::level)) {                                                \
    } else /* NOLINT(readability-inconsistent-ifelse-braces) */                                                             \
        ::allpix::log_context::active().log(::allpix::Level::level, std::source_location::current())

    /**
     * @brief Create a logging stream for a progress update that overwrites the previous one in place
     *
     * @param level The log level of the stream
     * @param identifier Unused, kept for compatibility, there is only one progress slot per sink
     */
#define LOG_PROGRESS(level, identifier)                                                                                     \
    if(!::allpix::log_context::active().shouldLog(::allpix::Level::level)) {                                                \
    } else /* NOLINT(readability-inconsistent-ifelse-braces) */                                                             \
        ::allpix::log_context::active().logProgress(::allpix::Level::level, std::source_location::current())

    /**
     * @brief Create a logging stream if the reporting level is high enough and this message has not yet been logged
     * @param level The log level of the stream
     */
#define LOG_ONCE(level) LOG_N(level, 1)

    /// @{
    /**
     * @brief Macros to generate and retrieve line-specific local variables to hold the logging count of a message
     *
     * Note: the double concat macro is needed to ensure __LINE__ is evaluated, see
     * https://stackoverflow.com/a/19666216/17555746
     */
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define GENERATE_LOG_VAR(Count)                                                                                             \
    static std::atomic<int> CONCAT(local___FUNCTION__, __LINE__) { Count }
#define GET_LOG_VARIABLE() CONCAT(local___FUNCTION__, __LINE__)
    /// @}

    /**
     * @brief Create a logging stream if the reporting level is high enough and this message has not yet been logged more
     * than max_log_count times.
     * @param level The log level of the stream
     * @param max_log_count Maximum number of times this message is allowed to be logged
     */
#define LOG_N(level, max_log_count)                                                                                         \
    GENERATE_LOG_VAR(max_log_count);                                                                                        \
    if(!(GET_LOG_VARIABLE() > 0 && ::allpix::log_context::active().shouldLog(::allpix::Level::level))) {                    \
    } else /* NOLINT(readability-inconsistent-ifelse-braces) */                                                             \
        ::allpix::log_context::active().log(::allpix::Level::level, std::source_location::current())                        \
            << ((--GET_LOG_VARIABLE() == 0) ? "[further messages suppressed] " : "")
    // NOLINTEND(cppcoreguidelines-macro-usage)
} // namespace allpix

#endif /* ALLPIX_LOG_MACROS_H */
