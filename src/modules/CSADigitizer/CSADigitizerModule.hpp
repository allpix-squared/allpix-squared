/**
 * @file
 * @brief Definition of charge-sensitive amplifier digitization module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_CSA_DIGITIZER_MODULE_H
#define ALLPIX_CSA_DIGITIZER_MODULE_H

#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelCharge.hpp"

#include <TFormula.h>
#include <TH1D.h>
#include <TH2D.h>

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to simulate digitization of collected charges
     * @note This module supports multithreading
     *
     * This module provides a relatively simple simulation of a charge-sensitive amplifier
     * with Krummenacher feedack circuit while adding electronics
     * noise and simulating the threshold as well as accounting for threshold dispersion and ADC noise.
     */
    class CSADigitizerModule : public Module {

        /**
         * @brief Different implemented digitization models
         */
        enum class DigitizerType {
            SIMPLE, ///< Simplified parametrisation
            CSA,    ///< Enter all contributions to the transfer function as parameters
            CUSTOM, ///< Custom impulse response function using a ROOT::TFormula expression
            GRAPH, ///< External graph in .csv format
        };

    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        CSADigitizerModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialize optional ROOT histograms
         */
        void initialize() override;

        /**
         * @brief Simulate digitization process
         */
        void run(Event* event) override;

        /**
         * @brief Finalize and write optional histograms
         */
        void finalize() override;

    private:
        // Control of module output settings
        bool output_plots_{}, output_pulsegraphs_{};
        bool store_tot_{false}, store_toa_{false}, ignore_polarity_{};
        Messenger* messenger_;
        DigitizerType model_;

        // Function to calculate impulse response
        std::unique_ptr<TFormula> calculate_impulse_response_;
        std::unique_ptr<TGraph> graph_impulse_response_;

        // Parameters of the electronics: Noise, time-over-threshold logic
        double sigmaNoise_{}, clockToT_{}, clockToA_{}, threshold_{};

        // Helper variables for transfer function
        double integration_time_{};
        std::vector<double> impulse_response_function_;
        std::once_flag first_event_flag_;

        // Output histograms
        Histogram<TH1D> h_tot{}, h_toa{};
        Histogram<TH2D> h_pxq_vs_tot{};

        /**
         * @brief Calculate time of first threshold crossing
         * @param timestep Step size of the input pulse
         * @param pulse    Pulse after amplification and electronics noise
         * @return Tuple containing information about threshold crossing: Boolean (true if crossed), unsigned int (number
         *         of ToA clock cycles before crossing) and double (time of crossing)
         */
        std::tuple<bool, unsigned int, double> get_toa(double timestep, const std::vector<double>& pulse) const;

        /**
         * @brief Calculate time-over-threshold
         * @param  timestep    Step size of the input pulse
         * @param arrival_time Time of crossing the threshold
         * @param  pulse       Pulse after amplification and electronics noise
         * @return             Number of clock cycles signal was over threshold
         */
        unsigned int get_tot(double timestep, double arrival_time, const std::vector<double>& pulse) const;

        /**
         * @brief Create output plots of the pulses
         */
        void create_output_pulsegraphs(const std::string& s_event_num,
                                       const std::string& s_pixel_index,
                                       const std::string& s_name,
                                       const std::string& s_title,
                                       double timestep,
                                       const std::vector<double>& plot_pulse_vec);
    };
} // namespace allpix

#endif /* ALLPIX_CSA_DIGITIZER_MODULE_H */
