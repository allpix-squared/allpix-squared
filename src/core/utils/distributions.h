/**
 * @file
 * @brief Wrapper for Boost.Random random number distributions used for portability
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_RANDOM_DISTRIBUTIONS_H
#define ALLPIX_RANDOM_DISTRIBUTIONS_H

#include <boost/random/normal_distribution.hpp>

namespace allpix {
    template <typename T> using normal_distribution = boost::normal_distribution<T>;
} // namespace allpix

#endif // ALLPIX_RANDOM_DISTRIBUTIONS_H
