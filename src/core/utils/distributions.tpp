/**
 * @file
 * @brief Implementation of portable distributions
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

namespace allpix {

    /**
     * Polar method due to Marsaglia.
     *
     * Devroye, L. Non-Uniform Random Variates Generation. Springer-Verlag,
     * New York, 1986, Ch. V, Sect. 4.4.
     */
    template <typename T>
    template <typename RNG>
    typename normal_distribution<T>::result_type normal_distribution<T>::operator()(RNG& rng, const param_type& param) {
        result_type __ret;
        distributionAdaptor<RNG, result_type> __aurng(rng);

        if(saved_available_) {
            saved_available_ = false;
            __ret = saved_;
        } else {
            result_type __x, __y, __r2;
            do {
                __x = result_type(2.0) * __aurng() - 1.0;
                __y = result_type(2.0) * __aurng() - 1.0;
                __r2 = __x * __x + __y * __y;
            } while(__r2 > 1.0 || __r2 == 0.0);

            const result_type __mult = std::sqrt(-2 * std::log(__r2) / __r2);
            saved_ = __x * __mult;
            saved_available_ = true;
            __ret = __y * __mult;
        }

        __ret = __ret * param.stddev() + param.mean();
        return __ret;
    }
} // namespace allpix
