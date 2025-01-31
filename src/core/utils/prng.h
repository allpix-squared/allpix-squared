/**
 * @file
 * @brief Provides a wrapper around the STL pseudo-random number generator Mersenne Twister
 *
 * @copyright Copyright (c) 2020-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PRNG_H
#define ALLPIX_PRNG_H

#include "core/utils/log.h"

#include <random>

namespace allpix {

    /**
     * @brief Wrapper around the STL's Mersenne Twister
     */
    class RandomNumberGenerator : public std::mt19937_64 {
    public:
        /// @{
        /**
         * @brief Disallow copy-assignment
         */
        RandomNumberGenerator& operator=(const RandomNumberGenerator&) = delete;

        /// @{
        /**
         * @brief Disallow move assignment
         */
        RandomNumberGenerator& operator=(RandomNumberGenerator&&) = delete;

        /**
         * Redefine function operator to retrieve pseudo-random numbers. This allows us to log the number at retrieval.
         * Technically we are shadowing the base class operator since it is non-virtual and explicitly call it from within.
         *
         * @return 64-bit pseudo-random number
         */
        std::uint_fast64_t operator()() {
            // Only copy if we want to log it
            IFLOG(PRNG) {
                auto prn = std::mt19937_64::operator()();
                LOG(PRNG) << "Using random number " << prn;
                return prn;
            }
            else {
                return std::mt19937_64::operator()();
            }
        }
    };
} // namespace allpix

#endif /* ALLPIX_PRNG_H */
