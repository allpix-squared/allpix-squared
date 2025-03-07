/**
 * @file
 * @brief Implementation of generic charge propagation module
 * @remarks Based on code from Paul Schuetze
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "GenericPropagationModule.hpp"

#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <Eigen/Core>

#include <Math/Point3D.h>
#include <Math/Vector3D.h>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "tools/ROOT.h"
#include "tools/runge_kutta.h"

#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

using namespace allpix;

/**
 * Besides binding the message and setting defaults for the configuration, the module copies some configuration variables to
 * local copies to speed up computation.
 */
GenericPropagationModule::GenericPropagationModule(Configuration& config,
                                                   Messenger* messenger,
                                                   std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {
    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector
    messenger_->bindSingle<DepositedChargeMessage>(this, MsgFlags::REQUIRED);

    // Set default value for config variables
    config_.setDefault<double>("spatial_precision", Units::get(0.25, "nm"));
    config_.setDefault<double>("timestep_start", Units::get(0.01, "ns"));
    config_.setDefault<double>("timestep_min", Units::get(0.001, "ns"));
    config_.setDefault<double>("timestep_max", Units::get(0.5, "ns"));
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 10);
    config_.setDefault<unsigned int>("max_charge_groups", 1000);
    config_.setDefault<double>("temperature", 293.15);

    // Models:
    config_.setDefault<std::string>("mobility_model", "jacoboni");
    config_.setDefault<std::string>("recombination_model", "none");
    config_.setDefault<std::string>("trapping_model", "none");
    config_.setDefault<std::string>("detrapping_model", "none");

    config_.setDefault<bool>("output_linegraphs", false);
    config_.setDefault<bool>("output_linegraphs_collected", false);
    config_.setDefault<bool>("output_linegraphs_recombined", false);
    config_.setDefault<bool>("output_linegraphs_trapped", false);
    config_.setDefault<bool>("output_animations", false);
    config_.setDefault<bool>("output_plots",
                             config_.get<bool>("output_linegraphs") || config_.get<bool>("output_animations"));
    config_.setDefault<bool>("output_animations_color_markers", false);
    config_.setDefault<double>("output_plots_step", config_.get<double>("timestep_max"));
    config_.setDefault<bool>("output_plots_use_pixel_units", false);
    config_.setDefault<bool>("output_plots_align_pixels", false);
    config_.setDefault<double>("output_plots_theta", 0.0f);
    config_.setDefault<double>("output_plots_phi", 0.0f);

    // Set defaults for charge carrier propagation:
    config_.setDefault<bool>("propagate_electrons", true);
    config_.setDefault<bool>("propagate_holes", false);
    if(!config_.get<bool>("propagate_electrons") && !config_.get<bool>("propagate_holes")) {
        throw InvalidValueError(
            config_,
            "propagate_electrons",
            "No charge carriers selected for propagation, enable 'propagate_electrons' or 'propagate_holes'.");
    }

    config_.setDefault<bool>("ignore_magnetic_field", false);

    // Set defaults for charge carrier multiplication
    config_.setDefault<std::string>("multiplication_model", "none");
    config_.setDefault<double>("multiplication_threshold", 1e-2);
    config_.setDefault<unsigned int>("max_multiplication_level", 5);

    // Copy some variables from configuration to avoid lookups:
    temperature_ = config_.get<double>("temperature");
    timestep_min_ = config_.get<double>("timestep_min");
    timestep_max_ = config_.get<double>("timestep_max");
    timestep_start_ = config_.get<double>("timestep_start");
    integration_time_ = config_.get<double>("integration_time");
    target_spatial_precision_ = config_.get<double>("spatial_precision");
    output_plots_ = config_.get<bool>("output_plots");
    output_linegraphs_ = config_.get<bool>("output_linegraphs");
    output_linegraphs_collected_ = config_.get<bool>("output_linegraphs_collected");
    output_linegraphs_recombined_ = config_.get<bool>("output_linegraphs_recombined");
    output_linegraphs_trapped_ = config_.get<bool>("output_linegraphs_trapped");
    output_animations_ = config_.get<bool>("output_animations");
    output_plots_step_ = config_.get<double>("output_plots_step");
    propagate_electrons_ = config_.get<bool>("propagate_electrons");
    propagate_holes_ = config_.get<bool>("propagate_holes");
    charge_per_step_ = config_.get<unsigned int>("charge_per_step");
    max_charge_groups_ = config_.get<unsigned int>("max_charge_groups");
    max_multiplication_level_ = config.get<unsigned int>("max_multiplication_level");

    // Enable multithreading of this module if multithreading is enabled and no per-event output plots are requested:
    // FIXME: Review if this is really the case or we can still use multithreading
    if(!(output_animations_ || output_linegraphs_)) {
        allow_multithreading();
    } else {
        LOG(WARNING) << "Per-event line graphs or animations requested, disabling parallel event processing";
    }

    boltzmann_kT_ = Units::get(8.6173333e-5, "eV/K") * temperature_;

    // Parameter for charge transport in magnetic field (approximated from graphs:
    // http://www.ioffe.ru/SVA/NSM/Semicond/Si/electric.html) FIXME
    electron_Hall_ = 1.15;
    hole_Hall_ = 0.9;
}

