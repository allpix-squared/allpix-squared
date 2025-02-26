/**
 * @file
 * @brief Utility to plot line graph diagrams of charge carrier drift paths
 *
 * @copyright Copyright (c) 2022-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LINE_GRAPHS_H
#define ALLPIX_LINE_GRAPHS_H

#include "core/module/Module.hpp"
#include "objects/PropagatedCharge.hpp"

#include <Math/Point3D.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH2F.h>
#include <TH3F.h>
#include <TPaveText.h>
#include <TPolyLine3D.h>
#include <TPolyMarker3D.h>
#include <TStyle.h>

namespace allpix {

    class LineGraph {

    public:
        using OutputPlotPoints = std::vector<
            std::pair<std::tuple<double, unsigned int, CarrierType, CarrierState>, std::vector<ROOT::Math::XYZPoint>>>;

        /**
         * @brief Generate line graphs of charge carrier drift paths
         *
         * @param event_num Index for this event
         * @param module Module to generate plots for, used to create output files and to obtain ROOT directory
         * @param config Configuration object used for this module instance
         * @param output_plot_points List of points cached for plotting
         * @param plotting_state State of charge carriers to be plotted. If state is set to CarrierState::UNKNOWN, all charge
         * carriers are plotted.
         */
        static void Create(uint64_t event_num, // NOLINT
                           Module* module,
                           const Configuration& config,
                           const OutputPlotPoints& output_plot_points,
                           CarrierState plotting_state) {

            auto model = module->getDetector()->getModel();

            auto title = (plotting_state == CarrierState::UNKNOWN ? "all" : allpix::to_string(plotting_state));
            LOG(TRACE) << "Writing line graph for " << title << " charge carriers";

            auto [minX, maxX, minY, maxY, scale_x, scale_y, max_charge, total_charge, tot_point_cnt, start_time] =
                get_plot_settings(model, config, output_plot_points);

            // Use a histogram to create the underlying frame
            auto* histogram_frame =
                new TH3F(("frame_" + module->getUniqueName() + "_" + std::to_string(event_num) + "_" + title).c_str(),
                         "",
                         10,
                         minX,
                         maxX,
                         10,
                         minY,
                         maxY,
                         10,
                         model->getSensorCenter().z() - model->getSensorSize().z() / 2.0,
                         model->getSensorCenter().z() + model->getSensorSize().z() / 2.0);
            histogram_frame->SetDirectory(module->getROOTDirectory());

            // Create the canvas for the line plot and set orientation
            auto canvas = std::make_unique<TCanvas>(("line_plot_" + std::to_string(event_num) + "_" + title).c_str(),
                                                    ("Propagation of charge for event " + std::to_string(event_num)).c_str(),
                                                    1280,
                                                    1024);
            canvas->cd();
            canvas->SetTheta(config.get<float>("output_plots_theta") * 180.0f / ROOT::Math::Pi());
            canvas->SetPhi(config.get<float>("output_plots_phi") * 180.0f / ROOT::Math::Pi());

            // Draw the frame on the canvas
            histogram_frame->GetXaxis()->SetTitle(
                (std::string("x ") + (config.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
            histogram_frame->GetYaxis()->SetTitle(
                (std::string("y ") + (config.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
            histogram_frame->GetZaxis()->SetTitle("z (mm)");
            histogram_frame->Draw();

            // Loop over all point sets created during propagation
            // The vector of unique_pointers is required in order not to delete the objects before the canvas is drawn.
            std::vector<std::unique_ptr<TPolyLine3D>> lines;
            short current_color = 1;
            for(const auto& [deposit, points] : output_plot_points) {
                // Check if we should plot this point:
                if(plotting_state != CarrierState::UNKNOWN && plotting_state != std::get<3>(deposit)) {
                    continue;
                }

                auto line = std::make_unique<TPolyLine3D>();
                for(const auto& point : points) {
                    line->SetNextPoint(point.x() / scale_x, point.y() / scale_y, point.z());
                }
                // Plot all lines with at least three points with different color
                if(line->GetN() >= 2) {
                    EColor plot_color = (std::get<2>(deposit) == CarrierType::ELECTRON ? EColor::kAzure : EColor::kOrange);
                    current_color = static_cast<short int>(plot_color - 9 + (static_cast<int>(current_color) + 1) % 19);
                    line->SetLineColor(current_color);
                    line->Draw("same");
                }
                lines.push_back(std::move(line));
            }

            // Draw and write canvas to module output file, then clear the stored lines
            canvas->Draw();
            module->getROOTDirectory()->WriteTObject(canvas.get());
        }

        /**
         * @brief Generate line graphs of charge carrier drift paths
         *
         * @param event_num Index for this event
         * @param module Module to generate plots for, used to create output files and to obtain ROOT directory
         * @param config Configuration object used for this module instance
         * @param output_plot_points List of points cached for plotting
         * carriers are plotted.
         */
        static void Animate(uint64_t event_num, // NOLINT
                            Module* module,
                            const Configuration& config,
                            const OutputPlotPoints& output_plot_points) {

            LOG(TRACE) << "Writing animation for all charge carriers";
            auto model = module->getDetector()->getModel();

            auto [minX, maxX, minY, maxY, scale_x, scale_y, max_charge, total_charge, tot_point_cnt, start_time] =
                get_plot_settings(model, config, output_plot_points);

            // Use a histogram to create the underlying frame
            auto* histogram_frame =
                new TH3F(("frame_" + module->getUniqueName() + "_" + std::to_string(event_num) + "_all").c_str(),
                         "",
                         10,
                         minX,
                         maxX,
                         10,
                         minY,
                         maxY,
                         10,
                         model->getSensorCenter().z() - model->getSensorSize().z() / 2.0,
                         model->getSensorCenter().z() + model->getSensorSize().z() / 2.0);
            histogram_frame->SetDirectory(module->getROOTDirectory());

            // Create canvas for GIF animition of process
            auto canvas = std::make_unique<TCanvas>(("animation_" + std::to_string(event_num) + "_all").c_str(),
                                                    ("Propagation of charge for event " + std::to_string(event_num)).c_str(),
                                                    1280,
                                                    1024);
            canvas->cd();

            // Change axis labels if close to zero or PI as they behave different here
            if(std::fabs(config.get<double>("output_plots_theta") / (ROOT::Math::Pi() / 2.0) -
                         std::round(config.get<double>("output_plots_theta") / (ROOT::Math::Pi() / 2.0))) < 1e-6 ||
               std::fabs(config.get<double>("output_plots_phi") / (ROOT::Math::Pi() / 2.0) -
                         std::round(config.get<double>("output_plots_phi") / (ROOT::Math::Pi() / 2.0))) < 1e-6) {
                histogram_frame->GetXaxis()->SetLabelOffset(-0.1f);
                histogram_frame->GetYaxis()->SetLabelOffset(-0.075f);
            } else {
                histogram_frame->GetXaxis()->SetTitleOffset(2.0f);
                histogram_frame->GetYaxis()->SetTitleOffset(2.0f);
            }

            // Draw frame on canvas
            histogram_frame->Draw();

            // Create the contour histogram
            std::vector<std::string> file_name_contour{};
            std::vector<TH2F*> histogram_contour{};
            file_name_contour.push_back(module->createOutputFile("contourX" + std::to_string(event_num) + "_all.gif"));
            histogram_contour.push_back(
                new TH2F(("contourX_" + module->getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                         "",
                         100,
                         minY,
                         maxY,
                         100,
                         model->getSensorCenter().z() - model->getSensorSize().z() / 2.0,
                         model->getSensorCenter().z() + model->getSensorSize().z() / 2.0));
            histogram_contour.back()->SetDirectory(module->getROOTDirectory());
            file_name_contour.push_back(module->createOutputFile("contourY" + std::to_string(event_num) + ".gif"));
            histogram_contour.push_back(
                new TH2F(("contourY_" + module->getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                         "",
                         100,
                         minX,
                         maxX,
                         100,
                         model->getSensorCenter().z() - model->getSensorSize().z() / 2.0,
                         model->getSensorCenter().z() + model->getSensorSize().z() / 2.0));
            histogram_contour.back()->SetDirectory(module->getROOTDirectory());
            file_name_contour.push_back(module->createOutputFile("contourZ" + std::to_string(event_num) + ".gif"));
            histogram_contour.push_back(
                new TH2F(("contourZ_" + module->getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                         "",
                         100,
                         minX,
                         maxX,
                         100,
                         minY,
                         maxY));
            histogram_contour.back()->SetDirectory(module->getROOTDirectory());

            // Create file and disable statistics for histogram
            std::string file_name_anim = module->createOutputFile("animation" + std::to_string(event_num) + ".gif");
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
                std::lround((Units::convert(config.get<long double>("output_plots_step"), "ms") / 10.0) *
                            config.get<long double>("output_animations_time_scaling", 1e9)));
            unsigned long plot_idx = 0;
            unsigned int point_cnt = 0;
            LOG_PROGRESS(INFO, module->getUniqueName() + "_OUTPUT_PLOTS")
                << "Written 0 of " << tot_point_cnt << " points for animation";
            while(point_cnt < tot_point_cnt) {
                std::vector<std::unique_ptr<TPolyMarker3D>> markers{};
                unsigned long min_idx_diff = std::numeric_limits<unsigned long>::max();

                // Reset the canvas
                canvas->Clear();
                canvas->SetTheta(config.get<float>("output_plots_theta") * 180.0f / ROOT::Math::Pi());
                canvas->SetPhi(config.get<float>("output_plots_phi") * 180.0f / ROOT::Math::Pi());
                canvas->Draw();

                // Reset the histogram frame
                histogram_frame->SetTitle("Charge propagation in sensor");
                histogram_frame->GetXaxis()->SetTitle(
                    (std::string("x ") + (config.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
                histogram_frame->GetYaxis()->SetTitle(
                    (std::string("y ") + (config.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
                histogram_frame->GetZaxis()->SetTitle("z (mm)");
                histogram_frame->Draw();

                auto text = std::make_unique<TPaveText>(-0.75, -0.75, -0.60, -0.65);
                auto time_ns = Units::convert(plot_idx * config.get<long double>("output_plots_step"), "ns");
                std::stringstream sstr;
                sstr << std::fixed << std::setprecision(2) << time_ns << "ns";
                auto time_str = std::string(8 - sstr.str().size(), ' ');
                time_str += sstr.str();
                text->AddText(time_str.c_str());
                text->Draw();

                // Plot all the required points
                for(const auto& [deposit, points] : output_plot_points) {
                    const auto& [time, charge, type, state] = deposit;

                    auto diff = static_cast<unsigned long>(
                        std::lround((time - start_time) / config.get<long double>("output_plots_step")));
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
                    marker->SetMarkerSize(
                        static_cast<float>(charge * config.get<double>("output_animations_marker_size", 1)) /
                        static_cast<float>(max_charge));
                    auto initial_z_perc = static_cast<int>(
                        ((points[0].z() + model->getSensorSize().z() / 2.0) / model->getSensorSize().z()) * 80);
                    initial_z_perc = std::max(std::min(79, initial_z_perc), 0);
                    if(config.get<bool>("output_animations_color_markers")) {
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
                                (std::string("y ") +
                                 (config.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)"))
                                    .c_str());
                            histogram_contour[i]->GetYaxis()->SetTitle("z (mm)");
                            break;
                        case 1 /* y */:
                            histogram_contour[i]->GetXaxis()->SetTitle(
                                (std::string("x ") +
                                 (config.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)"))
                                    .c_str());
                            histogram_contour[i]->GetYaxis()->SetTitle("z (mm)");
                            break;
                        case 2 /* z */:
                            histogram_contour[i]->GetXaxis()->SetTitle(
                                (std::string("x ") +
                                 (config.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)"))
                                    .c_str());
                            histogram_contour[i]->GetYaxis()->SetTitle(
                                (std::string("y ") +
                                 (config.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)"))
                                    .c_str());
                            break;
                        default:;
                        }
                        histogram_contour[i]->SetMinimum(1);
                        histogram_contour[i]->SetMaximum(total_charge /
                                                         config.get<double>("output_animations_contour_max_scaling", 10));
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

                LOG_PROGRESS(INFO, module->getUniqueName() + "_OUTPUT_PLOTS")
                    << "Written " << point_cnt << " of " << tot_point_cnt << " points for animation";
            }
        }

    private:
        static std::tuple<double, double, double, double, double, double, double, double, unsigned long, double>
        get_plot_settings(const std::shared_ptr<DetectorModel>& model,
                          const Configuration& config,
                          const OutputPlotPoints& output_plot_points) {

            // Convert to pixel units if necessary
            double scale_x = (config.get<bool>("output_plots_use_pixel_units") ? model->getPixelSize().x() : 1);
            double scale_y = (config.get<bool>("output_plots_use_pixel_units") ? model->getPixelSize().y() : 1);

            // Calculate the axis limits
            double minX = FLT_MAX, maxX = FLT_MIN;
            double minY = FLT_MAX, maxY = FLT_MIN;
            unsigned long tot_point_cnt = 0;
            double start_time = std::numeric_limits<double>::max();
            unsigned int total_charge = 0;
            unsigned int max_charge = 0;
            for(const auto& [deposit, points] : output_plot_points) {
                for(const auto& point : points) {
                    minX = std::min(minX, point.x() / scale_x);
                    maxX = std::max(maxX, point.x() / scale_x);

                    minY = std::min(minY, point.y() / scale_y);
                    maxY = std::max(maxY, point.y() / scale_y);
                }
                const auto& [time, charge, type, state] = deposit;
                start_time = std::min(start_time, time);
                total_charge += charge;
                max_charge = std::max(max_charge, charge);

                tot_point_cnt += points.size();
            }

            // Compute frame axis sizes if equal scaling is requested
            if(config.get<bool>("output_plots_use_equal_scaling", true)) {
                double centerX = (minX + maxX) / 2.0;
                double centerY = (minY + maxY) / 2.0;
                if(config.get<bool>("output_plots_use_pixel_units")) {
                    minX = centerX - model->getSensorSize().z() / model->getPixelSize().x() / 2.0;
                    maxX = centerX + model->getSensorSize().z() / model->getPixelSize().x() / 2.0;

                    minY = centerY - model->getSensorSize().z() / model->getPixelSize().y() / 2.0;
                    maxY = centerY + model->getSensorSize().z() / model->getPixelSize().y() / 2.0;
                } else {
                    minX = centerX - model->getSensorSize().z() / 2.0;
                    maxX = centerX + model->getSensorSize().z() / 2.0;

                    minY = centerY - model->getSensorSize().z() / 2.0;
                    maxY = centerY + model->getSensorSize().z() / 2.0;
                }
            }

            // Align on pixels if requested
            if(config.get<bool>("output_plots_align_pixels")) {
                if(config.get<bool>("output_plots_use_pixel_units")) {
                    minX = std::floor(minX - 0.5) + 0.5;
                    minY = std::floor(minY + 0.5) - 0.5;
                    maxX = std::ceil(maxX - 0.5) + 0.5;
                    maxY = std::ceil(maxY + 0.5) - 0.5;
                } else {
                    double div = minX / model->getPixelSize().x();
                    minX = (std::floor(div - 0.5) + 0.5) * model->getPixelSize().x();
                    div = minY / model->getPixelSize().y();
                    minY = (std::floor(div - 0.5) + 0.5) * model->getPixelSize().y();
                    div = maxX / model->getPixelSize().x();
                    maxX = (std::ceil(div + 0.5) - 0.5) * model->getPixelSize().x();
                    div = maxY / model->getPixelSize().y();
                    maxY = (std::ceil(div + 0.5) - 0.5) * model->getPixelSize().y();
                }
            }

            return {minX, maxX, minY, maxY, scale_x, scale_y, max_charge, total_charge, tot_point_cnt, start_time};
        };
    };
} // namespace allpix

#endif /* ALLPIX_LINE_GRAPHS_H */
