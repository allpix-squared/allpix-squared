/**
 * @file
 * @brief Compatibility header forwarding to the spdlog-based logging system
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LOG_H
#define ALLPIX_LOG_H

// The logging system now lives in src/core/log/. This header includes the new system for compatibility
#include "core/log/Logging.hpp"

#endif /* ALLPIX_LOG_H */
