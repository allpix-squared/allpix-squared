/**
 * @file
 * @brief Implementation of generic charge propagation module
 * @remarks Based on code from Paul Schuetze
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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
#include "core/utils/distributions.h"
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
    messenger_->bindSingle<DepositedChargeMessage>(this, MsgFlags::REQUIRED);

    // Set default value for config variables
    config_.setDefault<double>("spatial_precision", Units::get(0.25, "nm"));
    config_.setDefault<double>("timestep_start", Units::get(0.01, "ns"));
    config_.setDefault<double>("timestep_min", Units::get(0.001, "ns"));
    config_.setDefault<double>("timestep_max", Units::get(0.5, "ns"));
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 10);
    config_.setDefault<double>("temperature", 293.15);

    // Models:
    config_.setDefault<std::string>("mobility_model", "jacoboni");
    config_.setDefault<std::string>("recombination_model", "none");

    config_.setDefault<bool>("output_linegraphs", false);
    config_.setDefault<bool>("output_linegraphs_collected", false);
    config_.setDefault<bool>("output_linegraphs_recombined", false);
    config_.setDefault<bool>("output_animations", false);
    config_.setDefault<bool>("output_plots",
                             config_.get<bool>("output_linegraphs") || config_.get<bool>("output_animations"));
    config_.setDefault<bool>("output_animations_color_markers", false);
    config_.setDefault<double>("output_plots_step", config_.get<double>("timestep_max"));
    config_.setDefault<bool>("output_plots_use_pixel_units", false);
    config_.setDefault<bool>("output_plots_align_pixels", false);
    config_.setDefault<double>("output_plots_theta", 0.0f);
    config_.setDefault<double>("output_plots_phi", 0.0f);

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
    output_linegraphs_collected_ = config_.get<bool>("output_linegraphs_collected");
    output_linegraphs_recombined_ = config_.get<bool>("output_linegraphs_recombined");
    output_linegraphs_trapped_ = config_.get<bool>("output_linegraphs_trapped");
    output_animations_ = config_.get<bool>("output_animations");
    output_plots_step_ = config_.get<double>("output_plots_step");
    propagate_electrons_ = config_.get<bool>("propagate_electrons");
    propagate_holes_ = config_.get<bool>("propagate_holes");
    charge_per_step_ = config_.get<unsigned int>("charge_per_step");

    // Enable multithreading of this module if multithreading is enabled and no per-event output plots are requested:
    // FIXME: Review if this is really the case or we can still use multithreading
    if(!(output_animations_ || output_linegraphs_)) {
        allow_multithreading();
    } else {
        LOG(WARNING) << "Per-event line graphs or animations requested, disabling parallel event processing";
    }

    boltzmann_kT_ = Units::get(8.6173e-5, "eV/K") * temperature_;

    // Parameter for charge transport in magnetic field (approximated from graphs:
    // http://www.ioffe.ru/SVA/NSM/Semicond/Si/electric.html) FIXME
    electron_Hall_ = 1.15;
    hole_Hall_ = 0.9;
}

void GenericPropagationModule::create_output_plots(uint64_t event_num,
                                                   const OutputPlotPoints& output_plot_points,
                                                   CarrierState plotting_state) {
    auto title = (plotting_state == CarrierState::UNKNOWN ? "all" : allpix::to_string(plotting_state));
    LOG(TRACE) << "Writing output plots, for " << title << " charge carriers";

    // Convert to pixel units if necessary
    double scale_x = (config_.get<bool>("output_plots_use_pixel_units") ? model_->getPixelSize().x() : 1);
    double scale_y = (config_.get<bool>("output_plots_use_pixel_units") ? model_->getPixelSize().y() : 1);

    // Calculate the axis limits
    double minX = FLT_MAX, maxX = FLT_MIN;
    double minY = FLT_MAX, maxY = FLT_MIN;
    unsigned long tot_point_cnt = 0;
    double start_time = std::numeric_limits<double>::max();
    unsigned int total_charge = 0;
    unsigned int max_charge = 0;
    for(auto& [deposit, points] : output_plot_points) {
        for(auto& point : points) {
            minX = std::min(minX, point.x() / scale_x);
            maxX = std::max(maxX, point.x() / scale_x);

            minY = std::min(minY, point.y() / scale_y);
            maxY = std::max(maxY, point.y() / scale_y);
        }
        auto& [time, charge, type, state] = deposit;
        start_time = std::min(start_time, time);
        total_charge += charge;
        max_charge = std::max(max_charge, charge);

        tot_point_cnt += points.size();
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
            double div = minX / model_->getPixelSize().x();
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
    auto* histogram_frame = new TH3F(("frame_" + getUniqueName() + "_" + std::to_string(event_num) + "_" + title).c_str(),
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
    auto canvas = std::make_unique<TCanvas>(("line_plot_" + std::to_string(event_num) + "_" + title).c_str(),
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
    for(auto& [deposit, points] : output_plot_points) {
        // Check if we should plot this point:
        if(plotting_state != CarrierState::UNKNOWN && plotting_state != std::get<3>(deposit)) {
            continue;
        }

        auto line = std::make_unique<TPolyLine3D>();
        for(auto& point : points) {
            line->SetNextPoint(point.x() / scale_x, point.y() / scale_y, point.z());
        }
        // Plot all lines with at least three points with different color
        if(line->GetN() >= 3) {
            EColor plot_color = (std::get<2>(deposit) == CarrierType::ELECTRON ? EColor::kAzure : EColor::kOrange);
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
    canvas = std::make_unique<TCanvas>(("animation_" + std::to_string(event_num) + "_" + title).c_str(),
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
        file_name_contour.push_back(createOutputFile("contourX" + std::to_string(event_num) + "_" + title + ".gif"));
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
            for(auto& [deposit, points] : output_plot_points) {
                auto& [time, charge, type, state] = deposit;

                // Check if we should plot this point:
                if(plotting_state != CarrierState::UNKNOWN && plotting_state != state) {
                    continue;
                }

                auto diff = static_cast<unsigned long>(
                    std::round((time - start_time) / config_.get<long double>("output_plots_step")));
                if(plot_idx < diff) {
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
                marker->SetMarkerSize(static_cast<float>(charge * config_.get<double>("output_animations_marker_size", 1)) /
                                      static_cast<float>(max_charge));
                auto initial_z_perc = static_cast<int>(
                    ((points[0].z() + model_->getSensorSize().z() / 2.0) / model_->getSensorSize().z()) * 80);
                initial_z_perc = std::max(std::min(79, initial_z_perc), 0);
                if(config_.get<bool>("output_animations_color_markers")) {
                    marker->SetMarkerColor(static_cast<Color_t>(colors[initial_z_perc]->GetNumber()));
                }
                marker->SetNextPoint(points[idx].x() / scale_x, points[idx].y() / scale_y, points[idx].z());
                marker->Draw();
                markers.push_back(std::move(marker));

                histogram_contour[0]->Fill(points[idx].y() / scale_y, points[idx].z(), charge);
                histogram_contour[1]->Fill(points[idx].x() / scale_x, points[idx].z(), charge);
                histogram_contour[2]->Fill(points[idx].x() / scale_x, points[idx].y() / scale_y, charge);
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
}

void GenericPropagationModule::initialize() {

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
        if(direction && !propagate_electrons_) {
            LOG(WARNING) << "Electric field indicates electron collection at implants, but electrons are not propagated!";
        }
        if(!direction && !propagate_holes_) {
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
        step_length_histo_ =
            CreateHistogram<TH1D>("step_length_histo",
                                  "Step length;length [#mum];integration steps",
                                  100,
                                  0,
                                  static_cast<double>(Units::convert(0.25 * model_->getSensorSize().z(), "um")));

        drift_time_histo_ = CreateHistogram<TH1D>("drift_time_histo",
                                                  "Drift time;Drift time [ns];charge carriers",
                                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                                  0,
                                                  static_cast<double>(Units::convert(integration_time_, "ns")));

        uncertainty_histo_ =
            CreateHistogram<TH1D>("uncertainty_histo",
                                  "Position uncertainty;uncertainty [nm];integration steps",
                                  100,
                                  0,
                                  static_cast<double>(4 * Units::convert(config_.get<double>("spatial_precision"), "nm")));

        group_size_histo_ = CreateHistogram<TH1D>("group_size_histo",
                                                  "Charge carrier group size;group size;number of groups transported",
                                                  static_cast<int>(charge_per_step_ - 1),
                                                  1,
                                                  static_cast<double>(charge_per_step_));

        recombine_histo_ =
            CreateHistogram<TH1D>("recombination_histo",
                                  "Fraction of recombined charge carriers;recombination [N / N_{total}] ;number of events",
                                  100,
                                  0,
                                  1);

        trapped_histo_ = CreateHistogram<TH1D>(
            "trapping_histo", "Fraction of trapped charge carriers;trapping [N / N_{total}] ;number of events", 100, 0, 1);
    }

    // Prepare mobility model
    mobility_ = Mobility(config_, detector->hasDopingProfile());

    // Prepare recombination model
    recombination_ = Recombination(config_, detector->hasDopingProfile());

    trapping_ = Trapping(config_);
}

void GenericPropagationModule::run(Event* event) {
    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;

    // List of points to plot to plot for output plots
    OutputPlotPoints output_plot_points;

    // Loop over all deposits for propagation
    LOG(TRACE) << "Propagating charges in sensor";
    unsigned int propagated_charges_count = 0;
    unsigned int recombined_charges_count = 0;
    unsigned int trapped_charges_count = 0;
    unsigned int step_count = 0;
    long double total_time = 0;
    for(const auto& deposit : deposits_message->getData()) {

        if((deposit.getType() == CarrierType::ELECTRON && !propagate_electrons_) ||
           (deposit.getType() == CarrierType::HOLE && !propagate_holes_)) {
            LOG(DEBUG) << "Skipping charge carriers (" << deposit.getType() << ") on "
                       << Units::display(deposit.getLocalPosition(), {"mm", "um"});
            continue;
        }

        // Only process if within requested integration time:
        if(deposit.getLocalTime() > integration_time_) {
            LOG(DEBUG) << "Skipping charge carriers deposited beyond integration time: "
                       << Units::display(deposit.getGlobalTime(), "ns") << " global / "
                       << Units::display(deposit.getLocalTime(), {"ns", "ps"}) << " local";
            continue;
        }

        // Loop over all charges in the deposit
        unsigned int charges_remaining = deposit.getCharge();

        LOG(DEBUG) << "Set of charge carriers (" << deposit.getType() << ") on "
                   << Units::display(deposit.getLocalPosition(), {"mm", "um"});

        auto charge_per_step = charge_per_step_;
        while(charges_remaining > 0) {
            // Define number of charges to be propagated and remove charges of this step from the total
            if(charge_per_step > charges_remaining) {
                charge_per_step = charges_remaining;
            }
            charges_remaining -= charge_per_step;

            // Get position and propagate through sensor
            auto initial_position = deposit.getLocalPosition();

            // Add point of deposition to the output plots if requested
            if(output_linegraphs_) {
                output_plot_points.emplace_back(
                    std::make_tuple(deposit.getGlobalTime(), charge_per_step, deposit.getType(), CarrierState::MOTION),
                    std::vector<ROOT::Math::XYZPoint>());
            }

            // Propagate a single charge deposit
            auto [final_position, time, state] = propagate(
                initial_position, deposit.getType(), deposit.getLocalTime(), event->getRandomEngine(), output_plot_points);

            if(state == CarrierState::RECOMBINED) {
                LOG(DEBUG) << " Recombined " << charge_per_step << " at " << Units::display(final_position, {"mm", "um"})
                           << " in " << Units::display(time, "ns") << " time, removing";
                recombined_charges_count += charge_per_step;
                continue;
            } else if(state == CarrierState::TRAPPED) {
                LOG(DEBUG) << " Trapped " << charge_per_step << " at " << Units::display(final_position, {"mm", "um"})
                           << " in " << Units::display(time, "ns") << " time, removing";
                trapped_charges_count += charge_per_step;
                continue;
            }

            LOG(DEBUG) << " Propagated " << charge_per_step << " to " << Units::display(final_position, {"mm", "um"})
                       << " in " << Units::display(time, "ns") << " time, final state: " << allpix::to_string(state);

            // Create a new propagated charge and add it to the list
            auto global_position = detector_->getGlobalPosition(final_position);
            PropagatedCharge propagated_charge(final_position,
                                               global_position,
                                               deposit.getType(),
                                               charge_per_step,
                                               deposit.getLocalTime() + time,
                                               deposit.getGlobalTime() + time,
                                               state,
                                               &deposit);

            propagated_charges.push_back(std::move(propagated_charge));

            // Update statistical information
            ++step_count;
            propagated_charges_count += charge_per_step;
            total_time += charge_per_step * time;
            if(output_plots_) {
                drift_time_histo_->Fill(static_cast<double>(Units::convert(time, "ns")), charge_per_step);
                group_size_histo_->Fill(charge_per_step);
            }
        }
    }

    // Output plots if required
    if(output_linegraphs_) {
        create_output_plots(event->number, output_plot_points, CarrierState::UNKNOWN);
        if(output_linegraphs_collected_) {
            create_output_plots(event->number, output_plot_points, CarrierState::HALTED);
        }
        if(output_linegraphs_recombined_) {
            create_output_plots(event->number, output_plot_points, CarrierState::RECOMBINED);
        }
        if(output_linegraphs_trapped_) {
            create_output_plots(event->number, output_plot_points, CarrierState::TRAPPED);
        }
    }

    // Write summary and update statistics
    long double average_time = total_time / std::max(1u, propagated_charges_count);
    LOG(INFO) << "Propagated " << propagated_charges_count << " charges in " << step_count << " steps in average time of "
              << Units::display(average_time, "ns") << std::endl
              << "Recombined " << recombined_charges_count << " charges during transport" << std::endl
              << "Trapped " << trapped_charges_count << " charges during transport";
    total_propagated_charges_ += propagated_charges_count;
    total_steps_ += step_count;
    total_time_picoseconds_ += static_cast<long unsigned int>(total_time * 1e3);

    if(output_plots_) {
        auto total_charges = propagated_charges_count + recombined_charges_count + trapped_charges_count;
        recombine_histo_->Fill(static_cast<double>(recombined_charges_count) / total_charges);
        trapped_histo_->Fill(static_cast<double>(trapped_charges_count) / total_charges);
    }

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, propagated_charge_message, event);
}

/**
 * Propagation is simulated using a parameterization for the electron mobility. This is used to calculate the electron
 * velocity at every point with help of the electric field map of the detector. An Runge-Kutta integration is applied in
 * multiple steps, adding a random diffusion to the propagating charge every step.
 */
