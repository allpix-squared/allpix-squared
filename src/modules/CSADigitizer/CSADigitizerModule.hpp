/**
 * @file
 * @brief Definition of charge-sensitive amplifier digitization module
 * @copyright Copyright (c) 2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_CSA_DIGITIZER_MODULE_H
#define ALLPIX_CSA_DIGITIZER_MODULE_H

#include <memory>
#include <random>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelCharge.hpp"

#include "CSADigitizerModel.hpp"

#include <TH1D.h>
#include <TH2D.h>

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to simulate digitization of collected charges
     * @note This module supports parallelization
     *
     * This module provides a relatively simple simulation of a charge-sensitive amplifier
     * with Krummenacher feedack circuit while adding electronics
     * noise and simulating the threshold as well as accounting for threshold dispersion and ADC noise.
     */
    class CSADigitizerModule : public Module {
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

        // threshold logic control variables
        bool store_ts1_{false}, store_ts2_{false}, signal_is_ts2_{false};

        // FIXME: descripotion
        bool ignore_polarity_{};

        // FIXME: descripotion
        Messenger* messenger_;

        // Digitizer Model
        std::unique_ptr<csa::CSADigitizerModel> model_;

        // Parameters of the electronics: Noise, time-over-threshold logic
        double sigmaNoise_{}, clockTS2_{}, clockTS1_{}, threshold_{};

        // Helper variables for transfer function
        double integration_time_{};

        // Output histograms
        Histogram<TH1D> h_tot{}, h_toa{};
        Histogram<TH2D> h_pxq_vs_tot{};

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
