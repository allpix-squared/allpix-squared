/**
 * @file
 * @brief Implementation of CoulombProjectionPropagation module
 *
 * @copyright Copyright (c) 2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "CoulombProjectionPropagationModule.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "core/messenger/Messenger.hpp"
#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

using namespace allpix;

CoulombProjectionPropagationModule::CoulombProjectionPropagationModule(Configuration& config,
                                                                       Messenger* messenger,
                                                                       std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)),
      top_z_(detector_->getModel()->getSensorSize().z() / 2) {

    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector
    messenger_->bindSingle<DepositedChargeMessage>(this, MsgFlags::REQUIRED);

    // Set default value for config variables
    config_.setDefault<unsigned int>("charge_per_step", 10);
    config_.setDefault<unsigned int>("max_charge_groups", 1000);
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<bool>("propagate_holes", false);
    config_.setDefault<bool>("ignore_magnetic_field", false);

    integration_time_ = config_.get<double>("integration_time");
    charge_per_step_ = config_.get<unsigned int>("charge_per_step");
    max_charge_groups_ = config_.get<unsigned int>("max_charge_groups");
    output_plots_ = config_.get<bool>("output_plots");

    // Parameters of alpha(t) and beta(t): a0, a1, a2, a3, b0, b1, b2, b3, b4
    auto parameters = config_.getArray<double>("parameters");
    if(parameters.size() != alpha_parameters_.size() + beta_parameters_.size()) {
        throw InvalidValueError(config_,
                                "parameters",
                                "expected nine parameters a0, a1, a2, a3, b0, b1, b2, b3, b4 of alpha(t) and beta(t), got " +
                                    std::to_string(parameters.size()));
    }
    std::copy_n(parameters.begin(), alpha_parameters_.size(), alpha_parameters_.begin());
    std::copy_n(parameters.end() - static_cast<std::ptrdiff_t>(beta_parameters_.size()),
                beta_parameters_.size(),
                beta_parameters_.begin());

    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Set default for charge carrier propagation:
    if(config_.get<bool>("propagate_holes")) {
        propagate_type_ = CarrierType::HOLE;
        LOG(INFO) << "Holes are chosen for propagation. Electrons are therefore not propagated.";
    } else {
        propagate_type_ = CarrierType::ELECTRON;
    }

    auto temperature = config_.get<double>("temperature");

    // Mobility fixed to Jacoboni:
    mobility_ = std::make_unique<JacoboniCanali>(model_->getSensorMaterial(), temperature);

    // We need direct access to the critical field values of the model since we have a discrete integration of the formula
    // for the total drift time. Taken from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2)
    electron_Ec_ = Units::get(1.01 * std::pow(temperature, 1.55), "V/cm");
    hole_Ec_ = Units::get(1.24 * std::pow(temperature, 1.68), "V/cm");
}

double CoulombProjectionPropagationModule::ggd_alpha(double drift_time) const {
    // Parameterization in um with the drift time in ns
    const auto t = static_cast<double>(Units::convert(drift_time, "ns"));
    const auto& a = alpha_parameters_;
    return Units::get(a[0] + (a[1] * std::sqrt(t)) + (a[2] * t) + (a[3] * t * t), "um");
}

double CoulombProjectionPropagationModule::ggd_beta(double drift_time) const {
    // Parameterization with the drift time in ns, beta = 2 corresponds to a Gaussian distribution
    const auto t = static_cast<double>(Units::convert(drift_time, "ns"));
    const auto& b = beta_parameters_;
    return 2. + (b[0] * std::pow(t - b[1], b[2])) + (b[3] * std::exp(b[4] * t));
}

void CoulombProjectionPropagationModule::initialize() {
    if(detector_->getElectricFieldType() != FieldType::LINEAR) {
        throw ModuleError("This module should only be used with linear electric fields.");
    }

    if(detector_->hasDopingProfile() && detector_->getDopingProfileType() != FieldType::CONSTANT) {
        throw ModuleError("This module should only be used with constant doping concentration.");
    }

    if(detector_->hasMagneticField()) {
        if(!config_.get<bool>("ignore_magnetic_field")) {
            throw ModuleError("This module should not be used with magnetic fields. Add the option 'ignore_magnetic_field' "
                              "to the configuration if you would like to continue.");
        }
        LOG(WARNING) << "A magnetic field is switched on, but is set to be ignored for this module.";
    }

    // Find correct top side
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
        propagation_time_histo_ = CreateHistogram<TH1D>("propagation_time_histo",
                                                        "Propagation time;Propagation time [ns];charge carriers",
                                                        static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                                        0,
                                                        static_cast<double>(Units::convert(integration_time_, "ns")) * 2);
        initial_position_histo_ =
            CreateHistogram<TH1D>("initial_position_histo",
                                  "Initial position of collected charge carriers;Position z [um];charge carriers",
                                  100,
                                  static_cast<double>(Units::convert(-std::abs(top_z_), "um")),
                                  static_cast<double>(Units::convert(std::abs(top_z_), "um")));

        group_size_histo_ = CreateHistogram<TH1D>("group_size_histo",
                                                  "Charge carrier group size;group size;number of groups transported",
                                                  static_cast<int>(100 * charge_per_step_),
                                                  0,
                                                  static_cast<int>(100 * charge_per_step_));
    }
}

void CoulombProjectionPropagationModule::run(Event* event) {
    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;

    unsigned int total_charge = 0;
    unsigned int total_projected_charge = 0;

    // Loop over all deposits for propagation
    for(const auto& deposit : deposits_message->getData()) {

        auto type = deposit.getType();
        auto initial_position = deposit.getLocalPosition();
        // Selection of charge carrier:
        if(type != propagate_type_) {
            continue;
        }

        total_deposits_++;

        LOG(DEBUG) << "Set of " << deposit.getCharge() << " charge carriers (" << type << ") on "
                   << Units::display(initial_position, {"mm", "um"});

        // Get the electric field at the deposit position and the top of the sensor:
        auto efield_mag = std::sqrt(detector_->getElectricField(initial_position).Mag2());
        auto efield_mag_top = std::sqrt(detector_->getElectricField(ROOT::Math::XYZPoint(0., 0., top_z_)).Mag2());
        auto doping = detector_->getDopingConcentration(initial_position);

        // Only project if within the depleted region (i.e. efield not zero)
        if(efield_mag < std::numeric_limits<double>::epsilon()) {
            throw ModuleError(
                "Electric field is zero at the position of the charge carrier, cannot propagate charge carriers");
        }

        // Approximated drift time in a linear electric field with the Jacoboni-Canali mobility for beta = 1
        const auto distance = std::abs(top_z_ - initial_position.z());
        const auto critical_field = (type == CarrierType::ELECTRON ? electron_Ec_ : hole_Ec_);
        auto field_term = distance / efield_mag;
        if(std::abs(efield_mag_top - efield_mag) > 1e-9 * efield_mag) {
            const auto slope_efield = (efield_mag_top - efield_mag) / distance;
            field_term = std::log(efield_mag_top / efield_mag) / slope_efield;
        }
        const auto drift_time = (field_term + (distance / critical_field)) / (*mobility_)(type, 0, doping);

        const auto alpha = ggd_alpha(drift_time);
        const auto beta = ggd_beta(drift_time);
        LOG(TRACE) << "Drift time " << Units::display(drift_time, {"ns", "ps"}) << ", GGD alpha "
                   << Units::display(alpha, {"um", "nm"}) << ", beta " << beta;
        if(!(alpha > 0) || !(beta > 0)) {
            throw ModuleError("Invalid GGD parameters alpha = " + std::to_string(alpha) +
                              " mm, beta = " + std::to_string(beta) + " at a drift time of " +
                              std::to_string(Units::convert(drift_time, "ns")) + " ns, check the parameterization");
        }
        allpix::ggd_distribution<double> ggd_distribution(alpha, beta);

        const auto global_time = deposit.getGlobalTime() + drift_time;
        const auto local_time = deposit.getLocalTime() + drift_time;

        // Only add if within requested integration time:
        if(local_time > integration_time_) {
            LOG(DEBUG) << "Charge carriers propagation time not within integration time: "
                       << Units::display(global_time, "ns") << " global / " << Units::display(local_time, {"ns", "ps"})
                       << " local";
            total_charge += deposit.getCharge();
            continue;
        }

        if(output_plots_) {
            propagation_time_histo_->Fill(static_cast<double>(Units::convert(drift_time, "ns")), deposit.getCharge());
        }

        unsigned int charges_remaining = deposit.getCharge();
        total_charge += charges_remaining;

        auto charge_per_step = charge_per_step_;
        if(max_charge_groups_ > 0 && deposit.getCharge() / charge_per_step > max_charge_groups_) {
            charge_per_step =
                static_cast<unsigned int>(std::ceil(static_cast<double>(deposit.getCharge()) / max_charge_groups_));
            deposits_exceeding_max_groups_++;
            LOG(INFO) << "Deposited charge: " << deposit.getCharge()
                      << ", which exceeds the maximum number of charge groups allowed. Increasing charge_per_step to "
                      << charge_per_step << " for this deposit.";
        }
        while(charges_remaining > 0) {
            charge_per_step = std::min(charge_per_step, charges_remaining);
            charges_remaining -= charge_per_step;

            // Find projected position
            auto local_position = ROOT::Math::XYZPoint(initial_position.x() + ggd_distribution(event->getRandomEngine()),
                                                       initial_position.y() + ggd_distribution(event->getRandomEngine()),
                                                       top_z_);

            // Only add if within sensor volume:
            if(!model_->isWithinSensor(local_position)) {
                LOG(DEBUG) << "Charge carriers outside sensor volume at " << Units::display(local_position, {"mm", "um"});
                continue;
            }

            if(output_plots_) {
                initial_position_histo_->Fill(static_cast<double>(Units::convert(initial_position.z(), "um")),
                                              charge_per_step);
                group_size_histo_->Fill(charge_per_step);
            }

            // Produce charge carrier at this position
            propagated_charges.emplace_back(local_position,
                                            detector_->getGlobalPosition(local_position),
                                            deposit.getType(),
                                            charge_per_step,
                                            local_time,
                                            global_time,
                                            CarrierState::HALTED,
                                            &deposit);

            LOG(DEBUG) << "Propagated " << charge_per_step << " " << type << " to "
                       << Units::display(local_position, {"mm", "um"}) << " in " << Units::display(global_time, "ns")
                       << " global / " << Units::display(local_time, {"ns", "ps"}) << " local";

            total_projected_charge += charge_per_step;
        }
    }
    const auto charge_lost = total_charge - total_projected_charge;

    LOG(INFO) << "Total charge: " << total_charge << " (lost: " << charge_lost << ", "
              << (total_charge > 0 ? (100. * charge_lost / total_charge) : 0.) << "%)";

    LOG(DEBUG) << "Total count of propagated charge carriers: " << propagated_charges.size();

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, std::move(propagated_charge_message), event);
}

void CoulombProjectionPropagationModule::finalize() {
    if(output_plots_) {
        group_size_histo_->Get()->GetXaxis()->SetRange(1, group_size_histo_->Get()->GetNbinsX() + 1);
    }
    LOG(INFO) << deposits_exceeding_max_groups_ * 100.0 / std::max(1U, static_cast<unsigned int>(total_deposits_))
              << "% of deposits have charge exceeding the " << max_charge_groups_
              << " charge groups allowed, with a charge_per_step value of " << charge_per_step_ << ".";
}
