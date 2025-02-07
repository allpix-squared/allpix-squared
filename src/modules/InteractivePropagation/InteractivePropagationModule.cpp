/**
 * @file
 * @brief Implementation of InteractivePropagation module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "InteractivePropagationModule.hpp"

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

// Copied from TransientPropagation.cpp
void InteractivePropagationModule::run(Event* event) {
    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;
    // unsigned int propagated_charges_count = 0;
    // unsigned int recombined_charges_count = 0;
    // unsigned int trapped_charges_count = 0;

    // List of points to plot to plot for output plots
    LineGraph::OutputPlotPoints output_plot_points;

    // Create vector of propagating charges to store each charge groups position, location, time, type, etc.
    std::vector<PropagatedCharge> propagating_charges;

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

            auto [recombined, trapped, propagated] = propagate(event,
                                                               deposit,
                                                               deposit.getLocalPosition(),
                                                               deposit.getType(),
                                                               charge_per_step,
                                                               deposit.getLocalTime(),
                                                               deposit.getGlobalTime(),
                                                               0,
                                                               propagated_charges,
                                                               output_plot_points); // test

            // Add charge to propagating charge vector to be time-stepped later
            PropagatedCharge propagating_charge(deposit.getLocalPosition(),
                                            deposit.getGlobalPosition(),
                                            deposit.getType(),
                                            charge_per_step,
                                            deposit.getLocalTime(),
                                            deposit.getGlobalTime(),
                                            CarrierState::MOTION,
                                            &deposit);

            propagating_charges.push_back(std::move(propagating_charge));

            // Add point of deposition to the output plots if requested
            if(output_linegraphs_) {
                output_plot_points.emplace_back(std::make_tuple(deposit.getGlobalTime(), charge_per_step, deposit.getType(), CarrierState::MOTION),
                                                std::vector<ROOT::Math::XYZPoint>());
            }
        }
    }

    //Now we have a list of charges to propagate
    auto [recombined_charges_count, trapped_charges_count, propagated_charges_count] = propagate_together(event, propagating_charges, propagated_charges, output_plot_points);

    // Output plots if required
    if(output_linegraphs_) {
        LineGraph::Create(event->number, this, config_, output_plot_points, CarrierState::UNKNOWN);
        if(output_linegraphs_collected_) {
            LineGraph::Create(event->number, this, config_, output_plot_points, CarrierState::HALTED);
        }
        if(output_linegraphs_recombined_) {
            LineGraph::Create(event->number, this, config_, output_plot_points, CarrierState::RECOMBINED);
        }
        if(output_linegraphs_trapped_) {
            LineGraph::Create(event->number, this, config_, output_plot_points, CarrierState::TRAPPED);
        }
        if(config_.get<bool>("output_animations")) {
            LineGraph::Animate(event->number, this, config_, output_plot_points);
        }
    }

    LOG(INFO) << "Propagated " << propagated_charges_count << " charges" << std::endl
              << "Recombined " << recombined_charges_count << " charges during transport" << std::endl
              << "Trapped " << trapped_charges_count << " charges during transport";

    if(output_plots_) {
        auto total = (propagated_charges_count + recombined_charges_count + trapped_charges_count);
        recombine_histo_->Fill(static_cast<double>(recombined_charges_count) / (total == 0 ? 1 : total));
        trapped_histo_->Fill(static_cast<double>(trapped_charges_count) / (total == 0 ? 1 : total));
    }

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, std::move(propagated_charge_message), event);
}

/**
 * Propagation is simulated using a parameterization for the electron mobility. This is used to calculate the electron
 * velocity at every point with help of the electric field map of the detector. A Runge-Kutta integration is applied in
 * multiple steps, adding a random diffusion to the propagating charge every step.
 */
