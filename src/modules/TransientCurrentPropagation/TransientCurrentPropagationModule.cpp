/**
 * @file
 * @brief Implementation of [TransientCurrentPropagation] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "TransientCurrentPropagationModule.hpp"

#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>

#include <Eigen/Core>

#include "core/utils/log.h"
#include "tools/runge_kutta.h"

using namespace allpix;

TransientCurrentPropagationModule::TransientCurrentPropagationModule(Configuration config,
                                                                     Messenger* messenger,
                                                                     std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector), detector_(detector), messenger_(messenger) {

    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector:
    messenger_->bindSingle(this, &TransientCurrentPropagationModule::deposits_message_, MsgFlags::REQUIRED);

    // Seed the random generator with the module seed
    random_generator_.seed(getRandomSeed());

    // Set default value for config variables
    config_.setDefault<double>("spatial_precision", Units::get(0.25, "nm"));
    config_.setDefault<double>("timestep", Units::get(0.01, "ns"));
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 10);
    config_.setDefault<double>("temperature", 293.15);

    // Parameterization variables from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2)
    electron_Vm_ = Units::get(1.53e9 * std::pow(temperature_, -0.87), "cm/s");
    electron_Ec_ = Units::get(1.01 * std::pow(temperature_, 1.55), "V/cm");
    electron_Beta_ = 2.57e-2 * std::pow(temperature_, 0.66);

    hole_Vm_ = Units::get(1.62e8 * std::pow(temperature_, -0.52), "cm/s");
    hole_Ec_ = Units::get(1.24 * std::pow(temperature_, 1.68), "V/cm");
    hole_Beta_ = 0.46 * std::pow(temperature_, 0.17);

    boltzmann_kT_ = Units::get(8.6173e-5, "eV/K") * temperature_;
}

void TransientCurrentPropagationModule::init() {

    // FIXME check that we have a Ramo Weighting field for the detector!
}

void TransientCurrentPropagationModule::run(unsigned int) {

    // Create map for all pixels
    std::map<Pixel::Index, PixelPulse> pixel_map;

    // Loop over all deposits for propagation
    LOG(TRACE) << "Propagating charges in sensor";
    for(auto& deposit : deposits_message_->getData()) {

        // Loop over all charges in the deposit
        unsigned int charges_remaining = deposit.getCharge();

        LOG(DEBUG) << "Set of charge carriers (" << deposit.getType() << ") on "
                   << display_vector(deposit.getLocalPosition(), {"mm", "um"});

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
            propagate(position, deposit.getType(), charge_per_step, pixel_map);
        }
    }

    // Create vector of pixel pulses to return for this detector
    std::vector<PixelPulse> pixel_pulses;
    for(auto& pixel_index_pulse : pixel_map) {
        pixel_pulses.push_back(std::move(pixel_index_pulse.second));
    }

    // Create a new message with pixel pulses
    auto pixel_pulse_message = std::make_shared<PixelPulseMessage>(std::move(pixel_pulses), detector_);

    // Dispatch the message with pixel pulses
    messenger_->dispatchMessage(this, pixel_pulse_message);
}

/**
 * Propagation is simulated using a parameterization for the electron mobility. This is used to calculate the electron
 * velocity at every point with help of the electric field map of the detector. A Runge-Kutta integration is applied in
 * multiple steps, adding a random diffusion to the propagating charge every step.
 */
void TransientCurrentPropagationModule::propagate(const ROOT::Math::XYZPoint& pos,
                                                  const CarrierType& type,
                                                  const unsigned int,
                                                  std::map<Pixel::Index, PixelPulse>&) {

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
    double last_time = 0;
    while(detector_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position)) &&
          runge_kutta.getTime() < integration_time_) {

        // Save previous position and time
        last_position = position;
        last_time = runge_kutta.getTime();

        // Execute a Runge Kutta step
        auto step = runge_kutta.step();

        // Get the current result
        position = runge_kutta.getValue();

        // Get electric field at current position and fall back to empty field if it does not exist
        auto efield = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(position));

        // Apply diffusion step
        auto diffusion = carrier_diffusion(std::sqrt(efield.Mag2()), timestep);
        position += diffusion;

        // TODO implement calculation of Ramo induced charge
        // TODO loop over NxN pixels and add it to their respective pulses
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
}
