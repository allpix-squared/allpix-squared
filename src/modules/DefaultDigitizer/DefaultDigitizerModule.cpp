/**
 * @file
 * @brief Implementation of default digitization module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DefaultDigitizerModule.hpp"

#include "core/utils/unit.h"
#include "tools/ROOT.h"

#include <TFile.h>
#include <TH1D.h>
#include <TProfile.h>

#include "objects/PixelHit.hpp"

using namespace allpix;

DefaultDigitizerModule::DefaultDigitizerModule(Configuration& config,
                                               Messenger* messenger,
                                               std::shared_ptr<Detector> detector)
    : Module(config, std::move(detector)), messenger_(messenger) {
    // Require PixelCharge message for single detector
    messenger_->bindSingle<DefaultDigitizerModule, PixelChargeMessage>(this);

    // Set defaults for config variables
    config_.setDefault<int>("electronics_noise", Units::get(110, "e"));
    config_.setDefault<double>("gain", 1.0);
    config_.setDefault<double>("gain_smearing", 0.0);
    config_.setDefault<int>("threshold", Units::get(600, "e"));
    config_.setDefault<int>("threshold_smearing", Units::get(30, "e"));

    config_.setDefault<int>("adc_resolution", 0);
    config_.setDefault<int>("adc_smearing", Units::get(300, "e"));
    config_.setDefault<double>("adc_offset", Units::get(0, "e"));
    config_.setDefault<double>("adc_slope", Units::get(10, "e"));

    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<int>("output_plots_scale", Units::get(30, "ke"));
    config_.setDefault<int>("output_plots_bins", 100);
}

void DefaultDigitizerModule::init(std::mt19937_64&) {
    // Conversion to ADC units requested:
    if(config_.get<int>("adc_resolution") > 31) {
        throw InvalidValueError(config_, "adc_resolution", "precision higher than 31bit is not possible");
    }
    if(config_.get<int>("adc_resolution") > 0) {
        LOG(INFO) << "Converting charge to ADC units, ADC resolution: " << config_.get<int>("adc_resolution")
                  << "bit, max. value " << ((1 << config_.get<int>("adc_resolution")) - 1);
    }

    if(config_.get<bool>("output_plots")) {
        LOG(TRACE) << "Creating output plots";

        // Plot axis are in kilo electrons - convert from framework units!
        int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
        auto nbins = config_.get<int>("output_plots_bins");

        // Create histograms if needed
        h_pxq = new TH1D("pixelcharge", "raw pixel charge;pixel charge [ke];pixels", nbins, 0, maximum);
        h_pxq_noise = new TH1D("pixelcharge_noise", "pixel charge w/ el. noise;pixel charge [ke];pixels", nbins, 0, maximum);
        h_gain = new TH1D("gain", "applied gain; gain factor;events", 40, -20, 20);
        h_pxq_gain =
            new TH1D("pixelcharge_gain", "pixel charge w/ gain applied;pixel charge [ke];pixels", nbins, 0, maximum);
        h_thr = new TH1D("threshold", "applied threshold; threshold [ke];events", maximum, 0, maximum / 10);
        h_pxq_thr =
            new TH1D("pixelcharge_threshold", "pixel charge above threshold;pixel charge [ke];pixels", nbins, 0, maximum);
        h_pxq_adc_smear = new TH1D(
            "pixelcharge_adc_smeared", "pixel charge after ADC smearing;pixel charge [ke];pixels", nbins, 0, maximum);

        // Create final pixel charge plot with different axis, depending on whether ADC simulation is enabled or not
        if(config_.get<int>("adc_resolution") > 0) {
            int adcbins = ((1 << config_.get<int>("adc_resolution")) - 1);
            h_pxq_adc = new TH1D("pixelcharge_adc", "pixel charge after ADC;pixel charge [ADC];pixels", adcbins, 0, adcbins);
            h_calibration = new TH2D("charge_adc_calibration",
                                     "calibration curve of pixel charge to ADC units;pixel charge [ke];pixel charge [ADC]",
                                     nbins,
                                     0,
                                     maximum,
                                     adcbins,
                                     0,
                                     adcbins);
        } else {
            h_pxq_adc = new TH1D("pixelcharge_adc", "final pixel charge;pixel charge [ke];pixels", nbins, 0, maximum);
        }
    }
}

void DefaultDigitizerModule::run(Event* event) const {
    auto pixel_message = event->fetchMessage<PixelChargeMessage>();

    // Loop through all pixels with charges
    std::vector<PixelHit> hits;
    for(auto& pixel_charge : pixel_message->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto pixel_index = pixel.getIndex();
        auto charge = static_cast<double>(pixel_charge.getCharge());

        LOG(DEBUG) << "Received pixel " << pixel_index << ", charge " << Units::display(charge, "e");
        if(config_.get<bool>("output_plots")) {
            h_pxq->Fill(charge / 1e3);
        }

        // Add electronics noise from Gaussian:
        std::normal_distribution<double> el_noise(0, config_.get<unsigned int>("electronics_noise"));
        charge += el_noise(event->getRandomEngine());

        LOG(DEBUG) << "Charge with noise: " << Units::display(charge, "e");
        if(config_.get<bool>("output_plots")) {
            h_pxq_noise->Fill(charge / 1e3);
        }

        // Smear the gain factor, Gaussian distribution around "gain" with width "gain_smearing"
        std::normal_distribution<double> gain_smearing(config_.get<double>("gain"), config_.get<double>("gain_smearing"));
        double gain = gain_smearing(event->getRandomEngine());
        if(config_.get<bool>("output_plots")) {
            h_gain->Fill(gain);
        }

        // Apply the gain to the charge:
        charge *= gain;
        LOG(DEBUG) << "Charge after amplifier (gain): " << Units::display(charge, "e");
        if(config_.get<bool>("output_plots")) {
            h_pxq_gain->Fill(charge / 1e3);
        }

        // Smear the threshold, Gaussian distribution around "threshold" with width "threshold_smearing"
        std::normal_distribution<double> thr_smearing(config_.get<unsigned int>("threshold"),
                                                      config_.get<unsigned int>("threshold_smearing"));
        double threshold = thr_smearing(event->getRandomEngine());
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

        // Simulate ADC if resolution set to more than 0bit
        if(config_.get<int>("adc_resolution") > 0) {
            // temporarily store old charge for histogramming:
            auto original_charge = charge;

            // Add ADC smearing:
            std::normal_distribution<double> adc_smearing(0, config_.get<unsigned int>("adc_smearing"));
            charge += adc_smearing(event->getRandomEngine());
            if(config_.get<bool>("output_plots")) {
                h_pxq_adc_smear->Fill(charge / 1e3);
            }
            LOG(DEBUG) << "Smeared for simulating limited ADC sensitivity: " << Units::display(charge, "e");

            // Convert to ADC units and precision:
            charge = static_cast<double>(std::max(
                std::min(static_cast<int>((config_.get<double>("adc_offset") + charge) / config_.get<double>("adc_slope")),
                         (1 << config_.get<int>("adc_resolution")) - 1),
                0));
            LOG(DEBUG) << "Charge converted to ADC units: " << charge;

            if(config_.get<bool>("output_plots")) {
                h_calibration->Fill(original_charge / 1e3, charge);
                h_pxq_adc->Fill(charge);
            }
        } else {
            // Fill the final pixel charge
            if(config_.get<bool>("output_plots")) {
                h_pxq_adc->Fill(charge / 1e3);
            }
        }

        // Add the hit to the hitmap
        hits.emplace_back(pixel, 0, charge, &pixel_charge);
    }

    // Output summary and update statistics
    LOG(INFO) << "Digitized " << hits.size() << " pixel hits";
    {
        std::lock_guard<std::mutex> lock{stats_mutex_};
        total_hits_ += hits.size();
    }

    if(!hits.empty()) {
        // Create and dispatch hit message
        auto hits_message = std::make_shared<PixelHitMessage>(std::move(hits), getDetector());
        event->dispatchMessage(hits_message);
    }
}

void DefaultDigitizerModule::finalize() {
    if(config_.get<bool>("output_plots")) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        h_pxq->Write();
        h_pxq_noise->Write();
        h_gain->Write();
        h_pxq_gain->Write();
        h_thr->Write();
        h_pxq_thr->Write();
        h_pxq_adc->Write();

        if(config_.get<int>("adc_resolution") > 0) {
            h_pxq_adc_smear->Write();
            h_calibration->Write();
        }
    }

    LOG(INFO) << "Digitized " << total_hits_ << " pixel hits in total";
}

// check if the required messages are present in the event
bool DefaultDigitizerModule::isSatisfied(Event* event) const {
    try {
        auto pixel_message = event->fetchMessage<PixelChargeMessage>();
    } catch (std::out_of_range&) {
        return false;
    }

    return true;
}