// Initially copied from TransientPropagation.cpp
std::tuple<unsigned int, unsigned int, unsigned int> InteractivePropagationModule::propagate(Event* event,
                                      const DepositedCharge& deposit,
                                      const ROOT::Math::XYZPoint& pos,
                                      const CarrierType& type,
                                      unsigned int charge,
                                      const double initial_time_local,
                                      const double initial_time_global,
                                      const unsigned int level,
                                      std::vector<PropagatedCharge>& propagated_charges,
                                      LineGraph::OutputPlotPoints& output_plot_points) const {

    if(level > max_multiplication_level_) {
        LOG(WARNING) << "Found impact ionization shower with level larger than " << max_multiplication_level_
                     << ", interrupting";
        return {};
    }

    Eigen::Vector3d position(pos.x(), pos.y(), pos.z());
    std::map<Pixel::Index, Pulse> pixel_map;

    unsigned int propagated_charges_count = 0;
    unsigned int recombined_charges_count = 0;
    unsigned int trapped_charges_count = 0;

    // Add point of deposition to the output plots if requested
    if(output_linegraphs_) {
        output_plot_points.emplace_back(std::make_tuple(deposit.getGlobalTime(), charge, type, CarrierState::MOTION),
                                        std::vector<ROOT::Math::XYZPoint>());
    }
    auto output_plot_index = output_plot_points.size() - 1;

    // Store initial charge
    const unsigned int initial_charge = charge;

    // Define a function to compute the diffusion
    auto carrier_diffusion = [&](double efield_mag, double doping, double timestep) -> Eigen::Vector3d {
        double diffusion_constant = boltzmann_kT_ * mobility_(type, efield_mag, doping);
        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * timestep);

        // Compute the independent diffusion in three
        allpix::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
        auto x = gauss_distribution(event->getRandomEngine());
        auto y = gauss_distribution(event->getRandomEngine());
        auto z = gauss_distribution(event->getRandomEngine());
        return {x, y, z};
    };

    // Survival probability of this charge carrier package, evaluated at every step
    allpix::uniform_real_distribution<double> uniform_distribution(0, 1);

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

    // Create the runge kutta solver with an RK4 tableau, no error estimation required since we're not adapting step size
    auto runge_kutta = make_runge_kutta(
        tableau::RK4, (has_magnetic_field_ ? carrier_velocity_withB : carrier_velocity_noB), timestep_, position);

    // Continue propagation until the deposit is outside the sensor
    Eigen::Vector3d last_position = position;
    ROOT::Math::XYZVector efield{}, last_efield{};
    size_t next_idx = 0;
    auto state = CarrierState::MOTION;
    while(state == CarrierState::MOTION && (initial_time_local + runge_kutta.getTime()) < integration_time_) {
        // Update output plots if necessary (depending on the plot step)
        if(output_linegraphs_) { // Set final state of charge carrier for plotting:
            auto time_idx = static_cast<size_t>(runge_kutta.getTime() / output_plots_step_);
            while(next_idx <= time_idx) {
                output_plot_points.at(output_plot_index).second.push_back(static_cast<ROOT::Math::XYZPoint>(position));
                next_idx = output_plot_points.at(output_plot_index).second.size();
            }
        }

        // Save previous position and time
        last_position = position;
        last_efield = efield;

        // Get electric field at current (pre-step) position
        efield = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(position));
        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(position));

        // Execute a Runge Kutta step
        auto step = runge_kutta.step();

        // Get the current result
        position = runge_kutta.getValue();

        // Apply diffusion step
        auto diffusion = carrier_diffusion(std::sqrt(efield.Mag2()), doping, timestep_);
        position += diffusion;

        // If charge carrier reaches implant, interpolate surface position for higher accuracy:
        if(auto implant = model_->isWithinImplant(static_cast<ROOT::Math::XYZPoint>(position))) {
            LOG(TRACE) << "Carrier in implant: " << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"nm"});
            auto new_position = model_->getImplantIntercept(implant.value(),
                                                            static_cast<ROOT::Math::XYZPoint>(last_position),
                                                            static_cast<ROOT::Math::XYZPoint>(position));
            position = Eigen::Vector3d(new_position.x(), new_position.y(), new_position.z());
            state = CarrierState::HALTED;
        }

        // Check for overshooting outside the sensor and correct for it:
        if(!model_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position))) {
            // Reflect off the sensor surface with a certain probability, otherwise halt motion:
            if(uniform_distribution(event->getRandomEngine()) > surface_reflectivity_) {
                LOG(TRACE) << "Carrier outside sensor: "
                           << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"nm"});
                state = CarrierState::HALTED;
            }

            auto intercept = model_->getSensorIntercept(static_cast<ROOT::Math::XYZPoint>(last_position),
                                                        static_cast<ROOT::Math::XYZPoint>(position));

            if(state == CarrierState::HALTED) {
                position = Eigen::Vector3d(intercept.x(), intercept.y(), intercept.z());
            } else {
                // geom. reflection on x-y plane at upper sensor boundary (we have an implant on the lower edge)
                position = Eigen::Vector3d(position.x(), position.y(), 2. * intercept.z() - position.z());
                LOG(TRACE) << "Carrier was reflected on the sensor surface to "
                           << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um", "nm"});

                // Re-check if we ended in an implant - corner case.
                if(model_->isWithinImplant(static_cast<ROOT::Math::XYZPoint>(position))) {
                    LOG(TRACE) << "Ended in implant after reflection - halting";
                    state = CarrierState::HALTED;
                }

                // Re-check if we are within the sensor - reflection at sensor side walls:
                if(!model_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position))) {
                    position = Eigen::Vector3d(intercept.x(), intercept.y(), intercept.z());
                    state = CarrierState::HALTED;
                }
            }
            LOG(TRACE) << "Moved carrier to: " << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"nm"});
        }

        // Update final position after applying corrections from surface intercepts
        runge_kutta.setValue(position);

        // Update step length histogram
        if(output_plots_) {
            step_length_histo_->Fill(static_cast<double>(Units::convert(step.value.norm(), "um")));
        }

        // Physics effects:

        // Check if charge carrier is still alive:
        if(state == CarrierState::MOTION &&
           recombination_(type, doping, uniform_distribution(event->getRandomEngine()), timestep_)) {
            state = CarrierState::RECOMBINED;
        }

        // Check if the charge carrier has been trapped:
        if(state == CarrierState::MOTION &&
           trapping_(type, uniform_distribution(event->getRandomEngine()), timestep_, std::sqrt(efield.Mag2()))) {
            if(output_plots_) {
                trapping_time_histo_->Fill(runge_kutta.getTime(), charge);
            }

            auto detrap_time = detrapping_(type, uniform_distribution(event->getRandomEngine()), std::sqrt(efield.Mag2()));
            if((initial_time_local + runge_kutta.getTime() + detrap_time) < integration_time_) {
                // De-trap and advance in time if still below integration time
                LOG(TRACE) << "De-trapping charge carrier after " << Units::display(detrap_time, {"ns", "us"});
                runge_kutta.advanceTime(detrap_time);

                if(output_plots_) {
                    detrapping_time_histo_->Fill(static_cast<double>(Units::convert(detrap_time, "ns")), charge);
                }
            } else {
                // Mark as trapped otherwise
                state = CarrierState::TRAPPED;
            }
        }

        // Apply multiplication step: calculate gain factor from local efield and step length; Interpolate efield values
        // The multiplication factor is not scaled by the velocity fraction parallel to the electric field, as the
        // correction is negligible for semiconductors
        auto local_gain =
            multiplication_(type, (std::sqrt(efield.Mag2()) + std::sqrt(last_efield.Mag2())) / 2., step.value.norm());

        unsigned int n_secondaries = 0;

        if(local_gain > 1.0) {
            LOG(DEBUG) << "Calculated local gain of " << local_gain << " for step of "
                       << Units::display(step.value.norm(), {"um", "nm"}) << " from field of "
                       << Units::display(std::sqrt(last_efield.Mag2()), "kV/cm") << " to "
                       << Units::display(std::sqrt(efield.Mag2()), "kV/cm");

            // For each charge carrier draw a number to determine the number of
            // secondaries generated in this step
            double log_prob = 1. / std::log1p(-1. / local_gain);
            for(unsigned int i_carrier = 0; i_carrier < charge; ++i_carrier) {
                n_secondaries +=
                    static_cast<unsigned int>(std::log(uniform_distribution(event->getRandomEngine())) * log_prob);
            }
            if(n_secondaries != 0) {
                // Generate new charge carriers of the opposite type
                // Same-type charge carriers are generated by increasing the charge at the end of the step
                auto inverted_type = invertCarrierType(type);
                // Placing new charge carrier at the end of the step:
                auto carrier_pos = static_cast<ROOT::Math::XYZPoint>(position);
                LOG(DEBUG) << "Set of charge carriers (" << inverted_type << ") generated from impact ionization on "
                           << Units::display(carrier_pos, {"mm", "um"});
                if(output_plots_) {
                    multiplication_depth_histo_->Fill(carrier_pos.z(), n_secondaries);
                }

                auto [recombined, trapped, propagated] = propagate(event,
                                                                   deposit,
                                                                   carrier_pos,
                                                                   inverted_type,
                                                                   n_secondaries,
                                                                   initial_time_local + runge_kutta.getTime(),
                                                                   initial_time_global + runge_kutta.getTime(),
                                                                   level + 1,
                                                                   propagated_charges,
                                                                   output_plot_points);

                // Update statistics:
                recombined_charges_count += recombined;
                trapped_charges_count += trapped;
                propagated_charges_count += propagated;

                LOG(DEBUG) << "Continuing propagation of charge carrier set (" << type << ") at "
                           << Units::display(carrier_pos, {"mm", "um"});
            }

            auto gain = static_cast<double>(charge + n_secondaries) / initial_charge;
            if(gain > 50.) {
                LOG(WARNING) << "Detected gain of " << gain << ", local electric field of "
                             << Units::display(std::sqrt(efield.Mag2()), "kV/cm") << ", diode seems to be in breakdown";
            }
        }

        // Signal calculation:

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
                   << Units::display(initial_time_local + runge_kutta.getTime(), "ns");

        for(const auto& pixel_index : neighbors) {
            auto ramo = detector_->getWeightingPotential(static_cast<ROOT::Math::XYZPoint>(position), pixel_index);
            auto last_ramo = detector_->getWeightingPotential(static_cast<ROOT::Math::XYZPoint>(last_position), pixel_index);

            // Induced charge on electrode is q_int = q * (phi(x1) - phi(x0))
            auto induced = charge * (ramo - last_ramo) * static_cast<std::underlying_type<CarrierType>::type>(type);

            auto induced_primary = level != 0 ? 0.
                                              : initial_charge * (ramo - last_ramo) *
                                                    static_cast<std::underlying_type<CarrierType>::type>(type);
            auto induced_secondary = induced - induced_primary;

            LOG(TRACE) << "Pixel " << pixel_index << " dPhi = " << (ramo - last_ramo) << ", induced " << type
                       << " q = " << Units::display(induced, "e");

            // Create pulse if it doesn't exist. Store induced charge in the returned pulse iterator
            auto pixel_map_iterator = pixel_map.emplace(pixel_index, Pulse(timestep_, integration_time_));
            try {
                pixel_map_iterator.first->second.addCharge(induced, initial_time_local + runge_kutta.getTime());
            } catch(const PulseBadAllocException& e) {
                LOG(ERROR) << e.what() << std::endl
                           << "Ignoring pulse contribution at time "
                           << Units::display(initial_time_local + runge_kutta.getTime(), {"ms", "us", "ns"});
            }

            if(output_plots_) {
                auto inPixel_um_x = (position.x() - model_->getPixelCenter(xpixel, ypixel).x()) * 1e3;
                auto inPixel_um_y = (position.y() - model_->getPixelCenter(xpixel, ypixel).y()) * 1e3;

                potential_difference_->Fill(std::fabs(ramo - last_ramo));
                induced_charge_histo_->Fill(initial_time_local + runge_kutta.getTime(), induced);
                induced_charge_vs_depth_histo_->Fill(initial_time_local + runge_kutta.getTime(), position.z(), induced);
                induced_charge_map_->Fill(inPixel_um_x, inPixel_um_y, induced);
                if(type == CarrierType::ELECTRON) {
                    induced_charge_e_histo_->Fill(initial_time_local + runge_kutta.getTime(), induced);
                    induced_charge_e_vs_depth_histo_->Fill(
                        initial_time_local + runge_kutta.getTime(), position.z(), induced);
                    induced_charge_e_map_->Fill(inPixel_um_x, inPixel_um_y, induced);
                } else {
                    induced_charge_h_histo_->Fill(initial_time_local + runge_kutta.getTime(), induced);
                    induced_charge_h_vs_depth_histo_->Fill(
                        initial_time_local + runge_kutta.getTime(), position.z(), induced);
                    induced_charge_h_map_->Fill(inPixel_um_x, inPixel_um_y, induced);
                }
                if(!multiplication_.is<NoImpactIonization>()) {
                    induced_charge_primary_histo_->Fill(initial_time_local + runge_kutta.getTime(), induced_primary);
                    induced_charge_secondary_histo_->Fill(initial_time_local + runge_kutta.getTime(), induced_secondary);
                    if(type == CarrierType::ELECTRON) {
                        induced_charge_primary_e_histo_->Fill(initial_time_local + runge_kutta.getTime(), induced_primary);
                        induced_charge_secondary_e_histo_->Fill(initial_time_local + runge_kutta.getTime(),
                                                                induced_secondary);
                    } else {
                        induced_charge_primary_h_histo_->Fill(initial_time_local + runge_kutta.getTime(), induced_primary);
                        induced_charge_secondary_h_histo_->Fill(initial_time_local + runge_kutta.getTime(),
                                                                induced_secondary);
                    }
                }
            }
        }
        // Increase charge at the end of the step in case of impact ionization
        charge += n_secondaries;
    }

    if(output_plots_ && !multiplication_.is<NoImpactIonization>()) {
        auto gain = charge / initial_charge;
        if(level == 0) {
            gain_primary_histo_->Fill(gain, initial_charge);
            if(type == CarrierType::ELECTRON) {
                gain_e_histo_->Fill(gain, initial_charge);
            } else {
                gain_h_histo_->Fill(gain, initial_charge);
            }
        }
        if(type == CarrierType::ELECTRON) {
            gain_e_vs_x_->Fill(pos.x(), gain);
            gain_e_vs_y_->Fill(pos.y(), gain);
            gain_e_vs_z_->Fill(pos.z(), gain);
        } else {
            gain_h_vs_x_->Fill(pos.x(), gain);
            gain_h_vs_y_->Fill(pos.y(), gain);
            gain_h_vs_z_->Fill(pos.z(), gain);
        }
        gain_all_histo_->Fill(gain, initial_charge);

        multiplication_level_histo_->Fill(level, initial_charge);
    }

    // Set final state of charge carrier for plotting:
    if(output_linegraphs_) {
        // If drift time is larger than integration time or the charge carriers have been collected at the backside, reset:
        if(runge_kutta.getTime() >= integration_time_ || last_position.z() < -model_->getSensorSize().z() * 0.45) {
            std::get<3>(output_plot_points.at(output_plot_index).first) = CarrierState::UNKNOWN;
        } else {
            std::get<3>(output_plot_points.at(output_plot_index).first) = state;
        }
    }

    // Create a new propagated charge and add it to the list
    auto local_position = static_cast<ROOT::Math::XYZPoint>(position);
    auto global_position = detector_->getGlobalPosition(local_position);
    PropagatedCharge propagated_charge(local_position,
                                       global_position,
                                       type,
                                       std::move(pixel_map),
                                       initial_time_local + runge_kutta.getTime(),
                                       initial_time_global + runge_kutta.getTime(),
                                       state,
                                       &deposit);

    LOG(DEBUG) << " Propagated " << charge << " (initial: " << initial_charge << ") to "
               << Units::display(local_position, {"mm", "um"}) << " in " << Units::display(runge_kutta.getTime(), "ns")
               << " time, induced " << Units::display(propagated_charge.getCharge(), {"e"})
               << ", final state: " << allpix::to_string(state);

    propagated_charges.push_back(std::move(propagated_charge));

    if(state == CarrierState::RECOMBINED) {
        recombined_charges_count += charge;
        if(output_plots_) {
            recombination_time_histo_->Fill(runge_kutta.getTime(), charge);
        }
    } else if(state == CarrierState::TRAPPED) {
        trapped_charges_count += charge;
    } else {
        propagated_charges_count += charge;
    }

    if(output_plots_) {
        drift_time_histo_->Fill(static_cast<double>(Units::convert(runge_kutta.getTime(), "ns")), charge);
        group_size_histo_->Fill(initial_charge);
    }

    // Return statistics counters about this and all daughter propagated charge carrier groups and their final states
    return std::make_tuple(recombined_charges_count, trapped_charges_count, propagated_charges_count);
}

