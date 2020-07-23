/**
 * @file
 * @brief Implementation of ProjectionPropagation module
 * @copyright MIT License
 */

#include "ProjectionPropagationModule.hpp"

#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <utility>

#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

using namespace allpix;

ProjectionPropagationModule::ProjectionPropagationModule(Configuration& config,
                                                         Messenger* messenger,
                                                         std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {
    // Save detector model
    model_ = detector_->getModel();

    random_generator_.seed(getRandomSeed());

    // Require deposits message for single detector
    messenger_->bindSingle(this, &ProjectionPropagationModule::deposits_message_, MsgFlags::REQUIRED);

    // Set default value for config variables
    config_.setDefault<int>("charge_per_step", 10);
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<bool>("diffuse_deposit", false);

    integration_time_ = config_.get<double>("integration_time");
    output_plots_ = config_.get<bool>("output_plots");
    diffuse_deposit_ = config_.get<bool>("diffuse_deposit");

    // Set default for charge carrier propagation:
    config_.setDefault<bool>("propagate_holes", false);
    if(config_.get<bool>("propagate_holes")) {
        propagate_type_ = CarrierType::HOLE;
        LOG(INFO) << "Holes are chosen for propagation. Electrons are therefore not propagated.";
    } else {
        propagate_type_ = CarrierType::ELECTRON;
    }

    // Parameterization variables from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2)
    auto temperature = config_.get<double>("temperature");
    electron_Vm_ = Units::get(1.53e9 * std::pow(temperature, -0.87), "cm/s");
    electron_Ec_ = Units::get(1.01 * std::pow(temperature, 1.55), "V/cm");
    electron_Beta_ = 2.57e-2 * std::pow(temperature, 0.66);

    hole_Vm_ = Units::get(1.62e8 * std::pow(temperature, -0.52), "cm/s");
    hole_Ec_ = Units::get(1.24 * std::pow(temperature, 1.68), "V/cm");
    hole_Beta_ = 0.46 * std::pow(temperature, 0.17);

    boltzmann_kT_ = Units::get(8.6173e-5, "eV/K") * temperature;

    config_.setDefault<bool>("ignore_magnetic_field", false);
}

void ProjectionPropagationModule::init() {
    if(detector_->getElectricFieldType() != FieldType::LINEAR) {
        throw ModuleError("This module should only be used with linear electric fields.");
    }

    if(detector_->hasMagneticField() && !config_.get<bool>("ignore_magnetic_field")) {
        throw ModuleError("This module should not be used with magnetic fields. Add the option 'ignore_magnetic_field' to "
                          "the configuration if you would like to continue.");
    } else if(detector_->hasMagneticField() && config_.get<bool>("ignore_magnetic_field")) {
        LOG(WARNING) << "A magnetic field is switched on, but is set to be ignored for this module.";
    }

    // Find correct top side
    top_z_ = model_->getSensorSize().z() / 2;
    if(detector_->getElectricField({0, 0, top_z_}).z() > detector_->getElectricField({0, 0, -top_z_}).z()) {
        top_z_ *= -1;
    }
    if(propagate_type_ == CarrierType::HOLE) {
        top_z_ *= -1;
    }

    if(top_z_ < 0) {
        LOG(WARNING)
            << "Selected carriers are not propagated to the implant side, combination of propagated carrier and electric "
               "field is wrong!";
    }

    if(output_plots_) {
        // Initialize output plots
        propagation_time_histo_ = new TH1D("propagation_time_histo",
                                           "Propagation time (drift + diffusion);Propagation time [ns];charge carriers",
                                           static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                           0,
                                           static_cast<double>(Units::convert(integration_time_, "ns")) * 2);
        drift_time_histo_ = new TH1D("drift_time_histo",
                                     "Drift time (directed drift only);Drift time [ns];charge carriers",
                                     static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                     0,
                                     static_cast<double>(Units::convert(integration_time_, "ns")) * 2);
        initial_position_histo_ = new TH1D("initial_position_histo",
                                           "Initial position of collected charge carriers;Position z [um];charge carriers",
                                           100,
                                           static_cast<double>(Units::convert(-top_z_, "um")),
                                           static_cast<double>(Units::convert(top_z_, "um")));
        if(diffuse_deposit_) {
            diffusion_time_histo_ = new TH1D("diffusion_time_histo",
                                             "Diffusion time prior to drift;Diffusion time [ns];charge carriers",
                                             static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                             0,
                                             static_cast<double>(Units::convert(integration_time_, "ns")));
        }
    }
}

void ProjectionPropagationModule::run(unsigned int) {

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;

    double charge_lost = 0;
    double total_charge = 0;
    double total_projected_charge = 0;

    // Loop over all deposits for propagation
    for(auto& deposit : deposits_message_->getData()) {

        auto type = deposit.getType();
        auto initial_position = deposit.getLocalPosition();

        // Selection of charge carrier:
        if(type != propagate_type_) {
            continue;
        }

        LOG(DEBUG) << "Set of " << deposit.getCharge() << " charge carriers (" << type << ") on "
                   << Units::display(initial_position, {"mm", "um"});

        double projected_charge = 0;

        unsigned int charges_remaining = deposit.getCharge();
        total_charge += charges_remaining;

        auto charge_per_step = config_.get<unsigned int>("charge_per_step");
        while(charges_remaining > 0) {
            if(charge_per_step > charges_remaining) {
                charge_per_step = charges_remaining;
            }
            charges_remaining -= charge_per_step;

            auto position = initial_position;

            // Get the electric field at the position of the deposited charge and the top of the sensor:
            auto efield = detector_->getElectricField(position);
            double efield_mag = std::sqrt(efield.Mag2());
            auto efield_top = detector_->getElectricField(ROOT::Math::XYZPoint(0., 0., top_z_));
            double efield_mag_top = std::sqrt(efield_top.Mag2());

            // Define a lambda function to compute the carrier mobility
            auto carrier_mobility = [&](double efield_magn) {
                // Compute carrier mobility from constants and electric field magnitude
                double numerator, denominator;
                if(type == CarrierType::ELECTRON) {
                    numerator = electron_Vm_ / electron_Ec_;
                    denominator = std::pow(1. + std::pow(efield_magn / electron_Ec_, electron_Beta_), 1.0 / electron_Beta_);
                } else {
                    numerator = hole_Vm_ / hole_Ec_;
                    denominator = std::pow(1. + std::pow(efield_magn / hole_Ec_, hole_Beta_), 1.0 / hole_Beta_);
                }
                return numerator / denominator;
            };

            double diffusion_time = 0;

            // Only project if within the depleted region (i.e. efield not zero)
            if(efield_mag < std::numeric_limits<double>::epsilon()) {
                LOG(TRACE) << "Electric field is zero at " << Units::display(position, {"mm", "um"});
                if(!diffuse_deposit_) {
                    continue;
                }
                double diffusion_constant = boltzmann_kT_ * carrier_mobility(efield_mag);
                double diffusion_std_dev = std::sqrt(2. * diffusion_constant * integration_time_);
                LOG(TRACE) << "Diffusion width of this charge carrier is " << Units::display(diffusion_std_dev, "um");

                std::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
                double diffusion_x = gauss_distribution(random_generator_);
                double diffusion_y = gauss_distribution(random_generator_);
                double diffusion_z = gauss_distribution(random_generator_);
                auto diffusion_vec = ROOT::Math::XYZVector(diffusion_x, diffusion_y, diffusion_z);

                auto local_position_diffusion = position + diffusion_vec;

                auto efield_diffusion = detector_->getElectricField(local_position_diffusion);
                double efield_mag_diffusion = std::sqrt(efield_diffusion.Mag2());

                if(efield_mag_diffusion < std::numeric_limits<double>::epsilon() && (detector_->isWithinSensor(position))) {
                    LOG(TRACE) << "Charge carrier remains within undepleted volume";
                    continue;
                }

                std::function<ROOT::Math::XYZPoint(const ROOT::Math::XYZPoint&, const ROOT::Math::XYZPoint&)> interval;
                interval = [this, &interval](const ROOT::Math::XYZPoint& start,
                                             const ROOT::Math::XYZPoint& stop) -> ROOT::Math::XYZPoint {
                    // Break nested intervals at a precision of 0.01 um
                    if(std::sqrt((stop - ROOT::Math::XYZVector(start)).Mag2()) < 0.00001) {
                        return stop;
                    }
                    auto efield_center = detector_->getElectricField((stop + ROOT::Math::XYZVector(start)) / 2.);
                    double efield_center_mag = std::sqrt(efield_center.Mag2());
                    if(efield_center_mag > std::numeric_limits<double>::epsilon()) {
                        return interval(start, (stop + ROOT::Math::XYZVector(start)) / 2.);
                    } else {
                        return interval((stop + ROOT::Math::XYZVector(start)) / 2., stop);
                    }
                };

                position = interval(position, local_position_diffusion);
                efield = detector_->getElectricField(position);
                efield_mag = std::sqrt(efield.Mag2());
                diffusion_time = integration_time_ * std::sqrt((position - initial_position).Mag2() /
                                                               (local_position_diffusion - initial_position).Mag2());

                if(!detector_->isWithinSensor(position)) {
                    LOG(TRACE) << "Charge carrier diffused outside the sensor volume";
                    continue;
                }
                LOG(TRACE) << "Charge diffused to position: " << Units::display(position, {"mm", "um"});
                LOG(TRACE) << " ... with an electric field of " << Units::display(efield_mag, "V/cm");
                LOG(TRACE) << " ... and a diffusion time prior to the drift of " << Units::display(diffusion_time, "ns");
            }

            LOG(TRACE) << "Electric field at carrier position / top of the sensor: "
                       << Units::display(efield_mag_top, "V/cm") << " , " << Units::display(efield_mag, "V/cm");

            slope_efield_ = (efield_mag_top - efield_mag) / (std::abs(top_z_ - position.z()));

            // Calculate the drift time
            auto calc_drift_time = [&]() {
                double Ec = (type == CarrierType::ELECTRON ? electron_Ec_ : hole_Ec_);
                double zero_mobility = (type == CarrierType::ELECTRON ? electron_Vm_ / electron_Ec_ : hole_Vm_ / hole_Ec_);

                return ((log(efield_mag_top) - log(efield_mag)) / slope_efield_ + std::abs(top_z_ - position.z()) / Ec) /
                       zero_mobility;
            };
            LOG(TRACE) << "Electric field is " << Units::display(efield_mag, "V/cm");

            // Assume linear electric field over the depleted part of the sensor
            double diffusion_constant =
                boltzmann_kT_ * (carrier_mobility(efield_mag) + carrier_mobility(efield_mag_top)) / 2.;

            double drift_time = calc_drift_time();
            double propagation_time = drift_time + diffusion_time;
            LOG(TRACE) << "Drift time is " << Units::display(drift_time, "ns");

            if(output_plots_) {
                propagation_time_histo_->Fill(propagation_time, charge_per_step);
                drift_time_histo_->Fill(drift_time, charge_per_step);
                if(diffuse_deposit_) {
                    diffusion_time_histo_->Fill(diffusion_time, charge_per_step);
                }
            }

            double diffusion_std_dev = std::sqrt(2. * diffusion_constant * drift_time);
            LOG(TRACE) << "Diffusion width is " << Units::display(diffusion_std_dev, "um");

            std::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
            double diffusion_x = gauss_distribution(random_generator_);
            double diffusion_y = gauss_distribution(random_generator_);

            // Find projected position
            auto local_position = ROOT::Math::XYZPoint(position.x() + diffusion_x, position.y() + diffusion_y, top_z_);

            // Only add if within requested integration time:
            auto event_time = deposit.getEventTime() + propagation_time;
            if(propagation_time > integration_time_) {
                LOG(DEBUG) << "Charge carriers propagation time not within integration time: "
                           << Units::display(event_time, "ns");
                continue;
            }

            // Only add if within sensor volume:
            if(!detector_->isWithinSensor(local_position)) {
                LOG(DEBUG) << "Charge carriers outside sensor volume at " << Units::display(local_position, {"mm", "um"});
                // FIXME: drop charges if it ends up outside the sensor, could be optimized to estimate position on border
                continue;
            }

            if(output_plots_) {
                initial_position_histo_->Fill(static_cast<double>(Units::convert(initial_position.z(), "um")),
                                              charge_per_step);
            }

            auto global_position = detector_->getGlobalPosition(local_position);

            // Produce charge carrier at this position
            propagated_charges.emplace_back(
                local_position, global_position, deposit.getType(), charge_per_step, event_time, &deposit);

            LOG(DEBUG) << "Propagated " << charge_per_step << " " << type << " to "
                       << Units::display(local_position, {"mm", "um"}) << " in " << Units::display(event_time, "ns")
                       << " time";

            projected_charge += charge_per_step;
        }
        total_projected_charge += projected_charge;
    }
    charge_lost = total_charge - total_projected_charge;

    LOG(INFO) << "Total charge: " << total_charge << " (lost: " << charge_lost << ", " << (charge_lost / total_charge * 100.)
              << "%)";
    LOG(DEBUG) << "Total count of propagated charge carriers: " << propagated_charges.size();

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, propagated_charge_message);
}

void ProjectionPropagationModule::finalize() {
    if(output_plots_) {
        // Write output plots
        drift_time_histo_->Write();
        propagation_time_histo_->Write();
        initial_position_histo_->Write();
        if(diffuse_deposit_) {
            diffusion_time_histo_->Write();
        }
    }
}
