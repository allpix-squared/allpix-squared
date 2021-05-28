/**
 * @file
 * @brief Definition of MuPix digitization module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MUPIX_DIGITIZER_MODULE_H
#define ALLPIX_MUPIX_DIGITIZER_MODULE_H

#include <memory>
#include <random>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelCharge.hpp"

#include "MuPixModel.hpp"

#include <TF1.h>
#include <TH1D.h>
#include <TH1F.h>

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to simulate MuPix digitization of collected charges
     * @note This module supports parallelization
     *
     * This module provides a relatively simple simulation of a charge-sensitive amplifier
     * that works similar to MuPix type detectors. Compared to the CSADigitizer module,
     * amplification and threshold calculation work differently.
     */
    class MuPixDigitizerModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        MuPixDigitizerModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

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
        Messenger* messenger_;

        // Chip implementation
        std::unique_ptr<mupix::MuPixModel> model_;

        // Noise parameter
        double sigmaNoise_{};

        // Output histograms
        Histogram<TH1D> h_ts1{}, h_ts2{};
        Histogram<TH1F> h_tot{};

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

#endif /* ALLPIX_MUPIX_DIGITIZER_MODULE_H */
