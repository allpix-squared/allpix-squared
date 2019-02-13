/**
 * @file
 * @brief Implementation of charge propagation module with transient behavior simulation
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "TransientPropagationModule.hpp"

#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>

#include <Eigen/Core>

#include <TGraph.h>

#include "core/utils/log.h"
#include "objects/PixelCharge.hpp"
#include "tools/runge_kutta.h"

using namespace allpix;
using namespace ROOT::Math;

TransientPropagationModule::TransientPropagationModule(Configuration& config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)), messenger_(messenger) {
    using XYVectorInt = DisplacementVector2D<Cartesian2D<int>>;

    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector:
    messenger_->bindSingle(this, &TransientPropagationModule::deposits_message_, MsgFlags::REQUIRED);

    // Seed the random generator with the module seed
    random_generator_.seed(getRandomSeed());

    // Set default value for config variables
    config_.setDefault<double>("timestep", Units::get(0.01, "ns"));
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 10);
    config_.setDefault<double>("temperature", 293.15);
    config_.setDefault<bool>("output_pulsegraphs", false);
    config_.setDefault<bool>("output_plots", config_.get<bool>("output_pulsegraphs"));
    config_.setDefault<XYVectorInt>("induction_matrix", XYVectorInt(3, 3));

    // Copy some variables from configuration to avoid lookups:
    temperature_ = config_.get<double>("temperature");
    timestep_ = config_.get<double>("timestep");
    integration_time_ = config_.get<double>("integration_time");
    matrix_ = config_.get<XYVectorInt>("induction_matrix");

    if(matrix_.x() % 2 == 0 || matrix_.y() % 2 == 0) {
        throw InvalidValueError(config_, "induction_matrix", "Odd number of pixels in x and y required.");
    }

    output_plots_ = config_.get<bool>("output_plots");
    output_pulsegraphs_ = config_.get<bool>("output_pulsegraphs");

    // Parameterization variables from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2)
    electron_Vm_ = Units::get(1.53e9 * std::pow(temperature_, -0.87), "cm/s");
    electron_Ec_ = Units::get(1.01 * std::pow(temperature_, 1.55), "V/cm");
    electron_Beta_ = 2.57e-2 * std::pow(temperature_, 0.66);

    hole_Vm_ = Units::get(1.62e8 * std::pow(temperature_, -0.52), "cm/s");
    hole_Ec_ = Units::get(1.24 * std::pow(temperature_, 1.68), "V/cm");
    hole_Beta_ = 0.46 * std::pow(temperature_, 0.17);

    boltzmann_kT_ = Units::get(8.6173e-5, "eV/K") * temperature_;
}

void TransientPropagationModule::init() {

    auto detector = getDetector();

    // Check for electric field
    if(!detector->hasElectricField()) {
        LOG(WARNING) << "This detector does not have an electric field.";
    }

    if(!detector_->hasWeightingPotential()) {
        throw ModuleError("This module requires a weighting potential.");
    }

    if(output_plots_) {
        potential_difference_ =
            new TH1D("potential_difference",
                     "Weighting potential difference between two steps;#left|#Delta#phi_{w}#right| [a.u.];events",
                     500,
                     0,
                     1);
        induced_charge_histo_ = new TH1D("induced_charge_histo",
                                         "Induced charge per time, all pixels;Drift time [ns];charge [e]",
                                         static_cast<int>(integration_time_ / timestep_),
                                         0,
                                         static_cast<double>(Units::convert(integration_time_, "ns")));
        induced_charge_e_histo_ = new TH1D("induced_charge_e_histo",
                                           "Induced charge per time, electrons only, all pixels;Drift time [ns];charge [e]",
                                           static_cast<int>(integration_time_ / timestep_),
                                           0,
                                           static_cast<double>(Units::convert(integration_time_, "ns")));
        induced_charge_h_histo_ = new TH1D("induced_charge_h_histo",
                                           "Induced charge per time, holes only, all pixels;Drift time [ns];charge [e]",
                                           static_cast<int>(integration_time_ / timestep_),
                                           0,
                                           static_cast<double>(Units::convert(integration_time_, "ns")));
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
    }
}

void TransientPropagationModule::run(unsigned int event_num) {

    // Create map for all pixels
    std::map<Pixel::Index, Pulse> pixel_map;
    Pulse total_pulse(integration_time_, timestep_);

    // Loop over all deposits for propagation
    LOG(TRACE) << "Propagating charges in sensor";
    for(auto& deposit : deposits_message_->getData()) {

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

            // Propagate a single charge deposit
            auto prop_pair = propagate(position, deposit.getType(), charge_per_step, pixel_map);
            LOG(DEBUG) << " Propagated " << charge_per_step << " from " << Units::display(position, {"mm", "um"});

            if(output_plots_) {
                drift_time_histo_->Fill(static_cast<double>(Units::convert(prop_pair.second, "ns")), charge_per_step);
            }
        }
    }

    // Create vector of pixel pulses to return for this detector
    std::vector<PixelCharge> pixel_charges;
    for(auto& pixel_index_pulse : pixel_map) {
        total_pulse += pixel_index_pulse.second;

        // Fill a graphs with the individual pixel pulses:
        if(output_pulsegraphs_) {
            auto index = pixel_index_pulse.first;
            auto pulse = pixel_index_pulse.second.getPulse();
            std::string name =
                "pulse_px" + std::to_string(index.x()) + "-" + std::to_string(index.y()) + "_" + std::to_string(event_num);

            // Generate x-axis:
            std::vector<double> time(pulse.size());
            std::generate(time.begin(), time.end(), [ n = 0.0, step = timestep_ ]() mutable { return n += step; });

            auto pulse_graph = new TGraph(static_cast<int>(pulse.size()), &time[0], &pulse[0]);
            pulse_graph->GetXaxis()->SetTitle("t [ns]");
            pulse_graph->GetYaxis()->SetTitle("Q_{ind} [e]");
            pulse_graph->SetTitle(("Induced charge in pixel (" + std::to_string(index.x()) + "," +
                                   std::to_string(index.y()) +
                                   "), Q_{tot} = " + std::to_string(pixel_index_pulse.second.getCharge()) + " e")
                                      .c_str());
            pulse_graph->Write(name.c_str());
        }

        // Store the pulse:
        pixel_charges.emplace_back(detector_->getPixel(pixel_index_pulse.first), std::move(pixel_index_pulse.second));
    }

    LOG(INFO) << "Total charge induced on all pixels: " << Units::display(total_pulse.getCharge(), "e");

    // Create a new message with pixel pulses
    auto pixel_charge_message = std::make_shared<PixelChargeMessage>(std::move(pixel_charges), detector_);

    // Dispatch the message with pixel charges
    messenger_->dispatchMessage(this, pixel_charge_message);
}

/**
 * Propagation is simulated using a parameterization for the electron mobility. This is used to calculate the electron
 * velocity at every point with help of the electric field map of the detector. A Runge-Kutta integration is applied in
 * multiple steps, adding a random diffusion to the propagating charge every step.
 */
