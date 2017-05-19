#ifndef ALLPIX_DEFAULT_DIGITIZER_MODULE_H
#define ALLPIX_DEFAULT_DIGITIZER_MODULE_H

// stl includes
#include <memory>
#include <random>
#include <string>

// include the dependent objects
#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelCharge.hpp"

#include <TFile.h>
#include <TH1D.h>
#include <TProfile.h>

namespace allpix {
    /**
      * @brief Module to simulate digitization of collected charges
      *
      * This module provides a relatively simple simulation of the frontend electronics behavior. It simulates the
     * propagation of the signal of collected charges through the amplifier, comparator and ADC while adding electronics
     * noise and simulating the threshold as well as accounting for threshold dispersion and ADC noise.
      */
    class DefaultDigitizerModule : public Module {
    public:
        /**
           * @brief Constructor for the DefaultDigitizerModule, inheriting from the base class allpix::Module
           * @param config configuration object for this module as retrieved from the steering file
           * @param messenger pointer to the messenger object to allow binding to messages on the bus
       * @param detector pointer to the detector for this module instance
           */
        DefaultDigitizerModule(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialize ROOT histograms and ROOT output file
         */
        void init() override;

        /**
         * @brief Simulate digitization process
         */
        void run(unsigned int) override;

        /**
         * @brief Finalize histogramming and write the to the output file
         */
        void finalize() override;

    private:
        // random generator for this module
        std::mt19937_64 random_generator_;

        // configuration for this module
        Configuration config_;

        // messenger to bind to input message and dispatch output message
        Messenger* messenger_;

        // retrieved message containing collected charges per pixel for a specific detector
        std::shared_ptr<PixelChargeMessage> pixel_message_;

        // ROOT file for output plots
        TFile* output_file_;

        // Histograms
        TH1D *h_pxq, *h_pxq_noise, *h_thr, *h_pxq_thr, *h_pxq_adc;
    };
} // namespace allpix

#endif /* ALLPIX_DEFAULT_DIGITIZER_MODULE_H */
