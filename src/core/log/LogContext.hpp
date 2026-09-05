/**
 * @file
 * @brief Thread-local logging context used to resolve LOG macros
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LOG_CONTEXT_H
#define ALLPIX_LOG_CONTEXT_H

#include <cstdint>

#include "Logger.hpp"

namespace allpix {
    /**
     * @brief Thread-local logging context for modules
     *
     * The \ref LOG macro always logs through the current active \ref Logger on the calling thread. Outside of modules this
     * is the framework logger; inside module code it is that module's logger. This context also stores the lifecycle stage
     * and current event number.
     */
    namespace log_context {
        /**
         * @brief Get the currently active logger for this thread
         * @return Reference to the active logger, returns the framework logger unless a \ref LogContext is active
         */
        Logger& active();

        /**
         * @brief Get the current lifecycle-stage marker for this thread
         * @return Either 'C' (construction), 'I' (initialize), 'T' (thread init/finalize), 'R' (run) or 'F' (finalize).
         *         Returns a null character '\0' if no stage is set
         */
        char stage();

        /**
         * @brief Get the current event number for this thread
         * @return Event number, or 0 if no event is currently being processed
         */
        uint64_t event_num();

        /**
         * @brief Reset the context on this thread back to its defaults (framework logger, no stage, no event number)
         */
        void reset();
    } // namespace log_context

    /**
     * @brief Log context serving as RAII guard that enables a logger and sets stage/event number for its scope
     *
     * Used by the \ref ModuleManager and \ref ThreadPool around calls into module code. Restores the previous context on
     * destruction, including when the scope is left via an exception.
     */
    class LogContext {
    public:
        /**
         * @brief Construct the guard, making the given logger active for the remainder of the scope
         * @param logger Logger to make active
         * @param stage Lifecycle-stage marker to display
         * @param event_num Event number to display
         */
        explicit LogContext(Logger& logger, char stage = '\0', uint64_t event_num = 0);
        ~LogContext();

        /// @{
        /**
         * @brief Disable copying and moving
         */
        LogContext(const LogContext&) = delete;
        LogContext& operator=(const LogContext&) = delete;
        LogContext(LogContext&&) = delete;
        LogContext& operator=(LogContext&&) = delete;
        /// @}

    private:
        Logger* prev_logger_;
        char prev_stage_;
        uint64_t prev_event_;
    };
} // namespace allpix

#endif /* ALLPIX_LOG_CONTEXT_H */
