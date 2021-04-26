/**
 * @file
 * @brief Definition of MuPix10 with second threshold for the MuPix digitizer module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MUPIX10_DOUBLE_H
#define ALLPIX_MUPIX10_DOUBLE_H

#include <tuple>
#include <vector>

#include "MuPix10.hpp"

namespace allpix {
    namespace mupix {
        /**
         * @brief Implementation of MuPix10 with a second threshold
         *
         * This class uses overwrites the TS1 calculation by setting the bool
         * for the crossed signal only when it also crosses the higher
         * threshold.
         */
        class MuPix10Double : public MuPix10 {
        public:
            MuPix10Double(Configuration& config);

            std::tuple<bool, unsigned int> get_ts1(double timestep, const std::vector<double>& pulse) const;

        protected:
            // Second threshold
            double threshold_high_{};
        };
    } // namespace mupix
} // namespace allpix

#endif /* ALLPIX_MUPIX10_DOUBLE_H */
