// stl include
#include <string>

// module include
#include "DefaultDigitizerModule.hpp"

// framework includes
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"
#include "core/utils/random.h"

// ROOT includes
#include <TFile.h>
#include <TH1D.h>
#include <TProfile.h>

using namespace allpix;

// constructor to load the module
DefaultDigitizerModule::DefaultDigitizerModule(Configuration config,
                                               Messenger* messenger,
                                               std::shared_ptr<Detector> detector)
    : Module(std::move(detector)), config_(std::move(config)), messenger_(messenger), pixel_message_(nullptr) {

    // require PixelCharge message for single detector
    messenger_->bindSingle(this, &DefaultDigitizerModule::pixel_message_, MsgFlags::REQUIRED);

    // seed the random generator
    random_generator_.seed(get_random_seed());

    // set defaults for config variables
    config_.setDefault<int>("electronics_noise", Units::get(110, "e"));
    config_.setDefault<int>("threshold", Units::get(600, "e"));
    config_.setDefault<int>("threshold_smearing", Units::get(30, "e"));
    config_.setDefault<int>("adc_smearing", Units::get(300, "e"));

    plots = config_.get<bool>("output_plots", false);
}

// init debug plots
void DefaultDigitizerModule::init() {
    if(plots) {
        std::string file_name =
            getOutputPath(config_.get<std::string>("output_plots_file_name", "debug_digitizer") + ".root");
        output_file_ = new TFile(file_name.c_str(), "RECREATE");

        // book histograms:

        h_pxq = new TH1D("pixelcharge", "raw pixel charge;pixel charge [ke];pixels", 100, 0, 10);
        h_pxq_noise = new TH1D("pixelcharge_noise", "pixel charge w/ el. noise;pixel charge [ke];pixels", 100, 0, 10);
        h_thr = new TH1D("threshold", "applied threshold; threshold [ke];events", 10, 0, 30);
        h_pxq_thr = new TH1D("pixelcharge_threshold", "pixel charge above threshold;pixel charge [ke];pixels", 100, 0, 10);
        h_pxq_adc = new TH1D("pixelcharge_adc", "pixel charge after ADC;pixel charge [ke];pixels", 100, 0, 10);
    }
}

// run method fetching a message and outputting its own
void DefaultDigitizerModule::run(unsigned int) {

    // FIXME do I have to check if I received a message? I require them when binding...
    for(auto& pixel_charge : pixel_message_->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto charge = static_cast<double>(pixel_charge.getCharge());

        LOG(DEBUG) << "Received pixel " << pixel.x() << "," << pixel.y() << ", charge " << charge << "e";
        if(plots) {
            h_pxq->Fill(charge / 1e3);
        }

        // Add electronics noise from Gaussian:
        std::normal_distribution<double> el_noise(0, config_.get<unsigned int>("electronics_noise"));
        charge += el_noise(random_generator_);

        LOG(DEBUG) << "Charge with noise: " << charge;
        if(plots) {
            h_pxq_noise->Fill(charge / 1e3);
        }

        // FIXME Simulate gain / gain smearing

        // Smear the threshold
        std::normal_distribution<double> thr_smearing(0, config_.get<unsigned int>("threshold_smearing"));
        double threshold = config_.get<unsigned int>("threshold") + thr_smearing(random_generator_);
        if(plots) {
            h_thr->Fill(threshold / 1e3);
        }

        // Discard charges below threshold:
        if(charge < threshold) {
            LOG(DEBUG) << "Below smeared threshold: " << charge << " < " << threshold;
            continue;
        }

        LOG(DEBUG) << "Passed threshold: " << charge << " > " << threshold;
        if(plots) {
            h_pxq_thr->Fill(charge / 1e3);
        }

        // Add ADC smearing:
        std::normal_distribution<double> adc_smearing(0, config_.get<unsigned int>("adc_smearing"));
        charge += adc_smearing(random_generator_);
        if(plots) {
            h_pxq_adc->Fill(charge / 1e3);
        }

        // FIXME Simulate analog / digital cross talk
        // double crosstalk_neigubor_row = 0.00;
        // double crosstalk_neigubor_column = 0.00;
    }

    // FIXME Create and dispatch output message when defined (PixelHit)
}

// create file and write the histograms to it
void DefaultDigitizerModule::finalize() {
    // go to output file
    output_file_->cd();

    // write histograms
    LOG(INFO) << "Writing histograms to file";
    h_pxq->Write();
    h_pxq_noise->Write();
    h_thr->Write();
    h_pxq_thr->Write();
    h_pxq_adc->Write();

    // close the file
    output_file_->Close();
}
