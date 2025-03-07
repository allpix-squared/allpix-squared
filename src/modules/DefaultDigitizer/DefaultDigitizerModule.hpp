/**
 * @file
 * @brief Definition of default digitization module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_DEFAULT_DIGITIZER_MODULE_H
#define ALLPIX_DEFAULT_DIGITIZER_MODULE_H

#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelCharge.hpp"

#include "tools/ROOT.h"

#include <TFormula.h>
#include <TH1D.h>
#include <TH2D.h>

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to simulate digitization of collected charges
     * @note This module supports multithreading
     *
     * This module provides a relatively simple simulation of the frontend electronics behavior. It simulates the
     * propagation of the signal of collected charges through the amplifier, comparator and ADC while adding electronics
     * noise and simulating the threshold as well as accounting for threshold dispersion and ADC noise.
     */
    class DefaultDigitizerModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        DefaultDigitizerModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialize optional ROOT histograms
         */
        void initialize() override;

        /**
         * @brief Simulate digitization process
         */
        void run(Event*) override;

        /**
         * @brief Finalize and write optional histograms
         */
        void finalize() override;

    private:
        Messenger* messenger_;

        /**
         * @brief Helper function to calculate time of crossing the threshold
         * @param  pixel_charge PixelCharge object to calculate the threshold crossing for
         * @param  threshold    Threshold to be crossed
         * @return              Timestamp of threshold crossing in internal units
         */
        double time_of_arrival(const PixelCharge& pixel_charge, double threshold) const;

        // Configuration
        bool output_plots_{};

        unsigned int electronics_noise_{};
        std::unique_ptr<TFormula> gain_function_{};

        bool saturation_{};
        unsigned int saturation_mean_{}, saturation_width_{};

        unsigned int threshold_{}, threshold_smearing_{};

        int qdc_resolution_{};
        unsigned int qdc_smearing_{};
        double qdc_offset_{};
        double qdc_slope_{};
        bool allow_zero_qdc_{};

        int tdc_resolution_{};
        double tdc_smearing_{};
        double tdc_offset_{};
        double tdc_slope_{};
        bool allow_zero_tdc_{};

        // Statistics
        std::atomic<unsigned long long> total_hits_{};

        // Output histograms
        Histogram<TH1D> h_pxq, h_pxq_noise, h_gain, h_pxq_gain, h_thr, h_pxq_thr, h_pxq_sat, h_pxq_adc_smear, h_pxq_adc,
            h_px_toa, h_px_tdc_smear, h_px_tdc;
        Histogram<TH2D> h_calibration, h_toa_calibration;
    };
} // namespace allpix

#endif /* ALLPIX_DEFAULT_DIGITIZER_MODULE_H */
