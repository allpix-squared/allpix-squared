/**
 * @file
 * @brief Wrapper for Boost.Random random number distributions used for portability
 *
 * @copyright Copyright (c) 2019-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_RANDOM_DISTRIBUTIONS_H
#define ALLPIX_RANDOM_DISTRIBUTIONS_H

#include <boost/random/exponential_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/piecewise_linear_distribution.hpp>
#include <boost/random/poisson_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>

namespace allpix {
    template <typename T> using normal_distribution = boost::random::normal_distribution<T>;
    template <typename T> using piecewise_linear_distribution = boost::random::piecewise_linear_distribution<T>;
    template <typename T> using poisson_distribution = boost::random::poisson_distribution<T>;
    template <typename T> using uniform_real_distribution = boost::random::uniform_real_distribution<T>;
    template <typename T> using exponential_distribution = boost::random::exponential_distribution<T>;
} // namespace allpix

#endif // ALLPIX_RANDOM_DISTRIBUTIONS_H
