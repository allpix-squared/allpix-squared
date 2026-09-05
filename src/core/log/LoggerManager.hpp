/**
 * @file
 * @brief Central logger registry
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LOG_MANAGER_H
#define ALLPIX_LOG_MANAGER_H

#include <atomic>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <spdlog/sinks/sink.h>

#include "core/utils/enum.h"

#include "Level.hpp"
#include "Logger.hpp"
#include "Sinks.hpp"

namespace allpix {
    /**
     * @brief Central registry creating and owning all \ref Logger instances
     *
     * Every named "topic" (a module's unique name, "Geant4", the empty framework topic, or any name chosen, ...)
     * maps to a permanent \ref Logger, created on first request and kept alive until the end. All loggers created through
     * this manager share the same list of sinks, any sink added will be attached to all loggers.
     *
     * The \ref Level is a per-logger setting while formatting is configured globally and applies uniformly to all output
     * going through a given sink.
     */
    class LoggerManager {
    public:
        /**
         * @brief Access the single, global instance of the logger registry
         * @return Reference to the logger registry
         */
        static LoggerManager& getInstance();

        /// @{
        /* @brief Disable copying and moving */
        LoggerManager(const LoggerManager&) = delete;
        LoggerManager& operator=(const LoggerManager&) = delete;
        LoggerManager(LoggerManager&&) = delete;
        LoggerManager& operator=(LoggerManager&&) = delete;
        /// @}

        /**
         * @brief Get or create the logger for a given topic, with an explicit initial level
         * @param topic Unique name of the logger (e.g. a module unique name)
         * @param level Initial level to use if the logger does not yet exist (ignored otherwise)
         * @return Reference to the (new or pre-existing) logger
         */
        Logger& getLogger(const std::string& topic, Level level);

        /**
         * @brief Get or create the logger for a given topic, using the current global default level
         * @param topic Unique name of the logger
         * @return Reference to the (new or pre-existing) logger
         */
        Logger& getLogger(const std::string& topic);

        /**
         * @brief Get the shared core/framework logger (topic "")
         * @return Reference to the core logger
         */
        Logger& getDefault();

        /**
         * @brief Register a new sink, attaching it to existing loggers as well as all future loggers
         *
         * @param sink Sink to register
         */
        void addSink(const spdlog::sink_ptr& sink);

        /**
         * @brief Convenience wrapper around \ref addSink wrapping a plain output stream, formatted with the global
         *        \ref Format
         * @param stream Stream to write log output to; must remain valid for as long as logging may occur
         * @param style Style of the sink, plain text or colored output
         */
        void addStream(std::ostream& stream, SinkStyle style);

        /**
         * @brief Set the level used for the core logger and as the default for loggers created from now on
         * @param level New global default level
         */
        void setGlobalLevel(Level level);

        /**
         * @brief Set the format used by every sink registered via \ref addStream
         * @param format New global format
         */
        void setGlobalFormat(Format format);

        /**
         * @brief Check if \ref setGlobalLevel has already been called (e.g. from a command line override)
         * @return True if an explicit global level has been set
         */
        bool hasExplicitGlobalLevel() const { return global_level_explicit_.load(std::memory_order_relaxed); }

        /**
         * @brief Flush and detach all sinks from all loggers
         * @warning No other log message should be sent after this method is called
         * @note Does not destroy the loggers themselves, only their sinks
         */
        void finish();

    private:
        LoggerManager() = default;
        ~LoggerManager() = default;

        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::shared_ptr<Logger>> loggers_;
        std::vector<spdlog::sink_ptr> sinks_;
        std::vector<std::shared_ptr<StreamSink>> stream_sinks_;
        std::atomic<Level> global_level_{Level::WARNING};
        std::atomic<Format> global_format_{Format::DEFAULT};
        std::atomic<bool> global_level_explicit_{false};
    };

    /**
     * @brief Static helper functions.
     */
    class Log {
    public:
        static void finish() { LoggerManager::getInstance().finish(); }
        static Level getLevelFromString(const std::string& level);
        static std::string getStringFromLevel(Level level) { return enum_name(level); }
        static Format getFormatFromString(const std::string& format);
        static std::string getStringFromFormat(Format format) { return enum_name(format); }
    };
} // namespace allpix

#endif /* ALLPIX_LOG_MANAGER_H */
