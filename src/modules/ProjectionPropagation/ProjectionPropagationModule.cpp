/**
 * @file
 * @brief Implementation of [ProjectionPropagation] module
 * @copyright MIT License
 */

#include "ProjectionPropagationModule.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <utility>

#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

using namespace allpix;

ProjectionPropagationModule::ProjectionPropagationModule(Configuration config,
                                                         Messenger* messenger,
                                                         std::shared_ptr<Detector> detector)
    : Module(config, detector), config_(std::move(config)), messenger_(messenger), detector_(std::move(detector)) {
    // Save detector model
    model_ = detector_->getModel();

    random_generator_.seed(getRandomSeed());

    // Require deposits message for single detector
    messenger_->bindSingle(this, &ProjectionPropagationModule::deposits_message_, MsgFlags::REQUIRED);

    // Set default value for config variables
    config_.setDefault<double>("spatial_precision", Units::get(1.0, "um"));

    // Parameterization variables from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2)
    auto temperature = config_.get<double>("temperature");
    electron_Vm_ = Units::get(1.53e9 * std::pow(temperature, -0.87), "cm/s");
    electron_Ec_ = Units::get(1.01 * std::pow(temperature, 1.55), "V/cm");
    electron_Beta_ = 2.57e-2 * std::pow(temperature, 0.66);

    hole_Vm_ = Units::get(1.62e8 * std::pow(temperature, -0.52), "cm/s");
    hole_Ec_ = Units::get(1.24 * std::pow(temperature, 1.68), "V/cm");
    hole_Beta_ = 0.46 * std::pow(temperature, 0.17);

    spatial_precision_ = config_.get<double>("spatial_precision");
    boltzmann_kT_ = Units::get(8.6173e-5, "eV/K") * temperature;
}

void ProjectionPropagationModule::run(unsigned int) {

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;
    auto model = detector_->getModel();

    double charge_lost = 0;
    double total_charge = 0;
    double total_projected_charge = 0;

    // Loop over all deposits for propagation
    for(auto& deposit : deposits_message_->getData()) {

        auto position = deposit.getLocalPosition();
        auto type = deposit.getType();

        // FIXME allow selection of charge carrier:
        if(type != CarrierType::ELECTRON) {
            continue;
        }

        LOG(DEBUG) << "Set of " << deposit.getCharge() << " charge carriers (" << (type == CarrierType::ELECTRON ? "e" : "h")
                   << ") on " << display_vector(position, {"mm", "um"});

        // Define a lambda function to compute the carrier mobility
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

        // Get the electric field at the position of the deposited charge:
        auto efield = detector_->getElectricField(position);
        double efield_mag = std::sqrt(efield.Mag2());

        // Only project if within the depleted region (i.e. efield not zero)
        if(efield_mag < std::numeric_limits<double>::epsilon()) {
            LOG(TRACE) << "Electric field is zero at " << display_vector(position, {"mm", "um"});
            continue;
        }

        LOG(TRACE) << "Electric field is " << Units::display(efield_mag, "V/cm");

        // Assume constant electric field over the sensor:
        double diffusion_constant = boltzmann_kT_ * carrier_mobility(efield_mag);

        double drift_distance = model->getSensorSize().z() / 2. - position.z();
        LOG(TRACE) << "Drift distance is " << Units::display(drift_distance, "um");

        double drift_velocity = carrier_mobility(efield_mag) * efield_mag;
        LOG(TRACE) << "Drift velocity is " << Units::display(drift_velocity, "um/ns");

        double drift_time = drift_distance / drift_velocity;
        LOG(TRACE) << "Drift time is " << Units::display(drift_time, "ns");

        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * drift_time);
        LOG(TRACE) << "Diffusion width is " << Units::display(diffusion_std_dev, "um");

        double projected_charge = 0;

        unsigned int charges_remaining = deposit.getCharge();
        total_charge += charges_remaining;

        auto charge_per_step = config_.get<unsigned int>("charge_per_step");
        while(charges_remaining > 0) {
            if(charge_per_step > charges_remaining) {
                charge_per_step = charges_remaining;
            }
            charges_remaining -= charge_per_step;

            std::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
            double diffusion_x = gauss_distribution(random_generator_);
            double diffusion_y = gauss_distribution(random_generator_);

            auto projected_position = ROOT::Math::XYZPoint(
                position.x() + diffusion_x, position.y() + diffusion_y, model->getSensorSize().z() / 2.);

            // Only add if within sensor volume:
            auto local_position =
                ROOT::Math::XYZPoint(projected_position.x(), projected_position.y(), model->getSensorSize().z() / 2.);
            if(!detector_->isWithinSensor(local_position)) {
                continue;
            }

            auto global_position = detector_->getGlobalPosition(local_position);

            // Produce charge carrier at this position
            propagated_charges.emplace_back(
                projected_position, global_position, deposit.getType(), charge_per_step, deposit.getEventTime(), &deposit);

            LOG(DEBUG) << "Propagated " << charge_per_step << " charge carriers ("
                       << (type == CarrierType::ELECTRON ? "e" : "h") << ") to "
                       << display_vector(projected_position, {"mm", "um"});

            projected_charge += charge_per_step;
        }
        total_projected_charge += projected_charge;
    }
    charge_lost = total_charge - total_projected_charge;

    LOG(INFO) << "Total charge: " << total_charge << " (lost: " << charge_lost << ", " << (charge_lost / total_charge * 100.)
              << "%)";
    LOG(INFO) << "Total count of propagated charge carriers: " << propagated_charges.size();

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, propagated_charge_message);
}
