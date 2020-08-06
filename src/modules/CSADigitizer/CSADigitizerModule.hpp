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

        /**
         * @brief Different implemented digitization models
         */
        enum class DigitizerType {
            SIMPLE, ///< Simplified parametrisation
            CSA,    ///< Enter all contributions to the transfer function as parameters
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
        void init() override;

        /**
         * @brief Simulate digitization process
         */
        void run(unsigned int) override;

        /**
         * @brief Finalize and write optional histograms
         */
        void finalize() override;

        /**
         * @brief Compare output pulse with threshold for ToA/ToT
         */
        void compareWithThreshold(double& toa, double& tot, double timestep, std::vector<double> amplified_pulse_with_noise);

        /**
         * @brief Create output plots of the pulses
         */
        void createOutputPulsegraphs(const std::string& s_event_num,
                                     const std::string& s_pixel_index,
                                     const std::string& s_name,
                                     const std::string& s_title,
                                     double timestep,
                                     std::vector<double> plot_pulse_vec);

    private:
        bool output_plots_{}, output_pulsegraphs_{}, store_tot_{true};
        std::once_flag first_event_flag_;

        std::mt19937_64 random_generator_;

        Messenger* messenger_;

        // Input message with the charges on the pixels
        std::shared_ptr<PixelChargeMessage> pixel_message_;

        DigitizerType model_;
        // krummenacher current, detector capacitance, feedback capacitance, amp output capacitance,
        // transconductance, Boltzmann kT, feedback time constant, risetime time constant
        double ikrum_{}, capacitance_detector_{}, capacitance_feedback_{}, capacitance_output_;
        double gm_{}, boltzmann_kT_{}, tauF_{}, tauR_{};
        // parameters of the electronics: noise, time-over-threshold logic
        double sigmaNoise_{}, clockToT_{}, clockToA_{}, threshold_{};

        // helper variables for transfer function
        double transconductance_feedback_{}, resistance_feedback_{}, tmax_{};
        std::vector<double> impulse_response_function_;

        // Output histograms
        TH1D *h_tot{}, *h_toa{};
        TH2D* h_pxq_vs_tot{};
    };
} // namespace allpix

#endif /* ALLPIX_CSA_DIGITIZER_MODULE_H */
