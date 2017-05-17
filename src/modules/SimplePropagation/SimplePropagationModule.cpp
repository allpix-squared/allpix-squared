/**
 * @author Paul Schuetze <paul.schuetze@desy.de>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "SimplePropagationModule.hpp"

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
#include <TH3F.h>
#include <TPolyLine3D.h>
#include <TPolyMarker3D.h>

#include <iostream>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/random.h"
#include "core/utils/unit.h"
#include "tools/runge_kutta.h"

#include "objects/PropagatedCharge.hpp"

using namespace allpix;
using namespace ROOT::Math;

SimplePropagationModule::SimplePropagationModule(Configuration config,
                                                 Messenger* messenger,
                                                 std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), messenger_(messenger), detector_(std::move(detector)),
      debug_file_(nullptr) {
    // get detector model
    model_ = detector_->getModel();

    // require deposits message for single detector
    messenger_->bindSingle(this, &SimplePropagationModule::deposits_message_, MsgFlags::REQUIRED);

    // seed the random generator
    random_generator_.seed(get_random_seed());

    // set defaults for config variables
    config_.setDefault<double>("spatial_precision", Units::get(0.1, "nm"));
    config_.setDefault<double>("timestep_start", Units::get(0.01, "ns"));
    config_.setDefault<double>("timestep_min", Units::get(0.0005, "ns"));
    config_.setDefault<double>("timestep_max", Units::get(0.1, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 10);
    config_.setDefault<bool>("debug_plots", false);
}

// init debug plots
void SimplePropagationModule::init() {
    if(config_.get<bool>("debug_plots")) {
        LOG(DEBUG) << "DEBUG PLOT ENABLED";
        std::string file_name = getOutputPath(config_.get<std::string>("debug_plots_file_name", "debug_plots") + ".root");
        debug_file_ = new TFile(file_name.c_str(), "RECREATE");
    }
}

void SimplePropagationModule::create_debug_plots(unsigned int event_num) {
    LOG(DEBUG) << "Writing debug plots";

    // goto the file
    debug_file_->cd();

    // calculate the axis limits
    double minX = FLT_MAX, maxX = FLT_MIN;
    double minY = FLT_MAX, maxY = FLT_MIN;
    unsigned long tot_point_cnt = 0;
    long double start_time = std::numeric_limits<long double>::max();
    for(auto& time_points : debug_plot_points_) {
        for(auto& point : time_points.second) {
            minX = std::min(minX, point.x());
            maxX = std::max(maxX, point.x());

            minY = std::min(minY, point.y());
            maxY = std::max(maxY, point.y());
        }
        start_time = std::min(start_time, time_points.first);

        tot_point_cnt += time_points.second.size();
    }

    // create a frame for proper axis alignment
    TH3F* histogram_frame = new TH3F(("frame_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                                     "",
                                     10,
                                     minX,
                                     maxX,
                                     10,
                                     minY,
                                     maxY,
                                     10,
                                     model_->getSensorMinZ(),
                                     model_->getSensorMinZ() + model_->getSensorSizeZ());
    histogram_frame->GetXaxis()->SetLabelOffset(-0.1f);
    histogram_frame->GetYaxis()->SetLabelOffset(-0.1f);
    histogram_frame->SetStats(false);

    // create the main canvas
    auto* canvas = new TCanvas(("line_plot_" + std::to_string(event_num)).c_str(),
                               ("Propagation of charge for event " + std::to_string(event_num)).c_str());
    canvas->cd();
    canvas->SetTheta(config_.get("debug_plots_theta", 0.0f) * 180.0f / Pi());
    canvas->SetPhi(config_.get("debug_plots_phi", 0.0f) * 180.0f / Pi());

    // draw the frame
    histogram_frame->Draw();

    // loop over all point sets
    std::vector<TPolyLine3D*> lines;
    short color = 0;

    for(auto& time_points : debug_plot_points_) {
        auto line = new TPolyLine3D;
        for(auto& point : time_points.second) {
            line->SetNextPoint(point.x(), point.y(), point.z());
        }
        // plot all lines with at least three points with different color
        if(line->GetN() >= 3) {
            line->SetLineColor(color);
            line->Draw("same");
            color = static_cast<short>((static_cast<int>(color) + 10) % 101);
        }
        lines.push_back(line);
    }

    // draw and write canvas to file
    canvas->Draw();
    canvas->Write();

    // delete and clear variables
    for(auto& line : lines) {
        delete line;
    }
    delete canvas;

    // create gif animation of process
    canvas = new TCanvas(("animation_" + std::to_string(event_num)).c_str(),
                         ("Propagation of charge for event " + std::to_string(event_num)).c_str());
    canvas->cd();
    canvas->SetTheta(config_.get("debug_plots_theta", 0.0f) * 180.0f / Pi());
    canvas->SetPhi(config_.get("debug_plots_phi", 0.0f) * 180.0f / Pi());

    // draw frame
    histogram_frame->Draw();

    // plot points
    std::string file_name = getOutputPath("animation" + std::to_string(event_num) + ".gif");
    try {
        remove_path(file_name);
    } catch(std::invalid_argument&) {
        throw ModuleError("Cannot overwite gif animation");
    }

    auto animation_time =
        static_cast<unsigned int>(std::round((Units::convert(config_.get<long double>("debug_plots_step"), "ms") / 10.0) *
                                             config_.get<long double>("debug_plots_animation_time_scaling", 1e9)));
    unsigned long plot_idx = 0;
    unsigned int point_cnt = 0;
    while(point_cnt < tot_point_cnt) {
        TPolyMarker3D markers;
        unsigned long min_idx_diff = std::numeric_limits<unsigned long>::max();

        for(auto& time_points : debug_plot_points_) {
            auto points = time_points.second;

            auto diff = static_cast<unsigned long>(
                std::round((time_points.first - start_time) / config_.get<long double>("debug_plots_step")));
            if(static_cast<long>(plot_idx) - static_cast<long>(diff) < 0) {
                min_idx_diff = std::min(min_idx_diff, diff - plot_idx);
                continue;
            }
            auto idx = plot_idx - diff;
            if(idx >= points.size()) {
                continue;
            }
            min_idx_diff = 0;

            markers.SetNextPoint(points[idx].x(), points[idx].y(), points[idx].z());
            ++point_cnt;
        }

        if(min_idx_diff != 0) {
            canvas->Print((file_name + "+100").c_str());
            plot_idx += min_idx_diff;
        } else {
            markers.SetMarkerStyle(kFullCircle);
            markers.Draw();
            if(point_cnt < tot_point_cnt - 1) {
                canvas->Print((file_name + "+" + std::to_string(animation_time)).c_str());
            } else {
                canvas->Print((file_name + "++500").c_str());
            }
            ++plot_idx;
        }
    }
    delete canvas;
}

// run the propagation
void SimplePropagationModule::run(unsigned int event_num) {
    // create vector of propagated charges
    std::vector<PropagatedCharge> propagated_charges;

    // propagate all deposits
    LOG(INFO) << "Propagating charges in sensor";
    for(auto& deposit : deposits_message_->getData()) {
        // loop over all charges
        unsigned int electrons_remaining = deposit.getCharge();

        LOG(DEBUG) << "set of charges on " << deposit.getPosition();

        auto charge_per_step = config_.get<unsigned int>("charge_per_step");
        while(electrons_remaining > 0) {
            // define number of charges to be propagated and remove electrons of this step from the total
            if(charge_per_step > electrons_remaining) {
                charge_per_step = electrons_remaining;
            }
            electrons_remaining -= charge_per_step;

            // get position and propagate through sensor
            auto position = deposit.getPosition(); // NOTE: this is already a local position

            if(config_.get<bool>("debug_plots")) {
                debug_plot_points_.emplace_back(deposit.getEventTime(), std::vector<ROOT::Math::XYZPoint>());
            }

            // propagate a single charge deposit
            auto prop_pair = propagate(position);
            position = prop_pair.first;

            LOG(DEBUG) << " propagated " << charge_per_step << " to " << position << " in " << prop_pair.second << " time";

            // create a new propagated charge and add it to the list
            PropagatedCharge propagated_charge(position, charge_per_step, deposit.getEventTime() + prop_pair.second);
            propagated_charges.push_back(propagated_charge);
        }
    }

    // write debug plots if required
    if(config_.get<bool>("debug_plots")) {
        create_debug_plots(event_num);
    }

    // create a new message with propagated charges
    PropagatedChargeMessage propagated_charge_message(std::move(propagated_charges), detector_);

    // dispatch the message
    messenger_->dispatchMessage(propagated_charge_message, "implant");
}

std::pair<XYZPoint, double> SimplePropagationModule::propagate(const XYZPoint& root_pos) {
    // create a runge kutta solver using the electric field as step function
    Eigen::Vector3d position(root_pos.x(), root_pos.y(), root_pos.z());

    // define a function to compute the electron mobility
    auto electron_mobility = [&](double efield_mag) {
        /* Reference: https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2) */
        // variables for charge mobility
        auto temperature = config_.get<double>("temperature");
        double electron_Vm = Units::get(1.53e9 * std::pow(temperature, -0.87), "cm/s");
        double electron_Ec = Units::get(1.01 * std::pow(temperature, 1.55), "V/cm");
        double electron_Beta = 2.57e-2 * std::pow(temperature, 0.66);

        // compute electron mobility
        double numerator = electron_Vm / electron_Ec;
        double denominator = std::pow(1. + std::pow(efield_mag / electron_Ec, electron_Beta), 1.0 / electron_Beta);
        return numerator / denominator;
    };

    // define a function to compute the diffusion
    auto boltzmann_kT = Units::get(8.6173e-5, "eV/K") * config_.get<double>("temperature");
    auto timestep = config_.get<double>("timestep_start");
    auto electron_diffusion = [&](double efield_mag) -> Eigen::Vector3d {
        double diffusion_constant = boltzmann_kT * electron_mobility(efield_mag);
        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * timestep);

        std::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
        Eigen::Vector3d diffusion;
        for(int i = 0; i < 3; ++i) {
            diffusion[i] = gauss_distribution(random_generator_);
        }
        return diffusion;
    };

    // define a function to compute the electron velocity
    auto electron_velocity = [&](double, Eigen::Vector3d pos) -> Eigen::Vector3d {
        // get the electric field
        double* raw_field = detector_->getElectricFieldRaw(pos);
        if(raw_field == nullptr) {
            // return a zero electric field outside of the sensor
            return Eigen::Vector3d(0, 0, 0);
        }
        // compute the drift velocity
        auto efield = static_cast<Eigen::Map<Eigen::Vector3d>>(raw_field);
        return electron_mobility(efield.norm()) * (efield);
    };

    // build the runge kutta solver with an RKF5 tableau
    auto runge_kutta = make_runge_kutta(tableau::RK5, electron_velocity, timestep, position);

    // continue until outside the sensor (no electric field)
    // FIXME: we need to determine what would be a good time to stop
    double last_time = std::numeric_limits<double>::lowest();
    while(true) {
        // update debug plots if necessary
        if(config_.get<bool>("debug_plots") && runge_kutta.getTime() - last_time > config_.get<double>("debug_plots_step")) {
            debug_plot_points_.back().second.push_back(static_cast<XYZPoint>(runge_kutta.getValue()));
            last_time = runge_kutta.getTime();
        }

        // do a runge kutta step
        auto step = runge_kutta.step();

        // get the current result and timestep
        timestep = runge_kutta.getTimeStep();
        position = runge_kutta.getValue();

        // get electric field at current position and stop if field does not exist (outside sensor)
        double* raw_field = detector_->getElectricFieldRaw(position);
        if(raw_field == nullptr) {
            break;
        }

        // apply diffusion step
        auto efield = static_cast<Eigen::Map<Eigen::Vector3d>>(raw_field);
        auto diffusion = electron_diffusion(efield.norm());
        runge_kutta.setValue(position + diffusion);

        // adapt step size to precision
        double uncertainty = step.error.norm();
        auto target_spatial_precision = config_.get<double>("spatial_precision");
        if(model_->getSensorSizeZ() - position.z() < step.value.z() * 1.2) {
            timestep *= 0.7;
        } else {
            if(uncertainty > target_spatial_precision) {
                timestep *= 0.7;
            } else if(uncertainty < 0.5 * target_spatial_precision) {
                timestep *= 2;
            }
        }
        if(timestep > config_.get<double>("timestep_max")) {
            timestep = config_.get<double>("timestep_max");
        } else if(timestep < config_.get<double>("timestep_min")) {
            timestep = config_.get<double>("timestep_min");
        }
        runge_kutta.setTimeStep(timestep);
    }

    position = runge_kutta.getValue();
    return std::make_pair(static_cast<XYZPoint>(position), runge_kutta.getTime());
}

// write debug plots
void SimplePropagationModule::finalize() {
    if(config_.get<bool>("debug_plots")) {
        debug_file_->Close();
        delete debug_file_;
    }
}
