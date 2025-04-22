/**
 * @file
 * @brief Implementation of default digitization module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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

    if(config_.has("gain") && config_.has("gain_function")) {
        throw InvalidCombinationError(
            config_, {"gain", "gain_function"}, "Gain and Gain Function cannot be simultaneously configured.");
    }

    // Set defaults for config variables
    config_.setDefault<bool>("sample_all_channels", false);
    config_.setDefault<int>("electronics_noise", Units::get(110, "e"));

    if(!config_.has("gain_function")) {
        config_.setDefault<double>("gain", 1.0);
    }

    config_.setDefault<int>("threshold_smearing", Units::get(30, "e"));

    // QDC configuration
    config_.setDefault<int>("qdc_resolution", 0);
    config_.setDefault<int>("qdc_smearing", Units::get(0, "e"));
    config_.setDefault<double>("qdc_offset", Units::get(0, "e"));
    config_.setDefault<double>("qdc_slope", Units::get(10, "e"));
    config_.setDefault<bool>("allow_zero_qdc", false);

    // TDC configuration
    config_.setDefault<int>("tdc_resolution", 0);
    config_.setDefault<double>("tdc_smearing", Units::get(50.0, "ps"));
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
    sample_all_channels_ = config_.get<bool>("sample_all_channels");
    output_plots_ = config_.get<bool>("output_plots");

    electronics_noise_ = config_.get<unsigned int>("electronics_noise");

    if(config_.has("gain_function")) {
        gain_function_ = std::make_unique<TFormula>("gain_function", (config_.get<std::string>("gain_function")).c_str());

        if(!gain_function_->IsValid()) {
            throw InvalidValueError(
                config_, "gain_function", "The response function is not a valid ROOT::TFormula expression.");
        }

        auto parameters = config_.getArray<double>("gain_parameters");

        // check if number of parameters match up
        if(static_cast<size_t>(gain_function_->GetNpar()) != parameters.size()) {
            throw InvalidValueError(
                config_,
                "gain_parameters",
                "The number of function parameters does not line up with the number of parameters in the function.");
        }

        for(size_t n = 0; n < parameters.size(); ++n) {
            gain_function_->SetParameter(static_cast<int>(n), parameters[n]);
        }

        LOG(DEBUG) << "Gain response function successfully initialized with " << parameters.size() << " parameters";
    } else {
        gain_function_ = std::make_unique<TFormula>("gain_function", "[0]*x");
        gain_function_->SetParameter(0, config_.get<double>("gain"));
    }

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
    tdc_smearing_ = config_.get<double>("tdc_smearing");
    tdc_offset_ = config_.get<double>("tdc_offset");
    tdc_slope_ = config_.get<double>("tdc_slope");
    allow_zero_tdc_ = config_.get<bool>("allow_zero_tdc");

    // Require PixelCharge message for single detector if we sample only channels with signal, otherwise drop "REQUIRED" flag
    messenger_->bindSingle<PixelChargeMessage>(this, sample_all_channels_ ? MsgFlags::NONE : MsgFlags::REQUIRED);
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
        h_thr = CreateHistogram<TH1D>(
            "threshold", "applied threshold; threshold [ke];events", static_cast<int>(maximum), 0, maximum / 10);
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

    // We might not have a pixel charge message available:
    std::shared_ptr<PixelChargeMessage> pixel_message{nullptr};

    try {
        pixel_message = messenger_->fetchMessage<PixelChargeMessage>(this, event);
    } catch(const MessageNotFoundException&) {
    }

    // Ensure to not copy the data but to obtain only a reference - both returns from ternary operator must be references!
    const std::vector<PixelCharge>& dummy = std::vector<PixelCharge>();
    const auto& pixel_charges = (pixel_message ? pixel_message->getData() : dummy);

    // Select what to iterate over:
    std::set<Pixel::Index> pixels;
    if(sample_all_channels_) {
        // Loop through all pixels of the matrix
        pixels = getDetector()->getModel()->getPixels();
    } else {
        // Loop only over pixels with a PixelCharge entry
        for(const auto& px : pixel_charges) {
            pixels.emplace(px.getIndex());
        }
    }

    std::vector<PixelHit> hits;
    // Loop over selected channels:
    for(const auto& index : pixels) {

        const auto it = std::find_if(
            pixel_charges.begin(), pixel_charges.end(), [index](const auto& px) { return px.getIndex() == index; });
        const auto& pixel_charge = (it == pixel_charges.end() ? PixelCharge(getDetector()->getPixel(index), 0.) : *it);
        auto charge = static_cast<double>(pixel_charge.getAbsoluteCharge());

        LOG(DEBUG) << "Received pixel " << pixel_charge.getIndex() << ", (absolute) charge " << Units::display(charge, "e");
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

        // Apply the gain to the charge:
        auto charge_pregain = charge;
        charge = gain_function_->Eval(charge);
        LOG(DEBUG) << "Charge after amplifier (gain): " << Units::display(charge, "e");
        if(output_plots_) {
            // Calculate gain from pre- and post-charge, offset to avoid zero-division:
            h_gain->Fill(charge / (charge_pregain + std::numeric_limits<double>::epsilon()));
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
        LOG(DEBUG) << "Time of arrival: " << Units::display(time, {"ns", "ps"}) << " (local), "
                   << Units::display(pixel_charge.getGlobalTime() + time, {"ns", "ps"}) << " (global)";
        if(output_plots_) {
            h_px_toa->Fill(time);
        }

        // Store full arrival time for global timestamp and histogramming:
        auto original_time = time;

        // Simulate TDC if resolution set to more than 0bit
        if(tdc_resolution_ > 0) {
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

        // Add the hit to the hitmap. Use the address of the iterator for PixelCharge reference instead of the object since
        // the latter is a copy.
        hits.emplace_back(pixel_charge.getPixel(),
                          time,
                          pixel_charge.getGlobalTime() + original_time,
                          charge,
                          (it == pixel_charges.end() ? nullptr : &(*it)));
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
        auto bin = pulse.begin();
        double integrated_charge = 0;
        for(; bin != pulse.end(); bin++) {
            integrated_charge += *bin;
            if(std::abs(integrated_charge) >= threshold) {
                break;
            }
        }
        return pulse.getBinning() * static_cast<double>(std::distance(pulse.begin(), bin));
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
