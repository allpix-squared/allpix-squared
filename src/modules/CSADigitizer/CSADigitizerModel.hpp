/**
 * @file
 * @brief Definition of the CSADigitizerModel class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_CSA_DIGITIZER_MODEL_H
#define ALLPIX_CSA_DIGITIZER_MODEL_H

#include <tuple>
#include <vector>

#include "core/config/Configuration.hpp"

#include "objects/Pulse.hpp"

namespace allpix {
    namespace csa {
        /**
         * @brief Reference Implementation for a Digitizer
         *
         * This class provides three virtual functions: amplification and
         * TS1 and TS2 calculation. For TS1 and TS2 calculation, a reference
         * implementation exists, the amplification needs to be implemented.
         */
        class CSADigitizerModel {
        public:
            /**
             * @brief Essential virtual destructor
             */
            virtual ~CSADigitizerModel() = default;

            /**
             * @brief Called to configure variables
             */
            virtual void configure(Configuration& config);

            /**
             * @brief Amplifies charge pulse
             * @note This needs to be implemented.
             * @param pulse Pulse object as received by the module
             * @return amplified pulse vector
             */
            virtual std::vector<double> amplify_pulse(const Pulse& pulse) const;

            /**
             * @brief Calculate clock cycles of first threshold crossing (TS1)
             * @param timestep Step size of the input pulse
             * @param pulse    Pulse after amplification and electronics noise
             * @return Tuple containing information about threshold crossing: if crossed and clock cycles
             */
            virtual std::tuple<bool, unsigned int> get_ts1(double timestep, const std::vector<double>& pulse) const;

            /**
             * @brief Calculate clock cycles of first threshold crossing from above to below (TS2)
             * @param ts1      TS1 clock cycles of arrival
             * @param timestep Step size of the input pulse
             * @param pulse    Pulse after amplification and electronics noise
             * @return TS2 clock cycles of crossing
             */
            virtual unsigned int get_ts2(unsigned int ts1, double timestep, const std::vector<double>& pulse) const;

            /**
             * @brief Calculate time of first threshold crossing
             * @param timestep Step size of the input pulse
             * @param pulse    Pulse after amplification and electronics noise
             * @return Tuple containing information about threshold crossing: if crossed and time
             */
            std::tuple<bool, double> get_arrival(double timestep, const std::vector<double>& pulse) const;

            /**
             * @brief Calculate pulse integral
             * @param pulse Pulse after amplification and electronics noise
             * @return Integral of the pulse
             */
            double get_pulse_integral(const std::vector<double>& pulse) const;

        protected:
            /**
             * @brief Calculates whether a volatage is below the threshold
             * @param voltage Voltage to compare
             * @return Whether the volatage is below the threshold
             */
            bool calculate_is_below_threshold(double voltage) const;

            /**
             * @brief Calculates whether a volatage is above the threshold
             * @param voltage Voltage to compare
             * @return Whether the volatage is above the threshold
             */
            bool calculate_is_above_threshold(double voltage) const;

            // Parameters for the threshold logic
            double threshold_{}, clockTS1_{}, clockTS2_{};

            // Pulse calculation times
            double integration_time_{};

            // FIXME: descripotion
            bool ignore_polarity_{};
        };
    } // namespace csa
} // namespace allpix

#endif /* ALLPIX_CSA_DIGITIZER_MODEL_H */
