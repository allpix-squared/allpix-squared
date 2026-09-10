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
         * @return Reference to the active logger, returns the framework logger unless a context is currently acquired
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
         * @brief Reset the context on this thread back to its defaults (framework logger, no stage, no event number),
         *        discarding any outstanding acquire() calls
         */
        void reset();

        /**
         * @brief Check whether the message currently being dispatched on this thread is a progress update
         *
         * Only meaningful synchronously, while \ref Logger::log is dispatching into spdlog: this is how a color-styled
         * \ref StreamSink tells a \ref LOG_PROGRESS update apart from a regular log message, without needing any extra
         * field on spdlog's own `log_msg`. See \ref Logger::logProgress.
         */
        bool is_progress();

        /**
         * @brief Set whether the message about to be dispatched on this thread is a progress update
         * @note Internal: called only by \ref Logger::log immediately before dispatching to spdlog
         */
        void set_progress(bool progress);

        /**
         * @brief Acquire a new logging context on this thread, enabling the provided logger
         * @details Pushes the current context (active logger, stage, event number) aside and installs the new one. Every
         * call to acquire() must be matched by exactly one later call to \ref release() on the same thread, in LIFO order.
         *
         * @param logger Logger to make active
         * @param stage Lifecycle-stage marker to display, or '\0' for none
         * @param event_num Event number to display, or 0 for none
         */
        void acquire(Logger& logger, char stage = '\0', uint64_t event_num = 0);

        /**
         * @brief Release the most recently acquired logging context, restoring the one before it
         * @note Must be matched with a prior \ref acquire() call on the same thread. Calling this without a
         *       matching acquire() resets to the default context.
         */
        void release();
    } // namespace log_context
} // namespace allpix

#endif /* ALLPIX_LOG_CONTEXT_H */
