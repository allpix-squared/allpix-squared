/**
 * @file
 * @brief Definition of the MuPix10 CSADigitizerModel class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MUPIX10_H
#define ALLPIX_MUPIX10_H

#include "CSADigitizerModel.hpp"

namespace allpix {
    namespace csa {
        /**
         * @brief Implementation of a MuPix10 with a single threshold
         *
         * Implements charge dependent amplification with linear falling edge
         */
        class MuPix10 : public CSADigitizerModel {
        public:
            /**
             * @brief Essential virtual destructor
             */
            virtual ~MuPix10() = default;

            /**
             * @brief Model configuration
             */
            void configure(Configuration& config);

            /**
             * @brief Model amplification
             */
            std::vector<double> amplify_pulse(const Pulse& pulse) const;

        protected:
            // Impulse response function parameters
            double A_{}, R_{}, F_{};
        };
    } // namespace csa
} // namespace allpix

#endif /* ALLPIX_MUPIX10_H */
