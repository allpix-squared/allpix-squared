/**
 * @file
 * @brief Definition of the Simple CSADigitizerModel class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_SIMPLE_MODEL_H
#define ALLPIX_SIMPLE_MODEL_H

#include "CSADigitizerModel.hpp"

namespace allpix {
    namespace csa {
        /**
         * @brief Implementation of a simple charge sensitive amplifier
         *
         * Implements a CSA with Krummenacher Feedback with a simple
         * parametrisation of rise time, fall time, and feedback capacitance.
         */
        class SimpleModel : public CSADigitizerModel {
        public:
            /**
             * @brief Essential virtual destructor
             */
            virtual ~SimpleModel() = default;

            /**
             * @brief Model configuration
             */
            void configure(Configuration& config);

            /**
             * @brief Model amplification
             */
            std::vector<double> amplify_pulse(const Pulse& pulse) const;

        protected:
            /**
             * @brief Calculates impulse response vector
             */
            void precalculate_impulse_response();

            // Vector containing precalculated impulse response
            std::vector<double> impulse_response_;

            // Precision of precalculated impulse response
            double timestep_{};

            // Impulse response function parameters
            double resistance_feedback_{}, tauF_{}, tauR_{};
        };
    } // namespace csa
} // namespace allpix

#endif /* ALLPIX_SIMPLE_MODEL_H */
