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

        auto integrate_gauss = [&](double xhit, double yhit, double x1, double x2, double y1, double y2) {
            LOG(TRACE) << "Calculating integral for square " << x1 << "-" << x2 << ", " << y1 << "-" << y2
                       << " for charge at " << xhit << ", " << yhit;
            double integral = (-std::erf((x1 - xhit) / (std::sqrt(2.) * diffusion_std_dev)) +
                               std::erf((x2 - xhit) / (std::sqrt(2.) * diffusion_std_dev))) *
                              (-std::erf((y1 - yhit) / (std::sqrt(2.) * diffusion_std_dev)) +
                               std::erf((y2 - yhit) / (std::sqrt(2.) * diffusion_std_dev))) /
                              4;
            LOG(TRACE) << "Integral is " << integral / 4.;
            return integral;
        };

        double projected_charge = 0;
        // Loop over Gaussian distribution (+-3 sigma) with configurable precision
        for(double section_x = position.x() - 5 * diffusion_std_dev; section_x < position.x() + 5 * diffusion_std_dev;
            section_x += spatial_precision_) {
            for(double section_y = position.y() - 5 * diffusion_std_dev; section_y < position.y() + 5 * diffusion_std_dev;
                section_y += spatial_precision_) {
                LOG(TRACE) << "Looking at position " << Units::display(section_x, "um") << " "
                           << Units::display(section_y, "um");

                // calculate part integral of gaussian smeared charge cloud:
                double charge = deposit.getCharge() * integrate_gauss(position.x(),
                                                                      position.y(),
                                                                      section_x - spatial_precision_ / 2.,
                                                                      section_x + spatial_precision_ / 2.,
                                                                      section_y - spatial_precision_ / 2.,
                                                                      section_y + spatial_precision_ / 2.);
                projected_charge += charge;

                // Only add if within sensor volume:
                auto local_position = ROOT::Math::XYZPoint(section_x, section_y, model->getSensorSize().z() / 2.);
                if(!detector_->isWithinSensor(local_position)) {
                    continue;
                }

                auto global_position = detector_->getGlobalPosition(local_position);
                // Produce charge carrier at this position
                propagated_charges.emplace_back(
                    position, global_position, deposit.getType(), charge, deposit.getEventTime(), &deposit);
            }
        }
        LOG(DEBUG) << "Charge after projection: " << projected_charge << " (vs " << deposit.getCharge() << " before)";
        charge_lost += std::fabs(deposit.getCharge() - projected_charge);
        total_charge += projected_charge;
    }

    LOG(DEBUG) << "Total charge: " << total_charge << " (lost: " << charge_lost << ", "
               << (charge_lost / total_charge * 100.) << "%)";
    LOG(INFO) << "Total count of propagated charge carriers: " << propagated_charges.size();

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, propagated_charge_message);
}
