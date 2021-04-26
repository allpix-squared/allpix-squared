/**
 * @file
 * @brief Definition of MuPix10 for the MuPix digitizer module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MUPIX10_H
#define ALLPIX_MUPIX10_H

#include "MuPixModel.hpp"

namespace allpix {
    namespace mupix {
        /**
         * @brief Implementation of MuPix10 with a single threshold
         *
         * This class uses all reference functions. It implements a triangular
         * impulse response function for the amplification.
         */
        class MuPix10 : public MuPixModel {
        public:
            MuPix10(Configuration& config);

            std::vector<double> amplify_pulse(const Pulse& pulse) const;

        protected:
            // Cutoff time for pulse
            double pulse_cutoff_time_{};

            // Charge amplification parameters
            double A_m_{}, A_c_{}, A_mu_{};

            // Amplification shaping parameters
            double t_R_{}, t_F_{}, t_S_{};

            // Feedback parameters
            double Fb_{}, Fb_D_{};

            // Saturation parameters
            double U_sat_{};
        };
    } // namespace mupix
} // namespace allpix

#endif /* ALLPIX_MUPIX10_H */