std::tuple<ROOT::Math::XYZPoint, double, CarrierState>
GenericPropagationModule::propagate(const ROOT::Math::XYZPoint& pos,
                                    const CarrierType& type,
                                    const double initial_time,
                                    RandomNumberGenerator& random_generator,
                                    OutputPlotPoints& output_plot_points) const {
    // Create a runge kutta solver using the electric field as step function
    Eigen::Vector3d position(pos.x(), pos.y(), pos.z());

    // Define a function to compute the diffusion
    auto carrier_diffusion = [&](double efield_mag, double doping_concentration, double timestep) -> Eigen::Vector3d {
        double diffusion_constant = boltzmann_kT_ * mobility_(type, efield_mag, doping_concentration);
        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * timestep);

        // Compute the independent diffusion in three
        allpix::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
        Eigen::Vector3d diffusion;
        for(int i = 0; i < 3; ++i) {
            diffusion[i] = gauss_distribution(random_generator);
        }
        return diffusion;
    };

    // Survival probability of this charge carrier package, evaluated at every step
    std::uniform_real_distribution<double> survival(0, 1);

    // Define lambda functions to compute the charge carrier velocity with or without magnetic field
    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&)> carrier_velocity_noB =
        [&](double, const Eigen::Vector3d& cur_pos) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());
        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(cur_pos));

        return static_cast<int>(type) * mobility_(type, efield.norm(), doping) * efield;
    };

    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&)> carrier_velocity_withB =
        [&](double, const Eigen::Vector3d& cur_pos) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());

        Eigen::Vector3d velocity;
        Eigen::Vector3d bfield(magnetic_field_.x(), magnetic_field_.y(), magnetic_field_.z());

        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(cur_pos));

        auto mob = mobility_(type, efield.norm(), doping);
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
    auto state = CarrierState::MOTION;
    while(state == CarrierState::MOTION && (initial_time + runge_kutta.getTime()) < integration_time_) {
        // Update output plots if necessary (depending on the plot step)
        if(output_linegraphs_) {
            auto time_idx = static_cast<size_t>(runge_kutta.getTime() / output_plots_step_);
            while(next_idx <= time_idx) {
                output_plot_points.back().second.push_back(static_cast<ROOT::Math::XYZPoint>(position));
                next_idx = output_plot_points.back().second.size();
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
        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(position));

        // Apply diffusion step
        auto diffusion = carrier_diffusion(std::sqrt(efield.Mag2()), doping, timestep);
        position += diffusion;
        runge_kutta.setValue(position);

        // Check if we are still in the sensor:
        if(!detector_->getModel()->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position))) {
            state = CarrierState::HALTED;
        }

        // Check if charge carrier is still alive:
        if(recombination_(type,
                          detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(position)),
                          survival(random_generator),
                          timestep)) {
            state = CarrierState::RECOMBINED;
        }

        // Check if the charge carrier has been trapped:
        auto [trapped, traptime] = trapping_(type, survival(random_generator), timestep, std::sqrt(efield.Mag2()));
        if(trapped) {
            state = CarrierState::TRAPPED;
            // FIXME advance carrier time by traptime and allow de-trapping?
        }

        LOG(TRACE) << "Step from " << Units::display(static_cast<ROOT::Math::XYZPoint>(last_position), {"um", "mm"})
                   << " to " << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um", "mm"}) << " at "
                   << Units::display(initial_time + runge_kutta.getTime(), {"ps", "ns", "us"})
                   << ", state: " << allpix::to_string(state);
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
    if(state == CarrierState::HALTED) {
        auto intercept = model_->getSensorIntercept(static_cast<ROOT::Math::XYZPoint>(last_position),
                                                    static_cast<ROOT::Math::XYZPoint>(position));
        position = Eigen::Vector3d(intercept.x(), intercept.y(), intercept.z());
    }

    // Set final state of charge carrier for plotting:
    if(output_linegraphs_) {
        // If drift time is larger than integration time or the charge carriers have been collected at the backside, reset:
        if(time >= integration_time_ || last_position.z() < -model_->getSensorSize().z() * 0.45) {
            std::get<3>(output_plot_points.back().first) = CarrierState::UNKNOWN;
        } else {
            std::get<3>(output_plot_points.back().first) = state;
        }
    }

    if(state == CarrierState::RECOMBINED) {
        LOG(DEBUG) << "Charge carrier recombined after " << Units::display(last_time, {"ns"});
    } else if(state == CarrierState::TRAPPED) {
        LOG(DEBUG) << "Charge carrier trapped after " << Units::display(last_time, {"ns"}) << " at "
                   << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um", "mm"});
    }

    // Return the final position of the propagated charge
    return std::make_tuple(static_cast<ROOT::Math::XYZPoint>(position), initial_time + time, state);
}

void GenericPropagationModule::finalize() {
    if(output_plots_) {
        step_length_histo_->Write();
        drift_time_histo_->Write();
        uncertainty_histo_->Write();
        group_size_histo_->Write();
        recombine_histo_->Write();
        trapped_histo_->Write();
    }

    long double average_time = static_cast<long double>(total_time_picoseconds_) / 1e3 /
                               std::max(1u, static_cast<unsigned int>(total_propagated_charges_));
    LOG(INFO) << "Propagated total of " << total_propagated_charges_ << " charges in " << total_steps_
              << " steps in average time of " << Units::display(average_time, "ns");
}
