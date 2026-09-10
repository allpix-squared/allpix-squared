/**
 * @file
 * @brief Environment variable wrappers
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdlib>
#include <mutex>
#include <optional>
#include <string>

namespace allpix {

    /**
     * @brief Wrapper for std::getenv to read environment variables
     * @details This helper reads environment variables. If a default is provided and the variable could not be found, the
     *          default is returned
     *
     * @param name Name of the environment variable
     * @return Optional with the value read from the environment variable
     */
    inline std::optional<std::string> getenv(const std::string& name) {
        static std::mutex getenv_mutex;
        const std::scoped_lock<std::mutex> lock(getenv_mutex);

        const auto* val = std::getenv(name.c_str()); // NOLINT(concurrency-mt-unsafe)
        if(val == nullptr) {
            return std::nullopt;
        }
        return std::string(val);
    }

} // namespace allpix
