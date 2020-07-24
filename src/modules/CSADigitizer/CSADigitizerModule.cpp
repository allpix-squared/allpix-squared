/**
 * @file
 * @brief Implementation of charge sensitive amplifier digitization module
 * @copyright Copyright (c) 2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "CSADigitizerModule.hpp"

#include "core/utils/unit.h"
#include "tools/ROOT.h"

#include <TFile.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>

#include "objects/PixelHit.hpp"

using namespace allpix;

CSADigitizerModule::CSADigitizerModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(config, std::move(detector)), messenger_(messenger), pixel_message_(nullptr) {
    // Enable parallelization of this module if multithreading is enabled
    enable_parallelization();

    // Require PixelCharge message for single detector
    messenger_->bindSingle(this, &CSADigitizerModule::pixel_message_, MsgFlags::REQUIRED);

    // Seed the random generator with the global seed
    random_generator_.seed(getRandomSeed());

    // Read model
    auto model = config_.get<std::string>("model");
    std::transform(model.begin(), model.end(), model.begin(), ::tolower);
    if(model == "simple") {
        model_ = DigitizerType::SIMPLE;
    } else if(model == "csa") {
        model_ = DigitizerType::CSA;
    } else {
        throw InvalidValueError(config_, "model", "Invalid model, only 'simple' and 'csa' are supported.");
    }

    // Set defaults for config variables
    config_.setDefault<double>("feedback_capacitance", Units::get(5e-15, "C/V"));

    config_.setDefault<double>("integration_time", Units::get(0.5e-6, "s"));
    config_.setDefault<double>("threshold", Units::get(10e-3, "V"));
    config_.setDefault<double>("sigma_noise", Units::get(0.1e-3, "V"));
    config_.setDefault<double>("clock_bin_toa", Units::get(1.5625, "ns"));
    config_.setDefault<double>("clock_bin_tot", Units::get(25.0, "ns"));

    config_.setDefault<bool>("output_pulsegraphs", false);
    config_.setDefault<bool>("output_plots", config_.get<bool>("output_pulsegraphs"));
    config_.setDefault<int>("output_plots_scale", Units::get(30, "ke"));
    config_.setDefault<int>("output_plots_bins", 100);

    config_.setDefault<bool>("output_tot", true);

    if(model_ == DigitizerType::SIMPLE) {
        // defaults for the "simple" parametrisation
        config_.setDefault<double>("rise_time_constant", Units::get(1e-9, "s"));
        config_.setDefault<double>("feedback_time_constant", Units::get(10e-9, "s")); // R_f * C_f
    } else if(model_ == DigitizerType::CSA) {
        // and for the "advanced" csa
        config_.setDefault<double>("krummenacher_current", Units::get(20e-9, "C/s"));
        config_.setDefault<double>("detector_capacitance", Units::get(100e-15, "C/V"));
        config_.setDefault<double>("amp_output_capacitance", Units::get(20e-15, "C/V"));
        config_.setDefault<double>("transconductance", Units::get(50e-6, "C/s/V"));
        config_.setDefault<double>("temperature", 293.15);
    }

    // Copy some variables from configuration to avoid lookups:
    tmax_ = config_.get<double>("integration_time");
    clockToA_ = config_.get<double>("clock_bin_toa");
    clockToT_ = config_.get<double>("clock_bin_tot");
    if(model_ == DigitizerType::SIMPLE) {
        tauF_ = config_.get<double>("feedback_time_constant");
        tauR_ = config_.get<double>("rise_time_constant");
        cf_ = config_.get<double>("feedback_capacitance");
        rf_ = tauF_ / cf_;
        LOG(DEBUG) << "Parameters: cf " << Units::display(cf_ / 1e-15, "C/V") << ", rf " << Units::display(rf_, "V*s/C")
                   << ", tauF_ " << Units::display(tauF_, "s") << ", tauR_ " << Units::display(tauR_, "s");
    } else if(model_ == DigitizerType::CSA) {
        ikrum_ = config_.get<double>("krummenacher_current");
        cd_ = config_.get<double>("detector_capacitance");
        cf_ = config_.get<double>("feedback_capacitance");
        co_ = config_.get<double>("amp_output_capacitance");
        gm_ = config_.get<double>("transconductance");
        vt_ = config_.get<double>("temperature") * 8.617333262145e-11; // Boltzmann kT in MeV

        // helper variables: transconductance and resistance in the feedback loop
        // weak inversion: gf = I/(n V_t) (e.g. Binkley "Tradeoff and Optimisation in Analog CMOS design")
        // n is the weak inversion slope factor (degradation of exponential MOS drain current compared to bipolar transistor
        // collector current) n_wi typically 1.5, for circuit descriped in  Kleczek 2016 JINST11 C12001: I->I_krumm/2
        gf_ = ikrum_ / (2.0 * 1.5 * vt_);
        rf_ = 2. / gf_; // feedback resistor
        tauF_ = rf_ * cf_;
        tauR_ = (cd_ * co_) / (gm_ * cf_);
        LOG(DEBUG) << "Parameters: rf " << Units::display(rf_, "V*s/C") << ", cf_ " << Units::display(cf_, "C/V") << ", cd_ "
                   << Units::display(cd_, "C/V") << ", co_ " << Units::display(co_, "C/V") << ", gm_ "
                   << Units::display(gm_, "C/s/V") << ", tauF_ " << Units::display(tauF_, "s") << ", tauR_ "
                   << Units::display(tauR_, "s") << ", temperature " << config_.get<double>("temperature") << "K";
    }

    output_plots_ = config_.get<bool>("output_plots");
    output_pulsegraphs_ = config_.get<bool>("output_pulsegraphs");

    // impulse response of the CSA from Kleczek 2016 JINST11 C12001
    // H(s) = Rf / ((1+ tau_f s) * (1 + tau_r s)), with
    // tau_f = Rf Cf , rise time constant tau_r = (C_det * C_out) / ( gm_ * C_F )
    // inverse Laplace transform R/((1+a*s)*(1+s*b)) is (wolfram alpha) (R (e^(-t/a) - e^(-t/b))) /(a - b)
    fImpulseResponse_ = new TF1("fImpulseResponse_", "[0] * ( exp(-x / [1]) - exp(-x/[2]) ) / ([1]-[2])", 0, tmax_);
    fImpulseResponse_->SetParameters(rf_, tauF_, tauR_);
}

void CSADigitizerModule::init() {

    if(output_plots_) {
        LOG(TRACE) << "Creating output plots";

        // Plot axis are in kilo electrons - convert from framework units!
        int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
        auto nbins = config_.get<int>("output_plots_bins");

        // Create histograms if needed
        h_pxq = new TH1D("pixelcharge", "raw pixel charge;pixel charge [ke];pixels", nbins, 0, maximum);
        h_tot = new TH1D("tot", "time over threshold;time over threshold [ns];pixels", nbins, 0, tmax_);
        h_toa = new TH1D("toa", "time of arrival;time of arrival [ns];pixels", nbins, 0, tmax_);
        h_pxq_vs_tot =
            new TH2D("pxqvstot", "ToT vs raw pixel charge;pixel charge [ke];ToT [ns]", nbins, 0, maximum, nbins, 0, tmax_);
    }
}

void CSADigitizerModule::run(unsigned int event_num) {
    // Loop through all pixels with charges
    std::vector<PixelHit> hits;
    for(auto& pixel_charge : pixel_message_->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto pixel_index = pixel.getIndex();
        auto inputcharge = static_cast<double>(pixel_charge.getCharge());

        LOG(DEBUG) << "Received pixel " << pixel_index << ", charge " << Units::display(inputcharge, "e");

        const auto& pulse = pixel_charge.getPulse(); // the pulse containing charges and times
        auto pulse_vec = pulse.getPulse();           // the vector of the charges
        auto timestep = pulse.getBinning();
        const int npx = static_cast<int>(ceil(tmax_ / timestep));

        if(first_event_) { // initialize impulse response function - assume all time bins are equal
            impulseResponse_.reserve(npx);
            for(int ipx = 0; ipx < npx; ++ipx) {
                impulseResponse_.push_back(fImpulseResponse_->Eval(ipx * timestep));
            }
            first_event_ = false;
            LOG(TRACE) << "impulse response initalised. timestep  : " << timestep << ", tmax_ : " << tmax_ << ", npx "
                       << npx;
        }

        std::vector<double> output_vec(npx);
        auto input_length = pulse_vec.size();
        LOG(TRACE) << "Preparing pulse for pixel " << pixel_index << ", " << pulse_vec.size() << " bins of "
                   << Units::display(timestep, {"ps", "ns"}) << ", total charge: " << Units::display(pulse.getCharge(), "e");
        // convolution of the pulse (size input_length) with the impulse response (size npx)
        for(unsigned long int k = 0; k < npx; ++k) {
            for(unsigned long int i = 0; i <= k; ++i) {
                if((k - i) < input_length) {
                    output_vec.at(k) += pulse_vec.at(k - i) * impulseResponse_.at(i) * 1e9;
                    // time = timestep * static_cast<double>(k);
                }
            }
        }
        auto output_integral = std::accumulate(output_vec.begin(), output_vec.end(), 0.0);
        LOG(TRACE) << "amplified signal without noise " << output_integral << " mV in a pulse with " << output_vec.size()
                   << "bins";

        // apply noise on the amplified pulse
        // watch out - output_vec and output_with_noise should be in mV
        std::normal_distribution<double> pulse_smearing(0, config_.get<double>("sigma_noise"));
        std::vector<double> output_with_noise(output_vec.size());
        std::transform(output_vec.begin(), output_vec.end(), output_with_noise.begin(), [&pulse_smearing, this](auto& c) {
            return c + pulse_smearing(random_generator_) * 1e9;
        });

        // re-calculate pulse integral with the noise
        output_integral = std::accumulate(output_with_noise.begin(), output_with_noise.end(), 0.0);

        // TOA and TOT logic
        // to emulate e.g. Timepix3: fine ToA clock (e.g 640MHz) and coarse clock (e.g. 40MHz) also for ToT
        auto threshold_in_mV = config_.get<double>("threshold") * 1e9;
        bool is_over_threshold = false;
        double toa{}, tot{};
        // first find the point where the signal crosses the threshold, latch toa
        for(int i = 0; i * clockToA_ < tmax_; ++i) {
            auto index = static_cast<int>(floor(i * clockToA_ / timestep));
            if(output_vec[index] > threshold_in_mV) {
                is_over_threshold = true;
                toa = i * clockToA_;
                break;
            };
        }
        // only look for ToT if the threshold was crossed in the ToA loop
        if(is_over_threshold) {
            // start from the next tot clock cycle following toa
            for(int j = static_cast<int>(ceil(toa / clockToT_)); j * clockToT_ < tmax_; ++j) {
                auto index = static_cast<int>(floor(j * clockToT_ / timestep));
                if(output_vec[index] > threshold_in_mV) {
                    tot += clockToT_;
                } else {
                    is_over_threshold = false;
                    break;
                }
            }
        }
        LOG(INFO) << "TOA " << toa << " ns, TOT " << tot << " ns, pulse sum (with noise) " << output_integral << " mV";

        if(config_.get<bool>("output_plots")) {
            h_pxq->Fill(inputcharge / 1e3);
            h_tot->Fill(tot);
            h_toa->Fill(toa);
            h_pxq_vs_tot->Fill(inputcharge / 1e3, tot);
        }

        // Fill a graphs with the individual pixel pulses:
        if(output_pulsegraphs_) {
            // Generate x-axis:
            std::vector<double> time(pulse_vec.size());
            // clang-format off
            std::generate(time.begin(), time.end(), [n = 0.0, timestep]() mutable {  auto now = n; n += timestep; return now; });
            // clang-format on

            std::string name = "pulse_ev" + std::to_string(event_num) + "_px" + std::to_string(pixel_index.x()) + "-" +
                               std::to_string(pixel_index.y());
            auto pulse_graph = new TGraph(static_cast<int>(pulse_vec.size()), &time[0], &pulse_vec[0]);
            pulse_graph->GetXaxis()->SetTitle("t [ns]");
            pulse_graph->GetYaxis()->SetTitle("Q_{ind} [e]");
            pulse_graph->SetTitle(("Induced charge in pixel (" + std::to_string(pixel_index.x()) + "," +
                                   std::to_string(pixel_index.y()) + "), Q_{tot} = " + std::to_string(pulse.getCharge()) +
                                   " e")
                                      .c_str());
            getROOTDirectory()->WriteTObject(pulse_graph, name.c_str());

            // Generate graphs of integrated charge over time:
            std::vector<double> charge_vec;
            double charge = 0;
            for(const auto& bin : pulse_vec) {
                charge += bin;
                charge_vec.push_back(charge);
            }

            name = "charge_ev" + std::to_string(event_num) + "_px" + std::to_string(pixel_index.x()) + "-" +
                   std::to_string(pixel_index.y());
            auto charge_graph = new TGraph(static_cast<int>(charge_vec.size()), &time[0], &charge_vec[0]);
            charge_graph->GetXaxis()->SetTitle("t [ns]");
            charge_graph->GetYaxis()->SetTitle("Q_{tot} [e]");
            charge_graph->SetTitle(("Accumulated induced charge in pixel (" + std::to_string(pixel_index.x()) + "," +
                                    std::to_string(pixel_index.y()) + "), Q_{tot} = " + std::to_string(pulse.getCharge()) +
                                    " e")
                                       .c_str());
            getROOTDirectory()->WriteTObject(charge_graph, name.c_str());

            // -------- now the same for the amplified (and shaped) pulses
            std::vector<double> amptime(output_vec.size());
            // clang-format off
            std::generate(amptime.begin(), amptime.end(), [n = 0.0, timestep]() mutable {  auto now = n; n += timestep; return now; });
            // clang-format on

            name = "output_ev" + std::to_string(event_num) + "_px" + std::to_string(pixel_index.x()) + "-" +
                   std::to_string(pixel_index.y());
            auto output_graph = new TGraph(static_cast<int>(output_vec.size()), &amptime[0], &output_vec[0]);
            output_graph->GetXaxis()->SetTitle("t [ns]");
            output_graph->GetYaxis()->SetTitle("CSA output [mV]");
            output_graph->SetTitle(("Amplifier signal in pixel (" + std::to_string(pixel_index.x()) + "," +
                                    std::to_string(pixel_index.y()) + ")")
                                       .c_str());
            getROOTDirectory()->WriteTObject(output_graph, name.c_str());

            // -------- now the same for the amplified (and shaped) pulses with noise

            name = "output_with_noise_ev" + std::to_string(event_num) + "_px" + std::to_string(pixel_index.x()) + "-" +
                   std::to_string(pixel_index.y());
            auto output_with_noise_graph =
                new TGraph(static_cast<int>(output_with_noise.size()), &amptime[0], &output_with_noise[0]);
            output_with_noise_graph->GetXaxis()->SetTitle("t [ns]");
            output_with_noise_graph->GetYaxis()->SetTitle("CSA output [mV]");
            output_with_noise_graph->SetTitle(("Amplifier signal with added noise in pixel (" +
                                               std::to_string(pixel_index.x()) + "," + std::to_string(pixel_index.y()) + ")")
                                                  .c_str());
            getROOTDirectory()->WriteTObject(output_with_noise_graph, name.c_str());
        }

        // Add the hit to the hitmap
        if(config_.get<bool>("output_tot")) {
            hits.emplace_back(pixel, toa, tot, &pixel_charge);
        } else {
            hits.emplace_back(pixel, toa, output_integral, &pixel_charge);
        }
    }

    // Output summary and update statistics
    LOG(INFO) << "Digitized " << hits.size() << " pixel hits";

    if(!hits.empty()) {
        // Create and dispatch hit message
        auto hits_message = std::make_shared<PixelHitMessage>(std::move(hits), getDetector());
        messenger_->dispatchMessage(this, hits_message);
    }
}

void CSADigitizerModule::finalize() {
    if(config_.get<bool>("output_plots")) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        h_pxq->Write();
        h_tot->Write();
        h_toa->Write();
        h_pxq_vs_tot->Write();
    }
}
