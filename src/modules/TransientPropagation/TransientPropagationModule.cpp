/**
 * @file
 * @brief Implementation of charge propagation module with transient behavior simulation
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "TransientPropagationModule.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <Eigen/Core>

#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "objects/exceptions.h"
#include "tools/runge_kutta.h"

using namespace allpix;
using namespace ROOT::Math;

TransientPropagationModule::TransientPropagationModule(Configuration& config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector:
    messenger_->bindSingle<DepositedChargeMessage>(this, MsgFlags::REQUIRED);

    // Set default value for config variables
    config_.setDefault<double>("timestep", Units::get(0.01, "ns"));
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 10);
    config_.setDefault<unsigned int>("max_charge_groups", 1000);

    // Models:
    config_.setDefault<std::string>("mobility_model", "jacoboni");
    config_.setDefault<std::string>("recombination_model", "none");
    config_.setDefault<std::string>("trapping_model", "none");

    config_.setDefault<double>("temperature", 293.15);
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<unsigned int>("distance", 1);
    config_.setDefault<bool>("ignore_magnetic_field", false);
    config_.setDefault<bool>("enable_charge_multiplication", false);
    config_.setDefault<double>("charge_multiplication_threshold", 1e-2);

    // Copy some variables from configuration to avoid lookups:
    temperature_ = config_.get<double>("temperature");
    timestep_ = config_.get<double>("timestep");
    integration_time_ = config_.get<double>("integration_time");
    distance_ = config_.get<unsigned int>("distance");
    charge_per_step_ = config_.get<unsigned int>("charge_per_step");
    max_charge_groups_ = config_.get<unsigned int>("max_charge_groups");
    enable_multiplication_ = config_.get<bool>("enable_charge_multiplication");
    threshold_field_ = config_.get<double>("charge_multiplication_threshold");

    output_plots_ = config_.get<bool>("output_plots");
    boltzmann_kT_ = Units::get(8.6173e-5, "eV/K") * temperature_;

    // Parameter for charge transport in magnetic field (approximated from graphs:
    // http://www.ioffe.ru/SVA/NSM/Semicond/Si/electric.html) FIXME
    electron_Hall_ = 1.15;
    hole_Hall_ = 0.9;
}

void TransientPropagationModule::initialize() {

    // Check for electric field
    if(!detector_->hasElectricField()) {
        LOG(WARNING) << "This detector does not have an electric field.";
    }

    if(!detector_->hasWeightingPotential()) {
        throw ModuleError("This module requires a weighting potential.");
    }

    if(detector_->getElectricFieldType() == FieldType::LINEAR) {
        throw ModuleError("This module cannot be used with linear electric fields.");
    }

    // Prepare mobility model
    mobility_ = Mobility(config_, model_->getSensorMaterial(), detector_->hasDopingProfile());

    // Prepare recombination model
    recombination_ = Recombination(config_, detector_->hasDopingProfile());

    // Prepare trapping model
    trapping_ = Trapping(config_);

    // Check for magnetic field
    has_magnetic_field_ = detector_->hasMagneticField();
    if(has_magnetic_field_) {
        if(config_.get<bool>("ignore_magnetic_field")) {
            has_magnetic_field_ = false;
            LOG(WARNING) << "A magnetic field is switched on, but is set to be ignored for this module.";
        } else {
            LOG(DEBUG) << "This detector sees a magnetic field.";
        }
    }

    if(output_plots_) {
        potential_difference_ = CreateHistogram<TH1D>(
            "potential_difference",
            "Weighting potential difference between two steps;#left|#Delta#phi_{w}#right| [a.u.];events",
            500,
            0,
            1);
        induced_charge_histo_ = CreateHistogram<TH1D>("induced_charge_histo",
                                                      "Induced charge per time, all pixels;Drift time [ns];charge [e]",
                                                      static_cast<int>(integration_time_ / timestep_),
                                                      0,
                                                      static_cast<double>(Units::convert(integration_time_, "ns")));
        induced_charge_e_histo_ =
            CreateHistogram<TH1D>("induced_charge_e_histo",
                                  "Induced charge per time, electrons only, all pixels;Drift time [ns];charge [e]",
                                  static_cast<int>(integration_time_ / timestep_),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")));
        induced_charge_h_histo_ =
            CreateHistogram<TH1D>("induced_charge_h_histo",
                                  "Induced charge per time, holes only, all pixels;Drift time [ns];charge [e]",
                                  static_cast<int>(integration_time_ / timestep_),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")));
        step_length_histo_ =
            CreateHistogram<TH1D>("step_length_histo",
                                  "Step length;length [#mum];integration steps",
                                  100,
                                  0,
                                  static_cast<double>(Units::convert(0.25 * model_->getSensorSize().z(), "um")));
        group_size_histo_ = CreateHistogram<TH1D>("group_size_histo",
                                                  "Group size;size [charges];Number of groups",
                                                  static_cast<int>(100 * charge_per_step_),
                                                  0,
                                                  static_cast<int>(100 * charge_per_step_));

        drift_time_histo_ = CreateHistogram<TH1D>("drift_time_histo",
                                                  "Drift time;Drift time [ns];charge carriers",
                                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                                  0,
                                                  static_cast<double>(Units::convert(integration_time_, "ns")));

        recombine_histo_ =
            CreateHistogram<TH1D>("recombination_histo",
                                  "Fraction of recombined charge carriers;recombination [N / N_{total}] ;number of events",
                                  100,
                                  0,
                                  1);
        trapped_histo_ = CreateHistogram<TH1D>(
            "trapping_histo", "Fraction of trapped charge carriers;trapping [N / N_{total}] ;number of events", 100, 0, 1);
    }
}

void TransientPropagationModule::run(Event* event) {
    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;
    unsigned int propagated_charges_count = 0;
    unsigned int recombined_charges_count = 0;
    unsigned int trapped_charges_count = 0;

    // Loop over all deposits for propagation
    LOG(TRACE) << "Propagating charges in sensor";
    for(const auto& deposit : deposits_message->getData()) {

        // Only process if within requested integration time:
        if(deposit.getLocalTime() > integration_time_) {
            LOG(DEBUG) << "Skipping charge carriers deposited beyond integration time: "
                       << Units::display(deposit.getGlobalTime(), "ns") << " global / "
                       << Units::display(deposit.getLocalTime(), {"ns", "ps"}) << " local";
            continue;
        }

        total_deposits_++;

        // Loop over all charges in the deposit
        unsigned int charges_remaining = deposit.getCharge();

        LOG(DEBUG) << "Set of charge carriers (" << deposit.getType() << ") on "
                   << Units::display(deposit.getLocalPosition(), {"mm", "um"});

        auto charge_per_step = charge_per_step_;
        if(max_charge_groups_ > 0 && deposit.getCharge() / charge_per_step > max_charge_groups_) {
            charge_per_step = static_cast<unsigned int>(ceil(static_cast<double>(deposit.getCharge()) / max_charge_groups_));
            deposits_exceeding_max_groups_++;
            LOG(INFO) << "Deposited charge: " << deposit.getCharge()
                      << ", which exceeds the maximum number of charge groups allowed. Increasing charge_per_step to "
                      << charge_per_step << " for this deposit.";
        }
        while(charges_remaining > 0) {
            // Define number of charges to be propagated and remove charges of this step from the total
            if(charge_per_step > charges_remaining) {
                charge_per_step = charges_remaining;
            }
            charges_remaining -= charge_per_step;
            std::map<Pixel::Index, Pulse> px_map;

            // Get position and propagate through sensor
            auto [local_position, time, gain, state] = propagate(
                event, deposit.getLocalPosition(), deposit.getType(), charge_per_step, deposit.getLocalTime(), px_map);

            // Create a new propagated charge and add it to the list
            auto global_position = detector_->getGlobalPosition(local_position);
            PropagatedCharge propagated_charge(local_position,
                                               global_position,
                                               deposit.getType(),
                                               std::move(px_map),
                                               deposit.getLocalTime() + time,
                                               deposit.getGlobalTime() + time,
                                               state,
                                               &deposit);

            LOG(DEBUG) << " Propagated " << charge_per_step << " to " << Units::display(local_position, {"mm", "um"})
                       << " in " << Units::display(time, "ns") << " time, induced "
                       << Units::display(propagated_charge.getCharge(), {"e"})
                       << ", final state: " << allpix::to_string(state);

            propagated_charges.push_back(std::move(propagated_charge));

            if(state == CarrierState::RECOMBINED) {
                recombined_charges_count += charge_per_step;
            } else if(state == CarrierState::TRAPPED) {
                trapped_charges_count += charge_per_step;
            } else {
                propagated_charges_count += charge_per_step;
            }

            if(output_plots_) {
                drift_time_histo_->Fill(static_cast<double>(Units::convert(time, "ns")), charge_per_step);
                group_size_histo_->Fill(charge_per_step);
            }
        }
    }

    if(output_plots_) {
        auto total = (propagated_charges_count + recombined_charges_count + trapped_charges_count);
        recombine_histo_->Fill(static_cast<double>(recombined_charges_count) / (total == 0 ? 1 : total));
        trapped_histo_->Fill(static_cast<double>(trapped_charges_count) / (total == 0 ? 1 : total));
    }

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, propagated_charge_message, event);
}

/**
 * Propagation is simulated using a parameterization for the electron mobility. This is used to calculate the electron
 * velocity at every point with help of the electric field map of the detector. A Runge-Kutta integration is applied in
 * multiple steps, adding a random diffusion to the propagating charge every step.
 */
