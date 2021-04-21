/**
 * @file
 * @brief Implementation of generic charge propagation module
 * @remarks Based on code from Paul Schuetze
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "GenericPropagationModule.hpp"

#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <utility>

#include <Eigen/Core>

#include <Math/Point3D.h>
#include <Math/Vector3D.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH2F.h>
#include <TH3F.h>
#include <TPaveText.h>
#include <TPolyLine3D.h>
#include <TPolyMarker3D.h>
#include <TStyle.h>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "tools/ROOT.h"
#include "tools/runge_kutta.h"

#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

using namespace allpix;

/**
 * Besides binding the message and setting defaults for the configuration, the module copies some configuration variables to
 * local copies to speed up computation.
 */
GenericPropagationModule::GenericPropagationModule(Configuration& config,
                                                   Messenger* messenger,
                                                   std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {
    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector
    messenger_->bindSingle(this, &GenericPropagationModule::deposits_message_, MsgFlags::REQUIRED);

    // Seed the random generator with the module seed
    random_generator_.seed(getRandomSeed());

    // Set default value for config variables
    config_.setDefault<double>("spatial_precision", Units::get(0.25, "nm"));
    config_.setDefault<double>("timestep_start", Units::get(0.01, "ns"));
    config_.setDefault<double>("timestep_min", Units::get(0.001, "ns"));
    config_.setDefault<double>("timestep_max", Units::get(0.5, "ns"));
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 10);
    config_.setDefault<double>("temperature", 293.15);

    config_.setDefault<bool>("output_linegraphs", false);
    config_.setDefault<bool>("output_animations", false);
    config_.setDefault<bool>("output_plots",
                             config_.get<bool>("output_linegraphs") || config_.get<bool>("output_animations"));
    config_.setDefault<bool>("output_animations_color_markers", false);
    config_.setDefault<double>("output_plots_step", config_.get<double>("timestep_max"));
    config_.setDefault<bool>("output_plots_use_pixel_units", false);
    config_.setDefault<bool>("output_plots_align_pixels", false);
    config_.setDefault<double>("output_plots_theta", 0.0f);
    config_.setDefault<double>("output_plots_phi", 0.0f);
    config_.setDefault<bool>("output_plots_lines_at_implants", false);

    // Set defaults for charge carrier propagation:
    config_.setDefault<bool>("propagate_electrons", true);
    config_.setDefault<bool>("propagate_holes", false);
    if(!config_.get<bool>("propagate_electrons") && !config_.get<bool>("propagate_holes")) {
        throw InvalidValueError(
            config_,
            "propagate_electrons",
            "No charge carriers selected for propagation, enable 'propagate_electrons' or 'propagate_holes'.");
    }

    config_.setDefault<bool>("ignore_magnetic_field", false);

    // Copy some variables from configuration to avoid lookups:
    temperature_ = config_.get<double>("temperature");
    timestep_min_ = config_.get<double>("timestep_min");
    timestep_max_ = config_.get<double>("timestep_max");
    timestep_start_ = config_.get<double>("timestep_start");
    integration_time_ = config_.get<double>("integration_time");
    target_spatial_precision_ = config_.get<double>("spatial_precision");
    output_plots_ = config_.get<bool>("output_plots");
    output_linegraphs_ = config_.get<bool>("output_linegraphs");
    output_animations_ = config_.get<bool>("output_animations");
    output_plots_step_ = config_.get<double>("output_plots_step");
    output_plots_lines_at_implants_ = config_.get<bool>("output_plots_lines_at_implants");

    // Enable parallelization of this module if multithreading is enabled and no per-event output plots are requested:
    if(!(output_animations_ || output_linegraphs_)) {
        enable_parallelization();
    }

    // Parameterization variables from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2)
    electron_Vm_ = Units::get(1.53e9 * std::pow(temperature_, -0.87), "cm/s");
    electron_Ec_ = Units::get(1.01 * std::pow(temperature_, 1.55), "V/cm");
    electron_Beta_ = 2.57e-2 * std::pow(temperature_, 0.66);

    hole_Vm_ = Units::get(1.62e8 * std::pow(temperature_, -0.52), "cm/s");
    hole_Ec_ = Units::get(1.24 * std::pow(temperature_, 1.68), "V/cm");
    hole_Beta_ = 0.46 * std::pow(temperature_, 0.17);

    boltzmann_kT_ = Units::get(8.6173e-5, "eV/K") * temperature_;

    // Parameter for charge transport in magnetic field (approximated from graphs:
    // http://www.ioffe.ru/SVA/NSM/Semicond/Si/electric.html) FIXME
    electron_Hall_ = 1.15;
    hole_Hall_ = 0.9;
}

