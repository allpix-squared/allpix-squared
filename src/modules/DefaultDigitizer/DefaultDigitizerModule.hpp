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
    // define the module inheriting from the module base class
    class DefaultDigitizerModule : public Module {
    public:
        // constructor should take a pointer to the Configuration, the Messenger and the Geometry Manager
        DefaultDigitizerModule(Configuration, Messenger*, std::shared_ptr<Detector>);

        // init debug plots
        void init() override;

        // simulate digitization process:
        void run(unsigned int) override;

        // write the histograms
        void finalize() override;

    private:
        // random generator for this module
        std::mt19937_64 random_generator_;

        // configuration for this module
        Configuration config_;

        // messenger to emit test message
        Messenger* messenger_;

        // deposits for a specific detector
        std::shared_ptr<PixelChargeMessage> pixel_message_;

        // ROOT file for output plots
        TFile* output_file_;
        bool plots;

        // Histograms
        TH1D *h_pxq, *h_pxq_noise, *h_thr, *h_pxq_thr, *h_pxq_adc;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_H */