std::pair<ROOT::Math::XYZPoint, double> TransientPropagationModule::propagate(const ROOT::Math::XYZPoint& pos,
                                                                              const CarrierType& type,
                                                                              const unsigned int charge,
                                                                              std::map<Pixel::Index, Pulse>& pixel_map) {

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

    // Define a lambda function to compute the electron velocity
    auto carrier_velocity = [&](double, Eigen::Vector3d cur_pos) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        // Compute the drift velocity
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());

        return static_cast<int>(type) * carrier_mobility(efield.norm()) * efield;
    };

    // Create the runge kutta solver with an RKF5 tableau
    auto runge_kutta = make_runge_kutta(tableau::RK5, carrier_velocity, timestep_, position);

    // Continue propagation until the deposit is outside the sensor
    Eigen::Vector3d last_position = position;
    bool within_sensor = true;
    while(within_sensor && runge_kutta.getTime() < integration_time_) {
        // Save previous position and time
        last_position = position;

        // Execute a Runge Kutta step
        auto step = runge_kutta.step();

        // Get the current result
        position = runge_kutta.getValue();

        // Get electric field at current position and fall back to empty field if it does not exist
        auto efield = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(position));

        // Apply diffusion step
        auto diffusion = carrier_diffusion(std::sqrt(efield.Mag2()), timestep_);
        position += diffusion;
        runge_kutta.setValue(position);

        // Update step length histogram
        if(output_plots_) {
            step_length_histo_->Fill(static_cast<double>(Units::convert(step.value.norm(), "um")));
        }

        // Check for overshooting outside the sensor and correct for it:
        if(!detector_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position))) {
            LOG(TRACE) << "Carrier outside sensor: " << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"nm"});
            // within_sensor = false;

            auto check_position = position;
            check_position.z() = last_position.z();
            // Correct for position in z by interpolation to increase precision:
            if(detector_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(check_position))) {
                // FIXME this currently depends in the direction of the drift
                if(position.z() > 0 && type == CarrierType::HOLE) {
                    LOG(WARNING) << "Not stopping carrier " << type << " at "
                                 << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um"});
                } else if(position.z() < 0 && type == CarrierType::ELECTRON) {
                    LOG(WARNING) << "Not stopping carrier " << type << " at "
                                 << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um"});
                } else {
                    within_sensor = false;
                }

                // Carrier left sensor on top or bottom surface, interpolate
                auto z_cur_border = std::fabs(position.z() - model_->getSensorSize().z() / 2.0) + 1e-9;
                auto z_last_border = std::fabs(model_->getSensorSize().z() / 2.0 - last_position.z()) - 1e-9;
                auto z_total = z_cur_border + z_last_border;
                position = (z_last_border / z_total) * position + (z_cur_border / z_total) * last_position;
                LOG(TRACE) << "Moved carrier to: " << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"nm"});
            } else {
                within_sensor = false;
            }
        }

        // Find the nearest pixel
        auto xpixel = static_cast<int>(std::round(position.x() / model_->getPixelSize().x()));
        auto ypixel = static_cast<int>(std::round(position.y() / model_->getPixelSize().y()));
        LOG(TRACE) << "Moving carriers below pixel "
                   << Pixel::Index(static_cast<unsigned int>(xpixel), static_cast<unsigned int>(ypixel)) << " from "
                   << Units::display(static_cast<ROOT::Math::XYZPoint>(last_position), {"um", "mm"}) << " to "
                   << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um", "mm"}) << ", "
                   << Units::display(runge_kutta.getTime(), "ns");

        // Loop over NxN pixels:
        for(int x = xpixel - matrix_.x() / 2; x <= xpixel + matrix_.x() / 2; x++) {
            for(int y = ypixel - matrix_.y() / 2; y <= ypixel + matrix_.y() / 2; y++) {
                // Ignore if out of pixel grid
                if(x < 0 || x >= model_->getNPixels().x() || y < 0 || y >= model_->getNPixels().y()) {
                    LOG(TRACE) << "Pixel (" << x << "," << y << ") skipped, outside the grid";
                    continue;
                }

                Pixel::Index pixel_index(static_cast<unsigned int>(x), static_cast<unsigned int>(y));
                auto ramo = detector_->getWeightingPotential(static_cast<ROOT::Math::XYZPoint>(position), pixel_index);
                auto last_ramo =
                    detector_->getWeightingPotential(static_cast<ROOT::Math::XYZPoint>(last_position), pixel_index);

                // Induced charge on electrode is q_int = q * (phi(x1) - phi(x0))
                auto induced = charge * (ramo - last_ramo) * (-static_cast<std::underlying_type<CarrierType>::type>(type));
                LOG(TRACE) << "Pixel " << pixel_index << " dPhi = " << (ramo - last_ramo) << ", induced " << type
                           << " q = " << Units::display(induced, "e");

                // We require a smooth weighting potential, i.e. differences > 0.9 are unphysical and are ignored.
                // They are numerical artifacts from charge carriers crossing the sensor boundary
                if(std::fabs(ramo - last_ramo) > 0.9) {
                    LOG(INFO) << "Potential difference of " << (ramo - last_ramo) << " when moving " << type << " from "
                              << Units::display(static_cast<ROOT::Math::XYZPoint>(last_position), {"um", "mm"}) << " to "
                              << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um", "mm"}) << ", skipping";
                    continue;
                }

                // Check if this pulse exists already:
                if(pixel_map.find(pixel_index) == pixel_map.end()) {
                    pixel_map[pixel_index] = Pulse(integration_time_, timestep_);
                }

                // Store induced charge in the respective pixel pulse:
                pixel_map[pixel_index].addCharge(induced, runge_kutta.getTime());

                if(output_plots_) {
                    potential_difference_->Fill(std::fabs(ramo - last_ramo));
                    induced_charge_histo_->Fill(runge_kutta.getTime(), induced);
                    if(type == CarrierType::ELECTRON) {
                        induced_charge_e_histo_->Fill(runge_kutta.getTime(), induced);
                    } else {
                        induced_charge_h_histo_->Fill(runge_kutta.getTime(), induced);
                    }
                }
            }
        }
    }

    // Return the final position of the propagated charge
    return std::make_pair(static_cast<ROOT::Math::XYZPoint>(position), runge_kutta.getTime());
}

void TransientPropagationModule::finalize() {
    if(output_plots_) {
        potential_difference_->Write();
        step_length_histo_->Write();
        drift_time_histo_->Write();
        induced_charge_histo_->Write();
        induced_charge_e_histo_->Write();
        induced_charge_h_histo_->Write();
    }
}