void GenericPropagationModule::initialize() {

    // Check for electric field and output warning for slow propagation if not defined
    if(!detector_->hasElectricField()) {
        LOG(WARNING) << "This detector does not have an electric field.";
    }

    // For linear fields we can in addition check if the correct carriers are propagated
    if(detector_->getElectricFieldType() == FieldType::LINEAR) {
        auto probe_point = ROOT::Math::XYZPoint(model_->getSensorCenter().x(),
                                                model_->getSensorCenter().y(),
                                                model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.01);

        // Get the field close to the implants and check its sign:
        auto efield = detector_->getElectricField(probe_point);
        auto direction = std::signbit(efield.z());
        // Compare with propagated carrier type:
        if(direction && !propagate_electrons_) {
            LOG(WARNING) << "Electric field indicates electron collection at implants, but electrons are not propagated!";
        }
        if(!direction && !propagate_holes_) {
            LOG(WARNING) << "Electric field indicates hole collection at implants, but holes are not propagated!";
        }
    }

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
        step_length_histo_ =
            CreateHistogram<TH1D>("step_length_histo",
                                  "Step length;length [#mum];integration steps",
                                  100,
                                  0,
                                  static_cast<double>(Units::convert(0.25 * model_->getSensorSize().z(), "um")));

        drift_time_histo_ = CreateHistogram<TH1D>("drift_time_histo",
                                                  "Drift time;Drift time [ns];charge carriers",
                                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                                  0,
                                                  static_cast<double>(Units::convert(integration_time_, "ns")));

        uncertainty_histo_ =
            CreateHistogram<TH1D>("uncertainty_histo",
                                  "Position uncertainty;uncertainty [nm];integration steps",
                                  100,
                                  0,
                                  static_cast<double>(4 * Units::convert(config_.get<double>("spatial_precision"), "nm")));

        group_size_histo_ = CreateHistogram<TH1D>("group_size_histo",
                                                  "Charge carrier group size;group size;number of groups transported",
                                                  static_cast<int>(100 * charge_per_step_),
                                                  0,
                                                  static_cast<int>(100 * charge_per_step_));

        recombine_histo_ =
            CreateHistogram<TH1D>("recombination_histo",
                                  "Fraction of recombined charge carriers;recombination [N / N_{total}] ;number of events",
                                  100,
                                  0,
                                  1);

        trapped_histo_ = CreateHistogram<TH1D>(
            "trapping_histo",
            "Fraction of trapped charge carriers at final state;trapping [N / N_{total}] ;number of events",
            100,
            0,
            1);

        recombination_time_histo_ =
            CreateHistogram<TH1D>("recombination_time_histo",
                                  "Time until recombination of charge carriers;time [ns];charge carriers",
                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")));
        trapping_time_histo_ = CreateHistogram<TH1D>("trapping_time_histo",
                                                     "Local time of trapping of charge carriers;time [ns];charge carriers",
                                                     static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                                     0,
                                                     static_cast<double>(Units::convert(integration_time_, "ns")));
        detrapping_time_histo_ =
            CreateHistogram<TH1D>("detrapping_time_histo",
                                  "Time from trapping until detrapping of charge carriers;time [ns];charge carriers",
                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")));
        if(!multiplication_.is<NoImpactIonization>()) {
            gain_primary_histo_ = CreateHistogram<TH1D>(
                "gain_primary_histo",
                "Gain per primarily induced charge carrier group after propagation;gain;number of groups transported",
                24,
                1,
                25);
            gain_all_histo_ =
                CreateHistogram<TH1D>("gain_all_histo",
                                      "Gain per charge carrier group after propagation;gain;number of groups transported",
                                      24,
                                      1,
                                      25);
            gain_e_histo_ =
                CreateHistogram<TH1D>("gain_e_histo",
                                      "Gain per primary electron group after propagation;gain;number of groups transported",
                                      24,
                                      1,
                                      25);
            gain_h_histo_ =
                CreateHistogram<TH1D>("gain_h_histo",
                                      "Gain per primary hole group after propagation;gain;number of groups transported",
                                      24,
                                      1,
                                      25);
            multiplication_level_histo_ = CreateHistogram<TH1D>(
                "multiplication_level_histo",
                "Multiplication level of propagated charge carriers;multiplication level;charge carriers",
                static_cast<int>(max_multiplication_level_),
                0,
                static_cast<int>(max_multiplication_level_));
            multiplication_depth_histo_ =
                CreateHistogram<TH1D>("multiplication_depth_histo",
                                      "Generation depth of charge carriers via impact ionization;depth [mm];charge carriers",
                                      200,
                                      -model_->getSensorSize().z() / 2.,
                                      model_->getSensorSize().z() / 2.);
            gain_e_vs_x_ =
                CreateHistogram<TProfile>("gain_e_vs_x",
                                          "Gain per electron group after propagation vs x; x [mm]; gain per group",
                                          100,
                                          -model_->getSensorSize().x() / 2.,
                                          model_->getSensorSize().x() / 2.);
            gain_e_vs_y_ =
                CreateHistogram<TProfile>("gain_e_vs_y",
                                          "Gain per electron group after propagation vs y; x [mm]; gain per group",
                                          100,
                                          -model_->getSensorSize().y() / 2.,
                                          model_->getSensorSize().y() / 2.);
            gain_e_vs_z_ =
                CreateHistogram<TProfile>("gain_e_vs_z",
                                          "Gain per electron group after propagation vs z; x [mm]; gain per group",
                                          100,
                                          -model_->getSensorSize().z() / 2.,
                                          model_->getSensorSize().z() / 2.);
            gain_h_vs_x_ = CreateHistogram<TProfile>("gain_h_vs_x",
                                                     "Gain per hole group after propagation vs x; x [mm]; gain per group",
                                                     100,
                                                     -model_->getSensorSize().x() / 2.,
                                                     model_->getSensorSize().x() / 2.);
            gain_h_vs_y_ = CreateHistogram<TProfile>("gain_h_vs_y",
                                                     "Gain per hole group after propagation vs y; x [mm]; gain per group",
                                                     100,
                                                     -model_->getSensorSize().y() / 2.,
                                                     model_->getSensorSize().y() / 2.);
            gain_h_vs_z_ = CreateHistogram<TProfile>("gain_h_vs_z",
                                                     "Gain per hole group after propagation vs z; x [mm]; gain per group",
                                                     100,
                                                     -model_->getSensorSize().z() / 2.,
                                                     model_->getSensorSize().z() / 2.);
        }
    }

    // Prepare mobility model
    mobility_ = Mobility(config_, model_->getSensorMaterial(), detector_->hasDopingProfile());

    // Prepare recombination model
    recombination_ = Recombination(config_, detector_->hasDopingProfile());

    // Impact ionization model
    multiplication_ = ImpactIonization(config_);

    // Check multiplication and step size larger than a picosecond:
    if(!multiplication_.is<NoImpactIonization>() && timestep_max_ > 0.001) {
        LOG(WARNING) << "Charge multiplication enabled with maximum timestep larger than 1ps" << std::endl
                     << "This might lead to unphysical gain values.";
    }

    // Check for propagating both types of charge carrier
    if(!multiplication_.is<NoImpactIonization>() && (!propagate_electrons_ || !propagate_holes_)) {
        LOG(WARNING) << "Not propagating both types of charge carriers with charge multiplication enabled may lead to "
                        "unphysical results";
    }

    // Prepare trapping model
    trapping_ = Trapping(config_);

    // Prepare trapping model
    detrapping_ = Detrapping(config_);
}

void GenericPropagationModule::run(Event* event) {
    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;

    // List of points to plot to plot for output plots
    LineGraph::OutputPlotPoints output_plot_points;

    // Loop over all deposits for propagation
    LOG(TRACE) << "Propagating charges in sensor";
    unsigned int propagated_charges_count = 0;
    unsigned int recombined_charges_count = 0;
    unsigned int trapped_charges_count = 0;
    unsigned int step_count = 0;
    long double total_time = 0;
    for(const auto& deposit : deposits_message->getData()) {

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

            // Propagate a single charge deposit
            auto [recombined, trapped, propagated, steps, time] = propagate(event,
                                                                            deposit,
                                                                            deposit.getLocalPosition(),
                                                                            deposit.getType(),
                                                                            charge_per_step,
                                                                            deposit.getLocalTime(),
                                                                            deposit.getGlobalTime(),
                                                                            0,
                                                                            propagated_charges,
                                                                            output_plot_points);

            // Update statistical information
            recombined_charges_count += recombined;
            trapped_charges_count += trapped;
            propagated_charges_count += propagated;
            step_count += steps;
            total_time += time;
        }
    }

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
        if(output_animations_) {
            LineGraph::Animate(event->number, this, config_, output_plot_points);
        }
    }

    // Write summary and update statistics
    long double average_time = total_time / std::max(1u, propagated_charges_count);
    LOG(INFO) << "Propagated " << propagated_charges_count << " charges in " << step_count << " steps in average time of "
              << Units::display(average_time, "ns") << std::endl
              << "Recombined " << recombined_charges_count << " charges during transport" << std::endl
              << "Trapped " << trapped_charges_count << " charges during transport";
    total_propagated_charges_ += propagated_charges_count;
    total_steps_ += step_count;
    total_time_picoseconds_ += static_cast<long unsigned int>(total_time * 1e3);

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
 * velocity at every point with help of the electric field map of the detector. An Runge-Kutta integration is applied in
 * multiple steps, adding a random diffusion to the propagating charge every step.
 */
