/**
 * @file
 * @brief Definition of MuPix10 in ramp mode for the MuPix digitizer module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MUPIX10_RAMP_H
#define ALLPIX_MUPIX10_RAMP_H

#include <vector>

#include "MuPix10.hpp"

namespace allpix {
    namespace mupix {
        /**
         * @brief Implementation of MuPix10 in ramp mode
         *
         * This class uses overwrites the TS2 calculation by changing the
         * static threshold in the algorithm with a linear rising one.
         */
        class MuPix10Ramp : public MuPix10 {
        public:
            MuPix10Ramp(Configuration& config);

            unsigned int get_ts2(unsigned int arrival, double timestep, const std::vector<double>& pulse) const;

        protected:
            // Parameters for dynamic threshold
            double threshold_slew_rate_{};
        };
    } // namespace mupix
} // namespace allpix

#endif /* ALLPIX_MUPIX10_RAMP_H */
