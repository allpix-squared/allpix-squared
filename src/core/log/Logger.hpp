/**
 * @file
 * @brief Logger wrapping a spdlog logger and providing possibility to log via streams
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LOG_LOGGER_H
#define ALLPIX_LOG_LOGGER_H

#include <atomic>
#include <exception>
#include <memory>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>

#include <spdlog/logger.h>
#include <spdlog/sinks/sink.h>

#include "Level.hpp"

namespace allpix {
    /**
     * @brief Logger of the framework to inform the user of process
     *
     * This class implements a wrapper around the spdlog logger and provides additional features such as the possibility
     * to perform logging using streams (with << syntax rather than enclosing the log message in parentheses).
     * Should usually not be instantiated directly. The \ref LOG macro should be used instead to pass all the information.
     */
    class Logger {
    public:
        /**
         * @brief Log stream that executes logging upon its destruction
         */
        class LogStream final : public std::ostringstream {
        public:
            LogStream(const Logger& logger, Level level, std::source_location loc, bool progress = false)
                : logger_(logger), level_(level), loc_(loc), progress_(progress) {}
            ~LogStream() final { logger_.log(level_, this->view(), loc_, progress_); } // NOLINT(bugprone-exception-escape)

            /// @{
            /* @brief Disable copying and moving */
            LogStream(const LogStream&) = delete;
            LogStream& operator=(const LogStream&) = delete;
            LogStream(LogStream&&) = delete;
            LogStream& operator=(LogStream&&) = delete;
            /// @}

        private:
            const Logger& logger_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
            Level level_;
            std::source_location loc_;
            bool progress_;
        };

        /**
         * @brief Construct a logger
         * @param spdlog_logger Underlying spdlog logger
         * @param level Initial, permanent logging level of this logger
         */
        Logger(std::shared_ptr<spdlog::logger> spdlog_logger);

        /// @{
        /* @brief Disable copying and moving */
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;
        /// @}

        ~Logger();

        /**
         * @brief Check if a message at the given level should be logged given the currently configured log level
         *
         * @param level Log level to be tested against the logger configuration
         * @return Boolean indicating if the message should be logged
         */
        bool shouldLog(Level level) const { return spdlog_logger_->should_log(to_spdlog_level(level)); }

        /**
         * @brief Get the current logging level of this logger
         * @return Current log level
         */
        Level getLevel() const { return from_spdlog_level(spdlog_logger_->level()); }

        /**
         * @brief Set a new logging level for this logger
         * @param level The new log level
         */
        void setLevel(Level level) { spdlog_logger_->set_level(to_spdlog_level(level)); }

        /**
         * @brief Get the topic (unique name) of this logger
         * @return Logger topic
         */
        const std::string& getTopic() const { return spdlog_logger_->name(); }

        /**
         * @brief Get the underlying spdlog logger, e.g. to attach additional per-logger sinks
         * @return Shared pointer to the underlying spdlog logger
         */
        const std::shared_ptr<spdlog::logger>& getBackend() const { return spdlog_logger_; }

        /**
         * @brief Gives a stream to write to using the C++ stream syntax
         * @param level Logging level of the message
         * @param loc Source code location the log message originates from
         * @return A \ref LogStream to write to
         */
        LogStream log(Level level, std::source_location loc = std::source_location::current()) const {
            return {*this, level, loc};
        }

        /**
         * @brief Gives a stream to write a progress update to
         * @details Behaves exactly like \ref log, except the message is tagged as a progress update. Some sinks will render
         * this on a single line, overwriting previous messages tagged as progress.
         *
         * @param level Logging level of the message
         * @param loc Source code location the log message originates from
         * @return A \ref LogStream to write to
         */
        LogStream logProgress(Level level, std::source_location loc = std::source_location::current()) const {
            return {*this, level, loc, true};
        }

        /**
         * @brief Log a message
         *
         * @param level Level of the log message
         * @param message Log message
         * @param src_loc Source code location from which the log message emitted
         * @param progress Whether this message is a progress update
         */
        void log(Level level,
                 std::string_view message,
                 std::source_location src_loc = std::source_location::current(),
                 bool progress = false) const;

        /**
         * @brief Flush all sinks attached to this logger
         */
        void flush();

    private:
        std::shared_ptr<spdlog::logger> spdlog_logger_;
    };
} // namespace allpix

#endif /* ALLPIX_LOG_LOGGER_H */
