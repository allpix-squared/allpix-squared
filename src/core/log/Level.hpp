/**
 * @file
 * @brief Log levels
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LOG_LEVEL_H
#define ALLPIX_LOG_LEVEL_H

#include <stdexcept>
#include <string>
#include <string_view>

#include <spdlog/common.h>

#include "core/utils/enum.h"

namespace allpix {
    /**
     * @brief Logging detail level
     *
     * The numerical values are chosen to allow a direct cast to \ref spdlog::level::level_enum.
     */
    enum class Level : int { // NOLINT(performance-enum-size)
        PRNG = -1,           ///< Logging level printing every pseudo-random number requested (more verbose than TRACE)
        TRACE = 0,           ///< Software debugging information about what part is currently running
        DEBUG = 1,           ///< Detailed information about physics process
        INFO = 2,            ///< General information about processes (should not be called in run function)
        WARNING = 3,         ///< Possible issue that could lead to unexpected results
        STATUS = 4,          ///< Only critical progress information; stays visible up to and including the WARNING threshold
        ERROR = 5,           ///< Critical problems that usually lead to termination of the framework
        OFF = 6,             ///< Disables logging
    };
    using enum Level;

    /**
     * @brief Convert an Allpix verbosity level to the corresponding spdlog level
     * @param level Allpix verbosity level
     * @return spdlog level value
     */
    constexpr spdlog::level::level_enum to_spdlog_level(Level level) {
        return level == Level::PRNG ? spdlog::level::trace : static_cast<spdlog::level::level_enum>(level);
    }

    /**
     * @brief Format of the logger
     */
    enum class Format : int {
        SHORT = 0,   ///< Only include a single character for the log level, the section header and the message
        DEFAULT = 1, ///< Also include the time and a full logging level description
        LONG = 2,    ///< All of the above and also information about the file and line where the message was defined
    };
    using enum Format;

} // namespace allpix

#endif /* ALLPIX_LOG_LEVEL_H */
