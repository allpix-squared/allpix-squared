/**
 * @file
 * @brief Definition of the Krummenacher Current CSADigitizerModel class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_KRUMMENACHER_CURRENT_MODEL_H
#define ALLPIX_KRUMMENACHER_CURRENT_MODEL_H

#include "SimpleModel.hpp"

namespace allpix {
    namespace csa {
        /**
         * @brief Implementation of a CSA with Krummenacher Feedback
         *
         * This is identical to the simple model, but uses a more advanced
         * parametrisation for the impulse response.
         */
        class KrummenacherCurrentModel : public SimpleModel {
        public:
            /**
             * @brief Essential virtual destructor
             */
            virtual ~KrummenacherCurrentModel() = default;

            /**
             * @brief Model configuration
             */
            void configure(Configuration& config);
        };
    } // namespace csa
} // namespace allpix

#endif /* ALLPIX_KRUMMENACHER_CURRENT_MODEL_H */
