/**
 * @file
 * @brief Implementation of generic charge propagation module
 * @remarks Based on code from Paul Schuetze
 * @copyright MIT License
 */

#include "GenericPropagationModule.hpp"

#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>

#include <Eigen/Core>

#include <Math/Point3D.h>
#include <Math/Vector3D.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH2F.h>
#include <TH3F.h>
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
GenericPropagationModule::GenericPropagationModule(Configuration config,
                                                   Messenger* messenger,
                                                   std::shared_ptr<Detector> detector)
    : Module(config, detector), config_(std::move(config)), messenger_(messenger), detector_(std::move(detector)) {
    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector
    messenger_->bindSingle(this, &GenericPropagationModule::deposits_message_, MsgFlags::REQUIRED);

    // Seed the random generator with the module seed
    random_generator_.seed(getRandomSeed());

    // Set default value for config variables
    config_.setDefault<double>("spatial_precision", Units::get(0.1, "nm"));
    config_.setDefault<double>("timestep_start", Units::get(0.01, "ns"));
    config_.setDefault<double>("timestep_min", Units::get(0.0005, "ns"));
    config_.setDefault<double>("timestep_max", Units::get(0.1, "ns"));
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 10);

    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<double>("output_plots_step", config_.get<double>("timestep_max"));
    config_.setDefault<bool>("output_plots_use_pixel_units", false);
    config_.setDefault<double>("output_plots_theta", 0.0f);
    config_.setDefault<double>("output_plots_phi", 0.0f);

    // Copy some variables from configuration to avoid lookups:
    temperature_ = config_.get<double>("temperature");
    timestep_min_ = config_.get<double>("timestep_min");
    timestep_max_ = config_.get<double>("timestep_max");
    timestep_start_ = config_.get<double>("timestep_start");
    integration_time_ = config_.get<double>("integration_time");
    target_spatial_precision_ = config_.get<double>("spatial_precision");
    output_plots_ = config_.get<bool>("output_plots");
    output_plots_step_ = config_.get<double>("output_plots_step");

    // Parameterization variables from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2)
    electron_Vm_ = Units::get(1.53e9 * std::pow(temperature_, -0.87), "cm/s");
    electron_Ec_ = Units::get(1.01 * std::pow(temperature_, 1.55), "V/cm");
    electron_Beta_ = 2.57e-2 * std::pow(temperature_, 0.66);

    boltzmann_kT_ = Units::get(8.6173e-5, "eV/K") * temperature_;
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
    for(auto& deposit_points : output_plot_points_) {
        for(auto& point : deposit_points.second) {
            minX = std::min(minX, point.x());
            maxX = std::max(maxX, point.x());

            minY = std::min(minY, point.y());
            maxY = std::max(maxY, point.y());
        }
        start_time = std::min(start_time, deposit_points.first.getEventTime());
        total_charge += deposit_points.first.getCharge();

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
    std::vector<std::unique_ptr<TPolyLine3D>> lines;
    short current_color = 1;
    std::vector<short> colors;
    for(auto& deposit_points : output_plot_points_) {
        auto line = std::make_unique<TPolyLine3D>();
        for(auto& point : deposit_points.second) {
            line->SetNextPoint(point.x(), point.y(), point.z());
        }
        // Plot all lines with at least three points with different color
        if(line->GetN() >= 3) {
            line->SetLineColor(current_color);
            colors.push_back(current_color);
            line->Draw("same");
            current_color = static_cast<short>((static_cast<int>(current_color) + 10) % 101);
        } else {
            colors.push_back(0);
        }
        lines.push_back(std::move(line));
    }

    // Draw and write canvas to module output file
    canvas->Draw();
    canvas->Write();
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

    // Create the contour histogram
    std::vector<std::string> file_name_contour;
    std::vector<TH2F*> histogram_contour;
    file_name_contour.push_back(getOutputPath("contourX" + std::to_string(event_num) + ".gif"));
    histogram_contour.push_back(new TH2F(("contourX_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                                         "",
                                         100,
                                         minY,
                                         maxY,
                                         100,
                                         model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0,
                                         model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0));
    file_name_contour.push_back(getOutputPath("contourY" + std::to_string(event_num) + ".gif"));
    histogram_contour.push_back(new TH2F(("contourY_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                                         "",
                                         100,
                                         minX,
                                         maxX,
                                         100,
                                         model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0,
                                         model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0));
    file_name_contour.push_back(getOutputPath("contourZ" + std::to_string(event_num) + ".gif"));
    histogram_contour.push_back(new TH2F(
        ("contourZ_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(), "", 100, minX, maxX, 100, minY, maxY));

    // Delete previous GIF output files
    std::string file_name_anim = getOutputPath("animation" + std::to_string(event_num) + ".gif");
    try {
        remove_path(file_name_anim);
        for(size_t i = 0; i < 3; ++i) {
            remove_path(file_name_contour[i]);
            histogram_contour[i]->SetStats(false);
        }
    } catch(std::invalid_argument&) {
        throw ModuleError("Cannot overwite gif animation");
    }

    // Create animation of moving charges
    auto animation_time =
        static_cast<unsigned int>(std::round((Units::convert(config_.get<long double>("output_plots_step"), "ms") / 10.0) *
                                             config_.get<long double>("output_plots_animation_time_scaling", 1e9)));
    unsigned long plot_idx = 0;
    unsigned int point_cnt = 0;
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
            marker->SetMarkerSize(static_cast<float>(deposit_points.first.getCharge()) /
                                  config_.get<float>("charge_per_step"));
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
                canvas->SetTitle(
                    (std::string("Contour of charge propagation projected on the ") + static_cast<char>('X' + i) + "-axis")
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
                histogram_contour[i]->SetMaximum(total_charge / config_.get<double>("output_plots_contour_max_scaling", 10));
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

        LOG_PROGRESS(DEBUG, getUniqueName() + "_OUTPUT_PLOTS")
            << "Written " << point_cnt << " of " << tot_point_cnt << " points";
    }
    output_plot_points_.clear();
}

void GenericPropagationModule::run(unsigned int event_num) {
    // Check for electric field and output warning for slow propagation if not defined
    if(!getDetector()->hasElectricField()) {
        LOG(WARNING) << "Running this module without an electric field is not recommmended and can be very slow!";
    }

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;

    // Loop over all deposits for propagation
    LOG(TRACE) << "Propagating charges in sensor";
    unsigned int propagated_charges_count = 0;
    unsigned int step_count = 0;
    long double total_time = 0;
    for(auto& deposit : deposits_message_->getData()) {
        // Loop over all charges in the deposit
        unsigned int electrons_remaining = deposit.getCharge();

        LOG(DEBUG) << "Set of charges on " << display_vector(deposit.getLocalPosition(), {"mm", "um"});

        auto charge_per_step = config_.get<unsigned int>("charge_per_step");
        while(electrons_remaining > 0) {
            // Define number of charges to be propagated and remove electrons of this step from the total
            if(charge_per_step > electrons_remaining) {
                charge_per_step = electrons_remaining;
            }
            electrons_remaining -= charge_per_step;

            // Get position and propagate through sensor
            auto position = deposit.getLocalPosition();

            // Add point of deposition to the output plots if requested
            if(config_.get<bool>("output_plots")) {
                auto global_position = detector_->getGlobalPosition(position);
                output_plot_points_.emplace_back(
                    PropagatedCharge(position, global_position, charge_per_step, deposit.getEventTime()),
                    std::vector<ROOT::Math::XYZPoint>());
            }

            // Propagate a single charge deposit
            auto prop_pair = propagate(position);
            position = prop_pair.first;

            LOG(DEBUG) << " Propagated " << charge_per_step << " to " << display_vector(position, {"mm", "um"}) << " in "
                       << Units::display(prop_pair.second, "ns") << " time";

            // Create a new propagated charge and add it to the list
            auto global_position = detector_->getGlobalPosition(position);
            propagated_charges.emplace_back(
                position, global_position, charge_per_step, deposit.getEventTime() + prop_pair.second, &deposit);

            // Update statistical information
            ++step_count;
            propagated_charges_count += charge_per_step;
            total_time += prop_pair.second;
        }
    }

    // Output plots if required
    if(config_.get<bool>("output_plots")) {
        create_output_plots(event_num);
    }

    // Write summary and update statistics
    long double average_time = total_time / std::max(1u, step_count);
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
std::pair<ROOT::Math::XYZPoint, double> GenericPropagationModule::propagate(const ROOT::Math::XYZPoint& pos) {
    // Create a runge kutta solver using the electric field as step function
    Eigen::Vector3d position(pos.x(), pos.y(), pos.z());

    // Define a lambda function to compute the electron mobility
    // NOTE This function is typically the most frequently executed part of the framework and therefore the bottleneck
    auto electron_mobility = [&](double efield_mag) {
        // Compute electron mobility from constants and electric field magnitude
        double numerator = electron_Vm_ / electron_Ec_;
        double denominator = std::pow(1. + std::pow(efield_mag / electron_Ec_, electron_Beta_), 1.0 / electron_Beta_);
        return numerator / denominator;
    };

    // Define a function to compute the diffusion
    auto timestep = timestep_start_;
    auto electron_diffusion = [&](double efield_mag) -> Eigen::Vector3d {
        double diffusion_constant = boltzmann_kT_ * electron_mobility(efield_mag);
        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * timestep);

        // Compute the independent diffusion in three
        std::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
        Eigen::Vector3d diffusion;
        for(int i = 0; i < 3; ++i) {
            diffusion[i] = gauss_distribution(random_generator_);
        }
        return diffusion;
    };

    // Define a lambda function to compute the electron velocity
    auto electron_velocity = [&](double, Eigen::Vector3d cur_pos) -> Eigen::Vector3d {
        double* raw_field = detector_->getElectricFieldRaw(cur_pos);
        if(raw_field == nullptr) {
            // Return a zero electric field outside of the sensor
            return Eigen::Vector3d(0, 0, 0);
        }
        // Compute the drift velocity
        auto efield = static_cast<Eigen::Map<Eigen::Vector3d>>(raw_field);
        return -electron_mobility(efield.norm()) * efield;
    };

    // Create the runge kutta solver with an RKF5 tableau
    auto runge_kutta = make_runge_kutta(tableau::RK5, electron_velocity, timestep, position);

    // Continue propagation until the deposit is outside the sensor
    // FIXME: we need to determine what would be a good time to stop
    double zero_vec[] = {0, 0, 0};
    double last_time = std::numeric_limits<double>::lowest();
    while(detector_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position)) &&
          runge_kutta.getTime() < integration_time_) {
        // Update output plots if necessary (depending on the plot step)
        if(output_plots_ && runge_kutta.getTime() - last_time > output_plots_step_) {
            output_plot_points_.back().second.push_back(static_cast<ROOT::Math::XYZPoint>(runge_kutta.getValue()));
            last_time = runge_kutta.getTime();
        }

        // Execute a Runge Kutta step
        auto step = runge_kutta.step();

        // Get the current result and timestep
        timestep = runge_kutta.getTimeStep();
        position = runge_kutta.getValue();

        // Get electric field at current position and fall back to empty field if it does not exist
        double* raw_field = detector_->getElectricFieldRaw(position);
        if(raw_field == nullptr) {
            raw_field = zero_vec;
        }

        // Apply diffusion step
        auto efield = static_cast<Eigen::Map<Eigen::Vector3d>>(raw_field);
        auto diffusion = electron_diffusion(efield.norm());
        runge_kutta.setValue(position + diffusion);

        // Adapt step size to match target precision
        double uncertainty = step.error.norm();
        // Lower timestep when reaching the sensor edge
        if(model_->getSensorSize().z() - position.z() < step.value.z() * 1.2) {
            timestep *= 0.7;
        } else {
            if(uncertainty > target_spatial_precision_) {
                timestep *= 0.7;
            } else if(uncertainty < 0.5 * target_spatial_precision_) {
                timestep *= 2;
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

    // Return the final position of the propagated charge
    position = runge_kutta.getValue();
    return std::make_pair(static_cast<ROOT::Math::XYZPoint>(position), runge_kutta.getTime());
}

void GenericPropagationModule::finalize() {
    long double average_time = total_time_ / std::max(1u, total_steps_);
    LOG(INFO) << "Propagated total of " << total_propagated_charges_ << " charges in " << total_steps_
              << " steps in average time of " << Units::display(average_time, "ns");
}
