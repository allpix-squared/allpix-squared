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
#include <TH2F.h>
#include <TH3F.h>
#include <TPolyLine3D.h>
#include <TPolyMarker3D.h>
#include <TStyle.h>

#include <iostream>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/random.h"
#include "core/utils/unit.h"
#include "tools/runge_kutta.h"

#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

using namespace allpix;
using namespace ROOT::Math;

SimplePropagationModule::SimplePropagationModule(Configuration config,
                                                 Messenger* messenger,
                                                 std::shared_ptr<Detector> detector)
    : Module(config, detector), config_(std::move(config)), messenger_(messenger), detector_(std::move(detector)),
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

    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<bool>("output_plots_use_pixel_units", false);
    config_.setDefault<double>("output_plots_theta", 0.0f);
    config_.setDefault<double>("output_plots_phi", 0.0f);
}

// init debug plots
void SimplePropagationModule::init() {
    if(config_.get<bool>("output_plots")) {
        std::string file_name = getOutputPath(config_.get<std::string>("output_plots_file_name", "output_plots") + ".root");
        debug_file_ = new TFile(file_name.c_str(), "RECREATE");
    }
}

void SimplePropagationModule::create_output_plots(unsigned int event_num) {
    LOG(DEBUG) << "Writing debug plots";

    // enable prefer GL
    gStyle->SetCanvasPreferGL(kTRUE);

    // goto the file
    debug_file_->cd();

    // convert to pixel units if necessary
    if(config_.get<bool>("output_plots_use_pixel_units")) {
        for(auto& deposit_points : debug_plot_points_) {
            for(auto& point : deposit_points.second) {
                point.SetX((point.x() / model_->getPixelSizeX()) + 1);
                point.SetY((point.y() / model_->getPixelSizeY()) + 1);
            }
        }
    }

    // calculate the axis limits
    double minX = FLT_MAX, maxX = FLT_MIN;
    double minY = FLT_MAX, maxY = FLT_MIN;
    unsigned long tot_point_cnt = 0;
    long double start_time = std::numeric_limits<long double>::max();
    unsigned int total_charge = 0;
    for(auto& deposit_points : debug_plot_points_) {
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

    // create a frame for proper axis alignment
    double centerX = (minX + maxX) / 2.0;
    double centerY = (minY + maxY) / 2.0;

    // use equal axis if specified
    if(config_.get<bool>("output_plots_use_equal_scaling", true)) {
        if(config_.get<bool>("output_plots_use_pixel_units")) {
            minX = centerX - model_->getSensorSizeZ() / model_->getPixelSizeX() / 2.0;
            maxX = centerX + model_->getSensorSizeZ() / model_->getPixelSizeY() / 2.0;

            minY = centerY - model_->getSensorSizeZ() / model_->getPixelSizeX() / 2.0;
            maxY = centerY + model_->getSensorSizeZ() / model_->getPixelSizeY() / 2.0;
        } else {
            minX = centerX - model_->getSensorSizeZ() / 2.0;
            maxX = centerX + model_->getSensorSizeZ() / 2.0;

            minY = centerY - model_->getSensorSizeZ() / 2.0;
            maxY = centerY + model_->getSensorSizeZ() / 2.0;
        }
    }

    // create global frame
    auto* histogram_frame = new TH3F(("frame_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(),
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

    // create the line plot canvas
    auto canvas = std::make_unique<TCanvas>(("line_plot_" + std::to_string(event_num)).c_str(),
                                            ("Propagation of charge for event " + std::to_string(event_num)).c_str(),
                                            1280,
                                            1024);
    canvas->cd();
    canvas->SetTheta(config_.get<float>("output_plots_theta") * 180.0f / Pi());
    canvas->SetPhi(config_.get<float>("output_plots_phi") * 180.0f / Pi());

    // draw the frame
    histogram_frame->GetXaxis()->SetTitle(
        (std::string("x ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
    histogram_frame->GetYaxis()->SetTitle(
        (std::string("y ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
    histogram_frame->GetZaxis()->SetTitle("z (mm)");
    histogram_frame->Draw();

    // loop over all point sets
    std::vector<std::unique_ptr<TPolyLine3D>> lines;
    short current_color = 1;
    std::vector<short> colors;
    for(auto& deposit_points : debug_plot_points_) {
        auto line = std::make_unique<TPolyLine3D>();
        for(auto& point : deposit_points.second) {
            line->SetNextPoint(point.x(), point.y(), point.z());
        }
        // plot all lines with at least three points with different color
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

    // draw and write canvas to file
    canvas->Draw();
    canvas->Write();
    lines.clear();

    // create gif animation of process
    canvas = std::make_unique<TCanvas>(("animation_" + std::to_string(event_num)).c_str(),
                                       ("Propagation of charge for event " + std::to_string(event_num)).c_str(),
                                       1280,
                                       1024);
    canvas->cd();

    // change axis labels if close to zero or pi as they look different here
    if(std::fabs(config_.get<double>("output_plots_theta") / (Pi() / 2.0) -
                 std::round(config_.get<double>("output_plots_theta") / (Pi() / 2.0))) < 1e-6 ||
       std::fabs(config_.get<double>("output_plots_phi") / (Pi() / 2.0) -
                 std::round(config_.get<double>("output_plots_phi") / (Pi() / 2.0))) < 1e-6) {
        histogram_frame->GetXaxis()->SetLabelOffset(-0.1f);
        histogram_frame->GetYaxis()->SetLabelOffset(-0.075f);
    } else {
        histogram_frame->GetXaxis()->SetTitleOffset(2.0f);
        histogram_frame->GetYaxis()->SetTitleOffset(2.0f);
    }

    // draw frame
    histogram_frame->Draw();

    // create the contour histogram
    std::vector<std::string> file_name_contour;
    std::vector<TH2F*> histogram_contour;
    file_name_contour.push_back(getOutputPath("contourX" + std::to_string(event_num) + ".gif"));
    histogram_contour.push_back(new TH2F(("contourX_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                                         "",
                                         100,
                                         minY,
                                         maxY,
                                         100,
                                         model_->getSensorMinZ(),
                                         model_->getSensorMinZ() + model_->getSensorSizeZ()));
    file_name_contour.push_back(getOutputPath("contourY" + std::to_string(event_num) + ".gif"));
    histogram_contour.push_back(new TH2F(("contourY_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(),
                                         "",
                                         100,
                                         minX,
                                         maxX,
                                         100,
                                         model_->getSensorMinZ(),
                                         model_->getSensorMinZ() + model_->getSensorSizeZ()));
    file_name_contour.push_back(getOutputPath("contourZ" + std::to_string(event_num) + ".gif"));
    histogram_contour.push_back(new TH2F(
        ("contourZ_" + getUniqueName() + "_" + std::to_string(event_num)).c_str(), "", 100, minX, maxX, 100, minY, maxY));

    // delete previous output files
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

    // create animation of moving charges
    auto animation_time =
        static_cast<unsigned int>(std::round((Units::convert(config_.get<long double>("output_plots_step"), "ms") / 10.0) *
                                             config_.get<long double>("output_plots_animation_time_scaling", 1e9)));
    unsigned long plot_idx = 0;
    unsigned int point_cnt = 0;
    while(point_cnt < tot_point_cnt) {
        std::vector<std::unique_ptr<TPolyMarker3D>> markers;
        unsigned long min_idx_diff = std::numeric_limits<unsigned long>::max();

        canvas->Clear();
        // TPad pad; //"pad", "pad", 0.05, 0.05, 0.95, 0.95);
        canvas->SetTheta(config_.get<float>("output_plots_theta") * 180.0f / Pi());
        canvas->SetPhi(config_.get<float>("output_plots_phi") * 180.0f / Pi());
        canvas->Draw();
        // pad.cd();
        histogram_frame->SetTitle("Charge propagation in sensor");
        histogram_frame->GetXaxis()->SetTitle(
            (std::string("x ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
        histogram_frame->GetYaxis()->SetTitle(
            (std::string("y ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)")).c_str());
        histogram_frame->GetZaxis()->SetTitle("z (mm)");
        histogram_frame->Draw();

        for(auto& deposit_points : debug_plot_points_) {
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

            // draw and print contour histograms
            for(size_t i = 0; i < 3; ++i) {
                canvas->Clear();
                canvas->SetTitle(
                    (std::string("Contour of charge propagation projected on the ") + static_cast<char>('X' + i) + "-axis")
                        .c_str());
                // histogram_contour[i]->GetXaxis()->SetTitleOffset(1.25f);
                // histogram_contour[i]->GetYaxis()->SetTitleOffset(1.25f);
                switch(i) {
                case 0 /* x */:
                    histogram_contour[i]->GetXaxis()->SetTitle(
                        (std::string("x ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)"))
                            .c_str());
                    histogram_contour[i]->GetYaxis()->SetTitle("z (mm)");
                    break;
                case 1 /* y */:
                    histogram_contour[i]->GetXaxis()->SetTitle(
                        (std::string("y ") + (config_.get<bool>("output_plots_use_pixel_units") ? "(pixels)" : "(mm)"))
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

        LOG(DEBUG) << "Written " << point_cnt << " of " << tot_point_cnt << " points";
    }
}

// run the propagation
void SimplePropagationModule::run(unsigned int event_num) {
    // check for electric field
    if(!getDetector()->hasElectricField()) {
        LOG(WARNING) << "Running this module without an electric field is not recommmended and can be very slow!";
    }

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

            if(config_.get<bool>("output_plots")) {
                debug_plot_points_.emplace_back(PropagatedCharge(position, charge_per_step, deposit.getEventTime()),
                                                std::vector<ROOT::Math::XYZPoint>());
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
    if(config_.get<bool>("output_plots")) {
        create_output_plots(event_num);
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
    double zero_vec[] = {0, 0, 0};
    double last_time = std::numeric_limits<double>::lowest();
    while(detector_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position))) {
        // update debug plots if necessary
        if(config_.get<bool>("output_plots") &&
           runge_kutta.getTime() - last_time > config_.get<double>("output_plots_step")) {
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
            raw_field = zero_vec;
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
    if(config_.get<bool>("output_plots")) {
        debug_file_->Close();
        delete debug_file_;
    }
}
