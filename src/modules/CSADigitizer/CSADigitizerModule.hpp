/**
 * @file
 * @brief Definition of charge-sensitive amplifier digitization module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
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

#include <TF1.h>
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
        void init() override;

        /**
         * @brief Simulate digitization process
         */
        void run(unsigned int) override;

        /**
         * @brief Finalize and write optional histograms
         */
        void finalize() override;

    private:
      bool output_plots_{}, output_pulsegraphs_{}, first_event_{true};
      
        std::mt19937_64 random_generator_;

        Messenger* messenger_;

        // Input message with the charges on the pixels
        std::shared_ptr<PixelChargeMessage> pixel_message_;

        // Statistics
        unsigned long long total_hits_{};

      
        // krummenacher_current, detector_capacitance, feedback_capacitance, amp_output_capacitance, transconductance, v_temperature
        double ikrum_{}, cd_{}, cf_{}, co_{}, gm_{}, vt_{}, tauF_{}, tauR_{};
      
        // helper variables for transfer function
        double gf_{}, rf_{}, tmax_{};
        double *impulseResponse_{};	  
        TF1 *fImpulseResponse_{};

        std::vector<double> amplified_pulse_;
        // Output histograms
        TH1D *h_pxq{}, *h_tot{}, *h_toa{};
	TH2D *h_pxq_vs_tot{};
       
    };
} // namespace allpix

#endif /* ALLPIX_CSA_DIGITIZER_MODULE_H */
