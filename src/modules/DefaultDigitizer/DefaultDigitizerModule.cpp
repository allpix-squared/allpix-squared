// stl include
#include <string>

// module include
#include "DefaultDigitizerModule.hpp"

// framework includes
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"
#include "core/utils/random.h"
#include "core/utils/unit.h"
#include "tools/ROOT.h"

// ROOT includes
#include <TFile.h>
#include <TH1D.h>
#include <TProfile.h>

#include "objects/PixelHit.hpp"

using namespace allpix;

// Constructor to load the module
DefaultDigitizerModule::DefaultDigitizerModule(Configuration config,
                                               Messenger* messenger,
                                               std::shared_ptr<Detector> detector)
    : Module(config, std::move(detector)), config_(std::move(config)), messenger_(messenger), pixel_message_(nullptr) {

    // Require PixelCharge message for single detector
    messenger_->bindSingle(this, &DefaultDigitizerModule::pixel_message_, MsgFlags::REQUIRED);

    // Seed the random generator
    random_generator_.seed(get_random_seed());

    // Set defaults for config variables
    config_.setDefault<int>("electronics_noise", Units::get(110, "e"));
    config_.setDefault<int>("threshold", Units::get(600, "e"));
    config_.setDefault<int>("threshold_smearing", Units::get(30, "e"));
    config_.setDefault<int>("adc_smearing", Units::get(300, "e"));

    config_.setDefault<bool>("output_plots", false);
}

// Initialize output plots
void DefaultDigitizerModule::init() {
    if(config_.get<bool>("output_plots")) {
        LOG(TRACE) << "Creating output plots";

        // book histograms:
        h_pxq = new TH1D("pixelcharge", "raw pixel charge;pixel charge [ke];pixels", 100, 0, 10);
        h_pxq_noise = new TH1D("pixelcharge_noise_", "pixel charge w/ el. noise;pixel charge [ke];pixels", 100, 0, 10);
        h_thr = new TH1D("threshold", "applied threshold; threshold [ke];events", 100, 0, 10);
        h_pxq_thr = new TH1D("pixelcharge_threshold_", "pixel charge above threshold;pixel charge [ke];pixels", 100, 0, 10);
        h_pxq_adc = new TH1D("pixelcharge_adc", "pixel charge after ADC;pixel charge [ke];pixels", 100, 0, 10);
    }
}

// run method fetching a message and outputting its own
void DefaultDigitizerModule::run(unsigned int) {

    // Loop through all pixels with charges
    std::vector<PixelHit> hits;
    for(auto& pixel_charge : pixel_message_->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto charge = static_cast<double>(pixel_charge.getCharge());

        LOG(DEBUG) << "Received pixel " << pixel << ", charge " << Units::display(charge, "e");
        if(config_.get<bool>("output_plots")) {
            h_pxq->Fill(charge / 1e3);
        }

        // Add electronics noise from Gaussian:
        std::normal_distribution<double> el_noise(0, config_.get<unsigned int>("electronics_noise"));
        charge += el_noise(random_generator_);

        LOG(DEBUG) << "Charge with noise: " << Units::display(charge, "e");
        if(config_.get<bool>("output_plots")) {
            h_pxq_noise->Fill(charge / 1e3);
        }

        // FIXME Simulate gain / gain smearing

        // Smear the threshold, Gaussian distribution around "threshold" with width "threshold_smearing"
        std::normal_distribution<double> thr_smearing(config_.get<unsigned int>("threshold"),
                                                      config_.get<unsigned int>("threshold_smearing"));
        double threshold = thr_smearing(random_generator_);
        if(config_.get<bool>("output_plots")) {
            h_thr->Fill(threshold / 1e3);
        }

        // Discard charges below threshold:
        if(charge < threshold) {
            LOG(DEBUG) << "Below smeared threshold: " << Units::display(charge, "e") << " < "
                       << Units::display(threshold, "e");
            continue;
        }

        LOG(DEBUG) << "Passed threshold: " << Units::display(charge, "e") << " > " << Units::display(threshold, "e");
        if(config_.get<bool>("output_plots")) {
            h_pxq_thr->Fill(charge / 1e3);
        }

        // Add ADC smearing:
        std::normal_distribution<double> adc_smearing(0, config_.get<unsigned int>("adc_smearing"));
        charge += adc_smearing(random_generator_);
        if(config_.get<bool>("output_plots")) {
            h_pxq_adc->Fill(charge / 1e3);
        }

        // Add the hit to the hitmap
        hits.emplace_back(pixel, 0, charge, &pixel_charge);

        // FIXME Simulate analog / digital cross talk
        // double crosstalk_neigubor_row = 0.00;
        // double crosstalk_neigubor_column = 0.00;
    }

    // output summary and update statistics
    LOG(INFO) << "Digitized " << hits.size() << " pixel hits";
    total_hits_ += hits.size();

    if(!hits.empty()) {
        // Create and dispatch hit message
        auto hits_message = std::make_shared<PixelHitMessage>(std::move(hits), getDetector());
        messenger_->dispatchMessage(this, hits_message);
    }
}

// Create file and write the histograms to it if we have plots enabled
void DefaultDigitizerModule::finalize() {
    if(config_.get<bool>("output_plots")) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        h_pxq->Write();
        h_pxq_noise->Write();
        h_thr->Write();
        h_pxq_thr->Write();
        h_pxq_adc->Write();
    }

    LOG(INFO) << "Digitized " << total_hits_ << " pixel hits in total";
}
