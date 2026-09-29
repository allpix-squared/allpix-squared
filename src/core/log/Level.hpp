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

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

#include <spdlog/common.h>

#include "core/utils/enum.h"

namespace allpix {
    /**
     * @brief Logging detail level
     */
    enum class Level : std::uint8_t {
        PRNG = 0,    ///< Logging level printing every pseudo-random number requested
        TRACE = 1,   ///< Software debugging information about what part is currently running
        DEBUG = 2,   ///< Detailed information about physics process
        INFO = 3,    ///< General information about processes (should not be called in run function)
        WARNING = 4, ///< Possible issue that could lead to unexpected results
        STATUS = 5,  ///< Only critical progress information; stays visible up to and including the WARNING threshold
        ERROR = 6,   ///< Critical problems that usually lead to termination of the framework
    };
    using enum Level;

    /**
     * @brief Convert an Allpix verbosity level to the corresponding spdlog level
     * @param level Allpix verbosity level
     * @return spdlog level value
     */
    constexpr spdlog::level::level_enum to_spdlog_level(Level level) {
        return static_cast<spdlog::level::level_enum>(level);
    }

    /**
     * @brief Convert a spdlog level back to the corresponding Allpix verbosity level
     * @param level spdlog level value
     * @return Allpix verbosity level
     */
    constexpr Level from_spdlog_level(spdlog::level::level_enum level) { return static_cast<Level>(level); }

    /**
     * Compare two logging levels and return the lower one
     *
     * @tparam LevelL left logging level type
     * @tparam LevelR right logging level type
     * @param lhs Left logging level
     * @param rhs Right logging level
     * @return Minimum verbosity allpix::Level
     */
    template <typename LevelL, typename LevelR> constexpr Level min_level(LevelL lhs, LevelR rhs) {
        return static_cast<Level>(
            std::min(static_cast<std::underlying_type_t<Level>>(lhs), static_cast<std::underlying_type_t<Level>>(rhs)));
    }

    /**
     * @brief Format of the logger
     */
    enum class Format : std::uint8_t {
        SHORT = 0,   ///< Only include a single character for the log level, the section header and the message
        DEFAULT = 1, ///< Also include the time and a full logging level description
        LONG = 2,    ///< All of the above and also information about the file and line where the message was defined
    };
    using enum Format;

} // namespace allpix

#endif /* ALLPIX_LOG_LEVEL_H */
