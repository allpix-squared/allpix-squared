/**
 * @file
 * @brief Implementation of default digitization module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DefaultDigitizerModule.hpp"

#include "core/utils/unit.h"
#include "objects/PixelHit.hpp"
#include "tools/ROOT.h"

#include <TFile.h>
#include <TH1D.h>
#include <TProfile.h>

using namespace allpix;

DefaultDigitizerModule::DefaultDigitizerModule(Configuration& config,
                                               Messenger* messenger,
                                               std::shared_ptr<Detector> detector)
    : Module(config, std::move(detector)), messenger_(messenger) {
    // Enable parallelization of this module if multithreading is enabled
    enable_parallelization();

    // Require PixelCharge message for single detector
    messenger_->bindSingle<PixelChargeMessage>(this, MsgFlags::REQUIRED);

    config_.setAlias("qdc_resolution", "adc_resolution", true);
    config_.setAlias("qdc_smearing", "adc_smearing", true);
    config_.setAlias("qdc_offset", "adc_offset", true);
    config_.setAlias("qdc_slope", "adc_slope", true);
    config_.setAlias("allow_zero_qdc", "allow_zero_adc", true);

    // Set defaults for config variables
    config_.setDefault<int>("electronics_noise", Units::get(110, "e"));
    config_.setDefault<double>("gain", 1.0);
    config_.setDefault<double>("gain_smearing", 0.0);
    config_.setDefault<int>("threshold", Units::get(600, "e"));
    config_.setDefault<int>("threshold_smearing", Units::get(30, "e"));

    // QDC configuration
    config_.setDefault<int>("qdc_resolution", 0);
    config_.setDefault<int>("qdc_smearing", Units::get(300, "e"));
    config_.setDefault<double>("qdc_offset", Units::get(0, "e"));
    config_.setDefault<double>("qdc_slope", Units::get(10, "e"));
    config_.setDefault<bool>("allow_zero_qdc", false);

    // TDC configuration
    config_.setDefault<int>("tdc_resolution", 0);
    config_.setDefault<int>("tdc_smearing", Units::get(50, "ps"));
    config_.setDefault<double>("tdc_offset", Units::get(0, "ns"));
    config_.setDefault<double>("tdc_slope", Units::get(10, "ns"));
    config_.setDefault<bool>("allow_zero_tdc", false);

    // Plotting
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<int>("output_plots_scale", Units::get(30, "ke"));
    config_.setDefault<int>("output_plots_timescale", Units::get(300, "ns"));
    config_.setDefault<int>("output_plots_bins", 100);
}

void DefaultDigitizerModule::init() {
    // Conversion to ADC units requested:
    if(config_.get<int>("qdc_resolution") > 31) {
        throw InvalidValueError(config_, "qdc_resolution", "precision higher than 31bit is not possible");
    }
    if(config_.get<int>("tdc_resolution") > 31) {
        throw InvalidValueError(config_, "tdc_resolution", "precision higher than 31bit is not possible");
    }
    if(config_.get<int>("qdc_resolution") > 0) {
        LOG(INFO) << "Converting charge to QDC units, QDC resolution: " << config_.get<int>("qdc_resolution")
                  << "bit, max. value " << ((1 << config_.get<int>("qdc_resolution")) - 1);
    }
    if(config_.get<int>("tdc_resolution") > 0) {
        LOG(INFO) << "Converting time to TDC units, TDC resolution: " << config_.get<int>("tdc_resolution")
                  << "bit, max. value " << ((1 << config_.get<int>("tdc_resolution")) - 1);
    }

    if(config_.get<bool>("output_plots")) {
        LOG(TRACE) << "Creating output plots";

        // Plot axis are in kilo electrons - convert from framework units!
        int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
        auto nbins = config_.get<int>("output_plots_bins");

        // Create histograms if needed
        h_pxq = std::make_unique<ThreadedHistogram<TH1D>>(
            "pixelcharge", "raw pixel charge;pixel charge [ke];pixels", nbins, 0, maximum);
        h_pxq_noise = std::make_unique<ThreadedHistogram<TH1D>>(
            "pixelcharge_noise", "pixel charge w/ el. noise;pixel charge [ke];pixels", nbins, 0, maximum);
        h_gain = std::make_unique<ThreadedHistogram<TH1D>>("gain", "applied gain; gain factor;events", 40, -20, 20);
        h_pxq_gain = std::make_unique<ThreadedHistogram<TH1D>>(
            "pixelcharge_gain", "pixel charge w/ gain applied;pixel charge [ke];pixels", nbins, 0, maximum);
        h_thr = std::make_unique<ThreadedHistogram<TH1D>>(
            "threshold", "applied threshold; threshold [ke];events", maximum, 0, maximum / 10);
        h_pxq_thr = std::make_unique<ThreadedHistogram<TH1D>>(
            "pixelcharge_threshold", "pixel charge above threshold;pixel charge [ke];pixels", nbins, 0, maximum);

        // Create final pixel charge plot with different axis, depending on whether ADC simulation is enabled or not
        if(config_.get<int>("qdc_resolution") > 0) {
            h_pxq_adc_smear = std::make_unique<ThreadedHistogram<TH1D>>(
                "pixelcharge_adc_smeared", "pixel charge after ADC smearing;pixel charge [ke];pixels", nbins, 0, maximum);

            int adcbins = (1 << config_.get<int>("qdc_resolution"));
            h_pxq_adc = std::make_unique<ThreadedHistogram<TH1D>>(
                "pixelcharge_adc", "pixel charge after QDC;pixel charge [QDC];pixels", adcbins, 0, adcbins);
            h_calibration = std::make_unique<ThreadedHistogram<TH2D>>(
                "charge_adc_calibration",
                "calibration curve of pixel charge to QDC units;pixel charge [ke];pixel charge [QDC]",
                nbins,
                0,
                maximum,
                adcbins,
                0,
                adcbins);
        } else {
            h_pxq_adc = std::make_unique<ThreadedHistogram<TH1D>>(
                "pixelcharge_adc", "final pixel charge;pixel charge [ke];pixels", nbins, 0, maximum);
        }

        int time_maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_timescale"), "ns"));
        h_px_toa = std::make_unique<ThreadedHistogram<TH1D>>(
            "pixel_toa", "pixel time-of-arrival;pixel ToA [ns];pixels", nbins, 0, maximum);

        // Create time-of-arrival plot with different axis, depending on whether TDC simulation is enabled or not
        if(config_.get<int>("tdc_resolution") > 0) {
            h_px_tdc_smear =
                std::make_unique<ThreadedHistogram<TH1D>>("pixel_tdc_smeared",
                                                          "pixel time-of-arrival after TDC smearing;pixel ToA [ns];pixels",
                                                          nbins,
                                                          0,
                                                          time_maximum);

            int adcbins = (1 << config_.get<int>("tdc_resolution"));
            h_px_tdc = std::make_unique<ThreadedHistogram<TH1D>>(
                "pixel_tdc", "pixel time-of-arrival after TDC;pixel ToA [TDC];pixels", adcbins, 0, adcbins);
            h_toa_calibration = std::make_unique<ThreadedHistogram<TH2D>>(
                "tdc_calibration",
                "calibration curve of pixel time-of-arrival to TDC units;pixel ToA [ns];pixel ToA [TDC]",
                nbins,
                0,
                time_maximum,
                adcbins,
                0,
                adcbins);
        } else {
            h_px_tdc = std::make_unique<ThreadedHistogram<TH1D>>(
                "pixel_tdc", "final pixel time-of-arrival;pixel ToA [ns];pixels", nbins, 0, time_maximum);
        }
    }
}

void DefaultDigitizerModule::run(Event* event) {
    auto pixel_message = messenger_->fetchMessage<PixelChargeMessage>(this, event);

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

        // Simulate QDC if resolution set to more than 0bit
        if(config_.get<int>("qdc_resolution") > 0) {
            // temporarily store old charge for histogramming:
            auto original_charge = charge;

            // Add ADC smearing:
            std::normal_distribution<double> adc_smearing(0, config_.get<unsigned int>("qdc_smearing"));
            charge += adc_smearing(event->getRandomEngine());
            if(config_.get<bool>("output_plots")) {
                h_pxq_adc_smear->Fill(charge / 1e3);
            }
            LOG(DEBUG) << "Smeared for simulating limited QDC sensitivity: " << Units::display(charge, "e");

            // Convert to ADC units and precision, make sure ADC count is at least 1:
            charge = static_cast<double>(std::max(
                std::min(static_cast<int>((config_.get<double>("qdc_offset") + charge) / config_.get<double>("qdc_slope")),
                         (1 << config_.get<int>("qdc_resolution")) - 1),
                (config_.get<bool>("allow_zero_qdc") ? 0 : 1)));
            LOG(DEBUG) << "Charge converted to QDC units: " << charge;

            if(config_.get<bool>("output_plots")) {
                h_calibration->Fill(original_charge / 1e3, charge);
                h_pxq_adc->Fill(charge);
            }
        } else if(config_.get<bool>("output_plots")) {
            h_pxq_adc->Fill(charge / 1e3);
        }

        auto time = time_of_arrival(pixel_charge, threshold);
        LOG(DEBUG) << "Time of arrival: " << Units::display(time, {"ns", "ps"});
        if(config_.get<bool>("output_plots")) {
            h_px_toa->Fill(time);
        }

        // Simulate TDC if resolution set to more than 0bit
        if(config_.get<int>("tdc_resolution") > 0) {
            // temporarily store full arrival time for histogramming:
            auto original_time = time;

            // Add TDC smearing:
            std::normal_distribution<double> tdc_smearing(0, config_.get<unsigned int>("tdc_smearing"));
            time += tdc_smearing(event->getRandomEngine());
            if(config_.get<bool>("output_plots")) {
                h_px_tdc_smear->Fill(time);
            }
            LOG(DEBUG) << "Smeared for simulating limited TDC sensitivity: " << Units::display(time, {"ns", "ps"});

            // Convert to TDC units and precision, make sure TDC count is at least 1:
            time = static_cast<double>(std::max(
                std::min(static_cast<int>((config_.get<double>("tdc_offset") + time) / config_.get<double>("tdc_slope")),
                         (1 << config_.get<int>("tdc_resolution")) - 1),
                (config_.get<bool>("allow_zero_tdc") ? 0 : 1)));
            LOG(DEBUG) << "Time converted to TDC units: " << time;

            if(config_.get<bool>("output_plots")) {
                h_toa_calibration->Fill(original_time, time);
                h_px_tdc->Fill(time);
            }
        } else if(config_.get<bool>("output_plots")) {
            h_px_tdc->Fill(time);
        }

        // Add the hit to the hitmap
        hits.emplace_back(pixel, time, charge, &pixel_charge);
    }

    // Output summary and update statistics
    LOG(INFO) << "Digitized " << hits.size() << " pixel hits";
    total_hits_ += hits.size();

    if(!hits.empty()) {
        // Create and dispatch hit message
        auto hits_message = std::make_shared<PixelHitMessage>(std::move(hits), getDetector());
        messenger_->dispatchMessage(this, hits_message, event);
    }
}

double DefaultDigitizerModule::time_of_arrival(const PixelCharge& pixel_charge, double threshold) const {

    // If this PixelCharge has a pulse, we can find out when it crossed the threshold:
    const auto& pulse = pixel_charge.getPulse();
    if(pulse.isInitialized()) {
        auto charges = pulse.getPulse();
        std::vector<double>::iterator bin;
        double integrated_charge = 0;
        for(bin = charges.begin(); bin != charges.end(); bin++) {
            integrated_charge += *bin;
            if(integrated_charge >= threshold) {
                break;
            }
        }
        return pulse.getBinning() * static_cast<double>(std::distance(charges.begin(), bin));
    } else {
        LOG_ONCE(WARNING) << "Simulation chain does not allow for time-of-arrival calculation";
        return 0;
    }
}

void DefaultDigitizerModule::finalize() {
    if(config_.get<bool>("output_plots")) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";

        // Charge plots
        h_pxq->Write();
        h_pxq_noise->Write();
        h_gain->Write();
        h_pxq_gain->Write();
        h_thr->Write();
        h_pxq_thr->Write();

        h_pxq_adc->Write();
        if(config_.get<int>("qdc_resolution") > 0) {
            h_pxq_adc_smear->Write();
            h_calibration->Write();
        }

        // Time plots
        h_px_toa->Write();
        if(config_.get<int>("tdc_resolution") > 0) {
            h_px_tdc_smear->Write();
            h_toa_calibration->Write();
        }
        h_px_tdc->Write();
    }

    LOG(INFO) << "Digitized " << total_hits_ << " pixel hits in total";
}