std::tuple<unsigned int, unsigned int, unsigned int, unsigned int, long double>
GenericPropagationModule::propagate(Event* event,
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

    // Create a runge kutta solver using the electric field as step function
    Eigen::Vector3d position(pos.x(), pos.y(), pos.z());

    unsigned int propagated_charges_count = 0;
    unsigned int recombined_charges_count = 0;
    unsigned int trapped_charges_count = 0;
    unsigned int steps = 0;
    long double total_time = 0;

    // Add point of deposition to the output plots if requested
    if(output_linegraphs_) {
        output_plot_points.emplace_back(
            std::make_tuple(deposit.getGlobalTime(), charge, deposit.getType(), CarrierState::MOTION),
            std::vector<ROOT::Math::XYZPoint>());
    }
    auto output_plot_index = output_plot_points.size() - 1;

    // Store initial charge
    const unsigned int initial_charge = charge;

    // Define a function to compute the diffusion
    auto carrier_diffusion = [&](double efield_mag, double doping_concentration, double timestep) -> Eigen::Vector3d {
        double diffusion_constant = boltzmann_kT_ * mobility_(type, efield_mag, doping_concentration);
        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * timestep);

        // Compute the independent diffusion in three
        allpix::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
        auto x = gauss_distribution(event->getRandomEngine());
        auto y = gauss_distribution(event->getRandomEngine());
        auto z = gauss_distribution(event->getRandomEngine());
        return {x, y, z};
    };

    // Survival or detrap probability of this charge carrier package, evaluated at every step
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

    // Create the runge kutta solver with an RKF5 tableau, using different velocity calculators depending on the magnetic
    // field
    auto runge_kutta = make_runge_kutta(
        tableau::RK5, (has_magnetic_field_ ? carrier_velocity_withB : carrier_velocity_noB), timestep_start_, position);

    // Continue propagation until the deposit is outside the sensor
    Eigen::Vector3d last_position = position;
    ROOT::Math::XYZVector efield{}, last_efield{};
    double last_time = 0;
    size_t next_idx = 0;
    auto state = CarrierState::MOTION;
    while(state == CarrierState::MOTION && (initial_time_local + runge_kutta.getTime()) < integration_time_) {
        // Update output plots if necessary (depending on the plot step)
        if(output_linegraphs_) {
            auto time_idx = static_cast<size_t>(runge_kutta.getTime() / output_plots_step_);
            while(next_idx <= time_idx) {
                output_plot_points.at(output_plot_index).second.push_back(static_cast<ROOT::Math::XYZPoint>(position));
                next_idx = output_plot_points.at(output_plot_index).second.size();
            }
        }

        // Save previous position and time
        last_position = position;
        last_time = runge_kutta.getTime();
        last_efield = efield;

        // Get electric field at current (pre-step) position
        efield = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(position));
        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(position));

        // Execute a Runge Kutta step
        auto step = runge_kutta.step();

        // Get the current result and timestep
        auto timestep = runge_kutta.getTimeStep();
        position = runge_kutta.getValue();
        LOG(TRACE) << "Step from " << Units::display(static_cast<ROOT::Math::XYZPoint>(last_position), {"um"}) << " to "
                   << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um"});

        // Apply diffusion step
        auto diffusion = carrier_diffusion(std::sqrt(efield.Mag2()), doping, timestep);
        position += diffusion;
        runge_kutta.setValue(position);

        // Check if we are still in the sensor and not in an implant:
        if(!model_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position)) ||
           model_->isWithinImplant(static_cast<ROOT::Math::XYZPoint>(position))) {
            state = CarrierState::HALTED;
        }

        // Physics effects:

        // Check if charge carrier is still alive:
        if(state == CarrierState::MOTION &&
           recombination_(type,
                          detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(position)),
                          uniform_distribution(event->getRandomEngine()),
                          timestep)) {
            state = CarrierState::RECOMBINED;
        }

        // Check if the charge carrier has been trapped:
        if(state == CarrierState::MOTION &&
           trapping_(type, uniform_distribution(event->getRandomEngine()), timestep, std::sqrt(efield.Mag2()))) {
            if(output_plots_) {
                trapping_time_histo_->Fill(static_cast<double>(Units::convert(runge_kutta.getTime(), "ns")), charge);
            }

            auto detrap_time = detrapping_(type, uniform_distribution(event->getRandomEngine()), std::sqrt(efield.Mag2()));
            if((initial_time_local + runge_kutta.getTime() + detrap_time) < integration_time_) {
                LOG(DEBUG) << "De-trapping charge carrier after " << Units::display(detrap_time, {"ns", "us"});
                // De-trap and advance in time if still below integration time
                runge_kutta.advanceTime(detrap_time);

                if(output_plots_) {
                    detrapping_time_histo_->Fill(static_cast<double>(Units::convert(detrap_time, "ns")), charge);
                }
            } else {
                // Mark as trapped otherwise
                state = CarrierState::TRAPPED;
            }
        }

        LOG(TRACE) << "Step from " << Units::display(static_cast<ROOT::Math::XYZPoint>(last_position), {"um", "mm"})
                   << " to " << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um", "mm"}) << " at "
                   << Units::display(initial_time_local + runge_kutta.getTime(), {"ps", "ns", "us"})
                   << ", state: " << allpix::to_string(state);

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

            auto inverted_type = invertCarrierType(type);
            auto do_propagate = [&]() {
                return (inverted_type == CarrierType::ELECTRON && propagate_electrons_) ||
                       (inverted_type == CarrierType::HOLE && propagate_holes_);
            };

            if(n_secondaries > 0 && do_propagate()) {
                // Generate new charge carriers of the opposite type
                // Same-type charge carriers are generated by increasing the charge at the end of the step
                // Placing new charge carrier at the end of the step:
                auto carrier_pos = static_cast<ROOT::Math::XYZPoint>(position);
                LOG(DEBUG) << "Set of charge carriers (" << inverted_type << ") generated from impact ionization on "
                           << Units::display(carrier_pos, {"mm", "um"});
                if(output_plots_) {
                    multiplication_depth_histo_->Fill(carrier_pos.z(), n_secondaries);
                }

                auto [recombined, trapped, propagated, psteps, ptime] =
                    propagate(event,
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
                steps += psteps;
                total_time += ptime * charge;

                LOG(DEBUG) << "Continuing propagation of charge carrier set (" << type << ") at "
                           << Units::display(carrier_pos, {"mm", "um"});
            }

            auto gain = static_cast<double>(charge + n_secondaries) / initial_charge;
            if(gain > 50.) {
                LOG(WARNING) << "Detected gain of " << gain << ", local electric field of "
                             << Units::display(std::sqrt(efield.Mag2()), "kV/cm") << ", diode seems to be in breakdown";
            }
        }

        // Update step length histogram
        if(output_plots_) {
            step_length_histo_->Fill(static_cast<double>(Units::convert(step.value.norm(), "um")));
            uncertainty_histo_->Fill(static_cast<double>(Units::convert(step.error.norm(), "nm")));
        }

        // Adapt step size to match target precision
        double uncertainty = step.error.norm();

        // Lower timestep when reaching the sensor edge
        if(std::fabs(model_->getSensorSize().z() / 2.0 - position.z()) < 2 * step.value.z()) {
            timestep *= 0.75;
        } else {
            if(uncertainty > target_spatial_precision_) {
                timestep *= 0.75;
            } else if(2 * uncertainty < target_spatial_precision_) {
                timestep *= 1.5;
            }
        }
        // Limit the timestep to certain minimum and maximum step sizes
        if(timestep > timestep_max_) {
            timestep = timestep_max_;
        } else if(timestep < timestep_min_) {
            timestep = timestep_min_;
        }
        runge_kutta.setTimeStep(timestep);

        charge += n_secondaries;
    }

    // Find proper final position in the sensor
    auto time = runge_kutta.getTime();
    if(state == CarrierState::HALTED && !model_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position))) {
        auto intercept = model_->getSensorIntercept(static_cast<ROOT::Math::XYZPoint>(last_position),
                                                    static_cast<ROOT::Math::XYZPoint>(position));
        position = Eigen::Vector3d(intercept.x(), intercept.y(), intercept.z());
    }

    // Set final state of charge carrier for plotting:
    if(output_linegraphs_) {
        // If drift time is larger than integration time or the charge carriers have been collected at the backside, reset:
        if(!model_->isWithinImplant(static_cast<ROOT::Math::XYZPoint>(position)) &&
           (time >= integration_time_ || last_position.z() < -model_->getSensorSize().z() * 0.45)) {
            std::get<3>(output_plot_points.at(output_plot_index).first) = CarrierState::UNKNOWN;
        } else {
            std::get<3>(output_plot_points.at(output_plot_index).first) = state;
        }
    }

    auto gain = charge / initial_charge;
    if(output_plots_ && !multiplication_.is<NoImpactIonization>()) {
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

    if(state == CarrierState::RECOMBINED) {
        LOG(DEBUG) << "Charge carrier recombined after " << Units::display(last_time, {"ns"});
    } else if(state == CarrierState::TRAPPED) {
        LOG(DEBUG) << "Charge carrier trapped after " << Units::display(last_time, {"ns"}) << " at "
                   << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um", "mm"});
    }

    auto local_position = static_cast<ROOT::Math::XYZPoint>(position);

    if(state == CarrierState::RECOMBINED) {
        LOG(DEBUG) << " Recombined " << charge << " at " << Units::display(local_position, {"mm", "um"}) << " in "
                   << Units::display(time, "ns") << " time, removing";
        recombined_charges_count += charge;
        if(output_plots_) {
            recombination_time_histo_->Fill(static_cast<double>(Units::convert(time, "ns")), charge);
        }
    } else if(state == CarrierState::TRAPPED) {
        LOG(DEBUG) << " Trapped " << charge << " at " << Units::display(local_position, {"mm", "um"}) << " in "
                   << Units::display(time, "ns") << " time, removing";
        trapped_charges_count += charge;
    }
    propagated_charges_count += charge;
    ++steps;
    total_time += time * charge;

    LOG(DEBUG) << " Propagated " << charge << " to " << Units::display(local_position, {"mm", "um"}) << " in "
               << Units::display(time, "ns") << " time, gain " << gain << ", final state: " << allpix::to_string(state);

    // Create a new propagated charge and add it to the list
    auto global_position = detector_->getGlobalPosition(local_position);
    PropagatedCharge propagated_charge(local_position,
                                       global_position,
                                       deposit.getType(),
                                       charge,
                                       deposit.getLocalTime() + time,
                                       deposit.getGlobalTime() + time,
                                       state,
                                       &deposit);

    propagated_charges.push_back(std::move(propagated_charge));

    if(output_plots_) {
        drift_time_histo_->Fill(static_cast<double>(Units::convert(time, "ns")), charge);
        group_size_histo_->Fill(charge);
    }

    // Return statistics counters about this and all daughter propagated charge carrier groups and their final states
    return std::make_tuple(recombined_charges_count, trapped_charges_count, propagated_charges_count, steps, total_time);
}

