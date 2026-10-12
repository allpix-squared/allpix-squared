/**
 * @file
 * @brief Header for logging
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LOGGING_H
#define ALLPIX_LOGGING_H

#include "Level.hpp"
#include "LogContext.hpp"
#include "Logger.hpp"
#include "LoggerManager.hpp"
#include "Macros.hpp"

namespace allpix {
    // Compatibility aliases
    using LogLevel = Level;
    using LogFormat = Format;
} // namespace allpix

#endif /* ALLPIX_LOGGING_H */