std::tuple<ROOT::Math::XYZPoint, double, double, CarrierState>
TransientPropagationModule::propagate(Event* event,
                                      const ROOT::Math::XYZPoint& pos,
                                      const CarrierType& type,
                                      const unsigned int charge,
                                      const double initial_time,
                                      std::map<Pixel::Index, Pulse>& pixel_map) {
    Eigen::Vector3d position(pos.x(), pos.y(), pos.z());

    // Initialise gain
    double gain = 1;

    // Define a function to compute the diffusion
    auto carrier_diffusion = [&](double efield_mag, double doping, double timestep) -> Eigen::Vector3d {
        double diffusion_constant = boltzmann_kT_ * mobility_(type, efield_mag, doping);
        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * timestep);

        // Compute the independent diffusion in three
        allpix::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
        Eigen::Vector3d diffusion;
        for(int i = 0; i < 3; ++i) {
            diffusion[i] = gauss_distribution(event->getRandomEngine());
        }
        return diffusion;
    };

    // Define a function to compute the charge carrier multiplcation
    auto carrier_multiplication = [&](double efield_mag, double step_length) -> double {
        // experimental parameters from Massey model
        double a_n = 4.43e4;  // in mm^-1
        double a_p = 1.13e5;  // in mm^-1
        double c_n = 9.66e-2; // in MV mm^-1
        double c_p = 1.71e-1; // in MV mm^-1
        double d_n = 4.99e-5; // in MV mm^-1 K^-1
        double d_p = 1.09e-4; // in MV mm^-1 K^-1

        // Compute the gain
        if(abs(efield_mag) > threshold_field_) {

            // ionisation coefficient for electrons
            double b_n = c_n + d_n * temperature_;
            double alpha_ = a_n * std::exp(-(b_n / efield_mag));

            // ionisation coefficient for holes
            double b_p = c_p + d_p * temperature_;
            double beta_ = a_p * std::exp(-(b_p / efield_mag));

            return std::exp(step_length * (type == CarrierType::ELECTRON ? alpha_ : beta_));
        } else {
            return 1.0;
        }
    };

    // Survival probability of this charge carrier package, evaluated at every step
    std::uniform_real_distribution<double> survival(0, 1);

    // Define lambda functions to compute the charge carrier velocity with or without magnetic field
    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&)> carrier_velocity_noB =
        [&](double, const Eigen::Vector3d& cur_pos) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());

        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(cur_pos));

        return static_cast<int>(type) * mobility_(type, efield.norm(), doping) * efield;
    };

    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&)> carrier_velocity_withB =
        [&](double, const Eigen::Vector3d& cur_pos) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());

        Eigen::Vector3d velocity;
        auto magnetic_field = detector_->getMagneticField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d bfield(magnetic_field.x(), magnetic_field.y(), magnetic_field.z());

        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(cur_pos));

        auto mob = mobility_(type, efield.norm(), doping);
        auto exb = efield.cross(bfield);

        Eigen::Vector3d term1;
        double hallFactor = (type == CarrierType::ELECTRON ? electron_Hall_ : hole_Hall_);
        term1 = static_cast<int>(type) * mob * hallFactor * exb;

        Eigen::Vector3d term2 = mob * mob * hallFactor * hallFactor * efield.dot(bfield) * bfield;

        auto rnorm = 1 + mob * mob * hallFactor * hallFactor * bfield.dot(bfield);
        return static_cast<int>(type) * mob * (efield + term1 + term2) / rnorm;
    };

    // Create the runge kutta solver with an RKF5 tableau
    auto runge_kutta = make_runge_kutta(
        tableau::RK5, (has_magnetic_field_ ? carrier_velocity_withB : carrier_velocity_noB), timestep_, position);

    // Continue propagation until the deposit is outside the sensor
    Eigen::Vector3d last_position = position;
    auto state = CarrierState::MOTION;
    while(state == CarrierState::MOTION && (initial_time + runge_kutta.getTime()) < integration_time_) {
        // Save previous position and time
        last_position = position;

        // Execute a Runge Kutta step
        auto step = runge_kutta.step();

        // Get the current result
        position = runge_kutta.getValue();

        // Get electric field at current position and fall back to empty field if it does not exist
        auto efield = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(position));
        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(position));

        // Apply diffusion step
        auto diffusion = carrier_diffusion(std::sqrt(efield.Mag2()), doping, timestep_);
        position += diffusion;
        runge_kutta.setValue(position);

        // Check if charge carrier is still alive:
        if(recombination_(type,
                          detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(position)),
                          survival(event->getRandomEngine()),
                          timestep_)) {
            state = CarrierState::RECOMBINED;
        }

        // Check if the charge carrier has been trapped:
        auto [trapped, traptime] = trapping_(type, survival(event->getRandomEngine()), timestep_, std::sqrt(efield.Mag2()));
        if(trapped) {
            if((initial_time + runge_kutta.getTime() + traptime) < integration_time_) {
                // De-trap and advance in time if still below integration time
                LOG(TRACE) << "De-trapping charge carrier after " << Units::display(traptime, {"ns", "us"});
                runge_kutta.advanceTime(traptime);
            } else {
                // Mark as trapped otherwise
                state = CarrierState::TRAPPED;
            }
        }

        // Apply multiplication step, fully deterministic from local efield and step length
        double step_length = step.value.norm();
        if(enable_multiplication_) {
            gain *= carrier_multiplication(std::sqrt(efield.Mag2()), step_length);
        }

        // Update step length histogram
        if(output_plots_) {
            step_length_histo_->Fill(static_cast<double>(Units::convert(step.value.norm(), "um")));
        }

        // Check for overshooting outside the sensor and correct for it:
        if(!model_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position))) {
            LOG(TRACE) << "Carrier outside sensor: " << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"nm"});
            state = CarrierState::HALTED;

            auto intercept = model_->getSensorIntercept(static_cast<ROOT::Math::XYZPoint>(last_position),
                                                        static_cast<ROOT::Math::XYZPoint>(position));
            position = Eigen::Vector3d(intercept.x(), intercept.y(), intercept.z());
            LOG(TRACE) << "Moved carrier to: " << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"nm"});
        }

        // Find the nearest pixel - before and after the step
        auto [xpixel, ypixel] = model_->getPixelIndex(static_cast<ROOT::Math::XYZPoint>(position));
        auto [last_xpixel, last_ypixel] = model_->getPixelIndex(static_cast<ROOT::Math::XYZPoint>(last_position));
        auto idx = Pixel::Index(xpixel, ypixel);
        auto neighbors = model_->getNeighbors(idx, distance_);

        // If the charge carrier crossed pixel boundaries, ensure that we always calculate the induced current for both of
        // them by extending the induction matrix temporarily. Otherwise we end up doing "double-counting" because we would
        // only jump "into" a pixel but never "out". At the border of the induction matrix, this would create an imbalance.
        if(last_xpixel != xpixel || last_ypixel != ypixel) {
            auto last_idx = Pixel::Index(last_xpixel, last_ypixel);
            neighbors.merge(model_->getNeighbors(last_idx, distance_));
            LOG(TRACE) << "Carrier crossed boundary from pixel " << Pixel::Index(last_xpixel, last_ypixel) << " to pixel "
                       << Pixel::Index(xpixel, ypixel);
        }
        LOG(TRACE) << "Moving carriers below pixel " << Pixel::Index(xpixel, ypixel) << " from "
                   << Units::display(static_cast<ROOT::Math::XYZPoint>(last_position), {"um", "mm"}) << " to "
                   << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um", "mm"}) << ", "
                   << Units::display(initial_time + runge_kutta.getTime(), "ns");

        for(const auto& pixel_index : neighbors) {
            auto ramo = detector_->getWeightingPotential(static_cast<ROOT::Math::XYZPoint>(position), pixel_index);
            auto last_ramo = detector_->getWeightingPotential(static_cast<ROOT::Math::XYZPoint>(last_position), pixel_index);

            // Induced charge on electrode is q_int = q * (phi(x1) - phi(x0))
            auto induced = charge * (ramo - last_ramo) * static_cast<std::underlying_type<CarrierType>::type>(type);
            LOG(TRACE) << "Pixel " << pixel_index << " dPhi = " << (ramo - last_ramo) << ", induced " << type
                       << " q = " << Units::display(induced, "e");

            // Create pulse if it doesn't exist. Store induced charge in the returned pulse iterator
            auto pixel_map_iterator = pixel_map.emplace(pixel_index, Pulse(timestep_, integration_time_));
            try {
                pixel_map_iterator.first->second.addCharge(induced, initial_time + runge_kutta.getTime());
            } catch(const PulseBadAllocException& e) {
                LOG(ERROR) << e.what() << std::endl
                           << "Ignoring pulse contribution at time "
                           << Units::display(initial_time + runge_kutta.getTime(), {"ms", "us", "ns"});
            }

            if(output_plots_) {
                potential_difference_->Fill(std::fabs(ramo - last_ramo));
                induced_charge_histo_->Fill(initial_time + runge_kutta.getTime(), induced);
                if(type == CarrierType::ELECTRON) {
                    induced_charge_e_histo_->Fill(initial_time + runge_kutta.getTime(), induced);
                } else {
                    induced_charge_h_histo_->Fill(initial_time + runge_kutta.getTime(), induced);
                }
            }
        }
    }

    // Return the final position of the propagated charge
    return std::make_tuple(static_cast<ROOT::Math::XYZPoint>(position), initial_time + runge_kutta.getTime(), gain, state);
}

void TransientPropagationModule::finalize() {
    LOG(INFO) << deposits_exceeding_max_groups_ * 100.0 / total_deposits_ << "% of deposits have charge exceeding the "
              << max_charge_groups_ << " charge groups allowed, with a charge_per_step value of " << charge_per_step_ << ".";
    if(output_plots_) {
        group_size_histo_->Get()->GetXaxis()->SetRange(1, group_size_histo_->Get()->GetNbinsX() + 1);

        potential_difference_->Write();
        step_length_histo_->Write();
        group_size_histo_->Write();
        drift_time_histo_->Write();
        recombine_histo_->Write();
        trapped_histo_->Write();
        induced_charge_histo_->Write();
        induced_charge_e_histo_->Write();
        induced_charge_h_histo_->Write();
    }
}