void GenericPropagationModule::finalize() {
    if(output_plots_) {
        group_size_histo_->Get()->GetXaxis()->SetRange(1, group_size_histo_->Get()->GetNbinsX() + 1);

        step_length_histo_->Write();
        drift_time_histo_->Write();
        uncertainty_histo_->Write();
        group_size_histo_->Write();
        recombine_histo_->Write();
        trapped_histo_->Write();
        recombination_time_histo_->Write();
        trapping_time_histo_->Write();
        detrapping_time_histo_->Write();
        if(!multiplication_.is<NoImpactIonization>()) {
            gain_primary_histo_->Write();
            gain_all_histo_->Write();
            gain_e_histo_->Write();
            gain_h_histo_->Write();
            multiplication_level_histo_->Write();
            multiplication_depth_histo_->Write();
            gain_e_vs_x_->Write();
            gain_e_vs_y_->Write();
            gain_e_vs_z_->Write();
            gain_h_vs_x_->Write();
            gain_h_vs_y_->Write();
            gain_h_vs_z_->Write();
        }
    }

    long double average_time = static_cast<long double>(total_time_picoseconds_) / 1e3 /
                               std::max(1u, static_cast<unsigned int>(total_propagated_charges_));
    LOG(INFO) << "Propagated total of " << total_propagated_charges_ << " charges in " << total_steps_
              << " steps in average time of " << Units::display(average_time, "ns");
    LOG(INFO) << deposits_exceeding_max_groups_ * 100.0 / total_deposits_ << "% of deposits have charge exceeding the "
              << max_charge_groups_ << " charge groups allowed, with a charge_per_step value of " << charge_per_step_ << ".";
}
