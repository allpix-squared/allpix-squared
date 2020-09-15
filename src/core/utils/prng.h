/**
 * @file
 * @brief Provides a wrapper around the STL pseudo-random number generator Mersenne Twister
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PRNG_H
#define ALLPIX_PRNG_H

#include "core/utils/log.h"

#include <random>

namespace allpix {

    /**
     * @brief Wrapper around the STL's Mersenne Twister
     */
    class MersenneTwister : public std::mt19937_64 {
    public:
        std::uint_fast64_t operator()(){// Only copy if we want to log it
                                        IFLOG(PRNG){auto prn = std::mt19937_64::operator()();
        LOG(PRNG) << "Using random number " << prn;
        return prn;
    } else {
        return std::mt19937_64::operator()();
    }
}; // namespace allpix
}
;
} // namespace allpix

#endif /* ALLPIX_PRNG_H */
