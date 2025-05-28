/**
 * @file
 * @brief Wrapper for Boost.Random random number distributions used for portability
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
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

    // Generalized Gaussian Distribution (GGD) for CoulombProjectionPropagation module
    template <typename RealType = double> class generalized_gaussian_distribution {
    public:
        using result_type = RealType;

        // Constructor with parameters alpha and beta; mean = 0
        generalized_gaussian_distribution(RealType alpha = RealType{1}, RealType beta = RealType{2})
            : alpha_(alpha), beta_(beta), gamma_dist_(RealType{1} / beta, RealType{1}),
              uniform_dist_(RealType{-1}, RealType{1}) {}

        template <typename RNG> result_type operator()(RNG& rng) {
            const RealType u = gamma_dist_(rng);
            const RealType sign = (uniform_dist_(rng) < 0) ? -1.0 : 1.0;
            return sign * alpha_ * std::pow(u, RealType{1} / beta_);
        }

        static constexpr result_type min() noexcept { return std::numeric_limits<result_type>::lowest(); }
        static constexpr result_type max() noexcept { return std::numeric_limits<result_type>::max(); }

        RealType alpha() const noexcept { return alpha_; }
        RealType beta() const noexcept { return beta_; }

    private:
        RealType alpha_, beta_;
        std::gamma_distribution<RealType> gamma_dist_;
        boost::random::uniform_real_distribution<RealType> uniform_dist_;
    };

    template <typename T> using ggd_distribution = generalized_gaussian_distribution<T>;
} // namespace allpix

#endif // ALLPIX_RANDOM_DISTRIBUTIONS_H