void GenericPropagationModule::create_output_plots(unsigned int event_num) {
    LOG(TRACE) << "Writing output plots";

    // Convert to pixel units if necessary
    if(config_.get<bool>("output_plots_use_pixel_units")) {
        for(auto& deposit_points : output_plot_points_) {
            for(auto& point : deposit_points.second) {
                point.SetX((point.x() / model_->getPixelSize().x()) + 1);
                point.SetY((point.y() / model_->getPixelSize().y()) + 1);
            }
        }
    }

    // Calculate the axis limits
    double minX = FLT_MAX, maxX = FLT_MIN;
    double minY = FLT_MAX, maxY = FLT_MIN;
    unsigned long tot_point_cnt = 0;
    double start_time = std::numeric_limits<double>::max();
    unsigned int total_charge = 0;
    unsigned int max_charge = 0;
    for(auto& deposit_points : output_plot_points_) {
        for(auto& point : deposit_points.second) {
            minX = std::min(minX, point.x());
            maxX = std::max(maxX, point.x());

            minY = std::min(minY, point.y());
            maxY = std::max(maxY, point.y());
        }
        start_time = std::min(start_time, deposit_points.first.getEventTime());
        total_charge += deposit_points.first.getCharge();
        max_charge = std::max(max_charge, deposit_points.first.getCharge());

        tot_point_cnt += deposit_points.second.size();
    }

    // Compute frame axis sizes if equal scaling is requested
    if(config_.get<bool>("output_plots_use_equal_scaling", true)) {
        double centerX = (minX + maxX) / 2.0;
        double centerY = (minY + maxY) / 2.0;
        if(config_.get<bool>("output_plots_use_pixel_units")) {
            minX = centerX - model_->getSensorSize().z() / model_->getPixelSize().x() / 2.0;
            maxX = centerX + model_->getSensorSize().z() / model_->getPixelSize().x() / 2.0;

            minY = centerY - model_->getSensorSize().z() / model_->getPixelSize().y() / 2.0;
            maxY = centerY + model_->getSensorSize().z() / model_->getPixelSize().y() / 2.0;
        } else {
            minX = centerX - model_->getSensorSize().z() / 2.0;
            maxX = centerX + model_->getSensorSize().z() / 2.0;

            minY = centerY - model_->getSensorSize().z() / 2.0;
            maxY = centerY + model_->getSensorSize().z() / 2.0;
        }
    }

    // Align on pixels if requested
    if(config_.get<bool>("output_plots_align_pixels")) {
        if(config_.get<bool>("output_plots_use_pixel_units")) {
            minX = std::floor(minX - 0.5) + 0.5;
            minY = std::floor(minY + 0.5) - 0.5;
            maxX = std::ceil(maxX - 0.5) + 0.5;
            maxY = std::ceil(maxY + 0.5) - 0.5;
        } else {
            double div;
            div = minX / model_->getPixelSize().x();
            minX = (std::floor(div - 0.5) + 0.5) * model_->getPixelSize().x();
            div = minY / model_->getPixelSize().y();
            minY = (std::floor(div - 0.5) + 0.5) * model_->getPixelSize().y();
            div = maxX / model_->getPixelSize().x();
            maxX = (std::ceil(div + 0.5) - 0.5) * model_->getPixelSize().x();
            div = maxY / model_->getPixelSize().y();
            maxY = (std::ceil(div + 0.5) - 0.5) * model_->getPixelSize().y();
        }
    }

    // Use a histogram to create the underlying frame
    auto* histogram_frame = new TH3F(("frame_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                                     "",
                                     10,
                                     minX,
                                     maxX,
                                     10,
                                     minY,
                                     maxY,
                                     10,
                                     model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0,
                                     model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0);
    histogram_frame->SetDirectory(getROOTDirectory());

    // Create the canvas for the line plot and set orientation
    auto canvas = std::make_unique<TCanvas>(("line_plot_" + std::to_string(event_num)).c_str(),
                                            ("Propagation of charge for event " + std::to_string(event_num)).c_str(),
                                            1280,
                                            1024);
    canvas->cd();
    canvas->SetTheta(config_.get<float>("output_plots_theta") * 180.0f / ROOT::Math::Pi());
    canvas->SetPhi(config_.get<float>("output_plots_phi") * 180.0f / ROOT::Math::Pi());

    // Draw the frame on the canvas
    histogram_frame->GetXaxis()->SetTitle(
        (std::string("x ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
    histogram_frame->GetYaxis()->SetTitle(
        (std::string("y ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
    histogram_frame->GetZaxis()->SetTitle("z (mm)");
    histogram_frame->Draw();

    // Loop over all point sets created during propagation
    // The vector of unique_pointers is required in order not to delete the objects before the canvas is drawn.
    std::vector<std::unique_ptr<TPolyLine3D>> lines;
    short current_color = 1;
    for(auto& deposit_points : output_plot_points_) {
        auto line = std::make_unique<TPolyLine3D>();
        for(auto& point : deposit_points.second) {
            line->SetNextPoint(point.x(), point.y(), point.z());
        }
        // Plot all lines with at least three points with different color
        if(line->GetN() >= 3) {
            EColor plot_color = (deposit_points.first.getType() == CarrierType::ELECTRON ? EColor::kAzure : EColor::kOrange);
            current_color = static_cast<short int>(plot_color - 9 + (static_cast<int>(current_color) + 1) % 19);
            line->SetLineColor(current_color);
            line->Draw("same");
        }
        lines.push_back(std::move(line));
    }

    // Draw and write canvas to module output file, then clear the stored lines
    canvas->Draw();
    getROOTDirectory()->WriteTObject(canvas.get());
    lines.clear();

    // Create canvas for GIF animition of process
    canvas = std::make_unique<TCanvas>(("animation_" + std::to_string(event_num)).c_str(),
                                       ("Propagation of charge for event " + std::to_string(event_num)).c_str(),
                                       1280,
                                       1024);
    canvas->cd();

    // Change axis labels if close to zero or PI as they behave different here
    if(std::fabs(config_.get<double>("output_plots_theta") / (ROOT::Math::Pi() / 2.0) -
                 std::round(config_.get<double>("output_plots_theta") / (ROOT::Math::Pi() / 2.0))) < 1e-6 ||
       std::fabs(config_.get<double>("output_plots_phi") / (ROOT::Math::Pi() / 2.0) -
                 std::round(config_.get<double>("output_plots_phi") / (ROOT::Math::Pi() / 2.0))) < 1e-6) {
        histogram_frame->GetXaxis()->SetLabelOffset(-0.1f);
        histogram_frame->GetYaxis()->SetLabelOffset(-0.075f);
    } else {
        histogram_frame->GetXaxis()->SetTitleOffset(2.0f);
        histogram_frame->GetYaxis()->SetTitleOffset(2.0f);
    }

    // Draw frame on canvas
    histogram_frame->Draw();

    if(output_animations_) {
        // Create the contour histogram
        std::vector<std::string> file_name_contour;
        std::vector<TH2F*> histogram_contour;
        file_name_contour.push_back(createOutputFile("contourX" + std::to_string(event_num) + ".gif"));
        histogram_contour.push_back(new TH2F(("contourX_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                                             "",
                                             100,
                                             minY,
                                             maxY,
                                             100,
                                             model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0,
                                             model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0));
        histogram_contour.back()->SetDirectory(getROOTDirectory());
        file_name_contour.push_back(createOutputFile("contourY" + std::to_string(event_num) + ".gif"));
        histogram_contour.push_back(new TH2F(("contourY_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                                             "",
                                             100,
                                             minX,
                                             maxX,
                                             100,
                                             model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0,
                                             model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0));
        histogram_contour.back()->SetDirectory(getROOTDirectory());
        file_name_contour.push_back(createOutputFile("contourZ" + std::to_string(event_num) + ".gif"));
        histogram_contour.push_back(new TH2F(("contourZ_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                                             "",
                                             100,
                                             minX,
                                             maxX,
                                             100,
                                             minY,
                                             maxY));
        histogram_contour.back()->SetDirectory(getROOTDirectory());

        // Create file and disable statistics for histogram
        std::string file_name_anim = createOutputFile("animation" + std::to_string(event_num) + ".gif");
        for(size_t i = 0; i < 3; ++i) {
            histogram_contour[i]->SetStats(false);
        }

        // Remove temporary created files
        auto ret = remove(file_name_anim.c_str());
        for(size_t i = 0; i < 3; ++i) {
            ret |= remove(file_name_contour[i].c_str());
        }
        if(ret != 0) {
            throw ModuleError("Could not delete temporary animation files.");
        }

        // Create color table
        TColor* colors[80]; // NOLINT
        for(int i = 20; i < 100; ++i) {
            auto color_idx = TColor::GetFreeColorIndex();
            colors[i - 20] = new TColor(color_idx,
                                        static_cast<float>(i) / 100.0f - 0.2f,
                                        static_cast<float>(i) / 100.0f - 0.2f,
                                        static_cast<float>(i) / 100.0f - 0.2f);
        }

        // Create animation of moving charges
        auto animation_time = static_cast<unsigned int>(
            std::round((Units::convert(config_.get<long double>("output_plots_step"), "ms") / 10.0) *
                       config_.get<long double>("output_animations_time_scaling", 1e9)));
        unsigned long plot_idx = 0;
        unsigned int point_cnt = 0;
        LOG_PROGRESS(INFO, getUniqueName() + "_OUTPUT_PLOTS") << "Written 0 of " << tot_point_cnt << " points for animation";
        while(point_cnt < tot_point_cnt) {
            std::vector<std::unique_ptr<TPolyMarker3D>> markers;
            unsigned long min_idx_diff = std::numeric_limits<unsigned long>::max();

            // Reset the canvas
            canvas->Clear();
            canvas->SetTheta(config_.get<float>("output_plots_theta") * 180.0f / ROOT::Math::Pi());
            canvas->SetPhi(config_.get<float>("output_plots_phi") * 180.0f / ROOT::Math::Pi());
            canvas->Draw();

            // Reset the histogram frame
            histogram_frame->SetTitle("Charge propagation in sensor");
            histogram_frame->GetXaxis()->SetTitle(
                (std::string("x ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
            histogram_frame->GetYaxis()->SetTitle(
                (std::string("y ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
            histogram_frame->GetZaxis()->SetTitle("z (mm)");
            histogram_frame->Draw();

            auto text = std::make_unique<TPaveText>(-0.75, -0.75, -0.60, -0.65);
            auto time_ns = Units::convert(plot_idx * config_.get<long double>("output_plots_step"), "ns");
            std::stringstream sstr;
            sstr << std::fixed << std::setprecision(2) << time_ns << "ns";
            auto time_str = std::string(8 - sstr.str().size(), ' ');
            time_str += sstr.str();
            text->AddText(time_str.c_str());
            text->Draw();

            // Plot all the required points
            for(auto& deposit_points : output_plot_points_) {
                auto points = deposit_points.second;

                auto diff = static_cast<unsigned long>(std::round((deposit_points.first.getEventTime() - start_time) /
                                                                  config_.get<long double>("output_plots_step")));
                if(static_cast<long>(plot_idx) - static_cast<long>(diff) < 0) {
                    min_idx_diff = std::min(min_idx_diff, diff - plot_idx);
                    continue;
                }
                auto idx = plot_idx - diff;
                if(idx >= points.size()) {
                    continue;
                }
                min_idx_diff = 0;

                auto marker = std::make_unique<TPolyMarker3D>();
                marker->SetMarkerStyle(kFullCircle);
                marker->SetMarkerSize(static_cast<float>(deposit_points.first.getCharge() *
                                                         config_.get<double>("output_animations_marker_size", 1)) /
                                      static_cast<float>(max_charge));
                auto initial_z_perc = static_cast<int>(
                    ((points[0].z() + model_->getSensorSize().z() / 2.0) / model_->getSensorSize().z()) * 80);
                initial_z_perc = std::max(std::min(79, initial_z_perc), 0);
                if(config_.get<bool>("output_animations_color_markers")) {
                    marker->SetMarkerColor(static_cast<Color_t>(colors[initial_z_perc]->GetNumber()));
                }
                marker->SetNextPoint(points[idx].x(), points[idx].y(), points[idx].z());
                marker->Draw();
                markers.push_back(std::move(marker));

                histogram_contour[0]->Fill(points[idx].y(), points[idx].z(), deposit_points.first.getCharge());
                histogram_contour[1]->Fill(points[idx].x(), points[idx].z(), deposit_points.first.getCharge());
                histogram_contour[2]->Fill(points[idx].x(), points[idx].y(), deposit_points.first.getCharge());
                ++point_cnt;
            }

            // Create a step in the animation
            if(min_idx_diff != 0) {
                canvas->Print((file_name_anim + "+100").c_str());
                plot_idx += min_idx_diff;
            } else {
                // print animation
                if(point_cnt < tot_point_cnt - 1) {
                    canvas->Print((file_name_anim + "+" + std::to_string(animation_time)).c_str());
                } else {
                    canvas->Print((file_name_anim + "++100").c_str());
                }

                // Draw and print contour histograms
                for(size_t i = 0; i < 3; ++i) {
                    canvas->Clear();
                    canvas->SetTitle((std::string("Contour of charge propagation projected on the ") +
                                      static_cast<char>('X' + i) + "-axis")
                                         .c_str());
                    switch(i) {
                    case 0 /* x */:
                        histogram_contour[i]->GetXaxis()->SetTitle(
                            (std::string("y ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)"))
                                .c_str());
                        histogram_contour[i]->GetYaxis()->SetTitle("z (mm)");
                        break;
                    case 1 /* y */:
                        histogram_contour[i]->GetXaxis()->SetTitle(
                            (std::string("x ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)"))
                                .c_str());
                        histogram_contour[i]->GetYaxis()->SetTitle("z (mm)");
                        break;
                    case 2 /* z */:
                        histogram_contour[i]->GetXaxis()->SetTitle(
                            (std::string("x ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)"))
                                .c_str());
                        histogram_contour[i]->GetYaxis()->SetTitle(
                            (std::string("y ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)"))
                                .c_str());
                        break;
                    default:;
                    }
                    histogram_contour[i]->SetMinimum(1);
                    histogram_contour[i]->SetMaximum(total_charge /
                                                     config_.get<double>("output_animations_contour_max_scaling", 10));
                    histogram_contour[i]->Draw("CONTZ 0");
                    if(point_cnt < tot_point_cnt - 1) {
                        canvas->Print((file_name_contour[i] + "+" + std::to_string(animation_time)).c_str());
                    } else {
                        canvas->Print((file_name_contour[i] + "++100").c_str());
                    }
                    histogram_contour[i]->Reset();
                }
                ++plot_idx;
            }
            markers.clear();

            LOG_PROGRESS(INFO, getUniqueName() + "_OUTPUT_PLOTS")
                << "Written " << point_cnt << " of " << tot_point_cnt << " points for animation";
        }
    }
    output_plot_points_.clear();
}

void GenericPropagationModule::init() {

    auto detector = getDetector();

    // Check for electric field and output warning for slow propagation if not defined
    if(!detector->hasElectricField()) {
        LOG(WARNING) << "This detector does not have an electric field.";
    }

    // For linear fields we can in addition check if the correct carriers are propagated
    if(detector->getElectricFieldType() == FieldType::LINEAR) {
        auto model = detector_->getModel();
        auto probe_point = ROOT::Math::XYZPoint(model->getSensorCenter().x(),
                                                model->getSensorCenter().y(),
                                                model->getSensorCenter().z() + model->getSensorSize().z() / 2.01);

        // Get the field close to the implants and check its sign:
        auto efield = detector->getElectricField(probe_point);
        auto direction = std::signbit(efield.z());
        // Compare with propagated carrier type:
        if(direction && !config_.get<bool>("propagate_electrons")) {
            LOG(WARNING) << "Electric field indicates electron collection at implants, but electrons are not propagated!";
        }
        if(!direction && !config_.get<bool>("propagate_holes")) {
            LOG(WARNING) << "Electric field indicates hole collection at implants, but holes are not propagated!";
        }
    }

    // Check for magnetic field
    has_magnetic_field_ = detector->hasMagneticField();
    if(has_magnetic_field_) {
        if(config_.get<bool>("ignore_magnetic_field")) {
            has_magnetic_field_ = false;
            LOG(WARNING) << "A magnetic field is switched on, but is set to be ignored for this module.";
        } else {
            LOG(DEBUG) << "This detector sees a magnetic field.";
            magnetic_field_ = detector_->getMagneticField();
        }
    }

    if(output_plots_) {
        step_length_histo_ = new TH1D("step_length_histo",
                                      "Step length;length [#mum];integration steps",
                                      100,
                                      0,
                                      static_cast<double>(Units::convert(0.25 * model_->getSensorSize().z(), "um")));

        drift_time_histo_ = new TH1D("drift_time_histo",
                                     "Drift time;Drift time [ns];charge carriers",
                                     static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                     0,
                                     static_cast<double>(Units::convert(integration_time_, "ns")));

        uncertainty_histo_ =
            new TH1D("uncertainty_histo",
                     "Position uncertainty;uncertainty [nm];integration steps",
                     100,
                     0,
                     static_cast<double>(4 * Units::convert(config_.get<double>("spatial_precision"), "nm")));

        group_size_histo_ = new TH1D("group_size_histo",
                                     "Charge carrier group size;group size;number of groups trasnported",
                                     config_.get<int>("charge_per_step") - 1,
                                     1,
                                     static_cast<double>(config_.get<unsigned int>("charge_per_step")));
    }
}

void GenericPropagationModule::run(unsigned int event_num) {

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;

    // Loop over all deposits for propagation
    LOG(TRACE) << "Propagating charges in sensor";
    unsigned int propagated_charges_count = 0;
    unsigned int step_count = 0;
    long double total_time = 0;
    for(auto& deposit : deposits_message_->getData()) {

        if((deposit.getType() == CarrierType::ELECTRON && !config_.get<bool>("propagate_electrons")) ||
           (deposit.getType() == CarrierType::HOLE && !config_.get<bool>("propagate_holes"))) {
            LOG(DEBUG) << "Skipping charge carriers (" << deposit.getType() << ") on "
                       << Units::display(deposit.getLocalPosition(), {"mm", "um"});
            continue;
        }

        // Loop over all charges in the deposit
        unsigned int charges_remaining = deposit.getCharge();

        LOG(DEBUG) << "Set of charge carriers (" << deposit.getType() << ") on "
                   << Units::display(deposit.getLocalPosition(), {"mm", "um"});

        auto charge_per_step = config_.get<unsigned int>("charge_per_step");
        while(charges_remaining > 0) {
            // Define number of charges to be propagated and remove charges of this step from the total
            if(charge_per_step > charges_remaining) {
                charge_per_step = charges_remaining;
            }
            charges_remaining -= charge_per_step;

            // Get position and propagate through sensor
            auto position = deposit.getLocalPosition();

            // Add point of deposition to the output plots if requested
            if(output_linegraphs_) {
                auto global_position = detector_->getGlobalPosition(position);
                output_plot_points_.emplace_back(
                    PropagatedCharge(position, global_position, deposit.getType(), charge_per_step, deposit.getEventTime()),
                    std::vector<ROOT::Math::XYZPoint>());
            }

            // Propagate a single charge deposit
            auto prop_pair = propagate(position, deposit.getType());
            position = prop_pair.first;

            LOG(DEBUG) << " Propagated " << charge_per_step << " to " << Units::display(position, {"mm", "um"}) << " in "
                       << Units::display(prop_pair.second, "ns") << " time";

            // Create a new propagated charge and add it to the list
            auto global_position = detector_->getGlobalPosition(position);
            PropagatedCharge propagated_charge(position,
                                               global_position,
                                               deposit.getType(),
                                               charge_per_step,
                                               deposit.getEventTime() + prop_pair.second,
                                               &deposit);

            propagated_charges.push_back(std::move(propagated_charge));

            // Update statistical information
            ++step_count;
            propagated_charges_count += charge_per_step;
            total_time += charge_per_step * prop_pair.second;
            if(output_plots_) {
                drift_time_histo_->Fill(static_cast<double>(Units::convert(prop_pair.second, "ns")), charge_per_step);
                group_size_histo_->Fill(charge_per_step);
            }
        }
    }

    // Output plots if required
    if(output_linegraphs_) {
        create_output_plots(event_num);
    }

    // Write summary and update statistics
    long double average_time = total_time / std::max(1u, propagated_charges_count);
    LOG(INFO) << "Propagated " << propagated_charges_count << " charges in " << step_count << " steps in average time of "
              << Units::display(average_time, "ns");
    total_propagated_charges_ += propagated_charges_count;
    total_steps_ += step_count;
    total_time_ += total_time;

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, propagated_charge_message);
}

/**
 * Propagation is simulated using a parameterization for the electron mobility. This is used to calculate the electron
 * velocity at every point with help of the electric field map of the detector. An Runge-Kutta integration is applied in
 * multiple steps, adding a random diffusion to the propagating charge every step.
 */
std::pair<ROOT::Math::XYZPoint, double> GenericPropagationModule::propagate(const ROOT::Math::XYZPoint& pos,
                                                                            const CarrierType& type) {
    // Create a runge kutta solver using the electric field as step function
    Eigen::Vector3d position(pos.x(), pos.y(), pos.z());

    // Define a lambda function to compute the carrier mobility
    // NOTE This function is typically the most frequently executed part of the framework and therefore the bottleneck
    auto carrier_mobility = [&](double efield_mag) {
        // Compute carrier mobility from constants and electric field magnitude
        double numerator, denominator;
        if(type == CarrierType::ELECTRON) {
            numerator = electron_Vm_ / electron_Ec_;
            denominator = std::pow(1. + std::pow(efield_mag / electron_Ec_, electron_Beta_), 1.0 / electron_Beta_);
        } else {
            numerator = hole_Vm_ / hole_Ec_;
            denominator = std::pow(1. + std::pow(efield_mag / hole_Ec_, hole_Beta_), 1.0 / hole_Beta_);
        }
        return numerator / denominator;
    };

    // Define a function to compute the diffusion
    auto carrier_diffusion = [&](double efield_mag, double timestep) -> Eigen::Vector3d {
        double diffusion_constant = boltzmann_kT_ * carrier_mobility(efield_mag);
        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * timestep);

        // Compute the independent diffusion in three
        std::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
        Eigen::Vector3d diffusion;
        for(int i = 0; i < 3; ++i) {
            diffusion[i] = gauss_distribution(random_generator_);
        }
        return diffusion;
    };

    // Define lambda functions to compute the charge carrier velocity with or without magnetic field
    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&)> carrier_velocity_noB =
        [&](double, const Eigen::Vector3d& cur_pos) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());

        return static_cast<int>(type) * carrier_mobility(efield.norm()) * efield;
    };

    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&)> carrier_velocity_withB =
        [&](double, const Eigen::Vector3d& cur_pos) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());

        Eigen::Vector3d velocity;
        Eigen::Vector3d bfield(magnetic_field_.x(), magnetic_field_.y(), magnetic_field_.z());

        auto mob = carrier_mobility(efield.norm());
        auto exb = efield.cross(bfield);

        Eigen::Vector3d term1;
        double hallFactor = (type == CarrierType::ELECTRON ? electron_Hall_ : hole_Hall_);
        term1 = static_cast<int>(type) * mob * hallFactor * exb;

        Eigen::Vector3d term2 = mob * mob * hallFactor * hallFactor * efield.dot(bfield) * bfield;

        auto rnorm = 1 + mob * mob * hallFactor * hallFactor * bfield.dot(bfield);
        return static_cast<int>(type) * mob * (efield + term1 + term2) / rnorm;
    };

    // Create the runge kutta solver with an RKF5 tableau, using different velocity calculators depending on the magnetic
    // field
    auto runge_kutta = make_runge_kutta(
        tableau::RK5, (has_magnetic_field_ ? carrier_velocity_withB : carrier_velocity_noB), timestep_start_, position);

    // Continue propagation until the deposit is outside the sensor
    Eigen::Vector3d last_position = position;
    double last_time = 0;
    size_t next_idx = 0;
    while(detector_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position)) &&
          runge_kutta.getTime() < integration_time_) {
        // Update output plots if necessary (depending on the plot step)
        if(output_linegraphs_) {
            auto time_idx = static_cast<size_t>(runge_kutta.getTime() / output_plots_step_);
            while(next_idx <= time_idx) {
                output_plot_points_.back().second.push_back(static_cast<ROOT::Math::XYZPoint>(position));
                next_idx = output_plot_points_.back().second.size();
            }
        }

        // Save previous position and time
        last_position = position;
        last_time = runge_kutta.getTime();

        // Execute a Runge Kutta step
        auto step = runge_kutta.step();

        // Get the current result and timestep
        auto timestep = runge_kutta.getTimeStep();
        position = runge_kutta.getValue();

        // Get electric field at current position and fall back to empty field if it does not exist
        auto efield = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(position));

        // Apply diffusion step
        auto diffusion = carrier_diffusion(std::sqrt(efield.Mag2()), timestep);
        position += diffusion;
        runge_kutta.setValue(position);

        // Adapt step size to match target precision
        double uncertainty = step.error.norm();

        // Update step length histogram
        if(output_plots_) {
            step_length_histo_->Fill(static_cast<double>(Units::convert(step.value.norm(), "um")));
            uncertainty_histo_->Fill(static_cast<double>(Units::convert(step.error.norm(), "nm")));
        }

        // Lower timestep when reaching the sensor edge
        if(std::fabs(model_->getSensorSize().z() / 2.0 - position.z()) < 2 * step.value.z()) {
            timestep *= 0.75;
        } else {
            if(uncertainty > target_spatial_precision_) {
                timestep *= 0.75;
            } else if(2 * uncertainty < target_spatial_precision_) {
                timestep *= 1.5;
            }
        }
        // Limit the timestep to certain minimum and maximum step sizes
        if(timestep > timestep_max_) {
            timestep = timestep_max_;
        } else if(timestep < timestep_min_) {
            timestep = timestep_min_;
        }
        runge_kutta.setTimeStep(timestep);
    }

    // Find proper final position in the sensor
    auto time = runge_kutta.getTime();
    if(!detector_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position))) {
        auto check_position = position;
        check_position.z() = last_position.z();
        if(position.z() > 0 && detector_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(check_position))) {
            // Carrier left sensor on the side of the pixel grid, interpolate end point on surface
            auto z_cur_border = std::fabs(position.z() - model_->getSensorSize().z() / 2.0);
            auto z_last_border = std::fabs(model_->getSensorSize().z() / 2.0 - last_position.z());
            auto z_total = z_cur_border + z_last_border;
            position = (z_last_border / z_total) * position + (z_cur_border / z_total) * last_position;
            time = (z_last_border / z_total) * time + (z_cur_border / z_total) * last_time;
        } else {
            // Carrier left sensor on any order border, use last position inside instead
            position = last_position;
            time = last_time;
        }
    }

    // If requested, remove charge drift lines from plots if they did not reach the implant side within the integration time:
    if(output_linegraphs_ && output_plots_lines_at_implants_) {
        // If drift time is larger than integration time or the charge carriers have been collected at the backside, remove
        if(time >= integration_time_ || last_position.z() < -model_->getSensorSize().z() * 0.45) {
            output_plot_points_.pop_back();
        }
    }

    // Return the final position of the propagated charge
    return std::make_pair(static_cast<ROOT::Math::XYZPoint>(position), time);
}

void GenericPropagationModule::finalize() {
    if(output_plots_) {
        step_length_histo_->Write();
        drift_time_histo_->Write();
        uncertainty_histo_->Write();
        group_size_histo_->Write();
    }

    long double average_time = total_time_ / std::max(1u, total_propagated_charges_);
    LOG(INFO) << "Propagated total of " << total_propagated_charges_ << " charges in " << total_steps_
              << " steps in average time of " << Units::display(average_time, "ns");
}
