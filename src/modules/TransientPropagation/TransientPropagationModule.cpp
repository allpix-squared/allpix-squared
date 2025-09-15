/**
 * @file
 * @brief Implementation of charge propagation module with transient behavior simulation
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
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
    config_.setDefault<std::string>("detrapping_model", "none");

    config_.setDefault<double>("temperature", 293.15);
    config_.setDefault<unsigned int>("distance", 1);
    config_.setDefault<bool>("ignore_magnetic_field", false);
    config_.setDefault<double>("surface_reflectivity", 0.0);

    // Set defaults for charge carrier multiplication
    config_.setDefault<double>("multiplication_threshold", 1e-2);
    config_.setDefault<unsigned int>("max_multiplication_level", 5);
    config_.setDefault<std::string>("multiplication_model", "none");

    config_.setDefault<bool>("output_linegraphs", false);
    config_.setDefault<bool>("output_linegraphs_collected", false);
    config_.setDefault<bool>("output_linegraphs_recombined", false);
    config_.setDefault<bool>("output_linegraphs_trapped", false);
    config_.setDefault<bool>("output_animations", false);
    config_.setDefault<bool>("output_plots",
                             config_.get<bool>("output_linegraphs") || config_.get<bool>("output_animations"));
    config_.setDefault<bool>("output_animations_color_markers", false);
    config_.setDefault<double>("output_plots_step", config_.get<double>("timestep"));
    config_.setDefault<bool>("output_plots_use_pixel_units", false);
    config_.setDefault<bool>("output_plots_align_pixels", false);
    config_.setDefault<double>("output_plots_theta", 0.0f);
    config_.setDefault<double>("output_plots_phi", 0.0f);
    config_.setDefault<int>("output_max_gain_histo", 25);

    // Copy some variables from configuration to avoid lookups:
    temperature_ = config_.get<double>("temperature");
    timestep_ = config_.get<double>("timestep");
    integration_time_ = config_.get<double>("integration_time");
    distance_ = config_.get<unsigned int>("distance");
    charge_per_step_ = config_.get<unsigned int>("charge_per_step");
    max_charge_groups_ = config_.get<unsigned int>("max_charge_groups");
    boltzmann_kT_ = Units::get(8.6173333e-5, "eV/K") * temperature_;
    surface_reflectivity_ = config_.get<double>("surface_reflectivity");

    max_multiplication_level_ = config.get<unsigned int>("max_multiplication_level");

    output_plots_ = config_.get<bool>("output_plots");
    output_linegraphs_ = config_.get<bool>("output_linegraphs");
    output_linegraphs_collected_ = config_.get<bool>("output_linegraphs_collected");
    output_linegraphs_recombined_ = config_.get<bool>("output_linegraphs_recombined");
    output_linegraphs_trapped_ = config_.get<bool>("output_linegraphs_trapped");
    output_plots_step_ = config_.get<double>("output_plots_step");
    output_max_gain_histo_ = config.get<int>("output_max_gain_histo");

    // Avoids wrong gain histogram inputs
    if(output_max_gain_histo_ <2) {
        throw InvalidValueError(config, "output_max_gain_histo", "value must be >= 2");
    }

    // Enable multithreading of this module if multithreading is enabled and no per-event output plots are requested:
    // FIXME: Review if this is really the case or we can still use multithreading
    if(!(config_.get<bool>("output_animations") || output_linegraphs_)) {
        allow_multithreading();
    } else {
        LOG(WARNING) << "Per-event line graphs or animations requested, disabling parallel event processing";
    }

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
        LOG(ERROR) << "This module will likely produce unphysical results when applying linear electric fields.";
    }

    // Prepare mobility model
    mobility_ = Mobility(config_, model_->getSensorMaterial(), detector_->hasDopingProfile());

    // Prepare recombination model
    recombination_ = Recombination(config_, detector_->hasDopingProfile());

    // Prepare trapping model
    trapping_ = Trapping(config_);

    // Prepare detrapping model
    detrapping_ = Detrapping(config_);

    // Impact ionization model
    multiplication_ = ImpactIonization(config_);

    // Check multiplication and step size larger than a picosecond:
    if(!multiplication_.is<NoImpactIonization>() && timestep_ > 0.001) {
        LOG(WARNING) << "Charge multiplication enabled with maximum timestep larger than 1ps" << std::endl
                     << "This might lead to unphysical gain values.";
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

        auto pitch_x = static_cast<double>(Units::convert(model_->getPixelSize().x(), "um"));
        auto pitch_y = static_cast<double>(Units::convert(model_->getPixelSize().y(), "um"));

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
        if(!multiplication_.is<NoImpactIonization>()) {
            induced_charge_primary_histo_ =
                CreateHistogram<TH1D>("induced_charge_primary_histo",
                                      "Induced charge per time, primaries only, all pixels;Drift time [ns];charge [e]",
                                      static_cast<int>(integration_time_ / timestep_),
                                      0,
                                      static_cast<double>(Units::convert(integration_time_, "ns")));
            induced_charge_primary_e_histo_ = CreateHistogram<TH1D>(
                "induced_charge_primary_e_histo",
                "Induced charge per time, primary electrons only, all pixels;Drift time [ns];charge [e]",
                static_cast<int>(integration_time_ / timestep_),
                0,
                static_cast<double>(Units::convert(integration_time_, "ns")));
            induced_charge_primary_h_histo_ =
                CreateHistogram<TH1D>("induced_charge_primary_h_histo",
                                      "Induced charge per time, primary holes only, all pixels;Drift time [ns];charge [e]",
                                      static_cast<int>(integration_time_ / timestep_),
                                      0,
                                      static_cast<double>(Units::convert(integration_time_, "ns")));
            induced_charge_secondary_histo_ =
                CreateHistogram<TH1D>("induced_charge_secondary_histo",
                                      "Induced charge per time, secondaries only, all pixels;Drift time [ns];charge [e]",
                                      static_cast<int>(integration_time_ / timestep_),
                                      0,
                                      static_cast<double>(Units::convert(integration_time_, "ns")));
            induced_charge_secondary_e_histo_ = CreateHistogram<TH1D>(
                "induced_charge_secondary_e_histo",
                "Induced charge per time, secondary electrons only, all pixels;Drift time [ns];charge [e]",
                static_cast<int>(integration_time_ / timestep_),
                0,
                static_cast<double>(Units::convert(integration_time_, "ns")));
            induced_charge_secondary_h_histo_ =
                CreateHistogram<TH1D>("induced_charge_secondary_h_histo",
                                      "Induced charge per time, secondary holes only, all pixels;Drift time [ns];charge [e]",
                                      static_cast<int>(integration_time_ / timestep_),
                                      0,
                                      static_cast<double>(Units::convert(integration_time_, "ns")));
        }
        induced_charge_vs_depth_histo_ =
            CreateHistogram<TH2D>("induced_charge_vs_depth_histo",
                                  "Induced charge per time vs depth, all pixels;Drift time [ns];depth [mm];charge [e]",
                                  static_cast<int>(integration_time_ / timestep_),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")),
                                  100,
                                  -model_->getSensorSize().z() / 2.,
                                  model_->getSensorSize().z() / 2.);
        induced_charge_e_vs_depth_histo_ = CreateHistogram<TH2D>(
            "induced_charge_e_vs_depth_histo",
            "Induced charge per time vs depth, electrons only, all pixels;Drift time [ns];depth [mm];charge [e]",
            static_cast<int>(integration_time_ / timestep_),
            0,
            static_cast<double>(Units::convert(integration_time_, "ns")),
            100,
            -model_->getSensorSize().z() / 2.,
            model_->getSensorSize().z() / 2.);
        induced_charge_h_vs_depth_histo_ = CreateHistogram<TH2D>(
            "induced_charge_h_vs_depth_histo",
            "Induced charge per time vs depth, holes only, all pixels;Drift time [ns];depth [mm];charge [e]",
            static_cast<int>(integration_time_ / timestep_),
            0,
            static_cast<double>(Units::convert(integration_time_, "ns")),
            100,
            -model_->getSensorSize().z() / 2.,
            model_->getSensorSize().z() / 2.);
        induced_charge_map_ = CreateHistogram<TH2D>(
            "induced_charge_map",
            "Induced charge as a function of in-pixel carrier position;x%pitch [#mum];y%pitch [#mum];charge [e]",
            static_cast<int>(pitch_x),
            -pitch_x / 2,
            pitch_x / 2,
            static_cast<int>(pitch_y),
            -pitch_y / 2,
            pitch_y / 2);
        induced_charge_e_map_ = CreateHistogram<TH2D>("induced_charge_e_map",
                                                      "Induced charge as a function of in-pixel carrier position, electrons "
                                                      "only;x%pitch [#mum];y%pitch [#mum];charge [e]",
                                                      static_cast<int>(pitch_x),
                                                      -pitch_x / 2,
                                                      pitch_x / 2,
                                                      static_cast<int>(pitch_y),
                                                      -pitch_y / 2,
                                                      pitch_y / 2);
        induced_charge_h_map_ = CreateHistogram<TH2D>(
            "induced_charge_h_map",
            "Induced charge as a function of in-pixel carrier position, holes only;x%pitch [#mum];y%pitch [#mum];charge [e]",
            static_cast<int>(pitch_x),
            -pitch_x / 2,
            pitch_x / 2,
            static_cast<int>(pitch_y),
            -pitch_y / 2,
            pitch_y / 2);

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
        recombination_time_histo_ =
            CreateHistogram<TH1D>("recombination_time_histo",
                                  "Time until recombination of charge carriers;time [ns];charge carriers",
                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")));
        trapped_histo_ = CreateHistogram<TH1D>(
            "trapping_histo", "Fraction of trapped charge carriers;trapping [N / N_{total}] ;number of events", 100, 0, 1);
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
                output_max_gain_histo_ - 1,
                1,
                output_max_gain_histo_);
            gain_all_histo_ =
                CreateHistogram<TH1D>("gain_all_histo",
                                      "Gain per charge carrier group after propagation;gain;number of groups transported",
                                      output_max_gain_histo_ - 1,
                                      1,
                                      output_max_gain_histo_);
            gain_e_histo_ =
                CreateHistogram<TH1D>("gain_e_histo",
                                      "Gain per primary electron group after propagation;gain;number of groups transported",
                                      output_max_gain_histo_ - 1,
                                      1,
                                      output_max_gain_histo_);
            gain_h_histo_ =
                CreateHistogram<TH1D>("gain_h_histo",
                                      "Gain per primary hole group after propagation;gain;number of groups transported",
                                      output_max_gain_histo_ - 1,
                                      1,
                                      output_max_gain_histo_);
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
}

void TransientPropagationModule::run(Event* event) {
    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;
    unsigned int propagated_charges_count = 0;
    unsigned int recombined_charges_count = 0;
    unsigned int trapped_charges_count = 0;

    // List of points to plot to plot for output plots
    LineGraph::OutputPlotPoints output_plot_points;

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

            // Get position and propagate through sensor
            auto [recombined, trapped, propagated] = propagate(event,
                                                               deposit,
                                                               deposit.getLocalPosition(),
                                                               deposit.getType(),
                                                               charge_per_step,
                                                               deposit.getLocalTime(),
                                                               deposit.getGlobalTime(),
                                                               0,
                                                               propagated_charges,
                                                               output_plot_points);

            // Update statistics:
            recombined_charges_count += recombined;
            trapped_charges_count += trapped;
            propagated_charges_count += propagated;
            LOG(DEBUG) << "Propagated charges: " << propagated << ", recombined charges: " << recombined
                       << ", trapped charges : " << trapped;
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
std::tuple<unsigned int, unsigned int, unsigned int>
TransientPropagationModule::propagate(Event* event,
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
                     << ", limiting shower to this level (not propagating this charge carrier).";
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
            LOG(TRACE) << "Trapping charge " << charge << " at " << position.x() << "," << position.y() << ","
                       << position.z() << " and time " << runge_kutta.getTime();
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
                LOG(DEBUG) << "Set of " << n_secondaries << " charge carriers (" << inverted_type
                           << ") generated from impact ionization on " << Units::display(carrier_pos, {"mm", "um"});
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
                LOG_N(WARNING, 10) << "Detected gain of " << gain << ", local electric field of "
                                   << Units::display(std::sqrt(efield.Mag2()), "kV/cm")
                                   << ", diode seems to be in breakdown";
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
    propagated_charges_count += charge;

    if(state == CarrierState::RECOMBINED) {
        recombined_charges_count += charge;
        if(output_plots_) {
            recombination_time_histo_->Fill(runge_kutta.getTime(), charge);
        }
    } else if(state == CarrierState::TRAPPED) {
        LOG(DEBUG) << " Trapped " << charge << " at " << Units::display(local_position, {"mm", "um"}) << " in "
                   << Units::display(runge_kutta.getTime(), "ns") << " time, removing";
        trapped_charges_count += charge;
    }

    if(output_plots_) {
        drift_time_histo_->Fill(static_cast<double>(Units::convert(runge_kutta.getTime(), "ns")), charge);
        group_size_histo_->Fill(initial_charge);
    }

    // Return statistics counters about this and all daughter propagated charge carrier groups and their final states
    return std::make_tuple(recombined_charges_count, trapped_charges_count, propagated_charges_count);
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
        recombination_time_histo_->Write();
        trapped_histo_->Write();
        trapping_time_histo_->Write();
        induced_charge_histo_->Write();
        induced_charge_e_histo_->Write();
        induced_charge_h_histo_->Write();
        if(!multiplication_.is<NoImpactIonization>()) {
            induced_charge_primary_histo_->Write();
            induced_charge_primary_e_histo_->Write();
            induced_charge_primary_h_histo_->Write();
            induced_charge_secondary_histo_->Write();
            induced_charge_secondary_e_histo_->Write();
            induced_charge_secondary_h_histo_->Write();
        }
        induced_charge_vs_depth_histo_->Write();
        induced_charge_e_vs_depth_histo_->Write();
        induced_charge_h_vs_depth_histo_->Write();
        induced_charge_map_->Write();
        induced_charge_e_map_->Write();
        induced_charge_h_map_->Write();
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
}
