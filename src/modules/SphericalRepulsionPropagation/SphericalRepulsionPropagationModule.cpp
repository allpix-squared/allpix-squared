/**
 * @file
 * @brief Implementation of SphericalRepulsionPropagation module
 *
 * @copyright Copyright (c) 2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "SphericalRepulsionPropagationModule.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>
#include <utility>

#include "core/messenger/Messenger.hpp"
#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "physics/MaterialProperties.hpp"

using namespace allpix;

SphericalRepulsionPropagationModule::SphericalRepulsionPropagationModule(Configuration& config,
                                                                         Messenger* messenger,
                                                                         std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {
    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector
    messenger_->bindSingle<DepositedChargeMessage>(this, MsgFlags::REQUIRED);

    // Set default value for config variables
    config_.setDefault<double>("timestep", Units::get(0.01, "ns"));
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 5);
    config_.setDefault<unsigned int>("max_charge_groups", 0);
    config_.setDefault<double>("temperature", 293.15);
    config_.setDefault<std::string>("mobility_model", "jacoboni");
    config_.setDefault<bool>("propagate_electrons", true);
    config_.setDefault<bool>("propagate_holes", false);
    config_.setDefault<bool>("ignore_magnetic_field", false);
    config_.setDefault<bool>("output_plots", false);

    // Repulsion model
    if(ionization_energies.contains(model_->getSensorMaterial())) {
        config_.setDefault<double>("charge_creation_energy", ionization_energies[model_->getSensorMaterial()]);
    }
    if(model_->getSensorMaterial() == SensorMaterial::SILICON) {
        config_.setDefault<double>("relative_permittivity", 11.7);
    }

    if(!config_.get<bool>("propagate_electrons") && !config_.get<bool>("propagate_holes")) {
        throw InvalidValueError(
            config_,
            "propagate_electrons",
            "No charge carriers selected for propagation, enable 'propagate_electrons' or 'propagate_holes'.");
    }

    // Copy some variables from configuration to avoid lookups:
    timestep_ = config_.get<double>("timestep");
    integration_time_ = config_.get<double>("integration_time");
    temperature_ = config_.get<double>("temperature");
    charge_per_step_ = config_.get<unsigned int>("charge_per_step");
    max_charge_groups_ = config_.get<unsigned int>("max_charge_groups");
    propagate_electrons_ = config_.get<bool>("propagate_electrons");
    propagate_holes_ = config_.get<bool>("propagate_holes");
    output_plots_ = config_.get<bool>("output_plots");
    charge_creation_energy_ = config_.get<double>("charge_creation_energy");

    if(timestep_ <= 0) {
        throw InvalidValueError(config_, "timestep", "timestep needs to be positive");
    }
    if(charge_per_step_ == 0) {
        throw InvalidValueError(config_, "charge_per_step", "at least one charge carrier per group is required");
    }

    auto relative_permittivity = config_.get<double>("relative_permittivity");
    if(relative_permittivity <= 0) {
        throw InvalidValueError(config_, "relative_permittivity", "relative permittivity needs to be positive");
    }

    // Precalculate the Boltzmann constant and the Coulomb constant 1 / (4 pi eps0 eps_r):
    boltzmann_kT_ = Units::get(8.6173333e-5, "eV/K") * temperature_;
    coulomb_constant_ = 1. / (4. * M_PI * Units::get(8.8541878128e-12, "C/V/m") * relative_permittivity);

    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();
}

void SphericalRepulsionPropagationModule::initialize() {
    if(detector_->hasMagneticField()) {
        if(!config_.get<bool>("ignore_magnetic_field")) {
            throw ModuleError("This module should not be used with magnetic fields. Add the option 'ignore_magnetic_field' "
                              "to the configuration if you would like to continue.");
        }
        LOG(WARNING) << "A magnetic field is switched on, but is set to be ignored for this module.";
    }

    // Prepare mobility model
    mobility_ = Mobility(config_, model_->getSensorMaterial(), detector_->hasDopingProfile());

    // Sensor surfaces in local coordinates, the cloud is collected once its center crosses one of them
    sensor_min_z_ = model_->getSensorCenter().z() - model_->getSensorSize().z() / 2;
    sensor_max_z_ = model_->getSensorCenter().z() + model_->getSensorSize().z() / 2;

    if(output_plots_) {
        drift_time_histo_ = CreateHistogram<TH1D>("drift_time_histo",
                                                  "Drift time of charge clouds;Drift time [ns];charge carriers",
                                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                                  0,
                                                  static_cast<double>(Units::convert(integration_time_, "ns")));
        group_size_histo_ = CreateHistogram<TH1D>("group_size_histo",
                                                  "Charge carrier group size;group size;number of groups transported",
                                                  static_cast<int>(100 * charge_per_step_),
                                                  0,
                                                  static_cast<int>(100 * charge_per_step_));
        cloud_charge_histo_ =
            CreateHistogram<TH1D>("cloud_charge_histo", "Charge per cloud;cloud charge [e];charge clouds", 1000, 0, 20000);
        cloud_rms_vs_depth_ = CreateHistogram<TProfile>(
            "cloud_rms_vs_depth",
            "Lateral RMS of collected charge clouds;initial cloud position z [um];lateral RMS per axis [um]",
            100,
            static_cast<double>(Units::convert(sensor_min_z_, "um")),
            static_cast<double>(Units::convert(sensor_max_z_, "um")));
    }
}

void SphericalRepulsionPropagationModule::run(Event* event) {
    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);

    // Sort the deposits by carrier type, all deposits of one type form one charge cloud
    std::vector<const DepositedCharge*> electron_deposits;
    std::vector<const DepositedCharge*> hole_deposits;
    for(const auto& deposit : deposits_message->getData()) {
        if(deposit.getCharge() == 0) {
            continue;
        }
        if((deposit.getType() == CarrierType::ELECTRON && !propagate_electrons_) ||
           (deposit.getType() == CarrierType::HOLE && !propagate_holes_)) {
            LOG(DEBUG) << "Skipping charge carriers (" << deposit.getType() << ") on "
                       << Units::display(deposit.getLocalPosition(), {"mm", "um"});
            continue;
        }

        // Only process if within requested integration time:
        if(deposit.getLocalTime() > integration_time_) {
            LOG(DEBUG) << "Skipping charge carriers deposited beyond integration time: "
                       << Units::display(deposit.getGlobalTime(), "ns") << " global / "
                       << Units::display(deposit.getLocalTime(), {"ns", "ps"}) << " local";
            continue;
        }

        (deposit.getType() == CarrierType::ELECTRON ? electron_deposits : hole_deposits).push_back(&deposit);
    }

    std::vector<PropagatedCharge> propagated_charges;
    unsigned int total_charge = 0;
    unsigned int lost_charge = 0;
    unsigned int cloud_count = 0;

    for(auto [type, deposits] :
        {std::pair{CarrierType::ELECTRON, &electron_deposits}, std::pair{CarrierType::HOLE, &hole_deposits}}) {
        if(deposits->empty()) {
            continue;
        }
        for(const auto* deposit : *deposits) {
            total_charge += deposit->getCharge();
        }
        lost_charge += propagate_cloud(event, *deposits, type, propagated_charges);
        cloud_count++;
    }

    total_clouds_ += cloud_count;
    total_propagated_charges_ += total_charge - lost_charge;
    total_lost_charges_ += lost_charge;

    LOG(INFO) << "Propagated " << total_charge << " charges in " << cloud_count << " clouds (lost: " << lost_charge << ")";
    LOG(DEBUG) << "Total count of propagated charge carriers: " << propagated_charges.size();

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, std::move(propagated_charge_message), event);
}

unsigned int SphericalRepulsionPropagationModule::propagate_cloud(Event* event,
                                                                  const std::vector<const DepositedCharge*>& cloud,
                                                                  CarrierType type,
                                                                  std::vector<PropagatedCharge>& propagated_charges) {
    // Charge-weighted centroid of the cloud and earliest deposition time
    unsigned int cloud_charge = 0;
    ROOT::Math::XYZVector weighted_position;
    double start_time = std::numeric_limits<double>::max();
    for(const auto* deposit : cloud) {
        cloud_charge += deposit->getCharge();
        weighted_position += static_cast<ROOT::Math::XYZVector>(deposit->getLocalPosition()) * deposit->getCharge();
        start_time = std::min(start_time, deposit->getLocalTime());
    }
    auto center = ROOT::Math::XYZPoint(weighted_position / cloud_charge);
    const auto initial_center = center;

    // Width of the initial Gaussian from the photoelectron range R = 0.0171um * E[keV]^1.75, sigma = R / sqrt(15)
    const auto cloud_energy = static_cast<double>(Units::convert(cloud_charge * charge_creation_energy_, "keV"));
    const auto sigma_initial = Units::get(0.0044, "um") * std::pow(cloud_energy, 1.75);
    if(cloud_energy < 5. || cloud_energy > 25.) {
        LOG_ONCE(WARNING) << "Charge cloud with energy " << Units::display(cloud_charge * charge_creation_energy_, "keV")
                          << " outside the validated range of 5-25 keV for the initial Gaussian distribution";
    }

    LOG(DEBUG) << "Cloud of " << cloud_charge << " charge carriers (" << type << ") from " << cloud.size() << " deposits at "
               << Units::display(center, {"mm", "um"}) << ", initial width " << Units::display(sigma_initial, {"um", "nm"});

    auto charge_per_step = charge_per_step_;
    if(max_charge_groups_ > 0 && cloud_charge / charge_per_step > max_charge_groups_) {
        charge_per_step = static_cast<unsigned int>(std::ceil(static_cast<double>(cloud_charge) / max_charge_groups_));
        clouds_exceeding_max_groups_++;
        LOG(INFO) << "Cloud charge: " << cloud_charge
                  << ", which exceeds the maximum number of charge groups allowed. Increasing charge_per_step to "
                  << charge_per_step << " for this cloud.";
    }

    // Rebin the cloud into groups of charge carriers sampled from the initial Gaussian, positions are stored relative to
    // the cloud center. Each group is linked to the deposit its first carrier originates from.
    std::vector<const DepositedCharge*> group_deposit;
    std::vector<unsigned int> group_charge;
    std::vector<double> rel_x;
    std::vector<double> rel_y;
    std::vector<double> rel_z;
    allpix::normal_distribution<double> gauss_distribution(0, 1);
    auto current_deposit = cloud.begin();
    unsigned int deposit_charge_remaining = (*current_deposit)->getCharge();
    unsigned int charges_remaining = cloud_charge;
    while(charges_remaining > 0) {
        auto charge = std::min(charge_per_step, charges_remaining);
        charges_remaining -= charge;

        group_deposit.push_back(*current_deposit);
        group_charge.push_back(charge);
        rel_x.push_back(sigma_initial * gauss_distribution(event->getRandomEngine()));
        rel_y.push_back(sigma_initial * gauss_distribution(event->getRandomEngine()));
        rel_z.push_back(sigma_initial * gauss_distribution(event->getRandomEngine()));

        // Advance to the deposit the next group starts in
        while(charge >= deposit_charge_remaining && charges_remaining > 0) {
            charge -= deposit_charge_remaining;
            deposit_charge_remaining = (*(++current_deposit))->getCharge();
        }
        deposit_charge_remaining -= std::min(charge, deposit_charge_remaining);
    }

    // Holes drift along the field, electrons against it. The repulsion of like carriers always points outwards.
    const double sign = (type == CarrierType::HOLE ? 1. : -1.);
    const auto n_groups = group_charge.size();
    std::vector<size_t> order(n_groups);
    std::iota(order.begin(), order.end(), 0);
    std::vector<double> radius2(n_groups);

    auto state = CarrierState::MOTION;
    double time = 0;
    while(start_time + time < integration_time_) {
        const auto efield = detector_->getElectricField(center);
        const auto doping = detector_->getDopingConcentration(center);

        // Order the groups by their distance to the cloud center to obtain the enclosed charge
        for(size_t i = 0; i < n_groups; ++i) {
            radius2[i] = rel_x[i] * rel_x[i] + rel_y[i] * rel_y[i] + rel_z[i] * rel_z[i];
        }
        std::ranges::sort(order, [&](size_t a, size_t b) noexcept { return radius2[a] < radius2[b]; });

        double enclosed_charge = 0;
        double weighted_mobility = 0;
        for(auto i : order) {
            // A group of q carriers on the same shell sees on average (q - 1) / 2 of its own carriers enclosed
            const auto charge = static_cast<double>(group_charge[i]);
            const auto shell_charge = enclosed_charge + ((charge - 1.) / 2.);
            enclosed_charge += charge;

            // Radial repulsion field from Gauss' law, E = k Q / r^2
            double erep_x = 0;
            double erep_y = 0;
            double erep_z = 0;
            if(radius2[i] > 0) {
                const auto radius = std::sqrt(radius2[i]);
                const auto erep = coulomb_constant_ * shell_charge / radius2[i];
                erep_x = erep * rel_x[i] / radius;
                erep_y = erep * rel_y[i] / radius;
                erep_z = erep * rel_z[i] / radius;
            }

            // Mobility from the magnitude of the total field, the cloud field carries the sign of the carrier charge
            const auto total_field = std::sqrt((efield + sign * ROOT::Math::XYZVector(erep_x, erep_y, erep_z)).Mag2());
            const auto mobility = mobility_(type, total_field, doping);
            weighted_mobility += mobility * charge;

            // Repulsion and diffusion relative to the cloud center
            const auto diffusion_std_dev = std::sqrt(2. * boltzmann_kT_ * mobility * timestep_);
            rel_x[i] += mobility * erep_x * timestep_ + diffusion_std_dev * gauss_distribution(event->getRandomEngine());
            rel_y[i] += mobility * erep_y * timestep_ + diffusion_std_dev * gauss_distribution(event->getRandomEngine());
            rel_z[i] += mobility * erep_z * timestep_ + diffusion_std_dev * gauss_distribution(event->getRandomEngine());
        }

        // Drift of the cloud center with the mean mobility of the cloud
        const auto drift_step = sign * weighted_mobility / enclosed_charge * efield * timestep_;
        center += drift_step;
        time += timestep_;

        // The cloud is collected once its center reaches the sensor surface it drifts towards
        if((drift_step.z() > 0 && center.z() >= sensor_max_z_) || (drift_step.z() < 0 && center.z() <= sensor_min_z_)) {
            center.SetZ(std::clamp(center.z(), sensor_min_z_, sensor_max_z_));
            state = CarrierState::HALTED;
            break;
        }
    }

    LOG(DEBUG) << "Cloud " << (state == CarrierState::HALTED ? "collected" : "stopped at integration time") << " at "
               << Units::display(center, {"mm", "um"}) << " after " << Units::display(time, {"ns", "ps"});

    // Store the charge groups at their final positions, collected groups are projected onto the collection surface
    unsigned int lost_charge = 0;
    double rms2_sum = 0;
    for(size_t i = 0; i < n_groups; ++i) {
        const auto* deposit = group_deposit[i];
        auto local_position = ROOT::Math::XYZPoint(center.x() + rel_x[i],
                                                   center.y() + rel_y[i],
                                                   state == CarrierState::HALTED ? center.z() : center.z() + rel_z[i]);
        rms2_sum += (rel_x[i] * rel_x[i] + rel_y[i] * rel_y[i]) * group_charge[i];

        if(!model_->isWithinSensor(local_position)) {
            LOG(DEBUG) << "Charge carriers outside sensor volume at " << Units::display(local_position, {"mm", "um"});
            lost_charge += group_charge[i];
            continue;
        }

        if(output_plots_) {
            group_size_histo_->Fill(group_charge[i]);
        }

        propagated_charges.emplace_back(local_position,
                                        detector_->getGlobalPosition(local_position),
                                        type,
                                        group_charge[i],
                                        deposit->getLocalTime() + time,
                                        deposit->getGlobalTime() + time,
                                        state,
                                        deposit);
    }

    if(output_plots_) {
        drift_time_histo_->Fill(static_cast<double>(Units::convert(time, "ns")), cloud_charge);
        cloud_charge_histo_->Fill(cloud_charge);
        if(state == CarrierState::HALTED) {
            cloud_rms_vs_depth_->Fill(static_cast<double>(Units::convert(initial_center.z(), "um")),
                                      static_cast<double>(Units::convert(std::sqrt(rms2_sum / (2. * cloud_charge)), "um")));
        }
    }

    return lost_charge;
}

void SphericalRepulsionPropagationModule::finalize() {
    if(output_plots_) {
        group_size_histo_->Get()->GetXaxis()->SetRange(1, group_size_histo_->Get()->GetNbinsX() + 1);
    }

    LOG(INFO) << "Propagated total of " << total_propagated_charges_ << " charges in " << total_clouds_ << " clouds, lost "
              << total_lost_charges_ << " charges outside the sensor";
    LOG(INFO) << clouds_exceeding_max_groups_ * 100.0 / std::max(1U, static_cast<unsigned int>(total_clouds_))
              << "% of clouds have charge exceeding the " << max_charge_groups_
              << " charge groups allowed, with a charge_per_step value of " << charge_per_step_ << ".";
}
