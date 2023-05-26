/**
 * @file
 * @brief Utility to perform fast pow interpolation from tabulated data
 *
 *
 *
 * @copyright Copyright (c) 2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_TABULATED_POW_H
#define ALLPIX_TABULATED_POW_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

namespace allpix {
    /**
     * @brief Class to pre-calculate powers of a fixed exponent within a defined range
     *
     * This class implements a tabulated version of x^y where y is fixed and the range of x is known. When instantiating,
     * the range of x, value of y and the binning has to be provided. The exact value of pow(x, y) is calculated for each
     * of the bin boundaries. After construction, the result of x^y can be obtained for every value of x within the defined
     * range using linear interpolation between neighboring bins.
     */
    class TabulatedPow {
    private:
        // Tabulated pow values
        std::vector<double> table_;
        double x_min_;
        double dx_;

    public:
        /**
         * @brief  Constructs a new tabulated pow instance.
         * @param  min   The minimum value for the base
         * @param  max   The maximum value for the base
         * @param  y     Fixed value of the exponent
         * @param  bins  Number of bins to be calculated and cached
         */
        TabulatedPow(double min, double max, double y, size_t bins)
            : x_min_(min), dx_((max - min) / static_cast<double>(bins - 1)) {
            assert(bins >= 3);
            assert(min < max);

            // Generate lookup table:
            table_.resize(bins);
            for(size_t idx = 0; idx < bins; ++idx) {
                double x = dx_ * static_cast<double>(idx) + x_min_;
                table_[idx] = std::pow(x, y);
            }
        }

        /**
         * @brief  Gets the specified x.
         * @param  x     Value of the base to calculate x^y for.
         * @return Interpolated value of x^y
         *
         * @note The provided x has to be within the defined range provided to the constructor.
         */
        double get(double x) const {
            // Calculate position on pre-calculate table
            double pos = (x - x_min_) / dx_;

            // Calculate left index by truncation to integer, clamping to pre-calculated range
            size_t idx = std::min(static_cast<size_t>(pos), table_.size() - 2);

            // Linear interpolation between left and right bin
            double tmp = pos - static_cast<double>(idx);
            return table_[idx] * (1 - tmp) + tmp * table_[idx + 1];
        };
    };
} // namespace allpix

#endif /* ALLPIX_TABULATED_POW_H */