std::tuple<unsigned int, unsigned int, unsigned int>
InteractivePropagationModule::propagate_together(Event* event,
                                                 std::vector<PropagatedCharge>& propagating_charges,
                                                 std::vector<PropagatedCharge>& propagated_charges,
                                                 LineGraph::OutputPlotPoints& output_plot_points) const {

    // Set up functions and variables that should stay constant throughout time:

    std::map<Pixel::Index, Pulse> pixel_map;

    unsigned int propagated_charges_count = 0;
    unsigned int recombined_charges_count = 0;
    unsigned int trapped_charges_count = 0;
    
    auto output_plot_index = output_plot_points.size() - 1;

    // Define a function to compute the diffusion
    auto carrier_diffusion = [&](double efield_mag, double doping, double timestep, allpix::CarrierType type) -> Eigen::Vector3d {
        double diffusion_constant = boltzmann_kT_ * mobility_(type, efield_mag, doping);
        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * timestep);

        // Compute the independent diffusion in three
        allpix::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
        auto x = gauss_distribution(event->getRandomEngine());
        auto y = gauss_distribution(event->getRandomEngine());
        auto z = gauss_distribution(event->getRandomEngine());
        return {x, y, z};
    };

    // Survival probability of this charge carrier package, evaluated at every step
    allpix::uniform_real_distribution<double> uniform_distribution(0, 1);

    // Define lambda functions to compute the charge carrier velocity with or without magnetic field
    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&, allpix::CarrierType type)> carrier_velocity_noB =
        [&](double, const Eigen::Vector3d& cur_pos, allpix::CarrierType type) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());

        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(cur_pos));

        return static_cast<int>(type) * mobility_(type, efield.norm(), doping) * efield;
    };

    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&, allpix::CarrierType type)> carrier_velocity_withB =
        [&](double, const Eigen::Vector3d& cur_pos, allpix::CarrierType type) -> Eigen::Vector3d {
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

    //Define a function to compute the e-field given a desired local point and the list of charges
    auto dynamic_efield = [&](ROOT::Math::XYZPoint point) -> Eigen::Vector3d {

        // for now just return nothing for the dynamic field
        return Eigen::Vector3d(0,0,0);

        ROOT::Math::XYZVector field = ROOT::Math::XYZVector(0,0,0);

        for (PropagatedCharge charge : propagating_charges){
            ROOT::Math::XYZPoint local_position = charge.getLocalPosition();
            if (local_position == point){
                continue;
            }
            const double K = 100; //Coulomb's Constant: K = 8.99 × 10 9 N ⋅ m 2 /C 2 (i need to find the correct units to set correct value here)
            auto q = charge.getCharge();
            auto dist_vector = point - local_position; // A vector between the desired points
            auto dist_mag2 = dist_vector.Mag2();
            auto dist_mag = ROOT::Math::sqrt(dist_mag2);

            field = field + dist_vector * (K * q / dist_mag2 / dist_mag);
            
        }

        Eigen::Vector3d output = Eigen::Vector3d(field.x(),field.y(),field.z());

        return output;
    };

    // Create list of RK4 objects that correspond to each particle
    std::vector<allpix::RungeKutta<double, 4, 3>> runge_kutta_vector;

    for(auto charge : propagating_charges){
        runge_kutta_vector.push_back(make_runge_kutta(
                                     tableau::RK4, 
                                     (has_magnetic_field_ ? carrier_velocity_withB : carrier_velocity_noB), 
                                     timestep_, 
                                     charge.getLocalPosition()));
    }

    // Create the runge kutta solver with an RK4 tableau, no error estimation required since we're not adapting step size
    // auto runge_kutta = make_runge_kutta(
    //     tableau::RK4, (has_magnetic_field_ ? carrier_velocity_withB : carrier_velocity_noB), timestep_, position);

    // Variables taken into the time loop:

    // Eigen::Vector3d position(pos.x(), pos.y(), pos.z());
    // // Store initial charge
    // const unsigned int initial_charge = charge;

    return std::make_tuple(0,0,0);
}