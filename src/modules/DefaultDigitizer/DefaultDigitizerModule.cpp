/**
 * @file
 * @brief Implementation of default digitization module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DefaultDigitizerModule.hpp"

#include "core/utils/distributions.h"
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
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Require PixelCharge message for single detector
    messenger_->bindSingle<PixelChargeMessage>(this, MsgFlags::REQUIRED);

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

    // Simple front-end saturation
    config_.setDefault<bool>("saturation", false);
    config_.setDefault<int>("saturation_mean", Units::get(190, "ke"));
    config_.setDefault<int>("saturation_width", Units::get(20, "ke"));

    // Plotting
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<int>("output_plots_scale", Units::get(30, "ke"));
    config_.setDefault<int>("output_plots_timescale", Units::get(300, "ns"));
    config_.setDefault<int>("output_plots_bins", 100);

    // Cache config parameters
    output_plots_ = config_.get<bool>("output_plots");

    electronics_noise_ = config_.get<unsigned int>("electronics_noise");
    gain_ = config_.get<double>("gain");
    gain_smearing_ = config_.get<double>("gain_smearing");

    saturation_ = config_.get<bool>("saturation");
    saturation_mean_ = config_.get<unsigned int>("saturation_mean");
    saturation_width_ = config_.get<unsigned int>("saturation_width");

    threshold_ = config_.get<unsigned int>("threshold");
    threshold_smearing_ = config_.get<unsigned int>("threshold_smearing");

    qdc_resolution_ = config_.get<int>("qdc_resolution");
    qdc_smearing_ = config_.get<unsigned int>("qdc_smearing");
    qdc_offset_ = config_.get<double>("qdc_offset");
    qdc_slope_ = config_.get<double>("qdc_slope");
    allow_zero_qdc_ = config_.get<bool>("allow_zero_qdc");

    tdc_resolution_ = config_.get<int>("tdc_resolution");
    tdc_smearing_ = config_.get<unsigned int>("tdc_smearing");
    tdc_offset_ = config_.get<double>("tdc_offset");
    tdc_slope_ = config_.get<double>("tdc_slope");
    allow_zero_tdc_ = config_.get<bool>("allow_zero_tdc");
}

void DefaultDigitizerModule::initialize() {
    // Conversion to ADC units requested:
    if(qdc_resolution_ > 31) {
        throw InvalidValueError(config_, "qdc_resolution", "precision higher than 31bit is not possible");
    }
    if(tdc_resolution_ > 31) {
        throw InvalidValueError(config_, "tdc_resolution", "precision higher than 31bit is not possible");
    }
    if(qdc_resolution_ > 0) {
        LOG(INFO) << "Converting charge to QDC units, QDC resolution: " << qdc_resolution_ << "bit, max. value "
                  << ((1 << qdc_resolution_) - 1);
    }
    if(tdc_resolution_ > 0) {
        LOG(INFO) << "Converting time to TDC units, TDC resolution: " << tdc_resolution_ << "bit, max. value "
                  << ((1 << tdc_resolution_) - 1);
    }

    if(output_plots_) {
        LOG(TRACE) << "Creating output plots";

        // Plot axis are in kilo electrons - convert from framework units!
        auto maximum = static_cast<double>(Units::convert(config_.get<double>("output_plots_scale"), "ke"));
        auto nbins = config_.get<int>("output_plots_bins");

        // Create histograms if needed
        h_pxq = CreateHistogram<TH1D>("pixelcharge", "raw pixel charge;pixel charge [ke];pixels", nbins, 0, maximum);
        h_pxq_noise = CreateHistogram<TH1D>(
            "pixelcharge_noise", "pixel charge w/ el. noise;pixel charge [ke];pixels", nbins, 0, maximum);
        h_gain = CreateHistogram<TH1D>("gain", "applied gain; gain factor;events", 40, -20, 20);
        h_pxq_gain = CreateHistogram<TH1D>(
            "pixelcharge_gain", "pixel charge w/ gain applied;pixel charge [ke];pixels", nbins, 0, maximum);
        h_thr = CreateHistogram<TH1D>("threshold", "applied threshold; threshold [ke];events", maximum, 0, maximum / 10);
        h_pxq_sat = CreateHistogram<TH1D>(
            "pixelcharge_saturation", "pixel charge with front-end saturation;pixel charge [ke];pixels", nbins, 0, maximum);
        h_pxq_thr = CreateHistogram<TH1D>(
            "pixelcharge_threshold", "pixel charge above threshold;pixel charge [ke];pixels", nbins, 0, maximum);

        // Create final pixel charge plot with different axis, depending on whether ADC simulation is enabled or not
        if(qdc_resolution_ > 0) {
            h_pxq_adc_smear = CreateHistogram<TH1D>(
                "pixelcharge_adc_smeared", "pixel charge after ADC smearing;pixel charge [ke];pixels", nbins, 0, maximum);

            int adcbins = (1 << qdc_resolution_);
            h_pxq_adc = CreateHistogram<TH1D>(
                "pixelcharge_adc", "pixel charge after QDC;pixel charge [QDC];pixels", adcbins, 0, adcbins);
            h_calibration =
                CreateHistogram<TH2D>("charge_adc_calibration",
                                      "calibration curve of pixel charge to QDC units;pixel charge [ke];pixel charge [QDC]",
                                      nbins,
                                      0,
                                      maximum,
                                      adcbins,
                                      0,
                                      adcbins);
        } else {
            h_pxq_adc =
                CreateHistogram<TH1D>("pixelcharge_adc", "final pixel charge;pixel charge [ke];pixels", nbins, 0, maximum);
        }

        auto time_maximum = static_cast<double>(Units::convert(config_.get<double>("output_plots_timescale"), "ns"));
        h_px_toa = CreateHistogram<TH1D>("pixel_toa", "pixel time-of-arrival;pixel ToA [ns];pixels", nbins, 0, time_maximum);

        // Create time-of-arrival plot with different axis, depending on whether TDC simulation is enabled or not
        if(tdc_resolution_ > 0) {
            h_px_tdc_smear = CreateHistogram<TH1D>("pixel_tdc_smeared",
                                                   "pixel time-of-arrival after TDC smearing;pixel ToA [ns];pixels",
                                                   nbins,
                                                   0,
                                                   time_maximum);

            int adcbins = (1 << tdc_resolution_);
            h_px_tdc = CreateHistogram<TH1D>(
                "pixel_tdc", "pixel time-of-arrival after TDC;pixel ToA [TDC];pixels", adcbins, 0, adcbins);
            h_toa_calibration = CreateHistogram<TH2D>(
                "tdc_calibration",
                "calibration curve of pixel time-of-arrival to TDC units;pixel ToA [ns];pixel ToA [TDC]",
                nbins,
                0,
                time_maximum,
                adcbins,
                0,
                adcbins);
        } else {
            h_px_tdc = CreateHistogram<TH1D>(
                "pixel_tdc", "final pixel time-of-arrival;pixel ToA [ns];pixels", nbins, 0, time_maximum);
        }
    }
}

void DefaultDigitizerModule::run(Event* event) {
    auto pixel_message = messenger_->fetchMessage<PixelChargeMessage>(this, event);

    // Loop through all pixels with charges
    std::vector<PixelHit> hits;
    for(const auto& pixel_charge : pixel_message->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto pixel_index = pixel.getIndex();
        auto charge = static_cast<double>(pixel_charge.getAbsoluteCharge());

        LOG(DEBUG) << "Received pixel " << pixel_index << ", (absolute) charge " << Units::display(charge, "e");
        if(output_plots_) {
            h_pxq->Fill(charge / 1e3);
        }

        // Add electronics noise from Gaussian:
        allpix::normal_distribution<double> el_noise(0, electronics_noise_);
        charge += el_noise(event->getRandomEngine());

        LOG(DEBUG) << "Charge with noise: " << Units::display(charge, "e");
        if(output_plots_) {
            h_pxq_noise->Fill(charge / 1e3);
        }

        // Smear the gain factor, Gaussian distribution around "gain" with width "gain_smearing"
        allpix::normal_distribution<double> gain_smearing(gain_, gain_smearing_);
        double gain = gain_smearing(event->getRandomEngine());
        if(output_plots_) {
            h_gain->Fill(gain);
        }

        // Apply the gain to the charge:
        charge *= gain;
        LOG(DEBUG) << "Charge after amplifier (gain): " << Units::display(charge, "e");
        if(output_plots_) {
            h_pxq_gain->Fill(charge / 1e3);
        }

        // Simulate simple front-end saturation if enabled:
        if(saturation_) {
            allpix::normal_distribution<double> saturation_smearing(saturation_mean_, saturation_width_);
            auto saturation = saturation_smearing(event->getRandomEngine());
            if(charge > saturation) {
                LOG(DEBUG) << "Above front-end saturation, " << Units::display(charge, {"e", "ke"}) << " > "
                           << Units::display(saturation, {"e", "ke"}) << ", setting to saturation value";
                charge = saturation;
            }
        }

        if(output_plots_) {
            h_pxq_sat->Fill(charge / 1e3);
        }

        // Smear the threshold, Gaussian distribution around "threshold" with width "threshold_smearing"
        allpix::normal_distribution<double> thr_smearing(threshold_, threshold_smearing_);
        double threshold = thr_smearing(event->getRandomEngine());
        if(output_plots_) {
            h_thr->Fill(threshold / 1e3);
        }

        // Discard charges below threshold:
        if(charge < threshold) {
            LOG(DEBUG) << "Below smeared threshold: " << Units::display(charge, "e") << " < "
                       << Units::display(threshold, "e");
            continue;
        }

        LOG(DEBUG) << "Passed threshold: " << Units::display(charge, "e") << " > " << Units::display(threshold, "e");
        if(output_plots_) {
            h_pxq_thr->Fill(charge / 1e3);
        }

        // Simulate QDC if resolution set to more than 0bit
        if(qdc_resolution_ > 0) {
            // temporarily store old charge for histogramming:
            auto original_charge = charge;

            // Add ADC smearing:
            allpix::normal_distribution<double> adc_smearing(0, qdc_smearing_);
            charge += adc_smearing(event->getRandomEngine());
            if(output_plots_) {
                h_pxq_adc_smear->Fill(charge / 1e3);
            }
            LOG(DEBUG) << "Smeared for simulating limited QDC sensitivity: " << Units::display(charge, "e");

            // Convert to ADC units and precision, make sure ADC count is at least 1:
            charge = static_cast<double>(std::clamp(static_cast<int>((qdc_offset_ + charge) / qdc_slope_),
                                                    (allow_zero_qdc_ ? 0 : 1),
                                                    (1 << qdc_resolution_) - 1));
            LOG(DEBUG) << "Charge converted to QDC units: " << charge;

            if(output_plots_) {
                h_calibration->Fill(original_charge / 1e3, charge);
                h_pxq_adc->Fill(charge);
            }
        } else if(output_plots_) {
            h_pxq_adc->Fill(charge / 1e3);
        }

        auto time = time_of_arrival(pixel_charge, threshold);
        LOG(DEBUG) << "Local time of arrival: " << Units::display(time, {"ns", "ps"});
        if(output_plots_) {
            h_px_toa->Fill(time);
        }

        // Simulate TDC if resolution set to more than 0bit
        if(tdc_resolution_ > 0) {
            // temporarily store full arrival time for histogramming:
            auto original_time = time;

            // Add TDC smearing:
            allpix::normal_distribution<double> tdc_smearing(0, tdc_smearing_);
            time += tdc_smearing(event->getRandomEngine());
            if(output_plots_) {
                h_px_tdc_smear->Fill(time);
            }
            LOG(DEBUG) << "Smeared for simulating limited TDC sensitivity: " << Units::display(time, {"ns", "ps"});

            // Convert to TDC units and precision, make sure TDC count is at least 1:
            time = static_cast<double>(std::clamp(
                static_cast<int>((tdc_offset_ + time) / tdc_slope_), (allow_zero_tdc_ ? 0 : 1), (1 << tdc_resolution_) - 1));
            LOG(DEBUG) << "Time converted to TDC units: " << time;

            if(output_plots_) {
                h_toa_calibration->Fill(original_time, time);
                h_px_tdc->Fill(time);
            }
        } else if(output_plots_) {
            h_px_tdc->Fill(time);
        }

        // Add the hit to the hitmap
        hits.emplace_back(pixel, time, pixel_charge.getGlobalTime() + time, charge, &pixel_charge);
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
            if(std::abs(integrated_charge) >= threshold) {
                break;
            }
        }
        return pulse.getBinning() * static_cast<double>(std::distance(charges.begin(), bin));
    } else {
        LOG_ONCE(INFO) << "Simulation chain does not allow for time-of-arrival calculation";
        return 0;
    }
}

void DefaultDigitizerModule::finalize() {
    if(output_plots_) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";

        // Charge plots
        h_pxq->Write();
        h_pxq_noise->Write();
        h_gain->Write();
        h_pxq_gain->Write();
        h_thr->Write();
        h_pxq_sat->Write();
        h_pxq_thr->Write();

        h_pxq_adc->Write();
        if(qdc_resolution_ > 0) {
            h_pxq_adc_smear->Write();
            h_calibration->Write();
        }

        // Time plots
        h_px_toa->Write();
        if(tdc_resolution_ > 0) {
            h_px_tdc_smear->Write();
            h_toa_calibration->Write();
        }
        h_px_tdc->Write();
    }

    LOG(INFO) << "Digitized " << total_hits_ << " pixel hits in total";
}
